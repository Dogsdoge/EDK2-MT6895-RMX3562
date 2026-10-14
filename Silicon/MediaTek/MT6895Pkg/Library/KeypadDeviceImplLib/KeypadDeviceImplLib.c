/*
 * MT6895 keypad stub.
 *
 * Key support for the Xiaomi pearl is not wired up yet: the volume keys are
 * expected on GPIO pins that are not mapped, and the power key is handled by
 * the SPMI PMIC (MT6363), which is not implemented. Report no keys so the
 * keypad drivers load cleanly and the console keeps working.
 */

#include <Uefi.h>
#include <Library/DebugLib.h>
#include <Library/KeypadDeviceImplLib.h>

RETURN_STATUS
EFIAPI
KeypadDeviceImplConstructor(VOID)
{
  DEBUG((DEBUG_INFO, "KeypadDeviceImplLib: MT6895 keypad not implemented, no keys available\n"));
  return RETURN_SUCCESS;
}

EFI_STATUS
EFIAPI
KeypadDeviceImplReset(KEYPAD_DEVICE_PROTOCOL *This)
{
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
KeypadDeviceImplGetKeys(
    KEYPAD_DEVICE_PROTOCOL *This, KEYPAD_RETURN_API *KeypadReturnApi,
    UINT64 Delta)
{
  return EFI_SUCCESS;
}
