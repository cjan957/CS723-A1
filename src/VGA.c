#include "VGA.h"
#include "ConditionChecking.h"
#include "freq_relay.h"
#include "MeasurementTask.h"
#include "main.h"

void VGA_Draw(void *pvParameters) {


	//initialize VGA controllers
	alt_up_pixel_buffer_dma_dev *pixel_buf;
	pixel_buf = alt_up_pixel_buffer_dma_open_dev(VIDEO_PIXEL_BUFFER_DMA_NAME);
	if(pixel_buf == NULL){
		printf("can't find pixel buffer device\n");
	}

	alt_up_pixel_buffer_dma_clear_screen(pixel_buf, 0);

	alt_up_char_buffer_dev *char_buf;
	char_buf = alt_up_char_buffer_open_dev("/dev/video_character_buffer_with_dma");
	if(char_buf == NULL){
		printf("can't find char buffer device\n");
	}
	alt_up_char_buffer_clear(char_buf);


	//Set up plot axes
	alt_up_pixel_buffer_dma_draw_hline(pixel_buf, 100, 590, 200, ((0x3ff << 20) + (0x3ff << 10) + (0x3ff)), 0);
	alt_up_pixel_buffer_dma_draw_hline(pixel_buf, 100, 590, 300, ((0x3ff << 20) + (0x3ff << 10) + (0x3ff)), 0);
	alt_up_pixel_buffer_dma_draw_vline(pixel_buf, 100, 50, 200, ((0x3ff << 20) + (0x3ff << 10) + (0x3ff)), 0);
	alt_up_pixel_buffer_dma_draw_vline(pixel_buf, 100, 220, 300, ((0x3ff << 20) + (0x3ff << 10) + (0x3ff)), 0);

	alt_up_char_buffer_string(char_buf, "Frequency(Hz)", 4, 4);
	alt_up_char_buffer_string(char_buf, "52", 10, 7);
	alt_up_char_buffer_string(char_buf, "50", 10, 12);
	alt_up_char_buffer_string(char_buf, "48", 10, 17);
	alt_up_char_buffer_string(char_buf, "46", 10, 22);

	alt_up_char_buffer_string(char_buf, "df/dt(Hz/s)", 4, 26);
	alt_up_char_buffer_string(char_buf, "60", 10, 28);
	alt_up_char_buffer_string(char_buf, "30", 10, 30);
	alt_up_char_buffer_string(char_buf, "0", 10, 32);
	alt_up_char_buffer_string(char_buf, "-30", 9, 34);
	alt_up_char_buffer_string(char_buf, "-60", 9, 36);

	// Set up the text
	alt_up_char_buffer_string(char_buf, "Frequency Threshold: ", 7, 40);
	alt_up_char_buffer_string(char_buf, "ROC Threshold: ", 7, 42);
	alt_up_char_buffer_string(char_buf, "Last 5 Results(ms): ", 7, 44);
	alt_up_char_buffer_string(char_buf, "System Status", 7, 46);

	alt_up_char_buffer_string(char_buf, "Measured Time: ", 7, 48);
	alt_up_char_buffer_string(char_buf, "Maximum Time: ", 7, 50);
	alt_up_char_buffer_string(char_buf, "Minimum Time: ", 7, 52);
	alt_up_char_buffer_string(char_buf, "Average Time: ", 7, 54);

	alt_up_char_buffer_string(char_buf, "Time: " ,53, 1);


	char cond1[10] = "";
	char cond2[10]= "";

	char sysTime[10]= "";

	char timeDiff[10]= "";
	char avgTime[10]= "";
	char maxTime[10]= "";
	char minTime[10]= "";

	char firstValue[1]= "";
	char secondValue[1]= "";
	char thirdValue[1]= "";
	char fourthValue[1]= "";
	char fifthValue[1]= "";

	Measure tempMeasure;

	tempMeasure.max = 0;
	tempMeasure.min = 0;
	tempMeasure.avg = 0;

	tempMeasure.lastFive[4] = 0;
	tempMeasure.lastFive[3] = 0;
	tempMeasure.lastFive[2] = 0;
	tempMeasure.lastFive[1] = 0;
	tempMeasure.lastFive[0] = 0;


	double freq[100], dfreq[100];
	double ROC;
	int i = 99, j = 0;
	Line line_freq, line_roc;

	while(1) {

		// Receive frequency data from queue
		while(uxQueueMessagesWaiting(xDispFreqQueue) != 0){
			xQueueReceive(xDispFreqQueue, freq+i, 0 );
			xQueueReceive(xROCQueue, dfreq+i, 0 );
			i =	++i%100; //point to the next data (oldest) to be overwritten
		}

		xQueueReceive(xMeasurementQueue, (void *) &tempMeasure, 0);

		//clear old graph to draw new graph
		alt_up_pixel_buffer_dma_draw_box(pixel_buf, 101, 0, 639, 199, 0, 0);
		alt_up_pixel_buffer_dma_draw_box(pixel_buf, 101, 201, 639, 299, 0, 0);

		// Converts the values to be characters so that it can be plotted
		sprintf(cond1, "%.2f Hz", condition1_freqencyThreshold);
		sprintf(cond2, "%.2f Hz/s", condition2_freqencyThreshold);

		sprintf(sysTime, "%d s", _systemTime);
		sprintf(timeDiff, "%d ms", actualTimeDifference);
		sprintf(maxTime, "%d ms", tempMeasure.max);
		sprintf(minTime, "%d ms", tempMeasure.min);
		sprintf(avgTime, "%.2f ms", tempMeasure.avg);


		sprintf(firstValue, "%d", tempMeasure.lastFive[0]);
		sprintf(secondValue, "%d", tempMeasure.lastFive[1]);
		sprintf(thirdValue, "%d", tempMeasure.lastFive[2]);
		sprintf(fourthValue, "%d", tempMeasure.lastFive[3]);
		sprintf(fifthValue, "%d", tempMeasure.lastFive[4]);

		// Displays the last 5 results
		alt_up_char_buffer_string(char_buf, firstValue, 30, 44);
		alt_up_char_buffer_string(char_buf, secondValue, 34, 44);
		alt_up_char_buffer_string(char_buf, thirdValue, 38, 44);
		alt_up_char_buffer_string(char_buf, fourthValue,42, 44);
		alt_up_char_buffer_string(char_buf, fifthValue, 46, 44);


		// Displays the conditions
		alt_up_char_buffer_string(char_buf, cond1, 30, 40);
		alt_up_char_buffer_string(char_buf, cond2, 30, 42);

		// Displays all the calculated times
		alt_up_char_buffer_string(char_buf, timeDiff, 30, 48);
		alt_up_char_buffer_string(char_buf, maxTime, 30, 50);
		alt_up_char_buffer_string(char_buf, minTime, 30, 52);
		alt_up_char_buffer_string(char_buf, avgTime, 30, 54);

		if(!global_unstableFlag) {
			alt_up_char_buffer_string(char_buf, "Stable  ", 30, 46);
		} else {
			alt_up_char_buffer_string(char_buf, "Unstable", 30, 46);
		}

		alt_up_char_buffer_string(char_buf, sysTime,60, 1);


		// Plots the data
		for(j=0;j<99;++j){ //i here points to the oldest data, j loops through all the data to be drawn on VGA
			if (((int)(freq[(i+j)%100]) > MIN_FREQ) && ((int)(freq[(i+j+1)%100]) > MIN_FREQ)){
				//Calculate coordinates of the two data points to draw a line in between
				//Frequency plot
				line_freq.x1 = FREQPLT_ORI_X + FREQPLT_GRID_SIZE_X * j;
				line_freq.y1 = (int)(FREQPLT_ORI_Y - FREQPLT_FREQ_RES * (freq[(i+j)%100] - MIN_FREQ));

				line_freq.x2 = FREQPLT_ORI_X + FREQPLT_GRID_SIZE_X * (j + 1);
				line_freq.y2 = (int)(FREQPLT_ORI_Y - FREQPLT_FREQ_RES * (freq[(i+j+1)%100] - MIN_FREQ));

				//Frequency RoC plot
				line_roc.x1 = ROCPLT_ORI_X + ROCPLT_GRID_SIZE_X * j;
				line_roc.y1 = (int)(ROCPLT_ORI_Y - ROCPLT_ROC_RES * dfreq[(i+j)%100]);

				line_roc.x2 = ROCPLT_ORI_X + ROCPLT_GRID_SIZE_X * (j + 1);
				line_roc.y2 = (int)(ROCPLT_ORI_Y - ROCPLT_ROC_RES * dfreq[(i+j+1)%100]);

				//Draw
				alt_up_pixel_buffer_dma_draw_line(pixel_buf, line_freq.x1, line_freq.y1, line_freq.x2, line_freq.y2, 0x3ff << 0, 0);
				alt_up_pixel_buffer_dma_draw_line(pixel_buf, line_roc.x1, line_roc.y1, line_roc.x2, line_roc.y2, 0x3ff << 0, 0);
			}
		}

		vTaskDelay(10);

	}


}
