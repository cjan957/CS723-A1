#include "ConditionChecking.h"
#include "main.h"
#include "math.h"
#include "MeasurementTask.h"

double freqValue;
double ROC;

unsigned int unstable_status = 1;
unsigned int stable_status = 0;

double freq[100], dfreq[100];
int i = 99, j = 0;

// Checks the condition of the instantaneous frequency values and the rate of change values
// and determines whether the system is stable or unstable by setting a global flag to be true
void ConditionChecking(void *pvParameters)
{

	condition1_freqencyThreshold = 49;
	condition2_freqencyThreshold = 10;
	int shedInst = 1;

	int stopLCDReWriting = 0;

	while(1)
	{
		if( xSemaphoreTake( xConditionSemaphore,10) == pdTRUE )
		{
			stopLCDReWriting = 0;
			while(uxQueueMessagesWaiting( xFreqQueue ) != 0){

				xQueueReceive( xFreqQueue, (void *) &freqValue, portMAX_DELAY );

				freq[i] = freqValue;

				calculateROC();

				if(_maintenanceMode)
				{
					if(!stopLCDReWriting)
					{
						lcd = fopen(CHARACTER_LCD_NAME, "w");
						fprintf(lcd, "%c%s", ESC, CLEAR_LCD_STRING);
						fprintf(lcd, "Maintenance");
						fclose(lcd);
						stopLCDReWriting = 1;
					}
				} else
				{

					lcd = fopen(CHARACTER_LCD_NAME, "w");
					fprintf(lcd, "%c%s", ESC, CLEAR_LCD_STRING);
					fprintf(lcd, "Freq: %f \n isMonitoring: %d ", freqValue, isMonitoring);
					fclose(lcd);
				}

				// Checks whether the instantaneous is below the threshold or above the ROC threshold
				if( (freqValue < condition1_freqencyThreshold) || (fabs(dfreq[i]) > condition2_freqencyThreshold))
				{

					if(!_maintenanceMode) {
						//UNSTABLE

						// This is when it's unstable for the first time
						if(!isMonitoring && _currentSwitchValue != 0)
						{
							// Start monitoring, initially unstable (first time)
							if(xQueueSend(xInstructionQueue, &shedInst, 9999 ) != pdPASS)
							{
								printf("Failed to instrct to shed for the first time (frm condi checking)\n");
							}

							// Starts the 500 ms timer
							if(xTimerStart(xTimer500, 9999) != pdPASS)
							{
								printf("cannot start a timer");
							}

							// Start the timer for the time difference from the voltage peak to the first load shed
							if(xTimerStart(xTimeDiff,portMAX_DELAY) != pdPASS) {
								printf("cannot start a timer (timer that goes up by 1ms) for checking time from freq to first shed !\n");
							}
						}

						// Indicates that system is unstable
						global_unstableFlag = 1;
					}

				}
				else
				{
					global_unstableFlag = 0;

					// Mutex guarding
					taskENTER_CRITICAL();
					_timeDiff = 0;
					taskEXIT_CRITICAL();

					xTimerStop(xTimeDiff,1);
				}
			}
		}


	}
}

void calculateROC()
{

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




