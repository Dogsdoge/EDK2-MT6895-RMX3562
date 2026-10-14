/*
 * pearl (Xiaomi Redmi Note 12T Pro, MT6895) 12GB DRAM memory map.
 *
 * Reserved regions are taken from the LK "mblock" reservations (visible in
 * the mainline boot log as "OF: reserved mem ... mblock-NN-*") plus the
 * BL31/ATF/TEE layout, so EDK2's GCD/DXE never allocates from them and they
 * are reported reserved to the OS. The framebuffer (LK scanout region
 * 0xfe14f000, three slots of ~0xa3c000, visible 1080x2460 with a 1088px
 * pitch) is mapped write-through for the SimpleFbDxe handoff.
 */
#include <Library/BaseLib.h>
#include <Library/PlatformMemoryMapLib.h>

static ARM_MEMORY_REGION_DESCRIPTOR_EX gDeviceMemoryDescriptorEx[] = {
/*                                                    EFI_RESOURCE_ EFI_RESOURCE_ATTRIBUTE_ EFI_MEMORY_TYPE ARM_REGION_ATTRIBUTE_
     MemLabel(32 Char.),  MemBase,    MemSize, BuildHob, ResourceType, ResourceAttribute, MemoryType, CacheAttributes
--------------------- Register ---------------------*/
    {"Periphs",                0x0000000000, 0x040000000, AddMem, MEM_RES   , UNCACHEABLE   , RtCode , NS_DEVICE},
    {"UEFI DDR Low",           0x0040000000, 0x00c00000, AddMem, SYS_MEM   , SYS_MEM_CAP   , Conv   , WRITE_BACK},
    {"UEFI Stack",             0x0040c00000, 0x00040000, AddMem, SYS_MEM   , SYS_MEM_CAP   , BsData , WRITE_BACK},
    {"CPU Vectors",            0x0040c40000, 0x00010000, AddMem, SYS_MEM   , SYS_MEM_CAP   , BsCode , WRITE_BACK},
    {"DXE Heap",               0x0040c50000, 0x007430000, AddMem, SYS_MEM  , SYS_MEM_CAP   , Conv   , WRITE_BACK},
    {"Res kinfo",              0x0048080000, 0x00010000, AddMem, MEM_RES   , SYS_MEM_CAP   , Reserv , WRITE_BACK},
    {"DDR 0x48090000",         0x0048090000, 0x000e0000, AddMem, SYS_MEM   , SYS_MEM_CAP   , Conv   , WRITE_BACK},
    {"UEFI Log",               0x0048170000, 0x00010000, AddMem, MEM_RES   , SYS_MEM_CAP   , Reserv , WRITE_THROUGH_XN},
    {"DDR 0x48180000",         0x0048180000, 0x00080000, AddMem, SYS_MEM   , SYS_MEM_CAP   , Conv   , WRITE_BACK},
    {"Res BL31",               0x0048200000, 0x00200000, AddMem, MEM_RES   , SYS_MEM_CAP   , Reserv , WRITE_BACK},
    {"DDR 0x48400000",         0x0048400000, 0x00001000, AddMem, SYS_MEM   , SYS_MEM_CAP   , Conv   , WRITE_BACK},
    {"Res crypto",             0x0048401000, 0x00001000, AddMem, MEM_RES   , SYS_MEM_CAP   , Reserv , WRITE_BACK},
    {"DDR 0x48402000",         0x0048402000, 0x007bfe000, AddMem, SYS_MEM  , SYS_MEM_CAP   , Conv   , WRITE_BACK},
    {"UEFI FD",                0x0050000000, 0x000700000, AddMem, SYS_MEM   , SYS_MEM_CAP   , BsCode , WRITE_BACK},
    {"Res aee_lk",             0x0050700000, 0x00800000, AddMem, MEM_RES   , SYS_MEM_CAP   , Reserv , WRITE_BACK},
    {"DDR 0x50f00000",         0x0050f00000, 0x01f100000, AddMem, SYS_MEM  , SYS_MEM_CAP   , Conv   , WRITE_BACK},
    {"Res TEE",                0x0070000000, 0x06000000, AddMem, MEM_RES   , SYS_MEM_CAP   , Reserv , WRITE_BACK},
    {"DDR 0x76000000",         0x0076000000, 0x01ff0000, AddMem, SYS_MEM   , SYS_MEM_CAP   , Conv   , WRITE_BACK},
    {"Res 0x77ff0000",         0x0077ff0000, 0x00010000, AddMem, MEM_RES   , SYS_MEM_CAP   , Reserv , WRITE_BACK},
    {"DDR 0x78000000",         0x0078000000, 0x00c00000, AddMem, SYS_MEM   , SYS_MEM_CAP   , Conv   , WRITE_BACK},
    {"Res GPU mputab",         0x0078c00000, 0x01400000, AddMem, MEM_RES   , SYS_MEM_CAP   , Reserv , WRITE_BACK},
    {"Res adsp image",         0x007a000000, 0x01230000, AddMem, MEM_RES   , SYS_MEM_CAP   , Reserv , WRITE_BACK},
    {"DDR 0x7b230000",         0x007b230000, 0x00ec0000, AddMem, SYS_MEM   , SYS_MEM_CAP   , Conv   , WRITE_BACK},
    {"Res sspm-ap",            0x007c0f0000, 0x00110000, AddMem, MEM_RES   , SYS_MEM_CAP   , Reserv , WRITE_BACK},
    {"Res GPUEB",              0x007c200000, 0x00200000, AddMem, MEM_RES   , SYS_MEM_CAP   , Reserv , WRITE_BACK},
    {"Res 0x7c400000",         0x007c400000, 0x00400000, AddMem, MEM_RES   , SYS_MEM_CAP   , Reserv , WRITE_BACK},
    {"Res TEE SecMem",         0x007c800000, 0x00700000, AddMem, MEM_RES   , SYS_MEM_CAP   , Reserv , WRITE_BACK},
    {"Res consys-gps",         0x007cf00000, 0x00100000, AddMem, MEM_RES   , SYS_MEM_CAP   , Reserv , WRITE_BACK},
    {"Res smmu protpgd",       0x007d000000, 0x02200000, AddMem, MEM_RES   , SYS_MEM_CAP   , Reserv , WRITE_BACK},
    {"Res 0x7f200000",         0x007f200000, 0x00041000, AddMem, MEM_RES   , SYS_MEM_CAP   , Reserv , WRITE_BACK},
    {"DDR 0x7f241000",         0x007f241000, 0x000bf000, AddMem, SYS_MEM   , SYS_MEM_CAP   , Conv   , WRITE_BACK},
    {"Res MCUPM",              0x007f300000, 0x00100000, AddMem, MEM_RES   , SYS_MEM_CAP   , Reserv , WRITE_BACK},
    {"Res 0x7f400000",         0x007f400000, 0x00400000, AddMem, MEM_RES   , SYS_MEM_CAP   , Reserv , WRITE_BACK},
    {"Res 0x7f800000",         0x007f800000, 0x00600000, AddMem, MEM_RES   , SYS_MEM_CAP   , Reserv , WRITE_BACK},
    {"DDR 0x7fe00000",         0x007fe00000, 0x0003f000, AddMem, SYS_MEM   , SYS_MEM_CAP   , Conv   , WRITE_BACK},
    {"Res SSPM",               0x007fe3f000, 0x00180000, AddMem, MEM_RES   , SYS_MEM_CAP   , Reserv , WRITE_BACK},
    {"Res 0x7ffbf000",         0x007ffbf000, 0x00040000, AddMem, MEM_RES   , SYS_MEM_CAP   , Reserv , WRITE_BACK},
    {"Res 0x7ffff000",         0x007ffff000, 0x00001000, AddMem, MEM_RES   , SYS_MEM_CAP   , Reserv , WRITE_BACK},
    {"Res 0x80000000",         0x0080000000, 0x00100000, AddMem, MEM_RES   , SYS_MEM_CAP   , Reserv , WRITE_BACK},
    {"DDR 0x80100000",         0x0080100000, 0x09f00000, AddMem, SYS_MEM   , SYS_MEM_CAP   , Conv   , WRITE_BACK},
    {"Res ap_md_c_smem",       0x008a000000, 0x02360000, AddMem, MEM_RES   , SYS_MEM_CAP   , Reserv , WRITE_BACK},
    {"DDR 0x8c360000",         0x008c360000, 0x01ca0000, AddMem, SYS_MEM   , SYS_MEM_CAP   , Conv   , WRITE_BACK},
    {"Res ap_md_nc_smem",      0x008e000000, 0x00120000, AddMem, MEM_RES   , SYS_MEM_CAP   , Reserv , WRITE_BACK},
    {"DDR 0x8e120000",         0x008e120000, 0x003e0000, AddMem, SYS_MEM   , SYS_MEM_CAP   , Conv   , WRITE_BACK},
    {"Res consys_emi",         0x008e500000, 0x00800000, AddMem, MEM_RES   , SYS_MEM_CAP   , Reserv , WRITE_BACK},
    {"DDR 0x8ed00000",         0x008ed00000, 0x00090000, AddMem, SYS_MEM   , SYS_MEM_CAP   , Conv   , WRITE_BACK},
    {"Res apu 0x8ed90000",     0x008ed90000, 0x00270000, AddMem, MEM_RES   , SYS_MEM_CAP   , Reserv , WRITE_BACK},
    {"Res scp share",          0x008f000000, 0x006a2000, AddMem, MEM_RES   , SYS_MEM_CAP   , Reserv , WRITE_BACK},
    {"DDR 0x8f6a2000",         0x008f6a2000, 0x0025e000, AddMem, SYS_MEM   , SYS_MEM_CAP   , Conv   , WRITE_BACK},
    {"Res apu 0x8f900000",     0x008f900000, 0x00700000, AddMem, MEM_RES   , SYS_MEM_CAP   , Reserv , WRITE_BACK},
    {"DDR 0x90000000",         0x0090000000, 0x0f7ff000, AddMem, SYS_MEM   , SYS_MEM_CAP   , Conv   , WRITE_BACK},
    {"Res 0x9f7ff000",         0x009f7ff000, 0x00800000, AddMem, MEM_RES   , SYS_MEM_CAP   , Reserv , WRITE_BACK},
    {"DDR 0xa0000000",         0x00a0000000, 0x1d000000, AddMem, SYS_MEM   , SYS_MEM_CAP   , Conv   , WRITE_BACK},
    {"Res wifi dma",           0x00bd000000, 0x01400000, AddMem, MEM_RES   , SYS_MEM_CAP   , Reserv , WRITE_BACK},
    {"DDR 0xbe400000",         0x00be400000, 0x00a00000, AddMem, SYS_MEM   , SYS_MEM_CAP   , Conv   , WRITE_BACK},
    {"Res vcp",                0x00bee00000, 0x00800000, AddMem, MEM_RES   , SYS_MEM_CAP   , Reserv , WRITE_BACK},
    {"DDR 0xbf600000",         0x00bf600000, 0x000f0000, AddMem, SYS_MEM   , SYS_MEM_CAP   , Conv   , WRITE_BACK},
    {"Res ccci tag",           0x00bf6f0000, 0x00010000, AddMem, MEM_RES   , SYS_MEM_CAP   , Reserv , WRITE_BACK},
    {"Res apu 0xbf700000",     0x00bf700000, 0x00200000, AddMem, MEM_RES   , SYS_MEM_CAP   , Reserv , WRITE_BACK},
    {"Res scp 0xbf900000",     0x00bf900000, 0x00500000, AddMem, MEM_RES   , SYS_MEM_CAP   , Reserv , WRITE_BACK},
    {"Res 0xbfe00000",         0x00bfe00000, 0x00200000, AddMem, MEM_RES   , SYS_MEM_CAP   , Reserv , WRITE_BACK},
    {"DDR 0xc0000000",         0x00c0000000, 0x10000000, AddMem, SYS_MEM   , SYS_MEM_CAP   , Conv   , WRITE_BACK},
    {"Res md 0xd0000000",      0x00d0000000, 0x02710000, AddMem, MEM_RES   , SYS_MEM_CAP   , Reserv , WRITE_BACK},
    {"DDR 0xd2710000",         0x00d2710000, 0x008f0000, AddMem, SYS_MEM   , SYS_MEM_CAP   , Conv   , WRITE_BACK},
    {"Res md 0xd3000000",      0x00d3000000, 0x00240000, AddMem, MEM_RES   , SYS_MEM_CAP   , Reserv , WRITE_BACK},
    {"DDR 0xd3240000",         0x00d3240000, 0x00dc0000, AddMem, SYS_MEM   , SYS_MEM_CAP   , Conv   , WRITE_BACK},
    {"Res md 0xd4000000",      0x00d4000000, 0x07870000, AddMem, MEM_RES   , SYS_MEM_CAP   , Reserv , WRITE_BACK},
    {"DDR 0xdb870000",         0x00db870000, 0x11e70000, AddMem, SYS_MEM   , SYS_MEM_CAP   , Conv   , WRITE_BACK},
    {"Res md 0xed6e0000",      0x00ed6e0000, 0x00920000, AddMem, MEM_RES   , SYS_MEM_CAP   , Reserv , WRITE_BACK},
    {"DDR 0xee000000",         0x00ee000000, 0x0a000000, AddMem, SYS_MEM   , SYS_MEM_CAP   , Conv   , WRITE_BACK},
    {"Res adsp shared",        0x00f8000000, 0x00660000, AddMem, MEM_RES   , SYS_MEM_CAP   , Reserv , WRITE_BACK},
    {"DDR 0xf8660000",         0x00f8660000, 0x049a0000, AddMem, SYS_MEM   , SYS_MEM_CAP   , Conv   , WRITE_BACK},
    {"Res dfdmcu",             0x00fd000000, 0x00280000, AddMem, MEM_RES   , SYS_MEM_CAP   , Reserv , WRITE_BACK},
    {"DDR 0xfd280000",         0x00fd280000, 0x00ecf000, AddMem, SYS_MEM   , SYS_MEM_CAP   , Conv   , WRITE_BACK},
    {"Framebuffer",            0x00fe14f000, 0x01eb0000, AddMem, MEM_RES   , SYS_MEM_CAP   , Reserv , WRITE_THROUGH_XN},
    {"DDR 0xfffff000",         0x00fffff000, 0x240010000, AddMem, SYS_MEM  , SYS_MEM_CAP   , Conv   , WRITE_BACK},

//------------------- Terminator for MMU ---------------------
{"Terminator", 0, 0, 0, 0, 0, 0, 0}};

ARM_MEMORY_REGION_DESCRIPTOR_EX *GetPlatformMemoryMap()
{
  return gDeviceMemoryDescriptorEx;
}
