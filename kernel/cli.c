#include "cli.h"
#include "keyboard.h"
#include "screen.h"
#include "string.h"
#include "pit.h"
#include "rtc.h"
#include "logging.h"
#include "ata.h"
#include "test_framework.h"

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
    Log("Type 'printmbr' to display disk sector 0 in hex format");
    Log("Type 'writembr' to write test data to disk sector 0");
    Log("Type 'test_list' to list tests, 'test_run <name>' to run a test, 'test_run_all' to run all tests");
    
    Log("Init test framework...");
    test_framework_init();
    Log("Test framework ready");
    
    Log("Init frame allocator...");
    Memory_Init();
    Log("Frame allocator ready");
    
    Log("Register tests...");
    memory_tests_register();
    Log("Tests registered");
    
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
    else if (strncmp_equal(gCliState.commandBuffer, "printmbr", 8))
    {
        CLI_Command_PrintMBR();
    }
    else if (strncmp_equal(gCliState.commandBuffer, "writembr", 8))
    {
        const char* args = gCliState.commandBuffer + 8;
        while (*args == ' ') args++;
        CLI_Command_WriteMBR(args);
    }
    else if (strncmp_equal(gCliState.commandBuffer, "test_run ", 9))
    {
        const char* test_name = gCliState.commandBuffer + 9;
        CLI_Command_TestRun(test_name);
    }
    else if (strncmp_equal(gCliState.commandBuffer, "test_list", 9))
    {
        CLI_Command_TestList();
    }
    else if (strncmp_equal(gCliState.commandBuffer, "test_run_all", 12))
    {
        CLI_Command_TestRunAll();
    }
    else
    {
        extern PSCREEN gVideo;
        const char* msg = "Unknown command. Available: clear, cls, time, edit, printmbr, writembr, test_run, test_list, test_run_all";
        DWORD pos = ((gCliState.cursorPosition / MAX_COLUMNS) + 1) * MAX_COLUMNS;
        
        while (*msg && pos < MAX_OFFSET)
        {
            gVideo[pos].c = *msg;
            gVideo[pos].color = 0x0C;
            msg++;
            pos++;
        }
        
        gCliState.cursorPosition = ((pos / MAX_COLUMNS) + 1) * MAX_COLUMNS;
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
    DWORD pos = ((gCliState.cursorPosition / MAX_COLUMNS) + 1) * MAX_COLUMNS;

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
    
    PIT_GetCurrentTime(&dt);
    
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

// Helper to print a hex byte
static void PrintHexByte(DWORD* pos, BYTE value)
{
    extern PSCREEN gVideo;
    const char hex_chars[] = "0123456789ABCDEF";
    
    gVideo[*pos].c = hex_chars[(value >> 4) & 0x0F];
    gVideo[*pos].color = 0x0B;
    (*pos)++;
    gVideo[*pos].c = hex_chars[value & 0x0F];
    gVideo[*pos].color = 0x0B;
    (*pos)++;
}

// Helper to print offset (8 hex digits)
static void PrintOffset(DWORD* pos, DWORD offset)
{
    extern PSCREEN gVideo;
    const char hex_chars[] = "0123456789ABCDEF";
    
    for (int i = 7; i >= 0; i--)
    {
        gVideo[*pos].c = hex_chars[(offset >> (i * 4)) & 0x0F];
        gVideo[*pos].color = 0x0E;
        (*pos)++;
    }
}

void CLI_Command_PrintMBR(void)
{
    extern PSCREEN gVideo;
    BYTE sector_buffer[512];
    DWORD pos = ((gCliState.cursorPosition / MAX_COLUMNS) + 1) * MAX_COLUMNS;
    char msg[64];
    
    // Check how many devices we have
    DWORD device_count = ATA_GetDeviceCount();
    if (device_count == 0)
    {
        const char* error_msg = "No ATA devices found! Check Bochs config.";
        while (*error_msg && pos < MAX_OFFSET)
        {
            gVideo[pos].c = *error_msg;
            gVideo[pos].color = 0x0C;
            error_msg++;
            pos++;
        }
        gCliState.cursorPosition = ((pos / MAX_COLUMNS) + 1) * MAX_COLUMNS;
        CursorPosition(gCliState.cursorPosition);
        return;
    }
    
    cl_snprintf(msg, sizeof(msg), "Found %d ATA device(s). Reading from first device...", device_count);
    DWORD i = 0;
    while (msg[i] && pos < MAX_OFFSET)
    {
        gVideo[pos].c = msg[i];
        gVideo[pos].color = 0x0E;
        i++;
        pos++;
    }
    pos = ((pos / MAX_COLUMNS) + 1) * MAX_COLUMNS;
    
    // Get first ATA device
    PATA_DEVICE device = ATA_GetDevice(0);
    
    if (!device)
    {
        const char* error_msg = "Failed to get ATA device!";
        while (*error_msg && pos < MAX_OFFSET)
        {
            gVideo[pos].c = *error_msg;
            gVideo[pos].color = 0x0C;
            error_msg++;
            pos++;
        }
        gCliState.cursorPosition = ((pos / MAX_COLUMNS) + 1) * MAX_COLUMNS;
        CursorPosition(gCliState.cursorPosition);
        return;
    }
    
    // Read sector 0 (MBR)
    const char* reading_msg = "Reading sector 0...";
    while (*reading_msg && pos < MAX_OFFSET)
    {
        gVideo[pos].c = *reading_msg;
        gVideo[pos].color = 0x0E;
        reading_msg++;
        pos++;
    }
    pos = ((pos / MAX_COLUMNS) + 1) * MAX_COLUMNS;
    
    if (!ATA_ReadSectorsPIO(device, 0, 1, sector_buffer))
    {
        const char* error_msg = "Failed to read sector 0!";
        while (*error_msg && pos < MAX_OFFSET)
        {
            gVideo[pos].c = *error_msg;
            gVideo[pos].color = 0x0C;
            error_msg++;
            pos++;
        }
        gCliState.cursorPosition = ((pos / MAX_COLUMNS) + 1) * MAX_COLUMNS;
        CursorPosition(gCliState.cursorPosition);
        return;
    }
    
    const char* success_msg = "Read successful! Displaying MBR:";
    while (*success_msg && pos < MAX_OFFSET)
    {
        gVideo[pos].c = *success_msg;
        gVideo[pos].color = 0x0A;
        success_msg++;
        pos++;
    }
    pos = ((pos / MAX_COLUMNS) + 1) * MAX_COLUMNS;
    
    // Print header
    const char* header = "Offset(h) 00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F  Decoded text";
    while (*header && pos < MAX_OFFSET)
    {
        gVideo[pos].c = *header;
        gVideo[pos].color = 0x0F;
        header++;
        pos++;
    }
    
    pos = ((pos / MAX_COLUMNS) + 1) * MAX_COLUMNS;
    
    // Print each line (16 bytes per line, 32 lines for 512 bytes)
    for (DWORD line = 0; line < 32 && pos < MAX_OFFSET - MAX_COLUMNS; line++)
    {
        DWORD offset = line * 16;
        
        // Print offset
        PrintOffset(&pos, offset);
        
        // Space after offset
        gVideo[pos].c = ' ';
        gVideo[pos].color = 0x0F;
        pos++;
        gVideo[pos].c = ' ';
        gVideo[pos].color = 0x0F;
        pos++;
        
        // Print 16 hex bytes
        for (DWORD i = 0; i < 16; i++)
        {
            PrintHexByte(&pos, sector_buffer[offset + i]);
            
            // Space after each byte
            gVideo[pos].c = ' ';
            gVideo[pos].color = 0x0F;
            pos++;
        }
        
        // Space before decoded text
        gVideo[pos].c = ' ';
        gVideo[pos].color = 0x0F;
        pos++;
        
        // Print decoded text (ASCII representation)
        for (DWORD i = 0; i < 16; i++)
        {
            BYTE byte_val = sector_buffer[offset + i];
            char c;
            
            // Print printable ASCII or '.' for non-printable
            if (byte_val >= 0x20 && byte_val <= 0x7E)
            {
                c = (char)byte_val;
            }
            else
            {
                c = '.';
            }
            
            gVideo[pos].c = c;
            gVideo[pos].color = 0x0A;
            pos++;
        }
        
        // Move to next line
        pos = ((pos / MAX_COLUMNS) + 1) * MAX_COLUMNS;
    }
    
    gCliState.cursorPosition = pos;
    CursorPosition(pos);
}

void CLI_Command_WriteMBR(const char* custom_text)
{
    extern PSCREEN gVideo;
    BYTE sector_buffer[512];
    DWORD pos = ((gCliState.cursorPosition / MAX_COLUMNS) + 1) * MAX_COLUMNS;
    
    DWORD device_count = ATA_GetDeviceCount();
    if (device_count == 0)
    {
        const char* error_msg = "No ATA devices found!";
        while (*error_msg && pos < MAX_OFFSET)
        {
            gVideo[pos].c = *error_msg;
            gVideo[pos].color = 0x0C;
            error_msg++;
            pos++;
        }
        gCliState.cursorPosition = ((pos / MAX_COLUMNS) + 1) * MAX_COLUMNS;
        CursorPosition(gCliState.cursorPosition);
        return;
    }
    
    PATA_DEVICE device = ATA_GetDevice(0);
    
    if (!device)
    {
        const char* error_msg = "Failed to get ATA device!";
        while (*error_msg && pos < MAX_OFFSET)
        {
            gVideo[pos].c = *error_msg;
            gVideo[pos].color = 0x0C;
            error_msg++;
            pos++;
        }
        gCliState.cursorPosition = ((pos / MAX_COLUMNS) + 1) * MAX_COLUMNS;
        CursorPosition(gCliState.cursorPosition);
        return;
    }
    
    memset(sector_buffer, 0, 512);
    
    const char* text_to_write = (custom_text && *custom_text) ? custom_text : "MiniOS Test MBR - Default text";
    for (int i = 0; text_to_write[i] && i < 500; i++)
    {
        sector_buffer[i] = (BYTE)text_to_write[i];
    }
    
    sector_buffer[510] = 0x55;
    sector_buffer[511] = 0xAA;
    
    const char* writing_msg = "Writing test MBR to sector 0...";
    while (*writing_msg && pos < MAX_OFFSET)
    {
        gVideo[pos].c = *writing_msg;
        gVideo[pos].color = 0x0E;
        writing_msg++;
        pos++;
    }
    pos = ((pos / MAX_COLUMNS) + 1) * MAX_COLUMNS;
    
    if (!ATA_WriteSectorsPIO(device, 0, 1, sector_buffer))
    {
        const char* error_msg = "Failed to write sector 0!";
        while (*error_msg && pos < MAX_OFFSET)
        {
            gVideo[pos].c = *error_msg;
            gVideo[pos].color = 0x0C;
            error_msg++;
            pos++;
        }
        gCliState.cursorPosition = ((pos / MAX_COLUMNS) + 1) * MAX_COLUMNS;
        CursorPosition(gCliState.cursorPosition);
        return;
    }
    
    const char* success_msg = "Write successful! Boot signature (0x55AA) written.";
    while (*success_msg && pos < MAX_OFFSET)
    {
        gVideo[pos].c = *success_msg;
        gVideo[pos].color = 0x0A;
        success_msg++;
        pos++;
    }
    pos = ((pos / MAX_COLUMNS) + 1) * MAX_COLUMNS;
    
    const char* verify_msg = "Use 'printmbr' to verify the write.";
    while (*verify_msg && pos < MAX_OFFSET)
    {
        gVideo[pos].c = *verify_msg;
        gVideo[pos].color = 0x0B;
        verify_msg++;
        pos++;
    }
    
    gCliState.cursorPosition = ((pos / MAX_COLUMNS) + 1) * MAX_COLUMNS;
    CursorPosition(gCliState.cursorPosition);
}

void CLI_Command_TestRun(const char* test_name)
{
    extern PSCREEN gVideo;
    DWORD pos = ((gCliState.cursorPosition / MAX_COLUMNS) + 1) * MAX_COLUMNS;
    
    Log("CLI_Command_TestRun called");
    
    if (!test_name || *test_name == '\0')
    {
        Log("No test name provided");
        const char* error_msg = "Usage: test_run <test_name>";
        while (*error_msg && pos < MAX_OFFSET)
        {
            gVideo[pos].c = *error_msg;
            gVideo[pos].color = 0x0C;
            error_msg++;
            pos++;
        }
        gCliState.cursorPosition = ((pos / MAX_COLUMNS) + 1) * MAX_COLUMNS;
        CursorPosition(gCliState.cursorPosition);
        return;
    }
    
    Log("Calling test_framework_run with:");
    Log(test_name);
    test_framework_run(test_name);
    Log("test_framework_run returned");
    
    gCliState.cursorPosition = ((gCliState.cursorPosition / MAX_COLUMNS) + 2) * MAX_COLUMNS;
    CursorPosition(gCliState.cursorPosition);
}

void CLI_Command_TestList(void)
{
    test_framework_list();
    
    gCliState.cursorPosition = ((gCliState.cursorPosition / MAX_COLUMNS) + 2) * MAX_COLUMNS;
    CursorPosition(gCliState.cursorPosition);
}

void CLI_Command_TestRunAll(void)
{
    test_framework_run_all();
    
    gCliState.cursorPosition = ((gCliState.cursorPosition / MAX_COLUMNS) + 2) * MAX_COLUMNS;
    CursorPosition(gCliState.cursorPosition);
}
