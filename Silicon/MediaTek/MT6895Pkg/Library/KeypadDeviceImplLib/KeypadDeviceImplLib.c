/*
 * MT6895 (Xiaomi Redmi Note 12T Pro "pearl") keypad implementation.
 *
 * Key sources on this board (verified against the mainline device tree and
 * the vendor hardware):
 *
 *   Volume Down - MediaTek KPD controller (kp@1c00e000), hardware key 0.
 *                 State lives in KP_MEM1 bit 0 (16-bit register). The KPD
 *                 sits in the always-on VLP domain and is left running by
 *                 the LK, so a plain register read is enough.
 *
 *   Power       - MT6363 PMIC PWRKEY debounce, TOPSTATUS (0x1e) bit 1.
 *   Volume Up   - MT6363 PMIC HOMEKEY debounce, TOPSTATUS (0x1e) bit 3.
 *                 Both are read over SPMI (PMIF SWINF channel 2, slave 4,
 *                 register read command). Pressed = bit cleared.
 *
 * The SPMI PMIF/SPMI-master register layout of the MT6895 matches MT8195
 * (soc_chan = 2 -> SWINF channel 2), as verified by the mainline
 * spmi-mtk-pmif driver.
 */

#include <Uefi.h>
#include <Library/BaseMemoryLib.h>
#include <Library/BitmapLib.h>
#include <Library/DebugLib.h>
#include <Library/IoLib.h>
#include <Library/KeypadDeviceImplLib.h>
#include <Library/PcdLib.h>
#include <Library/TimerLib.h>
#include <Library/UefiLib.h>
#include <Protocol/KeypadDevice.h>

//
// MediaTek KPD controller (kp@1c00e000)
//
#define KPD_KP_MEM1        0x04
#define KPD_HW_KEY_VOL_DN  0

//
// MediaTek SPMI PMIF (spmi@1c804000, SWINF channel 2)
//
#define PMIF_SWINF2_ACC      0x0880
#define PMIF_SWINF2_RDATA    0x0894
#define PMIF_SWINF2_VLD_CLR  0x08A4
#define PMIF_SWINF2_STA      0x08A8

#define PMIF_SWINF_FSM(x)    (((x) >> 1) & 0x7)
#define PMIF_SWINF_IDLE      0x0
#define PMIF_SWINF_WFVLDCLR  0x6

#define PMIF_CMD_REG         1
#define PMIF_DELAY_US        2
#define PMIF_TIMEOUT_US      (10 * 1000)

//
// MT6363 PMIC (SPMI slave 4)
//
#define MT6363_SID           4
#define MT6363_TOPSTATUS     0x1e
#define MT6363_PWRKEY_MASK   BIT1
#define MT6363_HOMEKEY_MASK  BIT3

typedef enum {
  KEY_DEVICE_TYPE_UNKNOWN,
  KEY_DEVICE_TYPE_KPD,
  KEY_DEVICE_TYPE_PMIC
} KEY_DEVICE_TYPE;

typedef struct {
  KEY_CONTEXT     EfiKeyContext;
  BOOLEAN         IsValid;
  KEY_DEVICE_TYPE DeviceType;
  UINT32          Index;      // KPD hardware key or PMIC bit mask
  BOOLEAN         ActiveLow;  // PMIC key status bits are active low
} KEY_CONTEXT_PRIVATE;

UINTN gBitmapScanCodes[BITMAP_NUM_WORDS(0x18)]    = {0};
UINTN gBitmapUnicodeChars[BITMAP_NUM_WORDS(0x7f)] = {0};

EFI_KEY_DATA gKeyDataPowerDown = {.Key = {
                                      .UnicodeChar = (CHAR16)'s',
                                  }};

EFI_KEY_DATA gKeyDataPowerUp = {.Key = {
                                    .UnicodeChar = (CHAR16)'e',
                                }};

EFI_KEY_DATA gKeyDataPowerLongpress = {.Key = {
                                           .UnicodeChar = (CHAR16)' ',
                                       }};

#define MS2NS(ms) (((UINT64)(ms)) * 1000000ULL)

STATIC inline VOID
KeySetState(UINT16 ScanCode, CHAR16 UnicodeChar, BOOLEAN Value)
{
  if (ScanCode && ScanCode < 0x18) {
    if (Value)
      BitmapSet(gBitmapScanCodes, ScanCode);
    else
      BitmapClear(gBitmapScanCodes, ScanCode);
  }

  if (UnicodeChar && UnicodeChar < 0x7f) {
    if (Value)
      BitmapSet(gBitmapUnicodeChars, ScanCode);
    else
      BitmapClear(gBitmapUnicodeChars, ScanCode);
  }
}

STATIC inline BOOLEAN KeyGetState(UINT16 ScanCode, CHAR16 UnicodeChar)
{
  if (ScanCode && ScanCode < 0x18) {
    if (!BitmapTest(gBitmapScanCodes, ScanCode))
      return FALSE;
  }

  if (UnicodeChar && UnicodeChar < 0x7f) {
    if (!BitmapTest(gBitmapUnicodeChars, ScanCode))
      return FALSE;
  }

  return TRUE;
}

STATIC inline VOID LibKeyInitializeKeyContext(KEY_CONTEXT *Context)
{
  SetMem(&Context->KeyData, sizeof(Context->KeyData), 0);
  Context->Time      = 0;
  Context->State     = KEYSTATE_RELEASED;
  Context->Repeat    = FALSE;
  Context->Longpress = FALSE;
}

STATIC inline VOID LibKeyUpdateKeyStatus(
    KEY_CONTEXT *Context, KEYPAD_RETURN_API *KeypadReturnApi, BOOLEAN IsPressed,
    UINT64 Delta)
{
  // keep track of the actual state
  KeySetState(
      Context->KeyData.Key.ScanCode, Context->KeyData.Key.UnicodeChar,
      IsPressed);

  // update key time
  Context->Time += Delta;

  switch (Context->State) {
  case KEYSTATE_RELEASED:
    if (IsPressed) {
      // change to pressed
      Context->Time  = 0;
      Context->State = KEYSTATE_PRESSED;
    }
    break;

  case KEYSTATE_PRESSED:
    if (IsPressed) {
      // keyrepeat
      if (Context->Repeat && Context->Time >= MS2NS(200)) {
        KeypadReturnApi->PushEfikeyBufTail(KeypadReturnApi, &Context->KeyData);
        Context->Time   = 0;
        Context->Repeat = TRUE;
      }

      else if (!Context->Longpress && Context->Time >= MS2NS(500)) {
        // POWER, handle key combos
        if (Context->KeyData.Key.UnicodeChar == CHAR_CARRIAGE_RETURN) {
          if (KeyGetState(SCAN_DOWN, 0)) {
            // report 's'
            KeypadReturnApi->PushEfikeyBufTail(
                KeypadReturnApi, &gKeyDataPowerDown);
          }
          else if (KeyGetState(SCAN_UP, 0)) {
            // report 'e'
            KeypadReturnApi->PushEfikeyBufTail(
                KeypadReturnApi, &gKeyDataPowerUp);
          }
          else {
            // report spacebar
            KeypadReturnApi->PushEfikeyBufTail(
                KeypadReturnApi, &gKeyDataPowerLongpress);
          }
        }

        // post first keyrepeat event
        else {
          // only start keyrepeat if we're not doing a combo
          if (!KeyGetState(0, CHAR_CARRIAGE_RETURN)) {
            KeypadReturnApi->PushEfikeyBufTail(
                KeypadReturnApi, &Context->KeyData);
            Context->Time   = 0;
            Context->Repeat = TRUE;
          }
        }

        Context->Longpress = TRUE;
      }
    }

    else {
      if (!Context->Longpress) {
        // we supressed down, so report it now
        KeypadReturnApi->PushEfikeyBufTail(KeypadReturnApi, &Context->KeyData);
        Context->State = KEYSTATE_LONGPRESS_RELEASE;
      }

      else if (Context->Time >= MS2NS(10)) {
        // we reported another key already
        Context->Time      = 0;
        Context->Repeat    = FALSE;
        Context->Longpress = FALSE;
        Context->State     = KEYSTATE_RELEASED;
      }
    }
    break;

  case KEYSTATE_LONGPRESS_RELEASE:
    // change to released
    Context->Time      = 0;
    Context->Repeat    = FALSE;
    Context->Longpress = FALSE;
    Context->State     = KEYSTATE_RELEASED;
    break;

  default:
    ASSERT(FALSE);
    break;
  }
}

STATIC KEY_CONTEXT_PRIVATE KeyContextVolumeDown;
STATIC KEY_CONTEXT_PRIVATE KeyContextVolumeUp;
STATIC KEY_CONTEXT_PRIVATE KeyContextPower;

STATIC KEY_CONTEXT_PRIVATE *KeyList[] = { &KeyContextVolumeDown, &KeyContextVolumeUp, &KeyContextPower };

STATIC
VOID KeypadInitializeKeyContextPrivate(KEY_CONTEXT_PRIVATE *Context)
{
  Context->IsValid     = FALSE;
  Context->Index       = 0;
  Context->DeviceType  = KEY_DEVICE_TYPE_UNKNOWN;
  Context->ActiveLow   = FALSE;
}

STATIC
KEY_CONTEXT_PRIVATE *KeypadKeyCodeToKeyContext(UINT32 KeyCode)
{
  if (KeyCode == 114)
    return &KeyContextVolumeDown;
  else if (KeyCode == 115)
    return &KeyContextVolumeUp;
  else if (KeyCode == 116)
    return &KeyContextPower;
  else
    return NULL;
}

/*
 * Read the MediaTek SPMI PMIF software interface and return one register
 * byte of an SPMI slave (polled, no interrupts).
 */
STATIC
BOOLEAN
SpmiReadByte(
  IN  UINT8 Sid,
  IN  UINT16 Address,
  OUT UINT8 *Value)
{
  UINT32 Status;
  UINT32 Command;
  UINT32 Timeout;

  if (Value == NULL) {
    return FALSE;
  }

  /* Wait for the software interface FSM to be idle. */
  for (Timeout = PMIF_TIMEOUT_US / PMIF_DELAY_US; Timeout > 0; Timeout--) {
    Status = MmioRead32(FixedPcdGet32(PcdPmifBase) + PMIF_SWINF2_STA);
    if (PMIF_SWINF_FSM(Status) == PMIF_SWINF_IDLE) {
      break;
    }

    MicroSecondDelay(PMIF_DELAY_US);
  }

  if (PMIF_SWINF_FSM(Status) != PMIF_SWINF_IDLE) {
    return FALSE;
  }

  /* Register read command (1 byte). */
  Command = ((UINT32)PMIF_CMD_REG << 30) | ((UINT32)Sid << 24) | (UINT32)Address;
  MmioWrite32(FixedPcdGet32(PcdPmifBase) + PMIF_SWINF2_ACC, Command);

  /* Wait for the data valid / clear state. */
  for (Timeout = PMIF_TIMEOUT_US / PMIF_DELAY_US; Timeout > 0; Timeout--) {
    Status = MmioRead32(FixedPcdGet32(PcdPmifBase) + PMIF_SWINF2_STA);
    if (PMIF_SWINF_FSM(Status) == PMIF_SWINF_WFVLDCLR) {
      break;
    }

    MicroSecondDelay(PMIF_DELAY_US);
  }

  if (PMIF_SWINF_FSM(Status) != PMIF_SWINF_WFVLDCLR) {
    return FALSE;
  }

  *Value = (UINT8)MmioRead32(FixedPcdGet32(PcdPmifBase) + PMIF_SWINF2_RDATA);

  /* Clear the valid flag. */
  MmioWrite32(FixedPcdGet32(PcdPmifBase) + PMIF_SWINF2_VLD_CLR, 1);

  return TRUE;
}

/*
 * Read the MT6363 key status register once. Cached for all keys in a poll
 * cycle so a single SPMI transaction serves power and volume up.
 */
STATIC
BOOLEAN
PmicKeyStatus(
  OUT UINT8 *Status)
{
  static UINT64 LastPollNs = 0;
  static UINT8  CachedStatus = 0xFF;

  UINT64 Now = GetTimeInNanoSecond(GetPerformanceCounter());

  if ((Now - LastPollNs) < MS2NS(10)) {
    *Status = CachedStatus;
    return TRUE;
  }

  LastPollNs = Now;

  if (!SpmiReadByte(MT6363_SID, MT6363_TOPSTATUS, &CachedStatus)) {
    return FALSE;
  }

  *Status = CachedStatus;
  return TRUE;
}

STATIC
BOOLEAN
KeyIsPressed(
  IN KEY_CONTEXT_PRIVATE *Context,
  IN UINT8               PmicStatus)
{
  BOOLEAN State = FALSE;

  if (Context->DeviceType == KEY_DEVICE_TYPE_KPD) {
    UINT16 KpdState = MmioRead16(
        FixedPcdGet32(PcdKeypadBase) + KPD_KP_MEM1 +
        (Context->Index / 16) * 4);
    State = (KpdState & (1U << (Context->Index % 16))) != 0;
  } else if (Context->DeviceType == KEY_DEVICE_TYPE_PMIC) {
    State = (PmicStatus & (UINT8)Context->Index) == 0;
  }

  return State;
}

RETURN_STATUS
EFIAPI
KeypadDeviceImplConstructor(VOID)
{
  UINTN                Index;
  KEY_CONTEXT_PRIVATE *StaticContext;

  // Reset all keys
  for (Index = 0; Index < (sizeof(KeyList) / sizeof(KeyList[0])); Index++) {
    KeypadInitializeKeyContextPrivate(KeyList[Index]);
  }

  // Volume Down: MediaTek KPD, hardware key 0
  StaticContext              = KeypadKeyCodeToKeyContext(114);
  StaticContext->DeviceType  = KEY_DEVICE_TYPE_KPD;
  StaticContext->Index       = KPD_HW_KEY_VOL_DN;
  StaticContext->ActiveLow   = FALSE;
  StaticContext->IsValid     = TRUE;

  // Volume Up: MT6363 HOMEKEY (active low)
  StaticContext              = KeypadKeyCodeToKeyContext(115);
  StaticContext->DeviceType  = KEY_DEVICE_TYPE_PMIC;
  StaticContext->Index       = MT6363_HOMEKEY_MASK;
  StaticContext->ActiveLow   = TRUE;
  StaticContext->IsValid     = TRUE;

  // Power: MT6363 PWRKEY (active low)
  StaticContext              = KeypadKeyCodeToKeyContext(116);
  StaticContext->DeviceType  = KEY_DEVICE_TYPE_PMIC;
  StaticContext->Index       = MT6363_PWRKEY_MASK;
  StaticContext->ActiveLow   = TRUE;
  StaticContext->IsValid     = TRUE;

  DEBUG((DEBUG_INFO, "KeypadDeviceImplLib: MT6895 keys ready (KPD vol-down, PMIC power/vol-up)\n"));

  return RETURN_SUCCESS;
}

EFI_STATUS EFIAPI KeypadDeviceImplReset(KEYPAD_DEVICE_PROTOCOL *This)
{
  LibKeyInitializeKeyContext(&KeyContextVolumeDown.EfiKeyContext);
  KeyContextVolumeDown.EfiKeyContext.KeyData.Key.ScanCode = SCAN_DOWN;

  LibKeyInitializeKeyContext(&KeyContextVolumeUp.EfiKeyContext);
  KeyContextVolumeUp.EfiKeyContext.KeyData.Key.ScanCode = SCAN_UP;

  LibKeyInitializeKeyContext(&KeyContextPower.EfiKeyContext);
  KeyContextPower.EfiKeyContext.KeyData.Key.UnicodeChar = CHAR_CARRIAGE_RETURN;

  return EFI_SUCCESS;
}

EFI_STATUS KeypadDeviceImplGetKeys(
    KEYPAD_DEVICE_PROTOCOL *This, KEYPAD_RETURN_API *KeypadReturnApi,
    UINT64 Delta)
{
    UINT8   PmicStatus = 0xFF;
    UINTN   Index;

    PmicKeyStatus(&PmicStatus);

    for (Index = 0; Index < (sizeof(KeyList) / sizeof(KeyList[0])); Index++) {
        KEY_CONTEXT_PRIVATE *Context = KeyList[Index];

        // check if this is a valid key
        if (Context->IsValid == FALSE)
            continue;

        LibKeyUpdateKeyStatus(
            &Context->EfiKeyContext, KeypadReturnApi,
            KeyIsPressed(Context, PmicStatus), Delta);
    }

    return EFI_SUCCESS;
}
