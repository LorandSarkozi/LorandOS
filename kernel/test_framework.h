#ifndef _TEST_FRAMEWORK_H_
#define _TEST_FRAMEWORK_H_

#include "main.h"
#include "string.h"

#define MAX_TEST_NAME_LENGTH    64
#define MAX_TESTS               32

typedef BOOLEAN (*TEST_FUNCTION)(void);

typedef struct _TEST_ENTRY
{
    char name[MAX_TEST_NAME_LENGTH];
    TEST_FUNCTION test_func;
    BOOLEAN is_registered;
} TEST_ENTRY, *PTEST_ENTRY;

typedef struct _TEST_FRAMEWORK
{
    TEST_ENTRY tests[MAX_TESTS];
    DWORD test_count;
} TEST_FRAMEWORK, *PTEST_FRAMEWORK;

void test_framework_init(void);

BOOLEAN test_framework_register(const char* name, TEST_FUNCTION func);

BOOLEAN test_framework_run(const char* test_name);

void test_framework_run_all(void);

void test_framework_list(void);

PTEST_FRAMEWORK test_framework_get(void);

#endif
