#include "main.h"

// Forward declarations
extern void PIT_Handler(void);
extern void Keyboard_Handler(void);

// IRQ dispatch function called from assembly IRQ stubs
void IrqDispatch(BYTE irq, void* regs)
{
    switch (irq)
    {
        case 0:  // PIT Timer
            PIT_Handler();
            break;
            
        case 1:  // Keyboard
            Keyboard_Handler();
            break;
            
        // Other IRQs can be added here as needed
        default:
            // Unhandled IRQ - just send EOI
            {
                extern void PIC_SendEOI(BYTE irq);
                PIC_SendEOI(irq);
            }
            break;
    }
}
