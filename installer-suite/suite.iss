; GPL-2.0-or-later. Component applications retain separate ownership.
#include "..\build\suite-stage\inputs.iss"

[Setup]
AppId={{965E973A-6173-4BBC-9DE4-6FA8A320DB93}
AppName=VLC+ and Remote LSL Recorder
AppVersion={#BundleVersion}
AppPublisher=George Fejer
DefaultDirName={localappdata}\Programs\VLCPlusRecorderSuite
DefaultGroupName=VLC+ and LSL Recorder
OutputDir=..\build\suite-release
OutputBaseFilename=01_VLCPlus_and_LSL_Recorder_Setup_{#BundleVersion}_x64
Compression=none
PrivilegesRequired=lowest
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
WizardStyle=modern
LicenseFile=..\LICENSE
InfoBeforeFile=INSTALL.txt
UninstallDisplayIcon={app}\VLCPlus-and-Recorder.exe

[Files]
Source: "{#Stage}\VLCPlus-and-Recorder.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#Stage}\components.json"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#Stage}\SuiteSource.zip"; DestDir: "{app}\source"; Flags: ignoreversion
Source: "INSTALL.txt"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\LICENSE"; DestDir: "{app}\licenses"; DestName: "SUITE-GPL-2.0.txt"; Flags: ignoreversion
Source: "{#VlcInstaller}"; Flags: dontcopy
Source: "{#RecorderInstaller}"; Flags: dontcopy

[Tasks]
Name: desktopshortcut; Description: "Create a desktop shortcut that opens both apps"; GroupDescription: "Shortcuts:"

[Icons]
Name: "{group}\VLC+ and LSL Recorder"; Filename: "{app}\VLCPlus-and-Recorder.exe"; WorkingDir: "{app}"
Name: "{userdesktop}\VLC+ and LSL Recorder"; Filename: "{app}\VLCPlus-and-Recorder.exe"; WorkingDir: "{app}"; Tasks: desktopshortcut

[Run]
Filename: "{app}\VLCPlus-and-Recorder.exe"; Description: "Open VLC+ and Remote LSL Recorder"; Flags: nowait postinstall skipifsilent; Check: CanLaunch

[Code]
var
  ComponentsInstalled: Boolean;
  ComponentRestart: Boolean;

function PrepareToInstall(var NeedsRestart: Boolean): String;
var
  Code: Integer;
begin
  Result := '';
  ExtractTemporaryFile('VLCPlus-and-Recorder.exe');
  if not Exec(ExpandConstant('{tmp}\VLCPlus-and-Recorder.exe'), '--install-ready', '', SW_HIDE, ewWaitUntilTerminated, Code) then
    Result := 'Could not check whether the apps are open. Please close both and retry.'
  else if Code = 20 then
    Result := 'Close VLC+ and Remote LSL Recorder before installing. Stop and save any active recording first.'
  else if Code <> 0 then
    Result := 'Could not verify that the recorder is ready for installation.';
  if Result <> '' then Exit;
  ExtractTemporaryFile('{#VlcFile}');
  ExtractTemporaryFile('{#RecorderFile}');
  if CompareText(GetSHA256OfFile(ExpandConstant('{tmp}\{#VlcFile}')), '{#VlcSha}') <> 0 then
    Result := 'The VLC+ installer failed its integrity check.'
  else if CompareText(GetSHA256OfFile(ExpandConstant('{tmp}\{#RecorderFile}')), '{#RecorderSha}') <> 0 then
    Result := 'The Remote LSL Recorder installer failed its integrity check.';
end;

procedure InstallComponent(const FileName, Arguments, Name: String);
var
  Code: Integer;
begin
  WizardForm.StatusLabel.Caption := 'Installing ' + Name + '...';
  if not Exec(FileName, Arguments, '', SW_HIDE, ewWaitUntilTerminated, Code) then
    RaiseException('Could not run the ' + Name + ' installer. Previously installed applications have been preserved.');
  if Code = 3010 then ComponentRestart := True
  else if Code <> 0 then
    RaiseException(Name + ' installation failed (exit ' + IntToStr(Code) + '). Previously installed applications have been preserved.');
end;

procedure CurStepChanged(CurStep: TSetupStep);
var
  Code: Integer;
begin
  if CurStep = ssInstall then begin
    if not Exec(ExpandConstant('{tmp}\VLCPlus-and-Recorder.exe'), '--install-ready', '', SW_HIDE, ewWaitUntilTerminated, Code) then
      RaiseException('Could not check whether the apps are open.');
    if Code <> 0 then
      RaiseException('Close VLC+ and Remote LSL Recorder, stop and save any recording, then retry.');
    InstallComponent(ExpandConstant('{tmp}\{#VlcFile}'), '/VERYSILENT /SUPPRESSMSGBOXES /CURRENTUSER /NORESTART /NOCLOSEAPPLICATIONS /NORESTARTAPPLICATIONS /TASKS=', 'VLC+');
    if not Exec(ExpandConstant('{tmp}\VLCPlus-and-Recorder.exe'), '--install-ready', '', SW_HIDE, ewWaitUntilTerminated, Code) then
      RaiseException('Could not check whether Remote LSL Recorder is open.');
    if Code <> 0 then
      RaiseException('Close VLC+ and Remote LSL Recorder, stop and save any recording, then retry. VLC+ has been preserved.');
    InstallComponent(ExpandConstant('{tmp}\{#RecorderFile}'), '/S', 'Remote LSL Recorder');
    if not Exec(ExpandConstant('{tmp}\VLCPlus-and-Recorder.exe'), '--check', '', SW_HIDE, ewWaitUntilTerminated, Code) then
      RaiseException('Both applications must be installed before the joint shortcut can be created.');
    if Code <> 0 then
      RaiseException('The joint launcher could not locate both installed applications. Use their separate installers to repair them.');
    ComponentsInstalled := True;
  end;
end;

function NeedRestart(): Boolean;
begin
  Result := ComponentRestart;
end;

function CanLaunch(): Boolean;
begin
  Result := ComponentsInstalled and not ComponentRestart;
end;
