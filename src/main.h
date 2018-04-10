#ifndef MAIN_H_
#define MAIN_H_


// Standard includes
#include <stddef.h>
#include <stdio.h>
#include <string.h>

// Scheduler includes
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "FreeRTOS/timers.h"


#include "system.h"
#include "io.h"

#include "sys/alt_irq.h"

//NIOS2
#include "sys/alt_alarm.h"

#include <altera_avalon_pio_regs.h>

unsigned int global_unstableFlag;

unsigned int unstableTimerFlag;
unsigned int stableTimerFlag;

unsigned int stable_timer_running;
unsigned int unstable_timer_running;

unsigned int unstableFlag; //stable = 0, unstable = 1
unsigned int isMonitoring;





//TIMERS
TimerHandle_t unstableTimer500;
TimerHandle_t stableTimer500;

QueueHandle_t xShedLoadStatusQueue;


void ControlCentre(void *pvParameters);
void ManageLoad(void *pvParameters);
void LEDController(void *pvParameters);
void initTimers(void);

#endif /* MAIN_H_ */
