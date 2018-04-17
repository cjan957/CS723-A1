#include "main.h"
#include "freq_relay.h"
#include "ConditionChecking.h"
#include "TimerControl.h"
#include "VGA.h"
#include "Switches.h"
#include "keyboard.h"
#include "MeasurementTask.h"

// Definition of Task Stacks
#define   TASK_STACKSIZE       2048


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
const TickType_t delay500ms = 500/ portTICK_PERIOD_MS;

const TickType_t delay1s = 1000/ portTICK_PERIOD_MS;

int main(int argc, char* argv[], char* envp[])
{
	initOSDataStructs();
	initCreateTasks();
	initInterrupts();
	initTimers();
	initSemaphores();

	// Keyboard
	initKeyboard = 1;
	keyboardFlag = 0;

	_systemTime = 0;

	alt_up_ps2_dev * ps2_device = alt_up_ps2_open_dev(PS2_NAME);

	if(ps2_device == NULL){
		printf("can't find PS/2 device\n");
		return 1;
	}

	alt_up_ps2_clear_fifo (ps2_device) ;

	alt_irq_register(PS2_IRQ, ps2_device, ps2_isr);
	// register the PS/2 interrupt
	IOWR_8DIRECT(PS2_BASE,4,1);

	global_unstableFlag = 0;

	stable_timer_running = 0;
	unstable_timer_running = 0;

	unstableFlag = 0; //stable = 0, unstable = 1
	isMonitoring = 0;
	_currentSwitchValue = 0;

	actualTimeDifference = 0;
	_timeDiff = 0;

	//turn it on initially
	IOWR_ALTERA_AVALON_PIO_DATA(GREEN_LEDS_BASE, 0x1);

	//get initial switch value
	_currentSwitchValue = IORD_ALTERA_AVALON_PIO_DATA(SLIDE_SWITCH_BASE);
	IOWR_ALTERA_AVALON_PIO_DATA(RED_LEDS_BASE, _currentSwitchValue);

	vTaskStartScheduler();
	for (;;);
	return 0;
}

void initSemaphores(void)
{
	xTimer500Semaphore = xSemaphoreCreateBinary();
	xSwitchSemaphore = xSemaphoreCreateBinary();
	xButtonSemaphore = xSemaphoreCreateBinary();
	xKeyboardSemaphore = xSemaphoreCreateBinary();
	xConditionSemaphore = xSemaphoreCreateBinary();
	if(xTimer500Semaphore == NULL || xSwitchSemaphore == NULL || xButtonSemaphore == NULL)
	{
		printf("cant create semaphore(s)");
	}
}

void maintenance_button_int(void* context, alt_u32 id)
{
	_maintenanceMode = !_maintenanceMode;
	//xSemaphoreGiveFromISR(xButtonSemaphore, NULL);
	IOWR_ALTERA_AVALON_PIO_EDGE_CAP(PUSH_BUTTON_BASE, 0x7);
}


void initInterrupts(void)
{
	IOWR_ALTERA_AVALON_PIO_EDGE_CAP(PUSH_BUTTON_BASE, 0x7);
	IOWR_ALTERA_AVALON_PIO_IRQ_MASK(PUSH_BUTTON_BASE, 0x1);
	alt_irq_register(PUSH_BUTTON_IRQ, NULL, maintenance_button_int);
	alt_irq_register(FREQUENCY_ANALYSER_IRQ, 0, freq_relay);
}


// This function simply creates a message queue and a semaphore
int initOSDataStructs(void)
{
	xFreqQueue = xQueueCreate( 100, sizeof( double ) );
	xDispFreqQueue = xQueueCreate( 100, sizeof( double ) );
	xStatusQueue = xQueueCreate(100, sizeof(unsigned int));
	xInstructionQueue = xQueueCreate(100, sizeof(unsigned int ));
	xROCQueue = xQueueCreate(100, sizeof(double));
	xSwitchPositionQueue = xQueueCreate(10, sizeof(unsigned int));
	xShedLoadStatusQueue = xQueueCreate(10, sizeof(int[5]));
	xMeasurementQueue = xQueueCreate(10, sizeof(Measure));

	return 0;
}

// This function creates the tasks used in this example
int initCreateTasks(void)
{

	xTaskCreate(TimerControl, "TimerControl", 1024, NULL, 8 , NULL);
	xTaskCreate(ConditionChecking, "ConditionChecking", 1024, NULL, 9, NULL);
	xTaskCreate(ControlCentre, "ControlCentre", 1024, NULL, 7, NULL);
	xTaskCreate(ManageLoad, "ManageLoad", 1024, NULL, 6, NULL);

	xTaskCreate(VGA_Draw, "VGA_Draw", 4096, NULL, 3, NULL);
	xTaskCreate(SwitchRead, "SwitchRead", 1024, NULL, 1, NULL);
	xTaskCreate(LEDController, "LEDController", 1024, NULL, 5, NULL);
	xTaskCreate(keyboardProcessor, "Keyboard", 1024, NULL, 2, NULL);

	xTaskCreate(MeasurementTask, "Measure", 1024, NULL, 4, NULL);


	return 0;
}


void vTimer500Callback(xTimerHandle t_timer)
{
	xSemaphoreGiveFromISR(xTimer500Semaphore, NULL);
}

void vSystemTimeCallback(xTimerHandle t_timer)
{
	_systemTime++;
}

void vTimeDiffCallback(xTimerHandle t_timer)
{
	_timeDiff++;
}


void initTimers(void)
{
	xTimer500 = xTimerCreate("timer500", delay500ms, pdTRUE, NULL, vTimer500Callback);
	xSystemTime = xTimerCreate("SystemTime", delay1s, pdTRUE, NULL, vSystemTimeCallback);
	xTimeDiff = xTimerCreate("TimeDiff", 1, pdTRUE, NULL, vTimeDiffCallback);

	xTimerStart(xSystemTime,delay1s);
}


void ControlCentre(void *pvParameters)
{
	int shedInst = 1;
	int connectInst = 0;

	while(1)
	{

		//printf(" i shou.dnt be printing top \n");
		//wait for semaphore which will be given by timerISR (500)
		if(xSemaphoreTake(xTimer500Semaphore, portMAX_DELAY))
		{
			if(!_maintenanceMode)
			{
				printf("time up 500 \n");
				if(global_unstableFlag == 1) //unstable
				{
					printf("im unstable \n");
					//shed more
					if(xQueueSend(xInstructionQueue, &shedInst, portMAX_DELAY ) != pdPASS)
					{
						printf("Failed to instrct to shed (frm control cen");
					}
				}
				else //stable
				{
					printf("i shoulndt be printing if im maintain, global_unstable is:%d \n", global_unstableFlag);
					//connect more
					if(xQueueSend(xInstructionQueue, &connectInst, portMAX_DELAY ) != pdPASS)
					{
						printf("failed to instruct to connect (frm control cen)");
					}
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

	unsigned int firstShed = 1;

	int i = 0;

	while(1)
	{
		if(!_maintenanceMode)
		{
			//TODO: is this ok? waiting for two queue in the same task?
			//Check to see if there's any change to the switch value
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

							// Stop the time difference timer
							if(firstShed == 1) {
								actualTimeDifference = _timeDiff;
								_hasNewTimeDiff = 1;
								printf("Time Diff: %d\n", actualTimeDifference);
								xTimerStop(xTimeDiff, 1);
								firstShed = 0;
							}

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

				if (!foundLoadNotConnected) {
					firstShed = 1;
					taskENTER_CRITICAL();
					_timeDiff = 0;
					taskEXIT_CRITICAL();
					xTimerStop(xTimeDiff, portMAX_DELAY);
					//xTimerReset(xTimeDiff,1); //why here???
				}

				if(!foundLoadNotShed || !foundLoadNotConnected)
				{
					foundLoadNotShed = 0;
					xTimerStop(xTimer500, 500);
				}


				//push shed status to queue to send to led task
				if(xQueueSend(xShedLoadStatusQueue, &loadShedStatus, 10) != pdTRUE)
				{
					printf("shedding status of all loads was not sent! \n");
				}
			}
		}
		else
		{
			//replace currentSwitch array to be what the actual switch value is
			int currentSwitch = _currentSwitchValue;

			for(i = 0; i < 5; i++)
			{
				if((currentSwitch & masking[i]) != 0)
				{
					switchStatus[i] = 1;
				}
				else
				{
					switchStatus[i] = 0;
					loadShedStatus[i] = 0;
				}
				loadShedStatus[i] = 0;
			}
		}

		vTaskDelay(10);
	}
}



void LEDController(void *pvParameters)
{
	unsigned int shedLoadStatus[5] = {0,0,0,0,0};
	unsigned int new_switchStatus[5] = {0,0,0,0,0}; //new
	unsigned int prev_switchStatus[5] = {0,0,0,0,0};

	unsigned int swOnWhenMonitoring[5] = {0,0,0,0,0};
	unsigned int swOnMaskingValue = 0;

	int i;
	unsigned int multiplier = 1;

	unsigned int shedStatusBinary = 0;
	unsigned int switchStatusBinary = 0;

	unsigned int tempShedStatusBinary = 0;

	unsigned int masking[5] = {1,2,4,8,16};

	int redLEDs = _currentSwitchValue;

	while(1)
	{
		if(!_maintenanceMode)
		{
			xSemaphoreTake(xSwitchSemaphore, 10);
			//when there's a new switch value.....
			if((uxQueueMessagesWaiting(xSwitchPositionQueue) != 0) && (xQueueReceive(xSwitchPositionQueue, &switchStatusBinary, 10) == pdTRUE))
			{
				printf("switch values updated in LEDController! \n");

				for(i = 0; i < 5; i++)
				{
					if((switchStatusBinary & masking[i]) != 0)
					{
						new_switchStatus[i] = 1;

//						if(isMonitoring)
//						{
//							turnedOnWhileMonitoring[i] = 1;
//						}
					}
					else
					{
						new_switchStatus[i] = 0;
					}

					if(new_switchStatus[i] == 0)
					{
						shedLoadStatus[i] = 0; //set this load as 'not shed' since it has been turned off
					}
				}


				//Check if any switch is turned on when the relay is monitoring the load
				if(isMonitoring)
				{
					for(i = 0; i < 5; i++)
					{
						if((new_switchStatus[i] == 1) && (prev_switchStatus[i] == 0))
						{
							//swOnWhenMonitoring[i] = 1;
							swOnMaskingValue += masking[i];
						}
					}
				}


				//copy the new switch values to the prev Switch array
				for(i = 0; i < 5; i++)
				{
					prev_switchStatus[i] = new_switchStatus[i];
				}



				shedStatusBinary = 0;
				multiplier = 1;

				for(i = 0; i < 5; i++)
				{
					shedStatusBinary += shedLoadStatus[i] * multiplier;
					multiplier *= 2;
				}

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

				printf("Shed Status is %d, %d, %d, %d, %d \n", shedLoadStatus[4],shedLoadStatus[3],shedLoadStatus[2],shedLoadStatus[1],shedLoadStatus[0]);

			}


			//printf("led status g: %d, r: %d \n", shedStatusBinary, redLEDs);
			//find the right redLEDs value based on the masking 'turnedOnWhileMonitoring'


			// if there is a change
			// redLED is this
			//else
			if(!isMonitoring)
			{
				swOnMaskingValue = 0;
			}

			redLEDs = ~(shedStatusBinary & 0x1F);
			redLEDs = redLEDs & switchStatusBinary;
			redLEDs = redLEDs & 0x1F; //ignore all switches other than 0-4 switches
			redLEDs = redLEDs & ~swOnMaskingValue;

			IOWR_ALTERA_AVALON_PIO_DATA(RED_LEDS_BASE, redLEDs);
			IOWR_ALTERA_AVALON_PIO_DATA(GREEN_LEDS_BASE, shedStatusBinary);

		}
		else
		{	//under maintenance mode

			IOWR_ALTERA_AVALON_PIO_DATA(RED_LEDS_BASE, _currentSwitchValue);
			IOWR_ALTERA_AVALON_PIO_DATA(GREEN_LEDS_BASE, 0x0);


			//replace currentSwitch array to be what the actual switch value is
			int currentSwitch = _currentSwitchValue;

			for(i = 0; i < 5; i++)
			{
				if((currentSwitch & masking[i]) != 0)
				{
					new_switchStatus[i] = 1;
				}
				else
				{
					new_switchStatus[i] = 0;
				}
				shedLoadStatus[i] = 0;
			}

			shedStatusBinary = 0;
		}
		vTaskDelay(10);
	}

}


