#include "keyboard.h"
#include "pic.h"
#include "screen.h"
#include "cli.h"
#include <intrin.h>

static BYTE gShiftPressed = 0;
static BYTE gExtendedScancode = 0;
static KEYCODE gLastKey = KEY_UNKNOWN;
static BYTE gCapsLockOn = 0;

void Keyboard_Handler(void)
{
    BYTE scancode;
    KEYCODE key;
    
    scancode = __inbyte(KBD_DATA_PORT);
    
    if (scancode == KBD_SCANCODE_EXTENDED)
    {
        gExtendedScancode = 1;
        PIC_SendEOI(IRQ_KEYBOARD);
        return;
    }
    

    if (scancode & KBD_SCANCODE_RELEASE)
    {
        BYTE keycode = scancode & 0x7F;
        
        if (keycode == 0x2A || keycode == 0x36)
        {
            gShiftPressed = 0;
        }
        
        gExtendedScancode = 0;
        PIC_SendEOI(IRQ_KEYBOARD);
        return;
    }
    
    if (gExtendedScancode)
    {
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
        if (scancode < sizeof(_kkybrd_scancode_std) / sizeof(WORD))
        {
            key = (KEYCODE)_kkybrd_scancode_std[scancode];
        }
        else
        {
            key = KEY_UNKNOWN;
        }
        
        if (scancode == 0x2A || scancode == 0x36)
        {
            gShiftPressed = 1;
        }
    }
    
    gLastKey = key;
    
    // Handle CapsLock toggle
    if (key == KEY_CAPSLOCK)
    {
        gCapsLockOn = !gCapsLockOn;
        PIC_SendEOI(IRQ_KEYBOARD);
        return;
    }

    // Determine character to send to CLI
    char c = 0;
    
    // Check if it's a special key (not printable)
    if (key == KEY_RETURN || key == KEY_BACKSPACE || key == KEY_ESCAPE ||
        key == KEY_UP || key == KEY_DOWN || key == KEY_LEFT || key == KEY_RIGHT ||
        key == KEY_TAB || key >= 0x1000)  // Extended keys
    {
        // Special keys - don't set character
        c = 0;
    }
    else if (key != KEY_UNKNOWN && key < 256)
    {
        c = (char)key;
        
        // Apply CapsLock and Shift for letters
        if (c >= 'a' && c <= 'z')
        {
            BYTE shouldUppercase = (gShiftPressed && !gCapsLockOn) || (!gShiftPressed && gCapsLockOn);
            if (shouldUppercase)
            {
                c = c - 'a' + 'A';
            }
        }
        else if (gShiftPressed)
        {
            // Apply shift for special characters
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
    }
    
    // Send key to CLI for processing
    CLI_HandleKey(key, c);
    
    PIC_SendEOI(IRQ_KEYBOARD);
}


void Keyboard_Init(void)
{
    while (__inbyte(KBD_STATUS_PORT) & KBD_STATUS_OUT_FULL)
    {
        __inbyte(KBD_DATA_PORT);
    }
    
    PIC_ClearMask(IRQ_KEYBOARD);
}

KEYCODE Keyboard_GetLastKey(void)
{
    KEYCODE key = gLastKey;
    gLastKey = KEY_UNKNOWN;
    return key;
}
