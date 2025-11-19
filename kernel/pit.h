#ifndef _PIT_H_
#define _PIT_H_

#include "main.h"

// PIT ports
#define PIT_CHANNEL0    0x40    // Channel 0 data port
#define PIT_CHANNEL1    0x41    // Channel 1 data port
#define PIT_CHANNEL2    0x42    // Channel 2 data port
#define PIT_COMMAND     0x43    // Mode/Command register

// PIT command bits
#define PIT_MODE0       0x00    // Interrupt on terminal count
#define PIT_MODE1       0x02    // Hardware re-triggerable one-shot
#define PIT_MODE2       0x04    // Rate generator
#define PIT_MODE3       0x06    // Square wave generator
#define PIT_MODE4       0x08    // Software triggered strobe
#define PIT_MODE5       0x0A    // Hardware triggered strobe

#define PIT_BINARY      0x00    // Binary mode
#define PIT_BCD         0x01    // BCD mode

#define PIT_LATCH       0x00    // Latch count value command
#define PIT_LSB         0x10    // lobyte only
#define PIT_MSB         0x20    // hibyte only
#define PIT_BOTH        0x30    // lobyte/hibyte

#define PIT_CHANNEL_0   0x00
#define PIT_CHANNEL_1   0x40
#define PIT_CHANNEL_2   0x80
#define PIT_READBACK    0xC0

// PIT frequency
#define PIT_FREQUENCY   1193182  // Hz (1.193182 MHz)

// Function declarations
void PIT_Init(DWORD frequency);
QWORD PIT_GetTicks(void);

#endif // _PIT_H_
