#define MyAppName "ShutdownBuddy"
#define MyAppVersion "0.2"
#define MyAppPublisher "Peter Upfold"
#define MyAppURL "https://peter.upfold.org.uk/projects/shutdownbuddy"
#define MyAppExeName "ShutdownBuddy.exe"

[Setup]
; NOTE: The value of AppId uniquely identifies this application. Do not use the same AppId value in installers for other applications.
; (To generate a new GUID, click Tools | Generate GUID inside the IDE.)
AppId={{5B78AC4F-62D7-49F8-838E-8675832C9EF8}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
;AppVerName={#MyAppName} {#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}
AppUpdatesURL={#MyAppURL}
DefaultDirName={commonpf64}\{#MyAppName}
DefaultGroupName={#MyAppName}
;LicenseFile=C:\Users\Peter\source\repos\ShutdownBuddy\LICENSE
InfoBeforeFile=C:\Users\Peter\source\repos\ShutdownBuddy\installer-beforeinstall-doc.txt
OutputDir=C:\Users\Peter\source\repos\ShutdownBuddy\x64\output
OutputBaseFilename=ShutdownBuddy-setup
Compression=lzma
SolidCompression=yes
WizardStyle=modern
ArchitecturesInstallIn64BitMode=x64
DisableDirPage=yes

[Messages]
FinishedLabel=Setup has finished installing ShutdownBuddy. The service has been started.

[CustomMessages]

[Run]
Filename: sc.exe; StatusMsg: "Installing service..."; Parameters: "create ShutdownBuddy binPath= ""{app}\ShutdownBuddy.exe"" start= delayed-auto"
Filename: sc.exe; StatusMsg: "Starting service..."; Parameters: "start ShutdownBuddy"

[UninstallRun]
Filename: sc.exe; Parameters: "stop ShutdownBuddy"
Filename: sc.exe; Parameters: "delete ShutdownBuddy"

[Code]
var
  InputQueryWizardPageID: Integer;
  InputQueryWizardPage: TInputQueryWizardPage;
  EvaluationIntervalSecondsInt: Longint;
  ShutdownAfterIdleForSecondsInt: Longint;
  DebugLogInt: Longint;

function PrepareToInstall(var NeedsRestart: Boolean): String;
var
  ResultCode: Integer;
  MajorVersion: Cardinal;
  ShouldInstall: Boolean;
begin
  // stop the service if it exists
  Exec('sc.exe', 'stop ShutdownBuddy', '', SW_HIDE, ewWaitUntilTerminated, ResultCode);

  // determine if we should install the VC redist
  if (RegQueryDWordValue(HKEY_LOCAL_MACHINE, 'SOFTWARE\Microsoft\VisualStudio\14.0\VC\Runtimes\x64', 'Major', MajorVersion)) then
    begin
      ShouldInstall := MajorVersion < 14;
    end
  else
    begin
      ShouldInstall := True
    end;
  
  if (ShouldInstall) then
    begin
    ExtractTemporaryFile('vcredist.exe');
    Exec(ExpandConstant('{tmp}\vcredist.exe'), '/passive', '', SW_HIDE, ewWaitUntilTerminated, ResultCode);
    end;
  
end;

procedure InitializeWizard;
begin
  InputQueryWizardPage := CreateInputQueryPage(wpUserInfo,
    'Set ShutdownBuddy options in registry',
    'On this screen, you can customise the options for ShutdownBuddy.',
    'These can be changed later in the registry at HKLM\SOFTWARE\upfold.org.uk\ShutdownBuddy. If you enter anything other than an integer, the default values will be used.');
  InputQueryWizardPage.Add('Evaluation &interval (seconds)', False);
  InputQueryWizardPage.Add('&Shutdown after idle for (seconds)', False);
  InputQueryWizardPage.Add('&Debug logging enabled (0/1)', False);

  InputQueryWizardPage.Values[0] := '60' // default evaluation interval
  InputQueryWizardPage.Values[1] := '3600' // default shutdown after
  InputQueryWizardPage.Values[2] := '0' // default debug logging
  InputQueryWizardPageID := InputQueryWizardPage.ID;
end;

function NextButtonClick(CurPageID: Integer): Boolean;
begin
    case CurPageID of InputQueryWizardPageID:
      begin
             
        // command line switch will take precedence if provided, otherwise the UI entry will be used
        //    if an invalid value is provided (not an integer), we will always fall back to default values

        //EvaluationIntervalSeconds
        if ExpandConstant('{param:EvaluationIntervalSeconds|notset}') = 'notset' then
        begin
          EvaluationIntervalSecondsInt := StrToIntDef(InputQueryWizardPage.Values[0], 60);
        end
        else
        begin
          EvaluationIntervalSecondsInt := StrToIntDef(ExpandConstant('{param:EvaluationIntervalSeconds|notset}'), 60);
        end;

        //ShutdownAfterIdleForSeconds
        if ExpandConstant('{param:ShutdownAfterIdleForSeconds|notset}') = 'notset' then
        begin
          ShutdownAfterIdleForSecondsInt :=  StrToIntDef(InputQueryWizardPage.Values[1], 3600);
        end
        else
        begin
          ShutdownAfterIdleForSecondsInt :=  StrToIntDef(ExpandConstant('{param:ShutdownAfterIdleForSeconds|notset}'), 3600);
        end;

        //DebugLog
        if ExpandConstant('{param:DebugLog|notset}') = 'notset' then
        begin
          DebugLogInt :=  StrToIntDef(InputQueryWizardPage.Values[2], 0);
        end
        else
        begin
          DebugLogInt :=  StrToIntDef(ExpandConstant('{param:DebugLog|notset}'), 0);
        end;
        result := true
      end;
      else result := true;
    end;
end;

// for some reason, these have to accept a string param and return a string to use in the [Registry] section
function EvaluationIntervalSeconds(Param: String): String;
begin                             
  result := IntToStr(EvaluationIntervalSecondsInt)
end;

function ShutdownAfterIdleForSeconds(Param: String): string;
begin
  result := IntToStr(ShutdownAfterIdleForSecondsInt)
end;

function DebugLog(Param: String): String;
begin
  result := IntToStr(DebugLogInt)
end;

[Registry]
Root: HKLM64; Subkey: "SOFTWARE\upfold.org.uk\ShutdownBuddy"; ValueType: dword; ValueName: "EvaluationIntervalSeconds"; ValueData: "{code:EvaluationIntervalSeconds}"; Flags: uninsdeletevalue
Root: HKLM64; Subkey: "SOFTWARE\upfold.org.uk\ShutdownBuddy"; ValueType: dword; ValueName: "ShutdownAfterIdleForSeconds"; ValueData: "{code:ShutdownAfterIdleForSeconds}"; Flags: uninsdeletevalue
Root: HKLM64; Subkey: "SOFTWARE\upfold.org.uk\ShutdownBuddy"; ValueType: dword; ValueName: "DebugLog"; ValueData: "{code:DebugLog}"; Flags: uninsdeletevalue
Root: HKLM64; Subkey: "SOFTWARE\upfold.org.uk\ShutdownBuddy"; Flags: uninsdeletekeyifempty

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Files]
Source: "C:\Users\Peter\source\repos\ShutdownBuddy\x64\Release\{#MyAppExeName}"; DestDir: "{app}"; Flags: ignoreversion
Source: "C:\Users\Peter\source\repos\ShutdownBuddy\external-resources\vcredist.exe"; DestDir: "{tmp}"; Flags: deleteafterinstall
; NOTE: Don't use "Flags: ignoreversion" on any shared system files

[Icons]
;Name: "{group}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"
Name: "{group}\{cm:UninstallProgram,{#MyAppName}}"; Filename: "{uninstallexe}"

