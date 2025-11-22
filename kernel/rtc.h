#ifndef _RTC_H_
#define _RTC_H_

#include "main.h"
#include "string.h"

// CMOS/RTC ports
#define CMOS_ADDRESS    0x70
#define CMOS_DATA       0x71

// CMOS registers
#define CMOS_REG_SECONDS        0x00
#define CMOS_REG_MINUTES        0x02
#define CMOS_REG_HOURS          0x04
#define CMOS_REG_DAY            0x07
#define CMOS_REG_MONTH          0x08
#define CMOS_REG_YEAR           0x09
#define CMOS_REG_STATUS_A       0x0A
#define CMOS_REG_STATUS_B       0x0B

// Status register B flags
#define CMOS_STATUS_B_24HOUR    0x02
#define CMOS_STATUS_B_BINARY    0x04

// Date/Time structure
typedef struct _DATETIME
{
    BYTE second;
    BYTE minute;
    BYTE hour;
    BYTE day;
    BYTE month;
    WORD year;
} DATETIME, *PDATETIME;

// RTC functions
BYTE RTC_ReadRegister(BYTE reg);
BYTE RTC_BCDToBinary(BYTE bcd);
void RTC_GetDateTime(PDATETIME dt);
BOOLEAN RTC_IsUpdateInProgress(void);

#endif // _RTC_H_
