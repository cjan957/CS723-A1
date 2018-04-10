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
	initSemaphores();

	global_unstableFlag = 0;


	stable_timer_running = 0;
	unstable_timer_running = 0;

	unstableFlag = 0; //stable = 0, unstable = 1
	isMonitoring = 0;
	_currentSwitchValue = 0;

	//turn it on initially
	IOWR_ALTERA_AVALON_PIO_DATA(GREEN_LEDS_BASE, 0x1);
	//get initial switch value
	_currentSwitchValue = IORD_ALTERA_AVALON_PIO_DATA(SLIDE_SWITCH_BASE);
	IOWR_ALTERA_AVALON_PIO_DATA(RED_LEDS_BASE, _currentSwitchValue);
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

void initSemaphores(void)
{
	xTimer500Semaphore = xSemaphoreCreateBinary();
	xSwitchSemaphore = xSemaphoreCreateBinary();
	if(xTimer500Semaphore == NULL || xSwitchSemaphore == NULL)
	{
		printf("cant create semaphore(s)");
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


void vTimer500Callback(xTimerHandle t_timer)
{
	xSemaphoreGiveFromISR(xTimer500Semaphore, NULL);
}


void initTimers(void)
{
	xTimer500 = xTimerCreate("timer500", 500, pdTRUE, NULL, vTimer500Callback);
}



void ControlCentre(void *pvParameters)
{
	int shedInst = 1;
	int connectInst = 0;

	while(1)
	{

		//wait for semaphore which will be given by timerISR (500)
		if(xSemaphoreTake(xTimer500Semaphore, 999999))
		{
			printf("time up 500 \n");
			if(global_unstableFlag == 1) //unstable
			{
				//shed more
				if(xQueueSend(xInstructionQueue, &shedInst, 9999 ) != pdPASS)
				{
					printf("Failed to instrct to shed (frm control cen");
				}
			}
			else //stable
			{
				//connect more
				if(xQueueSend(xInstructionQueue, &connectInst, 9999 ) != pdPASS)
				{
					printf("failed to instruct to connect (frm control cen)");
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

	unsigned int foundLoadNotShed = 0;
	unsigned int foundLoadNotConnected = 0;

	unsigned int temp;

	int i = 0;

	while(1)
	{
		//TODO: is this ok? waiting for two queue in the same task?
		//Check to see if there's any change to the switch value
		//CHANGE TO PEEK!!!
		if((uxQueueMessagesWaiting(xSwitchPositionQueue) != 0) && (xQueuePeek(xSwitchPositionQueue, &currentSwitchValue, 10) == pdTRUE))
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

		//once peeking of switch value is done, allows LEDs task to receive from the switchPositionqueue
		xSemaphoreGive(xSwitchSemaphore);

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

				if(loadShedStatus[0] == 0 && loadShedStatus[1] == 0 && loadShedStatus[2] == 0 && loadShedStatus[3] == 0 && loadShedStatus[4] == 0)
				{
					taskENTER_CRITICAL();
					isMonitoring = 0;
					taskEXIT_CRITICAL();
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
						taskENTER_CRITICAL();
						isMonitoring = 1;
						taskEXIT_CRITICAL();
						break;
					}
				}

				break;
			default:
				printf("invalid inst");
				break;
			}


			/*
			foundLoadNotShed = 0;
			foundLoadNotConnected = 0;

			//if all shedable loads have been shed, stop timer 500
			for(i = 0; i < 5; i++)
			{
				temp = loadShedStatus[i];
				if (switchStatus[i] == 1) {
					if (temp) {
						foundLoadNotConnected = 1;
					}
					else {
						foundLoadNotShed = 1;
					}
				}
			}

			if(!foundLoadNotShed || !foundLoadNotConnected)
			{
				foundLoadNotShed = 0;
				xTimerStop(xTimer500, 500);
			}
			*/

			//push shed status to queue to send to led task
			if(xQueueSend(xShedLoadStatusQueue, &loadShedStatus, 10) != pdTRUE)
			{
				printf("shedding status of all loads was not sent! \n");
			}
		}

		vTaskDelay(10);
	}
}



void LEDController(void *pvParameters)
{
	unsigned int shedLoadStatus[5] = {0,0,0,0,0};

	int i;
	unsigned int multiplier = 1;

	unsigned int shedStatusBinary = 0;
	unsigned int switchStatusBinary = 0;

	int greenLEDs = 0;
	int redLEDs = _currentSwitchValue;

	while(1)
	{

		//
		xSemaphoreTake(xSwitchSemaphore, 10);
		if((uxQueueMessagesWaiting(xSwitchPositionQueue) != 0) && (xQueueReceive(xSwitchPositionQueue, &switchStatusBinary, 10) == pdTRUE))
		{
			printf("switch values updated in LEDController! \n");
			//redLEDs = switchStatusBinary;
		}
		xSemaphoreGive(xSwitchSemaphore);

		if((uxQueueMessagesWaiting(xShedLoadStatusQueue) != 0) && (xQueueReceive(xShedLoadStatusQueue, &shedLoadStatus, 10) == pdTRUE))
		{
			shedStatusBinary = 0;
			multiplier = 1;

			for(i = 0; i < 5; i++)
			{
				shedStatusBinary += shedLoadStatus[i] * multiplier;
				multiplier *= 2;
			}

			redLEDs = ~(shedStatusBinary & 0x1F);
			redLEDs = redLEDs & switchStatusBinary;
			redLEDs = redLEDs & 0x1F; //ignore all switches other than 0-4 switches

			printf("Shed Status is %d, %d, %d, %d, %d \n", shedLoadStatus[4],shedLoadStatus[3],shedLoadStatus[2],shedLoadStatus[1],shedLoadStatus[0]);
		}

		//printf("led status g: %d, r: %d \n", shedStatusBinary, redLEDs);


		IOWR_ALTERA_AVALON_PIO_DATA(RED_LEDS_BASE, redLEDs);
		IOWR_ALTERA_AVALON_PIO_DATA(GREEN_LEDS_BASE, shedStatusBinary);

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

