#ifndef CONDITIONCHECKING_H_
#define CONDITIONCHECKING_H_

#include "freq_relay.h"

double condition1_freqencyThreshold;
double condition2_freqencyThreshold;

QueueHandle_t xStatusQueue;
QueueHandle_t xROCQueue;

void ConditionChecking(void *pvParameters);
void calculateROC();

#endif /* CONDITIONCHECKING_H_ */
