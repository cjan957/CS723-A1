#ifndef MEASUREMENTTASK_H_
#define MEASUREMENTTASK_H_

#include "main.h"

void MeasurementTask(void *pvParameters);
void getMax();
void getMin();
void getAvg();
void getLastFive();

typedef struct {
	unsigned int max;
	unsigned int min;
	double avg;
	unsigned int lastFive[5];

} Measure;

QueueHandle_t xMeasurementQueue;

SemaphoreHandle_t xHasNewTimeDiff;

// System time
unsigned int _systemTime;
unsigned int _timeDiff;

unsigned int actualTimeDifference;


#endif /* MEASUREMENTTASK_H_ */
