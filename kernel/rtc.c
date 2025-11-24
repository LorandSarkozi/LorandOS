#include "rtc.h"
#include <intrin.h>

BYTE RTC_ReadRegister(BYTE reg)
{
    __outbyte(CMOS_ADDRESS, reg | 0x80);  // Set bit 7 to disable NMI
    return __inbyte(CMOS_DATA);
}

BYTE RTC_BCDToBinary(BYTE bcd)
{
    return ((bcd / 16) * 10) + (bcd & 0x0F);
}

BOOLEAN RTC_IsUpdateInProgress(void)
{
    return (RTC_ReadRegister(CMOS_REG_STATUS_A) & 0x80) != 0;
}

void RTC_GetDateTime(PDATETIME dt)
{
    BYTE statusB;
    BYTE isBinary;
    BYTE lastSecond;
    BYTE attempts = 0;
    
    // Read until we get two consistent readings
    do
    {
        while (RTC_IsUpdateInProgress());
        
        statusB = RTC_ReadRegister(CMOS_REG_STATUS_B);
        isBinary = (statusB & CMOS_STATUS_B_BINARY) != 0;
        
        // Read time and date registers
        dt->second = RTC_ReadRegister(CMOS_REG_SECONDS);
        dt->minute = RTC_ReadRegister(CMOS_REG_MINUTES);
        dt->hour = RTC_ReadRegister(CMOS_REG_HOURS);
        dt->day = RTC_ReadRegister(CMOS_REG_DAY);
        dt->month = RTC_ReadRegister(CMOS_REG_MONTH);
        dt->year = RTC_ReadRegister(CMOS_REG_YEAR);
        
        // Convert from BCD to binary if necessary
        if (!isBinary)
        {
            dt->second = RTC_BCDToBinary(dt->second);
            dt->minute = RTC_BCDToBinary(dt->minute);
            dt->hour = RTC_BCDToBinary(dt->hour);
            dt->day = RTC_BCDToBinary(dt->day);
            dt->month = RTC_BCDToBinary(dt->month);
            dt->year = RTC_BCDToBinary(dt->year);
        }
        
        // Read second again to check consistency
        lastSecond = RTC_ReadRegister(CMOS_REG_SECONDS);
        if (!isBinary)
        {
            lastSecond = RTC_BCDToBinary(lastSecond);
        }
        
        attempts++;
    } while (lastSecond != dt->second && attempts < 3);
    
    dt->year += 2000;
}
