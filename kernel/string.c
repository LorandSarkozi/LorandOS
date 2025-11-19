#include "string.h"


__forceinline
void
swap(
    BYTE* a,
    BYTE* b
)
{
    *a ^= *b;
    *b ^= *a;
    *a ^= *b;
}

#pragma optimize("", off)
void
cl_memset(
    PVOID address,
    BYTE value,
    DWORD size
)
{
    DWORD i;

    // validate parameters
    // size validation is done implicitly in the for loop
    if (NULL == address)
    {
        return;
    }

    for (i = 0; i < size; ++i)
    {
        ((BYTE*)address)[i] = value;
    }
}

void
cl_memmove(
    PVOID   Destination,
    void* Source,
    QWORD   Count
)
{
    PBYTE dst;
    const BYTE* src;
    QWORD i;

    if ((NULL == Destination) || (NULL == Source))
    {
        return;
    }

    dst = Destination;
    src = Source;

    for (i = 0; i < Count; ++i)
    {
        dst[i] = src[i];
    }
}
#pragma optimize("", on)

void
itoa(
    PVOID       valueAddress,
    BOOLEAN     signedValue,
    char* buffer,
    DWORD       base,
    BOOLEAN     is64BitValue
)
{
    DWORD index = 0;
    DWORD i;
    QWORD value;
    BOOLEAN negative;

    negative = FALSE;

    if (is64BitValue)
    {
        if (signedValue)
        {
            if (0 > *(INT64*)valueAddress)
            {
                negative = TRUE;
            }
        }

        if (negative)
        {
            value = (-(*(INT64*)valueAddress));
        }
        else
        {
            value = *(QWORD*)valueAddress;
        }
    }
    else
    {
        if (signedValue)
        {
            if (0 > *(INT32*)valueAddress)
            {
                negative = TRUE;
            }
        }

        if (negative)
        {
            value = (-(*(INT32*)valueAddress));
        }
        else
        {
            value = *(DWORD*)valueAddress;
        }
    }



    if (0 == value)
    {
        buffer[index] = '0';
        index++;
    }

    // we convert the number and get a
    // reversed ordered string
    while (0 != value)
    {
        // we get the current digit
        int digit = (value % base);

        // we convert it to an ASCII character
        if (digit > 9)
        {
            buffer[index] = (char)(digit - 10) + 'A';
        }
        else
        {
            buffer[index] = (char)digit + '0';
        }

        value = value / base;
        index++;
    }

    if (negative)
    {
        buffer[index] = '-';
        index++;
    }

    // we null terminate the string
    buffer[index] = '\0';

    for (i = 0; i < index / 2; ++i)
    {
        // we reverse the string to have it ordered right
        swap((BYTE*)&buffer[i], (BYTE*)&buffer[index - i - 1]);
    }
}

void
atoi(
    PVOID       valueAddress,
    char* buffer,
    DWORD       base,
    BOOLEAN     is64BitValue
)
{
    DWORD i;
    QWORD value;
    BOOLEAN negative;

    i = 0;
    value = 0;
    negative = FALSE;

    if ('-' == buffer[0])
    {
        negative = TRUE;
        i = 1;
    }

    for (; buffer[i] != '\0'; ++i)
    {
        char c;
        BYTE currentCharValue;

        // it's ok to use tolower even if we have a digit
        // because they are 0x30 -> 0x39 and an OR with 0x20 leaves
        // them unmodified
        c = tolower(buffer[i]);

        if ('0' <= c && c <= '9')
        {
            currentCharValue = c - '0';
        }
        else if ('a' <= c && c <= 'z')
        {
            currentCharValue = c - 'a' + 10;
        }
        else
        {
            currentCharValue = MAX_BYTE;
        }

        // digit
        value = value * base + currentCharValue;
    }

    value = negative ? -(INT64)value : value;

    if (is64BitValue)
    {
        *((PQWORD)valueAddress) = value;
    }
    else
    {

        *((PDWORD)valueAddress) = (DWORD)value;
    }
}

// 64 characters needed in case of %B specifier
// with NULL terminator => 65 characters are required
#define cl_vcl_snprintf_BUFFER_SIZE               65

int
cl_strcmp(
    char* str1,
    char* str2
)
{
    DWORD i = 0;

    while (('\0' != str1[i]) && ('\0' != str2[i]))
    {
        if (str1[i] > str2[i])
        {
            return 1;
        }

        if (str1[i] < str2[i])
        {
            return -1;
        }

        ++i;
    }

    // it means the second string is over but the first still has
    // some characters
    if ('\0' != str1[i])
    {
        return 1;
    }

    if ('\0' != str2[i])
    {
        return -1;
    }

    return 0;
}

int
cl_stricmp(
    char* str1,
    char* str2
)
{
    DWORD i = 0;

    while (('\0' != str1[i]) && ('\0' != str2[i]))
    {
        char c1 = tolower(str1[i]);
        char c2 = tolower(str2[i]);

        if (c1 > c2)
        {
            return 1;
        }

        if (c1 < c2)
        {
            return -1;
        }

        ++i;
    }

    // it means the second string is over but the first still has
    // some characters
    if ('\0' != str1[i])
    {
        return 1;
    }

    if ('\0' != str2[i])
    {
        return -1;
    }

    return 0;
}

int
cl_strncmp(
    char* str1,
    char* str2,
    DWORD length
)
{
    DWORD i = 0;

    for (i = 0; i < length && '\0' != str1[i] && '\0' != str2[i]; ++i)
    {
        if (str1[i] > str2[i])
        {
            return 1;
        }

        if (str1[i] < str2[i])
        {
            return -1;
        }
    }

    if (i == length)
    {
        // they are truly equal
        return 0;
    }

    if ('\0' != str1[i])
    {
        // string 1 is bigger
        return 1;
    }

    // string 2 is bigger
    return -1;
}

int
cl_strnicmp(
    char* str1,
    char* str2,
    DWORD length
)
{
    DWORD i = 0;

    for (i = 0; i < length && '\0' != str1[i] && '\0' != str2[i]; ++i)
    {
        char c1 = tolower(str1[i]);
        char c2 = tolower(str2[i]);

        if (c1 > c2)
        {
            return 1;
        }

        if (c1 < c2)
        {
            return -1;
        }
    }

    if (i == length)
    {
        // they are truly equal
        return 0;
    }

    if ('\0' != str1[i])
    {
        // string 1 is bigger
        return 1;
    }

    // string 2 is bigger
    return -1;
}

const
char*
cl_strchr(
    char* str,
    char c
)
{
    DWORD i;

    if (NULL == str)
    {
        return NULL;
    }

    i = 0;

    while ('\0' != str[i])
    {
        if (str[i] == c)
        {
            return (str + i);
        }
        ++i;
    }

    return str;
}

const
char*
cl_strrchr(
    char* str,
    char c
)
{
    DWORD i;
    const char* charIndex;

    if (NULL == str)
    {
        return NULL;
    }

    i = 0;
    charIndex = str;

    while ('\0' != str[i])
    {
        if (str[i] == c)
        {
            charIndex = (str + i);
        }
        ++i;
    }

    return charIndex;
}

void
cl_strcpy(
    char* dst,
    char* src
)
{
    DWORD i;

    i = 0;

    while ('\0' != src[i])
    {
        dst[i] = src[i];
        ++i;
    }

    dst[i] = '\0';
}

void
cl_strncpy(
    char* dst,
    char* src,
    DWORD length
)
{
    DWORD i = 0;

    for (i = 0; i < length && src[i] != '\0'; ++i)
    {
        dst[i] = src[i];
    }

    dst[i] = '\0';
}

DWORD
cl_strlen(
    char* str
)
{
    DWORD i;

    if (NULL == str)
    {
        return INVALID_STRING_SIZE;
    }

    i = 0;

    while ('\0' != str[i])
    {
        ++i;
    }

    return i;
}

DWORD
cl_strlen_s(
    char* str,
    DWORD   maxLen
)
{
    DWORD i;

    if (NULL == str)
    {
        return INVALID_STRING_SIZE;
    }

    i = 0;

    while (i < maxLen && '\0' != str[i])
    {
        ++i;
    }

    return i;
}

STATUS
cl_snprintf(
    char* outputBuffer,
    DWORD buffSize,
    char* inputBuffer,
    ...
)
{
    va_list va;

    va_start(va, inputBuffer);

    return cl_vsnprintf(outputBuffer, buffSize, inputBuffer, va);
}

STATUS
cl_vsnprintf(
    char* outputBuffer,
    DWORD       buffSize,
    char* inputBuffer,
    va_list     argptr
)
{
    char temp[cl_vcl_snprintf_BUFFER_SIZE];        // temporary buffer
    DWORD index;                // index in Buffer
    DWORD param_index;            // index which we want to find
    QWORD temp_value;
    DWORD outBufferOffset;
    DWORD temp_len;
    char* temp_str;

    if (NULL == outputBuffer)
    {
        return -1;
    }

    if (0 == buffSize)
    {
        return -2;
    }

    if (NULL == inputBuffer)
    {
        return -3;
    }

    index = 0;
    param_index = 1;
    temp_value = 0;
    outBufferOffset = 0;
    temp_len = 0;
    temp_str = temp;

    while ('\0' != inputBuffer[index])
    {
        if ('%' == inputBuffer[index])
        {
            DWORD digits = 0;
            DWORD fillSpaces = 0;
            char next = inputBuffer[index + 1];
            BOOLEAN firstSpecificator = TRUE;
            char fillChar = ' ';
            DWORD charsToCopy;

            while (('0' <= next) && (next <= '9'))
            {
                if (firstSpecificator)
                {
                    firstSpecificator = FALSE;

                    if ('0' == next)
                    {
                        fillChar = '0';
                    }
                }

                digits = digits * 10 + next - '0';
                index++;
                next = inputBuffer[index + 1];
            }


            switch (next)
            {
            case 'b':
                // we have an unsigned 32 bit value to print
                temp_value = va_arg(argptr, DWORD);
                itoa(&temp_value, FALSE, temp_str, BASE_TWO, FALSE);
                break;
            case 'B':
                // we have an unsigned 64 bit value to print
                temp_value = va_arg(argptr, QWORD);
                itoa(&temp_value, FALSE, temp_str, BASE_TWO, TRUE);
                break;
            case 'u':
                // we have an unsigned 32 bit value to print
                temp_value = va_arg(argptr, DWORD);
                itoa(&temp_value, FALSE, temp_str, BASE_TEN, FALSE);
                break;
            case 'U':
                // we have an unsigned 64 bit value to print
                temp_value = va_arg(argptr, QWORD);
                itoa(&temp_value, FALSE, temp_str, BASE_TEN, TRUE);
                break;
            case 'd':
                // we have a signed 32 bit value to print
                temp_value = va_arg(argptr, DWORD);
                itoa(&temp_value, TRUE, temp_str, BASE_TEN, FALSE);
                break;
            case 'D':
                // we have a signed 64 bit value
                temp_value = va_arg(argptr, QWORD);
                itoa(&temp_value, TRUE, temp_str, BASE_TEN, TRUE);
                break;
            case 'x':
                // we have a 32 bit hexadecimal value to print
                temp_value = va_arg(argptr, DWORD);
                itoa(&temp_value, FALSE, temp_str, BASE_HEXA, FALSE);
                break;
            case 'X':
                // we have a 64 bit hexadecimal value to print
                temp_value = va_arg(argptr, QWORD);
                itoa(&temp_value, FALSE, temp_str, BASE_HEXA, TRUE);
                break;
            case 'c':
                // we have a character value to print
                temp_value = va_arg(argptr, char);
                cl_strncpy(temp_str, (char*)&temp_value, sizeof(char));
                break;
            case 's':
            case 'S':
                // we have a string to print
                temp_str = va_arg(argptr, char*);
                break;
            default:
                // A parsing error - an incorrect string was supplied =>
                // return a status error
                return -0xFF;
            }

            temp_len = cl_strlen(temp_str);

            charsToCopy = (next == 'S') ? min(digits, temp_len) : temp_len;

            if (outBufferOffset + temp_len >= buffSize)
            {
                // we don't have any more space
#pragma warning(suppress:4146)
                return -buffSize;
            }
            // see if we need to pad the string with spaces or with digits
            fillSpaces = digits > temp_len ? digits - temp_len : 0;
            if (outBufferOffset + temp_len + fillSpaces >= buffSize)
            {
                // we don't have any more space
#pragma warning(suppress:4146)
                return -buffSize;
            }
            if (0 != fillSpaces)
            {
                // put spaces
                cl_memset(outputBuffer + outBufferOffset, fillChar, fillSpaces);
                outBufferOffset = outBufferOffset + fillSpaces;
            }

            if (charsToCopy != 0)
            {
                cl_strncpy(outputBuffer + outBufferOffset, temp_str, charsToCopy);
            }
            outBufferOffset = outBufferOffset + charsToCopy;

            param_index++;          // number of parameters parsed
            index += 2;             // we advance in the format string
            temp_str = temp;        // we need to reset temp_str to point to temp
                                    // else, after a string operation it will point to the
                                    // printed string
        }
        else
        {
            if (outBufferOffset + 1 >= buffSize)
            {
                // we don't have any more space
                outputBuffer[outBufferOffset] = '\0';
#pragma warning(suppress:4146)
                return -buffSize;
            }

            outputBuffer[outBufferOffset] = inputBuffer[index];

            outBufferOffset++;
            index++;
        }

    }

    outputBuffer[outBufferOffset] = '\0';

    return param_index - 1;
}

const
char*
cl_strtok_s(
    char* strToken,
    char* delimiters,
    char** context
)
{
    DWORD i;
    DWORD j;
    char* pStr;

    if (NULL == delimiters)
    {
        return NULL;
    }

    if (NULL == context)
    {
        return NULL;
    }

    if ((NULL == *context) && (NULL == strToken))
    {
        return NULL;
    }

    pStr = (NULL == *context) ? strToken : *context;

    if ('\0' == pStr[0])
    {
        // we reached the end of the string
        return NULL;
    }

    for (i = 0; pStr[i] != '\0'; ++i)
    {
        for (j = 0; delimiters[j] != '\0'; ++j)
        {
            if (pStr[i] == delimiters[j])
            {
                pStr[i] = '\0';
                *context = &pStr[i + 1];
                return pStr;
            }
        }
    }

    // we return the whole string
    // if we reached the end of the string the context must point to NULL as well
    // else we go over our string boundary
    *context = pStr[i] == '\0' ? &pStr[i] : &pStr[i + 1];

    return pStr;
}

DWORD
cl_strcelem(
    char* string,
    char        delimiter
)
{
    DWORD noOfElements;
    const char* pPrevValue;

    if (string == NULL)
    {
        return INVALID_STRING_SIZE;
    }

    // If we have n elements we will have n-1 spaces
    // => start with 1 element
    noOfElements = 1;
    pPrevValue = string;

    for (const char* curPosition = cl_strchr(string, delimiter);
        curPosition != pPrevValue;
        curPosition = cl_strchr(curPosition, delimiter))
    {
        noOfElements++;

        // need to increment by 1 else we'll get stuck in a loop because strchr returns a pointer
        // to the first occurrence of the character
        curPosition++;
        pPrevValue = curPosition;
    }

    return noOfElements;
}

void
cl_strtrim(
                        char* string
)
{
    DWORD startIndex = MAX_DWORD;

    // find the first non-space character
    for (DWORD i = 0; string[i] != '\0'; i++)
    {
        if (!isspace(string[i]))
        {
            startIndex = i;
            break;
        }
    }

    // if only whitespaces were found
    if (MAX_DWORD == startIndex)
    {
        string[0] = '\0';
        return;
    }

    DWORD lengthAfterRightTrim = cl_strlen(string);

    // insert '\0' after last non-space character
    for (DWORD i = cl_strlen(string); i > startIndex; i--)
    {
        if (!isspace(string[i - 1]))
        {
            string[i] = '\0';
            lengthAfterRightTrim = i;
            break;
        }
    }

    // if the first non-space character is not at the beginning of the buffer
    if (0 != startIndex)
    {
        // shift all characters to the left
        cl_memmove(string, &string[startIndex], lengthAfterRightTrim);
    }
}


