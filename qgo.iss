; Install script for qGo
; created for Inno Setup Compiler 5.2.3

[Setup]
AppName=qGo
AppVerName=qGo 2.0.0
AppPublisher=qGo Project
AppPublisherURL=http://qgo.sourceforge.net/
AppSupportURL=http://qgo.sourceforge.net/
AppUpdatesURL=http://qgo.sourceforge.net/
DefaultDirName={pf}\qGo
DefaultGroupName=qGo
AllowNoIcons=yes
UninstallDisplayIcon={app}\qGo.exe
;AlwaysCreateUninstallIcon=yes
;LicenseFile=D:\projekt\cGo\COPYING
; uncomment the following line if you want your installation to run on NT 3.51 too.
; MinVersion=4,3.51

[Tasks]
Name: "desktopicon"; Description: "Create a &desktop icon"; GroupDescription: "Additional icons:"; MinVersion: 4,4
;Name: "desktopmenuicon"; Description: "Create a &desktop icon for menu"; GroupDescription: "Additional icons:"; MinVersion: 4,4; Flags: unchecked
Name: "quicklaunchicon"; Description: "Create a &Quick Launch icon"; GroupDescription: "Additional icons:"; MinVersion: 4,4; Flags: unchecked
Name: "defaultsgfviewer"; Description: "Make qGo the default sgf viewer"; GroupDescription: "Default viewer:"; MinVersion: 4,4

[Files]
Source: "src\bin\qGo.exe"; DestDir: "{app}"; Flags: ignoreversion
;Source: "D:\projekt\cGo\html\*.html"; DestDir: "{app}\html"; Flags: ignoreversion
;Source: "D:\projekt\cGo\html\images\*.png"; DestDir: "{app}\html\images"; Flags: ignoreversion
Source: "src\ressources\sounds\*.wav"; DestDir: "{app}\sounds"; Flags: ignoreversion
;Source: "D:\projekt\cGo\src\pics\*.png"; DestDir: "{app}\pics"; Flags: ignoreversion
Source: "src\translations\*.qm"; DestDir: "{app}\translations"; Flags: ignoreversion
Source: "src\board\ressources\pics\barcelona_cherry.png"; DestDir: "{app}\boardtextures"; Flags: ignoreversion
Source: "src\board\ressources\pics\eurobeech.png"; DestDir: "{app}\boardtextures"; Flags: ignoreversion
Source: "src\board\ressources\pics\goldenbeech.png"; DestDir: "{app}\boardtextures"; Flags: ignoreversion
Source: "src\board\ressources\pics\lemontree.png"; DestDir: "{app}\boardtextures"; Flags: ignoreversion
Source: "src\board\ressources\pics\manitoba.png"; DestDir: "{app}\boardtextures"; Flags: ignoreversion
Source: "src\board\ressources\pics\maple.png"; DestDir: "{app}\boardtextures"; Flags: ignoreversion
Source: "src\board\ressources\pics\paper.png"; DestDir: "{app}\boardtextures"; Flags: ignoreversion
Source: "src\board\ressources\pics\wood.png"; DestDir: "{app}\boardtextures"; Flags: ignoreversion
Source: "src\board\ressources\pics\wood3.png"; DestDir: "{app}\boardtextures"; Flags: ignoreversion
Source: "src\board\ressources\pics\wood4.png"; DestDir: "{app}\boardtextures"; Flags: ignoreversion
Source: "src\board\ressources\pics\wood5.png"; DestDir: "{app}\boardtextures"; Flags: ignoreversion
;Source: "D:\projekt\cGo\TODO"; DestDir: "{app}"; Flags: ignoreversion
;Source: "D:\projekt\cGo\ChangeLog"; DestDir: "{app}"; Flags: ignoreversion
Source: "COPYING"; DestDir: "{app}"; Flags: ignoreversion
;Source: "D:\projekt\cGo\NEWS"; DestDir: "{app}"; Flags: ignoreversion
;Source: "D:\projekt\cGo\README"; DestDir: "{app}"; Flags: ignoreversion
Source: "AUTHORS"; DestDir: "{app}"; Flags: ignoreversion
Source: "c:\Qt\4.5.2\bin\QtCore4.dll"; DestDir: "{app}"; Flags: onlyifdoesntexist
Source: "c:\Qt\4.5.2\bin\QtGui4.dll"; DestDir: "{app}"; Flags: onlyifdoesntexist
Source: "c:\Qt\4.5.2\bin\QtNetwork4.dll"; DestDir: "{app}"; Flags: onlyifdoesntexist
Source: "c:\Qt\4.5.2\bin\mingwm10.dll"; DestDir: "{app}"; Flags: onlyifdoesntexist

[Registry]
Root: HKCR; Subkey: ".sgf"; ValueType: string; ValueName: ""; ValueData: "qGo"; Tasks: defaultsgfviewer; Flags: uninsdeletevalue
Root: HKCR; Subkey: "qGo"; ValueType: string; ValueName: ""; ValueData: "qGo"; Tasks: defaultsgfviewer; Flags: uninsdeletekey
Root: HKCR; Subkey: "qGo\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\qGo.exe,0"; Tasks: defaultsgfviewer
Root: HKCR; Subkey: "qGo\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\qGo.exe"" ""%1"""; Tasks: defaultsgfviewer
Root: HKLM; Subkey: "SOFTWARE\qGo"; ValueType: string; ValueName: "Location"; ValueData: "{app}"; Flags: uninsdeletekey
Root: HKLM; Subkey: "SOFTWARE\Microsoft\Windows\CurrentVersion\App Paths\qGo.exe"; ValueType: string; ValueName: ""; ValueData: "{app}\qGo.exe"; Flags: uninsdeletekey
Root: HKLM; Subkey: "SOFTWARE\Microsoft\Windows\CurrentVersion\App Paths\qGo.exe"; ValueType: string; ValueName: "Path"; ValueData: "{app}"; Flags: uninsdeletekey
Root: HKCU; Subkey: "Software\Microsoft\Windows\CurrentVersion\Explorer\FileExts\.sgf"; ValueType: string; ValueName: "Application"; ValueData: "qGo.exe"; Tasks: defaultsgfviewer; Flags: uninsdeletevalue

[INI]
Filename: "{app}\qGo.url"; Section: "InternetShortcut"; Key: "URL"; String: "http://qgo.sourceforge.net/"

[Icons]
Name: "{group}\qGo"; Filename: "{app}\qGo.exe"; WorkingDir: "{app}"
;Name: "{group}\qGo Menu"; Filename: "{app}\qGo.exe"; Parameters: "-menu"; WorkingDir: "{app}"
Name: "{group}\qGo Website"; Filename: "{app}\qGo.url"
Name: "{userdesktop}\qGo"; Filename: "{app}\qGo.exe"; MinVersion: 4,4; Tasks: desktopicon; WorkingDir: "{app}"
;Name: "{userdesktop}\qGo Menu"; Filename: "{app}\qGo.exe"; Parameters: "-menu"; MinVersion: 4,4; Tasks: desktopmenuicon; WorkingDir: "{app}"
Name: "{userappdata}\Microsoft\Internet Explorer\Quick Launch\qGo"; Filename: "{app}\qGo.exe"; MinVersion: 4,4; Tasks: quicklaunchicon; WorkingDir: "{app}"

[Run]
Filename: "{app}\qGo.exe"; Description: "Launch qGo"; Flags: nowait postinstall skipifsilent

[UninstallDelete]
Type: files; Name: "{app}\qGo.url"

