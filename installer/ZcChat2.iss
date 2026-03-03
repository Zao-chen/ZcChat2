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

;dist根目录dll
Source: "{#MyAppPath}\*.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#MyAppPath}\**\*.dll"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs skipifsourcedoesntexist

;Qt部署目录(注意：windeployqt默认不是plugins结构，而是platforms/imageformats/...)
Source: "{#MyAppPath}\platforms\*"; DestDir: "{app}\platforms"; Flags: ignoreversion recursesubdirs createallsubdirs skipifsourcedoesntexist
Source: "{#MyAppPath}\imageformats\*"; DestDir: "{app}\imageformats"; Flags: ignoreversion recursesubdirs createallsubdirs skipifsourcedoesntexist
Source: "{#MyAppPath}\styles\*"; DestDir: "{app}\styles"; Flags: ignoreversion recursesubdirs createallsubdirs skipifsourcedoesntexist
Source: "{#MyAppPath}\iconengines\*"; DestDir: "{app}\iconengines"; Flags: ignoreversion recursesubdirs createallsubdirs skipifsourcedoesntexist
Source: "{#MyAppPath}\multimedia\*"; DestDir: "{app}\multimedia"; Flags: ignoreversion recursesubdirs createallsubdirs skipifsourcedoesntexist
Source: "{#MyAppPath}\tls\*"; DestDir: "{app}\tls"; Flags: ignoreversion recursesubdirs createallsubdirs skipifsourcedoesntexist
Source: "{#MyAppPath}\generic\*"; DestDir: "{app}\generic"; Flags: ignoreversion recursesubdirs createallsubdirs skipifsourcedoesntexist
Source: "{#MyAppPath}\networkinformation\*"; DestDir: "{app}\networkinformation"; Flags: ignoreversion recursesubdirs createallsubdirs skipifsourcedoesntexist
Source: "{#MyAppPath}\translations\*"; DestDir: "{app}\translations"; Flags: ignoreversion recursesubdirs createallsubdirs skipifsourcedoesntexist

;默认用户模板
Source: "{#MyAppPath}\.config\ZcChat2\*"; DestDir: "{userdocs}\ZcChat2"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{group}\ZcChat2"; Filename: "{app}\ZcChat2.exe"
Name: "{commondesktop}\ZcChat2"; Filename: "{app}\ZcChat2.exe"

[Run]
Filename: "{app}\ZcChat2.exe"; Description: "启动 ZcChat2"; Flags: nowait postinstall skipifsilent