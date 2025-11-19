#include "main.h"
#include "screen.h"
#include "logging.h"
#include "pic.h"
#include "pit.h"
#include "keyboard.h"

extern void TriggerPF(void);
extern void TriggerUD2(void);

void KernelMain()
{
    __magic();    // break into BOCHS
    
    __enableSSE();  // only for demo; in the future will be called from __init.asm

    ClearScreen();

    InitLogging();

    Log("Logging initialized!");

    HelloBoot();

 
    Log("Initializing PIC");
    PIC_Init();
    
   
    Log("Programming PIT timer");
    PIT_Init(100);
    
  
    Log("Initializing keyboard");
    Keyboard_Init();
    
    // Enable interrupts
    Log("Enabling interrupts");
    __sti();
    
    Log("System ready! Type on keyboard...");

    // Main idle loop
    while (1)
    {
        __halt();  // Wait for next interrupt
    }

    // TODO!!! Implement a simple console

    // TODO!!! read disk sectors using PIO mode ATA

    // TODO!!! Memory management: virtual, physical and heap memory allocators

//    Log("IDT loaded... Test triggering:");
//
//#if 1
//    Log("Triggering #PF (with error code)...");  // will return 0x000E for PF
//    TriggerPF();     
//#else
//    Log("Triggering #UD (no error code)..."); // will return 0x0006 for UD
//    TriggerUD2();    
//#endif
}
