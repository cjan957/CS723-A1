#ifndef CONDITIONCHECKING_H_
#define CONDITIONCHECKING_H_

#include "freq_relay.h"

QueueHandle_t xStatusQueue;
QueueHandle_t xROCQueue;

void ConditionChecking(void *pvParameters);
void calculateROC();

#endif /* CONDITIONCHECKING_H_ */
