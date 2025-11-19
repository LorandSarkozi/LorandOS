#include "pit.h"
#include "pic.h"
#include <intrin.h>

static volatile QWORD gTickCount = 0;

// PIT timer interrupt handler (called from IRQ dispatcher)
void PIT_Handler(void)
{
    gTickCount++;
    PIC_SendEOI(IRQ_TIMER);
}

// Initialize PIT channel 0 to generate periodic interrupts
void PIT_Init(DWORD frequency)
{
    DWORD divisor;
    BYTE low, high;
    
    // Calculate divisor
    divisor = PIT_FREQUENCY / frequency;
    
    low = (BYTE)(divisor & 0xFF);
    high = (BYTE)((divisor >> 8) & 0xFF);
    
    // Send command: Channel 0, lobyte/hibyte, Mode 3 (square wave), binary
    __outbyte(PIT_COMMAND, PIT_CHANNEL_0 | PIT_BOTH | PIT_MODE3 | PIT_BINARY);
    
    // Send divisor
    __outbyte(PIT_CHANNEL0, low);
    __outbyte(PIT_CHANNEL0, high);
    
    // Now unmask IRQ0 (PIT) after programming the device
    PIC_ClearMask(IRQ_TIMER);
}

// Get current tick count
QWORD PIT_GetTicks(void)
{
    return gTickCount;
}
