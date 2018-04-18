#include "ControlCentre.h"

// Sends instructions through a queue to the ManageLoad task whether to shed or connect
// It will send an instruction to connect if it is stable and shed if it is unstable
void ControlCentre(void *pvParameters)
{
	int shedInst = 1;
	int connectInst = 0;

	while(1)
	{

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
					//connect more
					if(xQueueSend(xInstructionQueue, &connectInst, portMAX_DELAY ) != pdPASS)
					{
						printf("failed to instruct to connect (frm control cen)");
					}
				}
			}
		}
	}
}



