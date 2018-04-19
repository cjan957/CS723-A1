# COMPSYS 723 Assignment 1
## Group 23
## Chanokpol Janveerawat and Josh Asi

### Hardware Requirements
- DE2-115 Development Board
- PS/2 Keyboard
- VGA monitor

### Set up
- Make a new Nios II Application and BSP from Template
- Select the Nios II CPU 
- Specify a project name
- Choose a suitable project location
- Choose New Hello World
- Copy the ‘src’ folder into where the software folder resides
- Refresh by pressing F5
- Build project
- Right click the project folder and then Run as NIOS II hardware

_Please note that depending on the keyboard used, the counter which increments the number of key presses should be adjusted. Some keyboards duplicate the key press twice and some duplicate it three times. If the keyboards are not working correctly when testing, please make sure to adjust the keyboardFlag condition in the function **ps2_isr** in **keyboard.c.** For example (keyboardFlag > 1) instead of (keyboardFlag > 2)._
