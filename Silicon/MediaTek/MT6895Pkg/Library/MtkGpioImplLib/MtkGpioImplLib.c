/*
 * MT6895 GPIO Configuration
 *
 * The GPIO bank base 0x10005000 matches the mainline mt6895 pinctrl "gpio"
 * register range. The DRV/R/PUPD tables are intentionally minimal: the pearl
 * key pins are not mapped yet and LK has already configured every pin, so
 * UEFI must not touch them. The single placeholder entry keeps any stray
 * table lookup inside the GPIO register bank.
 */

#include <Protocol/MtkGpio.h>

#define GPIO_BASE (0x10005000)

MTK_GPIO_PLATFORM_INFO PlatformInfo = {
  .BaseAddr      = GPIO_BASE,
  .SetOffset     = 0x4,
  .ResetOffset   = 0x8,
  .DirOffset     = 0x0,
  .DataOutOffset = 0x100,
  .DataInOffset  = 0x200,
  .ModeOffset    = 0x300,
  .MaxPin        = 0
};

struct MTK_GPIO_DRV_TABLE DrvTable[] = {
  { .Pin = 0, .Base = GPIO_BASE, .Mask = 0, .Shift = 0 },
};

struct MTK_GPIO_R_TABLE RTable[] = {
  { .Pin = 0, .R0Base = GPIO_BASE, .R1Base = GPIO_BASE, .R0Shift = 0, .R1Shift = 0 },
};

struct MTK_GPIO_PUPD_TABLE PupdTable[] = {
  { .Pin = 0, .Base = GPIO_BASE, .Shift = 0 },
};
