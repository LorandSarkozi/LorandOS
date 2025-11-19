#pragma pack(push,1)
typedef enum _MEMORY_MAP_TYPE
{
    MemoryMapTypeUsableRAM = 1, // singurul tip de memorie utilizabila
    MemoryMapTypeReserved,
    MemoryMapTypeACPIReclaimable,
    MemoryMapTypeACPINVSMemory,
    MemoryMapTypeBadMemory,
    MemoryMapTypeMax = MemoryMapTypeBadMemory + 1
} MEMORY_MAP_TYPE;

#define MEMORY_MAP_ENTRY_EA_VALID_ENTRY             ((DWORD)1<<0)
#define MEMORY_MAP_ENTRY_EA_NON_VOLATILE            ((DWORD)1<<1)

typedef struct _INT15_MEMORY_MAP_ENTRY
{
    QWORD           BaseAddress;
    QWORD           Length;
    DWORD           Type;
    DWORD           ExtendedAttributes;
} INT15_MEMORY_MAP_ENTRY;
#pragma pack(pop)

INT15_MEMORY_MAP_ENTRY int15_e820_entries[] = {

    // first MB of memory is not retrieved properly by INT15H E820,
    // for first megabyte availability you can consult https://wiki.osdev.org/Memory_Map_(x86)#Real_mode_address_space_.28.3C_1_MiB.29
    //{ 0x0, 0x9F000, MemoryMapTypeUsableRAM, MEMORY_MAP_ENTRY_EA_VALID_ENTRY},
    {0x9F000, 0x1000, MemoryMapTypeReserved, MEMORY_MAP_ENTRY_EA_VALID_ENTRY},
    {0xE8000, 0x18000, MemoryMapTypeReserved, MEMORY_MAP_ENTRY_EA_VALID_ENTRY},
    {0x100000, 0x1FEF0000, MemoryMapTypeUsableRAM, MEMORY_MAP_ENTRY_EA_VALID_ENTRY},
    {0x1FFF0000, 0x10000, MemoryMapTypeACPIReclaimable, MEMORY_MAP_ENTRY_EA_VALID_ENTRY},
    {0xFFFC0000, 0x40000, MemoryMapTypeReserved, MEMORY_MAP_ENTRY_EA_VALID_ENTRY}
};