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

// Definition of Task Stacks
#define   TASK_STACKSIZE       2048

// Definition of Task Priorities
#define PRINT_STATUS_TASK_PRIORITY 14
#define GETSEM_TASK1_PRIORITY      13
#define GETSEM_TASK2_PRIORITY      12
#define RECEIVE_TASK1_PRIORITY    11
#define RECEIVE_TASK2_PRIORITY    10
#define SEND_TASK_PRIORITY        9

// Definition of Message Queue
#define   MSG_QUEUE_SIZE  30
QueueHandle_t msgqueue;

QueueHandle_t xFreqQueue;
QueueHandle_t xStatusQueue;
QueueHandle_t xInstructionQueue;

// used to delete a task
TaskHandle_t xHandle;

// Definition of Semaphore
SemaphoreHandle_t shared_resource_sem;

// globals variables
unsigned int number_of_messages_sent = 0;
unsigned int number_of_messages_received_task1 = 0;
unsigned int number_of_messages_received_task2 = 0;
unsigned int getsem_task1_got_sem = 0;
unsigned int getsem_task2_got_sem = 0;
char sem_owner_task_name[20];

double currentFrequency = 0.0;
unsigned int unstableFlag = 0; //stable = 0, unstable = 1
unsigned int isMonitoring = 0;

unsigned int condition1_freqencyThreshold = 50;
unsigned int condition2_freqencyThreshold = 50;

//semaphore
SemaphoreHandle_t shedSemaphore;
SemaphoreHandle_t connectSemaphore;

//timers
alt_alarm unstableTimer500;
alt_alarm stableTimer500;

//timer flags
unsigned int unstableTimerFlag = 0;
unsigned int stableTimerFlag = 0;

unsigned int stable_timer_running = 0;
unsigned int unstable_timer_running = 0;

FILE* lcd;

#define ESC 27
#define CLEAR_LCD_STRING "[2J"

unsigned int numberOfLoadConnected = 5;


// Local Function Prototypes
int initOSDataStructs(void);
int initCreateTasks(void);

void initInterrupts(void);
//void LCDController(void *);


alt_u32 unstableTimer_isr_function(void *context)
{
	unstableTimerFlag = 1;
	printf("unstabletimer done");
	return 10;
}

alt_u32 stableTimer_isr_function(void *context)
{
	stableTimerFlag = 1;
	printf("stabletimer done");
	return 10;
}

const TickType_t delay50ms = 50/ portTICK_PERIOD_MS;
const TickType_t delay10ms = 10/ portTICK_PERIOD_MS;


void freq_relay(){

	#define SAMPLING_FREQ 16000.0
	double temp = SAMPLING_FREQ/(double)IORD(FREQUENCY_ANALYSER_BASE, 0);

	xQueueSendToBackFromISR( xFreqQueue, &temp, pdFALSE );

	return;
}

unsigned int global_unstableFlag = 0;

void ConditionChecking(void *pvParameters)
{
	double freqValue;
	unsigned int unstable_status = 1;
	unsigned int stable_status = 0;

	while(1)
	{
		if(uxQueueMessagesWaiting( xFreqQueue ) != 0){

			xQueueReceive( xFreqQueue, (void *) &freqValue, 0 );
			taskENTER_CRITICAL();
			printf("Frequency: %fHz\n", freqValue);
			taskEXIT_CRITICAL();

			//condition 1 checking ONLY
			if(freqValue < condition1_freqencyThreshold)
			{
				xQueueSend(xStatusQueue, &unstable_status, 10);
				global_unstableFlag = 1;
			}
			else
			{
				xQueueSend(xStatusQueue, &stable_status, 10);
				global_unstableFlag = 0;
			}
		}
		vTaskDelay(10);
	}
}





void TimerControl(void *pvParameters)
{
	while(1)
	{
		if(isMonitoring)
		{
			//printf("in timer control");
			if(global_unstableFlag == 1 && unstable_timer_running == 0)
			{
				printf("first triggered \n");
				//start timers
				alt_alarm_start(&unstableTimer500, 500, unstableTimer_isr_function, NULL);
				alt_alarm_stop(&stableTimer500);
				stable_timer_running = 0;
				unstable_timer_running = 1;

				unstableTimerFlag = 0;
				stableTimerFlag = 0;
			}
			else if( (global_unstableFlag == 0 && unstable_timer_running == 1))
			{
				//start timers
				printf("second triggered \n");
				alt_alarm_start(&stableTimer500, 500, stableTimer_isr_function, NULL);
				alt_alarm_stop(&unstableTimer500);
				stable_timer_running = 1;
				unstable_timer_running = 0;

				unstableTimerFlag = 0;
				stableTimerFlag = 0;
			}
		}
		vTaskDelay(10);
	}
}




void ControlCentre(void *pvParameters)
{
	unsigned int isUnstable = 0;

	while(1)
	{
		if(uxQueueMessagesWaiting( xStatusQueue ) != 0){

			xQueueReceive( xStatusQueue, (void *) &isUnstable, 0 );

			if(!isMonitoring)
			{
				//printf("ControlCentre isUnstable : %d", isUnstable);
				//unstable for the first time
				if(isUnstable == 1)
				{
					printf("initial unstable \n");
					xQueueSend(xInstructionQueue, &isUnstable, 10);
					isMonitoring = 1;
					alt_alarm_start(&unstableTimer500, 500, unstableTimer_isr_function, NULL);
					unstable_timer_running = 1;
					stable_timer_running = 0;
				}
			}
			else{
				if(isUnstable == 1 && unstableTimerFlag == 1)
				{
					printf("unstable = %d \n", isUnstable);
					xQueueSend(xInstructionQueue, &isUnstable, 10);
					unstableTimerFlag =0;
				}
				else if(isUnstable == 0 && stableTimerFlag == 1)
				{
					printf("stable = %d \n", isUnstable);
					xQueueSend(xInstructionQueue, &isUnstable, 10);
					stableTimerFlag = 0;
				}
			}
		}
		vTaskDelay(10);
	}
}

void ManageLoad(void *pvParameters)
{
	unsigned int instruction = 3;
	while(1)
	{
		if(uxQueueMessagesWaiting(xInstructionQueue) != 0)
		{
			xQueueReceive(xInstructionQueue, &instruction, 10);

			printf("INSTRUCTION RECEIVED IN MANAGE LOAD AS : %d \n", instruction);

			if(instruction == 0)
			{//connect
				IOWR_ALTERA_AVALON_PIO_DATA(GREEN_LEDS_BASE, 0x1);
				instruction = 3;

			}
			else if(instruction == 1)
			{//shed
				IOWR_ALTERA_AVALON_PIO_DATA(GREEN_LEDS_BASE, 0x0);
				instruction = 3;
			}
			else{
				//not expected
				printf("unexpected beh");
			}

		}
		vTaskDelay(10);
	}
}

void Connector(void *pvParameters)
{
	while(1)
	{
		if(xSemaphoreTake(connectSemaphore, portMAX_DELAY))
		{
			numberOfLoadConnected--;
		}
	}
}

void LCDController(void *pvParameters)
{
	while(1)
	{
		lcd = fopen(CHARACTER_LCD_NAME, "w");
		fprintf(lcd, "%c%s", ESC, CLEAR_LCD_STRING);
		fprintf(lcd, "Current Load");
		fclose(lcd);
		vTaskDelay(50);
	}
}


// The following test prints out status information every 3 seconds.
void print_status_task(void *pvParameters)
{
	//	while (1)
	//	{
	//		vTaskDelay(3000);
	//		printf("****************************************************************\n");
	//		printf("Hello From FreeRTOS Running on NIOS II.  Here is the status:\n");
	//		printf("\n");
	//		printf("The number of messages sent by the send_task:         %d\n", number_of_messages_sent);
	//		printf("\n");
	//		printf("The number of messages received by the receive_task1: %d\n", number_of_messages_received_task1);
	//		printf("\n");
	//		printf("The number of messages received by the receive_task2: %d\n", number_of_messages_received_task2);
	//		printf("\n");
	//		printf("The shared resource is owned by: %s\n", &sem_owner_task_name[0]);
	//		printf("\n");
	//		printf("The Number of times getsem_task1 acquired the semaphore %d\n", getsem_task1_got_sem);
	//		printf("\n");
	//		printf("The Number of times getsem_task2 acquired the semaphore %d\n", getsem_task2_got_sem);
	//		printf("\n");
	//		printf("****************************************************************\n");
	//		printf("\n");
	//	}
}

// The next two task compete for a shared resource via a semaphore.  The name of
// the task that owns the semaphore is copied into the global variable
// sem_owner_task_name[].
void getsem_task1(void *pvParameters)
{
	//	while (1)
	//	{
	//		xSemaphoreTake(shared_resource_sem, portMAX_DELAY);
	//		// block forever until receive the mutex
	//		strcpy(&sem_owner_task_name[0], "getsem_task1");
	//		getsem_task1_got_sem++;
	//		xSemaphoreGive(shared_resource_sem);
	//		vTaskDelay(100);
	//	}
}

void getsem_task2(void *pvParameters)
{
	//	while (1)
	//	{
	//		xSemaphoreTake(shared_resource_sem, portMAX_DELAY);
	//		// block forever until receive the mutex
	//		strcpy(&sem_owner_task_name[0], "getsem_task2");
	//		getsem_task2_got_sem++;
	//		xSemaphoreGive(shared_resource_sem);
	//		vTaskDelay(130);
	//	}
}

// The following task fills up a message queue with incrementing data.  The data
// is not actually used by the application.  If the queue is full the task is
// suspended for 1 second.
void send_task(void *pvParameters)
{
	//	unsigned int msg = 0;
	//	while (1)
	//	{
	//		if (xQueueSend(msgqueue, (void *)&msg, 0) == pdPASS)
	//		{
	//			// in the message queue
	//			msg++;
	//			number_of_messages_sent++;
	//		}
	//		else
	//		{
	//			vTaskDelay(1000);
	//		}
	//	}
}

// The next two task pull messages in the queue at different rates.  The number
// of messages received by the task is incremented when a new message is received
void receive_task1(void *pvParameters)
{
	unsigned int *msg;
	while (1)
	{
		//		xQueueReceive(msgqueue, &msg, portMAX_DELAY);
		//		number_of_messages_received_task1++;
		//		vTaskDelay(333);
	}
}

void receive_task2(void *pvParameters)
{
	unsigned int *msg;
	while (1)
	{
		//		xQueueReceive(msgqueue, &msg, portMAX_DELAY);
		//		number_of_messages_received_task2++;
		//		vTaskDelay(1000);
	}
}

int main(int argc, char* argv[], char* envp[])
{
	initOSDataStructs();
	initCreateTasks();
	initInterrupts();

	//turn it on initially
	IOWR_ALTERA_AVALON_PIO_DATA(GREEN_LEDS_BASE, 0x1);
	//shedSemaphore = xSemaphoreCreateBinary();
	//connectSemaphore = xSemaphoreCreateBinary();

	//	if(shedSemaphore == NULL || connectSemaphore == NULL)
	//	{
	//		taskENTER_CRITICAL();
	//		printf("Insufficient heap to create semaphores");
	//		taskEXIT_CRITICAL();
	//	}

	vTaskStartScheduler();
	for (;;);
	return 0;
}

void initInterrupts(void)
{
	alt_irq_register(FREQUENCY_ANALYSER_IRQ, 0, freq_relay);
}

// This function simply creates a message queue and a semaphore
int initOSDataStructs(void)
{
	//	msgqueue = xQueueCreate( MSG_QUEUE_SIZE, sizeof( void* ) );
	xFreqQueue = xQueueCreate( 100, sizeof( double ) );
	xStatusQueue = xQueueCreate(100, sizeof(unsigned int));
	xInstructionQueue = xQueueCreate(100, sizeof(unsigned int ));
	//	shared_resource_sem = xSemaphoreCreateCounting( 9999, 1 );
	return 0;
}

// This function creates the tasks used in this example
int initCreateTasks(void)
{

	//xTaskCreate(TimerControl, "TimerControl", 1024, NULL, 8 , NULL);
	xTaskCreate(ConditionChecking, "ConditionChecking", 1024, NULL, 9, NULL);
	xTaskCreate(ControlCentre, "ControlCentre", 1024, NULL, 7, NULL);
	xTaskCreate(ManageLoad, "ManageLoad", 1024, NULL, 6, NULL);
	//	xTaskCreate(Connector, "Connector", 1024, NULL, 2, NULL);
	//xTaskCreate(LCDController, "LCDController", 1024, NULL, 8, NULL);
	return 0;
}
