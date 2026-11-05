/** @file
  cmi bring-up menu.

  Draws a visible UniXNU-style menu straight onto the bootloader-inherited
  framebuffer via GOP Blt, and moves the highlight with any SimpleTextIn key.
  This intentionally does NOT depend on BDS launching UniXNU.efi: on this
  board BDS uses the null DeviceBootManagerLib, so the firmware boot option
  never runs. The menu here is a bring-up indicator; the real UniXNU menu is
  UniXNULib/Ui.c (it needs the same GOP, so once this works the BDS path can
  be fixed separately).

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/
#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Protocol/GraphicsOutput.h>
#include <Protocol/SimpleTextIn.h>

#define MENU_ITEMS  3

STATIC CONST CHAR16 *mItems[MENU_ITEMS] = {
  L"Mi 10 Pro  (UniXNU)",
  L"Restart",
  L"Power off"
};

STATIC
VOID
FillRect (
  IN EFI_GRAPHICS_OUTPUT_PROTOCOL *Gop,
  IN UINTN X,
  IN UINTN Y,
  IN UINTN W,
  IN UINTN H,
  IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL *Color
  )
{
  Gop->Blt (Gop, (EFI_GRAPHICS_OUTPUT_BLT_PIXEL *)Color, EfiBltVideoFill,
            0, 0, X, Y, W, H, 0);
}

STATIC
VOID
DrawMenu (
  IN EFI_GRAPHICS_OUTPUT_PROTOCOL *Gop,
  IN UINTN Selected
  )
{
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL Bg   = { 0x00, 0x00, 0x00, 0x00 };
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL Hi   = { 0x30, 0x90, 0xFF, 0x00 };
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL Bar  = { 0xFF, 0xFF, 0xFF, 0x00 };
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL Line = { 0x40, 0x40, 0x40, 0x00 };
  UINTN Width  = Gop->Mode->Info->HorizontalResolution;
  UINTN Height = Gop->Mode->Info->VerticalResolution;
  UINTN I;
  UINTN RowH   = Height / 16;
  UINTN Top    = Height / 4;
  UINTN Left   = Width / 8;
  UINTN RowW   = Width * 3 / 4;

  // Clean background (this also proves Blt works).
  FillRect (Gop, 0, 0, Width, Height, &Bg);
  // Title bar.
  FillRect (Gop, Left, Top - RowH, RowW, 6, &Bar);

  for (I = 0; I < MENU_ITEMS; I++) {
    UINTN Y = Top + I * RowH;
    if (I == Selected) {
      FillRect (Gop, Left - 16, Y - 6, RowW + 32, RowH - 8, &Hi);
    } else {
      FillRect (Gop, Left - 16, Y - 6, RowW + 32, RowH - 8, &Line);
    }
    // Recolor the inner part so the text area stays dark on the highlight row.
    FillRect (Gop, Left - 8, Y, RowW + 16, RowH - 20, &Bg);
  }

  // Titles go to the DEBUG console (already mirrored onto the framebuffer by
  // RamoopsSerialPortLib). ConOut/GraphicsConsole binding is not guaranteed on
  // this board, so the menu does not depend on it.
  DEBUG ((DEBUG_ERROR, "CMI MENU:  UniXNU - Mi 10 Pro (cmi)\n"));
  for (I = 0; I < MENU_ITEMS; I++) {
    DEBUG ((DEBUG_ERROR, "CMI MENU: %s %s\n",
            (I == Selected) ? L" >" : L"  ", mItems[I]));
  }
  DEBUG ((DEBUG_ERROR, "CMI MENU: drawn, selected=%u\n", (UINT32)Selected));
}

EFI_STATUS
EFIAPI
CmiMenuEntry (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_GRAPHICS_OUTPUT_PROTOCOL   *Gop;
  EFI_SIMPLE_TEXT_INPUT_PROTOCOL *ConIn;
  EFI_STATUS                     Status;
  EFI_INPUT_KEY                  Key;
  UINTN                          Selected = 0;

  Status = gBS->LocateProtocol (&gEfiGraphicsOutputProtocolGuid, NULL, (VOID **)&Gop);
  DEBUG ((DEBUG_ERROR, "CMI MENU: GOP locate = %r\n", Status));
  if (EFI_ERROR (Status)) {
    return Status;
  }
  DEBUG ((DEBUG_ERROR, "CMI MENU: GOP %ux%u pitch %u\n",
          Gop->Mode->Info->HorizontalResolution,
          Gop->Mode->Info->VerticalResolution,
          Gop->Mode->Info->PixelsPerScanLine));

  ConIn = NULL;
  Status = gBS->LocateProtocol (&gEfiSimpleTextInProtocolGuid, NULL, (VOID **)&ConIn);
  DEBUG ((DEBUG_ERROR, "CMI MENU: ConIn locate = %r\n", Status));

  DrawMenu (Gop, Selected);

  // Simple polling loop. Key strokes come from UmiButtonsDxe (volume/power).
  for ( ; ;) {
    if (ConIn != NULL && !EFI_ERROR (ConIn->ReadKeyStroke (ConIn, &Key))) {
      DEBUG ((DEBUG_ERROR, "CMI MENU: key scancode=%u unicode=%u\n",
              Key.ScanCode, Key.UnicodeChar));
      if (Key.ScanCode == 0x01 || Key.UnicodeChar == 0x0D) {   // SCAN_UP / CR
        Selected = (Selected + MENU_ITEMS - 1) % MENU_ITEMS;
      } else {
        Selected = (Selected + 1) % MENU_ITEMS;
      }
      DrawMenu (Gop, Selected);
    } else {
      gBS->Stall (100000);   // 100 ms
    }
  }

  return EFI_SUCCESS;
}
