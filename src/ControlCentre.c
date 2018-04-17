#include "ControlCentre.h"

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
	}
}



