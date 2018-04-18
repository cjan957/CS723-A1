#include "MeasurementTask.h"

unsigned int count = 1;
double average;
unsigned int max;
unsigned int min;

double accumulated;

Measure measure;


// Calculates the maximum, minimum, average, and the last five results
// but only runs when there is a new time difference value
void MeasurementTask(void *pvParameters) {

	average = (double) actualTimeDifference;;
	max = actualTimeDifference;
	min = actualTimeDifference;

	measure.avg = (double) actualTimeDifference;
	measure.max = actualTimeDifference;
	measure.min = actualTimeDifference;

	accumulated = 0;

	while(1) {

		if(xSemaphoreTake(xHasNewTimeDiff, portMAX_DELAY)) // When first load is shed (LED off)
		{
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

}

void getLastFive() {

	unsigned int resultIndex = 4;

	// Shifts the results along
	for (resultIndex = 4; resultIndex > 0; resultIndex--) {
		measure.lastFive[resultIndex] = measure.lastFive[resultIndex - 1];
	}

	// Stores the latest value
	measure.lastFive[0] = actualTimeDifference;

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

	// Calculates the average for all the values rather than the last five results
	accumulated += (double) actualTimeDifference;
	average = accumulated / count;
	measure.avg = average;
	count++;

}


