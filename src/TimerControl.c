
#include "main.h"
#include "TimerControl.h"


// Restarts the timer when there is a status change
void TimerControl(void *pvParameters)
{

	int previousStatus = 3;

	while(1)
	{
		//this task should't be scheduled until isMonitoring is true, refer to control center
		if(isMonitoring)
		{
			if(global_unstableFlag != previousStatus)
			{
				//printf("status changed (from timercontrol) \n");
				if(xTimerReset(xTimer500, 9999) != pdPASS)
				{
					printf("cannot start 500 timer, current status is now: %d prev was: %d", global_unstableFlag, previousStatus);
				}
				else
				{
					//timer restart successfully
					previousStatus = global_unstableFlag;
				}
			}
		}
		vTaskDelay(10);
	}
}





