#include "MeasurementTask.h"

unsigned int count = 1;
double average;
unsigned int max;
unsigned int min;

double accumulated;

Measure measure;

void MeasurementTask(void *pvParameters) {

	average = (double) actualTimeDifference;;
	max = actualTimeDifference;
	min = actualTimeDifference;

	measure.avg = (double) actualTimeDifference;
	measure.max = actualTimeDifference;
	measure.min = actualTimeDifference;

	accumulated = 0;

	while(1) {
		if (global_unstableFlag) {

			if (_hasNewTimeDiff) { //when first load is shed (LED off)
				getMax();
				getMin();
				getAvg();
				getLastFive();

				if(xQueueSend(xMeasurementQueue, &measure, 0) != pdPASS)
				{
					printf("missing data! \n");
				}
			}
		}
		vTaskDelay(1);
	}

}

void getLastFive() {

	unsigned int resultIndex = 4;
	if (_hasNewTimeDiff) {



		for (resultIndex = 4; resultIndex > 0; resultIndex--) {
			measure.lastFive[resultIndex] = measure.lastFive[resultIndex - 1];
		}

		measure.lastFive[0] = actualTimeDifference;

		taskENTER_CRITICAL();
		_hasNewTimeDiff = 0;
		taskEXIT_CRITICAL();

	}

}

void getMax() {


	if (actualTimeDifference > max) {
		max = actualTimeDifference;
	}

	measure.max = max;


}
void getMin() {

	if (actualTimeDifference < min) {
		min = actualTimeDifference;
	}
	measure.min = min;

}

void getAvg() {

	accumulated += (double) actualTimeDifference;
	average = accumulated / count;
	measure.avg = average;
	count++;



}


