[Setup]
AppName=Apollo Compiler
AppVersion=3.0.1
AppPublisher=SCXRPIUS
DefaultDirName={autopf}\Apollo
DefaultGroupName=Apollo
OutputBaseFilename=Apollo_Setup_v3.0.1
Compression=lzma2
SolidCompression=yes
ArchitecturesAllowed=x64
ArchitecturesInstallIn64BitMode=x64
; We need admin privileges to write to system PATH
PrivilegesRequired=admin
ChangesEnvironment=yes

[Files]
Source: "build\apollo.exe"; DestDir: "{app}"; Flags: ignoreversion

[Registry]
; Add Apollo to system PATH safely
Root: HKLM; Subkey: "SYSTEM\CurrentControlSet\Control\Session Manager\Environment"; \
    ValueType: expandsz; ValueName: "Path"; ValueData: "{olddata};{app}"; \
    Check: NeedsAddPath(ExpandConstant('{app}'))

[Code]
function NeedsAddPath(Param: string): boolean;
var
  OrigPath: string;
begin
  if not RegQueryStringValue(HKEY_LOCAL_MACHINE,
    'SYSTEM\CurrentControlSet\Control\Session Manager\Environment',
    'Path', OrigPath)
  then begin
    Result := True;
    exit;
  end;
  { Check if path is already present to prevent duplicate appending }
  Result := Pos(';' + Uppercase(Param) + ';', ';' + Uppercase(OrigPath) + ';') = 0;
end;
