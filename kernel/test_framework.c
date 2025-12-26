#include "test_framework.h"
#include "logging.h"
#include "screen.h"
#include "cli.h"

static TEST_FRAMEWORK gTestFramework;

static void PrintToScreen(const char* msg)
{
    extern PSCREEN gVideo;
    extern CLI_STATE gCliState;
    
    DWORD pos = gCliState.cursorPosition;
    
    while (*msg && pos < MAX_OFFSET)
    {
        gVideo[pos].c = *msg;
        gVideo[pos].color = 0x0A;
        pos++;
        msg++;
        
        if (pos % MAX_COLUMNS == 0)
        {
            pos = ((pos / MAX_COLUMNS)) * MAX_COLUMNS;
        }
    }
    
    pos = ((pos / MAX_COLUMNS) + 1) * MAX_COLUMNS;
    gCliState.cursorPosition = pos;
    CursorPosition(pos);
}

void test_framework_init(void)
{
    memset(&gTestFramework, 0, sizeof(TEST_FRAMEWORK));
    gTestFramework.test_count = 0;
}

BOOLEAN test_framework_register(const char* name, TEST_FUNCTION func)
{
    if (!name || !func || gTestFramework.test_count >= MAX_TESTS)
    {
        return FALSE;
    }

    DWORD index = gTestFramework.test_count;
    
    DWORD i = 0;
    while (name[i] && i < MAX_TEST_NAME_LENGTH - 1)
    {
        gTestFramework.tests[index].name[i] = name[i];
        i++;
    }
    gTestFramework.tests[index].name[i] = '\0';
    
    gTestFramework.tests[index].test_func = func;
    gTestFramework.tests[index].is_registered = TRUE;
    gTestFramework.test_count++;

    return TRUE;
}

BOOLEAN test_framework_run(const char* test_name)
{
    Log("test_framework_run START");
    
    if (!test_name)
    {
        Log("test_name is NULL");
        return FALSE;
    }
    
    char buf[64];
    cl_snprintf(buf, sizeof(buf), "test_count: %d", gTestFramework.test_count);
    Log(buf);

    for (DWORD i = 0; i < gTestFramework.test_count; i++)
    {
        if (gTestFramework.tests[i].is_registered)
        {
            BOOLEAN match = TRUE;
            DWORD j = 0;
            while (test_name[j] && gTestFramework.tests[i].name[j])
            {
                if (test_name[j] != gTestFramework.tests[i].name[j])
                {
                    match = FALSE;
                    break;
                }
                j++;
            }
            
            if (match && test_name[j] == '\0' && gTestFramework.tests[i].name[j] == '\0')
            {
                PrintToScreen("Running: ");
                PrintToScreen(gTestFramework.tests[i].name);
                
                BOOLEAN result = gTestFramework.tests[i].test_func();
                
                if (result)
                {
                    PrintToScreen("[PASS]");
                }
                else
                {
                    PrintToScreen("[FAIL]");
                }
                
                return result;
            }
        }
    }

    PrintToScreen("Test not found: ");
    PrintToScreen(test_name);
    return FALSE;
}

void test_framework_run_all(void)
{
    char buffer[128];
    cl_snprintf(buffer, sizeof(buffer), "Running %d tests", gTestFramework.test_count);
    PrintToScreen(buffer);

    DWORD passed = 0;
    DWORD failed = 0;

    for (DWORD i = 0; i < gTestFramework.test_count; i++)
    {
        if (gTestFramework.tests[i].is_registered)
        {
            PrintToScreen("Running: ");
            PrintToScreen(gTestFramework.tests[i].name);
            
            BOOLEAN result = gTestFramework.tests[i].test_func();
            
            if (result)
            {
                PrintToScreen("[PASS]");
                passed++;
            }
            else
            {
                PrintToScreen("[FAIL]");
                failed++;
            }
        }
    }

    memset(buffer, 0, sizeof(buffer));
    cl_snprintf(buffer, sizeof(buffer), "Tests: %d passed, %d failed", passed, failed);
    PrintToScreen(buffer);
}

void test_framework_list(void)
{
    char buffer[128];
    cl_snprintf(buffer, sizeof(buffer), "Available tests (%d):", gTestFramework.test_count);
    PrintToScreen(buffer);

    for (DWORD i = 0; i < gTestFramework.test_count; i++)
    {
        if (gTestFramework.tests[i].is_registered)
        {
            PrintToScreen(gTestFramework.tests[i].name);
        }
    }
}

PTEST_FRAMEWORK test_framework_get(void)
{
    return &gTestFramework;
}
