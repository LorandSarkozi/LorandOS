#ifndef _KEYBOARD_H_
#define _KEYBOARD_H_

#include "main.h"
#include "scancode.h"

// Keyboard ports
#define KBD_DATA_PORT       0x60
#define KBD_STATUS_PORT     0x64
#define KBD_COMMAND_PORT    0x64

// Keyboard commands
#define KBD_CMD_SET_LED     0xED
#define KBD_CMD_ECHO        0xEE
#define KBD_CMD_SCANCODE    0xF0
#define KBD_CMD_IDENTIFY    0xF2
#define KBD_CMD_TYPEMATIC   0xF3
#define KBD_CMD_ENABLE      0xF4
#define KBD_CMD_DISABLE     0xF5
#define KBD_CMD_RESET       0xFF

// Keyboard status bits
#define KBD_STATUS_OUT_FULL 0x01
#define KBD_STATUS_IN_FULL  0x02

// Special scancodes
#define KBD_SCANCODE_EXTENDED   0xE0
#define KBD_SCANCODE_RELEASE    0x80

// Function declarations
void Keyboard_Init(void);
void Keyboard_Handler(void);
KEYCODE Keyboard_GetLastKey(void);

#endif // _KEYBOARD_H_
