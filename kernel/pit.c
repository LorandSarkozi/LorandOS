#include "pit.h"
#include "pic.h"
#include <intrin.h>

static volatile QWORD gTickCount = 0;

void PIT_Handler(void)
{
    gTickCount++;
    PIC_SendEOI(IRQ_TIMER);
}

void PIT_Init(DWORD frequency)
{
    DWORD divisor;
    BYTE low, high;
    

    divisor = PIT_FREQUENCY / frequency;
    
    low = (BYTE)(divisor & 0xFF);
    high = (BYTE)((divisor >> 8) & 0xFF);
   
    __outbyte(PIT_COMMAND, PIT_CHANNEL_0 | PIT_BOTH | PIT_MODE3 | PIT_BINARY);

    __outbyte(PIT_CHANNEL0, low);
    __outbyte(PIT_CHANNEL0, high);

    PIC_ClearMask(IRQ_TIMER);
}

QWORD PIT_GetTicks(void)
{
    return gTickCount;
}
