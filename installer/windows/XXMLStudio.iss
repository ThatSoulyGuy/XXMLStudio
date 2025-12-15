; XXMLStudio Inno Setup Script
; This script creates a Windows installer for XXMLStudio

#define AppName "XXMLStudio"
#define AppVersion "${VERSION}"
#define AppPublisher "XXML"
#define AppURL "https://xxml.dev"
#define AppExeName "XXMLStudio.exe"
#define DeployDir "${DEPLOY_DIR}"
#define OutputDir "${OUTPUT_DIR}"

[Setup]
; Application information
AppId={{A1B2C3D4-E5F6-7890-ABCD-EF1234567890}
AppName={#AppName}
AppVersion={#AppVersion}
AppVerName={#AppName} {#AppVersion}
AppPublisher={#AppPublisher}
AppPublisherURL={#AppURL}
AppSupportURL={#AppURL}
AppUpdatesURL={#AppURL}

; Installation directories
DefaultDirName={autopf}\{#AppName}
DefaultGroupName={#AppName}
DisableProgramGroupPage=yes

; Output settings
OutputDir={#OutputDir}
OutputBaseFilename={#AppName}-{#AppVersion}-Setup
SetupIconFile={#DeployDir}\..\..\..\resources\icons\app-icon.ico
UninstallDisplayIcon={app}\{#AppExeName}

; Compression
Compression=lzma2/ultra64
SolidCompression=yes
LZMAUseSeparateProcess=yes

; Privileges (don't require admin for per-user install)
PrivilegesRequired=lowest
PrivilegesRequiredOverridesAllowed=dialog

; Appearance
WizardStyle=modern
WizardImageFile=compiler:WizModernImage.bmp
WizardSmallImageFile=compiler:WizModernSmallImage.bmp

; Misc
DisableWelcomePage=no
ShowLanguageDialog=auto
AllowNoIcons=yes
ChangesAssociations=yes

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked
Name: "quicklaunchicon"; Description: "{cm:CreateQuickLaunchIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked; OnlyBelowVersion: 6.1; Check: not IsAdminInstallMode
Name: "fileassoc"; Description: "Associate .xxml files with {#AppName}"; GroupDescription: "File associations:"; Flags: checkedonce
Name: "projectassoc"; Description: "Associate .xxmlp project files with {#AppName}"; GroupDescription: "File associations:"; Flags: checkedonce

[Files]
; Main application and Qt libraries
Source: "{#DeployDir}\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs

; Visual C++ Redistributable (if needed)
; Source: "vc_redist.x64.exe"; DestDir: "{tmp}"; Flags: deleteafterinstall; Check: NeedsVCRedist

[Icons]
Name: "{group}\{#AppName}"; Filename: "{app}\{#AppExeName}"
Name: "{group}\{cm:UninstallProgram,{#AppName}}"; Filename: "{uninstallexe}"
Name: "{autodesktop}\{#AppName}"; Filename: "{app}\{#AppExeName}"; Tasks: desktopicon
Name: "{userappdata}\Microsoft\Internet Explorer\Quick Launch\{#AppName}"; Filename: "{app}\{#AppExeName}"; Tasks: quicklaunchicon

[Registry]
; File associations for .xxml files
Root: HKA; Subkey: "Software\Classes\.xxml"; ValueType: string; ValueName: ""; ValueData: "XXMLStudio.xxmlfile"; Flags: uninsdeletevalue; Tasks: fileassoc
Root: HKA; Subkey: "Software\Classes\XXMLStudio.xxmlfile"; ValueType: string; ValueName: ""; ValueData: "XXML Source File"; Flags: uninsdeletekey; Tasks: fileassoc
Root: HKA; Subkey: "Software\Classes\XXMLStudio.xxmlfile\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\{#AppExeName},0"; Tasks: fileassoc
Root: HKA; Subkey: "Software\Classes\XXMLStudio.xxmlfile\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\{#AppExeName}"" ""%1"""; Tasks: fileassoc

; File associations for .xxmlp project files
Root: HKA; Subkey: "Software\Classes\.xxmlp"; ValueType: string; ValueName: ""; ValueData: "XXMLStudio.xxmlpfile"; Flags: uninsdeletevalue; Tasks: projectassoc
Root: HKA; Subkey: "Software\Classes\XXMLStudio.xxmlpfile"; ValueType: string; ValueName: ""; ValueData: "XXML Project File"; Flags: uninsdeletekey; Tasks: projectassoc
Root: HKA; Subkey: "Software\Classes\XXMLStudio.xxmlpfile\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\{#AppExeName},1"; Tasks: projectassoc
Root: HKA; Subkey: "Software\Classes\XXMLStudio.xxmlpfile\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\{#AppExeName}"" ""%1"""; Tasks: projectassoc

; Add to PATH (optional)
; Root: HKLM; Subkey: "SYSTEM\CurrentControlSet\Control\Session Manager\Environment"; ValueType: expandsz; ValueName: "Path"; ValueData: "{olddata};{app}"; Check: NeedsAddPath('{app}')

[Run]
; Run after installation
Filename: "{app}\{#AppExeName}"; Description: "{cm:LaunchProgram,{#StringChange(AppName, '&', '&&')}}"; Flags: nowait postinstall skipifsilent

; Install VC++ Redistributable if needed
; Filename: "{tmp}\vc_redist.x64.exe"; Parameters: "/quiet /norestart"; StatusMsg: "Installing Visual C++ Redistributable..."; Check: NeedsVCRedist

[UninstallDelete]
; Clean up user data (optional, uncomment to enable)
; Type: filesandordirs; Name: "{userappdata}\XXMLStudio"

[Code]
// Check if Visual C++ Redistributable is needed
function NeedsVCRedist(): Boolean;
begin
  // Check for VC++ 2015-2022 redistributable
  Result := not RegKeyExists(HKLM, 'SOFTWARE\Microsoft\VisualStudio\14.0\VC\Runtimes\x64');
end;

// Check if path needs to be added
function NeedsAddPath(Param: string): Boolean;
var
  OrigPath: string;
begin
  if not RegQueryStringValue(HKLM, 'SYSTEM\CurrentControlSet\Control\Session Manager\Environment', 'Path', OrigPath)
  then begin
    Result := True;
    exit;
  end;
  Result := Pos(';' + Param + ';', ';' + OrigPath + ';') = 0;
end;

// Initialize setup
function InitializeSetup(): Boolean;
begin
  Result := True;
end;

// Cleanup before uninstall
procedure CurUninstallStepChanged(CurUninstallStep: TUninstallStep);
begin
  if CurUninstallStep = usPostUninstall then
  begin
    // Notify shell of changes
    RegDeleteKeyIncludingSubkeys(HKCU, 'Software\Classes\.xxml');
    RegDeleteKeyIncludingSubkeys(HKCU, 'Software\Classes\XXMLStudio.xxmlfile');
    RegDeleteKeyIncludingSubkeys(HKCU, 'Software\Classes\.xxmlp');
    RegDeleteKeyIncludingSubkeys(HKCU, 'Software\Classes\XXMLStudio.xxmlpfile');
  end;
end;
