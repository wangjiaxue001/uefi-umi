# Inherited display experiment: source drivers only.
APRIORI DXE {
  INF MdeModulePkg/Core/Dxe/DxeMain.inf
  INF MdeModulePkg/Universal/PCD/Dxe/Pcd.inf
  INF MdeModulePkg/Universal/ReportStatusCodeRouter/RuntimeDxe/ReportStatusCodeRouterRuntimeDxe.inf
  INF MdeModulePkg/Universal/StatusCodeHandler/RuntimeDxe/StatusCodeHandlerRuntimeDxe.inf
  INF MdeModulePkg/Core/RuntimeDxe/RuntimeDxe.inf
  INF ArmPkg/Drivers/CpuDxe/CpuDxe.inf
  INF ArmPkg/Drivers/ArmGicDxe/ArmGicDxe.inf
  INF EmbeddedPkg/MetronomeDxe/MetronomeDxe.inf
  INF ArmPkg/Drivers/TimerDxe/TimerDxe.inf
  INF MdeModulePkg/Universal/ResetSystemRuntimeDxe/ResetSystemRuntimeDxe.inf
  INF MdeModulePkg/Universal/Variable/RuntimeDxe/VariableRuntimeDxe.inf
  INF MdeModulePkg/Universal/WatchdogTimerDxe/WatchdogTimer.inf
  INF EmbeddedPkg/EmbeddedMonotonicCounter/EmbeddedMonotonicCounter.inf
  INF EmbeddedPkg/RealTimeClockRuntimeDxe/RealTimeClockRuntimeDxe.inf
  INF MdeModulePkg/Universal/PrintDxe/PrintDxe.inf
  INF MdeModulePkg/Universal/DevicePathDxe/DevicePathDxe.inf
  INF MdeModulePkg/Universal/CapsuleRuntimeDxe/CapsuleRuntimeDxe.inf
  INF MdeModulePkg/Universal/HiiDatabaseDxe/HiiDatabaseDxe.inf
  INF MdeModulePkg/Universal/Disk/UnicodeCollation/EnglishDxe/EnglishDxe.inf
  INF MdeModulePkg/Universal/Disk/DiskIoDxe/DiskIoDxe.inf
  INF FatPkg/EnhancedFatDxe/Fat.inf
  INF umiPkg/Drivers/UmiRamPayloadDxe/UmiRamPayloadDxe.inf
  # GOP and input must exist BEFORE BdsDxe entry: BDS registers ConIn/ConOut
  # variables and connects the consoles in BeforeConsole. Dispatch order below
  # follows the observed DXE execution evidence (screen DEBUG log):
  # InheritedFbDxe installs GOP onto DisplayDevicePath; UmiButtonsDxe provides
  # SimpleTextIn on KeypadDevicePath; GraphicsConsoleDxe then binds both.
  INF umiPkg/Drivers/InheritedFbDxe/InheritedFbDxe.inf
  INF umiPkg/Drivers/UmiButtonsDxe/UmiButtonsDxe.inf
  INF MdeModulePkg/Universal/Console/ConPlatformDxe/ConPlatformDxe.inf
  INF MdeModulePkg/Universal/Console/ConSplitterDxe/ConSplitterDxe.inf
  INF MdeModulePkg/Universal/Console/GraphicsConsoleDxe/GraphicsConsoleDxe.inf
  INF DarwinBootPkg/Driver/DarwinBoot.inf
  # Writes Boot####/BootOrder with UniXNU.efi first; must run before BDS boots.
  INF UniXNUPkg/Driver/UniXNUDxe/UniXNUDxe.inf
  # BDS last: connects consoles, counts down (PcdPlatformBootTimeOut=3),
  # then boots BootOrder -> UniXNU.efi -> UniXNURun() draws the official menu.
  INF MdeModulePkg/Universal/BdsDxe/BdsDxe.inf
}
