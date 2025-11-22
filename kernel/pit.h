#ifndef _PIT_H_
#define _PIT_H_

#include "main.h"

// PIT ports
#define PIT_CHANNEL0    0x40    
#define PIT_CHANNEL1    0x41   
#define PIT_CHANNEL2    0x42   
#define PIT_COMMAND     0x43   

// PIT command bits
#define PIT_MODE0       0x00   
#define PIT_MODE1       0x02    
#define PIT_MODE2       0x04   
#define PIT_MODE3       0x06   
#define PIT_MODE4       0x08   
#define PIT_MODE5       0x0A   

#define PIT_BINARY      0x00    
#define PIT_BCD         0x01    

#define PIT_LATCH       0x00   
#define PIT_LSB         0x10    
#define PIT_MSB         0x20   
#define PIT_BOTH        0x30    

#define PIT_CHANNEL_0   0x00
#define PIT_CHANNEL_1   0x40
#define PIT_CHANNEL_2   0x80
#define PIT_READBACK    0xC0

// PIT frequency
#define PIT_FREQUENCY   1193182  

// Function declarations
void PIT_Init(DWORD frequency);
QWORD PIT_GetTicks(void);

#endif 
