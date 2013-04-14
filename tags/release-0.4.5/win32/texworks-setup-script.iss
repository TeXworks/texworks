; UTF-8 script for Inno Setup Unicode

; Definitions using Inno Setup Preprocessor
#define APPNAME      GetStringFileInfo("..\release/TeXworks.exe", PRODUCT_NAME)
#define APPVERNAME   GetStringFileInfo("..\release/TeXworks.exe", PRODUCT_VERSION)
#define APPPUBLISHER GetStringFileInfo("..\release/TeXworks.exe", COMPANY_NAME)
#define APPCOPYRIGHT GetStringFileInfo("..\release/TeXworks.exe", LEGAL_COPYRIGHT)
#define VERSIONINFO  GetFileVersion("..\release/TeXworks.exe")

[Setup]
; NOTE: The value of AppId uniquely identifies this application.
; Do not use the same AppId value in installers for other applications.
; (To generate a new GUID, click Tools | Generate GUID inside the IDE.)
AppId={{41DA4817-4D2A-4D83-AD02-6A2D95DC8DCB}
AppName={#APPNAME}
AppVerName={#APPVERNAME}
AppPublisher={#APPPUBLISHER}
AppPublisherURL=http://www.tug.org/texworks/
AppSupportURL=http://www.tug.org/texworks/
AppUpdatesURL=http://www.tug.org/texworks/
AppCopyright={#APPCOPYRIGHT}
DefaultDirName={pf}\{#APPNAME}
DefaultGroupName={#APPNAME}
AllowNoIcons=yes
LicenseFile=..\COPYING
OutputBaseFilename={#APPNAME}-setup-v{#VERSIONINFO}
SetupIconFile=..\res\images\TeXworks-setup.ico
Compression=lzma
SolidCompression=yes
ChangesAssociations=yes
VersionInfoVersion={#VERSIONINFO}
WizardSmallImageFile=..\res\images\TeXworks-small.bmp

[Languages]
Name: "ca"; MessagesFile: "compiler:Languages\Catalan.isl"
Name: "cs"; MessagesFile: "compiler:Languages\Czech.isl"
Name: "de"; MessagesFile: "compiler:Languages\German.isl"
Name: "en"; MessagesFile: "compiler:Default.isl"
Name: "es"; MessagesFile: "compiler:Languages\Spanish.isl"
Name: "fr"; MessagesFile: "compiler:Languages\French.isl"
Name: "it"; MessagesFile: "compiler:Languages\Italian.isl"
Name: "ja"; MessagesFile: "compiler:Languages\Japanese.isl"
Name: "nl"; MessagesFile: "compiler:Languages\Dutch.isl"
Name: "pl"; MessagesFile: "compiler:Languages\Polish.isl"
Name: "pt_BR"; MessagesFile: "compiler:Languages\BrazilianPortuguese.isl"
Name: "ru"; MessagesFile: "compiler:Languages\Russian.isl"
Name: "sl"; MessagesFile: "compiler:Languages\Slovenian.isl"
; additional Inno Setup languages using contributed translations
Name: "af"; MessagesFile: "extra-setup-languages\Afrikaans-1-5.1.11.isl"
Name: "ar"; MessagesFile: "extra-setup-languages\Arabic-4-5.1.11.isl"
Name: "fa"; MessagesFile: "extra-setup-languages\Farsi-2-5.1.11.isl"
Name: "ko"; MessagesFile: "extra-setup-languages\Korean-5-5.1.11.isl"
Name: "tr"; MessagesFile: "extra-setup-languages\Turkish-3-5.1.11.isl"
Name: "zh_cn"; MessagesFile: "extra-setup-languages\ChineseSimp-12-5.1.11.isl"

[CustomMessages]
ManualName=A short manual for TeXworks
ca.CreateFileAssoc=Obri el següents fitxers amb TeXworks per defecte:
cs.CreateFileAssoc=Nastavit TeXworks jako výchozí program pro otevírání následujících typů souborů:
de.CreateFileAssoc=Folgende Dateitypen standardmäßig mit TeXworks öffnen:
en.CreateFileAssoc=Open the following file types with TeXworks by default:
es.CreateFileAssoc=De manera predeterminada abrir los siguientes tipos de archivo con TeXworks:
fr.CreateFileAssoc=Ouvrir par défaut les types de fichiers suivant avec TeXworks:
it.CreateFileAssoc=Apri di default i seguenti tipi di file con TeXworks:
nl.CreateFileAssoc=Verbind de volgende bestandstypen met TeXworks:
pl.CreateFileAssoc=Domyślnie otwieraj następujące typy plików za pomocą edytora TeXworks:
pt_BR.CreateFileAssoc=
ru.CreateFileAssoc=Открывать следующие файлы по умолчанию в TeXworks:
sl.CreateFileAssoc=Uporabi TeXworks za odpiranje naslednjih vrst datotek:

af.CreateFileAssoc=Maak by verstek die volgende lêertipes met TeXworks oop:
ar.CreateFileAssoc=افتح أنواع الملفات التالية في TeXworks مبدئيا:
fa.CreateFileAssoc=پرونده‌های از نوع زیر را به‌طور پیش‌فرض با تک‌ورکس بازکن:
ja.CreateFileAssoc=デフォルトで次の種類のファイルを TeXworks で開く:
ko.CreateFileAssoc=다음 유형의 파일을 열 때 기본값으로 TeXworks를 이용합니다:
tr.CreateFileAssoc=Aşağıdaki dosya türlerini öntanımlı olarak TeXworks ile aç:
zh_cn.CreateFileAssoc=默认使用 TeXworks 打开下列文件类型：
; Inno Setup doesn't support Faroese (yet)
; fo.CreateFileAssoc=Opna sum standard hesi fílusløg við TeXworks:

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"
Name: "quicklaunchicon"; Description: "{cm:CreateQuickLaunchIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked
Name: "texfileassoc"; Description: "{cm:AssocFileExtension,TeXworks,.tex}"; GroupDescription: "{cm:CreateFileAssoc}"
Name: "ltxfileassoc"; Description: "{cm:AssocFileExtension,TeXworks,.ltx}"; GroupDescription: "{cm:CreateFileAssoc}"
Name: "styfileassoc"; Description: "{cm:AssocFileExtension,TeXworks,.sty}"; GroupDescription: "{cm:CreateFileAssoc}"
Name: "clsfileassoc"; Description: "{cm:AssocFileExtension,TeXworks,.cls}"; GroupDescription: "{cm:CreateFileAssoc}"
Name: "dtxfileassoc"; Description: "{cm:AssocFileExtension,TeXworks,.dtx}"; GroupDescription: "{cm:CreateFileAssoc}"
Name: "pdffileassoc"; Description: "{cm:AssocFileExtension,TeXworks,.pdf}"; GroupDescription: "{cm:CreateFileAssoc}"; Flags: unchecked

[Files]
Source: "..\release\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{group}\TeXworks"; Filename: "{app}\TeXworks.exe"
Name: "{group}\{cm:ProgramOnTheWeb,TeXworks}"; Filename: "http://www.tug.org/texworks/"
Name: "{group}\{cm:ManualName}"; Filename: "{app}\texworks-help\TeXworks-manual\en\TeXworks-manual.pdf"
Name: "{group}\{cm:UninstallProgram,TeXworks}"; Filename: "{uninstallexe}"
Name: "{commondesktop}\TeXworks"; Filename: "{app}\TeXworks.exe"; Tasks: desktopicon
Name: "{userappdata}\Microsoft\Internet Explorer\Quick Launch\TeXworks"; Filename: "{app}\TeXworks.exe"; Tasks: quicklaunchicon

[Registry]
Root: HKCR; Subkey: ".tex"; ValueType: string; ValueName: ""; ValueData: "TeXworksTeXFile"; Flags: uninsdeletevalue; Tasks: texfileassoc
Root: HKCR; Subkey: ".tex"; ValueType: string; ValueName: "Content Type"; ValueData: "text/x-tex"; Flags: uninsdeletevalue; Tasks: texfileassoc
Root: HKCR; Subkey: "TeXworksTeXFile"; ValueType: string; ValueName: ""; ValueData: "(La)TeX File"; Flags: uninsdeletekey; Tasks: texfileassoc
Root: HKCR; Subkey: "TeXworksTeXFile\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\TeXworks.exe,1"; Tasks: texfileassoc
Root: HKCR; Subkey: "TeXworksTeXFile\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\TeXworks.exe"" ""%1"""; Tasks: texfileassoc

Root: HKCR; Subkey: ".ltx"; ValueType: string; ValueName: ""; ValueData: "TeXworksTeXFile"; Flags: uninsdeletevalue; Tasks: ltxfileassoc
Root: HKCR; Subkey: ".ltx"; ValueType: string; ValueName: "Content Type"; ValueData: "text/x-tex"; Flags: uninsdeletevalue; Tasks: ltxfileassoc
Root: HKCR; Subkey: "TeXworksTeXFile"; ValueType: string; ValueName: ""; ValueData: "(La)TeX File"; Flags: uninsdeletekey; Tasks: ltxfileassoc
Root: HKCR; Subkey: "TeXworksTeXFile\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\TeXworks.exe,1"; Tasks: ltxfileassoc
Root: HKCR; Subkey: "TeXworksTeXFile\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\TeXworks.exe"" ""%1"""; Tasks: ltxfileassoc

Root: HKCR; Subkey: ".sty"; ValueType: string; ValueName: ""; ValueData: "TeXworksStyFile"; Flags: uninsdeletevalue; Tasks: styfileassoc
Root: HKCR; Subkey: ".sty"; ValueType: string; ValueName: "Content Type"; ValueData: "text/x-tex"; Flags: uninsdeletevalue; Tasks: styfileassoc
Root: HKCR; Subkey: "TeXworksStyFile"; ValueType: string; ValueName: ""; ValueData: "(La)TeX Style File"; Flags: uninsdeletekey; Tasks: styfileassoc
Root: HKCR; Subkey: "TeXworksStyFile\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\TeXworks.exe,1"; Tasks: styfileassoc
Root: HKCR; Subkey: "TeXworksStyFile\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\TeXworks.exe"" ""%1"""; Tasks: styfileassoc

Root: HKCR; Subkey: ".cls"; ValueType: string; ValueName: ""; ValueData: "TeXworksClsFile"; Flags: uninsdeletevalue; Tasks: clsfileassoc
Root: HKCR; Subkey: ".cls"; ValueType: string; ValueName: "Content Type"; ValueData: "text/x-tex"; Flags: uninsdeletevalue; Tasks: clsfileassoc
Root: HKCR; Subkey: "TeXworksClsFile"; ValueType: string; ValueName: ""; ValueData: "(La)TeX Class File"; Flags: uninsdeletekey; Tasks: clsfileassoc
Root: HKCR; Subkey: "TeXworksClsFile\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\TeXworks.exe,1"; Tasks: clsfileassoc
Root: HKCR; Subkey: "TeXworksClsFile\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\TeXworks.exe"" ""%1"""; Tasks: clsfileassoc

Root: HKCR; Subkey: ".dtx"; ValueType: string; ValueName: ""; ValueData: "TeXworksDtxFile"; Flags: uninsdeletevalue; Tasks: dtxfileassoc
Root: HKCR; Subkey: ".dtx"; ValueType: string; ValueName: "Content Type"; ValueData: "text/x-tex"; Flags: uninsdeletevalue; Tasks: dtxfileassoc
Root: HKCR; Subkey: "TeXworksDtxFile"; ValueType: string; ValueName: ""; ValueData: "Documented (La)TeX File"; Flags: uninsdeletekey; Tasks: dtxfileassoc
Root: HKCR; Subkey: "TeXworksDtxFile\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\TeXworks.exe,1"; Tasks: dtxfileassoc
Root: HKCR; Subkey: "TeXworksDtxFile\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\TeXworks.exe"" ""%1"""; Tasks: dtxfileassoc

Root: HKCR; Subkey: ".pdf"; ValueType: string; ValueName: ""; ValueData: "TeXworksPDFFile"; Flags: uninsdeletevalue; Tasks: pdffileassoc
Root: HKCR; Subkey: ".pdf"; ValueType: string; ValueName: "Content Type"; ValueData: "application/pdf"; Flags: uninsdeletevalue; Tasks: pdffileassoc
Root: HKCR; Subkey: "TeXworksPDFFile"; ValueType: string; ValueName: ""; ValueData: "Portable Document Format"; Flags: uninsdeletekey; Tasks: pdffileassoc
Root: HKCR; Subkey: "TeXworksPDFFile\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\TeXworks.exe,1"; Tasks: pdffileassoc
Root: HKCR; Subkey: "TeXworksPDFFile\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\TeXworks.exe"" ""%1"""; Tasks: pdffileassoc

[Run]
Filename: "{app}\TeXworks.exe"; Description: "{cm:LaunchProgram,TeXworks}"; Flags: nowait postinstall skipifsilent
