
#define MyAppName "GPK_MP"
#ifndef AppVersion
  #define AppVersion "1.0.0"
#endif
#define SourceDir "D:\Repo\GPK_MP\GPK_MP"

[Setup]
AppId={{B72F6C21-9E43-4D81-A7C2-5F39B2A61480}
AppName={#MyAppName}
AppVersion={#AppVersion}
AppPublisher=GPK_MP Team
DefaultDirName=C:\Program Files (x86)\Корсары - ГПК rev.3
DefaultGroupName=GPK_MP
DisableProgramGroupPage=yes
DisableWelcomePage=no
WizardStyle=modern

PrivilegesRequired=admin
PrivilegesRequiredOverridesAllowed=dialog

OutputDir=D:\Repo\GPK_MP\Installer
OutputBaseFilename=GPK_MP_Setup_{#AppVersion}

Compression=lzma2
SolidCompression=yes

Uninstallable=yes
UninstallDisplayName=GPK_MP
SetupLogging=yes

CloseApplications=yes
RestartApplications=no

[Languages]
Name: "russian"; MessagesFile: "compiler:Languages\Russian.isl"

[Files]
Source: "{#SourceDir}\*"; DestDir: "{app}"; \
    Flags: ignoreversion recursesubdirs createallsubdirs

[Code]

function NextButtonClick(CurPageID: Integer): Boolean;
begin
  Result := True;

  if CurPageID = wpSelectDir then
  begin
    if not DirExists(WizardDirValue) then
    begin
      MsgBox(
        'Указанная папка игры не существует!' + #13#10 +
        'Выберите существующую папку Корсары - ГПК rev.3.',
        mbError, MB_OK
      );

      Result := False;
    end;
  end;
end;

function InitializeSetup(): Boolean;
begin
  Result := True;
end;
