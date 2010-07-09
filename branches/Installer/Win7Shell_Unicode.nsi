;###########################################################################################
;
; Windows 7 Taskbar Information Installer         
; Copyright (c) 2009-2010 by Magyari Attila
; Installer script: Copyright (c) 2009-2010 by Paweł Porwisz                                   
;
;###########################################################################################

; Definitions
	!define W7S_VERSION "1.13"
	!define W7S_ALT_VERSION "1_13"
	!define W7S_REVISION "0"
	!define W7S_BUILD "0"
	!define W7S_PLUGIN "Win7 Taskbar"
	!define W7S_PLUGIN_DESC "Windows 7 Taskbar Integration"
	!define W7S_PLUGIN_NAME "gen_win7shell"	
	!define W7S_UNINSTALLER "Uninst_Win7Shell"
	
;	!define W7S_PAYPAL_LINK ""	
	!define W7S_WEB_PAGE "http://code.google.com/p/win7shell/"
	!define W7S_HELP_LINK "http://code.google.com/p/win7shell/w/list"
	!define W7S_COMPANY "Magyari Attila"
	!define W7S_AUTHOR "Magyari Attila"	
	!define /Date W7S_COPYRIGHT "Copyright (c) %Y"	
	
;###########################################################################################
; CONFIGURATION
;###########################################################################################

	Name "${W7S_PLUGIN} v${W7S_VERSION}"
	OutFile "${W7S_PLUGIN}_v${W7S_ALT_VERSION}.exe"		
	SetCompressor /SOLID lzma
	Caption $(W7S_Caption)
	BrandingText $(W7S_Branding)

	InstType $(W7S_Full)
	InstType $(W7S_Minimal)
	
;###########################################################################################
; DESTINATION FOLDER
;###########################################################################################

	InstallDir "$PROGRAMFILES\Winamp"
	InstallDirRegKey "HKLM" "Software\Microsoft\Windows\CurrentVersion\Uninstall\Winamp" "UninstallString"
	RequestExecutionLevel "admin"
	XPStyle "on"

;###########################################################################################
; HEADER FILES
;###########################################################################################

	!include "MUI2.nsh"
	!include "Sections.nsh"
	!include "nsDialogs.nsh"
	!include "WinVer.nsh"
	!include "WordFunc.nsh"
	!include "FileFunc.nsh"	
	
;###########################################################################################
; INTERFACE SETTINGS 
;###########################################################################################

	!define MUI_LANGDLL_WINDOWTITLE $LANG_TITLE
	!define MUI_LANGDLL_INFO $(W7S_Language_Text)
	!define MUI_FINISHPAGE_TITLE_3LINES
	!define MUI_WELCOMEPAGE_TITLE_3LINES
	!define MUI_WELCOMEPAGE_TITLE $(W7S_Welcome_Title)
	!define MUI_WELCOMEPAGE_TEXT $(W7S_Welcome_Text)

	!define MUI_HEADERIMAGE_LEFT
	!define MUI_HEADERIMAGE
	!define MUI_HEADERIMAGE_BITMAP "Images\header.bmp"
	
	!define MUI_ABORTWARNING
	!define MUI_COMPONENTSPAGE_SMALLDESC
	!define MUI_ICON "Images\install.ico"
	!define MUI_COMPONENTSPAGE_CHECKBITMAP "${NSISDIR}\Contrib\Graphics\Checks\modern.bmp"
	!define MUI_WELCOMEFINISHPAGE_BITMAP "Images\welcome_finish.bmp"
	!define MUI_UNICON "Images\uninstall.ico"
	!define MUI_UNCOMPONENTSPAGE_CHECKBITMAP "${NSISDIR}\Contrib\Graphics\Checks\modern.bmp"
	!define MUI_UNWELCOMEFINISHPAGE_BITMAP "Images\welcome_finish.bmp"
	
; Installer pages	
	!insertmacro MUI_PAGE_WELCOME
	!insertmacro MUI_PAGE_LICENSE "Files\License\License.rtf"	
	!insertmacro MUI_PAGE_COMPONENTS
	!insertmacro MUI_PAGE_DIRECTORY
	!insertmacro MUI_PAGE_INSTFILES
	Page custom CreateFinishPage CheckFinishPage ""

; Uninstaller pages
	!define MUI_WELCOMEPAGE_TITLE_3LINES
	!define MUI_WELCOMEPAGE_TITLE $(W7S_Un_Welcome_Title)
	!define MUI_WELCOMEPAGE_TEXT $(W7S_Un_Welcome_Text)
	!insertmacro MUI_UNPAGE_WELCOME
	!insertmacro MUI_UNPAGE_CONFIRM	
	!insertmacro MUI_UNPAGE_INSTFILES
	!insertmacro MUI_UNPAGE_FINISH
	!define MUI_UNHEADERIMAGE
	!define MUI_UNHEADERIMAGE_BITMAP "Images\header.bmp"
	!define MUI_UNABORTWARNING
	!define MUI_UNCOMPONENTSPAGE_SMALLDESC	
	
;###########################################################################################
; INSTALLER  LANGUAGE
;###########################################################################################

	!define MUI_LANGDLL_ALLLANGUAGES
	!insertmacro MUI_RESERVEFILE_LANGDLL
	
; Language: English (1033) DEFAULT
	!insertmacro MUI_LANGUAGE "English" 		
	!include "Languages\W7S_en_us.nsh"
	
; Language: Czech (1029)
	!insertmacro MUI_LANGUAGE "Czech"			
	!include "Languages\W7S_cs_cz.nsh"
	
; Language: French (1036)	
	!insertmacro MUI_LANGUAGE "French"			
	!include "Languages\W7S_fr_fr.nsh"

; Language: German (1031)	
	!insertmacro MUI_LANGUAGE "German"			
	!include "Languages\W7S_de_de.nsh"

; Language: Polish (1045)
	!insertmacro MUI_LANGUAGE "Polish"			
	!include "Languages\W7S_pl_pl.nsh"	

; Language: Brazilian Portuguese (1046)
	!insertmacro MUI_LANGUAGE "PortugueseBR"		
	!include "Languages\W7S_pt_br.nsh"

; Language: Romanian (1048)	
	!insertmacro MUI_LANGUAGE "Romanian"		
	!include "Languages\W7S_ro_ro.nsh"

; Language: Russian (1049)	
	!insertmacro MUI_LANGUAGE "Russian"			
	!include "Languages\W7S_ru_ru.nsh"

; Language: Slovak (1051)	
	!insertmacro MUI_LANGUAGE "Slovak"			
	!include "Languages\W7S_sk_sk.nsh"

; Language: Slovenian (1060)
	!insertmacro MUI_LANGUAGE "Slovenian"			
	!include "Languages\W7S_sl_sl.nsh"
	
; Language: Spanish (1034)
	!insertmacro MUI_LANGUAGE "Spanish"			
	!include "Languages\W7S_es_us.nsh"

; Language: Swedish (1053)	
	!insertmacro MUI_LANGUAGE "Swedish"			
	!include "Languages\W7S_sv_se.nsh"

; Language: Turkish (1055)	
	!insertmacro MUI_LANGUAGE "Turkish"			
	!include "Languages\W7S_tr_tr.nsh"

; Language: Dutch (1043)	
	!insertmacro MUI_LANGUAGE "Dutch"			
	!include "Languages\W7S_nl_nl.nsh"
	
; Language: Italian (1040)	
	!insertmacro MUI_LANGUAGE "Italian"			
	!include "Languages\W7S_it_it.nsh"	
	
; Language: Greek (1032)	
	!insertmacro MUI_LANGUAGE "Greek"			
	!include "Languages\W7S_gr_gr.nsh"		

; Language: Ukrainian (1058)	
	!insertmacro MUI_LANGUAGE "Ukrainian"			
	!include "Languages\W7S_ua_ua.nsh"

; Language: Chinese Traditional (1028)	
	!insertmacro MUI_LANGUAGE "TradChinese"			
	!include "Languages\W7S_zh_HanT.nsh"

; Language: Chinese Simplified (2052)	
	!insertmacro MUI_LANGUAGE "SimpChinese"			
	!include "Languages\W7S_zh_HanS.nsh"	
	
; Language: Hungarian (1038)	
	!insertmacro MUI_LANGUAGE "Hungarian"			
	!include "Languages\W7S_hu_hu.nsh"	
	
; Language: Danish (1030)	
	!insertmacro MUI_LANGUAGE "Danish"			
	!include "Languages\W7S_da_dk.nsh"		
	
; Language: Bulgarian (1026)	
	!insertmacro MUI_LANGUAGE "Bulgarian"			
	!include "Languages\W7S_bg_bg.nsh"			

;###########################################################################################
; VERSION INFORMATION
;###########################################################################################

	VIProductVersion "${W7S_VERSION}.${W7S_REVISION}.${W7S_BUILD}"
	VIAddVersionKey "ProductName" "${W7S_PLUGIN}"
	VIAddVersionKey "ProductVersion" "${W7S_VERSION}"		
	VIAddVersionKey "Comments" "${W7S_PLUGIN} v${W7S_VERSION}, ${W7S_WEB_PAGE}"
	VIAddVersionKey "CompanyName" "${W7S_COMPANY}"
	VIAddVersionKey "LegalCopyright" "${W7S_COPYRIGHT}, ${W7S_AUTHOR}"
	VIAddVersionKey "FileDescription" "${W7S_PLUGIN_DESC} v${W7S_VERSION}"
	VIAddVersionKey "FileVersion" "${W7S_VERSION}"		

	
;###########################################################################################
; WINAMP INI PATH DETECTION
;###########################################################################################
	
	Var /Global WINAMP_INI_DIR

!macro WinampINIPath UN_PREFIX

	Function ${UN_PREFIX}GetWinampIniPath

	StrCpy $WINAMP_INI_DIR $INSTDIR
	ClearErrors

	${If} ${FileExists} "$INSTDIR\paths.ini"
		ReadINIStr $0 "$INSTDIR\paths.ini" "Winamp" "inidir" ; Read paths.ini file - search for path to winamp.ini (inidir={26}\Winamp)
		${If} $0 != "" ; if entry inidir= exists
			${WordFind2X} $0 "{" "}" "E+1" $2 ;get CSIDL number (dec)
			${If} ${Errors} ;check error
				${IfNot} ${FileExists} "$0\*.*" ; There is no {CSIDL} in path. Check the path... (it can be normal path, or with %SPECIALFOLDER%)
					${WordFind2X} $0 "%" "%" "E+1" $2 ; for example: %AppData%

					${If} $2 == "WINAMP_ROOT_DIR"
						ClearErrors
						${GetRoot} "$WINAMP_INI_DIR" $3 ; default c:\, root dir of winamp installation
						${WordReplace} "$0" "%$2%" "$3" "E+1" $R0 ;Replace %WINAMP_ROOT_DIR% in path to WINAMP Installation disk ($3)
						${If} ${Errors} ;check error
							Return ;if error, return
						${Else} ;path is winamp installation disk + dir
							StrCpy $WINAMP_INI_DIR $R0
							DetailPrint "$(W7S_Account): $WINAMP_INI_DIR"
						${EndIf}

					${ElseIf} $2 == "WINAMP_PROGRAM_DIR"
						ClearErrors
						${WordReplace} "$0" "%$2%" "$WINAMP_INI_DIR" "E+1" $R0 ;Replace %WINAMP_PROGRAM_DIR% in path to WINAMP Installation DIR ($WINAMP_INI_DIR)
						${If} ${Errors} ;check error
							Return ;if error, return
						${Else} ;path is winamp installation dir
							StrCpy $WINAMP_INI_DIR $R0
							DetailPrint "$(W7S_Account): $WINAMP_INI_DIR"
						${EndIf}
					${Else}
						ClearErrors
						ReadEnvStr $R0 "$2" ;check any %SpecialDir% var
						${If} $R0 != ""
							${WordReplace} "$0" "%$2%" "$R0" "E+1" $R0 ;Replace %WINAMP_PROGRAM_DIR% in path to WINAMP Installation DIR ($WINAMP_INI_DIR)
							${If} ${Errors} ;check error
								Return ;if error, return
							${Else} ;get special folder path
								StrCpy $WINAMP_INI_DIR $R0
								DetailPrint "$(W7S_Account): $WINAMP_INI_DIR"
							${EndIf}
						${Else}
							Return ;if error, return (So, the Path is default - $INSTDIR)
						${EndIf}
					${EndIf}

				${Else} ;normal path, use it
					StrCpy $WINAMP_INI_DIR $0
					DetailPrint "$WINAMP_INI_DIR"
				${EndIf}
			${Else} ;No error, get special folder path
				System::Call "shell32::SHGetSpecialFolderPath(i $HWNDPARENT, t .r4, i $2, i0) i .r3"
				ClearErrors
				${WordReplace} "$0" "{$2}" "$4" "E+1" $R0 ;Replace {ID} in path to folder path
				${If} ${Errors} ;check error
					Return ;if error, return
				${Else} ;get special folder path
					StrCpy $WINAMP_INI_DIR $R0
					DetailPrint "$(W7S_Account): $WINAMP_INI_DIR"
				${EndIf}
			${EndIf}
		${Else} ; Entry (inidir=) in Paths.ini doeasnt exists
			DetailPrint "$(W7S_No_Account): $WINAMP_INI_DIR"
			Return
		${EndIf}
	${Else} ; File paths.ini doeasnt exists
		DetailPrint "$(W7S_No_Account): $WINAMP_INI_DIR"
		Return
	${EndIf}

FunctionEnd

!macroend

	!insertmacro WinampINIPath ""
	!insertmacro WinampINIPath "un."

	
;###########################################################################################
; CLOSING ALL WINAMP INSTANCES
;###########################################################################################
	
!macro SharedWinamp un
	
	Function ${un}CloseWinamp

		Push $5
 
		FindWindow $5 "Winamp v1.x"
		IntCmp $5 0 NoWinamp
		DetailPrint "$(W7S_Running_Winamp)"
		MessageBox MB_YESNO|MB_ICONEXCLAMATION "$(W7S_Close_Winamp)" IDYES KillWinamp IDNO Running_NoKill
	
		KillWinamp:
			FindWindow $5 "Winamp v1.x"
			IntCmp $5 0 NoMoreWinampToKill
			DetailPrint "$(W7S_Closing_Winamp)"
			SendMessage $5 16 0 0
			Sleep 100
			Goto KillWinamp
	
		NoMoreWinampToKill:
			DetailPrint "$(W7S_No_More_Winamp)"
			Goto Running_NoKill
	
		NoWinamp:	
			DetailPrint "$(W7S_No_Winamp)"	
	
		Running_NoKill:	
  
		Pop $5
 
	FunctionEnd

!macroend

; Insert function as an installer and uninstaller function.
	!insertmacro SharedWinamp ""
	!insertmacro SharedWinamp "un."	

	
Function .onVerifyInstDir

	${If} ${FileExists} "$INSTDIR\Winamp.exe"
		Return
	${Else}
		Abort
	${EndIf}

FunctionEnd


	ShowInstDetails "show"

;###########################################################################################
; INSTALLER  SECTIONS 
;###########################################################################################

Section "-Pre"

	DetailPrint "$(W7S_Winamp_Path)"
		Call GetWinampIniPath
		
	DetailPrint "$(W7S_Check_Winamp)"
		Call CloseWinamp
		Sleep 500	
	
SectionEnd


Section "!$(W7S_PluginFile)" "W7S_Sec_PluginFile"

	SectionIn 1 2 RO
	
	SetOutPath $INSTDIR\Plugins
		File "Files\Plugin\${W7S_PLUGIN_NAME}.dll"
 
SectionEnd


Section "$(W7S_Language)" "W7S_Sec_Language"

	SectionIn 1
	SetOutPath $WINAMP_INI_DIR\Plugins		
	
	${Select} $Language
			
		${Case} "1029" ; Czech (1029)
			File "Files\Languages\CZECH_win7shell.lng"			
		${Case} "1036" ; French (1036)
			File "Files\Languages\FRENCH_win7shell.lng"
		${Case} "1031" ; German (1031)
			File "Files\Languages\GERMAN_win7shell.lng"
		${Case} "1045" ; Polish (1045) 
			File "Files\Languages\POLISH_win7shell.lng"
		${Case} "1046" ; Brazilian Portuguese (1046)
			File "Files\Languages\PT_BR_win7shell.lng"
		${Case} "1048" ; Romanian (1048)
			File "Files\Languages\ROMANIAN_win7shell.lng"		
		${Case} "1049" ; Russian (1049)
			File "Files\Languages\RUSSIAN_win7shell.lng"
		${Case} "1051" ; Slovak (1051)
			File "Files\Languages\SLOVAK_win7shell.lng"
		${Case} "1060" ; Slovenian (1060)
			File "Files\Languages\SLOVENIAN_win7shell.lng"		
		${Case} "1034" ; Spanish (1034)
			File "Files\Languages\SPANISH_win7shell.lng"
		${Case} "1053" ; Swedish (1053)
			File "Files\Languages\SWEDISH_win7shell.lng"
		${Case} "1055" ; Turkish (1055)
			File "Files\Languages\TURKISH_win7shell.lng"
		${Case} "1043" ; Dutch (1043)
			File "Files\Languages\DUTCH_win7shell.lng"
		${Case} "1040" ; Italian (1040)
			File "Files\Languages\ITALIAN_win7shell.lng"
		${Case} "1032" ; Greek (1032)
			File "Files\Languages\GREEK_win7shell.lng"
		${Case} "1058" ; Ukrainian (1058)
			File "Files\Languages\UKR_win7shell.lng"
		${Case} "1028" ; Chinese Traditional (1028)
			File "Files\Languages\zh_HanT_win7shell.lng"
		${Case} "2052" ; Chinese Simplified (2052)
			File "Files\Languages\zh_HanS_win7shell.lng"
		${Case} "1038" ; Hungarian (1038)
			File "Files\Languages\HUN_win7shell.lng"
		${Case} "1030" ; Danish (1030)
			File "Files\Languages\DANISH_win7shell.lng"
		${Case} "1026" ; Bulgarian (1026)
			File "Files\Languages\BULGARIAN_win7shell.lng"	
		${CaseElse} ;  English
			Return
	${EndSelect}
	
SectionEnd


Section "-Leave"

; Registry entries
	WriteRegStr "HKLM" "Software\Microsoft\Windows\CurrentVersion\Uninstall\${W7S_PLUGIN}" "UninstallString" '"$INSTDIR\${W7S_UNINSTALLER}.exe"'
	WriteRegStr "HKLM" "Software\Microsoft\Windows\CurrentVersion\Uninstall\${W7S_PLUGIN}" "InstallLocation" "$INSTDIR"
	WriteRegStr "HKLM" "Software\Microsoft\Windows\CurrentVersion\Uninstall\${W7S_PLUGIN}" "DisplayName" "${W7S_PLUGIN} v${W7S_VERSION}"
	WriteRegStr "HKLM" "Software\Microsoft\Windows\CurrentVersion\Uninstall\${W7S_PLUGIN}" "DisplayIcon" "$INSTDIR\Winamp.exe,0"
	WriteRegStr "HKLM" "Software\Microsoft\Windows\CurrentVersion\Uninstall\${W7S_PLUGIN}" "DisplayVersion" "${W7S_VERSION}"
	WriteRegStr "HKLM" "Software\Microsoft\Windows\CurrentVersion\Uninstall\${W7S_PLUGIN}" "URLInfoAbout" "${W7S_WEB_PAGE}"
	WriteRegStr "HKLM" "Software\Microsoft\Windows\CurrentVersion\Uninstall\${W7S_PLUGIN}" "HelpLink" "${W7S_HELP_LINK}" 
	WriteRegStr "HKLM" "Software\Microsoft\Windows\CurrentVersion\Uninstall\${W7S_PLUGIN}" "Publisher" "${W7S_COMPANY}"
	WriteRegDWORD "HKLM" "Software\Microsoft\Windows\CurrentVersion\Uninstall\${W7S_PLUGIN}" "NoModify" "1"
	WriteRegDWORD "HKLM" "Software\Microsoft\Windows\CurrentVersion\Uninstall\${W7S_PLUGIN}" "NoRepair" "1"

; Create uninstaller
	WriteUninstaller "$INSTDIR\${W7S_UNINSTALLER}.exe"

	SetAutoClose false
	
SectionEnd


Function .onInit

; Language
	Var /GLOBAL LANG_TITLE
		StrCpy $LANG_TITLE  $(W7S_Language_Title)
	!insertmacro MUI_LANGDLL_DISPLAY
	
	InitPluginsDir
	
	Var /Global Dialog
	
; Finish Page Variables
	Var /Global Label1
	Var /Global Label1_Font
	Var /Global Label2
	Var /Global Label4	
;	Var /Global CheckBox1
	Var /Global CheckBox2	
	Var /Global Control_State	
	Var /GLOBAL Img_Left		
	Var /GLOBAL Img_Handle_Left
	File /oname=$PLUGINSDIR\Img_Left.bmp "Images\welcome_finish.bmp"

; Variables for PayPal button	
;	Var /Global Label3
;	Var /GLOBAL Button	
;	File /oname=$PLUGINSDIR\PayPal.bmp "Images\donate.bmp"

	${If} ${IsWin7} ;${AtLeastWin7}
		${If} $Language == "1033"
			SectionSetText ${W7S_Sec_Language} ""
		${EndIf}	
			; OK, Windows 7
	${Else}
		MessageBox MB_YESNO $(W7S_Win7_Warning) IDYES YES IDNO NO ; other system - ask what to do...
			NO:
				Abort ; Abort installation
			YES:
				Return ; Continue installation
	${EndIf}

FunctionEnd


Function CreateFinishPage

    LockWindow on
    GetDlgItem $0 $HWNDPARENT 1028
    ShowWindow $0 ${SW_HIDE}
    GetDlgItem $0 $HWNDPARENT 1256
    ShowWindow $0 ${SW_HIDE}
    GetDlgItem $0 $HWNDPARENT 1045
    ShowWindow $0 ${SW_NORMAL}
    LockWindow off

    nsDialogs::Create /NOUNLOAD 1044
	Pop $Dialog
	
	${If} $Dialog == error
		Abort
	${EndIf}
	
    SetCtlColors $Dialog "" "0xFFFFFF"

	${NSD_CreateBitmap} 0u 0u 109u 193u ""
	Pop $Img_Left
	${NSD_SetImage} $Img_Left $PLUGINSDIR\Img_Left.bmp $Img_Handle_Left
	
	${NSD_CreateLabel} 115u 20u 63% 30u "$(W7S_FinishPage_1)"
	Pop $Label1
	${NSD_AddStyle} $Label1 ${WS_VISIBLE}|${WS_CHILD}|${WS_CLIPSIBLINGS}
	CreateFont $Label1_Font "TAHOMA" "13" "700"
	SendMessage $Label1 ${WM_SETFONT} $Label1_Font 0	
    SetCtlColors $Label1 "0x000000" "TRANSPARENT"
	
	${NSD_CreateLabel} 115u 60u 63% 30u "$(W7S_FinishPage_2)"
	Pop $Label2
	${NSD_AddStyle} $Label2 ${WS_VISIBLE}|${WS_CHILD}|${WS_CLIPSIBLINGS}
    SetCtlColors $Label2 "0x000000" "TRANSPARENT"	

/*	
	${NSD_CreateButton} 115u 95u 58 35 ""
	Pop $Button
	${NSD_AddStyle} $Button "${BS_BITMAP}" 
	System::Call 'user32::LoadImage(i 0, t "$PLUGINSDIR\PayPal.bmp", i ${IMAGE_BITMAP}, i 0, i 0, i ${LR_CREATEDIBSECTION}|${LR_LOADFROMFILE}) i.s' 
	Pop $1 
	SendMessage $Button ${BM_SETIMAGE} ${IMAGE_BITMAP} $1
	${NSD_OnClick} $Button Button_Click		

	${NSD_CreateLabel} 160u 95u 50% 30u "$(W7S_FinishPage_3)"
	Pop $Label3
	${NSD_AddStyle} $Label3 ${WS_VISIBLE}|${WS_CHILD}|${WS_CLIPSIBLINGS}
    SetCtlColors $Label3 "0x000000" "TRANSPARENT"
*/
	${NSD_CreateLabel} 120u 148u 63% 10u "$(W7S_FinishPage_4)"
	Pop $Label4
	${NSD_AddStyle} $Label4 ${WS_VISIBLE}|${WS_CHILD}|${WS_CLIPSIBLINGS}
    SetCtlColors $Label4 "0x000000" "TRANSPARENT"	
/*
	${NSD_CreateCheckBox} 125u 143u 63% 16u "$(W7S_FinishPage_6)"
	Pop $CheckBox1
	;${NSD_Check} $CheckBox1
	SetCtlColors $CheckBox1 "0x000000" "0xFFFFFF"
*/	
	${NSD_CreateCheckBox} 125u 159u 63% 16u "$(W7S_FinishPage_5)"
	Pop $CheckBox2	
	${NSD_Check} $CheckBox2		
    SetCtlColors $CheckBox2 "0x000000" "0xFFFFFF"	


	GetDlgItem $R0 $HWNDPARENT 1
	SendMessage $R0 ${WM_SETTEXT} 0 "STR:$(W7S_FinishPage_7)"
	
	nsDialogs::Show
	${NSD_FreeImage} $Img_Handle_Left

FunctionEnd

/*
Function Button_Click

	Pop $0
	ExecShell "open" "${W7S_PayPal_Link}"

FunctionEnd
*/

Function CheckFinishPage
/*
; Run Winamp	
	${NSD_GetState} $CheckBox1 $Control_State
	${If} $Control_State = ${BST_CHECKED}
		ExecShell "open" "$INSTDIR\winamp.exe"
	${EndIf}
*/
	${NSD_GetState} $CheckBox2 $Control_State
	${If} $Control_State = ${BST_CHECKED}
		ExecShell "open" "${W7S_WEB_PAGE}"
	${EndIf}
	
FunctionEnd


;###########################################################################################
; INSTALLER DESCRIPTIONS SECTION 
;###########################################################################################

	!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
		!insertmacro MUI_DESCRIPTION_TEXT ${W7S_Sec_PluginFile} $(W7S_Desc_PluginFile)
		!insertmacro MUI_DESCRIPTION_TEXT ${W7S_Sec_Language} $(W7S_Desc_Language)
	!insertmacro MUI_FUNCTION_DESCRIPTION_END
	
	
;###########################################################################################
; UNINSTALLER
;###########################################################################################

	ShowUninstDetails "show"

Function un.onInit

; Language
	StrCpy $LANG_TITLE $(W7S_Un_Language_Title)
	!insertmacro MUI_UNGETLANGUAGE
	InitPluginsDir

FunctionEnd


Section "-Un.Pre"

	DetailPrint "$(W7S_Winamp_Path)"
		Call un.GetWinampIniPath
		
	DetailPrint "$(W7S_Check_Winamp)"
		Call un.CloseWinamp
		Sleep 500
		
SectionEnd


Section "-Un.Uninstall"

	Delete "$INSTDIR\Plugins\${W7S_PLUGIN_NAME}.dll"
; Czech	
	${If} ${FileExists} "$WINAMP_INI_DIR\Plugins\CZECH_win7shell.lng"
		Delete "$WINAMP_INI_DIR\Plugins\CZECH_win7shell.lng"
	${EndIf}	
; French
	${If} ${FileExists} "$WINAMP_INI_DIR\Plugins\FRENCH_win7shell.lng"
		Delete "$WINAMP_INI_DIR\Plugins\FRENCH_win7shell.lng"
	${EndIf}		
; German		
	${If} ${FileExists} "$WINAMP_INI_DIR\Plugins\GERMAN_win7shell.lng"
		Delete "$WINAMP_INI_DIR\Plugins\GERMAN_win7shell.lng"
	${EndIf}		
;Polish
	${If} ${FileExists} "$WINAMP_INI_DIR\Plugins\POLISH_win7shell.lng"
		Delete "$WINAMP_INI_DIR\Plugins\POLISH_win7shell.lng"
	${EndIf}		
; Portuguese-BR
	${If} ${FileExists} "$WINAMP_INI_DIR\Plugins\PT_BR_win7shell.lng"
		Delete "$WINAMP_INI_DIR\Plugins\PT_BR_win7shell.lng"
	${EndIf}		
; Romanian
	${If} ${FileExists} "$WINAMP_INI_DIR\Plugins\ROMANIAN_win7shell.lng"
		Delete "$WINAMP_INI_DIR\Plugins\ROMANIAN_win7shell.lng"
	${EndIf}		
; Russian
	${If} ${FileExists} "$WINAMP_INI_DIR\Plugins\RUSSIAN_win7shell.lng"
		Delete "$WINAMP_INI_DIR\Plugins\RUSSIAN_win7shell.lng"
	${EndIf}
; Slovak
	${If} ${FileExists} "$WINAMP_INI_DIR\Plugins\SLOVAK_win7shell.lng"
		Delete "$WINAMP_INI_DIR\Plugins\SLOVAK_win7shell.lng"
	${EndIf}		
; Slovenian
	${If} ${FileExists} "$WINAMP_INI_DIR\Plugins\SLOVENIAN_win7shell.lng"
		Delete "$WINAMP_INI_DIR\Plugins\SLOVENIAN_win7shell.lng"
	${EndIf}		
; Spanish
	${If} ${FileExists} "$WINAMP_INI_DIR\Plugins\SPANISH_win7shell.lng"
		Delete "$WINAMP_INI_DIR\Plugins\SPANISH_win7shell.lng"
	${EndIf}		
; Swedish
	${If} ${FileExists} "$WINAMP_INI_DIR\Plugins\SWEDISH_win7shell.lng"
		Delete "$WINAMP_INI_DIR\Plugins\SWEDISH_win7shell.lng"
	${EndIf}		
; Turkish
	${If} ${FileExists} "$WINAMP_INI_DIR\Plugins\TURKISH_win7shell.lng"
		Delete "$WINAMP_INI_DIR\Plugins\TURKISH_win7shell.lng"		
	${EndIf}		
		
; Dutch
	${If} ${FileExists} "$WINAMP_INI_DIR\Plugins\DUTCH_win7shell.lng"
		Delete "$WINAMP_INI_DIR\Plugins\DUTCH_win7shell.lng"		
	${EndIf}
			
; Italian
	${If} ${FileExists} "$WINAMP_INI_DIR\Plugins\ITALIAN_win7shell.lng"
		Delete "$WINAMP_INI_DIR\Plugins\ITALIAN_win7shell.lng"		
	${EndIf}
			
; Greek
	${If} ${FileExists} "$WINAMP_INI_DIR\Plugins\GREEK_win7shell.lng"
		Delete "$WINAMP_INI_DIR\Plugins\GREEK_win7shell.lng"		
	${EndIf}
	
; Ukrainian
	${If} ${FileExists} "$WINAMP_INI_DIR\Plugins\UKR_win7shell.lng"
		Delete "$WINAMP_INI_DIR\Plugins\UKR_win7shell.lng"		
	${EndIf}	
	
; Trad Chinese
	${If} ${FileExists} "$WINAMP_INI_DIR\Plugins\zh_HanT_win7shell.lng"
		Delete "$WINAMP_INI_DIR\Plugins\zh_HanT_win7shell.lng"		
	${EndIf}
	
; Simp Chinese
	${If} ${FileExists} "$WINAMP_INI_DIR\Plugins\zh_HanS_win7shell.lng"
		Delete "$WINAMP_INI_DIR\Plugins\zh_HanS_win7shell.lng"		
	${EndIf}	
	
; Hungarian
	${If} ${FileExists} "$WINAMP_INI_DIR\Plugins\HUN_win7shell.lng"
		Delete "$WINAMP_INI_DIR\Plugins\HUN_win7shell.lng"		
	${EndIf}		
	
; Danish
	${If} ${FileExists} "$WINAMP_INI_DIR\Plugins\DANISH_win7shell.lng"
		Delete "$WINAMP_INI_DIR\Plugins\HUN_win7shell.lng"		
	${EndIf}
	
; Bulgarian
	${If} ${FileExists} "$WINAMP_INI_DIR\Plugins\BULGARIAN_win7shell.lng"
		Delete "$WINAMP_INI_DIR\Plugins\BULGARIAN_win7shell.lng"		
	${EndIf}	
	
	
	DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${W7S_PLUGIN}"	
	Delete /REBOOTOK "$INSTDIR\${W7S_UNINSTALLER}.exe"
	SetAutoClose false
	
SectionEnd


;###########################################################################################
; THE END
;###########################################################################################