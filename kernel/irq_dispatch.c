#include "main.h"

extern void PIT_Handler(void);
extern void Keyboard_Handler(void);

void IrqDispatch(BYTE irq, void* regs)
{
    switch (irq)
    {
        case 0: 
            PIT_Handler();
            break;
            
        case 1:  
            Keyboard_Handler();
            break;
            
        default:
            {
                extern void PIC_SendEOI(BYTE irq);
                PIC_SendEOI(irq);
            }
            break;
    }
}
