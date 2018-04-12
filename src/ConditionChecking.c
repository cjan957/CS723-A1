#include "ConditionChecking.h"
#include "main.h"

double freqValue;
double ROC;

unsigned int unstable_status = 1;
unsigned int stable_status = 0;

double freq[100], dfreq[100];
int i = 99, j = 0;

void test() {

	xQueueReceive( xROCQueue, (void *) &ROC, 0 );

//	taskENTER_CRITICAL();
//	printf("Value: %f\n", ROC);
//	taskEXIT_CRITICAL();
}


void ConditionChecking(void *pvParameters)
{

	condition1_freqencyThreshold = 50;
	condition2_freqencyThreshold = 7;

	while(1)
	{
		while(uxQueueMessagesWaiting( xFreqQueue ) != 0){

			xQueueReceive( xFreqQueue, (void *) &freqValue, 0 );

			freq[i] = freqValue;

			calculateROC();

			//condition 1 checking ONLY
			if( (freqValue < condition1_freqencyThreshold) || (dfreq[i] < condition1_freqencyThreshold))
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

void calculateROC() {

	//calculate frequency RoC

	if(i==0){
		dfreq[0] = (freq[0]-freq[99]) * 2.0 * freq[0] * freq[99] / (freq[0]+freq[99]);
	}
	else{
		dfreq[i] = (freq[i]-freq[i-1]) * 2.0 * freq[i]* freq[i-1] / (freq[i]+freq[i-1]);
	}

	if (dfreq[i] > 100.0){
		dfreq[i] = 100.0;
	}

	xQueueSend(xROCQueue, &dfreq[i], 0);


	i =	++i%100;

}




