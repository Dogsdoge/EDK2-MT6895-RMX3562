/** @file
  MediaTek UFS host controller platform driver (MT6895 / pearl).

  Provides the EDKII_UFS_HOST_CONTROLLER_PROTOCOL consumed by the generic
  MdeModulePkg UfsPassThruDxe driver.

  The bootloader (LK) already brings up the UFS PHY, clocks and the UniPro
  link (it boots from UFS), and the mainline kernel's ufs-mediatek driver
  relies on the same setup (its device tree intentionally omits the clocks,
  resets and PHY for this board). This driver therefore only:

    - sets the MediaTek "CLK_CG early" bit in REG_UFS_XOUFS_CTRL (0x140),
      mirroring ufs_mtk_hce_enable_notify() of the mainline driver, which
      bypasses the abnormal ERR_CHK signal and prevents host hangs
    - exposes the UFSHCI MMIO registers at PcdUfsHostControllerBase
    - provides identity DMA mapping with cache maintenance (the UFS
      controller is not behind a programmed IOMMU on this platform)

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>

#include <Library/BaseLib.h>
#include <Library/CacheMaintenanceLib.h>
#include <Library/DebugLib.h>
#include <Library/IoLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>

#include <Protocol/UfsHostController.h>

#define MTK_UFS_HC_PRIVATE_SIGNATURE  SIGNATURE_32 ('M', 'T', 'K', 'U')

//
// MediaTek vendor registers inside the UFSHCI MMIO range.
//
#define MTK_REG_UFS_XOUFS_CTRL  0x140
#define MTK_XOUFS_CTRL_CLK_CG   BIT7

typedef struct {
  UINT32                              Signature;
  EDKII_UFS_HOST_CONTROLLER_PROTOCOL  UfsHcProtocol;
  UINTN                               UfsHcBase;
} MTK_UFS_HC_PRIVATE_DATA;

#define MTK_UFS_HC_PRIVATE_DATA_FROM_THIS(a) \
  CR (a, MTK_UFS_HC_PRIVATE_DATA, UfsHcProtocol, MTK_UFS_HC_PRIVATE_SIGNATURE)

//
// Bookkeeping returned through the Mapping pointer of Map().
//
typedef struct {
  VOID                                *HostAddress;
  UINTN                               NumberOfBytes;
  EDKII_UFS_HOST_CONTROLLER_OPERATION Operation;
} MTK_UFS_MAPPING;

/**
  Get the MMIO base address of the UFS host controller.

  @param[in]  This      A pointer to the protocol instance.
  @param[out] MmioBar   The MMIO base address of the UFS host controller.

  @retval EFI_SUCCESS   The operation succeeds.
**/
EFI_STATUS
EFIAPI
MtkUfsGetMmioBar (
  IN     EDKII_UFS_HOST_CONTROLLER_PROTOCOL  *This,
  OUT UINTN                                  *MmioBar
  )
{
  MTK_UFS_HC_PRIVATE_DATA  *Private;

  if ((This == NULL) || (MmioBar == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Private  = MTK_UFS_HC_PRIVATE_DATA_FROM_THIS (This);
  *MmioBar = Private->UfsHcBase;

  return EFI_SUCCESS;
}

/**
  Allocate pages suitable for an EdkiiUfsHcOperationBusMasterCommonBuffer
  mapping.

  @param[in]  This         A pointer to the protocol instance.
  @param[in]  Type         The allocation type (passed to AllocatePages).
  @param[in]  MemoryType   The type of memory to allocate.
  @param[in]  Pages        The number of pages to allocate.
  @param[out] HostAddress  The base address of the allocated range.
  @param[in]  Attributes   Requested attributes (ignored; cache maintenance
                           is done in Map()/Unmap()).

  @retval EFI_SUCCESS           The requested memory pages were allocated.
  @retval EFI_INVALID_PARAMETER One or more parameters are invalid.
  @retval EFI_OUT_OF_RESOURCES  The memory pages could not be allocated.
**/
EFI_STATUS
EFIAPI
MtkUfsAllocateBuffer (
  IN     EDKII_UFS_HOST_CONTROLLER_PROTOCOL  *This,
  IN     EFI_ALLOCATE_TYPE                   Type,
  IN     EFI_MEMORY_TYPE                     MemoryType,
  IN     UINTN                               Pages,
  OUT VOID                                   **HostAddress,
  IN     UINT64                              Attributes
  )
{
  EFI_STATUS  Status;

  if ((This == NULL) || (HostAddress == NULL) || (Pages == 0)) {
    return EFI_INVALID_PARAMETER;
  }

  Status = gBS->AllocatePages (Type, MemoryType, Pages, (EFI_PHYSICAL_ADDRESS *)HostAddress);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Make sure no stale cache lines can be written back over the DMA data.
  //
  InvalidateDataCacheRange (*HostAddress, EFI_PAGES_TO_SIZE (Pages));

  return EFI_SUCCESS;
}

/**
  Frees memory that was allocated with AllocateBuffer().

  @param[in] This         A pointer to the protocol instance.
  @param[in] Pages        The number of pages to free.
  @param[in] HostAddress  The base address of the allocated range.

  @retval EFI_SUCCESS           The requested memory pages were freed.
  @retval EFI_INVALID_PARAMETER One or more parameters are invalid.
**/
EFI_STATUS
EFIAPI
MtkUfsFreeBuffer (
  IN  EDKII_UFS_HOST_CONTROLLER_PROTOCOL  *This,
  IN  UINTN                               Pages,
  IN  VOID                                *HostAddress
  )
{
  if ((This == NULL) || (HostAddress == NULL) || (Pages == 0)) {
    return EFI_INVALID_PARAMETER;
  }

  return gBS->FreePages ((EFI_PHYSICAL_ADDRESS)(UINTN)HostAddress, Pages);
}

/**
  Provides the UFS controller-specific addresses needed to access system
  memory. The controller is not behind an IOMMU on this platform, so the
  mapping is the identity; cache maintenance is performed according to the
  transfer direction.

  @param[in]     This           A pointer to the protocol instance.
  @param[in]     Operation      Read/write direction of the DMA transfer.
  @param[in]     HostAddress    The system memory address to map.
  @param[in,out] NumberOfBytes  On input the number of bytes to map, on
                                output the number of bytes mapped.
  @param[out]    DeviceAddress  The resulting device address.
  @param[out]    Mapping        A value to pass to Unmap().

  @retval EFI_SUCCESS           The range was mapped.
  @retval EFI_INVALID_PARAMETER One or more parameters are invalid.
  @retval EFI_OUT_OF_RESOURCES  The request could not be completed.
**/
EFI_STATUS
EFIAPI
MtkUfsMap (
  IN     EDKII_UFS_HOST_CONTROLLER_PROTOCOL   *This,
  IN     EDKII_UFS_HOST_CONTROLLER_OPERATION  Operation,
  IN     VOID                                 *HostAddress,
  IN OUT UINTN                                *NumberOfBytes,
  OUT    EFI_PHYSICAL_ADDRESS                 *DeviceAddress,
  OUT    VOID                                 **Mapping
  )
{
  MTK_UFS_MAPPING  *MapInfo;

  if ((This == NULL) || (HostAddress == NULL) || (NumberOfBytes == NULL) ||
      (DeviceAddress == NULL) || (Mapping == NULL))
  {
    return EFI_INVALID_PARAMETER;
  }

  switch (Operation) {
    case EdkiiUfsHcOperationBusMasterRead:
      //
      // Device reads from system memory: flush the producer's writes so the
      // DMA sees them.
      //
      WriteBackDataCacheRange (HostAddress, *NumberOfBytes);
      break;
    case EdkiiUfsHcOperationBusMasterWrite:
      //
      // Device writes to system memory: drop stale cache lines before the
      // DMA, so they cannot be written back over the device's data.
      //
      InvalidateDataCacheRange (HostAddress, *NumberOfBytes);
      break;
    case EdkiiUfsHcOperationBusMasterCommonBuffer:
      break;
    default:
      return EFI_INVALID_PARAMETER;
  }

  MapInfo = AllocatePool (sizeof (MTK_UFS_MAPPING));
  if (MapInfo == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  MapInfo->HostAddress   = HostAddress;
  MapInfo->NumberOfBytes = *NumberOfBytes;
  MapInfo->Operation     = Operation;

  *DeviceAddress = (EFI_PHYSICAL_ADDRESS)(UINTN)HostAddress;
  *Mapping       = MapInfo;

  return EFI_SUCCESS;
}

/**
  Completes the Map() operation and releases any corresponding resources.

  @param[in] This     A pointer to the protocol instance.
  @param[in] Mapping  The mapping value returned from Map().

  @retval EFI_SUCCESS           The range was unmapped.
  @retval EFI_INVALID_PARAMETER One or more parameters are invalid.
**/
EFI_STATUS
EFIAPI
MtkUfsUnmap (
  IN  EDKII_UFS_HOST_CONTROLLER_PROTOCOL  *This,
  IN  VOID                                *Mapping
  )
{
  MTK_UFS_MAPPING  *MapInfo;

  if ((This == NULL) || (Mapping == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  MapInfo = (MTK_UFS_MAPPING *)Mapping;

  if (MapInfo->Operation == EdkiiUfsHcOperationBusMasterWrite) {
    //
    // Drop the stale lines again so the consumer reads the device's data.
    //
    InvalidateDataCacheRange (MapInfo->HostAddress, MapInfo->NumberOfBytes);
  }

  FreePool (MapInfo);

  return EFI_SUCCESS;
}

/**
  Flushes all posted write transactions from the UFS bus to the attached
  UFS device.

  @param[in] This  A pointer to the protocol instance.

  @retval EFI_SUCCESS  The posted writes were flushed.
**/
EFI_STATUS
EFIAPI
MtkUfsFlush (
  IN  EDKII_UFS_HOST_CONTROLLER_PROTOCOL  *This
  )
{
  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  MemoryFence ();

  return EFI_SUCCESS;
}

/**
  Read from the UFS host controller MMIO space.

  @param[in]     This    A pointer to the protocol instance.
  @param[in]     Width   Width of the memory operations.
  @param[in]     Offset  Offset within the UFS host controller MMIO space.
  @param[in]     Count   Number of memory operations to perform.
  @param[in,out] Buffer  Destination buffer for the read.

  @retval EFI_SUCCESS           The data was read.
  @retval EFI_INVALID_PARAMETER One or more parameters are invalid.
**/
EFI_STATUS
EFIAPI
MtkUfsMmioRead (
  IN     EDKII_UFS_HOST_CONTROLLER_PROTOCOL        *This,
  IN     EDKII_UFS_HOST_CONTROLLER_PROTOCOL_WIDTH  Width,
  IN     UINT64                                    Offset,
  IN     UINTN                                     Count,
  IN OUT VOID                                      *Buffer
  )
{
  MTK_UFS_HC_PRIVATE_DATA  *Private;
  UINTN                    Address;
  UINTN                    Index;

  if ((This == NULL) || (Buffer == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Private = MTK_UFS_HC_PRIVATE_DATA_FROM_THIS (This);
  Address = Private->UfsHcBase + (UINTN)Offset;

  switch (Width) {
    case EfiUfsHcWidthUint8:
      for (Index = 0; Index < Count; Index++) {
        ((UINT8 *)Buffer)[Index] = MmioRead8 (Address + Index);
      }

      break;
    case EfiUfsHcWidthUint16:
      for (Index = 0; Index < Count; Index++) {
        ((UINT16 *)Buffer)[Index] = MmioRead16 (Address + Index * sizeof (UINT16));
      }

      break;
    case EfiUfsHcWidthUint32:
      for (Index = 0; Index < Count; Index++) {
        ((UINT32 *)Buffer)[Index] = MmioRead32 (Address + Index * sizeof (UINT32));
      }

      break;
    case EfiUfsHcWidthUint64:
      for (Index = 0; Index < Count; Index++) {
        ((UINT64 *)Buffer)[Index] = MmioRead64 (Address + Index * sizeof (UINT64));
      }

      break;
    default:
      return EFI_INVALID_PARAMETER;
  }

  return EFI_SUCCESS;
}

/**
  Write to the UFS host controller MMIO space.

  @param[in]     This    A pointer to the protocol instance.
  @param[in]     Width   Width of the memory operations.
  @param[in]     Offset  Offset within the UFS host controller MMIO space.
  @param[in]     Count   Number of memory operations to perform.
  @param[in,out] Buffer  Source buffer for the write.

  @retval EFI_SUCCESS           The data was written.
  @retval EFI_INVALID_PARAMETER One or more parameters are invalid.
**/
EFI_STATUS
EFIAPI
MtkUfsMmioWrite (
  IN     EDKII_UFS_HOST_CONTROLLER_PROTOCOL        *This,
  IN     EDKII_UFS_HOST_CONTROLLER_PROTOCOL_WIDTH  Width,
  IN     UINT64                                    Offset,
  IN     UINTN                                     Count,
  IN OUT VOID                                      *Buffer
  )
{
  MTK_UFS_HC_PRIVATE_DATA  *Private;
  UINTN                    Address;
  UINTN                    Index;

  if ((This == NULL) || (Buffer == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Private = MTK_UFS_HC_PRIVATE_DATA_FROM_THIS (This);
  Address = Private->UfsHcBase + (UINTN)Offset;

  switch (Width) {
    case EfiUfsHcWidthUint8:
      for (Index = 0; Index < Count; Index++) {
        MmioWrite8 (Address + Index, ((UINT8 *)Buffer)[Index]);
      }

      break;
    case EfiUfsHcWidthUint16:
      for (Index = 0; Index < Count; Index++) {
        MmioWrite16 (Address + Index * sizeof (UINT16), ((UINT16 *)Buffer)[Index]);
      }

      break;
    case EfiUfsHcWidthUint32:
      for (Index = 0; Index < Count; Index++) {
        MmioWrite32 (Address + Index * sizeof (UINT32), ((UINT32 *)Buffer)[Index]);
      }

      break;
    case EfiUfsHcWidthUint64:
      for (Index = 0; Index < Count; Index++) {
        MmioWrite64 (Address + Index * sizeof (UINT64), ((UINT64 *)Buffer)[Index]);
      }

      break;
    default:
      return EFI_INVALID_PARAMETER;
  }

  return EFI_SUCCESS;
}

STATIC MTK_UFS_HC_PRIVATE_DATA  mMtkUfsHcPrivate = {
  MTK_UFS_HC_PRIVATE_SIGNATURE,
  {
    MtkUfsGetMmioBar,
    MtkUfsAllocateBuffer,
    MtkUfsFreeBuffer,
    MtkUfsMap,
    MtkUfsUnmap,
    MtkUfsFlush,
    MtkUfsMmioRead,
    MtkUfsMmioWrite
  },
  0
};

/**
  Entry point of the MediaTek UFS host controller platform driver.

  @param[in] ImageHandle  The firmware allocated handle for the EFI image.
  @param[in] SystemTable  A pointer to the EFI System Table.

  @retval EFI_SUCCESS  The protocol was installed.
  @retval others       The driver failed to initialize.
**/
EFI_STATUS
EFIAPI
MtkUfsDxeInitialize (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  UINT32  Value;

  mMtkUfsHcPrivate.UfsHcBase = FixedPcdGet32 (PcdUfsHostControllerBase);
  if (mMtkUfsHcPrivate.UfsHcBase == 0) {
    DEBUG ((DEBUG_ERROR, "MtkUfsDxe: PcdUfsHostControllerBase is not set\n"));
    return EFI_DEVICE_ERROR;
  }

  //
  // MediaTek early setting, mirroring ufs_mtk_hce_enable_notify(PRE_CHANGE):
  // turn on CLK_CG early to bypass an abnormal ERR_CHK signal and prevent a
  // host controller hang.
  //
  Value = MmioRead32 (mMtkUfsHcPrivate.UfsHcBase + MTK_REG_UFS_XOUFS_CTRL);
  MmioWrite32 (
    mMtkUfsHcPrivate.UfsHcBase + MTK_REG_UFS_XOUFS_CTRL,
    Value | MTK_XOUFS_CTRL_CLK_CG
    );

  DEBUG ((
    DEBUG_INFO,
    "MtkUfsDxe: UFS host controller at 0x%lx (XOUFS_CTRL=0x%x)\n",
    mMtkUfsHcPrivate.UfsHcBase,
    MmioRead32 (mMtkUfsHcPrivate.UfsHcBase + MTK_REG_UFS_XOUFS_CTRL)
    ));

  return gBS->InstallMultipleProtocolInterfaces (
                &ImageHandle,
                &gEdkiiUfsHostControllerProtocolGuid,
                &mMtkUfsHcPrivate.UfsHcProtocol,
                NULL
                );
}
