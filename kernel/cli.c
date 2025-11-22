#include "cli.h"
#include "screen.h"
#include "string.h"
#include "pit.h"
#include "rtc.h"
#include "logging.h"

CLI_STATE gCliState;

static BOOLEAN strncmp_equal(const char* s1, const char* s2, DWORD n)
{
    for (DWORD i = 0; i < n; i++)
    {
        if (s1[i] != s2[i]) return FALSE;
        if (s1[i] == '\0') return TRUE;
    }
    return TRUE;
}

static DWORD strlen_cli(const char* str)
{
    DWORD len = 0;
    while (str[len] != '\0' && len < CLI_MAX_COMMAND_LENGTH)
    {
        len++;
    }
    return len;
}

static void memcpy_cli(void* dest, const void* src, DWORD size)
{
    BYTE* d = (BYTE*)dest;
    const BYTE* s = (const BYTE*)src;
    for (DWORD i = 0; i < size; i++)
    {
        d[i] = s[i];
    }
}

void CLI_Init(void)
{
    memset(&gCliState, 0, sizeof(CLI_STATE));
    gCliState.mode = CLI_MODE_COMMAND;
    gCliState.capsLockOn = FALSE;
    
    ClearScreen();
    Log("MiniOS CLI v1.0");
    Log("Type 'clear' to clear screen, 'time' for system time, 'edit' for editor");
    CLI_PrintPrompt();
}

void CLI_PrintPrompt(void)
{
    extern PSCREEN gVideo;
    const char* prompt = CLI_PROMPT;
    DWORD pos = gCliState.cursorPosition;
    
    while (*prompt && pos < MAX_OFFSET)
    {
        gVideo[pos].c = *prompt;
        gVideo[pos].color = CLI_PROMPT_COLOR;
        prompt++;
        pos++;
    }
    
    gCliState.cursorPosition = pos;
    CursorPosition(pos);
}

void CLI_Clear(void)
{
    ClearScreen();
    gCliState.cursorPosition = 0;
    gCliState.commandLength = 0;
}

void CLI_HandleKey(KEYCODE key, char c)
{
    extern PSCREEN gVideo;
    
    if (gCliState.mode == CLI_MODE_EDIT)
    {
        CLI_HandleEditKey(key, c);
        return;
    }

    if (key == KEY_RETURN)
    {
        gCliState.commandBuffer[gCliState.commandLength] = '\0';

        gCliState.cursorPosition = ((gCliState.cursorPosition / MAX_COLUMNS) + 1) * MAX_COLUMNS;

        if (gCliState.commandLength > 0)
        {
            CLI_ProcessCommand();
        }
        gCliState.commandLength = 0;
        CLI_PrintPrompt();
    }
    else if (key == KEY_BACKSPACE)
    {
        if (gCliState.commandLength > 0)
        {
            gCliState.commandLength--;
            gCliState.cursorPosition--;
            gVideo[gCliState.cursorPosition].c = ' ';
            gVideo[gCliState.cursorPosition].color = 0x0F;
            CursorPosition(gCliState.cursorPosition);
        }
    }
    else if (key == KEY_UP)
    {
        if (gCliState.historyCount > 0)
        {
            if (gCliState.historyIndex > 0)
            {
                gCliState.historyIndex--;
            }
  
            DWORD startPos = gCliState.cursorPosition - gCliState.commandLength;
            for (DWORD i = 0; i < gCliState.commandLength; i++)
            {
                gVideo[startPos + i].c = ' ';
            }
  
            memcpy_cli(gCliState.commandBuffer, 
                      gCliState.history[gCliState.historyIndex],
                      CLI_MAX_COMMAND_LENGTH);
            gCliState.commandLength = strlen_cli(gCliState.commandBuffer);
     
            gCliState.cursorPosition = startPos;
            for (DWORD i = 0; i < gCliState.commandLength; i++)
            {
                gVideo[gCliState.cursorPosition].c = gCliState.commandBuffer[i];
                gVideo[gCliState.cursorPosition].color = 0x0F;
                gCliState.cursorPosition++;
            }
            CursorPosition(gCliState.cursorPosition);
        }
    }
    else if (key == KEY_DOWN)
    {
        if (gCliState.historyCount > 0 && gCliState.historyIndex < gCliState.historyCount - 1)
        {
            gCliState.historyIndex++;
            
            DWORD startPos = gCliState.cursorPosition - gCliState.commandLength;
            for (DWORD i = 0; i < gCliState.commandLength; i++)
            {
                gVideo[startPos + i].c = ' ';
            }
            
            memcpy_cli(gCliState.commandBuffer, 
                      gCliState.history[gCliState.historyIndex],
                      CLI_MAX_COMMAND_LENGTH);
            gCliState.commandLength = strlen_cli(gCliState.commandBuffer);

            gCliState.cursorPosition = startPos;
            for (DWORD i = 0; i < gCliState.commandLength; i++)
            {
                gVideo[gCliState.cursorPosition].c = gCliState.commandBuffer[i];
                gVideo[gCliState.cursorPosition].color = 0x0F;
                gCliState.cursorPosition++;
            }
            CursorPosition(gCliState.cursorPosition);
        }
    }
    else if (c != 0 && gCliState.commandLength < CLI_MAX_COMMAND_LENGTH - 1)
    {
        gCliState.commandBuffer[gCliState.commandLength] = c;
        gCliState.commandLength++;

        gVideo[gCliState.cursorPosition].c = c;
        gVideo[gCliState.cursorPosition].color = 0x0F;
        gCliState.cursorPosition++;
        
        if (gCliState.cursorPosition >= MAX_OFFSET)
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
            
            gCliState.cursorPosition = MAX_OFFSET - MAX_COLUMNS;
        }
        
        CursorPosition(gCliState.cursorPosition);
    }
}

void CLI_ProcessCommand(void)
{
    if (gCliState.commandLength > 0)
    {
        if (gCliState.historyCount >= CLI_HISTORY_SIZE)
        {
            for (DWORD i = 0; i < CLI_HISTORY_SIZE - 1; i++)
            {
                memcpy_cli(gCliState.history[i], 
                          gCliState.history[i + 1], 
                          CLI_MAX_COMMAND_LENGTH);
            }
            gCliState.historyCount = CLI_HISTORY_SIZE - 1;
        }
 
        memcpy_cli(gCliState.history[gCliState.historyCount], 
                  gCliState.commandBuffer, 
                  gCliState.commandLength);
        gCliState.history[gCliState.historyCount][gCliState.commandLength] = '\0';
        gCliState.historyCount++;
        gCliState.historyIndex = gCliState.historyCount;
    }
    
    if (strncmp_equal(gCliState.commandBuffer, "clear", 5) || 
        strncmp_equal(gCliState.commandBuffer, "cls", 3))
    {
        CLI_Command_Clear();
    }
    else if (strncmp_equal(gCliState.commandBuffer, "time", 4))
    {
        CLI_Command_Time();
    }
    else if (strncmp_equal(gCliState.commandBuffer, "edit", 4))
    {
        CLI_EnterEditMode();
    }
    else
    {
        Log("Unknown command. Available: clear, cls, time, edit");
    }
}

void CLI_Command_Clear(void)
{
    CLI_Clear();
}

static void PrintNumber2Digits(DWORD* pos, BYTE value)
{
    extern PSCREEN gVideo;
    
    if (value < 10)
    {
        gVideo[*pos].c = '0';
        gVideo[*pos].color = 0x0E;
        (*pos)++;
        gVideo[*pos].c = '0' + value;
        gVideo[*pos].color = 0x0E;
        (*pos)++;
    }
    else
    {
        gVideo[*pos].c = '0' + (value / 10);
        gVideo[*pos].color = 0x0E;
        (*pos)++;
        gVideo[*pos].c = '0' + (value % 10);
        gVideo[*pos].color = 0x0E;
        (*pos)++;
    }
}

void CLI_Command_Time(void)
{
    extern PSCREEN gVideo;
    char buffer[64];
    QWORD ticks = PIT_GetTicks();
    DATETIME dt;
    DWORD pos = gCliState.cursorPosition;

    memset(buffer, 0, sizeof(buffer));

    const char* msg = "Ticks since boot: ";
    while (*msg && pos < MAX_OFFSET)
    {
        gVideo[pos].c = *msg;
        gVideo[pos].color = 0x0E;
        msg++;
        pos++;
    }
    
    itoa(&ticks, FALSE, buffer, BASE_TEN, TRUE);
    DWORD i = 0;
    while (buffer[i] && i < 63 && pos < MAX_OFFSET)
    {
        gVideo[pos].c = buffer[i];
        gVideo[pos].color = 0x0E;
        i++;
        pos++;
    }
    
    pos = ((pos / MAX_COLUMNS) + 1) * MAX_COLUMNS;
    
    RTC_GetDateTime(&dt);
    
    const char* timeMsg = "Current time: ";
    while (*timeMsg && pos < MAX_OFFSET)
    {
        gVideo[pos].c = *timeMsg;
        gVideo[pos].color = 0x0E;
        timeMsg++;
        pos++;
    }

    PrintNumber2Digits(&pos, dt.day);
    gVideo[pos].c = '/';
    gVideo[pos].color = 0x0E;
    pos++;
    
    PrintNumber2Digits(&pos, dt.month);
    gVideo[pos].c = '/';
    gVideo[pos].color = 0x0E;
    pos++;

    BYTE year2 = dt.year % 100;
    PrintNumber2Digits(&pos, year2);
    gVideo[pos].c = ' ';
    gVideo[pos].color = 0x0E;
    pos++;
    
    PrintNumber2Digits(&pos, dt.hour);
    gVideo[pos].c = ':';
    gVideo[pos].color = 0x0E;
    pos++;

    PrintNumber2Digits(&pos, dt.minute);
    gVideo[pos].c = ':';
    gVideo[pos].color = 0x0E;
    pos++;

    PrintNumber2Digits(&pos, dt.second);

    pos = ((pos / MAX_COLUMNS) + 1) * MAX_COLUMNS;
    gCliState.cursorPosition = pos;
    CursorPosition(pos);
}

void CLI_EnterEditMode(void)
{
    gCliState.mode = CLI_MODE_EDIT;
    gCliState.editCursorRow = 0;
    gCliState.editCursorCol = 0;
    gCliState.editScrollOffset = 0;
    
    CLI_RefreshEditScreen();
}

void CLI_ExitEditMode(void)
{
    gCliState.mode = CLI_MODE_COMMAND;
    
    CLI_Clear();
    Log("Exited edit mode");
    CLI_PrintPrompt();
}

void CLI_HandleEditKey(KEYCODE key, char c)
{
    extern PSCREEN gVideo;
    
    if (key == KEY_ESCAPE)
    {
        CLI_ExitEditMode();
        return;
    }
    else if (key == KEY_UP)
    {
        if (gCliState.editCursorRow > 0)
        {
            gCliState.editCursorRow--;
        }
        else if (gCliState.editScrollOffset > 0)
        {
            gCliState.editScrollOffset--;
            CLI_RefreshEditScreen();
        }
    }
    else if (key == KEY_DOWN)
    {
        if (gCliState.editCursorRow < MAX_LINES - 1)
        {
            if (gCliState.editCursorRow + gCliState.editScrollOffset < gCliState.editBufferLines)
            {
                gCliState.editCursorRow++;
            }
        }
        else if (gCliState.editCursorRow + gCliState.editScrollOffset < EDIT_BUFFER_LINES - 1)
        {
            gCliState.editScrollOffset++;
            CLI_RefreshEditScreen();
        }
    }
    else if (key == KEY_LEFT)
    {
        if (gCliState.editCursorCol > 0)
        {
            gCliState.editCursorCol--;
        }
    }
    else if (key == KEY_RIGHT)
    {
        if (gCliState.editCursorCol < MAX_COLUMNS - 1)
        {
            gCliState.editCursorCol++;
        }
    }
    else if (key == KEY_RETURN)
    {
        if (gCliState.editCursorRow < MAX_LINES - 1)
        {
            gCliState.editCursorRow++;
            gCliState.editCursorCol = 0;
        }
        else if (gCliState.editCursorRow + gCliState.editScrollOffset < EDIT_BUFFER_LINES - 1)
        {
            gCliState.editScrollOffset++;
            gCliState.editCursorCol = 0;
            CLI_RefreshEditScreen();
        }

        DWORD currentLine = gCliState.editCursorRow + gCliState.editScrollOffset;
        if (currentLine >= gCliState.editBufferLines)
        {
            gCliState.editBufferLines = currentLine + 1;
        }
    }
    else if (key == KEY_BACKSPACE)
    {
        if (gCliState.editCursorCol > 0)
        {
            gCliState.editCursorCol--;
            DWORD bufferRow = gCliState.editCursorRow + gCliState.editScrollOffset;
            gCliState.editBuffer[bufferRow][gCliState.editCursorCol] = ' ';
            
            DWORD screenPos = gCliState.editCursorRow * MAX_COLUMNS + gCliState.editCursorCol;
            gVideo[screenPos].c = ' ';
            gVideo[screenPos].color = 0x0F;
        }
    }
    else if (c != 0)
    {
        DWORD bufferRow = gCliState.editCursorRow + gCliState.editScrollOffset;
        gCliState.editBuffer[bufferRow][gCliState.editCursorCol] = c;
        
        DWORD screenPos = gCliState.editCursorRow * MAX_COLUMNS + gCliState.editCursorCol;
        gVideo[screenPos].c = c;
        gVideo[screenPos].color = 0x0F;
        
        if (gCliState.editCursorCol < MAX_COLUMNS - 1)
        {
            gCliState.editCursorCol++;
        }
     
        if (bufferRow >= gCliState.editBufferLines)
        {
            gCliState.editBufferLines = bufferRow + 1;
        }
    }

    DWORD cursorPos = gCliState.editCursorRow * MAX_COLUMNS + gCliState.editCursorCol;
    CursorPosition(cursorPos);
}

void CLI_RefreshEditScreen(void)
{
    extern PSCREEN gVideo;
    
    for (DWORD row = 0; row < MAX_LINES; row++)
    {
        DWORD bufferRow = row + gCliState.editScrollOffset;
        for (DWORD col = 0; col < MAX_COLUMNS; col++)
        {
            DWORD screenPos = row * MAX_COLUMNS + col;
            if (bufferRow < EDIT_BUFFER_LINES)
            {
                char c = gCliState.editBuffer[bufferRow][col];
                gVideo[screenPos].c = (c == 0) ? ' ' : c;
            }
            else
            {
                gVideo[screenPos].c = ' ';
            }
            gVideo[screenPos].color = 0x0F;
        }
    }
    
    DWORD cursorPos = gCliState.editCursorRow * MAX_COLUMNS + gCliState.editCursorCol;
    CursorPosition(cursorPos);
}
