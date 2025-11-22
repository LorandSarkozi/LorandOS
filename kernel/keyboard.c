#include "keyboard.h"
#include "pic.h"
#include "screen.h"
#include <intrin.h>

static BYTE gShiftPressed = 0;
static BYTE gExtendedScancode = 0;
static KEYCODE gLastKey = KEY_UNKNOWN;
static int gCursorPos = 0;

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
    
    if (gCursorPos >= MAX_OFFSET)
    {
 
        for (int i = 0; i < MAX_OFFSET - MAX_COLUMNS; i++)
        {
            gVideo[i] = gVideo[i + MAX_COLUMNS];
        }
  
        for (int i = MAX_OFFSET - MAX_COLUMNS; i < MAX_OFFSET; i++)
        {
            gVideo[i].c = ' ';
            gVideo[i].color = 0x0F;
        }
        gCursorPos = MAX_OFFSET - MAX_COLUMNS;
    }
}
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

    if (key != KEY_UNKNOWN && key < 256)
    {
        char c = (char)key;
       
        if (c >= 'a' && c <= 'z' && gShiftPressed)
        {
            c = c - 'a' + 'A';
        }
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
