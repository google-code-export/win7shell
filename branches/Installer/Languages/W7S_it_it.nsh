;###########################################################################################

; Lang:				Italian
; LangID			6969
; Last udpdated:	06.10.2009
; Author:			AlexanderPD			
; Email:			pdalex@gmail.com


; Notes:
; Use ';' or '#' for comments
; Strings must be in double quotes.
; Only edit the strings in quotes:
; Make sure there's no trailing spaces at ends of lines
; To use double quote inside string - '$\'
; To force new line  - '$\r$\n'
; To insert tabulation  - '$\t'

;###########################################################################################

; Language selection
	LangString W7S_Language_Title ${LANG_ITALIAN} "Lingua dell'installer"
	LangString W7S_Un_Language_Title ${LANG_ITALIAN} "Lingua dell'uninstaller"	
	LangString W7S_Language_Text ${LANG_ITALIAN} "Seleziona un linguaggio:"

; First Page of Installer
	LangString W7S_Welcome_Title ${LANG_ITALIAN} "Benvenuto nell'installazione guidata del plugin $(^NameDA)"
	LangString W7S_Welcome_Text ${LANG_ITALIAN} "Questo programma ti guiderà durante l'installazione del plugin $(^NameDA).$\r$\n$\r$\nRaccomandiamo di chiudere Winamp prima di proseguire con l'installazione, così da poter aggiornare i componenti di Winamp senza errori.$\n$\nQuesto plugin ha bisogno di Windows 7 per funzionare!$\r$\n$\r$\n$_CLICK"

; Installer Header	
	LangString W7S_Caption ${LANG_ITALIAN} "Installazione del plugin ${W7S_PLUGIN} v${W7S_VERSION}"		
	LangString W7S_Branding ${LANG_ITALIAN} "${W7S_PLUGIN_DESC} v${W7S_VERSION}"			
	
; Installation type	
	LangString W7S_Full ${LANG_ITALIAN} "Completa"
	LangString W7S_Minimal ${LANG_ITALIAN} "Minimale"
	
; Installer sections
	LangString W7S_PluginFile ${LANG_ITALIAN} "${W7S_PLUGIN} plugin"
	LangString W7S_Language ${LANG_ITALIAN} "Lingua Italiana"
	
; Installer sections descriptions	
	LangString W7S_Desc_PluginFile ${LANG_ITALIAN} "Verrà installato il plugin ${W7S_PLUGIN}."
	LangString W7S_Desc_Language ${LANG_ITALIAN} "Verrà installata la traduzione italiana per il plugin ${W7S_PLUGIN}."

	
; Finish Page	
	LangString W7S_FinishPage_1 ${LANG_ITALIAN} "Installazione del plugin ${W7S_PLUGIN} v${W7S_VERSION} completata"
	LangString W7S_FinishPage_2 ${LANG_ITALIAN} "L'installazione guidata del plugin ${W7S_PLUGIN} v${W7S_VERSION} è stata completata. Ora puoi iniziare ad usare il plugin ${W7S_PLUGIN} in Winamp."
;	LangString W7S_FinishPage_3 ${LANG_ITALIAN} "Se ti è piaciuto il plugin ${W7S_PLUGIN} e vuoi aiutare gli sviluppi futuri puoi effettuare una donazione."
	LangString W7S_FinishPage_4 ${LANG_ITALIAN} "Cosa vuoi fare ora?"
	LangString W7S_FinishPage_5 ${LANG_ITALIAN} "Vai all'homepage del plugin"
	LangString W7S_FinishPage_6 ${LANG_ITALIAN} "Apri Winamp"
	LangString W7S_FinishPage_7 ${LANG_ITALIAN} "Fine"
	
; First Page of Uninstaller
	LangString W7S_Un_Welcome_Title ${LANG_ITALIAN} "Benvenuto alla disinstallazione guidata del plugin $(^NameDA)"
	LangString W7S_Un_Welcome_Text ${LANG_ITALIAN} "Questo programma ti guiderà durante la disinstallazione del plugin $(^NameDA).$\r$\n$\r$\nPrima di iniziare assicurati che Winamp sia chiuso.$\r$\n$\r$\n$_CLICK"

; Installation
	LangString W7S_Account ${LANG_ITALIAN} "Configurazione multi-utente"
	LangString W7S_No_Account ${LANG_ITALIAN} "Configurazione mono-utente"
	LangString W7S_Winamp_Path ${LANG_ITALIAN} "Specifica il path del file di configurazione di Winamp..."
	LangString W7S_Win7_Warning ${LANG_ITALIAN} "Ora non stai usando Windows 7. Questo plugin ha bisogno di Windows 7 per funzionare correttamente.$\r$\nVuoi installarlo lo stesso?"
	
; Close all instances of Winamp
	LangString W7S_Running_Winamp ${LANG_ITALIAN} "Winamp è ancora in esecuzione!"
	LangString W7S_Closing_Winamp ${LANG_ITALIAN} "        Stò chiudendo Winamp (winamp.exe)..."
	LangString W7S_No_Winamp ${LANG_ITALIAN} "OK. Winamp non è in esecuzione..."
	LangString W7S_No_More_Winamp ${LANG_ITALIAN} "        OK. Winamp è chiuso..."  
	LangString W7S_Check_Winamp ${LANG_ITALIAN} "Controllo se Winamp è in esecuzione..."
	LangString W7S_Close_Winamp  ${LANG_ITALIAN} "Prima di continuare, chiudi tutte le istanze di Winamp.$\r$\nSei sicuro di voler chiudere Winamp (raccomandato)?"	