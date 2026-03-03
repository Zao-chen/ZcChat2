#define MyAppPath "..\dist"

[Setup]
AppId={{7A6E3B6B-6B3F-4B7C-9F76-7B7A9B1D2C10}
AppName=ZcChat2
AppVersion={#MyAppVersion}
AppPublisher=ChenZao
DefaultDirName={autopf}\ZcChat2
DefaultGroupName=ZcChat2
OutputDir=output
OutputBaseFilename=ZcChat2_Setup
Compression=lzma
SolidCompression=yes
WizardStyle=modern

[Languages]
Name: "chinesesimp"; MessagesFile: "compiler:Default.isl"

[Files]
Source: "{#MyAppPath}\ZcChat2.exe"; DestDir: "{app}"; Flags: ignoreversion

;把dist里所有dll都带上(包含子目录，防止未来改结构漏掉)
Source: "{#MyAppPath}\*.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#MyAppPath}\**\*.dll"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs skipifsourcedoesntexist

;Qt插件(必须是plugins结构)
Source: "{#MyAppPath}\plugins\*"; DestDir: "{app}\plugins"; Flags: ignoreversion recursesubdirs createallsubdirs

;默认用户模板
Source: "{#MyAppPath}\.config\ZcChat2\*"; DestDir: "{userdocs}\ZcChat2"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{group}\ZcChat2"; Filename: "{app}\ZcChat2.exe"
Name: "{commondesktop}\ZcChat2"; Filename: "{app}\ZcChat2.exe"

[Run]
Filename: "{app}\ZcChat2.exe"; Description: "启动 ZcChat2"; Flags: nowait postinstall skipifsilent