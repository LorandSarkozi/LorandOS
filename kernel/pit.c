#include "pit.h"
#include "pic.h"
#include "rtc.h"
#include <intrin.h>

static volatile QWORD gTickCount = 0;
static DWORD gPitFrequency = 0;
static DATETIME gBootTime;
static BOOLEAN gTimeInitialized = FALSE;

#define TIMEZONE_OFFSET_HOURS (0)
#define TIMEZONE_OFFSET_MINUTES (-12) 

void PIT_Handler(void)
{
    gTickCount++;
    PIC_SendEOI(IRQ_TIMER);
}

void PIT_Init(DWORD frequency)
{
    DWORD divisor;
    BYTE low, high;
    
    gPitFrequency = frequency;

    divisor = PIT_FREQUENCY / frequency;
    
    low = (BYTE)(divisor & 0xFF);
    high = (BYTE)((divisor >> 8) & 0xFF);
   
    __outbyte(PIT_COMMAND, PIT_CHANNEL_0 | PIT_BOTH | PIT_MODE3 | PIT_BINARY);

    __outbyte(PIT_CHANNEL0, low);
    __outbyte(PIT_CHANNEL0, high);

    PIC_ClearMask(IRQ_TIMER);
    RTC_GetDateTime(&gBootTime);
    
    int adjustedHour = (int)gBootTime.hour + TIMEZONE_OFFSET_HOURS;
    if (adjustedHour >= 24)
    {
        gBootTime.hour = adjustedHour - 24;
        gBootTime.day++;
    }
    else if (adjustedHour < 0)
    {
        gBootTime.hour = adjustedHour + 24;
        gBootTime.day--;
    }
    else
    {
        gBootTime.hour = adjustedHour;
    }

    int adjustedMinute = (int)gBootTime.minute + TIMEZONE_OFFSET_MINUTES;
    if (adjustedMinute >= 60)
    {
        gBootTime.minute = adjustedMinute - 60;
        gBootTime.hour++;
        if (gBootTime.hour >= 24)
        {
            gBootTime.hour = 0;
            gBootTime.day++;
        }
    }
    else if (adjustedMinute < 0)
    {
        gBootTime.minute = adjustedMinute + 60;
        if (gBootTime.hour == 0)
        {
            gBootTime.hour = 23;
            gBootTime.day--;
        }
        else
        {
            gBootTime.hour--;
        }
    }
    else
    {
        gBootTime.minute = adjustedMinute;
    }
    
    gTimeInitialized = TRUE;
}

QWORD PIT_GetTicks(void)
{
    return gTickCount;
}

void PIT_GetCurrentTime(PDATETIME dt)
{
    if (!gTimeInitialized)
    {
        RTC_GetDateTime(dt);
        return;
    }
    
    QWORD ticks = gTickCount;
    QWORD elapsedSeconds = ticks / gPitFrequency;
   
    QWORD bootTotalSeconds = (QWORD)gBootTime.hour * 3600 + 
                             (QWORD)gBootTime.minute * 60 + 
                             (QWORD)gBootTime.second;
    
    QWORD currentTotalSeconds = bootTotalSeconds + elapsedSeconds;
    
    QWORD elapsedDays = currentTotalSeconds / 86400;
    currentTotalSeconds %= 86400;  
   
    DWORD hours = (DWORD)(currentTotalSeconds / 3600);
    currentTotalSeconds %= 3600;
    DWORD minutes = (DWORD)(currentTotalSeconds / 60);
    DWORD seconds = (DWORD)(currentTotalSeconds % 60);

    DWORD day = gBootTime.day + (DWORD)elapsedDays;
    DWORD month = gBootTime.month;
    DWORD year = gBootTime.year;

    BYTE daysInMonth[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

    while (day > daysInMonth[month - 1])
    {
        if (month == 2 && year % 4 == 0 && (year % 100 != 0 || year % 400 == 0))
        {
            if (day > 29)
            {
                day -= 29;
                month++;
            }
            else
            {
                break;
            }
        }
        else
        {
            day -= daysInMonth[month - 1];
            month++;
        }
        
        if (month > 12)
        {
            month = 1;
            year++;
        }
    }
    
    dt->second = (BYTE)seconds;
    dt->minute = (BYTE)minutes;
    dt->hour = (BYTE)hours;
    dt->day = (BYTE)day;
    dt->month = (BYTE)month;
    dt->year = (WORD)year;
}
