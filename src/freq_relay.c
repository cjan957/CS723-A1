#include "freq_relay.h"
#define SAMPLING_FREQ 16000.0

// Transmits the new frequency values into queues and enables for the conditionChecking task to resume by giving a semaphore.
void freq_relay(){


	double temp = SAMPLING_FREQ/(double)IORD(FREQUENCY_ANALYSER_BASE, 0);

	// Sends data to queues
	xQueueSendToBackFromISR(xFreqQueue, &temp, pdFALSE); // Sends to the ConditionChecking task
	xQueueSendToBackFromISR(xDispFreqQueue, &temp, pdFALSE); // For plotting in the VGA task

	xSemaphoreGiveFromISR(xConditionSemaphore, pdFALSE);

	return;
}


