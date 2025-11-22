#include "pic.h"
#include <intrin.h>

static void io_wait(void)
{
    __outbyte(0x80, 0);
}

void PIC_Init(void)
{
    BYTE mask1, mask2;
    
 
    mask1 = __inbyte(PIC1_DATA);
    mask2 = __inbyte(PIC2_DATA);
    
    __outbyte(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4);
    io_wait();
    __outbyte(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4);
    io_wait();
    
    __outbyte(PIC1_DATA, PIC1_OFFSET);  
    io_wait();
    __outbyte(PIC2_DATA, PIC2_OFFSET);  
    io_wait();
    
    __outbyte(PIC1_DATA, 0x04); 
    io_wait();
    __outbyte(PIC2_DATA, 0x02); 
    io_wait();
 
    __outbyte(PIC1_DATA, ICW4_8086);
    io_wait();
    __outbyte(PIC2_DATA, ICW4_8086);
    io_wait();
    
    __outbyte(PIC1_DATA, 0xFF);
    __outbyte(PIC2_DATA, 0xFF);
}

void PIC_SendEOI(BYTE irq)
{
    if (irq >= 8)
    {
        __outbyte(PIC2_COMMAND, PIC_EOI);
    }
    __outbyte(PIC1_COMMAND, PIC_EOI);
}

void PIC_SetMask(BYTE irq)
{
    WORD port;
    BYTE value;
    
    if (irq < 8)
    {
        port = PIC1_DATA;
    }
    else
    {
        port = PIC2_DATA;
        irq -= 8;
    }
    
    value = __inbyte(port) | (1 << irq);
    __outbyte(port, value);
}

void PIC_ClearMask(BYTE irq)
{
    WORD port;
    BYTE value;
    
    if (irq < 8)
    {
        port = PIC1_DATA;
    }
    else
    {
        port = PIC2_DATA;
        irq -= 8;
    }
    
    value = __inbyte(port) & ~(1 << irq);
    __outbyte(port, value);
}
