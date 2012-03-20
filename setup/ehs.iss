;
; $Id: opennx.iss 643 2011-06-23 12:07:58Z felfert $
;
#undef DEBUG

#define APPNAME "EHS"
; Automatically get version from executable resp. dll
#define APPEXE "setupdir\bin\libehs-1.dll"
#include "version.iss"

#define APPIDSTR "{56D797D7-543C-408F-BBEB-B56787873D2F}"
#define APPIDVAL "{" + APPIDSTR

[Setup]
AppName={#=APPNAME}
AppVersion={#=APPFULLVER}
AppVerName={#=APPFULLVERNAME}
AppPublisher=Fritz Elfert
AppPublisherURL=http://opennx.net
AppCopyright=(C) 2011 Fritz Elfert
VersionInfoVersion={#=APPFULLVER}
DefaultDirName={pf}\{#=APPNAME}
DefaultGroupName={#=APPNAME}
#ifdef DEBUG
PrivilegesRequired=none
#endif
DisableStartupPrompt=true
OutputDir=.
OutputBaseFileName={#=SETUPFVNAME}
ShowLanguageDialog=no
MinVersion=0,5.0.2195sp3
AppID={#=APPIDVAL}
UninstallFilesDir={app}\uninstall
Compression=lzma/ultra64
SolidCompression=yes
SetupLogging=yes
WizardImageFile=compiler:wizmodernimage-IS.bmp
WizardSmallImageFile=compiler:wizmodernsmallimage-IS.bmp
; The following breaks in older wine versions, so we
; check the wine version in the invoking script and
; define BADWINE, if we are crossbuilding and have a
; broken wine version.
#ifndef BADWINE
;SetupIconFile=setupdir\bin\nx.ico
#endif
;UninstallDisplayIcon={app}\bin\opennx.exe
;LicenseFile=lgpl.rtf

[Languages]
Name: "en"; MessagesFile: "compiler:Default.isl"
Name: "de"; MessagesFile: "compiler:Languages\German.isl"
Name: "es"; MessagesFile: "compiler:Languages\Spanish.isl"
Name: "fr"; MessagesFile: "compiler:Languages\French.isl"
Name: "ru"; MessagesFile: "compiler:Languages\Russian.isl"

[CustomMessages]
uninst_ehs=Uninstall EHS
doc=Documentation

de.uninst_ehs=Deinstalliere EHS
de.doc=Dokumentation

[Files]
Source: setupdir\*; DestDir: {app}; Flags: recursesubdirs replacesameversion

[Icons]
Name: "{group}\{cm:doc}"; Filename: "{app}\doc\index.html";
Name: "{group}\{cm:uninst_ehs}"; Filename: "{uninstallexe}";
