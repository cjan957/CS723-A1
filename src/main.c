#include "main.h"
#include "freq_relay.h"
#include "ConditionChecking.h"
#include "TimerControl.h"
#include "VGA.h"
#include "Switches.h"

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

FILE* lcd;

#define ESC 27
#define CLEAR_LCD_STRING "[2J"

unsigned int numberOfLoadConnected = 5;


// Local Function Prototypes
int initOSDataStructs(void);
int initCreateTasks(void);

void initInterrupts(void);
//void LCDController(void *);


const TickType_t delay50ms = 50/ portTICK_PERIOD_MS;
const TickType_t delay10ms = 10/ portTICK_PERIOD_MS;

int main(int argc, char* argv[], char* envp[])
{
	initOSDataStructs();
	initCreateTasks();
	initInterrupts();
	initTimers();

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

void initInterrupts(void)
{
	alt_irq_register(FREQUENCY_ANALYSER_IRQ, 0, freq_relay);
}

// This function simply creates a message queue and a semaphore
int initOSDataStructs(void)
{
	//	msgqueue = xQueueCreate( MSG_QUEUE_SIZE, sizeof( void* ) );
	xFreqQueue = xQueueCreate( 100, sizeof( double ) );
	xDispFreqQueue = xQueueCreate( 100, sizeof( double ) );
	xStatusQueue = xQueueCreate(100, sizeof(unsigned int));
	xInstructionQueue = xQueueCreate(100, sizeof(unsigned int ));
	xROCQueue = xQueueCreate(100, sizeof(double));
	xSwitchPositionQueue = xQueueCreate(10, sizeof(unsigned int));

	xShedLoadStatusQueue = xQueueCreate(10, sizeof(int[5]));
	//	shared_resource_sem = xSemaphoreCreateCounting( 9999, 1 );
	return 0;
}

// This function creates the tasks used in this example
int initCreateTasks(void)
{

	xTaskCreate(TimerControl, "TimerControl", 1024, NULL, 8 , NULL);
	xTaskCreate(ConditionChecking, "ConditionChecking", 1024, NULL, 9, NULL);
	xTaskCreate(ControlCentre, "ControlCentre", 1024, NULL, 7, NULL);
	xTaskCreate(ManageLoad, "ManageLoad", 1024, NULL, 6, NULL);
	//xTaskCreate(VGA_Draw, "VGA_Draw", 4096, NULL, 3, NULL);
	xTaskCreate(SwitchRead, "SwitchRead", 1024, NULL, 1, NULL);
	xTaskCreate(LEDController, "LEDController", 1024, NULL, 5, NULL);

	//	xTaskCreate(Connector, "Connector", 1024, NULL, 2, NULL);
	//xTaskCreate(LCDController, "LCDController", 1024, NULL, 8, NULL);
	return 0;
}

void vUnstableTimerCallback(xTimerHandle t_timer)
{
	unstableTimerFlag = 1;

}

void vStableTimerCallback(xTimerHandle t_timer)
{
	stableTimerFlag = 1;
}


void initTimers(void)
{
	unstableTimer500 = xTimerCreate("unstableTimer", 500, pdTRUE, NULL, vUnstableTimerCallback);
	stableTimer500 = xTimerCreate("stableTimer", 500, pdTRUE, NULL, vStableTimerCallback);
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
				if(isUnstable == 1)
				{
					//printf("initial unstable \n");
					xQueueSend(xInstructionQueue, &isUnstable, 10);
					isMonitoring = 1;
					if(xTimerStart(unstableTimer500, 0) != pdPASS)
					{
						//printf("cannot start unstable timer");
					}
					unstable_timer_running = 1;
					stable_timer_running = 0;
				}
			}
			else{
				if(isUnstable == 1 && unstableTimerFlag == 1)
				{
					//printf("unstable = %d \n", isUnstable);
					xQueueSend(xInstructionQueue, &isUnstable, 10);
					unstableTimerFlag =0;
				}
				else if(isUnstable == 0 && stableTimerFlag == 1)
				{
					//printf("stable = %d \n", isUnstable);
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
	//instruction, 1 = shed, 0 = connect, others: ignore
	unsigned int instruction = 3;

	//keep track of shedStatus of each load, 1 if load has been shed.
	//index 0 = lowest priority, 5 = highest
	unsigned int loadShedStatus[5] = {0,0,0,0,0};

	unsigned int switchStatus[5] = {0,0,0,0,0};

	//masking for each switch position
	unsigned int masking[5] = {1,2,4,8,16};

	unsigned int currentSwitchValue = 0;

	int i = 0;

	while(1)
	{
		//Check to see if there's any change to the switch value
		//CHANGE TO PEEK!!!
		if((uxQueueMessagesWaiting(xSwitchPositionQueue) != 0) && (xQueueReceive(xSwitchPositionQueue, &currentSwitchValue, 10) == pdTRUE))
		{
			for(i = 0; i < 5; i++)
			{
				if((currentSwitchValue & masking[i]) != 0)
				{
					switchStatus[i] = 1;
				}
				else
				{
					switchStatus[i] = 0;
					loadShedStatus[i] = 0;
				}
			}
			printf("Current Switch Position is %d, %d, %d, %d, %d \n", switchStatus[4],switchStatus[3],switchStatus[2],switchStatus[1],switchStatus[0]);
		}

		//Check to see if there's any new instruction
		if((uxQueueMessagesWaiting(xInstructionQueue) != 0) && (xQueueReceive(xInstructionQueue, &instruction, 10) == pdTRUE))
		{
			switch(instruction)
			{
			case 0: //connect
				//Connect the load, based on its priority and whether load's switch is on
				for(i = 4; i >= 0; i--)
				{
					if((loadShedStatus[i] == 1) && (switchStatus[i] == 1))
					{
						loadShedStatus[i] = 0; //CONNECT
						break;
					}
				}
				break;
			case 1: //shed
				//Shed the load if it's hasn't been shed, based on the priority and whether
				//switch is on.
				for(i = 0; i < 5; i++)
				{
					if((loadShedStatus[i] == 0) && (switchStatus[i] == 1))
					{
						loadShedStatus[i] = 1; //SHED
						break;
					}
				}
				break;
			default:
				printf("invalid inst");
			}

			//push shed status to queue to send to led task
			if(xQueueSend(xShedLoadStatusQueue, &loadShedStatus, 10) == pdTRUE)
			{
				printf("shedding status sent successfully! \n");
			}
		}

		vTaskDelay(10);
	}
}



void LEDController(void *pvParameters)
{
	unsigned int shedLoadStatus[5] = {0,0,0,0,0};
	while(1)
	{
		if((uxQueueMessagesWaiting(xShedLoadStatusQueue) != 0) && (xQueueReceive(xShedLoadStatusQueue, &shedLoadStatus, 10) == pdTRUE))
		{
			printf("Shed Status is %d, %d, %d, %d, %d \n", shedLoadStatus[4],shedLoadStatus[3],shedLoadStatus[2],shedLoadStatus[1],shedLoadStatus[0]);
		}
		vTaskDelay(10);
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

