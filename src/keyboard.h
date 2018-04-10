

#ifndef KEYBOARD_H_
#define KEYBOARD_H_

#include "altera_up_avalon_ps2.h"
#include "altera_up_ps2_keyboard.h"
#include "main.h"

unsigned int  initKeyboard;
unsigned int keyboardFlag;


typedef struct {

	char ascii;
	unsigned char key;
	KB_CODE_TYPE decode_mode;

} keyInput;

keyInput keys;

void ps2_isr (void* context, alt_u32 id);
void keyboardProcessor(void *pvParameters);

QueueHandle_t xKeyboardQueue;
SemaphoreHandle_t keyboardSemaphore;

#endif /* KEYBOARD_H_ */
