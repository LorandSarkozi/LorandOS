# MiniOS Hardware Interrupt Architecture Guide

## Table of Contents
1. [System Overview](#system-overview)
2. [Component 1: PIC (Programmable Interrupt Controller)](#component-1-pic)
3. [Component 2: PIT (Programmable Interval Timer)](#component-2-pit) - Coming next
4. [Component 3: Keyboard Driver](#component-3-keyboard) - Coming next
5. [Complete Interrupt Flow](#complete-interrupt-flow) - Coming next

---

## System Overview

This diagram shows how all components work together:

```
┌─────────────────────────────────────────────────────────────────┐
│                         MiniOS Kernel                           │
│                                                                 │
│  ┌──────────────┐    ┌──────────────┐    ┌──────────────┐     │
│  │   main.c     │    │     IDT      │    │  Handlers    │     │
│  │              │    │  (256 entries│    │              │     │
│  │ 1. PIC_Init()│───▶│              │    │ PIT_Handler  │     │
│  │ 2. PIT_Init()│    │ [0-31]: CPU  │◀───│ Keyboard_..  │     │
│  │ 3. Kbd_Init()│    │    Exceptions│    │ IrqDispatch  │     │
│  │ 4. __sti()   │    │              │    │              │     │
│  │              │    │ [32-47]: IRQs│    │              │     │
│  │ 5. __halt()  │    │   (Hardware) │    │              │     │
│  └──────────────┘    └──────┬───────┘    └──────▲───────┘     │
│                             │                     │             │
└─────────────────────────────┼─────────────────────┼─────────────┘
                              │                     │
                    ┌─────────▼─────────┐          │
                    │   Hardware IRQs   │          │
                    │                   │          │
                    │  IRQ0: PIT Timer──┼──────────┘
                    │  IRQ1: Keyboard───┼──────────┘
                    │  IRQ2-15: Other   │
                    └─────────▲─────────┘
                              │
                    ┌─────────┴─────────┐
                    │  PIC (8259 Chip)  │
                    │  Master + Slave   │
                    │  Remap 0→32       │
                    └───────────────────┘
```

**Key Initialization Flow:**
1. **PIC_Init()** - Remap IRQs, mask all interrupts
2. **PIT_Init(100)** - Program timer, unmask IRQ0
3. **Keyboard_Init()** - Initialize keyboard, unmask IRQ1
4. **__sti()** - Enable CPU interrupts (set IF flag)
5. **__halt()** loop - Wait for interrupts

---

## Component 1: PIC (Programmable Interrupt Controller)

### What is the PIC?

The **8259 PIC** is a hardware chip that manages interrupt requests (IRQs) from devices. Your PC has TWO PICs:
- **Master PIC** - Handles IRQ 0-7
- **Slave PIC** - Handles IRQ 8-15, connected to Master's IRQ2

```
Hardware Devices           PICs              CPU (IDT Vectors)
────────────────          ─────             ─────────────────

IRQ 0: Timer    ────┐
IRQ 1: Keyboard ────┤
IRQ 2: ─────────────┤──▶ Master PIC ────▶  [Before Remap]
IRQ 3: COM2     ────┤     (Port 0x20)       Vectors 0-7
IRQ 4: COM1     ────┤                       ❌ CONFLICT with
IRQ 5: LPT2     ────┤                          CPU Exceptions!
IRQ 6: Floppy   ────┤
IRQ 7: LPT1     ────┘
                                            [After Remap]
IRQ 8: RTC      ────┐                       Vectors 32-39 ✅
IRQ 9: Free     ────┤
IRQ10: Free     ────┤──▶ Slave PIC  ────▶  Vectors 40-47 ✅
IRQ11: Free     ────┤    (Port 0xA0)
IRQ12: Mouse    ────┤         │
IRQ13: FPU      ────┤         │
IRQ14: ATA1     ────┤         └──▶ Cascaded to
IRQ15: ATA2     ────┘              Master IRQ2
```

### Why Remap the PIC?

**Problem:** By default, the PIC sends IRQs to CPU vectors 0-15, but the CPU reserves vectors 0-31 for exceptions:

```
Vector Range    Default Use          Problem
─────────────   ────────────         ───────
0-31            CPU Exceptions       Reserved by CPU
                (#DE, #PF, #GP...)   

0-7             PIC IRQs (Master)    ❌ OVERLAP!
8-15            PIC IRQs (Slave)     ❌ OVERLAP!
```

**Solution:** Remap PIC to vectors 32-47:

```
Vector Range    After Remapping
─────────────   ───────────────
0-31            CPU Exceptions ✅
32-39           PIC Master IRQs ✅
40-47           PIC Slave IRQs ✅
48-255          Available
```

### PIC Initialization Sequence

The PIC is programmed with **ICWs (Initialization Command Words)**:

```c
void PIC_Init(void)
{
    // Step 1: Send ICW1 - Start initialization
    __outbyte(PIC1_COMMAND, 0x11);  // Init + ICW4 needed
    __outbyte(PIC2_COMMAND, 0x11);
    
    // Step 2: Send ICW2 - Set vector offsets
    __outbyte(PIC1_DATA, 0x20);     // Master → vectors 32-39
    __outbyte(PIC2_DATA, 0x28);     // Slave → vectors 40-47
    
    // Step 3: Send ICW3 - Set up cascading
    __outbyte(PIC1_DATA, 0x04);     // Master: slave on IRQ2
    __outbyte(PIC2_DATA, 0x02);     // Slave: cascade identity
    
    // Step 4: Send ICW4 - Set mode
    __outbyte(PIC1_DATA, 0x01);     // 8086 mode
    __outbyte(PIC2_DATA, 0x01);
    
    // Step 5: Mask all interrupts
    __outbyte(PIC1_DATA, 0xFF);     // Mask all on master
    __outbyte(PIC2_DATA, 0xFF);     // Mask all on slave
}
```

### PIC Masking Strategy

The **IRQ mask register** controls which interrupts are enabled:

```
Bit:  7   6   5   4   3   2   1   0
IRQ:  7   6   5   4   3   2   1   0   (Master PIC)
      │   │   │   │   │   │   │   └── 0: Timer (PIT)
      │   │   │   │   │   │   └────── 1: Keyboard
      │   │   │   │   │   └────────── 2: Cascade (Slave)
      │   │   │   │   └────────────── 3: COM2
      ...

Mask Value:
  1 = IRQ disabled (masked)
  0 = IRQ enabled (unmasked)

Example:
  0xFF (11111111) = All IRQs masked ❌
  0xFC (11111100) = Only IRQ0 & IRQ1 enabled ✅
```

**Our Strategy:**
1. **Initial state**: All IRQs masked (`0xFF`)
2. **After PIT_Init()**: Unmask IRQ0 only
3. **After Keyboard_Init()**: Unmask IRQ1 too

```c
// Unmask a specific IRQ
void PIC_ClearMask(BYTE irq)
{
    WORD port = (irq < 8) ? PIC1_DATA : PIC2_DATA;
    BYTE value = __inbyte(port) & ~(1 << (irq % 8));
    __outbyte(port, value);
}

// Example: Unmask IRQ0 (Timer)
PIC_ClearMask(0);  // Clears bit 0 in master mask
```

### Sending EOI (End of Interrupt)

After handling an interrupt, you **MUST** send EOI to the PIC:

```c
void PIC_SendEOI(BYTE irq)
{
    if (irq >= 8) {
        __outbyte(PIC2_COMMAND, 0x20);  // EOI to slave
    }
    __outbyte(PIC1_COMMAND, 0x20);      // EOI to master
}
```

**Why EOI is important:**
```
Without EOI:                    With EOI:
────────────                    ─────────
1. IRQ arrives                  1. IRQ arrives
2. CPU handles it               2. CPU handles it
3. PIC waits...                 3. Send EOI ✅
4. ❌ No more IRQs!             4. PIC ready for next IRQ ✅
   (PIC is stuck)               5. More IRQs can arrive
```

### PIC Port Summary

```
Port    Name            Purpose
────    ────            ───────
0x20    PIC1_COMMAND    Master PIC command
0x21    PIC1_DATA       Master PIC data (mask/ICW)
0xA0    PIC2_COMMAND    Slave PIC command
0xA1    PIC2_DATA       Slave PIC data (mask/ICW)
```

---

## Component 2: PIT (Programmable Interval Timer)

### What is the PIT?

The **8253/8254 PIT** is a hardware timer chip that generates periodic interrupts. It has 3 channels:
- **Channel 0** - Connected to IRQ0, generates interrupts ✅ (We use this!)
- **Channel 1** - DRAM refresh (legacy, not used in modern PCs)
- **Channel 2** - PC speaker control

### PIT Clock and Frequency

The PIT runs at a base frequency of **1.193182 MHz**:

```
PIT Base Clock: 1,193,182 Hz
        │
        └──▶ Divider ──▶ Output Frequency
              (programmable)

Example:
  Divisor = 11932  →  Output = 1,193,182 / 11932 = 100 Hz (10ms per tick)
  Divisor = 1193   →  Output = 1,193,182 / 1193  = 1000 Hz (1ms per tick)
```

### How the PIT Works

```
┌─────────────────────────────────────────────────────────┐
│  PIT Channel 0 (16-bit counter)                         │
│                                                          │
│  ┌──────────┐    Counts Down    ┌─────────────┐        │
│  │ Reload   │                    │   Counter   │        │
│  │ Value    │◀───────────────────│   Register  │        │
│  │ (Divisor)│                    │             │        │
│  └────┬─────┘                    └──────┬──────┘        │
│       │                                 │               │
│       │ Initial Load                    │ Decrement     │
│       │                                 │ on each clock │
│       └─────────────────────────────────┘               │
│                                         │               │
│                                  Reaches Zero?          │
│                                         │               │
│                                         ▼               │
│                              ┌──────────────────┐       │
│                              │  Generate IRQ0   │───────┼──▶ To PIC
│                              │  Reload Counter  │       │
│                              └──────────────────┘       │
└─────────────────────────────────────────────────────────┘
```

### PIT Modes

The PIT has 6 operating modes. We use **Mode 3 (Square Wave Generator)**:

```
Mode 0: Interrupt on Terminal Count
Mode 1: Hardware Re-triggerable One-Shot
Mode 2: Rate Generator
Mode 3: Square Wave Generator ✅ (Best for periodic interrupts)
Mode 4: Software Triggered Strobe
Mode 5: Hardware Triggered Strobe
```

**Mode 3 Output:**
```
Time:     0ms   10ms  20ms  30ms  40ms  50ms  (for 100Hz)
          │     │     │     │     │     │
Output:   ┌─────┐     ┌─────┐     ┌─────┐
          │     │     │     │     │     │
          ┘     └─────┘     └─────┘     └────
                ▲           ▲           ▲
                IRQ         IRQ         IRQ
```

### PIT Initialization

```c
void PIT_Init(DWORD frequency)  // frequency = desired Hz (e.g., 100)
{
    // Step 1: Calculate divisor
    DWORD divisor = 1193182 / frequency;
    //  For 100 Hz: divisor = 1193182 / 100 = 11932
    
    // Step 2: Split into low and high bytes
    BYTE low  = (BYTE)(divisor & 0xFF);         // Low 8 bits
    BYTE high = (BYTE)((divisor >> 8) & 0xFF);  // High 8 bits
    
    // Step 3: Send command byte
    //   Format: Channel | Access | Mode | BCD
    //   0x36 = 00 11 011 0
    //        = Ch0 | Both bytes | Mode 3 | Binary
    __outbyte(0x43, 0x36);
    
    // Step 4: Send divisor (low byte first, then high)
    __outbyte(0x40, low);
    __outbyte(0x40, high);
    
    // Step 5: Unmask IRQ0 in PIC
    PIC_ClearMask(0);
}
```

### PIT Command Byte Breakdown

```
Bit:    7   6 | 5   4 | 3   2   1 | 0
        ─────   ─────   ─────────   ─
        SC1 SC0│RW1 RW0│M2  M1  M0 │BCD

SC (Select Channel):
  00 = Channel 0 ✅
  01 = Channel 1
  10 = Channel 2
  11 = Read-back command

RW (Read/Write):
  00 = Counter latch
  01 = LSB only
  10 = MSB only
  11 = LSB then MSB ✅

M (Mode):
  000 = Mode 0
  011 = Mode 3 ✅
  ...

BCD:
  0 = Binary ✅
  1 = BCD

Our command: 0x36 = 0011 0110
             = Channel 0 | LSB+MSB | Mode 3 | Binary
```

### PIT Interrupt Handler

Every time the counter reaches zero, IRQ0 fires:

```c
static volatile QWORD gTickCount = 0;

void PIT_Handler(void)
{
    gTickCount++;              // Increment tick counter
    PIC_SendEOI(IRQ_TIMER);   // Tell PIC we're done
}
```

**Timing Calculation:**
```
Frequency = 100 Hz
Period = 1/100 = 0.01 seconds = 10 milliseconds per tick

After 1 second:
  gTickCount = 100 ticks
  
After 5 seconds:
  gTickCount = 500 ticks
  
Elapsed time = gTickCount / frequency
             = 500 / 100 = 5 seconds
```

### PIT Port Summary

```
Port    Purpose
────    ───────
0x40    Channel 0 data port (read/write counter)
0x41    Channel 1 data port
0x42    Channel 2 data port
0x43    Mode/Command register (write only)
```

### Complete PIT Flow

```
Initialization:                 Runtime:
───────────────                 ────────

1. Calculate divisor            1. PIT counter decrements
   (11932 for 100Hz)               on each clock tick
                                   
2. Send command (0x36)          2. Counter reaches 0
   to port 0x43                    
                                3. PIT sends signal to
3. Send low byte                   PIC IRQ0 line
   to port 0x40                    
                                4. PIC forwards to CPU
4. Send high byte                  vector 32 (if unmasked)
   to port 0x40                    
                                5. CPU calls irq0 handler
5. Unmask IRQ0 in PIC              
                                6. irq0 calls IrqDispatch(0)
6. Enable interrupts (__sti)       
                                7. IrqDispatch calls PIT_Handler()
                                   
                                8. PIT_Handler increments tick
                                   
                                9. Send EOI to PIC
                                   
                                10. Return (iretq)
                                   
                                11. PIT reloads counter,
                                    cycle repeats!
```

---

## Component 3: Keyboard Driver

### How PS/2 Keyboard Works

The keyboard controller (8042 chip) handles keyboard input:

```
┌──────────────┐
│   Keyboard   │
│  (Hardware)  │
└──────┬───────┘
       │ Scancodes
       ▼
┌──────────────┐
│  8042 Chip   │──IRQ1──▶ PIC ──▶ CPU (Vector 33)
│ (Controller) │
└──────┬───────┘
       │
   Port 0x60 (Data)
   Port 0x64 (Status/Command)
```

### Scancodes Explained

When you press a key, the keyboard sends a **scancode**:

```
Key Event          Scancode Sent
──────────         ─────────────
Press 'A'     →    0x1E
Release 'A'   →    0x9E  (0x1E + 0x80)

Press 'Shift' →    0x2A
Hold 'Shift'
Press 'A'     →    0x1E  (keyboard sends same code!)
Release 'A'   →    0x9E
Release 'Shift'→   0xAA  (0x2A + 0x80)
```

**Scancode Types:**
1. **Make code** - Sent when key is pressed (bit 7 = 0)
2. **Break code** - Sent when key is released (bit 7 = 1, make code + 0x80)
3. **Extended scancodes** - Prefixed with 0xE0 (for arrow keys, etc.)

### Scancode Tables

We use lookup tables (`scancode.h`) to convert scancodes to characters:

```c
// Example entries from _kkybrd_scancode_std[]
WORD _kkybrd_scancode_std[] = {
    KEY_UNKNOWN,      // 0x00
    KEY_ESCAPE,       // 0x01
    KEY_1,            // 0x02  → '1'
    KEY_2,            // 0x03  → '2'
    ...
    KEY_Q,            // 0x10  → 'q'
    KEY_W,            // 0x11  → 'w'
    KEY_E,            // 0x12  → 'e'
    ...
    KEY_A,            // 0x1E  → 'a'
    ...
};
```

### Keyboard Interrupt Flow

```
User Presses Key
      │
      ▼
┌─────────────────────────────────────────────────────────┐
│ 1. Keyboard sends scancode to 8042 controller          │
└───────────────────┬─────────────────────────────────────┘
                    ▼
┌─────────────────────────────────────────────────────────┐
│ 2. 8042 raises IRQ1 signal                             │
└───────────────────┬─────────────────────────────────────┘
                    ▼
┌─────────────────────────────────────────────────────────┐
│ 3. PIC forwards to CPU vector 33 (IRQ1 remapped)       │
└───────────────────┬─────────────────────────────────────┘
                    ▼
┌─────────────────────────────────────────────────────────┐
│ 4. CPU calls irq1 handler in __init.asm                │
└───────────────────┬─────────────────────────────────────┘
                    ▼
┌─────────────────────────────────────────────────────────┐
│ 5. irq1 calls IrqDispatch(1)                           │
└───────────────────┬─────────────────────────────────────┘
                    ▼
┌─────────────────────────────────────────────────────────┐
│ 6. IrqDispatch calls Keyboard_Handler()                │
└───────────────────┬─────────────────────────────────────┘
                    ▼
┌─────────────────────────────────────────────────────────┐
│ 7. Keyboard_Handler():                                 │
│    - Reads scancode from port 0x60                     │
│    - Checks for extended scancode (0xE0)               │
│    - Checks for key release (bit 7 set)                │
│    - Decodes scancode using lookup table               │
│    - Handles shift state                               │
│    - Displays character on VGA screen                  │
│    - Sends EOI to PIC                                  │
└───────────────────┬─────────────────────────────────────┘
                    ▼
┌─────────────────────────────────────────────────────────┐
│ 8. Return from interrupt (iretq)                       │
└─────────────────────────────────────────────────────────┘
```

### Keyboard Handler State Machine

```c
void Keyboard_Handler(void)
{
    BYTE scancode = __inbyte(0x60);  // Read from keyboard
    
    // State 1: Extended scancode prefix?
    if (scancode == 0xE0) {
        gExtendedScancode = 1;  // Remember for next interrupt
        PIC_SendEOI(IRQ_KEYBOARD);
        return;
    }
    
    // State 2: Key release?
    if (scancode & 0x80) {  // Bit 7 set = release
        BYTE keycode = scancode & 0x7F;  // Remove release bit
        if (keycode == 0x2A || keycode == 0x36) {
            gShiftPressed = 0;  // Release shift
        }
        gExtendedScancode = 0;
        PIC_SendEOI(IRQ_KEYBOARD);
        return;
    }
    
    // State 3: Key press - decode it
    if (gExtendedScancode) {
        key = _kkybrd_scancode_ext[scancode];  // Extended table
        gExtendedScancode = 0;
    } else {
        key = _kkybrd_scancode_std[scancode];  // Standard table
        
        // Check if shift was pressed
        if (scancode == 0x2A || scancode == 0x36) {
            gShiftPressed = 1;
        }
    }
    
    // State 4: Display character
    if (key >= 'a' && key <= 'z' && gShiftPressed) {
        key = key - 'a' + 'A';  // Convert to uppercase
    }
    // ... handle symbols with shift ...
    
    PutCharAtCursor((char)key);  // Display on VGA
    PIC_SendEOI(IRQ_KEYBOARD);
}
```

### Handling Special Keys

#### Shift Key

```
Shift Tracking:
──────────────

gShiftPressed = 0  (initially)

User presses left shift (0x2A):
  → gShiftPressed = 1

User presses 'a' (0x1E):
  → Scancode table returns 'a'
  → Check gShiftPressed == 1
  → Convert 'a' → 'A'
  → Display 'A'

User releases left shift (0xAA):
  → gShiftPressed = 0

User presses 'a' again:
  → Display 'a' (lowercase)
```

#### Number/Symbol Shift

```c
if (gShiftPressed) {
    switch (c) {
        case '1': c = '!'; break;
        case '2': c = '@'; break;
        case '3': c = '#'; break;
        case '4': c = '$'; break;
        // ... and so on
    }
}
```

#### Backspace

```c
if (key == KEY_BACKSPACE) {
    if (gCursorPos > 0) {
        gCursorPos--;
        gVideo[gCursorPos].c = ' ';  // Clear character
    }
}
```

#### Enter

```c
if (key == KEY_RETURN) {
    // Move to start of next line
    gCursorPos = ((gCursorPos / 80) + 1) * 80;
}
```

### VGA Text Mode Display

Characters are written directly to VGA memory at `0xB8000`:

```
VGA Memory Layout:
─────────────────

Address         Character   Attribute
───────         ─────────   ─────────
0xB8000         'H'         0x0F  (white on black)
0xB8002         'e'         0x0F
0xB8004         'l'         0x0F
0xB8006         'l'         0x0F
0xB8008         'o'         0x0F
...

Each position = 2 bytes:
  Byte 0: ASCII character
  Byte 1: Attribute (color)

Screen = 80 columns × 25 rows = 2000 characters = 4000 bytes
```

### Auto-Scroll Implementation

When cursor reaches bottom of screen:

```c
if (gCursorPos >= MAX_OFFSET) {  // MAX_OFFSET = 2000
    // Move all lines up by one
    for (int i = 0; i < MAX_OFFSET - MAX_COLUMNS; i++) {
        gVideo[i] = gVideo[i + MAX_COLUMNS];
    }
    
    // Clear last line
    for (int i = MAX_OFFSET - MAX_COLUMNS; i < MAX_OFFSET; i++) {
        gVideo[i].c = ' ';
        gVideo[i].color = 0x0F;
    }
    
    // Move cursor to start of last line
    gCursorPos = MAX_OFFSET - MAX_COLUMNS;
}
```

### Keyboard Port Summary

```
Port    Purpose
────    ───────
0x60    Data port (read scancode / write command)
0x64    Status port (read) / Command port (write)
```

### Complete Keyboard Example

```
User types: "Hi!"

1. Press 'H' (Shift + 'h'):
   - Scancode 0x2A (shift press)
   - Scancode 0x23 ('h' press)
   - Look up 0x23 → 'h'
   - Shift is pressed → Convert to 'H'
   - Display 'H' at gVideo[gCursorPos]
   - gCursorPos++

2. Release 'H':
   - Scancode 0xA3 (0x23 + 0x80)
   - Ignore (release event)

3. Release Shift:
   - Scancode 0xAA (0x2A + 0x80)
   - gShiftPressed = 0

4. Press 'i':
   - Scancode 0x17
   - Look up 0x17 → 'i'
   - Shift not pressed → Display 'i'
   - gCursorPos++

5. Press '1' with Shift:
   - Shift press (0x2A)
   - '1' press (0x02)
   - Look up 0x02 → '1'
   - Shift pressed → Convert '1' → '!'
   - Display '!'
   - gCursorPos++

Screen now shows: "Hi!"
```

---

## Complete Interrupt Flow - Putting It All Together

### The Big Picture: From Hardware to Screen

This section shows how ALL components work together from boot to keystroke display.

```
═══════════════════════════════════════════════════════════════════
                    BOOT & INITIALIZATION PHASE
═══════════════════════════════════════════════════════════════════

┌──────────────────────────────────────────────────────────────────┐
│  1. CPU starts in __init.asm                                     │
│     - Sets up paging                                             │
│     - Calls InitIDT() → Installs exception + IRQ handlers        │
│     - Calls KernelMain()                                         │
└──────────────┬───────────────────────────────────────────────────┘
               │
               ▼
┌──────────────────────────────────────────────────────────────────┐
│  2. KernelMain() initializes hardware                            │
│                                                                   │
│     PIC_Init():                                                  │
│       ├─ Remap IRQs: 0-15 → Vectors 32-47                       │
│       └─ Mask ALL interrupts (0xFF)                             │
│                                                                   │
│     PIT_Init(100):                                               │
│       ├─ Program timer for 100 Hz                               │
│       └─ Unmask IRQ0 ✅                                          │
│                                                                   │
│     Keyboard_Init():                                             │
│       ├─ Flush keyboard buffer                                  │
│       └─ Unmask IRQ1 ✅                                          │
│                                                                   │
│     __sti():                                                     │
│       └─ Enable CPU interrupts (RFLAGS.IF = 1) ✅               │
└──────────────┬───────────────────────────────────────────────────┘
               │
               ▼
┌──────────────────────────────────────────────────────────────────┐
│  3. System enters idle loop                                      │
│     while(1) { __halt(); }  ← Wait for interrupts               │
└──────────────────────────────────────────────────────────────────┘

═══════════════════════════════════════════════════════════════════
                         RUNTIME PHASE
═══════════════════════════════════════════════════════════════════

[Example: User presses 'A' key while timer is ticking]
```

### Scenario 1: Timer Interrupt (Every 10ms)

```
TIME: 0ms ─────────────────────────────────────────────────────────

Hardware Level:
┌──────────────┐
│  PIT Timer   │  Counter reaches 0
│  (Channel 0) │────────┐
└──────────────┘        │
                        ▼
                   ┌─────────┐
                   │   PIC   │  IRQ0 line activated
                   │ Master  │────────┐
                   └─────────┘        │
                                      ▼
                                 ┌─────────┐
                                 │   CPU   │  Vector 32 (IRQ0)
                                 └────┬────┘
                                      │
Software Level:                       ▼
┌─────────────────────────────────────────────────────────────┐
│ IDT[32] points to irq0 handler in __init.asm               │
└───────────────────┬─────────────────────────────────────────┘
                    │
                    ▼
┌─────────────────────────────────────────────────────────────┐
│ irq0: (Assembly)                                            │
│   1. Push all registers (RAX, RBX, ... R15)                │
│   2. Call IrqDispatch(0, &registers)                       │
└───────────────────┬─────────────────────────────────────────┘
                    │
                    ▼
┌─────────────────────────────────────────────────────────────┐
│ IrqDispatch(0): (C)                                         │
│   switch(0):                                                │
│     case 0: PIT_Handler(); break;                          │
└───────────────────┬─────────────────────────────────────────┘
                    │
                    ▼
┌─────────────────────────────────────────────────────────────┐
│ PIT_Handler(): (C)                                          │
│   1. gTickCount++  (now = 1, 2, 3, ...)                    │
│   2. PIC_SendEOI(0)                                         │
│      └─ __outbyte(0x20, 0x20)  → Tell PIC we're done       │
└───────────────────┬─────────────────────────────────────────┘
                    │
                    ▼
┌─────────────────────────────────────────────────────────────┐
│ Return to irq0: (Assembly)                                  │
│   1. Pop all registers (restore CPU state)                 │
│   2. iretq  → Return from interrupt                        │
└───────────────────┬─────────────────────────────────────────┘
                    │
                    ▼
┌─────────────────────────────────────────────────────────────┐
│ CPU resumes: __halt() in main loop                          │
│ Waits for next interrupt...                                 │
└─────────────────────────────────────────────────────────────┘

TIME: 10ms ─────────────────────────────────────────────────────────
                      (Cycle repeats)
```

### Scenario 2: Keyboard Interrupt (User presses 'A')

```
TIME: 5ms (between timer ticks) ───────────────────────────────────

User Action:
┌──────────────┐
│ User presses │
│  'A' key     │
└──────┬───────┘
       │
       ▼
Hardware Level:
┌──────────────┐
│   Keyboard   │  Sends scancode 0x1E
│   Hardware   │────────┐
└──────────────┘        │
                        ▼
┌──────────────┐
│ 8042 Chip    │  Stores in buffer
│ (Controller) │  Raises IRQ1 signal
└──────┬───────┘
       │
       ▼
┌──────────────┐
│     PIC      │  IRQ1 line activated
│   Master     │────────┐
└──────────────┘        │
                        ▼
                   ┌─────────┐
                   │   CPU   │  Interrupted! (even during __halt)
                   └────┬────┘  Vector 33 (IRQ1)
                        │
Software Level:         │
                        ▼
┌─────────────────────────────────────────────────────────────┐
│ IDT[33] points to irq1 handler in __init.asm               │
└───────────────────┬─────────────────────────────────────────┘
                    │
                    ▼
┌─────────────────────────────────────────────────────────────┐
│ irq1: (Assembly)                                            │
│   1. Push all registers                                    │
│   2. Call IrqDispatch(1, &registers)                       │
└───────────────────┬─────────────────────────────────────────┘
                    │
                    ▼
┌─────────────────────────────────────────────────────────────┐
│ IrqDispatch(1): (C)                                         │
│   switch(1):                                                │
│     case 1: Keyboard_Handler(); break;                     │
└───────────────────┬─────────────────────────────────────────┘
                    │
                    ▼
┌─────────────────────────────────────────────────────────────┐
│ Keyboard_Handler(): (C)                                     │
│                                                              │
│   Step 1: Read scancode                                    │
│     scancode = __inbyte(0x60)  →  0x1E                     │
│                                                              │
│   Step 2: Check extended?                                  │
│     if (scancode == 0xE0) → NO                             │
│                                                              │
│   Step 3: Check release?                                   │
│     if (scancode & 0x80) → NO (bit 7 = 0)                  │
│                                                              │
│   Step 4: Decode scancode                                  │
│     key = _kkybrd_scancode_std[0x1E]  →  'a'               │
│                                                              │
│   Step 5: Check shift                                      │
│     gShiftPressed == 0  →  Keep lowercase 'a'              │
│                                                              │
│   Step 6: Display character                                │
│     PutCharAtCursor('a'):                                  │
│       gVideo[gCursorPos].c = 'a'                           │
│       gVideo[gCursorPos].color = 0x0F                      │
│       gCursorPos++                                          │
│                                                              │
│   Step 7: Send EOI                                         │
│     PIC_SendEOI(1)                                         │
│       __outbyte(0x20, 0x20)  → Tell PIC we're done         │
└───────────────────┬─────────────────────────────────────────┘
                    │
                    ▼
┌─────────────────────────────────────────────────────────────┐
│ Return to irq1: (Assembly)                                  │
│   1. Pop all registers                                     │
│   2. iretq  → Return from interrupt                        │
└───────────────────┬─────────────────────────────────────────┘
                    │
                    ▼
┌─────────────────────────────────────────────────────────────┐
│ CPU resumes: __halt() in main loop                          │
│                                                              │
│ Screen now shows: "a" ✅                                    │
└─────────────────────────────────────────────────────────────┘
```

### Timing Diagram: Concurrent Interrupts

```
Time:  0ms    5ms   10ms   15ms   20ms   25ms   30ms
       │      │     │      │      │      │      │
PIT:   ●─────────── ●───────────── ●───────────── ●     (Every 10ms)
       │      │     │      │      │      │      │
Kbd:   ─────── ●─────────── ●────────────── ●──────     (User typing)
       │      │     │      │      │      │      │
CPU:   │      │     │      │      │      │      │
       ▼      ▼     ▼      ▼      ▼      ▼      ▼
       Tick   'a'  Tick   'b'   Tick   'c'   Tick

Legend:
  ● = Interrupt occurs
  CPU handles each interrupt one at a time (cannot interrupt an interrupt handler)
```

### What Happens if Both Fire Simultaneously?

```
PIC Priority (Lower IRQ = Higher Priority):

If IRQ0 (Timer) and IRQ1 (Keyboard) signal at EXACT same time:

┌─────────┐
│   PIC   │  IRQ0 has priority
│         │───▶ CPU handles IRQ0 first
│         │    (Timer interrupt)
│         │
│         │───▶ Then CPU handles IRQ1
└─────────┘    (Keyboard interrupt)

The PIC queues IRQ1 until IRQ0 is complete (EOI sent).
```

### Memory Map During Interrupts

```
Memory Address  Content
──────────────  ────────────────────────────────────
0x000B8000      VGA Text Buffer (2000 chars × 2 bytes)
                ├─ Modified by Keyboard_Handler
                └─ User sees characters appear here

0x001xxxxx      Kernel Code & Data
                ├─ PIC_Init, PIT_Init, Keyboard_Init
                ├─ Handlers: PIT_Handler, Keyboard_Handler
                ├─ gTickCount (volatile QWORD)
                ├─ gCursorPos (int)
                └─ gShiftPressed (BYTE)

0x00200000      Kernel Entry Point (from link script)

IDT Table       256 entries × 16 bytes = 4096 bytes
                ├─ [0-31]: Exception handlers (your previous work)
                └─ [32-47]: IRQ handlers (today's work)

Stack           Grows down from 0x200000
                ├─ Saved registers during interrupts
                └─ Local variables
```

### Interrupt Vector Map (Final State)

```
Vector  Type        Handler          Purpose
──────  ─────────   ──────────────   ─────────────────────
0-31    Exceptions  isr0-isr31       CPU errors (#PF, #GP, etc.)
                    ↓
                    IsrDispatch()    Dumps state & halts

32      IRQ0        irq0  ──────▶    PIT_Handler()
33      IRQ1        irq1  ──────▶    Keyboard_Handler()
34      IRQ2        irq2  ──────▶    (Unhandled)
35      IRQ3        irq3  ──────▶    (Unhandled)
36      IRQ4        irq4  ──────▶    (Unhandled)
37      IRQ5        irq5  ──────▶    (Unhandled)
38      IRQ6        irq6  ──────▶    (Unhandled)
39      IRQ7        irq7  ──────▶    (Unhandled)
40      IRQ8        irq8  ──────▶    (Unhandled)
41      IRQ9        irq9  ──────▶    (Unhandled)
42      IRQ10       irq10 ──────▶    (Unhandled)
43      IRQ11       irq11 ──────▶    (Unhandled)
44      IRQ12       irq12 ──────▶    (Unhandled)
45      IRQ13       irq13 ──────▶    (Unhandled)
46      IRQ14       irq14 ──────▶    (Unhandled)
47      IRQ15       irq15 ──────▶    (Unhandled)
                    ↓
                    IrqDispatch()    Routes to specific handler
                                    or sends EOI for unhandled

48-255  Available   (Not installed)
```

### Summary: The Complete Journey

```
1. BOOT
   └─▶ InitIDT() sets up interrupt table
   └─▶ KernelMain() initializes hardware

2. HARDWARE INIT
   └─▶ PIC: Remap & mask all
   └─▶ PIT: Program timer, unmask IRQ0
   └─▶ Keyboard: Flush buffer, unmask IRQ1
   └─▶ CPU: Enable interrupts (__sti)

3. IDLE LOOP
   └─▶ CPU: __halt() (low power, waiting)

4. INTERRUPT ARRIVES
   └─▶ Hardware ──▶ PIC ──▶ CPU Vector
   └─▶ IDT lookup ──▶ Assembly stub ──▶ C handler
   └─▶ Handle ──▶ Send EOI ──▶ Return (iretq)
   └─▶ Resume idle loop

5. REPEAT FOREVER
   └─▶ Timer ticks every 10ms
   └─▶ Keyboard responds to user
   └─▶ Screen updates in real-time
```


