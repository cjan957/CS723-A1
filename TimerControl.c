
#include "main.h"
#include "TimerControl.h"

void TimerControl(void *pvParameters)
{
	while(1)
	{
		if(isMonitoring)
		{
			//printf("in timer control");
			if(global_unstableFlag == 1 && unstable_timer_running == 0)
			{
				printf("first triggered \n");
				//start timers
				if(xTimerStart(unstableTimer500, 0) != pdPASS)
				{
					printf("cannot start UNstable timer");
				}
				if(xTimerStop(stableTimer500, 0) != pdPASS)
				{
					printf("cannot stop stable timer");
				}
				stable_timer_running = 0;
				unstable_timer_running = 1;

				//or use semaphore
				unstableTimerFlag = 0;
				stableTimerFlag = 0;
			}
			else if( (global_unstableFlag == 0 && unstable_timer_running == 1))
			{
				//start timers
				printf("second triggered \n");

				if(xTimerStart(stableTimer500, 0) != pdPASS)
				{
					printf("cannot start stable timer");
				}
				if(xTimerStop(unstableTimer500, 0) != pdPASS)
				{
					printf("cannot stop UNstable timer");
				}

				stable_timer_running = 1;
				unstable_timer_running = 0;

				//or use semaphore
				unstableTimerFlag = 0;
				stableTimerFlag = 0;
			}
		}
		vTaskDelay(10);
	}
}





