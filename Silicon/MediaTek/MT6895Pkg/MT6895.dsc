## @file
#
#  Copyright (c) 2011-2015, ARM Limited. All rights reserved.
#  Copyright (c) 2014, Linaro Limited. All rights reserved.
#  Copyright (c) 2015 - 2016, Intel Corporation. All rights reserved.
#  Copyright (c) 2018 - 2019, Bingxing Wang. All rights reserved.
#  Copyright (c) 2022, Xilin Wu. All rights reserved.
#
#  SPDX-License-Identifier: BSD-2-Clause-Patent
#
##

################################################################################
#
# Defines Section - statements that will be processed to create a Makefile.
#
################################################################################

[Defines]
  SOC_PLATFORM            = mt6895
  USE_PHYSICAL_TIMER      = TRUE
  HAS_BROKEN_TIMER        = TRUE

!include MediaTekPkg/MediaTekCommonDsc.inc

[PcdsFixedAtBuild.common]
  # mt6895 timer: 13 MHz, standard ARMv8 PPIs (GIC 29/30/27/26)
  gArmTokenSpaceGuid.PcdArmArchTimerFreqInHz|13000000
  gArmTokenSpaceGuid.PcdArmArchTimerSecIntrNum|29
  gArmTokenSpaceGuid.PcdArmArchTimerIntrNum|30
  gArmTokenSpaceGuid.PcdArmArchTimerVirtIntrNum|27
  gArmTokenSpaceGuid.PcdArmArchTimerHypIntrNum|26
  gArmTokenSpaceGuid.PcdGicDistributorBase|0x0c000000
  gArmTokenSpaceGuid.PcdGicRedistributorsBase|0x0c040000
  gEfiMdeModulePkgTokenSpaceGuid.PcdAcpiDefaultOemRevision|0x00000850

  gMediaTekTokenSpaceGuid.PcdMipiFrameBufferLayers|3
  gMediaTekTokenSpaceGuid.PcdWatchdogWorkingTimer|0
  # MT6895 watchdog lives at 0x1c007000 (mainline: mtk-wdt 1c007000.watchdog),
  # not the 0x10007000 default used by older MediaTek SoCs.
  gMediaTekTokenSpaceGuid.PcdWatchdogBaseAddress|0x1c007000

  # MT6895 (Dimensity 8200): 4x Cortex-A55 + 3x Cortex-A78 + 1x Cortex-A78
  # MPIDR clusters: 0x000 (A55), 0x100 (A78), 0x200 (A78 prime)
  gArmPlatformTokenSpaceGuid.PcdCoreCount|8
  gArmPlatformTokenSpaceGuid.PcdClusterCount|3

[LibraryClasses.common]
  KeypadDeviceImplLib|MT6895Pkg/Library/KeypadDeviceImplLib/KeypadDeviceImplLib.inf
  PlatformPeiLib|MT6895Pkg/Library/PlatformPeiLib/PlatformPeiLib.inf
  PlatformPrePiLib|MT6895Pkg/Library/PlatformPrePiLib/PlatformPrePiLib.inf
  MtkGpioImplLib|MT6895Pkg/Library/MtkGpioImplLib/MtkGpioImplLib.inf
  PmicWrapperImplLib|MT6895Pkg/Library/PmicWrapperImplLib/PmicWrapperImplLib.inf
  # NOTE: the MT6895 PMICs (MT6363/MT6368) hang off the SPMI bus (pmif @
  # 0x1c804000), not the legacy PWRAP block. PMIC drivers and the hardware RTC
  # are not implemented yet, so PmicWrapperDxe/MT6358PmicDxe are not shipped
  # on pearl and the common virtual RTC library is used instead.

[Components.common]
  MediaTekPkg/Drivers/MtkGpioDxe/MtkGpioDxe.inf
