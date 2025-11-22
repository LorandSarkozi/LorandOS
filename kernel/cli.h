#ifndef _CLI_H_
#define _CLI_H_

#include "main.h"
#include "screen.h"
#include "scancode.h"

#define CLI_MAX_COMMAND_LENGTH      128
#define CLI_HISTORY_SIZE            10
#define CLI_PROMPT                  "> "
#define CLI_PROMPT_COLOR            0x0A
#define EDIT_BUFFER_LINES           (MAX_LINES * 2)  // MAX_LINES visible + MAX_LINES buffered
#define EDIT_LINE_LENGTH            MAX_COLUMNS

// CLI modes
typedef enum _CLI_MODE
{
    CLI_MODE_COMMAND = 0,
    CLI_MODE_EDIT = 1
} CLI_MODE;

// CLI state structure
typedef struct _CLI_STATE
{
    CLI_MODE mode;
    char commandBuffer[CLI_MAX_COMMAND_LENGTH];
    DWORD commandLength;
    DWORD cursorPosition;
    
    // Command history
    char history[CLI_HISTORY_SIZE][CLI_MAX_COMMAND_LENGTH];
    DWORD historyCount;
    DWORD historyIndex;
    
    // Edit mode state
    char editBuffer[EDIT_BUFFER_LINES][EDIT_LINE_LENGTH];
    DWORD editCursorRow;
    DWORD editCursorCol;
    DWORD editScrollOffset;
    BYTE editBufferLines;
    
    // Keyboard state
    BYTE capsLockOn;
    BYTE shiftPressed;
} CLI_STATE, *PCLI_STATE;

// CLI functions
void CLI_Init(void);
void CLI_HandleKey(KEYCODE key, char c);
void CLI_ProcessCommand(void);
void CLI_PrintPrompt(void);
void CLI_Clear(void);

// Edit mode functions
void CLI_EnterEditMode(void);
void CLI_ExitEditMode(void);
void CLI_HandleEditKey(KEYCODE key, char c);
void CLI_RefreshEditScreen(void);

// Command handlers
void CLI_Command_Clear(void);
void CLI_Command_Time(void);

// Exposed for keyboard handler
extern CLI_STATE gCliState;

#endif
