#ifndef _STRING_H_
#define _STRING_H_


#include "main.h"

// most often used numbering bases
#define BASE_TWO            2
#define BASE_TEN            10
#define BASE_HEXA           16

#define isascii(c)          ( ( 0x1F < (c)) && ((c) < 0x80) )
#define isupper(c)          ( ( 'A' <= (c)) && ((c) <= 'Z' ) )
#define islower(c)          ( ( 'a' <= (c)) && ((c) <= 'z'))
#define tolower(c)          ( (c) | LOWER_UPPER_DIFF)
#define toupper(c)          ( (c) & (~LOWER_UPPER_DIFF))
#define isspace(c)          ((' ' == (c)) || ('\t' == (c)) || ('\n' == (c)) || ('\r' == (c)))

#define LOWER_UPPER_DIFF    ('a' - 'A')

typedef BYTE                BOOLEAN;
typedef void*               PVOID;

#ifndef TRUE
#define TRUE                        ( 1 == 1 )
#endif

#ifndef FALSE
#define FALSE                       ( 1 == 0 )
#endif

#define MAX_NIBBLE                  0xFU
#define MAX_BYTE                    0xFFU
#define MAX_WORD                    0xFFFFU
#define MAX_DWORD                   0xFFFFFFFFUL
#define MAX_QWORD                   0xFFFFFFFFFFFFFFFFULL

#define INVALID_STRING_SIZE             MAX_DWORD

typedef _Return_type_success_(SUCCEEDED(return)) DWORD  STATUS;

#ifndef _VA_LIST_DEFINED
#define _VA_LIST_DEFINED
typedef PBYTE               va_list;
#endif

#define STACKITEM_SIZE      sizeof(PVOID)

// Initializes the va_list
#define va_start(List,LastArg)     \
            ((List)=((va_list)&(LastArg) + STACKITEM_SIZE))

// Retrieves the value of the next argument
// And increases the List pointer
#define va_arg(List, Type)	\
	((List) += STACKITEM_SIZE, *((Type *)((List) - STACKITEM_SIZE)))

#ifndef max
#define max(a,b)                            ((a)>(b)?(a):(b))
#endif // max

#ifndef min
#define min(a,b)                            ((a)<(b)?(a):(b))
#endif // min

//******************************************************************************
// Function:      itoa
// Description:   Converts a number into its string representation from the
//                specified base.
//                If the number digits occupied by value in base Base is under
//                MinimumDigits then the rest is completed with leading zeros.
// Returns:       void
// Parameter:     IN PVOID valueAddress - Pointer to the number to convert
// Parameter:     IN BOOLEAN signedValue - If set the value is signed, else unsigned
// Parameter:     OUT char * buffer - Buffer in which to write the number
// Parameter:     IN DWORD base - Numeric base of the input value
// Parameter:     IN DWORD minimumDigits - The minimum number of digits to place
//                in the buffer
// Parameter:     IN BOOLEAN is64BitValue - If set the value is treated as a 64bit
//                value
//******************************************************************************
void
itoa(
    PVOID       valueAddress,
    BOOLEAN     signedValue,
    char*       buffer,
    DWORD       base,
    BOOLEAN     is64BitValue
    );

void
atoi(
    PVOID       valueAddress,
    char*       buffer,
    DWORD       base,
    BOOLEAN     is64BitValue
    );

STATUS
cl_snprintf(
    char* outputBuffer,
    DWORD buffSize,
    char* inputBuffer,
    ...
);

STATUS
cl_vsnprintf(
    char* outputBuffer,
    DWORD       buffSize,
    char* inputBuffer,
    va_list     argptr
);

#define atoi32(addr,buf,base)       atoi((addr),(buf),(base),FALSE)
#define atoi64(addr,buf,base)       atoi((addr),(buf),(base),TRUE)

#endif // _STRING_H_