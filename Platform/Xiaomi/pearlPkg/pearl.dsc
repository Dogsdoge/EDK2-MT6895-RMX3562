[Defines]
  PLATFORM_NAME                  = pearl
  PLATFORM_GUID                  = 4A7C2E18-3B9D-4F52-8E64-1C5A9D7F0B23
  PLATFORM_VERSION               = 0.1
  DSC_SPECIFICATION              = 0x00010019
  OUTPUT_DIRECTORY               = Build/$(PLATFORM_NAME)
  SUPPORTED_ARCHITECTURES        = AARCH64
  BUILD_TARGETS                  = DEBUG|RELEASE
  SKUID_IDENTIFIER               = DEFAULT
  FLASH_DEFINITION               = pearlPkg/pearl.fdf
  DEVICE_DXE_FV_COMPONENTS       = pearlPkg/pearl.fdf.inc

[PcdsFixedAtBuild.common]
  # Xiaomi Redmi Note 12T Pro (pearl), MT6895, 12GB LPDDR5
  gArmTokenSpaceGuid.PcdSystemMemoryBase|0x40000000
  gArmTokenSpaceGuid.PcdSystemMemorySize|0x300000000 # 12GB
  gArmTokenSpaceGuid.PcdCpuVectorBaseAddress|0x40C40000
  gEmbeddedTokenSpaceGuid.PcdPrePiStackBase|0x40C00000
  gEmbeddedTokenSpaceGuid.PcdPrePiStackSize|0x00040000
  # DXE heap ends at 0x48080000 (LK kinfo/minirdump reservation starts there)
  gMediaTekTokenSpaceGuid.PcdUefiMemPoolBase|0x40C50000
  gMediaTekTokenSpaceGuid.PcdUefiMemPoolSize|0x007430000

  # Simple Framebuffer: pearl LK scanout region 0xfe14f000 (3 slots of
  # ~0xa3c000, mblock-12-framebuffer). Width is the scan-buffer pitch
  # (1088px = 4352B row), VisibleWidth the real 1080px panel width.
  gMediaTekTokenSpaceGuid.PcdMipiFrameBufferWidth|1088
  gMediaTekTokenSpaceGuid.PcdMipiFrameBufferVisibleWidth|1080
  gMediaTekTokenSpaceGuid.PcdMipiFrameBufferHeight|2460
  gMediaTekTokenSpaceGuid.PcdMipiFrameBufferLayers|3
  gMediaTekTokenSpaceGuid.PcdMipiFrameBufferAddress|0xfe14f000
  gMediaTekTokenSpaceGuid.PcdMipiFrameBufferPixelFormat|1   # RGB (a8r8g8b8 word order)

  # Display overlay (OVL) takeover: mt6895 disp_ovl0 @ 0x14002000,
  # disp_ovl0_2l @ 0x14003000 (mainline mt6895.dtsi).
  gMediaTekTokenSpaceGuid.PcdMipiFrameBufferOvl0Base|0x14002000
  gMediaTekTokenSpaceGuid.PcdMipiFrameBufferOvl0_2LBase|0x14003000

[LibraryClasses]
  PlatformMemoryMapLib|pearlPkg/Library/PlatformMemoryMapLib/PlatformMemoryMapLib.inf

!include Silicon/MediaTek/MT6895Pkg/MT6895.dsc

[PcdsFixedAtBuild.common]
  # Bring-up: INFO-level debug output (driver loads, init messages) without
  # the POOL/LOAD spam. Printing is already enabled in RELEASE
  # (PcdDebugPropertyMask=0x0e); asserts stay compiled out.
  gEfiMdePkgTokenSpaceGuid.PcdDebugPrintErrorLevel|0x80000046
