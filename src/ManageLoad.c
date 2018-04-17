#include "ManageLoad.h"
#include "main.h"
#include "MeasurementTask.h"
#include "Switches.h"

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
								xSemaphoreGive(xHasNewTimeDiff);
								//_hasNewTimeDiff = 1;
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




