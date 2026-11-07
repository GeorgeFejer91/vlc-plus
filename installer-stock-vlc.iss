#define ProductName "VLC+"
#define ProductVersion "0.3.4"

[Setup]
AppId={{F836C1D9-43B0-4A71-95D7-2854030B68AE}
AppName={#ProductName}
AppVersion={#ProductVersion}
AppPublisher=George Fejer
DefaultDirName={localappdata}\Programs\VLCPlus
DefaultGroupName=VLC+
OutputDir=build\stock-vlc-package\out
OutputBaseFilename=VLC_Plus_Setup_{#ProductVersion}_x64
Compression=zip/1
SolidCompression=no
PrivilegesRequired=lowest
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
WizardStyle=modern
LicenseFile=LICENSE
UninstallDisplayIcon={app}\vlc.exe

[Files]
Source: "build\stock-vlc-package\stage\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{userprograms}\VLC+"; Filename: "{app}\vlc.exe"; WorkingDir: "{app}"
Name: "{userdesktop}\VLC+"; Filename: "{app}\vlc.exe"; WorkingDir: "{app}"; Tasks: desktopicon

[Tasks]
Name: "desktopicon"; Description: "Create a desktop shortcut for video drag-and-drop"; GroupDescription: "Additional shortcuts:"

[Run]
Filename: "{app}\vlc.exe"; Description: "Open VLC+"; Flags: nowait postinstall skipifsilent
