#include "main.h"
#include "freq_relay.h"
#include "ConditionChecking.h"


// Definition of Task Stacks
#define   TASK_STACKSIZE       2048

//QueueHandle_t xFreqQueue;
//QueueHandle_t xStatusQueue;
QueueHandle_t xInstructionQueue;

// Definition of Semaphore
SemaphoreHandle_t shared_resource_sem;

//semaphore
SemaphoreHandle_t shedSemaphore;
SemaphoreHandle_t connectSemaphore;



//timer flags


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

int main(int argc, char* argv[], char* envp[])
{
	initOSDataStructs();
	initCreateTasks();
	initInterrupts();

	global_unstableFlag = 0;

	unstableTimerFlag = 0;
	stableTimerFlag = 0;

	stable_timer_running = 0;
	unstable_timer_running = 0;

	unstableFlag = 0; //stable = 0, unstable = 1
	isMonitoring = 0;

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
