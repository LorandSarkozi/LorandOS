# LorandOS

A minimal operating system kernel written in C/Assembly for x86/x64 architecture with a functional CLI and text editor.

## Project Structure

```
LorandOS/
├── kernel/                      # Main kernel directory
│   ├── __init.asm              # Assembly entry point and initialization
│   ├── main.c/h                # Kernel entry point and initialization
│   │
│   ├── Core Drivers
│   │   ├── screen.c/h          # VGA text mode screen driver
│   │   ├── keyboard.c/h        # PS/2 keyboard driver with CapsLock/Shift
│   │   ├── pic.c/h             # Programmable Interrupt Controller
│   │   ├── pit.c/h             # Programmable Interval Timer
│   │   ├── rtc.c/h             # Real-Time Clock (CMOS) driver
│   │   │
│   │   ├── irq_dispatch.c      # Hardware interrupt dispatcher
│   │   └── isr_dispatch.cpp    # Exception handler dispatcher
│   │
│   ├── CLI System (NEW)
│   │   └── cli.c/h             # Command-line interface with editor
│   │
│   ├── Utilities
│   │   ├── string.c/h          # String manipulation and formatting
│   │   ├── logging.c/h         # Debug logging system
│   │   ├── scancode.h          # Keyboard scancode mappings
│   │   └── bochs_map.h         # Bochs debugger mappings
│   │
│   ├── Build Configuration
│   │   ├── kernel.vcxproj      # Visual Studio project file
│   │   ├── kernel.vcxproj.filters
│   │   └── postbuild.cmd       # Post-build script
│   │
│   └── utils/                  # Build utilities (NASM, etc.)
│
└── README.md                   # This file
```

## New Features Added

### 🖥️ Command-Line Interface (CLI)

A fully functional command-line interface with the following capabilities:

#### **Commands**

| Command | Description |
|---------|-------------|
| `clear` or `cls` | Clears the screen |
| `time` | Shows system uptime (in ticks) and current date/time from RTC |
| `edit` | Enters full-screen text editor mode |

#### **CLI Features**

- ✅ **Command history** - Navigate through last 10 commands using ↑/↓ arrow keys
- ✅ **Auto-scroll** - Automatically scrolls when text exceeds screen height
- ✅ **Backspace support** - Delete characters with backspace
- ✅ **Command prompt** - Green `> ` prompt for user input

### 📝 Text Editor

Press `edit` to enter a full-screen text editor:

- **Arrow keys** - Move cursor (←, →, ↑, ↓)
- **Typing** - Direct text input at cursor position
- **Enter** - New line
- **Backspace** - Delete character before cursor
- **ESC** - Exit editor and return to CLI
- **Buffer** - Supports 50 lines total (25 visible + 25 buffered)
- **Scroll** - Scroll up/down beyond visible screen area
- **Persistent** - Editor state maintained between sessions

### ⌨️ Keyboard Enhancements

- **CapsLock** - Toggle uppercase/lowercase (persistent toggle)
- **Shift modifier** - Hold for uppercase letters and special characters
  - Letters: `a-z` → `A-Z`
  - Numbers: `1-9,0` → `!@#$%^&*()`
  - Symbols: `-=[];\',./` → `_+{}:"|<>?`
- **Key release detection** - Proper handling of modifier key releases

### 🕐 Real-Time Clock (RTC)

New CMOS RTC driver provides:

- Date reading (day, month, year)
- Time reading (hour, minute, second)
- Format: `DD/MM/YY HH:MM:SS`
- Automatic BCD to binary conversion
- Update-in-progress detection for accurate readings

## Technical Details

### Memory Map

- **Kernel base address**: `0x200000`
- **Video memory**: `0xB8000` (VGA text mode, 80x25)
- **Stack**: Set up in `__init.asm`

### Interrupt Handling

| IRQ | Handler | Description |
|-----|---------|-------------|
| IRQ 0 | `PIT_Handler` | Programmable Interval Timer (100 Hz) |
| IRQ 1 | `Keyboard_Handler` | PS/2 Keyboard input |

### Screen Configuration

- **Resolution**: 80 columns × 25 rows
- **Color format**: 4-bit background + 4-bit foreground
- **Default colors**:
  - CLI prompt: Green (`0x0A`)
  - Normal text: White on black (`0x0F`)
  - Time output: Yellow (`0x0E`)

### CLI Architecture

```
Keyboard IRQ → Keyboard_Handler() → CLI_HandleKey()
                                          ↓
                                    Command Mode ←→ Edit Mode
                                          ↓
                              CLI_ProcessCommand()
                                          ↓
                          ┌─────────────┬─────────────┬──────────────┐
                          ↓             ↓             ↓              ↓
                   clear/cls         time          edit        (unknown)
```

### Data Structures

```c
// CLI State
typedef struct _CLI_STATE {
    CLI_MODE mode;                                      // Command or Edit mode
    char commandBuffer[128];                            // Current command
    char history[10][128];                              // Command history
    char editBuffer[50][80];                            // Editor text buffer
    DWORD cursorPosition;                               // Screen cursor pos
    // ... (keyboard state, edit cursor, scroll offset)
} CLI_STATE;

// Date/Time
typedef struct _DATETIME {
    BYTE second, minute, hour;
    BYTE day, month;
    WORD year;
} DATETIME;
```

## Building the Project

### Prerequisites

- Visual Studio 2022 (or compatible)
- NASM assembler (included in `utils/`)
- Windows SDK

### Build Steps

1. Open `kernel\kernel.sln` in Visual Studio
2. Select **Release** configuration (x86 or x64)
3. Build → Build Solution (F7)
4. Output: `kernel\Release\kernel.exe`

### Running

Use the provided `test.cmd` script or run in an emulator:
- **Bochs** - x86 emulator with debugging
- **QEMU** - Fast x86/x64 emulator
- **VirtualBox** - Full VM (requires bootloader)

## How to Use the New CLI Features

### First Boot

After building and running the kernel, you'll see:
1. System initialization messages (PIC, PIT, Keyboard)
2. CLI welcome message
3. Command prompt: `> `

The CLI is **automatically started** - just start typing commands!

### Available Commands

#### 1. **`time`** - Display System Time
Shows two pieces of information:
- **Ticks since boot**: Internal timer counter (100 ticks per second)
- **Current date/time**: Real-time clock from CMOS in format `DD/MM/YY HH:MM:SS`

```
> time
Ticks since boot: 12345
Current time: 22/11/24 21:30:45
> _
```

#### 2. **`clear`** or **`cls`** - Clear Screen
Clears the entire screen and resets cursor to top-left

```
> clear
[Screen clears]
> _
```

#### 3. **`edit`** - Text Editor
Enters full-screen text editor mode:
- Type anywhere on screen
- **Arrow keys** (↑↓←→) - Move cursor
- **Enter** - New line
- **Backspace** - Delete previous character
- **ESC** - Exit editor and return to CLI

```
> edit
[Screen clears, editor starts]
[Type your text here...]
[Press ESC when done]
> _
```

### Advanced Features

#### Command History
- **↑ (Up Arrow)**: Previous command (up to 10 commands)
- **↓ (Down Arrow)**: Next command
- Commands are automatically saved as you type them

```
> time
...
> clear
...
> ↑
> clear█  [previous command loaded]
> ↑
> time█  [even earlier command]
```

#### Keyboard Modifiers

**CapsLock Toggle**:
- Press CapsLock once to enable (stays on)
- Press again to disable
- When ON: letters typed are uppercase
- Works independently of Shift

**Shift Key** (hold while typing):
- Letters: `a-z` → `A-Z`
- Numbers: `1234567890` → `!@#$%^&*()`
- Symbols:
  - `-` → `_`
  - `=` → `+`
  - `[` → `{`
  - `]` → `}`
  - `;` → `:`
  - `'` → `"`
  - `,` → `<`
  - `.` → `>`
  - `/` → `?`

**CapsLock + Shift**:
- Inverts: if CapsLock is ON, Shift makes it lowercase
- Useful for typing like normal keyboards

#### Auto-Scroll
- When text reaches the bottom of the screen, it automatically scrolls up
- No manual intervention needed
- Works in both command mode and editor mode

### Example Session

```
[Boot sequence...]
MiniOS CLI v1.0
Type 'clear' to clear screen, 'time' for system time, 'edit' for editor
> time
Ticks since boot: 12345
Current time: 22/11/24 21:30:45
> edit
Hello World!
This is my OS.
[Press ESC]
Exited edit mode
> ↑↑
> time
Ticks since boot: 67890
Current time: 22/11/24 21:35:12
> clear
[Screen clears]
> _
```

## Key Files Description

### Core System

- **`__init.asm`** - Assembly bootstrap, sets up GDT, IDT, stack, and jumps to C
- **`main.c`** - Kernel initialization sequence and main loop
- **`irq_dispatch.c`** - Routes hardware IRQs to appropriate handlers
- **`isr_dispatch.cpp`** - Routes CPU exceptions to handlers

### Drivers

- **`screen.c`** - VGA text mode interface, cursor control, scrolling
- **`keyboard.c`** - PS/2 keyboard driver with scancode translation
- **`pic.c`** - 8259 PIC initialization and IRQ masking
- **`pit.c`** - Timer initialization and tick counter
- **`rtc.c`** - CMOS clock reading with BCD conversion

### CLI System

- **`cli.c`** - Complete CLI implementation
  - Command parser and executor
  - Full-screen editor with buffer management
  - Command history ring buffer
  - Keyboard input handling for both modes
  - Screen refresh and cursor management

### Utilities

- **`string.c`** - Integer to string conversion (`itoa`), string formatting (`snprintf`)
- **`logging.c`** - Debug output to screen during boot
- **`scancode.h`** - Complete PS/2 scancode to keycode translation tables

## Future Enhancements

- [ ] File system support
- [ ] ATA/IDE disk driver
- [ ] Memory management (virtual, physical, heap)
- [ ] More CLI commands (ls, cat, etc.)
- [ ] Editor features (copy/paste, search, save/load)
- [ ] Better scroll handling with scroll bars
- [ ] Tab completion for commands
- [ ] Colored syntax highlighting in editor

## Development Notes

### Debugging

- Use Bochs debugger with `__magic()` breakpoints (`XCHG BX, BX`)
- Check `logging.c` output during initialization
- PIC and PIT must be properly initialized before enabling interrupts

### Known Limitations

- No file system - editor text is volatile (lost on reboot)
- Fixed 80x25 text mode only
- No mouse support
- Single-threaded (no multitasking)
- x86/x64 real mode only (no protected mode features used)

## License

Educational/Learning Project

## Credits

Developed as a minimal OS kernel learning project with focus on:
- Hardware interaction (VGA, PS/2, PIT, PIC, CMOS)
- Interrupt handling
- Command-line interface design
- Text editor implementation
