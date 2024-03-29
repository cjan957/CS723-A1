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


unsigned int stable_timer_running;
unsigned int unstable_timer_running;

unsigned int unstableFlag; //stable = 0, unstable = 1
unsigned int isMonitoring;

unsigned int _currentSwitchValue;
unsigned int _timerSinceStartup;

unsigned int _maintenanceMode;

unsigned int _hasNewTimeDiff;

//TIMERS
TimerHandle_t xTimer500;
TimerHandle_t xSystemTime;
TimerHandle_t xTimeDiff;

QueueHandle_t xShedLoadStatusQueue;
QueueHandle_t xInstructionQueue;

SemaphoreHandle_t xTimer500Semaphore;
SemaphoreHandle_t xSwitchSemaphore;
SemaphoreHandle_t xButtonSemaphore;



FILE* lcd;

#define ESC 27
#define CLEAR_LCD_STRING "[2J"

void ControlCentre(void *pvParameters);
void ManageLoad(void *pvParameters);
void LEDController(void *pvParameters);
void initTimers(void);
void initSemaphores(void);
void initFlags(void);
int setupKeyboard(void);

#endif /* MAIN_H_ */
