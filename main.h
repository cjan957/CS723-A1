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

#include "system.h"
#include "io.h"

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

alt_alarm unstableTimer500;
alt_alarm stableTimer500;

alt_u32 unstableTimer_isr_function(void *context);
alt_u32 stableTimer_isr_function(void *context);


#endif /* MAIN_H_ */
