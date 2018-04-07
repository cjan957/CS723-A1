/*
 * freq_relay.h
 *
 *  Created on: 6/04/2018
 *      Author: jasi433
 */

#ifndef FREQ_RELAY_H_
#define FREQ_RELAY_H_

// Scheduler includes
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"

#include <altera_avalon_pio_regs.h>

QueueHandle_t xFreqQueue;
QueueHandle_t xDispFreqQueue;

void freq_relay();



#endif /* FREQ_RELAY_H_ */
