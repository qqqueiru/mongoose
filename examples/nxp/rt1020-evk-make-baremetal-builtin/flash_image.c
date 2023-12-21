#include "dcd.h"      // pin settings for MIMXRT1020-EVK board
#include "hal.h"
#include "flexspi.h"  // peripheral structures

extern uint32_t __isr_vector[];

// RM 9.7.2
__attribute__((section(".dcd"), used))
const uint8_t __ivt_dcd_data[] = {__DCD_DATA};

// RM 9.7.1
__attribute__((section(".dat"), used)) const uint32_t __ivt_boot_data[] = {
    FlexSPI_AMBA_BASE,  // boot start location
    8 * 1024 * 1024,    // size
    0,                  // Plugin flag
    0Xffffffff          // empty - extra data word
};

__attribute__((section(".ivt"), used)) const uint32_t __ivt[8] = {
    0x412000d1,                  // header: 41 - version, 2000 size, d1 tag
    (uint32_t) __isr_vector,     // entry
    0,                           // reserved
    (uint32_t) __ivt_dcd_data,   // dcd
    (uint32_t) __ivt_boot_data,  // boot data
    (uint32_t) __ivt,            // this is us - ivt absolute address
    0,                           // csf absolute address
    0,                           // reserved for HAB
};

// MIMXRT1060-EVKB flash chip config: S25LP064A-JBLE
__attribute__((section(".cfg"), used))
const flexspi_nor_config_t __qspi_flash_cfg = {
    .memConfig = {.tag = FLEXSPI_CFG_BLK_TAG,
                  .version = FLEXSPI_CFG_BLK_VERSION,
                  .readSampleClkSrc = 1,  // ReadSampleClk_LoopbackFromDqsPad
                  .csHoldTime = 3,
                  .csSetupTime = 3,
                  .controllerMiscOption = BIT(4),
                  .deviceType = 1,  // serial NOR
                  .sflashPadType = 4,
                  .serialClkFreq = 7,  // 133MHz
                  .sflashA1Size = 8 * 1024 * 1024,
                  .lookupTable = __FLEXSPI_QSPI_LUT},
    .pageSize = 256,
    .sectorSize = 4 * 1024,
    .ipcmdSerialClkFreq = 1,
    .blockSize = 64 * 1024,
    .isUniformBlockSize = false};
