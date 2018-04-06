#include "ConditionChecking.h"
#include "main.h"

unsigned int condition1_freqencyThreshold = 50;
unsigned int condition2_freqencyThreshold = 50;

//unsigned int global_unstableFlag = 0;

double freqValue;
unsigned int unstable_status = 1;
unsigned int stable_status = 0;

void ConditionChecking(void *pvParameters)
{
	while(1)
	{
		if(uxQueueMessagesWaiting( xFreqQueue ) != 0){

			xQueueReceive( xFreqQueue, (void *) &freqValue, 0 );
			taskENTER_CRITICAL();
			printf("Frequency: %fHz\n", freqValue);
			taskEXIT_CRITICAL();

			//condition 1 checking ONLY
			if(freqValue < condition1_freqencyThreshold)
			{
				xQueueSend(xStatusQueue, &unstable_status, 10);
				global_unstableFlag = 1;
			}
			else
			{
				xQueueSend(xStatusQueue, &stable_status, 10);
				global_unstableFlag = 0;
			}
		}
		vTaskDelay(10);
	}
}




