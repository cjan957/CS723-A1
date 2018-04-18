#include "main.h"
#include "Switches.h"

// Polls for the switch values and sends data to the queue only when there is a change in the switch values
void SwitchRead (void *pvParameters)
{
	unsigned int currentSwitchValue;
	unsigned int previousSwitchValue = 999999;
	while(1)
	{
		//Read switch from IO // Polling
		currentSwitchValue = IORD_ALTERA_AVALON_PIO_DATA(SLIDE_SWITCH_BASE);
		currentSwitchValue = currentSwitchValue & 0x1F; //masking
		_currentSwitchValue = currentSwitchValue; // Updates the global switch value

		// Should not be monitoring when no loads are connected
		if(currentSwitchValue == 0)
		{
			taskENTER_CRITICAL();
			isMonitoring = 0;
			taskEXIT_CRITICAL();
		}

		//Push current switch position to SwitchQueue
		if(currentSwitchValue != previousSwitchValue)
		{
			if(xQueueSend(xSwitchPositionQueue, &currentSwitchValue, 10) == pdFALSE)
			{
				printf("failed to pushed switch to queue \n");
			}
			else{
				printf("PUSHED value: %d \n", currentSwitchValue);
				previousSwitchValue = currentSwitchValue;
			}
		}
		vTaskDelay(250);
	}
}



