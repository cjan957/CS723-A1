
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
				alt_alarm_start(&unstableTimer500, 500, unstableTimer_isr_function, NULL);
				alt_alarm_stop(&stableTimer500);
				stable_timer_running = 0;
				unstable_timer_running = 1;

				unstableTimerFlag = 0;
				stableTimerFlag = 0;
			}
			else if( (global_unstableFlag == 0 && unstable_timer_running == 1))
			{
				//start timers
				printf("second triggered \n");
				alt_alarm_start(&stableTimer500, 500, stableTimer_isr_function, NULL);
				alt_alarm_stop(&unstableTimer500);
				stable_timer_running = 1;
				unstable_timer_running = 0;

				unstableTimerFlag = 0;
				stableTimerFlag = 0;
			}
		}
		vTaskDelay(10);
	}
}





