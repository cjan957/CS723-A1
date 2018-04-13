#include "keyboard.h"
#include "ConditionChecking.h"


void ps2_isr (void* context, alt_u32 id)
{

	keyboardFlag++;

	char ascii;
	int status = 0;
	unsigned char key = 0;
	KB_CODE_TYPE decode_mode;
	status = decode_scancode(context, &decode_mode , &key , &ascii) ;

	if (initKeyboard == 1) {
		keyboardFlag = 0;
		initKeyboard = 0;
	}

	if (keyboardFlag  > 2) {
		keyboardFlag = 0;
		if ( status == 0 ) //success
		{
			keys.decode_mode = decode_mode;
			keys.key = key;
			keys.ascii = ascii;

			xSemaphoreGiveFromISR(xKeyboardSemaphore, pdFALSE);

		}
	}
}

void keyboardProcessor(void *pvParameters) {

	int tempChar[2] = {0};
	unsigned int index = 0;

	/* Condition 0: Not checking
	 * Condition 1: Changing instantaneous frequency
	 * Condition 2: Changing Rate of Change
	 */
	unsigned int condition = 0;

	while(1) {

		if(( xSemaphoreTake( xKeyboardSemaphore,10) == pdTRUE ) && (_maintenanceMode)){

			// print out the result
			switch ( keys.decode_mode )
			{
			case KB_ASCII_MAKE_CODE :
				// Convert to ascii
				//printf("TEST: %d\n", keys.ascii - 48);

				if (((int)keys.ascii >= 48) && ((int)keys.ascii <= 57)) {

					if (index > 2) {
						index = 0;
					} else {
						tempChar[index] = keys.ascii - 48;
						index++;
					}
				}

				break ;
			case KB_LONG_BINARY_MAKE_CODE :
				// do nothing
			case KB_BINARY_MAKE_CODE :

				// TODO: Must prevent from changing conditions incorrectly
				// Press F1 for changing instantaneous frequency
				if (keys.key == 5) {
					condition = 1;
					printf ("F1 PRESSED!\n");
				}
				// Press F2 for changing ROC
				else if (keys.key == 6) {
					condition = 2;
					printf ("F2 PRESSED!\n");
				}
				// Enter key
				else if (keys.key == 90 ) {
					printf ("Enter PRESSED!\n");

					int i, k = 0;
					// Set condition threshold
					switch(condition) {

					case 1:

						for (i = 0; i < 2; i++) {
							k = 10 * k + tempChar[i];
						}
						condition1_freqencyThreshold = k;
						printf("Frequency Value: %f\n", condition1_freqencyThreshold);


						break;
					case 2:
						for (i = 0; i < 2; i++) {
							k = 10 * k + tempChar[i];
						}
						condition2_freqencyThreshold = k;
						printf("Threshold Value: %f\n", condition2_freqencyThreshold);
						break;
					default:
						break;
					}

					condition = 0;
					index = 0;

				}
				break ;
			case KB_BREAK_CODE :
				// do nothing
			default :
				//printf ( "DEFAULT   : %x\n", keys.key  ) ;
				break ;
			}

			// Clear the struct
			keys.decode_mode = 6;
			keys.key = 0;

		}
	}

}
