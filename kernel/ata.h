#ifndef _ATA_H_
#define _ATA_H_

#include "main.h"
#include "ata_commands.h"
#include "string.h"

// ATA I/O Ports for Primary and Secondary bus
#define ATA_PRIMARY_IO_BASE         0x1F0
#define ATA_PRIMARY_CTRL_BASE       0x3F6
#define ATA_SECONDARY_IO_BASE       0x170
#define ATA_SECONDARY_CTRL_BASE     0x376

// ATA Register Offsets from IO base
#define ATA_REG_DATA                0x00
#define ATA_REG_ERROR               0x01
#define ATA_REG_FEATURES            0x01
#define ATA_REG_SECTOR_COUNT        0x02
#define ATA_REG_LBA_LOW             0x03
#define ATA_REG_LBA_MID             0x04
#define ATA_REG_LBA_HIGH            0x05
#define ATA_REG_DEVICE              0x06
#define ATA_REG_STATUS              0x07
#define ATA_REG_COMMAND             0x07

// Control Register offset from control base
#define ATA_REG_CONTROL             0x00
#define ATA_REG_ALT_STATUS          0x00

// Status Register Bits
#define ATA_SR_BSY                  0x80    // Busy
#define ATA_SR_DRDY                 0x40    // Drive ready
#define ATA_SR_DF                   0x20    // Drive write fault
#define ATA_SR_DSC                  0x10    // Drive seek complete
#define ATA_SR_DRQ                  0x08    // Data request ready
#define ATA_SR_CORR                 0x04    // Corrected data
#define ATA_SR_IDX                  0x02    // Index
#define ATA_SR_ERR                  0x01    // Error

// Device selection bits
#define ATA_DEVICE_MASTER           0xA0
#define ATA_DEVICE_SLAVE            0xB0

// Device structure
typedef struct _ATA_DEVICE
{
    BOOLEAN present;
    BOOLEAN is_atapi;
    WORD io_base;
    WORD ctrl_base;
    BYTE device_number;  // 0 = master, 1 = slave
    ATA_IDENTIFY_RESPONSE identify_data;
} ATA_DEVICE, *PATA_DEVICE;

// ATA Controller structure
typedef struct _ATA_CONTROLLER
{
    ATA_DEVICE devices[4];  // 2 buses x 2 devices = 4 total devices
    DWORD device_count;
} ATA_CONTROLLER, *PATA_CONTROLLER;

// Function declarations
void ATA_Init(void);
void ATA_SoftwareReset(WORD ctrl_base);
BOOLEAN ATA_DetectDevice(WORD io_base, WORD ctrl_base, BYTE device);
BOOLEAN ATA_IdentifyDevice(WORD io_base, WORD ctrl_base, BYTE device, PATA_IDENTIFY_RESPONSE response);
void ATA_Wait400NS(WORD ctrl_base);
BYTE ATA_WaitStatus(WORD io_base, BYTE mask, BYTE value, DWORD timeout);
BOOLEAN ATA_ReadSectorsPIO(PATA_DEVICE device, DWORD lba, BYTE sector_count, PVOID buffer);
BOOLEAN ATA_WriteSectorsPIO(PATA_DEVICE device, DWORD lba, BYTE sector_count, PVOID buffer);
void ATA_PrintDeviceInfo(PATA_DEVICE device);
PATA_DEVICE ATA_GetDevice(DWORD index);
DWORD ATA_GetDeviceCount(void);

#endif // _ATA_H_
