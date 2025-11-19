#include "keyboard.h"
#include "pic.h"
#include "screen.h"
#include <intrin.h>

static BYTE gShiftPressed = 0;
static BYTE gExtendedScancode = 0;
static KEYCODE gLastKey = KEY_UNKNOWN;
static int gCursorPos = 0;

// Write to VGA at current cursor position
static void PutCharAtCursor(char c)
{
    extern PSCREEN gVideo;
    
    if (c == '\n')
    {
        gCursorPos = ((gCursorPos / MAX_COLUMNS) + 1) * MAX_COLUMNS;
    }
    else if (c == '\b')
    {
        if (gCursorPos > 0)
        {
            gCursorPos--;
            gVideo[gCursorPos].c = ' ';
            gVideo[gCursorPos].color = 0x0F;
        }
    }
    else
    {
        if (gCursorPos < MAX_OFFSET)
        {
            gVideo[gCursorPos].c = c;
            gVideo[gCursorPos].color = 0x0F;
            gCursorPos++;
        }
    }
    
    // Scroll if needed
    if (gCursorPos >= MAX_OFFSET)
    {
        // Simple scroll: move everything up one line
        for (int i = 0; i < MAX_OFFSET - MAX_COLUMNS; i++)
        {
            gVideo[i] = gVideo[i + MAX_COLUMNS];
        }
        // Clear last line
        for (int i = MAX_OFFSET - MAX_COLUMNS; i < MAX_OFFSET; i++)
        {
            gVideo[i].c = ' ';
            gVideo[i].color = 0x0F;
        }
        gCursorPos = MAX_OFFSET - MAX_COLUMNS;
    }
}

// Keyboard interrupt handler
void Keyboard_Handler(void)
{
    BYTE scancode;
    KEYCODE key;
    
    // Read scancode from keyboard
    scancode = __inbyte(KBD_DATA_PORT);
    
    // Check for extended scancode prefix (0xE0)
    if (scancode == KBD_SCANCODE_EXTENDED)
    {
        gExtendedScancode = 1;
        PIC_SendEOI(IRQ_KEYBOARD);
        return;
    }
    
    // Check if it's a key release (bit 7 set)
    if (scancode & KBD_SCANCODE_RELEASE)
    {
        BYTE keycode = scancode & 0x7F;
        
        // Handle shift release
        if (keycode == 0x2A || keycode == 0x36)
        {
            gShiftPressed = 0;
        }
        
        gExtendedScancode = 0;
        PIC_SendEOI(IRQ_KEYBOARD);
        return;
    }
    
    // Key press
    if (gExtendedScancode)
    {
        // Extended scancode
        if (scancode < sizeof(_kkybrd_scancode_ext) / sizeof(WORD))
        {
            key = (KEYCODE)_kkybrd_scancode_ext[scancode];
        }
        else
        {
            key = KEY_UNKNOWN;
        }
        gExtendedScancode = 0;
    }
    else
    {
        // Standard scancode
        if (scancode < sizeof(_kkybrd_scancode_std) / sizeof(WORD))
        {
            key = (KEYCODE)_kkybrd_scancode_std[scancode];
        }
        else
        {
            key = KEY_UNKNOWN;
        }
        
        // Handle shift press
        if (scancode == 0x2A || scancode == 0x36)
        {
            gShiftPressed = 1;
        }
    }
    
    gLastKey = key;
    
    // Display the key if it's printable
    if (key != KEY_UNKNOWN && key < 256)
    {
        char c = (char)key;
        
        // Handle shift for letters
        if (c >= 'a' && c <= 'z' && gShiftPressed)
        {
            c = c - 'a' + 'A';
        }
        // Handle shift for numbers and symbols
        else if (gShiftPressed)
        {
            switch (c)
            {
                case '1': c = '!'; break;
                case '2': c = '@'; break;
                case '3': c = '#'; break;
                case '4': c = '$'; break;
                case '5': c = '%'; break;
                case '6': c = '^'; break;
                case '7': c = '&'; break;
                case '8': c = '*'; break;
                case '9': c = '('; break;
                case '0': c = ')'; break;
                case '-': c = '_'; break;
                case '=': c = '+'; break;
                case '[': c = '{'; break;
                case ']': c = '}'; break;
                case '\\': c = '|'; break;
                case ';': c = ':'; break;
                case '\'': c = '"'; break;
                case ',': c = '<'; break;
                case '.': c = '>'; break;
                case '/': c = '?'; break;
                case '`': c = '~'; break;
            }
        }
        
        PutCharAtCursor(c);
    }
    else if (key == KEY_RETURN)
    {
        PutCharAtCursor('\n');
    }
    else if (key == KEY_BACKSPACE)
    {
        PutCharAtCursor('\b');
    }
    
    PIC_SendEOI(IRQ_KEYBOARD);
}

// Initialize keyboard
void Keyboard_Init(void)
{
    // The keyboard is already initialized by BIOS in most cases
    // We just need to enable it and unmask IRQ1
    
    // Flush the keyboard buffer
    while (__inbyte(KBD_STATUS_PORT) & KBD_STATUS_OUT_FULL)
    {
        __inbyte(KBD_DATA_PORT);
    }
    
    // Unmask IRQ1 (keyboard) after programming
    PIC_ClearMask(IRQ_KEYBOARD);
}

// Get last pressed key
KEYCODE Keyboard_GetLastKey(void)
{
    KEYCODE key = gLastKey;
    gLastKey = KEY_UNKNOWN;
    return key;
}
