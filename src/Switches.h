/*
 * Switches.h
 *
 *  Created on: 7/04/2018
 *      Author: cjan957
 */

#ifndef SWITCHES_H_
#define SWITCHES_H_

#include "main.h"

QueueHandle_t xSwitchPositionQueue;
void SwitchRead (void *pvParameters);

#endif /* SWITCHES_H_ */
