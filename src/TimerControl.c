
#include "main.h"
#include "TimerControl.h"

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


			/*
			//printf("in timer control");
			if(global_unstableFlag == 1 && unstable_timer_running == 0)
			{
				printf("first triggered \n");
				//start timers
				if(xTimerStart(unstableTimer500, 10) != pdPASS)
				{
					printf("cannot start UNstable timer");
				}
				if(xTimerStop(stableTimer500, 10) != pdPASS)
				{
					printf("cannot stop stable timer");
				}
				stable_timer_running = 0;
				unstable_timer_running = 1;

			}
			else if( (global_unstableFlag == 0 && unstable_timer_running == 1))
			{
				//start timers
				printf("second triggered \n");

				if(xTimerStart(stableTimer500, 10) != pdPASS)
				{
					printf("cannot start stable timer");
				}
				if(xTimerStop(unstableTimer500, 10) != pdPASS)
				{
					printf("cannot stop UNstable timer");
				}

				stable_timer_running = 1;
				unstable_timer_running = 0;

			}
			*/
		}
		vTaskDelay(10);
	}
}





