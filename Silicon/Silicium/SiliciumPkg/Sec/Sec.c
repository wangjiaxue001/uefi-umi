#include "UmiSmmuTrace.h"
/**
  Copyright (c) 2011-2017, ARM Limited. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/PcdLib.h>
#include <Library/DebugLib.h>
#include <Library/PrePiLib.h>
#include <Library/DebugAgentLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/SerialPortLib.h>
#include <Library/PlatformSecLib.h>
#include <Library/PrePiHobListPointerLib.h>
#include <Library/CacheMaintenanceLib.h>
#include <Library/MemoryMapHelperLib.h>
#include <Library/TimerLib.h>
#include <Library/HobLib.h>

#include "Sec.h"

// Bring-up progress marks (cmi port, 2026-11-05). The inherited framebuffer at
// 0x9C000000 is already being scanned out (ABL's Mi logo), and ClearFrameBuffer
// proved it is writable from SEC. Draw one white 48x48 block per milestone so
// a hang can be located by counting blocks on screen. No font, no DXE, no GOP.
#define MARK_FB      0x9C000000ULL
#define MARK_STRIDE  1080U
#define MARK_W       48U
#define MARK_H       48U

STATIC
VOID
ProgressMark (
  UINT32 Step
  )
{
  volatile UINT32 *Fb = (volatile UINT32 *)(UINTN)MARK_FB;
  UINT32 S;
  UINTN X, Y;
  UINTN X0;

  // Redraw all marks: ClearFrameBuffer() wipes them, so later calls restore.
  for (S = 0; S <= Step; S++) {
    X0 = (UINTN)S * (MARK_W + 8U);
    for (Y = 0; Y < MARK_H; Y++) {
      for (X = 0; X < MARK_W; X++) {
        Fb[Y * MARK_STRIDE + X0 + X] = 0xFFFFFFFFU;
      }
    }
  }
}

// First argument (x0) of the previous stage, saved by ModuleEntryPoint.S
extern UINT64  gSecEntryArg0;

#ifndef MDEPKG_NDEBUG
#ifndef UMI_SKIP_EARLY_FRAMEBUFFER
STATIC
VOID
ClearFrameBuffer (VOID)
{
  EFI_MEMORY_REGION_DESCRIPTOR FrameBufferRegion;

  // Locate "Display Reserved" Memory Region
  LocateMemoryRegionByName ("Display Reserved", &FrameBufferRegion);
  LocateMemoryRegionByName ("Display_Reserved", &FrameBufferRegion);

  // Clear Memory Region
  if (FrameBufferRegion.Address != 0 && FrameBufferRegion.Length != 0) {
    ZeroMem ((VOID *)FrameBufferRegion.Address, FrameBufferRegion.Length);
  }
}

#endif // UMI_SKIP_EARLY_FRAMEBUFFER

STATIC
VOID
PrintFirmwareVersion (VOID)
{
  // umi has no qualified framebuffer until real GOP is installed.
#ifndef UMI_SKIP_EARLY_FRAMEBUFFER
  ClearFrameBuffer ();
#endif

  // Print UEFI Version Message
  DEBUG ((EFI_D_WARN, "\n"));
  DEBUG ((EFI_D_WARN, "Project Silicium for %a %a\n",            FixedPcdGetPtr (PcdSmbiosSystemManufacturer), FixedPcdGetPtr (PcdSmbiosSystemModel)));
  DEBUG ((EFI_D_WARN, "Firmware Version %s Build at %a on %a\n", PcdGetPtr (PcdFirmwareVersionString), __TIME__, __DATE__));
  DEBUG ((EFI_D_WARN, "\n"));
}
#endif

STATIC
EFI_STATUS
InitializeMemory (VOID)
{
  EFI_STATUS                    Status;
  EFI_MEMORY_REGION_DESCRIPTOR  UefiMemoryRegion;
  EFI_HOB_HANDOFF_INFO_TABLE   *HobList;

  // Locate "DXE Heap" Memory Region
  LocateMemoryRegionByName ("DXE Heap", &UefiMemoryRegion);
  LocateMemoryRegionByName ("DXE_Heap", &UefiMemoryRegion);

  // Save UEFI Memory Details
  UINTN UefiMemoryBase = UefiMemoryRegion.Address;
  UINTN UefiMemorySize = UefiMemoryRegion.Length;

  // Verify UEFI Memory Base
  if (UefiMemoryBase == 0 && UefiMemorySize == 0) {
    DEBUG ((EFI_D_ERROR, "Failed to Locate \"DXE Heap\" Memory Region!\n"));
    return EFI_NOT_FOUND;
  }

  // Initialize HOB List
  HobList = HobConstructor ((VOID *)UefiMemoryBase, UefiMemorySize, (VOID *)UefiMemoryBase, (VOID *)(UefiMemoryBase + UefiMemorySize));
  PrePeiSetHobList (HobList);

  // Initialize MMU & Memory HOBs
  Status = MemoryPeim (UefiMemoryBase, UefiMemorySize);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  // Build Stack HOB
  BuildStackHob (FixedPcdGet64 (PcdCPUCoresStackBase), FixedPcdGet32 (PcdCPUCorePrimaryStackSize));

  return EFI_SUCCESS;
}

STATIC
VOID
SecMain (VOID)
{
  EFI_STATUS Status;

  SerialPortInitialize ();
  DEBUG ((EFI_D_ERROR, "UMI: SEC start\n"));
  ProgressMark (0);
  // Do Platform Specific Initialization
  UmiSmmuSnapshot ("before-platform");
  PlatformInitialize ();
  UmiSmmuSnapshot ("after-platform");
  ProgressMark (1);
  DEBUG ((EFI_D_ERROR, "UMI: platform initialized\n"));

#ifndef MDEPKG_NDEBUG
  // Initialize Serial Port
  Status = SerialPortInitialize ();
  if (EFI_ERROR (Status)) {
    return;
  }

  // Print Firmware Version
  PrintFirmwareVersion ();
  ProgressMark (2);

  // Initialize Debug Agent
  InitializeDebugAgent (DEBUG_AGENT_INIT_POSTMEM_SEC, NULL, NULL);
  SaveAndSetDebugTimerInterrupt (TRUE);
  ProgressMark (3);
#endif

  DEBUG ((EFI_D_ERROR, "UMI: memory init start\n"));
  // Initialize Memory
  Status = InitializeMemory ();
  ProgressMark (4);
  DEBUG ((EFI_D_ERROR, "UMI: memory init returned %r\n", Status));
  if (EFI_ERROR (Status)) {
    return;
  }

  // Build CPU HOB
  BuildCpuHob (ArmGetPhysicalAddressBits (), PcdGet8 (PcdPrePiCpuIoSize));

  // Set the Boot Mode
  SetBootMode (BOOT_WITH_DEFAULT_SETTINGS);

  DEBUG ((EFI_D_ERROR, "UMI: platform PEI start\n"));
  // Initialize Platform HOBs
  Status = PlatformPeim ();
  ProgressMark (5);
  DEBUG ((EFI_D_ERROR, "UMI: platform PEI returned %r\n", Status));
  if (EFI_ERROR (Status)) {
    return;
  }

  // Pass on what the previous stage handed over in x0 (a DT address for a
  // Linux-style boot loader), so a DXE driver can use it.
  {
    UINT64  EntryArg = gSecEntryArg0;

    BuildGuidDataHob (&gSiliciumEntryArgHobGuid, &EntryArg, sizeof (EntryArg));
  }

  // Run Library Cconstructors
  ProcessLibraryConstructorList ();

  DEBUG ((EFI_D_ERROR, "UMI: FV decompress start\n"));
  // Decompress all FVs
  Status = DecompressFirstFv ();
  ProgressMark (6);
  DEBUG ((EFI_D_ERROR, "UMI: FV decompress returned %r\n", Status));
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "Failed to Decompress FV! Status = %r\n", Status));
    return;
  }

  DEBUG ((EFI_D_ERROR, "UMI: DXE core load start\n"));
  // Load DXE Core
  Status = LoadDxeCoreFromFv (NULL, 0);
  ProgressMark (7);
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "Failed to Load DXE Core! Status = %r\n", Status));
  }
}

VOID
SecEntry (VOID)
{
  // Disable Data Cache
  ArmDisableDataCache ();

  // Invalidate Instruction Cache
  ArmInvalidateInstructionCache ();

  // Enable Instruction Cache
  ArmEnableInstructionCache ();

  // Invalidate Stack & UEFI FD D-Cache
  InvalidateDataCacheRange ((VOID *)FixedPcdGet64 (PcdCPUCoresStackBase), FixedPcdGet32 (PcdCPUCorePrimaryStackSize));
  InvalidateDataCacheRange ((VOID *)FixedPcdGet64 (PcdFdBaseAddress),     FixedPcdGet32 (PcdFdSize));

  // Verify Exception Vector Table
  ASSERT (((UINTN)SecVectorTable & ARM_VECTOR_TABLE_ALIGNMENT) == 0);

  // Enable new Exception Vector Table
  ArmWriteVBar ((UINTN)SecVectorTable);

  // Enter Main SEC Function
  SecMain ();
}
