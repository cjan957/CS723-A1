#include "ConditionChecking.h"
#include "main.h"

unsigned int condition1_freqencyThreshold = 50;
unsigned int condition2_freqencyThreshold = 50;

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
	int shedInst = 1;

	while(1)
	{
		while(uxQueueMessagesWaiting( xFreqQueue ) != 0){

			xQueueReceive( xFreqQueue, (void *) &freqValue, 0 );

			freq[i] = freqValue;

//			TODO: is this ok?
			calculateROC();

			lcd = fopen(CHARACTER_LCD_NAME, "w");
			fprintf(lcd, "%c%s", ESC, CLEAR_LCD_STRING);
			fprintf(lcd, "Freq: %f \n isManaging: %d ", freqValue, isMonitoring);
			fclose(lcd);

			//condition 1 checking ONLY
			if(freqValue < condition1_freqencyThreshold)
			{
				//UNSTABLE

				if(!isMonitoring)
				{
					//start monitoring, initially unstable (first time)
					taskENTER_CRITICAL();
					isMonitoring = 1;
					taskEXIT_CRITICAL();

					if(xQueueSend(xInstructionQueue, &shedInst, 9999 ) != pdPASS)
					{
						printf("Failed to instrct to shed for the first time (frm condi checking)");
					}
					if(xTimerStart(xTimer500, 9999) != pdPASS)
					{
						printf("cannot start a timer");
					}
				}
				//indicates that system is unstable
				global_unstableFlag = 1;
			}
			else
			{
				global_unstableFlag = 0;
			}
		}
		//test();
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




