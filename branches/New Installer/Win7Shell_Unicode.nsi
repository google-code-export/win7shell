;###########################################################################################
;
; Windows 7 Taskbar Information Installer         
; Copyright (c) 2009-2010 by Magyari Attila
; Copyright (c) 2009-2010 by Darren Owen aka DrO
; Installer script: Copyright (c) 2009-2010 by Paweł Porwisz aka Pepe                                   
;
;###########################################################################################

; Definitions
	!define W7S_VERSION "1.14"
	!define W7S_ALT_VERSION "1_14"
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

	Name "${W7S_PLUGIN_DESC} v${W7S_VERSION}"
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
	!include "Languages\Win7shell-en-us.nsh"
	
; Language: Czech (1029)
	!insertmacro MUI_LANGUAGE "Czech"			
	!include "Languages\Win7shell-cs-cz.nsh"
	
; Language: French (1036)	
	!insertmacro MUI_LANGUAGE "French"			
	!include "Languages\Win7shell-fr-fr.nsh"

; Language: German (1031)	
	!insertmacro MUI_LANGUAGE "German"			
	!include "Languages\Win7shell-de-de.nsh"

; Language: Polish (1045)
	!insertmacro MUI_LANGUAGE "Polish"			
	!include "Languages\Win7shell-pl-pl.nsh"	

; Language: Brazilian Portuguese (1046)
	!insertmacro MUI_LANGUAGE "PortugueseBR"		
	!include "Languages\Win7shell-pt-br.nsh"

; Language: Romanian (1048)	
	!insertmacro MUI_LANGUAGE "Romanian"		
	!include "Languages\Win7shell-ro-ro.nsh"

; Language: Russian (1049)	
	!insertmacro MUI_LANGUAGE "Russian"			
	!include "Languages\Win7shell-ru-ru.nsh"

; Language: Slovak (1051)	
	!insertmacro MUI_LANGUAGE "Slovak"			
	!include "Languages\Win7shell-sk-sk.nsh"

; Language: Slovenian (1060)
	!insertmacro MUI_LANGUAGE "Slovenian"			
	!include "Languages\Win7shell-sl-sl.nsh"
	
; Language: Spanish (1034)
	!insertmacro MUI_LANGUAGE "Spanish"			
	!include "Languages\Win7shell-es-us.nsh"

; Language: Swedish (1053)	
	!insertmacro MUI_LANGUAGE "Swedish"			
	!include "Languages\Win7shell-sv-se.nsh"

; Language: Turkish (1055)	
	!insertmacro MUI_LANGUAGE "Turkish"			
	!include "Languages\Win7shell-tr-tr.nsh"

; Language: Dutch (1043)	
	!insertmacro MUI_LANGUAGE "Dutch"			
	!include "Languages\Win7shell-nl-nl.nsh"
	
; Language: Italian (1040)	
	!insertmacro MUI_LANGUAGE "Italian"			
	!include "Languages\Win7shell-it-it.nsh"	
	
; Language: Greek (1032)	
	!insertmacro MUI_LANGUAGE "Greek"			
	!include "Languages\Win7shell-gr-gr.nsh"		

; Language: Ukrainian (1058)	
	!insertmacro MUI_LANGUAGE "Ukrainian"			
	!include "Languages\Win7shell-ua-ua.nsh"

; Language: Chinese Traditional (1028)	
	!insertmacro MUI_LANGUAGE "TradChinese"			
	!include "Languages\Win7shell-zh-HanT.nsh"

; Language: Chinese Simplified (2052)	
	!insertmacro MUI_LANGUAGE "SimpChinese"			
	!include "Languages\Win7shell-zh-HanS.nsh"	
	
; Language: Hungarian (1038)	
	!insertmacro MUI_LANGUAGE "Hungarian"			
	!include "Languages\Win7shell-hu-hu.nsh"	
	
; Language: Danish (1030)	
	!insertmacro MUI_LANGUAGE "Danish"			
	!include "Languages\Win7shell-da-dk.nsh"		
	
; Language: Bulgarian (1026)	
	!insertmacro MUI_LANGUAGE "Bulgarian"			
	!include "Languages\Win7shell-bg-bg.nsh"			
	
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
	
	${Select} $Language

		${Case} "1029" ; Czech (1029)
			SetOutPath $INSTDIR\Lang\cs-cz
			File "Files\Languages\Czech\gen_win7shell.lng"		
		${Case} "1036" ; French (1036)
			SetOutPath $INSTDIR\Lang\fr-fr		
			File "Files\Languages\French\gen_win7shell.lng"
		${Case} "1031" ; German (1031)
			SetOutPath $INSTDIR\Lang\de-de		
			File "Files\Languages\German\gen_win7shell.lng"
		${Case} "1045" ; Polish (1045) 
			SetOutPath $INSTDIR\Lang\pl-pl		
			File "Files\Languages\Polish\gen_win7shell.lng"
		${Case} "1046" ; Brazilian Portuguese (1046)
			SetOutPath $INSTDIR\Lang\pt-br		
			File "Files\Languages\PortugueseBR\gen_win7shell.lng"
		${Case} "1048" ; Romanian (1048)
			SetOutPath $INSTDIR\Lang\ro-ro		
			File "Files\Languages\Romanian\gen_win7shell.lng"		
		${Case} "1049" ; Russian (1049)
			SetOutPath $INSTDIR\Lang\ru-ru		
			File "Files\Languages\Russian\gen_win7shell.lng"
		${Case} "1051" ; Slovak (1051)
			SetOutPath $INSTDIR\Lang\sk-sk		
			File "Files\Languages\Slovak\gen_win7shell.lng"
		${Case} "1060" ; Slovenian (1060)
			SetOutPath $INSTDIR\Lang\sl-si		
			File "Files\Languages\Slovenian\gen_win7shell.lng"		
		${Case} "1034" ; Spanish (1034)
			SetOutPath $INSTDIR\Lang\es-us		
			File "Files\Languages\Spanish\gen_win7shell.lng"
		${Case} "1053" ; Swedish (1053)
			SetOutPath $INSTDIR\Lang\sv-se		
			File "Files\Languages\Swedish\gen_win7shell.lng"
		${Case} "1055" ; Turkish (1055)
			SetOutPath $INSTDIR\Lang\tr-tr		
			File "Files\Languages\Turkish\gen_win7shell.lng"
		${Case} "1043" ; Dutch (1043)
			SetOutPath $INSTDIR\Lang\nl-nl		
			File "Files\Languages\Dutch\gen_win7shell.lng"
		${Case} "1040" ; Italian (1040)
			SetOutPath $INSTDIR\Lang\it-it		
			File "Files\Languages\Italian\gen_win7shell.lng"
		${Case} "1032" ; Greek (1032)
			SetOutPath $INSTDIR\Lang\el-gr		
			File "Files\Languages\Greek\gen_win7shell.lng"
		${Case} "1058" ; Ukrainian (1058)
			SetOutPath $INSTDIR\Lang\ua-ua		
			File "Files\Languages\Ukrainian\gen_win7shell.lng"
		${Case} "1028" ; Chinese Traditional (1028)
			SetOutPath $INSTDIR\Lang\zh-tw		
			File "Files\Languages\TradChinese\gen_win7shell.lng"
		${Case} "2052" ; Chinese Simplified (2052)
			SetOutPath $INSTDIR\Lang\zh-cn		
			File "Files\Languages\SimpChinese\gen_win7shell.lng"
		${Case} "1038" ; Hungarian (1038)
			SetOutPath $INSTDIR\Lang\hu-hu
			File "Files\Languages\Hungarian\gen_win7shell.lng"
		${Case} "1030" ; Danish (1030)
			SetOutPath $INSTDIR\Lang\da-dk
			File "Files\Languages\Danish\gen_win7shell.lng"
		${Case} "1026" ; Bulgarian (1026)
			SetOutPath $INSTDIR\Lang\bg-bl
			File "Files\Languages\Bulgarian\gen_win7shell.lng"	
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

	${Select} $Language

		${Case} "1029" ; Czech (1029)
			${If} ${FileExists} "$INSTDIR\Lang\cs-cz\gen_win7shell.lng"
				Delete "$INSTDIR\Lang\cs-cz\gen_win7shell.lng"
				RMDir "$INSTDIR\Lang\cs-cz"
			${EndIf}		
		${Case} "1036" ; French (1036)
			${If} ${FileExists} "$INSTDIR\Lang\fr-fr\gen_win7shell.lng"
				Delete "$INSTDIR\Lang\fr-fr\gen_win7shell.lng"
				RMDir "$INSTDIR\Lang\fr-fr"		
			${EndIf}
		${Case} "1031" ; German (1031)
			${If} ${FileExists} "$INSTDIR\Lang\de-de\gen_win7shell.lng"
				Delete "$INSTDIR\Lang\de-de\gen_win7shell.lng"
				RMDir "$INSTDIR\Lang\de-de"		
			${EndIf}	
		${Case} "1045" ; Polish (1045) 
			${If} ${FileExists} "$INSTDIR\Lang\pl-pl\gen_win7shell.lng"
				Delete "$INSTDIR\Lang\pl-pl\gen_win7shell.lng"
				RMDir "$INSTDIR\Lang\pl-pl"		
			${EndIf}
		${Case} "1046" ; Brazilian Portuguese (1046)
			${If} ${FileExists} "$INSTDIR\Lang\pt-br\gen_win7shell.lng"
				Delete "$INSTDIR\Lang\pt-br\gen_win7shell.lng"
				RMDir "$INSTDIR\Lang\pt-br"		
			${EndIf}
		${Case} "1048" ; Romanian (1048)
			${If} ${FileExists} "$INSTDIR\Lang\ro-ro\gen_win7shell.lng"
				Delete "$INSTDIR\Lang\ro-ro\gen_win7shell.lng"
				RMDir "$INSTDIR\Lang\ro-ro"		
			${EndIf}		
		${Case} "1049" ; Russian (1049)
			${If} ${FileExists} "$INSTDIR\Lang\ru-ru\gen_win7shell.lng"
				Delete "$INSTDIR\Lang\ru-ru\gen_win7shell.lng"
				RMDir "$INSTDIR\Lang\ru-ru"		
			${EndIf}
		${Case} "1051" ; Slovak (1051)
			${If} ${FileExists} "$INSTDIR\Lang\sk-sk\gen_win7shell.lng"
				Delete "$INSTDIR\Lang\sk-sk\gen_win7shell.lng"
				RMDir "$INSTDIR\Lang\sk-sk"		
			${EndIf}	
		${Case} "1060" ; Slovenian (1060)
			${If} ${FileExists} "$INSTDIR\Lang\sl-si\gen_win7shell.lng"
				Delete "$INSTDIR\Lang\sl-si\gen_win7shell.lng"
				RMDir "$INSTDIR\Lang\sl-si"		
			${EndIf}		
		${Case} "1034" ; Spanish (1034)
			${If} ${FileExists} "$INSTDIR\Lang\es-us\gen_win7shell.lng"
				Delete "$INSTDIR\Lang\es-us\gen_win7shell.lng"
				RMDir "$INSTDIR\Lang\es-us"		
			${EndIf}
		${Case} "1053" ; Swedish (1053)
			${If} ${FileExists} "$INSTDIR\Lang\sv-se\gen_win7shell.lng"
				Delete "$INSTDIR\Lang\sv-se\gen_win7shell.lng"
				RMDir "$INSTDIR\Lang\sv-se"		
			${EndIf}
		${Case} "1055" ; Turkish (1055)
			${If} ${FileExists} "$INSTDIR\Lang\tr-tr\gen_win7shell.lng"
				Delete "$INSTDIR\Lang\tr-tr\gen_win7shell.lng"
				RMDir "$INSTDIR\Lang\tr-tr"
			${EndIf}	
		${Case} "1043" ; Dutch (1043)
			${If} ${FileExists} "$INSTDIR\Lang\nl-nl\gen_win7shell.lng"
				Delete "$INSTDIR\Lang\nl-nl\gen_win7shell.lng"
				RMDir "$INSTDIR\Lang\nl-nl"		
			${EndIf}
		${Case} "1040" ; Italian (1040)
			${If} ${FileExists} "$INSTDIR\Lang\it-it\gen_win7shell.lng"
				Delete "$INSTDIR\Lang\it-it\gen_win7shell.lng"
				RMDir "$INSTDIR\Lang\it-it"		
			${EndIf}
		${Case} "1032" ; Greek (1032)
			${If} ${FileExists} "$INSTDIR\Lang\el-gr\gen_win7shell.lng"
				Delete "$INSTDIR\Lang\el-gr\gen_win7shell.lng"
				RMDir "$INSTDIR\Lang\el-gr"		
			${EndIf}
		${Case} "1058" ; Ukrainian (1058)
			${If} ${FileExists} "$INSTDIR\Lang\ua-ua\gen_win7shell.lng"
				Delete "$INSTDIR\Lang\ua-ua\gen_win7shell.lng"
				RMDir "$INSTDIR\Lang\ua-ua"		
			${EndIf}	
		${Case} "1028" ; Chinese Traditional (1028)
			${If} ${FileExists} "$INSTDIR\Lang\zh-tw\gen_win7shell.lng"
				Delete "$INSTDIR\Lang\zh-tw\gen_win7shell.lng"
				RMDir "$INSTDIR\Lang\zh-tw"		
			${EndIf}
		${Case} "2052" ; Chinese Simplified (2052)
			${If} ${FileExists} "$INSTDIR\Lang\zh-cn\gen_win7shell.lng"
				Delete "$INSTDIR\Lang\zh-cn\gen_win7shell.lng"
				RMDir "$INSTDIR\Lang\zh-cn"		
			${EndIf}	
		${Case} "1038" ; Hungarian (1038)
			${If} ${FileExists} "$INSTDIR\Lang\hu-hu\gen_win7shell.lng"
				Delete "$INSTDIR\Lang\hu-hu\gen_win7shell.lng"
				RMDir "$INSTDIR\Lang\hu-hu"		
			${EndIf}
		${Case} "1030" ; Danish (1030)
			${If} ${FileExists} "$INSTDIR\Lang\da-dk\gen_win7shell.lng"
				Delete "$INSTDIR\Lang\da-dk\gen_win7shell.lng"
				RMDir "$INSTDIR\Lang\da-dk"		
			${EndIf}
		${Case} "1026" ; Bulgarian (1026)
			${If} ${FileExists} "$INSTDIR\Lang\bg-bl\gen_win7shell.lng"
				Delete "$INSTDIR\Lang\bg-bl\gen_win7shell.lng"
				RMDir "$INSTDIR\Lang\bg-bl"		
			${EndIf}
		${CaseElse} ;  English
			Return
	${EndSelect}
	
	DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${W7S_PLUGIN}"	
	Delete /REBOOTOK "$INSTDIR\${W7S_UNINSTALLER}.exe"
	SetAutoClose false
	
SectionEnd


;###########################################################################################
; THE END
;###########################################################################################