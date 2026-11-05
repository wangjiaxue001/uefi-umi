/** @file
  cmi bring-up menu.

  Paints a visible UniXNU-style menu straight into the bootloader-inherited
  framebuffer at 0x9C000000 (1080x2340, BGRX32). This is the same physical
  window that SEC's ClearFrameBuffer and the screen DEBUG console already use
  successfully on this board, so it does not depend on GOP being installed.

  The entry point MUST NOT block the DXE dispatcher: it draws once, registers a
  periodic timer event that polls SimpleTextIn (volume/power keys from
  UmiButtonsDxe), and returns. The event keeps the menu alive after BDS.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/
#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Protocol/SimpleTextIn.h>

#define FB_BASE   0x9C000000ULL
#define FB_W      1080U
#define FB_H      2340U
#define STRIDE    FB_W
#define ROW_H     (FB_H / 16)
#define TOP       (FB_H / 4)
#define LEFT      (FB_W / 8)
#define ROWW      (FB_W * 3 / 4)
#define ITEMS     3

STATIC CONST CHAR16 *mItems[ITEMS] = {
  L"Mi 10 Pro  (UniXNU)",
  L"Restart",
  L"Power off"
};

STATIC volatile UINT32 *mFb = (volatile UINT32 *)(UINTN)FB_BASE;
STATIC UINTN            mSelected = 0;
STATIC EFI_EVENT        mPollEvent = NULL;

STATIC
VOID
FillRect (
  IN UINTN X,
  IN UINTN Y,
  IN UINTN W,
  IN UINTN H,
  IN UINT32 Color
  )
{
  UINTN Row, Col;
  for (Row = 0; Row < H; Row++) {
    for (Col = 0; Col < W; Col++) {
      mFb[(Y + Row) * STRIDE + (X + Col)] = Color;
    }
  }
}

STATIC
VOID
DrawMenu (
  VOID
  )
{
  UINTN I;

  FillRect (0, 0, FB_W, FB_H, 0x00000000U);            // black
  FillRect (LEFT, TOP - ROW_H, ROWW, 6, 0x00FFFFFFU);  // title bar (white)

  for (I = 0; I < ITEMS; I++) {
    UINTN Y = TOP + I * ROW_H;
    UINT32 Frame = (I == mSelected) ? 0x00FF9030U : 0x00404040U;
    FillRect (LEFT - 16, Y - 6, ROWW + 32, ROW_H - 8, Frame);
    FillRect (LEFT - 8, Y, ROWW + 16, ROW_H - 20, 0x00000000U);
  }

  DEBUG ((DEBUG_ERROR, "CMI MENU: UniXNU - Mi 10 Pro (cmi)\n"));
  for (I = 0; I < ITEMS; I++) {
    DEBUG ((DEBUG_ERROR, "CMI MENU: %s %s\n",
            (I == mSelected) ? L" >" : L"  ", mItems[I]));
  }
  DEBUG ((DEBUG_ERROR, "CMI MENU: drawn, selected=%u\n", (UINT32)mSelected));
}

STATIC
VOID
EFIAPI
PollMenu (
  IN EFI_EVENT Event,
  IN VOID     *Context
  )
{
  EFI_SIMPLE_TEXT_INPUT_PROTOCOL *ConIn;
  EFI_INPUT_KEY                  Key;
  EFI_STATUS                     Status;

  Status = gBS->LocateProtocol (&gEfiSimpleTextInProtocolGuid, NULL, (VOID **)&ConIn);
  if (EFI_ERROR (Status)) {
    return;   // UmiButtonsDxe not dispatched yet
  }
  if (EFI_ERROR (ConIn->ReadKeyStroke (ConIn, &Key))) {
    return;   // no key
  }
  DEBUG ((DEBUG_ERROR, "CMI MENU: key scan=%u char=%u\n",
          Key.ScanCode, Key.UnicodeChar));
  if (Key.ScanCode == 0x01 || Key.UnicodeChar == 0x0D) {
    mSelected = (mSelected + ITEMS - 1) % ITEMS;
  } else {
    mSelected = (mSelected + 1) % ITEMS;
  }
  DrawMenu ();
}

EFI_STATUS
EFIAPI
CmiMenuEntry (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS Status;

  DEBUG ((DEBUG_ERROR, "CMI MENU: driver entry\n"));
  DrawMenu ();

  Status = gBS->CreateEvent (EVT_TIMER | EVT_NOTIFY_SIGNAL, TPL_CALLBACK,
                             PollMenu, NULL, &mPollEvent);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "CMI MENU: CreateEvent = %r\n", Status));
    return Status;
  }
  Status = gBS->SetTimer (mPollEvent, TimerPeriodic, 100000);  // 10 ms
  DEBUG ((DEBUG_ERROR, "CMI MENU: poll timer = %r\n", Status));
  return EFI_SUCCESS;
}
