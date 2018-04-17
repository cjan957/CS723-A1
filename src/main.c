#include "main.h"
#include "freq_relay.h"
#include "ConditionChecking.h"
#include "TimerControl.h"
#include "VGA.h"
#include "Switches.h"
#include "keyboard.h"
#include "MeasurementTask.h"
#include "ManageLoad.h"
#include "LEDController.h"

// Definition of Task Stacks
#define   TASK_STACKSIZE       2048

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
	setupKeyboard();
	initFlags();

	// Initialise LEDs
	IOWR_ALTERA_AVALON_PIO_DATA(GREEN_LEDS_BASE, 0x0);

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
	xHasNewTimeDiff =  xSemaphoreCreateBinary();
	if(xTimer500Semaphore == NULL || xSwitchSemaphore == NULL || xButtonSemaphore == NULL)
	{
		printf("cant create semaphore(s)");
	}
}

void maintenance_button_int(void* context, alt_u32 id)
{
	_maintenanceMode = !_maintenanceMode;
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

	xTaskCreate(ConditionChecking, "ConditionChecking", TASK_STACKSIZE, NULL, 9, NULL);
	xTaskCreate(TimerControl, "TimerControl", TASK_STACKSIZE, NULL, 8 , NULL);
	xTaskCreate(ControlCentre, "ControlCentre", TASK_STACKSIZE, NULL, 7, NULL);
	xTaskCreate(ManageLoad, "ManageLoad", TASK_STACKSIZE, NULL, 6, NULL);
	xTaskCreate(LEDController, "LEDController", TASK_STACKSIZE, NULL, 5, NULL);
	xTaskCreate(MeasurementTask, "Measure", TASK_STACKSIZE, NULL, 4, NULL);
	xTaskCreate(VGA_Draw, "VGA_Draw", 4096, NULL, 3, NULL);
	xTaskCreate(keyboardProcessor, "Keyboard", TASK_STACKSIZE, NULL, 2, NULL);
	xTaskCreate(SwitchRead, "SwitchRead", TASK_STACKSIZE, NULL, 1, NULL);

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

int setupKeyboard(void)
{

	initKeyboard = 1;
	keyboardFlag = 0;

	alt_up_ps2_dev * ps2_device = alt_up_ps2_open_dev(PS2_NAME);

	if(ps2_device == NULL){
		printf("can't find PS/2 device\n");
		return 1;
	}

	alt_up_ps2_clear_fifo (ps2_device) ;

	alt_irq_register(PS2_IRQ, ps2_device, ps2_isr);
	// register the PS/2 interrupt
	IOWR_8DIRECT(PS2_BASE,4,1);

	return 0;

}

void initFlags(void)
{

	_systemTime = 0;

	global_unstableFlag = 0;

	stable_timer_running = 0;
	unstable_timer_running = 0;

	unstableFlag = 0; //stable = 0, unstable = 1
	isMonitoring = 0;
	_currentSwitchValue = 0;

	actualTimeDifference = 0;
	_timeDiff = 0;

}

