#include "ata.h"
#include "logging.h"
#include "pit.h"
static ATA_CONTROLLER gAtaController;

static inline BYTE inb(WORD port)
{
    return __inbyte(port);
}

static inline void outb(WORD port, BYTE value)
{
    __outbyte(port, value);
}

static inline WORD inw(WORD port)
{
    return __inword(port);
}

static inline void outw(WORD port, WORD value)
{
    __outword(port, value);
}

void ATA_Wait400NS(WORD ctrl_base)
{
    for (int i = 0; i < 4; i++)
    {
        inb(ctrl_base + ATA_REG_ALT_STATUS);
    }
}

void ATA_SoftwareReset(WORD ctrl_base)
{
    outb(ctrl_base + ATA_REG_CONTROL, 0x04);
   
    ATA_Wait400NS(ctrl_base);
    ATA_Wait400NS(ctrl_base);
    ATA_Wait400NS(ctrl_base);
    ATA_Wait400NS(ctrl_base);
    ATA_Wait400NS(ctrl_base);
    ATA_Wait400NS(ctrl_base);
    ATA_Wait400NS(ctrl_base);
    ATA_Wait400NS(ctrl_base);
    ATA_Wait400NS(ctrl_base);
    ATA_Wait400NS(ctrl_base);
    ATA_Wait400NS(ctrl_base);
    ATA_Wait400NS(ctrl_base);
    
    outb(ctrl_base + ATA_REG_CONTROL, 0x00);
    
    for (int i = 0; i < 5000; i++)
    {
        ATA_Wait400NS(ctrl_base);
    }
}

BYTE ATA_WaitStatus(WORD io_base, BYTE mask, BYTE value, DWORD timeout)
{
    BYTE status;
    DWORD iterations = 0;
    const DWORD MAX_ITERATIONS = 100000;  
    
    while (iterations < MAX_ITERATIONS)
    {
        status = inb(io_base + ATA_REG_STATUS);
        
        if (status == 0xFF)
        {
            return 0xFF;
        }
        
        if ((status & mask) == value)
        {
            return status;
        }
        
        iterations++;
    }
    
    return 0xFF; 
}

BOOLEAN ATA_IdentifyDevice(WORD io_base, WORD ctrl_base, BYTE device, PATA_IDENTIFY_RESPONSE response)
{
    BYTE status;
    WORD* buffer = (WORD*)response;
    
  
    BYTE device_select = (device == 0) ? 0xA0 : 0xB0;
    outb(io_base + ATA_REG_DEVICE, device_select);
    
    ATA_Wait400NS(ctrl_base);

    outb(ctrl_base + ATA_REG_CONTROL, 0x02);

    outb(io_base + ATA_REG_COMMAND, ATA_CMD_IDENTIFY);
   
    ATA_Wait400NS(ctrl_base);

    status = inb(io_base + ATA_REG_STATUS);
   
    if (status == 0)
    {
        return FALSE;
    }
    
   
    if (status == 0xFF)
    {
        return FALSE;
    }
   
    status = ATA_WaitStatus(io_base, ATA_SR_BSY, 0, 5000);
    if (status == 0xFF)
    {
        return FALSE;  
    }
    
    BYTE lba_mid = inb(io_base + ATA_REG_LBA_MID);
    BYTE lba_high = inb(io_base + ATA_REG_LBA_HIGH);
    
    if ((lba_mid == 0x14 && lba_high == 0xEB) ||
        (lba_mid == 0x3C && lba_high == 0xC3) ||
        (lba_mid == 0x69 && lba_high == 0x96))
    {
       
        return FALSE;
    }
    
    if (status & ATA_SR_ERR)
    {
        return FALSE;
    }
    
    status = ATA_WaitStatus(io_base, ATA_SR_DRQ, ATA_SR_DRQ, 5000);
    if (status == 0xFF || !(status & ATA_SR_DRQ))
    {
        return FALSE;
    }
    
    for (int i = 0; i < 256; i++)
    {
        buffer[i] = inw(io_base + ATA_REG_DATA);
    }
    
    return TRUE;
}

BOOLEAN ATA_DetectDevice(WORD io_base, WORD ctrl_base, BYTE device)
{
    ATA_IDENTIFY_RESPONSE identify;
    memset(&identify, 0, sizeof(ATA_IDENTIFY_RESPONSE));
    
    return ATA_IdentifyDevice(io_base, ctrl_base, device, &identify);
}

BOOLEAN ATA_ReadSectorsPIO(PATA_DEVICE device, DWORD lba, BYTE sector_count, PVOID buffer)
{
    WORD* word_buffer = (WORD*)buffer;
    BYTE status;
    
    if (!device || !device->present || sector_count == 0)
    {
        return FALSE;
    }
    
  
    if (lba >= 0x10000000)
    {
        return FALSE;
    }

    outb(device->ctrl_base + ATA_REG_CONTROL, 0x02);

    BYTE device_select = 0xE0 | ((device->device_number & 1) << 4) | ((lba >> 24) & 0x0F);
    outb(device->io_base + ATA_REG_DEVICE, device_select);
    ATA_Wait400NS(device->ctrl_base);
  
    outb(device->io_base + ATA_REG_SECTOR_COUNT, sector_count);
    outb(device->io_base + ATA_REG_LBA_LOW, (BYTE)(lba & 0xFF));
    outb(device->io_base + ATA_REG_LBA_MID, (BYTE)((lba >> 8) & 0xFF));
    outb(device->io_base + ATA_REG_LBA_HIGH, (BYTE)((lba >> 16) & 0xFF));

    outb(device->io_base + ATA_REG_COMMAND, ATA_CMD_READ_SECTORS);
  
    for (BYTE sector = 0; sector < sector_count; sector++)
    {
  
        status = ATA_WaitStatus(device->io_base, ATA_SR_BSY, 0, 5000);
        if (status == 0xFF)
        {
            return FALSE;
        }
  
        if (status & (ATA_SR_ERR | ATA_SR_DF))
        {
            return FALSE;
        }
        
        status = ATA_WaitStatus(device->io_base, ATA_SR_DRQ, ATA_SR_DRQ, 5000);
        if (status == 0xFF || !(status & ATA_SR_DRQ))
        {
            return FALSE;
        }
        
        for (int i = 0; i < 256; i++)
        {
            word_buffer[sector * 256 + i] = inw(device->io_base + ATA_REG_DATA);
        }
      
        ATA_Wait400NS(device->ctrl_base);
    }
    
    return TRUE;
}

BOOLEAN ATA_WriteSectorsPIO(PATA_DEVICE device, DWORD lba, BYTE sector_count, PVOID buffer)
{
    WORD* word_buffer = (WORD*)buffer;
    BYTE status;
    
    if (!device || !device->present || sector_count == 0)
    {
        return FALSE;
    }
    
    if (lba >= 0x10000000)
    {
        return FALSE;
    }

    outb(device->ctrl_base + ATA_REG_CONTROL, 0x02);

    BYTE device_select = 0xE0 | ((device->device_number & 1) << 4) | ((lba >> 24) & 0x0F);
    outb(device->io_base + ATA_REG_DEVICE, device_select);
    ATA_Wait400NS(device->ctrl_base);
  
    outb(device->io_base + ATA_REG_SECTOR_COUNT, sector_count);
    outb(device->io_base + ATA_REG_LBA_LOW, (BYTE)(lba & 0xFF));
    outb(device->io_base + ATA_REG_LBA_MID, (BYTE)((lba >> 8) & 0xFF));
    outb(device->io_base + ATA_REG_LBA_HIGH, (BYTE)((lba >> 16) & 0xFF));

    outb(device->io_base + ATA_REG_COMMAND, ATA_CMD_WRITE_SECTORS);
  
    for (BYTE sector = 0; sector < sector_count; sector++)
    {
        status = ATA_WaitStatus(device->io_base, ATA_SR_BSY, 0, 5000);
        if (status == 0xFF)
        {
            return FALSE;
        }
  
        if (status & (ATA_SR_ERR | ATA_SR_DF))
        {
            return FALSE;
        }
        
        status = ATA_WaitStatus(device->io_base, ATA_SR_DRQ, ATA_SR_DRQ, 5000);
        if (status == 0xFF || !(status & ATA_SR_DRQ))
        {
            return FALSE;
        }
        
        for (int i = 0; i < 256; i++)
        {
            outw(device->io_base + ATA_REG_DATA, word_buffer[sector * 256 + i]);
        }
      
        ATA_Wait400NS(device->ctrl_base);
    }
    
    status = ATA_WaitStatus(device->io_base, ATA_SR_BSY, 0, 5000);
    if (status == 0xFF)
    {
        return FALSE;
    }
    
    if (status & (ATA_SR_ERR | ATA_SR_DF))
    {
        return FALSE;
    }
    
    return TRUE;
}

void ATA_PrintDeviceInfo(PATA_DEVICE device)
{
    char buffer[128];
    char model[41];
    char serial[21];
    
    if (!device || !device->present)
    {
        Log("No device present");
        return;
    }
    
    for (int i = 0; i < 20; i++)
    {
        model[i * 2] = device->identify_data.ModelNumber[i * 2 + 1];
        model[i * 2 + 1] = device->identify_data.ModelNumber[i * 2];
    }
    model[40] = '\0';
    
   
    for (int i = 0; i < 10; i++)
    {
        serial[i * 2] = device->identify_data.SerialNumbers[i * 2 + 1];
        serial[i * 2 + 1] = device->identify_data.SerialNumbers[i * 2];
    }
    serial[20] = '\0';
    
    for (int i = 39; i >= 0 && (model[i] == ' ' || model[i] == '\0'); i--)
    {
        model[i] = '\0';
    }
    
    Log("  Model: ");
    Log(model);
    Log("  Serial: ");
    Log(serial);
    
    memset(buffer, 0, sizeof(buffer));
    DWORD sectors = device->identify_data.Address28Bit;
    DWORD size_mb = (sectors / 2048); 
    
    cl_snprintf(buffer, sizeof(buffer), "  Capacity: %d MB (%d sectors)", size_mb, sectors);
    Log(buffer);
}

void ATA_Init(void)
{
    memset(&gAtaController, 0, sizeof(ATA_CONTROLLER));
    gAtaController.device_count = 0;
    
    Log("Initializing ATA subsystem...");

    BYTE test_status = inb(ATA_PRIMARY_IO_BASE + ATA_REG_STATUS);
    if (test_status == 0xFF)
    {
        Log("No ATA controller detected (floating bus)");
        return;
    }
   
    WORD io_bases[] = { ATA_PRIMARY_IO_BASE, ATA_SECONDARY_IO_BASE };
    WORD ctrl_bases[] = { ATA_PRIMARY_CTRL_BASE, ATA_SECONDARY_CTRL_BASE };
    const char* bus_names[] = { "Primary", "Secondary" };
    const char* device_names[] = { "Master", "Slave" };
    
    DWORD device_index = 0;
    
    for (int bus = 0; bus < 2; bus++)
    {
        for (int dev = 0; dev < 2; dev++)
        {
            PATA_DEVICE device = &gAtaController.devices[device_index];
            device->io_base = io_bases[bus];
            device->ctrl_base = ctrl_bases[bus];
            device->device_number = (BYTE)dev;
            device->present = FALSE;
            device->is_atapi = FALSE;
            
            if (ATA_IdentifyDevice(io_bases[bus], ctrl_bases[bus], (BYTE)dev, &device->identify_data))
            {
                device->present = TRUE;
                gAtaController.device_count++;
                
                char msg[64];
                cl_snprintf(msg, sizeof(msg), "Found %s %s ATA device", bus_names[bus], device_names[dev]);
                Log(msg);
                ATA_PrintDeviceInfo(device);
            }
            
            device_index++;
        }
    }
    
    char summary[64];
    cl_snprintf(summary, sizeof(summary), "ATA init done. %d device(s)", gAtaController.device_count);
    Log(summary);
}

PATA_DEVICE ATA_GetDevice(DWORD index)
{
    if (index >= 4)
    {
        return NULL;
    }
    
    PATA_DEVICE device = &gAtaController.devices[index];
    return device->present ? device : NULL;
}

DWORD ATA_GetDeviceCount(void)
{
    return gAtaController.device_count;
}
