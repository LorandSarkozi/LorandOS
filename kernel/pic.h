#ifndef _PIC_H_
#define _PIC_H_

#include "main.h"

// PIC ports
#define PIC1_COMMAND    0x20
#define PIC1_DATA       0x21
#define PIC2_COMMAND    0xA0
#define PIC2_DATA       0xA1

// PIC commands
#define PIC_EOI         0x20    

// ICW1 bits
#define ICW1_ICW4       0x01   
#define ICW1_SINGLE     0x02   
#define ICW1_INTERVAL4  0x04    
#define ICW1_LEVEL      0x08   
#define ICW1_INIT       0x10   

// ICW4 bits
#define ICW4_8086       0x01   
#define ICW4_AUTO       0x02   
#define ICW4_BUF_SLAVE  0x08   
#define ICW4_BUF_MASTER 0x0C  
#define ICW4_SFNM       0x10    

// Remap vectors
#define PIC1_OFFSET     0x20    
#define PIC2_OFFSET     0x28   

// IRQ numbers
#define IRQ_TIMER       0
#define IRQ_KEYBOARD    1
#define IRQ_CASCADE     2
#define IRQ_COM2        3
#define IRQ_COM1        4
#define IRQ_LPT2        5
#define IRQ_FLOPPY      6
#define IRQ_LPT1        7
#define IRQ_RTC         8
#define IRQ_FREE1       9
#define IRQ_FREE2       10
#define IRQ_FREE3       11
#define IRQ_MOUSE       12
#define IRQ_FPU         13
#define IRQ_ATA1        14
#define IRQ_ATA2        15

void PIC_Init(void);
void PIC_SendEOI(BYTE irq);
void PIC_SetMask(BYTE irq);
void PIC_ClearMask(BYTE irq);

#endif 
