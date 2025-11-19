#include "pic.h"
#include <intrin.h>

// IO delay for PIC operations
static void io_wait(void)
{
    // Port 0x80 is used for POST codes, writing to it causes a delay
    __outbyte(0x80, 0);
}

// Initialize and remap the PIC
void PIC_Init(void)
{
    BYTE mask1, mask2;
    
    // Save current masks
    mask1 = __inbyte(PIC1_DATA);
    mask2 = __inbyte(PIC2_DATA);
    
    // Start initialization sequence (ICW1)
    __outbyte(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4);
    io_wait();
    __outbyte(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4);
    io_wait();
    
    // ICW2: Set vector offsets
    __outbyte(PIC1_DATA, PIC1_OFFSET);  // Master PIC offset to 0x20 (32)
    io_wait();
    __outbyte(PIC2_DATA, PIC2_OFFSET);  // Slave PIC offset to 0x28 (40)
    io_wait();
    
    // ICW3: Set up cascading
    __outbyte(PIC1_DATA, 0x04);  // Tell Master PIC that slave is at IRQ2
    io_wait();
    __outbyte(PIC2_DATA, 0x02);  // Tell Slave PIC its cascade identity
    io_wait();
    
    // ICW4: Set mode
    __outbyte(PIC1_DATA, ICW4_8086);
    io_wait();
    __outbyte(PIC2_DATA, ICW4_8086);
    io_wait();
    
    // Mask all interrupts initially (we'll unmask specific ones after programming devices)
    __outbyte(PIC1_DATA, 0xFF);
    __outbyte(PIC2_DATA, 0xFF);
}

// Send End of Interrupt signal
void PIC_SendEOI(BYTE irq)
{
    if (irq >= 8)
    {
        // Send EOI to slave PIC
        __outbyte(PIC2_COMMAND, PIC_EOI);
    }
    // Always send EOI to master PIC
    __outbyte(PIC1_COMMAND, PIC_EOI);
}

// Mask (disable) an IRQ
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

// Unmask (enable) an IRQ
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
