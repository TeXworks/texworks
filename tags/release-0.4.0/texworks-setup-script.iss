; UTF-8 script for Inno Setup Unicode

; Definitions using Inno Setup Preprocessor
#define APPNAME      GetStringFileInfo("release/TeXworks.exe", PRODUCT_NAME)
#define APPVERNAME   GetStringFileInfo("release/TeXworks.exe", PRODUCT_VERSION)
#define APPPUBLISHER GetStringFileInfo("release/TeXworks.exe", COMPANY_NAME)
#define APPCOPYRIGHT GetStringFileInfo("release/TeXworks.exe", LEGAL_COPYRIGHT)
#define VERSIONINFO  GetFileVersion("release/TeXworks.exe")

[Setup]
; NOTE: The value of AppId uniquely identifies this application.
; Do not use the same AppId value in installers for other applications.
; (To generate a new GUID, click Tools | Generate GUID inside the IDE.)
AppId={{41DA4817-4D2A-4D83-AD02-6A2D95DC8DCB}
AppName={#APPNAME}
AppVerName={#APPVERNAME}
AppPublisher={#APPPUBLISHER}
AppPublisherURL=http://texworks.org/
AppSupportURL=http://texworks.org/
AppUpdatesURL=http://texworks.org/
AppCopyright={#APPCOPYRIGHT}
DefaultDirName={pf}\{#APPNAME}
DefaultGroupName={#APPNAME}
AllowNoIcons=yes
LicenseFile=COPYING
OutputBaseFilename={#APPNAME}-setup-v{#VERSIONINFO}
SetupIconFile=res\images\TeXworks.ico
Compression=lzma
SolidCompression=yes
ChangesAssociations=yes
VersionInfoVersion={#VERSIONINFO}
WizardSmallImageFile=res\images\TeXworks-small.bmp

[Languages]
Name: "en"; MessagesFile: "compiler:Default.isl"
Name: "ca"; MessagesFile: "compiler:Languages\Catalan.isl"
Name: "cs"; MessagesFile: "compiler:Languages\Czech.isl"
Name: "nl"; MessagesFile: "compiler:Languages\Dutch.isl"
Name: "fr"; MessagesFile: "compiler:Languages\French.isl"
Name: "de"; MessagesFile: "compiler:Languages\German.isl"
Name: "it"; MessagesFile: "compiler:Languages\Italian.isl"
Name: "pl"; MessagesFile: "compiler:Languages\Polish.isl"
Name: "pt_BR"; MessagesFile: "compiler:Languages\BrazilianPortuguese.isl"
Name: "ru"; MessagesFile: "compiler:Languages\Russian.isl"
Name: "sl"; MessagesFile: "compiler:Languages\Slovenian.isl"
Name: "es"; MessagesFile: "compiler:Languages\Spanish.isl"
; additional Inno Setup languages using contributed translations
Name: "ar"; MessagesFile: "extra-setup-languages\Arabic-4-5.1.11.isl"
Name: "zh_cn"; MessagesFile: "extra-setup-languages\ChineseSimp-12-5.1.11.isl"
Name: "ja"; MessagesFile: "extra-setup-languages\Japanese-5-5.1.11.isl"
Name: "ko"; MessagesFile: "extra-setup-languages\Korean-5-5.1.11.isl"
Name: "fa"; MessagesFile: "extra-setup-languages\Farsi-2-5.1.11.isl"
Name: "tr"; MessagesFile: "extra-setup-languages\Turkish-3-5.1.11.isl"

[CustomMessages]
en.CreateFileAssoc=Open the following file types with TeXworks by default:
ca.CreateFileAssoc=Obre els següents tipus de fitxer amb TeXworks per defecte:
cs.CreateFileAssoc=Nastavit TeXworks jako výchozí program pro otevírání následujících typů souborů:
nl.CreateFileAssoc=Open de volgende bestandstypen standaard met TeXworks:
fr.CreateFileAssoc=Par défaut, ouvrir les types de fichiers suivants avec TeXworks:
de.CreateFileAssoc=Folgende Dateitypen standardmäßig mit TeXworks öffnen:
it.CreateFileAssoc=Impostare TeXworks come programma predefinito per i seguenti tipi di file:
pl.CreateFileAssoc=
pt_BR.CreateFileAssoc=
ru.CreateFileAssoc=Открывать следующие файлы по умолчанию в TeXworks:
sl.CreateFileAssoc=Uporabi TeXworks za privzeto odpiranje datotek z naslednjimi končnicami:
es.CreateFileAssoc=Abrir los siguientes tipos de archivo con TeXworks por omisión:

ar.CreateFileAssoc=افتح أنواع الملفات التالية افتراضيا في TeXworks
zh_cn.CreateFileAssoc=默认使用 TeXworks 打开以下文件类型：
ja.CreateFileAssoc=デフォルトで次の種類のファイルを TeXworks で開く:
ko.CreateFileAssoc=다음 파일 유형을 TeXworks로 열기:
fa.CreateFileAssoc=پرونده‌های از نوع زیر را به‌صورت پیش‌فرض با TeXworks باز کن:
tr.CreateFileAssoc=Aşağıdaki dosya türlerini öntanımlı olarak TeXworks ile aç:

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"
Name: "quicklaunchicon"; Description: "{cm:CreateQuickLaunchIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked
Name: "texfileassoc"; Description: "{cm:AssocFileExtension,TeXworks,.tex}"; GroupDescription: "{cm:CreateFileAssoc}"
Name: "pdffileassoc"; Description: "{cm:AssocFileExtension,TeXworks,.pdf}"; GroupDescription: "{cm:CreateFileAssoc}"; Flags: unchecked

[Files]
Source: "release\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{group}\TeXworks"; Filename: "{app}\TeXworks.exe"
Name: "{group}\{cm:ProgramOnTheWeb,TeXworks}"; Filename: "http://texworks.org/"
Name: "{group}\{cm:UninstallProgram,TeXworks}"; Filename: "{uninstallexe}"
Name: "{commondesktop}\TeXworks"; Filename: "{app}\TeXworks.exe"; Tasks: desktopicon
Name: "{userappdata}\Microsoft\Internet Explorer\Quick Launch\TeXworks"; Filename: "{app}\TeXworks.exe"; Tasks: quicklaunchicon

[Registry]
Root: HKCR; Subkey: ".tex"; ValueType: string; ValueName: ""; ValueData: "TeXworksTeXFile"; Flags: uninsdeletevalue; Tasks: texfileassoc
Root: HKCR; Subkey: "TeXworksTeXFile"; ValueType: string; ValueName: ""; ValueData: "(La)TeX File"; Flags: uninsdeletekey; Tasks: texfileassoc
Root: HKCR; Subkey: "TeXworksTeXFile\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\TeXworks.exe,1"; Tasks: texfileassoc
Root: HKCR; Subkey: "TeXworksTeXFile\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\TeXworks.exe"" ""%1"""; Tasks: texfileassoc

Root: HKCR; Subkey: ".pdf"; ValueType: string; ValueName: ""; ValueData: "TeXworksPDFFile"; Flags: uninsdeletevalue; Tasks: pdffileassoc
Root: HKCR; Subkey: "TeXworksPDFFile"; ValueType: string; ValueName: ""; ValueData: "Portable Document Format"; Flags: uninsdeletekey; Tasks: pdffileassoc
Root: HKCR; Subkey: "TeXworksPDFFile\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\TeXworks.exe,1"; Tasks: pdffileassoc
Root: HKCR; Subkey: "TeXworksPDFFile\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\TeXworks.exe"" ""%1"""; Tasks: pdffileassoc

[Run]
Filename: "{app}\TeXworks.exe"; Description: "{cm:LaunchProgram,TeXworks}"; Flags: nowait postinstall skipifsilent
