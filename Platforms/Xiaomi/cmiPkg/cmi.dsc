## @file
# Xiaomi Mi 10 Pro (cmi), SM8250: UEFI 菜单 bring-up。
# 从 uefi-umi/Platforms/Xiaomi/umiPkg 派生；SoC 层复用 KonaPkg/KonaPkg.dsc.inc，
# 板级差异只体现在 PCD 与（待替换的）按键实现。
# SPDX-License-Identifier: BSD-2-Clause-Patent
##
[Defines]
  PLATFORM_NAME                  = cmi
  PLATFORM_GUID                  = E55D11A0-A430-4F6B-B9E8-BCAE1F4A54C7
  PLATFORM_VERSION               = 0.1
  DSC_SPECIFICATION              = 0x00010005
  OUTPUT_DIRECTORY               = Build/cmiPkg
  SUPPORTED_ARCHITECTURES        = AARCH64
  BUILD_TARGETS                  = DEBUG
  SKUID_IDENTIFIER               = DEFAULT
  FLASH_DEFINITION               = cmiPkg/cmi.fdf
  USE_CUSTOM_DISPLAY_DRIVER      = 1
  SOC_TYPE                       = 0

!include KonaPkg/KonaPkg.dsc.inc

[BuildOptions]
  # 排障实验（2026-11-05）：暂时放开 SEC 早期画屏（往 Display Reserved 0x9C000000
  # 直接写像素，不依赖 DXE/GOP）。若屏幕从"卡 Mi logo"变成有图案/文字，说明 SEC
  # 存活、死点在 DXE；若仍是 Mi logo，死点在 SEC 或 shim 跳转本身。
  # （原 umi 的 opt-out 注释与宏名 UMI_ 前缀保留，因为消费方是 SiliciumPkg/Sec/Sec.c）
  *_CLANGPDB_AARCH64_CC_FLAGS = -D UNIXNU_BRINGUP_MENU=1

[PcdsFixedAtBuild]
  # 实测：memory base 0x80000000（三 bank 求和 0x1fbb00000，见设备仓库 inputs.md）
  gArmTokenSpaceGuid.PcdSystemMemoryBase|0x80000000
  gArmPlatformTokenSpaceGuid.PcdCPUCoresStackBase|0x9FF90000
  gArmPlatformTokenSpaceGuid.PcdCPUCorePrimaryStackSize|0x40000

  gSiliciumPkgTokenSpaceGuid.PcdSmbiosSystemManufacturer|"Xiaomi"
  gSiliciumPkgTokenSpaceGuid.PcdSmbiosSystemModel|"Mi 10 Pro"
  gSiliciumPkgTokenSpaceGuid.PcdSmbiosSystemRetailModel|"cmi"
  gSiliciumPkgTokenSpaceGuid.PcdSmbiosSystemRetailSku|"cmi-research"
  gSiliciumPkgTokenSpaceGuid.PcdSmbiosSystemBoardModel|"cmi"
  # 实测：1080x2340，24 bpp（面板 qcom,mdss_dsi_j2_p2_1_38_0c_0a_dsc_cmd）
  gSiliciumPkgTokenSpaceGuid.PcdFrameBufferWidth|1080
  gSiliciumPkgTokenSpaceGuid.PcdFrameBufferHeight|2340
  gSiliciumPkgTokenSpaceGuid.PcdFrameBufferColorDepth|32
  gQcomPkgTokenSpaceGuid.PcdPlatformType|"LA"

  gEfiMdeModulePkgTokenSpaceGuid.PcdEmuVariableNvModeEnable|TRUE
  gEfiMdeModulePkgTokenSpaceGuid.PcdResetOnMemoryTypeInformationChange|FALSE
  gEfiMdeModulePkgTokenSpaceGuid.PcdCapsuleInRamSupport|FALSE
  gUniXNUPkgTokenSpaceGuid.PcdUniXNUDebugLevel|0x80000000

[PcdsFeatureFlag]
  gEfiMdeModulePkgTokenSpaceGuid.PcdSupportUpdateCapsuleReset|FALSE
  gEfiMdeModulePkgTokenSpaceGuid.PcdSupportProcessCapsuleAtRuntime|FALSE

# 以下库与驱动复用 umiPkg 的实现：它们属于 SM8250 平台或与板级无关。
# 唯一需要按 cmi 改的是按键映射（见 [Components] 里 UmiButtonsDxe 的注释）。
[LibraryClasses]
  MemoryMapLib|umiPkg/Library/MemoryMapLib/MemoryMapLib.inf
  ConfigurationMapLib|umiPkg/Library/ConfigurationMapLib/ConfigurationMapLib.inf
  SerialPortLib|umiPkg/Library/RamoopsSerialPortLib/RamoopsSerialPortLib.inf
  DeviceBootManagerLib|SiliciumNullPkg/Library/PlatformBootManagerLibNull/PlatformBootManagerLibNull.inf
  PlatformBootOptionsLib|SiliciumNullPkg/Library/PlatformBootOptionsLibNull/PlatformBootOptionsLibNull.inf
  SecureBootHandlerLib|umiPkg/Library/NullSecureBootHandlerLib/NullSecureBootHandlerLib.inf
  AcpiTableUpdateLib|SiliciumNullPkg/Library/AcpiTableUpdateLibNull/AcpiTableUpdateLibNull.inf
  RamManagerLib|SiliciumNullPkg/Library/RamManagerLibNull/RamManagerLibNull.inf
  UniXNULib|UniXNUPkg/Library/UniXNULib/UniXNULib.inf
  FdtLib|MdePkg/Library/BaseFdtLib/BaseFdtLib.inf
  CapsuleLib|MdeModulePkg/Library/DxeCapsuleLibNull/DxeCapsuleLibNull.inf

[Components]
  UniXNUPkg/Application/UniXNU/UniXNU.inf
  UniXNUPkg/Driver/UniXNUDxe/UniXNUDxe.inf

[Components]
  umiPkg/Drivers/InheritedFbDxe/InheritedFbDxe.inf

[LibraryClasses]
  ResetSystemLib|umiPkg/Library/UmiResetSystemLib/UmiResetSystemLib.inf
  HwResetSystemLib|umiPkg/Library/UmiResetSystemLib/UmiResetSystemLib.inf
  UmiSpmiLib|umiPkg/Library/UmiSpmiLib/UmiSpmiLib.inf
  UmiRpmhLib|umiPkg/Library/UmiRpmhLib/UmiRpmhLib.inf

[Components]
  # 按键：已核对，cmi 与 umi 的来源一致，直接复用，无需改代码。
  #   设备 DT：vol_up { gpios = <&pm8150_gpio 0x6 0x1> }（PMIC GPIO6，低有效）
  #            hall_key { gpios = <&tlmm 0xe 0x1> }（tlmm，不在驱动范围内）
  #            pon_1 = KPDPWR → linux,code 0x74（电源）
  #            pon_2 = RESIN  → linux,code 0x72（音量下）
  #   UmiButtons 读 PON PPID 0x008（bit0 电源 / bit1 音量下）与 PMIC GPIO PPID 0x0C5
  #   （bit0 低有效 → 音量上），与上面的 DT 完全对应。
  umiPkg/Drivers/UmiButtonsDxe/UmiButtonsDxe.inf

[Components]
  MdeModulePkg/Universal/Disk/UnicodeCollation/EnglishDxe/EnglishDxe.inf
  MdeModulePkg/Universal/Disk/DiskIoDxe/DiskIoDxe.inf
  FatPkg/EnhancedFatDxe/Fat.inf
  umiPkg/Drivers/UmiRamPayloadDxe/UmiRamPayloadDxe.inf
  umiPkg/Application/UmiPayloadCheck/UmiPayloadCheck.inf
  umiPkg/Application/UmiPayload64Check/UmiPayload64Check.inf
  umiPkg/Application/UmiMemoryDiag/UmiMemoryDiag.inf

[Components]
  DarwinBootPkg/Driver/DarwinBoot.inf
