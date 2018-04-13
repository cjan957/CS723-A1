#include "freq_relay.h"
#define SAMPLING_FREQ 16000.0

void freq_relay(){


	double temp = SAMPLING_FREQ/(double)IORD(FREQUENCY_ANALYSER_BASE, 0);

	xQueueSendToBackFromISR( xFreqQueue, &temp, pdFALSE );
	xQueueSendToBackFromISR( xDispFreqQueue, &temp, pdFALSE );

	xSemaphoreGiveFromISR(xConditionSemaphore, pdFALSE);

	return;
}


