#include "LEDController.h"
#include "Switches.h"
#include "main.h"

void LEDController(void *pvParameters)
{
	unsigned int shedLoadStatus[5] = {0,0,0,0,0};
	unsigned int new_switchStatus[5] = {0,0,0,0,0}; //new
	unsigned int prev_switchStatus[5] = {0,0,0,0,0};

	unsigned int swOnWhenMonitoring[5] = {0,0,0,0,0};
	unsigned int swOnMaskingValue = 0;

	int i;
	unsigned int multiplier = 1;

	unsigned int shedStatusBinary = 0;
	unsigned int switchStatusBinary = 0;

	unsigned int tempShedStatusBinary = 0;

	unsigned int masking[5] = {1,2,4,8,16};

	int redLEDs = _currentSwitchValue;

	while(1)
	{
		if(!_maintenanceMode)
		{
			xSemaphoreTake(xSwitchSemaphore, 10);
			//when there's a new switch value.....
			if((uxQueueMessagesWaiting(xSwitchPositionQueue) != 0) && (xQueueReceive(xSwitchPositionQueue, &switchStatusBinary, 10) == pdTRUE))
			{
				printf("switch values updated in LEDController! \n");

				for(i = 0; i < 5; i++)
				{
					if((switchStatusBinary & masking[i]) != 0)
					{
						new_switchStatus[i] = 1;

					}
					else
					{
						new_switchStatus[i] = 0;
					}

					if(new_switchStatus[i] == 0)
					{
						shedLoadStatus[i] = 0; //set this load as 'not shed' since it has been turned off
					}
				}


				//Check if any switch is turned on when the relay is monitoring the load
				if(isMonitoring)
				{
					for(i = 0; i < 5; i++)
					{
						if((new_switchStatus[i] == 1) && (prev_switchStatus[i] == 0))
						{
							//swOnWhenMonitoring[i] = 1;
							swOnMaskingValue += masking[i];
						}
					}
				}


				//copy the new switch values to the prev Switch array
				for(i = 0; i < 5; i++)
				{
					prev_switchStatus[i] = new_switchStatus[i];
				}



				shedStatusBinary = 0;
				multiplier = 1;

				for(i = 0; i < 5; i++)
				{
					shedStatusBinary += shedLoadStatus[i] * multiplier;
					multiplier *= 2;
				}

			}
			xSemaphoreGive(xSwitchSemaphore);

			if((uxQueueMessagesWaiting(xShedLoadStatusQueue) != 0) && (xQueueReceive(xShedLoadStatusQueue, &shedLoadStatus, 10) == pdTRUE))
			{

				shedStatusBinary = 0;
				multiplier = 1;

				for(i = 0; i < 5; i++)
				{
					shedStatusBinary += shedLoadStatus[i] * multiplier;
					multiplier *= 2;
				}

				printf("Shed Status is %d, %d, %d, %d, %d \n", shedLoadStatus[4],shedLoadStatus[3],shedLoadStatus[2],shedLoadStatus[1],shedLoadStatus[0]);

			}

			if(!isMonitoring)
			{
				swOnMaskingValue = 0;
			}

			redLEDs = ~(shedStatusBinary & 0x1F);
			redLEDs = redLEDs & switchStatusBinary;
			redLEDs = redLEDs & 0x1F; //ignore all switches other than 0-4 switches
			redLEDs = redLEDs & ~swOnMaskingValue;

			IOWR_ALTERA_AVALON_PIO_DATA(RED_LEDS_BASE, redLEDs);
			IOWR_ALTERA_AVALON_PIO_DATA(GREEN_LEDS_BASE, shedStatusBinary);

		}
		else
		{	//under maintenance mode

			IOWR_ALTERA_AVALON_PIO_DATA(RED_LEDS_BASE, _currentSwitchValue);
			IOWR_ALTERA_AVALON_PIO_DATA(GREEN_LEDS_BASE, 0x0);


			//replace currentSwitch array to be what the actual switch value is
			int currentSwitch = _currentSwitchValue;

			for(i = 0; i < 5; i++)
			{
				if((currentSwitch & masking[i]) != 0)
				{
					new_switchStatus[i] = 1;
				}
				else
				{
					new_switchStatus[i] = 0;
				}
				shedLoadStatus[i] = 0;
			}

			shedStatusBinary = 0;
		}
		vTaskDelay(10);
	}

}



