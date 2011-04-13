#define WIN32_LEAN_AND_MEAN
#define _SECURE_SCL 0
#define _CRT_SECURE_NO_DEPRECATE 1
#define _CRT_NONSTDC_NO_DEPRECATE 1
#define TAGLIB_STATIC

#define ICONSIZEPX 50
#define APPID L"Winamp"
#define INIFILE_NAME L"win7shell.ini"

#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <commdlg.h>
#include <dwmapi.h>
#include <string>
#include <vector>
#include <list>
#include <sstream>
#include <shlobj.h>
#include <shellapi.h>
#include <strsafe.h>
#include <shobjidl.h>
#include <gdiplus.h>
#include <process.h>
#include <cctype>
#include <ctime>
#include <fstream>
#include <map>

#include "gen_win7shell.h"
#include "sdk/winamp/gen.h"
#include "sdk/winamp/wa_ipc.h"
#include "sdk/Agave/Language/lang.h"
#include "resource.h"
#include "api.h"
#include "tools.h"
#include "VersionChecker.h"
#include "tabs.h"
#include "metadata.h"
#include "jumplist.h"
#include "albumart.h"
#include "settings.h"
#include "taskbar.h"
#include "renderer.h"

UINT WM_TASKBARBUTTONCREATED ;
const std::wstring cur_version(__T("1.14"));
const std::string cur_versionA("1.14 Test Build 9");
WNDPROC lpWndProcOld = 0;
char playstate = -1;
bool thumbshowing = false, 
     uninstall = false,
     cfgopen = false;
HWND cfgwindow = 0, ratewnd = 0;
HIMAGELIST theicons;
HHOOK hMouseHook;
int width = 200, height = 120;

sSettings Settings;
iTaskBar taskbar;
MetaData metadata;
renderer* thumbnaildrawer;

api_memmgr* WASABI_API_MEMMGR = 0;
api_service* WASABI_API_SVC = 0;
api_albumart* AGAVE_API_ALBUMART = 0;
api_playlists* playlistsApi = 0;
api_language* WASABI_API_LNG = 0;
// these two must be declared as they're used by the language api's
// when the system is comparing/loading the different resources
HINSTANCE WASABI_API_LNG_HINST = 0,
		  WASABI_API_ORIG_HINST = 0;

template <class api_T>
void ServiceBuild(api_T *&api_t, GUID factoryGUID_t)
{
	if (WASABI_API_SVC)
	{
		waServiceFactory *factory = WASABI_API_SVC->service_getServiceByGuid(factoryGUID_t);
		if (factory)
			api_t = (api_T *)factory->getInterface();
	}
}

// CALLBACKS
VOID CALLBACK TimerProc(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime);
LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
static INT_PTR CALLBACK cfgWndProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
static INT_PTR CALLBACK rateWndProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK TabHandler(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK KeyboardEvent(int nCode, WPARAM wParam, LPARAM lParam);

// THREAD PROCS
void CheckVersion(void *dummy);

void updateToolbar(bool first);
HBITMAP DrawThumbnail(int width, int height, int force);
std::wstring SearchDir(std::wstring path);
std::wstring MetaWord(std::wstring word);
//std::wstring GetLine(int linenumber, bool &center, bool &largefont, bool &forceleft, bool &shadow, bool &darkbox, bool &dontscroll);
std::wstring getToolTip(int button);
void SetupJumpList();

// Winamp EVENTS
int  init(void);
void config(void);
void quit(void);

// this structure contains plugin information, version, name...
// GPPHDR_VER is the version of the winampGeneralPurposePlugin (GPP) structure
winampGeneralPurposePlugin plugin = {
  GPPHDR_VER,  // version of the plugin, defined in "gen_myplugin.h"
  PLUGIN_NAME, // name/title of the plugin, defined in "gen_myplugin.h"
  init,        // function name which will be executed on init event
  config,      // function name which will be executed on config event
  quit,        // function name which will be executed on quit event
  0,           // handle to Winamp main window, loaded by winamp when this dll is loaded
  0            // hinstance to this dll, loaded by winamp when this dll is loaded
};

static char pluginTitle[MAX_PATH];
char* BuildPluginName(void){
	if(!pluginTitle[0]){
		StringCchPrintfA(pluginTitle,MAX_PATH,WASABI_API_LNGSTRING(IDS_PLUGIN_NAME),
						 cur_versionA.c_str());
	}
	return pluginTitle;
}

static wchar_t pluginTitleW[MAX_PATH];
wchar_t* BuildPluginNameW(void){
	if(!pluginTitleW[0]){
		StringCchPrintf(pluginTitleW,MAX_PATH,WASABI_API_LNGSTRINGW(IDS_PLUGIN_NAME),
						 cur_version.c_str());
	}
	return pluginTitleW;
}

// event functions follow
int init() 
{
    if(tools::GetWinampVersion(plugin.hwndParent) < 0x5050){
		MessageBoxA(plugin.hwndParent,"This plug-in requires Winamp v5.5 and up for it to work.\t\n"
									  "Please upgrade your Winamp client to be able to use this.",
									  plugin.description,MB_OK|MB_ICONINFORMATION);
		return GEN_INIT_FAILURE;
	}
	else{
		WASABI_API_SVC = (api_service*)SendMessage(plugin.hwndParent, WM_WA_IPC, 0, IPC_GET_API_SERVICE);
		if (WASABI_API_SVC == (api_service*)1) WASABI_API_SVC = NULL;

		if(WASABI_API_SVC != NULL)
		{
			/************************************************************************/
			/* Winamp services                                                      */
			/************************************************************************/
			ServiceBuild(WASABI_API_MEMMGR,memMgrApiServiceGuid);
			ServiceBuild(AGAVE_API_ALBUMART,albumArtGUID);
			ServiceBuild(AGAVE_API_PLAYLISTS,api_playlistsGUID);
			ServiceBuild(WASABI_API_LNG,languageApiGUID);
			WASABI_API_START_LANG(plugin.hDllInstance,GenWin7ShellLangGUID);

			plugin.description = BuildPluginName();

            // Accept messages even if Winamp was run as Administrator
            ChangeWindowMessageFilter(WM_COMMAND, 1);
            ChangeWindowMessageFilter(WM_DWMSENDICONICTHUMBNAIL, 1);
            ChangeWindowMessageFilter(WM_DWMSENDICONICLIVEPREVIEWBITMAP, 1);

            // Set Application model to APPID
            // TODO: auto-pin icon (?)
			if (SetCurrentProcessExplicitAppUserModelID(APPID) != S_OK)
				MessageBoxEx(plugin.hwndParent,
							 WASABI_API_LNGSTRINGW(IDS_ERROR_SETTING_APPID),
							 BuildPluginNameW(),
							 MB_ICONWARNING | MB_OK, 0);

            // Read Settings into struct
			ZeroMemory(&Settings, sizeof(Settings));
            std::wstring SettingsFile = tools::getWinampINIPath(plugin.hwndParent) + L"\\Plugins\\" + INIFILE_NAME;
            {
                SettingsManager SManager(SettingsFile);
                SManager.ReadSettings(Settings);   
            }            

            // If enabled, create jumplist
            if (Settings.JLrecent || Settings.JLfrequent || Settings.JLtasks || Settings.JLbms)
			{
				SetupJumpList();
			}            


            // Check for updates
            if (!Settings.DisableUpdates && Settings.LastUpdateCheck != tools::GetCurrentDay())
				if (SendMessage(plugin.hwndParent,WM_WA_IPC,0,IPC_INETAVAILABLE))
                {
					_beginthread(CheckVersion, 0, NULL);
                    Settings.LastUpdateCheck = tools::GetCurrentDay();
                    wstringstream s;
                    s << Settings.LastUpdateCheck;
                    WritePrivateProfileStringW(SECTION_NAME_GENERAL, L"LastUpdateCheck", 
                        s.str().c_str(), SettingsFile.c_str());
                }

            // Clear message queue
			MSG msg;
			while (PeekMessage(&msg, 0,0,0, PM_REMOVE))
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}

           // Register taskbarcreated message
           WM_TASKBARBUTTONCREATED = RegisterWindowMessage(L"TaskbarButtonCreated");

           // Override window procedure
           lpWndProcOld = (WNDPROC)SetWindowLongW(plugin.hwndParent,GWLP_WNDPROC,(LONG_PTR)WndProc);

           // Timers, settings, icons
           if (Settings.VuMeter)
               SetTimer(plugin.hwndParent, 6668, 50, TimerProc);

           int interval = (Settings.LowFrameRate == 1) ? 400 : 100;
           SetTimer(plugin.hwndParent, 6667, interval, TimerProc);     

           Settings.play_volume = IPC_GETVOLUME(plugin.hwndParent);

           theicons = tools::prepareIcons(plugin.hDllInstance);   
           metadata.setWinampWindow(plugin.hwndParent);

           // Create the taskbar interface
           if (!taskbar.Reset(plugin.hwndParent))
           {
               MessageBoxEx(plugin.hwndParent,
                   WASABI_API_LNGSTRINGW(IDS_TASKBAR_INITIALIZE_FAILURE),
                   BuildPluginNameW(), MB_ICONSTOP, 0);
           }
           else if (Settings.Thumbnailbuttons)
           {
               taskbar.SetWindowAttr(Settings.Thumbnailenabled, Settings.VolumeControl);
               if SUCCEEDED(taskbar.SetImageList(theicons))
                   updateToolbar(true);
           }       

           // Set up hook for mouse scroll volume control
           if (Settings.VolumeControl)
           {
	           hMouseHook = SetWindowsHookEx(WH_MOUSE_LL, (HOOKPROC) KeyboardEvent, plugin.hDllInstance, NULL);
	           if (hMouseHook == NULL)
		           MessageBoxEx(plugin.hwndParent,
				             WASABI_API_LNGSTRINGW(IDS_ERROR_REGISTERING_MOUSE_HOOK),
				             BuildPluginNameW(), MB_ICONWARNING, 0);
           }

			// send this on init(..) to update the text incase Winamp has already started
			// to play something and taskbar text scrolling is disabled (or enabled :) )
            if (Settings.RemoveTitle)
			{
				SendMessageW(plugin.hwndParent,WM_SETTEXT,0,0);
			}

            // Create album art wrapper and thumbnail renderer
            static AlbumArt albumart(WASABI_API_MEMMGR, AGAVE_API_ALBUMART);
            thumbnaildrawer = new renderer(Settings, metadata, albumart, plugin.hwndParent);

            return GEN_INIT_SUCCESS;
		}
	}
	return GEN_INIT_FAILURE;
}

void config()
{
	if ((cfgopen) && (cfgwindow != NULL))
	{
		SetForegroundWindow(cfgwindow);
		return;
	}
	cfgwindow = WASABI_API_CREATEDIALOGW(IID_MAIN,plugin.hwndParent,cfgWndProc);

	RECT rc;
	int screenWidth = GetSystemMetrics(SM_CXSCREEN);
	int screenHeight = GetSystemMetrics(SM_CYSCREEN);
	GetWindowRect(cfgwindow, &rc);
	MoveWindow(cfgwindow, (screenWidth/2) - ((rc.right - rc.left)/2),
		(screenHeight/2) - ((rc.bottom-rc.top)/2), rc.right - rc.left, rc.bottom - rc.top, true);

	ShowWindow(cfgwindow,SW_SHOW);
}

void quit() 
{	
	if (hMouseHook != NULL)
		UnhookWindowsHookEx(hMouseHook);

	if(uninstall == false)
	{
		sResumeSettings sResume;
		sResume.ResumePosition = Settings.play_playlistpos;
		sResume.ResumeTime = Settings.play_current;

		SetupJumpList();

		SettingsManager SManager(tools::getWinampINIPath(plugin.hwndParent) + L"\\Plugins\\" + INIFILE_NAME);
        SManager.WriteResume(sResume);
    }

    delete thumbnaildrawer;
} 

void CheckVersion(void *dummy)
{
	VersionChecker *vc = new VersionChecker();
	std::wstring news = vc->IsNewVersion(cur_version), vermsg;
	wchar_t tmp[256], tmp2[256], tmp3[256];
	if (news != __T(""))
	{
		news.erase(news.length()-1, 1);
		vermsg = WASABI_API_LNGSTRINGW_BUF(IDS_NEW_VERSION_AVAILABLE,tmp,256);
		vermsg += news + L"\n\n";
		vermsg += WASABI_API_LNGSTRINGW_BUF(IDS_DISABLE_UPDATE_CHECK,tmp2,256);
		vermsg += __T("\n");
		vermsg += WASABI_API_LNGSTRINGW_BUF(IDS_OPEN_DOWNLOAD_PAGE,tmp3,256);

		if (MessageBoxEx(plugin.hwndParent,
						 vermsg.c_str(),
						 BuildPluginNameW(), 
						 MB_ICONINFORMATION | MB_YESNO | MB_SETFOREGROUND, 0) == IDYES)
		{
			ShellExecute(NULL, L"open", L"http://code.google.com/p/win7shell/", NULL, NULL, SW_SHOW);
		}
	}
	delete vc;
}

std::wstring MetaWord(std::wstring word)
{
	if (word == __T("%curtime%"))
	{
		int res = SendMessage(plugin.hwndParent,WM_WA_IPC,0,IPC_GETOUTPUTTIME);
		if (res == -1)
			return __T("~");
		return tools::SecToTime(res/1000);
	}

	if (word == __T("%timeleft%"))
	{
		int cur = SendMessage(plugin.hwndParent,WM_WA_IPC,0,IPC_GETOUTPUTTIME);
		int tot = SendMessage(plugin.hwndParent,WM_WA_IPC,1,IPC_GETOUTPUTTIME);
		if (cur == -1 || tot == -1)
			return __T("~");
        return tools::SecToTime(tot - cur / 1000);
	}

	if (word == __T("%totaltime%"))
	{
		int res = SendMessage(plugin.hwndParent,WM_WA_IPC,1,IPC_GETOUTPUTTIME);
		if (res == -1)
			return __T("");
		return tools::SecToTime(res);
	}

	if (word == __T("%kbps%"))
	{
		int inf=SendMessage(plugin.hwndParent,WM_WA_IPC,1,IPC_GETINFO);
		if (inf == NULL)
			return std::wstring(__T(""));
		std::wstringstream w;
		w << inf;
		return w.str();
	}

	if (word == __T("%khz%"))
	{
		int inf=SendMessage(plugin.hwndParent,WM_WA_IPC,0,IPC_GETINFO);
		if (inf == NULL)
			return std::wstring(__T(""));
		std::wstringstream w;
		w << inf;
		return w.str();
	}

	if (word == __T("%c%"))
		return __T("‡center‡");

	if (word == __T("%l%"))
		return __T("‡largefont‡");

	if (word == __T("%f%"))
		return __T("‡forceleft‡");

	if (word == __T("%s%"))
		return __T("‡shadow‡");

	if (word == __T("%b%"))
		return __T("‡darkbox‡");

	if (word == __T("%d%"))
		return __T("‡dontscroll‡");

	if (word == __T("%Settings.play_volume%"))
	{
		std::wstringstream w;
		w << (Settings.play_volume * 100) /255;
		return w.str();
	}

	if (word == __T("%shuffle%"))
	{
		static wchar_t tmp[8];
		return WASABI_API_LNGSTRINGW_BUF((SendMessage(plugin.hwndParent,WM_WA_IPC,0,IPC_GET_SHUFFLE)?IDS_ON:IDS_OFF),tmp,8);
	}

	if (word == __T("%repeat%"))
	{
		static wchar_t tmp[8];
		return WASABI_API_LNGSTRINGW_BUF((SendMessage(plugin.hwndParent,WM_WA_IPC,0,IPC_GET_REPEAT)?IDS_ON:IDS_OFF),tmp,8);
	}

	if (word == __T("%curpl%"))
	{
		std::wstringstream w;
		w << (SendMessage(plugin.hwndParent,WM_WA_IPC,0,IPC_GETLISTLENGTH)?Settings.play_playlistpos+1:0);
		return w.str();
	}

	if (word == __T("%totalpl%"))
	{
		std::wstringstream w;
		w << SendMessage(plugin.hwndParent,WM_WA_IPC,0,IPC_GETLISTLENGTH);
		return w.str();
	}

	if (word == __T("%rating1%") || word == __T("%rating2%"))
	{
		std::wstringstream w;
		int x = SendMessage(plugin.hwndParent,WM_WA_IPC,0,IPC_GETRATING);
		if (word == __T("%rating1%"))
			w << x;
		else
		{
			for (int i=0; i < x; i++)
				w << __T("\u2605");
			for (int i=0; i < 5-x; i++)
				w << __T("\u2606"/*" \u25CF"*/);
		}
		return w.str();
	}

    return metadata.getMetadata(word.substr(1, word.length()-2));
}

std::wstring getToolTip(int button)
{
	int strID = -1;

	switch (button)
	{
	case 0:
		strID = IDS_PLAY;
		break;
	case 1:
		strID = IDS_PREVIOUS;
		break;
	case 2:
		strID = IDS_PAUSE;
		break;
	case 3:
		strID = IDS_STOP;
		break;
	case 4:
		strID = IDS_NEXT;
		break;
	case 5:
		strID = IDS_RATE;
		break;
	case 6:
		strID = IDS_VOLUME_DOWN;
		break;
	case 7:
		strID = IDS_VOLUME_UP;
		break;
	case 8:
		strID = IDS_OPEN_FILE;
		break;
	case 9:
		strID = IDS_MUTE;
		break;
	case 10:
		strID = IDS_STOP_AFTER_CURRENT;
		break;
	}

	if(strID != -1)
	{
		return WASABI_API_LNGSTRINGW(strID);
	}
	return L"";
}

void updateToolbar(bool first)
{
	if (!Settings.Thumbnailbuttons)
		return;

	std::vector<THUMBBUTTON> thbButtons;
	
	for (int i=0; i<16; i++)	
		if (Settings.Buttons[i] && thbButtons.size() < 7)
		{
			THUMBBUTTON button;
            ZeroMemory(&button, sizeof(button));
			button.dwMask = THB_BITMAP;// | THB_TOOLTIP;
			button.iId = i;
            button.hIcon = NULL;
            button.iBitmap = NULL;

			if (i == 9 || i == 4)
			{
				button.dwMask = button.dwMask | THB_FLAGS;
				button.dwFlags = THBF_DISMISSONCLICK;
			}
			
			if (i == 1)
			{				
				if (playstate == PLAYSTATE_PLAYING)
				{
					button.iBitmap = 2;
					wcsncpy(button.szTip, getToolTip(2).c_str(), 260);
				}
				else
				{
					button.iBitmap = 0;
					wcsncpy(button.szTip, getToolTip(0).c_str(), 260);
				}
			}
			else
			{
				button.iBitmap = i+1;
				wcsncpy(button.szTip, getToolTip(i+1).c_str(), 260);
			}
			
			thbButtons.push_back(button);
		}	

        if (!SUCCEEDED(taskbar.ThumbBarUpdateButtons(thbButtons, first)))
        {
            MessageBox(plugin.hwndParent, L"Error settings thumbnail buttons", L"Taskbar Error",
                MB_ICONWARNING);
        }
}

// std::wstring GetLine(int linenumber, bool &center, bool &largefont, bool &forceleft, bool &shadow, bool &darkbox, bool &dontscroll)
// {
// 	std::wstring::size_type pos = 0, open = std::wstring::npos;
// 	std::wstring text, metaword;
// 	center = 0;
// 	int count = 0;
// 	if (linenumber == 0)
// 		text = Settings.Text.substr(pos, Settings.Text.find_first_of(NEWLINE, pos));
// 	else
// 	{
// 		do 
// 		{
// 			pos = Settings.Text.find_first_of(NEWLINE, pos+1);
// 			if (pos != std::wstring::npos)
// 				count++;
// 		}
// 		while ( (count < linenumber) && (pos != std::wstring::npos) );
// 
// 		text = Settings.Text.substr(pos+1, Settings.Text.find_first_of(NEWLINE, pos+1)-pos-1);
// 	}
// 
// 	pos = 0;
// 	do
// 	{
// 		pos = text.find_first_of('%', pos);		
// 		if (pos != std::wstring::npos)
// 		{
// 			if (open != std::wstring::npos)
// 			{
// 				metaword = MetaWord(text.substr(open, pos-open+1));
// 				if (metaword == __T("‡center‡"))
// 				{
// 					metaword = __T("");
// 					center = true;
// 				}
// 				if (metaword == __T("‡largefont‡"))
// 				{
// 					metaword = __T("");
// 					largefont = true;
// 				}
// 				if (metaword == __T("‡forceleft‡"))
// 				{
// 					metaword = __T("");
// 					forceleft = true;
// 				}
// 				if (metaword == __T("‡shadow‡"))
// 				{
// 					metaword = __T("");
// 					shadow = true;
// 				}
// 				if (metaword == __T("‡darkbox‡"))
// 				{
// 					metaword = __T("");
// 					darkbox = true;
// 				}
// 				if (metaword == __T("‡dontscroll‡"))
// 				{
// 					metaword = __T("");
// 					dontscroll = true;
// 				}
// 
// 				text.replace(open, pos-open+1, metaword);
// 				open = std::wstring::npos;
// 				pos = std::wstring::npos;
// 			}
// 			else
// 				open = pos;
// 			pos++;
// 		}
// 	}
// 	while (pos!=std::wstring::npos);
// 
// 	return text;
// }

// HBITMAP DrawThumbnail(int width, int height, int force)
// {	
// 	int bg;
// 	(force == -1) ? bg = Settings.Thumbnailbackground : bg = force;
// 
// 	//Create background
// 	if (!background)
// 		switch (bg)
// 		{
// 		case 0: //transparent
// 			background = new Bitmap(width, height, PixelFormat32bppPARGB);
// 			totheleft = true;
// 			break;
// 
// 		case 1: //album art
// 			{
// 				ARGB32* cur_image = 0;
// 				int cur_w = 0, cur_h = 0;
// 				
// 				if (AGAVE_API_ALBUMART && AGAVE_API_ALBUMART->GetAlbumArt(metadata.getFileName().c_str(), L"cover", 
// 					&cur_w, &cur_h, &cur_image) == ALBUMART_SUCCESS)
// 				{
// 					BITMAPINFO bmi;
// 					ZeroMemory(&bmi, sizeof bmi);
// 					bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
// 					bmi.bmiHeader.biWidth = cur_w;
// 					bmi.bmiHeader.biHeight = -cur_h;
// 					bmi.bmiHeader.biPlanes = 1;
// 					bmi.bmiHeader.biBitCount = 32;
// 					bmi.bmiHeader.biCompression = BI_RGB;
// 					bmi.bmiHeader.biSizeImage = 0;
// 					bmi.bmiHeader.biXPelsPerMeter = 0;
// 					bmi.bmiHeader.biYPelsPerMeter = 0;
// 					bmi.bmiHeader.biClrUsed = 0;
// 					bmi.bmiHeader.biClrImportant = 0;
// 
// 					Bitmap tmpbmp(&bmi, cur_image);
// 					background = new Bitmap(width, height, PixelFormat32bppPARGB);
// 					Graphics gfx(background);
// 
// 					gfx.SetSmoothingMode(SmoothingModeNone);
// 					gfx.SetInterpolationMode(InterpolationModeHighQuality);
// 					gfx.SetPixelOffsetMode(PixelOffsetModeNone);
// 					gfx.SetCompositingQuality(CompositingQualityHighSpeed);
// 
// 					if (Settings.AsIcon)
// 					{							
// 						gfx.DrawImage(&tmpbmp, RectF(0, 0, ICONSIZEPX, ICONSIZEPX));
// 					}
// 					else
// 					{
// 						float ratio = (float)cur_h / (float)(height);
// 						float widthD = (float)width;
// 						if (ratio == 0)
// 							ratio = 1;
// 						ratio = (float)cur_w / ratio;
// 						if (ratio > widthD)
// 							ratio = widthD;
// 						gfx.DrawImage(&tmpbmp, RectF(((widthD-ratio)/2.0f), 0, ratio, (float)height));
// 					}
// 
// 					if (cur_image) 
// 						WASABI_API_MEMMGR->sysFree(cur_image); 
// 
// 					totheleft = false;
// 				}
// 				else
// 				{
// // 					AlbumArt AA;
// // 					Bitmap tmpbmp(height, height, PixelFormat32bppPARGB);
// // 					if (AA.getAA(metadata.getFileName(), tmpbmp, height))
// // 					{                                       
// // 						background = new Bitmap(width, height, PixelFormat32bppPARGB);
// // 						Graphics gfx(background);
// // 						gfx.SetInterpolationMode(InterpolationModeHighQualityBilinear);
// // 						if (Settings.AsIcon)
// // 						{                                                       
// // 							gfx.DrawImage(&tmpbmp, RectF(0, 0, ICONSIZEPX, ICONSIZEPX));
// // 						}
// // 						else
// // 							gfx.DrawImage(&tmpbmp, RectF(static_cast<REAL>(((double)width-(double)height)/(double)2),
// // 							0,static_cast<REAL>(height),static_cast<REAL>(height)));        
// // 						totheleft = false;
// // 					}          
// // 					else
// // 						if (force != -1)
// // 						{
// // 							background = new Bitmap(width, height, PixelFormat32bppPARGB);					
// // 							totheleft = true;
// // 						}
// // 						else
// // 							DrawThumbnail(width, height, Settings.Revertto);
// 				}
// 			}
// 			break;
// 
// // 		case 2: //default
// // 			if (Settings.AsIcon)
// // 			{
// // 				Bitmap tmp(plugin.hDllInstance, MAKEINTRESOURCE(IDB_BACKGROUND));
// // 				background = new Bitmap(width, height, PixelFormat32bppPARGB);
// // 				Graphics gfx(background);
// // 				gfx.DrawImage(&tmp, RectF(0, 0, ICONSIZEPX, ICONSIZEPX));
// // 				totheleft = true;
// // 			}
// // 			else
// // 				background = new Bitmap(plugin.hDllInstance, MAKEINTRESOURCE(IDB_BACKGROUND));
// // 			break;
// 
// 		case 3: //custom
// 			if (!Settings.BGPath.empty())
// 			{
// 				Image img(Settings.BGPath.c_str(), true);
// 				if (img.GetType() != 0)
// 				{
// 					background = new Bitmap(width, height, PixelFormat32bppPARGB);
// 					Graphics gfx(background);
// 					gfx.SetInterpolationMode(InterpolationModeHighQualityBilinear);
// 					if (Settings.AsIcon)
// 						gfx.DrawImage(&img, RectF(0, 0, ICONSIZEPX, ICONSIZEPX));
// 					else
// 						gfx.DrawImage(&img, RectF(0, 0, static_cast<REAL>(width), static_cast<REAL>(height)));
// 					totheleft = false;
// 				}
// 				else
// 				{		
// 					if (force != -1)
// 					{
// 						totheleft = true;
// 						background = new Bitmap(width, height, PixelFormat32bppPARGB);	
// 					}
// 					else
// 						DrawThumbnail(width, height, Settings.Revertto);
// 				}				
// 			}
// 			else
// 			{				
// 				if (force != -1)
// 				{
// 					totheleft = true;
// 					background = new Bitmap(width, height, PixelFormat32bppPARGB);					
// 				}
// 				else
// 					DrawThumbnail(width, height, Settings.Revertto);
// 				Settings.Thumbnailbackground = 0;
// 			}
// 			break;
// 
// 		default: 
// 			totheleft = true;
// 			background = new Bitmap(width, height, PixelFormat32bppPARGB);
// 	}
// 
// 	if (force >= 0)
// 		return NULL;
// 
// 	//Draw text
// 	HBITMAP retbmp;
// 	int lines = 1;
// 	HDC DC0 = GetDC(0);
// 
// 	int textheight=0;
// 	Font font(DC0, const_cast<LOGFONT*> (&Settings.font));
// 	LONG oldsize = Settings.font.lfHeight;
// 	int x = -((Settings.font.lfHeight * 72) / GetDeviceCaps(DC0, LOGPIXELSY));
// 	x += 4;
// 	Settings.font.lfHeight = -MulDiv(x, GetDeviceCaps(DC0, LOGPIXELSY), 72);
// 	Font largefont(DC0, const_cast<LOGFONT*> (&Settings.font));
// 	Settings.font.lfHeight = oldsize;
// 	StringFormat sf(StringFormatFlagsNoWrap);
// 
// 	Graphics tmpgfx(background);
// 
// 	struct texts {
// 		std::wstring text;
// 		int height;
// 		bool center;
// 		bool largefont;
// 		bool forceleft;
// 		bool shadow;
// 		bool darkbox;
// 		bool dontscroll;
// 	};
// 
// 	for (std::wstring::size_type i = 0; i < Settings.Text.length(); i++)
// 		if (Settings.Text[i] == NEWLINE)
// 			lines++;
// 
// 	std::vector<texts> TX;
// 	RectF retrect;
// 
// 	for (int i=0; i<lines; i++)
// 	{
// 		texts tx;
// 		ZeroMemory(&tx, sizeof(tx));
// 		tx.text = L"Some random song";//GetLine(i, tx.center, tx.largefont, tx.forceleft, tx.shadow, tx.darkbox, tx.dontscroll);
// 		if (tx.largefont)
// 			tmpgfx.MeasureString(tx.text.c_str(), -1, &largefont, RectF(0, 0, static_cast<REAL>(width), static_cast<REAL>(height)), &sf, &retrect);
// 		else
// 			tmpgfx.MeasureString(tx.text.c_str(), -1, &font, RectF(0, 0, static_cast<REAL>(width), static_cast<REAL>(height)), &sf, &retrect);
// 
// 		if (retrect.GetBottom() == 0)
// 			tmpgfx.MeasureString(L"QWEXCyjM", -1, &font, RectF(0, 0, static_cast<REAL>(width), static_cast<REAL>(height)), &sf, &retrect);			
// 		
// 		tx.height = static_cast<int>(retrect.GetBottom());
// 		textheight += tx.height;
// 		TX.push_back(tx);
// 	}	
// 
// 	if (!Settings.Shrinkframe)	
// 		textheight = height-2;
// 		
// 	if (Settings.Thumbnailpb)
// 		textheight += 25;
// 
// 	if (textheight > height-2)
// 		textheight = height-2;
// 
// 	Bitmap bmp(background->GetWidth(), textheight+2, PixelFormat32bppPARGB);
// 	Graphics gfx(&bmp);
// 	
// 	gfx.SetSmoothingMode(SmoothingModeNone);
// 	gfx.SetInterpolationMode(InterpolationModeNearestNeighbor);
// 	gfx.SetPixelOffsetMode(PixelOffsetModeNone);
// 	gfx.SetCompositingQuality(CompositingQualityHighSpeed);
// 
// 	if (Settings.AsIcon)
// 		gfx.DrawImage(background, RectF(0, 0, static_cast<REAL>(background->GetWidth()), static_cast<REAL>(background->GetHeight())));
// 	else
// 		gfx.DrawImage(background, RectF(0, 0, static_cast<REAL>(background->GetWidth()), static_cast<REAL>(textheight+2)));
// 
// 	if ( (Settings.Thumbnailbackground == 0) || (Settings.Thumbnailbackground == 1) )
// 		(Settings.Antialias) ? gfx.SetTextRenderingHint(TextRenderingHintAntiAlias) : gfx.SetTextRenderingHint(TextRenderingHintAntiAliasGridFit);
// 	else	
// 		(Settings.Antialias) ? gfx.SetTextRenderingHint(TextRenderingHintAntiAlias) : gfx.SetTextRenderingHint(TextRenderingHintClearTypeGridFit);		
// 
// 	SolidBrush bgcolor(Color(GetRValue(Settings.bgcolor), GetGValue(Settings.bgcolor), GetBValue(Settings.bgcolor)));
// 	SolidBrush fgcolor(Color(GetRValue(Settings.text_color), GetGValue(Settings.text_color), GetBValue(Settings.text_color)));
// 
// 	int heightsofar=0;
// 	for (int i=0; i < lines; i++)
// 	{
// 		sf.SetAlignment(StringAlignmentNear);
// 		int left = 2;
// 		if (Settings.AsIcon && !totheleft && !TX[i].forceleft)
// 			left = ICONSIZEPX + 4;
// 
// 		if (i)
// 			heightsofar += TX[i-1].height;
// 
// 		PointF point(static_cast<REAL>(left), static_cast<REAL>(heightsofar-1));
// 		if (TX[i].largefont)
// 			gfx.MeasureString(TX[i].text.c_str(), -1, &largefont, point, &sf, &retrect);
// 		else
// 			gfx.MeasureString(TX[i].text.c_str(), -1, &font, point, &sf, &retrect);
// 
// 		bool longtext = false;
// 		if (left + retrect.Width - 1 > width && !TX[i].dontscroll)
// 		{
// 			TX[i].text += L"     ";
// 			size_t pos = (step > 9) ? (step - 9) % TX[i].text.length() : 0;
// 			std::wstring tmp = TX[i].text.substr(0, pos);
// 			TX[i].text.erase(0, pos);
// 			TX[i].text += tmp;
// 			longtext = true;
// 		}
// 		else
// 		{
// 			sf.SetAlignment(static_cast<StringAlignment>(TX[i].center));
// 			if (TX[i].center)
// 				if (Settings.AsIcon && !TX[i].forceleft && !totheleft)
// 					retrect.X = (((REAL)(196 - ICONSIZEPX) / (REAL)2) - ((REAL)retrect.Width / (REAL)2)) + ICONSIZEPX;
// 				else
// 				{
// 					retrect.X = 98 - ((REAL)retrect.Width / 2);
// 					if (retrect.X <= 0)
// 						retrect.X = (float)left;
// 				}
// 			if (Settings.AsIcon && retrect.X < (ICONSIZEPX + 4) && !totheleft && !TX[i].forceleft)
// 				retrect.X = ICONSIZEPX + 4;
// 		}
// 
// 		if (TX[i].darkbox)
// 		{			
// 			SolidBrush boxbrush(Color::MakeARGB(120, GetRValue(Settings.bgcolor),
// 				GetGValue(Settings.bgcolor), GetBValue(Settings.bgcolor)));
// 
// 			retrect.Y += 2;
// 			retrect.Height -= 1;
// 			gfx.FillRectangle(&boxbrush, retrect);
// 		}
// 
// 		if (TX[i].shadow)
// 		{		
// 			PointF shadowpt(static_cast<REAL>(left+1), static_cast<REAL>(heightsofar));
// 			RectF shadowrect(retrect.X + 1, retrect.Y + 1, retrect.Width, retrect.Height);
// 			if (TX[i].largefont)
// 			{
// 				if (longtext)
// 					gfx.DrawString(TX[i].text.c_str(), -1, &largefont, shadowpt, &sf, &bgcolor);
// 				else
// 					gfx.DrawString(TX[i].text.c_str(), -1, &largefont, shadowrect, &sf, &bgcolor);					
// 			}
// 			else
// 			{
// 				if (longtext)
// 					gfx.DrawString(TX[i].text.c_str(), -1, &font, shadowpt, &sf, &bgcolor);
// 				else
// 					gfx.DrawString(TX[i].text.c_str(), -1, &font, shadowrect, &sf, &bgcolor);						
// 			}
// 		}
// 
// 		if (TX[i].largefont)
// 		{
// 			if (longtext)
// 				gfx.DrawString(TX[i].text.c_str(), -1, &largefont, point, &sf, &fgcolor);
// 			else
// 				gfx.DrawString(TX[i].text.c_str(), -1, &largefont, retrect, &sf, &fgcolor);
// 		}
// 		else
// 		{
// 			if (longtext)
// 				gfx.DrawString(TX[i].text.c_str(), -1, &font, point, &sf, &fgcolor);
// 			else
// 				gfx.DrawString(TX[i].text.c_str(), -1, &font, retrect, &sf, &fgcolor);
// 		}
// 	}
// 
// 	if (Settings.play_total > 0 && Settings.play_current >0 && Settings.Thumbnailpb)
// 	{
// 		gfx.SetSmoothingMode(SmoothingModeAntiAlias);		
// 		int Y = bmp.GetHeight()-15;
// 		Pen p1(Color::White, 1);
// 		Pen p2(Color::MakeARGB(80, 0, 0, 0), 1);
// 
// 		SolidBrush b1(Color::MakeARGB(150, 0, 0, 0));	
// 		SolidBrush b2(Color::MakeARGB(220, 255, 255, 255));
// 		RectF R1(44, (REAL)Y, 10, 9);
// 		RectF R2((REAL)width - 56, (REAL)Y, 10, 9);
// 
// 		gfx.FillRectangle(&b1, 46, Y, width-94, 9);
// 		gfx.DrawLine(&p2, 46, Y+2, 46, Y+6);
// 		gfx.DrawLine(&p2, width-48, Y+2, width-48, Y+6);
// 
// 		gfx.DrawArc(&p1, R1, -90.0f, -180.0f);
// 		gfx.DrawArc(&p1, R2, 90.0f, -180.0f);
// 		
// 		gfx.DrawLine(&p1, 48, Y, width-49, Y);
// 		gfx.DrawLine(&p1, 48, Y+9, width-49, Y+9);
// 
// 		gfx.SetSmoothingMode(SmoothingModeDefault);
// 		gfx.FillRectangle(&b2, 48, Y+3, (Settings.play_current*(width-96))/Settings.play_total, 4);
// 	}
// 	
// 	bmp.GetHBITMAP(Color::Black, &retbmp);
// 
// 	ReleaseDC(0, DC0);
// 
// 	return retbmp;
// }


LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
// 	case WM_DWMSENDICONICLIVEPREVIEWBITMAP:
// 		{
// 			thumbshowing = true;
// 			RECT rect;
// 			GetWindowRect(plugin.hwndParent, &rect);
// 			Bitmap bmp(rect.right-rect.left-1, rect.bottom-rect.top-1, PixelFormat32bppPARGB);
// 			StringFormat sf(StringFormatFlagsNoWrap);
// 			sf.SetAlignment(StringAlignmentCenter);
// 			Font font(L"Segoe UI", 18);
// 			SolidBrush brush(Color::MakeARGB(200, 0, 0, 0));
// 			std::wstringstream ss;
// 			ss << L"Volume: " << ((Settings.play_volume * 100) /255) << L"%\n" << metadata.getMetadata(L"title");
// 			ss << L"\n" << metadata.getMetadata(L"artist");
// 
// 			Graphics gfx(&bmp);
// 			gfx.SetTextRenderingHint(TextRenderingHintAntiAlias);
// 
// 			gfx.DrawString(ss.str().c_str(), -1, &font, RectF(0, 0, (float)(rect.right-rect.left-1), (float)(rect.bottom-rect.top-1)), &sf, &brush);
// 			HBITMAP hbmp;
// 			bmp.GetHBITMAP(Color::Black, &hbmp);
// 			
// 			POINT pt;
// 			pt.x = 0;
// 			pt.y = 0;
// 
// 			DwmSetIconicLivePreviewBitmap(plugin.hwndParent, hbmp, &pt, 0);
// 			DeleteObject(hbmp);
// 			return 0;
// 		}
// 		break;

	case WM_DWMSENDICONICTHUMBNAIL:
		{
            // drawit            
			if (!thumbshowing)
			{                
                OutputDebugString(L"Fire!\n");
                width = HIWORD(lParam);
                height = LOWORD(lParam);

                thumbnaildrawer->ThumbnailPopup();
                SetTimer(plugin.hwndParent, 6670, Settings.LowFrameRate ? 41 : 21, TimerProc); 
                SetTimer(plugin.hwndParent, 6672, 10000, TimerProc);
				thumbshowing = true;
            }
			return 0;
		}
		break;	

	case WM_COMMAND:		
		if (HIWORD(wParam) == THBN_CLICKED)
			switch (LOWORD(wParam))
			{
			case 0:
				{
					SendMessage(plugin.hwndParent, WM_COMMAND, MAKEWPARAM(40044,0), 0);
					Settings.play_playlistpos = SendMessage(plugin.hwndParent,WM_WA_IPC,0,IPC_GETLISTPOS);

                    if (Settings.Thumbnailbackground == BG_ALBUMART)
                    {
					    thumbnaildrawer->ClearBackground();

                        if (playstate != PLAYSTATE_PLAYING)
                        {
                            DwmInvalidateIconicBitmaps(plugin.hwndParent);
                        }
                    }

					int index = SendMessage(plugin.hwndParent,WM_WA_IPC,0,IPC_GETLISTPOS);
					wchar_t *p = (wchar_t*)SendMessage(plugin.hwndParent,WM_WA_IPC,index,IPC_GETPLAYLISTFILEW); 

					if (p != NULL)
                    {
                        metadata.reset(p, false);
                    }

					break;
				}

			case 1:
				{
					int res = SendMessage(plugin.hwndParent,WM_WA_IPC,0,IPC_ISPLAYING);
					if (res == 1)
                    {
						SendMessage(plugin.hwndParent, WM_COMMAND, MAKEWPARAM(40046,0), 0);									
                    }
					else					
                    {
						SendMessage(plugin.hwndParent, WM_COMMAND, MAKEWPARAM(40045,0), 0);					
                    }
					playstate = (char)res;
				}
				break;

			case 2:
				SendMessage(plugin.hwndParent, WM_COMMAND, MAKEWPARAM(40047,0), 0);
				playstate = PLAYSTATE_NOTPLAYING; 
				break;

			case 3:
				{				
					SendMessage(plugin.hwndParent, WM_COMMAND, MAKEWPARAM(40048,0), 0);
					Settings.play_playlistpos = SendMessage(plugin.hwndParent,WM_WA_IPC,0,IPC_GETLISTPOS);

                    if (Settings.Thumbnailbackground == BG_ALBUMART)
                    {
                        thumbnaildrawer->ClearBackground();

                        if (playstate != PLAYSTATE_PLAYING)
                        {
                            DwmInvalidateIconicBitmaps(plugin.hwndParent);
                        }                        
                    }



					int index = SendMessage(plugin.hwndParent,WM_WA_IPC,0,IPC_GETLISTPOS);
					wchar_t *p = (wchar_t*)SendMessage(plugin.hwndParent,WM_WA_IPC,index,IPC_GETPLAYLISTFILEW); 
					if (p != NULL)
						metadata.reset(p, false);
				}
				break;

			case 4:
				{
					ratewnd = WASABI_API_CREATEDIALOGW(IDD_RATEDLG,plugin.hwndParent,rateWndProc);

					RECT rc;
					POINT point;						
					GetCursorPos(&point);
					GetWindowRect(ratewnd, &rc);
					MoveWindow(ratewnd, point.x - 85, point.y - 15, rc.right - rc.left, rc.bottom - rc.top, false);	
					SetTimer(plugin.hwndParent, 6669, 5000, TimerProc);
					ShowWindow(ratewnd, SW_SHOW);
				}
				break;

			case 5:
				Settings.play_volume -= 25;
				if (Settings.play_volume < 0)
					Settings.play_volume = 0;
				SendMessage(plugin.hwndParent,WM_WA_IPC,Settings.play_volume,IPC_SETVOLUME);
				break;

			case 6:
				Settings.play_volume += 25;
				if (Settings.play_volume > 255)
					Settings.play_volume = 255;
				SendMessage(plugin.hwndParent,WM_WA_IPC,Settings.play_volume,IPC_SETVOLUME);
				break;

			case 7:
				SendMessage(plugin.hwndParent,WM_WA_IPC,(WPARAM)(HWND)0,IPC_OPENFILEBOX);
				break;

			case 8:
				static int lastvolume;
				if (Settings.play_volume == 0)
				{					
					Settings.play_volume = lastvolume;
					SendMessage(plugin.hwndParent,WM_WA_IPC,Settings.play_volume,IPC_SETVOLUME);
				}
				else
				{
					lastvolume = Settings.play_volume;
					SendMessage(plugin.hwndParent,WM_WA_IPC,0,IPC_SETVOLUME);
				}
				break;

			case 9:
				SendMessage(plugin.hwndParent, WM_COMMAND, MAKEWPARAM(40157,0), 0);
				break;
			}
		break;

		case WM_WA_IPC:
			switch (lParam)
			{
			case IPC_PLAYING_FILEW:
				{				
					std::wstring filename = (wchar_t*)wParam;
					// TODO - not sure on this one
					//int true_msg = true;
					if (filename.empty())
					{
						int index = SendMessage(plugin.hwndParent,WM_WA_IPC,0,IPC_GETLISTPOS);
						wchar_t *p = (wchar_t*)SendMessage(plugin.hwndParent,WM_WA_IPC,index,IPC_GETPLAYLISTFILEW); 
						if (p != NULL)
							filename = p;
						//true_msg = false;
					}

					metadata.reset(filename, false);

					Settings.play_playlistpos = SendMessage(plugin.hwndParent,WM_WA_IPC,0,IPC_GETLISTPOS);

					Settings.play_total = SendMessage(plugin.hwndParent,WM_WA_IPC,2,IPC_GETOUTPUTTIME);
					Settings.play_current = 0;

					DwmInvalidateIconicBitmaps(plugin.hwndParent);

					playstate = PLAYSTATE_PLAYING;

                    if ((Settings.JLrecent || Settings.JLfrequent) && !tools::is_in_recent(filename))
					{
						std::wstring title(metadata.getMetadata(L"title") + L" - " + 
							metadata.getMetadata(L"artist"));

						if (Settings.play_total > 0)
							title += L"  (" + tools::SecToTime(Settings.play_total/1000) + L")";						

						IShellLink * psl;
						SHARDAPPIDINFOLINK applink;						
                        if (/*true_msg  && */tools::__CreateShellLink(filename.c_str(), title.c_str(), &psl) == S_OK)
						{
							time_t rawtime;
							time (&rawtime);
							std::wstring timestr = _wctime(&rawtime);
							psl->SetDescription(timestr.c_str());
							applink.psl = psl;
							applink.pszAppID = APPID;
							SHAddToRecentDocs(SHARD_LINK, psl); 
						}
					}

					if (Settings.Thumbnailbackground == BG_ALBUMART)
					{
                        thumbnaildrawer->ClearBackground();
					}

                    thumbnaildrawer->ThumbnailPopup();
				}
				break;

			case IPC_CB_MISC:
				switch (wParam)
				{
					case IPC_CB_MISC_STATUS:
						playstate = (char)SendMessage(plugin.hwndParent,WM_WA_IPC,0,IPC_ISPLAYING);
					
						updateToolbar(false);

						if (Settings.Overlay)
						{
							wchar_t tmp[64] = {0};
							switch (playstate)
							{
							case 1:
								{
									HICON icon = ImageList_GetIcon(theicons, 0, 0);
                                    taskbar.SetIconOverlay(icon, WASABI_API_LNGSTRINGW_BUF(IDS_PLAYING,tmp,64));
									DestroyIcon(icon);
								}
								break;
							case 3:
								{
									HICON icon = ImageList_GetIcon(theicons, 2, 0);
									taskbar.SetIconOverlay(icon, WASABI_API_LNGSTRINGW_BUF(IDS_PAUSED,tmp,64));
									DestroyIcon(icon);
								}
								break;
							default:
								{
									HICON icon = ImageList_GetIcon(theicons, 3, 0);
									taskbar.SetIconOverlay(icon, WASABI_API_LNGSTRINGW_BUF(IDS_PAUSED,tmp,64));
									DestroyIcon(icon);
								}								
							}
						}
						break;

					case IPC_CB_MISC_VOLUME:
						Settings.play_volume = IPC_GETVOLUME(plugin.hwndParent);
						break;
				}
				break;
			}
			break;

		case WM_SYSCOMMAND:
			if (wParam == SC_CLOSE)
				SendMessage(plugin.hwndParent, WM_COMMAND, MAKEWPARAM(40001,0), 0);
			break;

		case WM_SETTEXT:
			if (Settings.RemoveTitle && !wParam)
			{
				wchar_t tmp[64] = {0};
				for(int i = IDC_PCB1; i <= IDC_PCB10; i++)
				{
					if(Settings.Buttons[i-IDC_PCB1])
					{
						StringCchCat(tmp,64,L"       ");
					}
				}
				SendMessageW(hwnd,WM_SETTEXT,wParam+1,(LPARAM)tmp);
				return FALSE;
			}
	}

    if (message == WM_TASKBARBUTTONCREATED)
    {
        if (taskbar.Reset(plugin.hwndParent))
        {
            if (Settings.Thumbnailbuttons)
                if SUCCEEDED(taskbar.SetImageList(theicons))
                    updateToolbar(true);
        }         
        else
        {
            MessageBoxEx(plugin.hwndParent,
                WASABI_API_LNGSTRINGW(IDS_TASKBAR_INITIALIZE_FAILURE),
                BuildPluginNameW(), MB_ICONSTOP, 0);
        }

        SetupJumpList();
        taskbar.SetWindowAttr(Settings.Thumbnailenabled, Settings.VolumeControl);
    }

	LRESULT ret = CallWindowProc(lpWndProcOld,hwnd,message,wParam,lParam);	 

	switch(message)
	{
		case WM_WA_IPC:
			switch (lParam)
			{
			case IPC_ADDBOOKMARK:
			case IPC_ADDBOOKMARKW:
				{
					if(wParam) SetupJumpList();
				}
				break;
			}
			break;
	}

	return ret;
}

inline void CheckThumbShowing()
{
    if (!thumbshowing)
        return;

    POINT pt;
    GetCursorPos(&pt);
    std::wstring class_name;
    class_name.resize(200);
    GetClassName(WindowFromPoint(pt), &class_name[0], 200);
    class_name.resize(wcslen(class_name.c_str()));

    if (class_name != L"TaskListThumbnailWnd"
        && class_name != L"MSTaskListWClass"
        && class_name != L"ToolbarWindow32")
    {
        thumbshowing = false;	
        KillTimer(plugin.hwndParent, 6670);
        DwmInvalidateIconicBitmaps(plugin.hwndParent);
    }
}

VOID CALLBACK TimerProc(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime)
{
    switch (idEvent)
	{
    case 6668: //vumeter proc
        {
            if (Settings.VuMeter)
            {
                static int (*export_sa_get)(int channel) = (int(__cdecl *)(int)) SendMessage(plugin.hwndParent, WM_WA_IPC, 0, IPC_GETVUDATAFUNC);
                int audiodata = export_sa_get(0);

                if (playstate != PLAYSTATE_PLAYING || audiodata == -1)
                {
                    taskbar.SetProgressState(TBPF_NOPROGRESS);
                }
                else
                {
                    if (audiodata <= 150)
                        taskbar.SetProgressState(TBPF_NORMAL);
                    else if (audiodata > 150 && audiodata < 210)
                        taskbar.SetProgressState(TBPF_PAUSED);
                    else
                        taskbar.SetProgressState(TBPF_ERROR);		

                    taskbar.SetProgressValue(audiodata, 255);			
                }
            }
            break;
        }

    case 6669: //rate wnd
        {
            if (ratewnd != NULL)
                DestroyWindow(ratewnd);
            KillTimer(plugin.hwndParent, 6669);
            break;
        }

    case 6670: //scroll redraw
        {
            OutputDebugString(L"Redraw\n");
            HBITMAP thumbnail = thumbnaildrawer->GetThumbnail(width, height);
            if (Settings.Shrinkframe)
	            Sleep(50);

            HRESULT hr = DwmSetIconicThumbnail(plugin.hwndParent, thumbnail, 0);
            if (FAILED(hr))
            {
                KillTimer(plugin.hwndParent, 6670);
	            MessageBoxEx(plugin.hwndParent,
				             WASABI_API_LNGSTRINGW(IDS_ERROR_SETTING_THUMBNAIL),
				             BuildPluginNameW(), MB_ICONERROR, 0);
	            Settings.Thumbnailenabled = false;
	            taskbar.SetWindowAttr(Settings.Thumbnailenabled, Settings.VolumeControl);
            }

            DeleteObject(thumbnail);
        }

    case 6672: //check thumbshowing just in case - every 10 sec
        {
                CheckThumbShowing();
        }

    case 6667: // main timer
	    {
            if (thumbshowing)
		    {
                CheckThumbShowing();
		    }
    	
		    if (playstate == -1)
		    {
	 		    WndProc(plugin.hwndParent, WM_WA_IPC, (WPARAM)L"", IPC_PLAYING_FILEW);
			    WndProc(plugin.hwndParent, WM_WA_IPC, IPC_CB_MISC_STATUS, IPC_CB_MISC);
		    }
    	
		    if (!(Settings.Progressbar || Settings.VuMeter))
			    taskbar.SetProgressState(TBPF_NOPROGRESS);	
    	
		    if (playstate == PLAYSTATE_PLAYING || Settings.Thumbnailpb)
			    if (Settings.play_total <= 0)
				    Settings.play_total = SendMessage(plugin.hwndParent,WM_WA_IPC,1,IPC_GETOUTPUTTIME) * 1000;
    	
		    int cp = SendMessage(plugin.hwndParent,WM_WA_IPC,0,IPC_GETOUTPUTTIME);
		    if (Settings.play_current == cp)
		    {
			    return;
		    }
		    Settings.play_current = cp;
    	
		    switch (playstate)
		    {
			    case 1: 
    	
				    if (Settings.play_current == -1 || Settings.play_total <= 0)
				    { 
					    static unsigned char count2=0;
					    if (count2 == 8)
					    {   
						    count2 = 0;
						    metadata.reset(L"", true);
					    }
					    else
						    count2++;
				    }
    	
					    if (Settings.Progressbar)
					    {					
						    if (Settings.play_current == -1 || Settings.play_total <= 0)
						    {
							    if (Settings.Streamstatus)
								    taskbar.SetProgressState(TBPF_INDETERMINATE);
							    else						
								    taskbar.SetProgressState(TBPF_NOPROGRESS);
						    }
						    else
						    {
							    taskbar.SetProgressState(TBPF_NORMAL);
	                            taskbar.SetProgressValue(Settings.play_current, Settings.play_total);
						    }
					    }
				    break;
    	
			    case 3: 
					    if (Settings.Progressbar)
					    {
						    taskbar.SetProgressState(TBPF_PAUSED);
						    taskbar.SetProgressValue(Settings.play_current, Settings.play_total);	
    	
						    if (SendMessage(plugin.hwndParent,WM_WA_IPC,1,IPC_GETOUTPUTTIME) == -1)
							    taskbar.SetProgressValue(100, 100);
					    }
				    break;
    	
			    default:
    	
				    if (Settings.Progressbar) 
				    {
					    if (Settings.Stoppedstatus)
					    {
						    taskbar.SetProgressState(TBPF_ERROR);
						    taskbar.SetProgressValue(100, 100);
					    }
					    else
						    taskbar.SetProgressState(TBPF_NOPROGRESS);
				    }
				    break;
		    }
        }
        break;
	}
}

static INT_PTR CALLBACK rateWndProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDC_RATE1:
			SendMessage(plugin.hwndParent,WM_WA_IPC,0,IPC_SETRATING);
			DestroyWindow(hwndDlg);
			break;

		case IDC_RATE2:
			SendMessage(plugin.hwndParent,WM_WA_IPC,1,IPC_SETRATING);
			DestroyWindow(hwndDlg);
			break;

		case IDC_RATE3:
			SendMessage(plugin.hwndParent,WM_WA_IPC,2,IPC_SETRATING);
			DestroyWindow(hwndDlg);
			break;

		case IDC_RATE4:
			SendMessage(plugin.hwndParent,WM_WA_IPC,3,IPC_SETRATING);
			DestroyWindow(hwndDlg);
			break;

		case IDC_RATE5:
			SendMessage(plugin.hwndParent,WM_WA_IPC,4,IPC_SETRATING);
			DestroyWindow(hwndDlg);
			break;

		case IDC_RATE6:
			SendMessage(plugin.hwndParent,WM_WA_IPC,5,IPC_SETRATING);
			DestroyWindow(hwndDlg);
			break;
		}
		break;
	}

	return 0;
}

static INT_PTR CALLBACK cfgWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	static HWND TabCtrl;
	static HWND Tab1, Tab2, Tab3, Tab4, Tab5, Tab6;
	static sSettings Settings_backup;	
	static bool applypressed = false;

	switch(msg)
	{
	case WM_INITDIALOG:
		{			
			Tab1 = WASABI_API_CREATEDIALOGW(IID_TAB1, hwnd, TabHandler);
			Tab2 = WASABI_API_CREATEDIALOGW(IID_TAB2, hwnd, TabHandler);
			Tab3 = WASABI_API_CREATEDIALOGW(IID_TAB3, hwnd, TabHandler);
			Tab4 = WASABI_API_CREATEDIALOGW(IID_TAB4, hwnd, TabHandler);
			Tab5 = WASABI_API_CREATEDIALOGW(IID_TAB5, hwnd, TabHandler);
			Tab6 = WASABI_API_CREATEDIALOGW(IID_TAB6, hwnd, TabHandler);

			TabCtrl = GetDlgItem(hwnd, IDC_TAB);

			AddTab(TabCtrl, Tab1, WASABI_API_LNGSTRINGW(IDS_TEXT), -1, plugin.hwndParent);
			AddTab(TabCtrl, Tab2, WASABI_API_LNGSTRINGW(IDS_BACKGROUND), -1, plugin.hwndParent);
			AddTab(TabCtrl, Tab3, WASABI_API_LNGSTRINGW(IDS_OPTIONS), -1, plugin.hwndParent);
			AddTab(TabCtrl, Tab4, WASABI_API_LNGSTRINGW(IDS_TASKBAR_ICON), -1, plugin.hwndParent);
			AddTab(TabCtrl, Tab5, WASABI_API_LNGSTRINGW(IDS_JUMPLIST), -1, plugin.hwndParent);

			std::wstring cv;
			cv = WASABI_API_LNGSTRINGW(IDS_DEVELOPMENT);
			cv += __T(" v") + cur_version;
			AddTab(TabCtrl, Tab6, const_cast<wchar_t*> (cv.c_str()), -1, plugin.hwndParent);
			
			TabToFront(TabCtrl, Settings.LastTab);

			for (int i=0; i<10; i++)
			{
				HICON icon = ImageList_GetIcon(theicons, i+1, 0);
				SendMessage(GetDlgItem(Tab3, IDC_PCB1+i), BM_SETIMAGE, IMAGE_ICON, (LPARAM)icon);
				DestroyIcon(icon);
			}

			HICON okbut = (HICON)LoadImage(plugin.hDllInstance, MAKEINTRESOURCE(IDI_ICON18), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
			HICON applybut = (HICON)LoadImage(plugin.hDllInstance, MAKEINTRESOURCE(IDI_ICON20), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
			HICON cancelbut = (HICON)LoadImage(plugin.hDllInstance, MAKEINTRESOURCE(IDI_ICON19), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
			if (okbut != NULL && applybut != NULL && cancelbut != NULL)
			{
				SendMessage(GetDlgItem(hwnd, IDC_BUTTON1), BM_SETIMAGE, IMAGE_ICON, (LPARAM)okbut);
				SendMessage(GetDlgItem(hwnd, IDC_BUTTON8), BM_SETIMAGE, IMAGE_ICON, (LPARAM)applybut);
				SendMessage(GetDlgItem(hwnd, IDC_BUTTON2), BM_SETIMAGE, IMAGE_ICON, (LPARAM)cancelbut);
			}

			cfgopen = TRUE;
		}
		break;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
			case IDC_BUTTON1: //OK
				{
					cfgopen = FALSE;
					bool b1 = Settings.Thumbnailbuttons, b2 = false;
					bool Buttons[16]; 
					std::copy(Settings.Buttons, Settings.Buttons+16, Buttons);
					bool recent = Settings.JLrecent;
					bool frequent = Settings.JLfrequent;
					bool tasks = Settings.JLtasks;
					bool bms = Settings.JLbms;
					
                    SettingsManager::ReadSettings_FromForm(Tab1, plugin.hwndParent, Settings);
                    SettingsManager::ReadSettings_FromForm(Tab2, plugin.hwndParent, Settings);
                    SettingsManager::ReadSettings_FromForm(Tab3, plugin.hwndParent, Settings);
                    SettingsManager::ReadSettings_FromForm(Tab4, plugin.hwndParent, Settings);
                    SettingsManager::ReadSettings_FromForm(Tab5, plugin.hwndParent, Settings);
                    SettingsManager::ReadSettings_FromForm(Tab6, plugin.hwndParent, Settings);

                    if (recent != Settings.JLrecent ||
						frequent != Settings.JLfrequent ||
						tasks != Settings.JLtasks ||
						bms != Settings.JLbms)
					{
						SetupJumpList();					
					}

					taskbar.SetWindowAttr(Settings.Thumbnailenabled, Settings.VolumeControl);

					if (Settings.VuMeter)
						SetTimer(plugin.hwndParent, 6668, 50, TimerProc);
					else
						KillTimer(plugin.hwndParent, 6668);

                    thumbnaildrawer->ClearBackground();

					for (int i=0; i<16; i++)
						if (Settings.Buttons[i] != Buttons[i])
						{
							b2 = true;
							break;
						}

					if (b1 != Settings.Thumbnailbuttons || b2)
					{
						wchar_t title[128];
						if(MessageBoxEx(cfgwindow,
									 WASABI_API_LNGSTRINGW(IDS_MOD_PLAYER_BUTTONS_RESTART),
									 WASABI_API_LNGSTRINGW_BUF(IDS_APPLYING_SETTINGS, title, 128),
									 MB_ICONWARNING | MB_YESNO, 0) == IDYES)
						{
							PostMessage(plugin.hwndParent,WM_WA_IPC,0,IPC_RESTARTWINAMP);
						}
					}

					if (Settings.Overlay)
						WndProc(plugin.hwndParent, WM_WA_IPC, IPC_CB_MISC_STATUS, IPC_CB_MISC);
					else
                        taskbar.SetIconOverlay(NULL, NULL);

					if (Settings.VolumeControl)
					{
						if (!hMouseHook)
						{
							hMouseHook = SetWindowsHookEx(WH_MOUSE_LL, (HOOKPROC) KeyboardEvent, plugin.hDllInstance, NULL);
							if (hMouseHook == NULL)
								MessageBoxEx(plugin.hwndParent,
											 WASABI_API_LNGSTRINGW(IDS_ERROR_REGISTERING_MOUSE_HOOK),
											 BuildPluginNameW(),
											 MB_ICONWARNING, 0);
						}
					}
					else
					{
						if (hMouseHook)
						{
							UnhookWindowsHookEx(hMouseHook);
							hMouseHook = NULL;
						}
					}

					int interval = (Settings.LowFrameRate == 1) ? 400 : 100;
					SetTimer(plugin.hwndParent, 6667, interval, TimerProc);		
					
                    // Write settings to .ini file
                    SettingsManager SManager(tools::getWinampINIPath(plugin.hwndParent) + L"\\Plugins\\" + INIFILE_NAME);
                    SManager.WriteSettings(Settings);

					DestroyWindow(hwnd);
				}
				break;
			case IDC_BUTTON2:
			case IDCANCEL:
				if (applypressed)
				{	
					applypressed = false;

					memcpy(&Settings, &Settings_backup, sizeof(Settings_backup));

					SetupJumpList();				

					taskbar.SetWindowAttr(Settings.Thumbnailenabled, Settings.VolumeControl);

					if (Settings.VuMeter)
						SetTimer(plugin.hwndParent, 6668, 50, TimerProc);
					else
						KillTimer(plugin.hwndParent, 6668);

                    thumbnaildrawer->ClearBackground();

					if (Settings.Overlay)
						WndProc(plugin.hwndParent, WM_WA_IPC, IPC_CB_MISC_STATUS, IPC_CB_MISC);
					else
                        taskbar.SetIconOverlay(NULL, NULL);

					if (Settings.VolumeControl)
					{
						if (!hMouseHook)
						{
							hMouseHook = SetWindowsHookEx(WH_MOUSE_LL, (HOOKPROC) KeyboardEvent, plugin.hDllInstance, NULL);
							if (hMouseHook == NULL)
								MessageBoxEx(plugin.hwndParent,
											 WASABI_API_LNGSTRINGW(IDS_ERROR_REGISTERING_MOUSE_HOOK),
											 BuildPluginNameW(),
											 MB_ICONWARNING, 0);
						}
					}
					else
					{
						if (hMouseHook)
						{
							UnhookWindowsHookEx(hMouseHook);
							hMouseHook = NULL;
						}
					}

					int interval = (Settings.LowFrameRate == 1) ? 400 : 100;
					SetTimer(plugin.hwndParent, 6667, interval, TimerProc);
				}

				cfgopen = FALSE;
				DestroyWindow(hwnd);
				break;	

			case IDC_BUTTON8: //Apply
				{					
					if (!applypressed)
					{					
						memcpy(&Settings_backup, &Settings, sizeof(Settings));
					}					

					applypressed = true;

					bool b1 = Settings.Thumbnailbuttons, b2 = false;
					bool Buttons[16]; 
					std::copy(Settings.Buttons, Settings.Buttons+16, Buttons);
					bool recent = Settings.JLrecent;
					bool frequent = Settings.JLfrequent;
					bool tasks = Settings.JLtasks;
					bool bms = Settings.JLbms;

                    SettingsManager::ReadSettings_FromForm(Tab1, plugin.hwndParent, Settings);
                    SettingsManager::ReadSettings_FromForm(Tab2, plugin.hwndParent, Settings);
                    SettingsManager::ReadSettings_FromForm(Tab3, plugin.hwndParent, Settings);
                    SettingsManager::ReadSettings_FromForm(Tab4, plugin.hwndParent, Settings);
                    SettingsManager::ReadSettings_FromForm(Tab5, plugin.hwndParent, Settings);
                    SettingsManager::ReadSettings_FromForm(Tab6, plugin.hwndParent, Settings);

					if (recent != Settings.JLrecent ||
						frequent != Settings.JLfrequent ||
						tasks != Settings.JLtasks ||
						bms != Settings.JLbms)
					{
						SetupJumpList();					
					}

					for (int i=0; i<16; i++)
					{
						if (Settings.Buttons[i] != Buttons[i])
						{
							b2 = true;
							break;
						}
					}

					if (b1 != Settings.Thumbnailbuttons || b2)
					{
						wchar_t title[128];
						if(MessageBoxEx(cfgwindow, WASABI_API_LNGSTRINGW(IDS_MOD_PLAYER_BUTTONS_RESTART),
									WASABI_API_LNGSTRINGW_BUF(IDS_APPLYING_SETTINGS, title, 128),
									MB_ICONWARNING | MB_YESNO, 0) == IDYES)
						{
							PostMessage(hwnd, WM_COMMAND, MAKEWPARAM(IDC_BUTTON1,0), (LPARAM)GetDlgItem(hwnd, IDC_BUTTON1));
							PostMessage(plugin.hwndParent,WM_WA_IPC,0,IPC_RESTARTWINAMP);
							break;
						}
					}

					taskbar.SetWindowAttr(Settings.Thumbnailenabled, Settings.VolumeControl);

					if (Settings.VuMeter)
						SetTimer(plugin.hwndParent, 6668, 50, TimerProc);
					else
						KillTimer(plugin.hwndParent, 6668);

                    thumbnaildrawer->ClearBackground();

					if (Settings.Overlay)
						WndProc(plugin.hwndParent, WM_WA_IPC, IPC_CB_MISC_STATUS, IPC_CB_MISC);
					else
                        taskbar.SetIconOverlay(NULL, NULL);

					if (Settings.VolumeControl)
					{
						if (!hMouseHook)
						{
							hMouseHook = SetWindowsHookEx(WH_MOUSE_LL, (HOOKPROC) KeyboardEvent, plugin.hDllInstance, NULL);
							if (hMouseHook == NULL)
								MessageBoxEx(plugin.hwndParent,
											 WASABI_API_LNGSTRINGW(IDS_ERROR_REGISTERING_MOUSE_HOOK),
											 BuildPluginNameW(),
											 MB_ICONWARNING, 0);
						}
					}
					else
					{
						if (hMouseHook)
						{
							UnhookWindowsHookEx(hMouseHook);
							hMouseHook = NULL;
						}
					}

					int interval = (Settings.LowFrameRate == 1) ? 400 : 100;
					SetTimer(plugin.hwndParent, 6667, interval, TimerProc);
				}
				break;
		}
	break;

	case WM_NOTIFY:
		switch(((NMHDR *)lParam)->code)
        {
        case TCN_SELCHANGE: // Get currently selected tab window to front
				Settings.LastTab = SendMessage(TabCtrl, TCM_GETCURSEL, 0, 0);
                TabToFront(TabCtrl, -1);
                break;
        case TRBN_THUMBPOSCHANGING:
                {
	                wstringstream size;
	                size << "Icon size (" << SendMessage(GetDlgItem(hwnd, IDC_SLIDER1), TBM_GETPOS, NULL, NULL) << "%)";
	                SetWindowTextW(GetDlgItem(hwnd, IDC_STATIC_SIZE), size.str().c_str());
                }
            break;
        default:
                return false;
        }		
	break;		

	case WM_CLOSE:
		cfgopen = FALSE;
		TabCleanup(TabCtrl);
		DestroyWindow(hwnd);
		DestroyWindow(Tab1);
		DestroyWindow(Tab2);
		DestroyWindow(Tab3);
		DestroyWindow(Tab4);
		DestroyWindow(Tab5);
		DestroyWindow(Tab6);
		break;
	}
		
	return 0;
}

INT_PTR CALLBACK TabHandler(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	switch(Message)
	{
		case WM_INITDIALOG:			
			SettingsManager::WriteSettings_ToForm(hwnd, plugin.hwndParent, Settings);
			break;

		case WM_COMMAND:
			switch(LOWORD(wParam))
			{
				case IDC_BUTTON3:				
                        SetWindowText(GetDlgItem(hwnd, IDC_EDIT2), tools::GetFileName(cfgwindow).c_str());
						if (GetWindowTextLength(GetDlgItem(hwnd, IDC_EDIT2)) == 0)
							SendMessage(GetDlgItem(hwnd, IDC_CHECK20), (UINT) BM_SETCHECK, 0, 0);
						else					
							SendMessage(GetDlgItem(hwnd, IDC_CHECK20), (UINT) BM_SETCHECK, 1, 0);				
					break;
				case IDC_BUTTON4:
					{
						std::wstring msgtext = WASABI_API_LNGSTRINGW(IDS_HELP1);
						msgtext += __T("\n\n");
						msgtext += WASABI_API_LNGSTRINGW(IDS_HELP2);
						msgtext += __T("\n\n");
						msgtext += __T("\t---\t ");
						msgtext += WASABI_API_LNGSTRINGW(IDS_HELP27);
						msgtext += __T(" \t---\n");
						msgtext += __T("%c%  -  ");
						msgtext += WASABI_API_LNGSTRINGW(IDS_HELP3);
						msgtext += __T("\n");
						msgtext += __T("%l%  -  ");
						msgtext += WASABI_API_LNGSTRINGW(IDS_HELP22);
						msgtext += __T("\n");
						msgtext += __T("%f%  -  ");
						msgtext += WASABI_API_LNGSTRINGW(IDS_HELP23);
						msgtext += __T("\n");
						msgtext += __T("%s%  -  ");
						msgtext += WASABI_API_LNGSTRINGW(IDS_HELP24);
						msgtext += __T("\n");
						msgtext += __T("%b%  -  ");
						msgtext += WASABI_API_LNGSTRINGW(IDS_HELP25);
						msgtext += __T("\n");
						msgtext += __T("%d%  -  ");
						msgtext += WASABI_API_LNGSTRINGW(IDS_HELP28);
						msgtext += __T("\n");
						msgtext += __T("\t---\t ");
						msgtext += WASABI_API_LNGSTRINGW(IDS_HELP26);
						msgtext += __T(" \t---\n");
						msgtext += __T("%title%  -  ");
						msgtext += WASABI_API_LNGSTRINGW(IDS_HELP5);
						msgtext += __T("\n");
						msgtext += __T("%artist%  -  ");
						msgtext += WASABI_API_LNGSTRINGW(IDS_HELP6);
						msgtext += __T("\n");
						msgtext += __T("%album%  -    ");
						msgtext += WASABI_API_LNGSTRINGW(IDS_HELP7);
						msgtext += __T("\n");
						msgtext += __T("%year%  -  ");
						msgtext += WASABI_API_LNGSTRINGW(IDS_HELP8);
						msgtext += __T("\n");
						msgtext += __T("%track%  -  ");
						msgtext += WASABI_API_LNGSTRINGW(IDS_HELP9);
						msgtext += __T("\n");
						msgtext += __T("%rating1%  -  ");
						msgtext += WASABI_API_LNGSTRINGW(IDS_HELP10);
						msgtext += __T("\n");
						msgtext += __T("%rating2%  -  ");
						msgtext += WASABI_API_LNGSTRINGW(IDS_HELP11);
						msgtext += __T("\n");
						msgtext += __T("%curtime%  -  ");
						msgtext += WASABI_API_LNGSTRINGW(IDS_HELP12);
						msgtext += __T("\n");
						msgtext += __T("%timeleft%  -  ");
						msgtext += WASABI_API_LNGSTRINGW(IDS_HELP13);
						msgtext += __T("\n");
						msgtext += __T("%totaltime%  -  ");
						msgtext += WASABI_API_LNGSTRINGW(IDS_HELP14);
						msgtext += __T("\n");
						msgtext += __T("%curpl%  -  ");
						msgtext += WASABI_API_LNGSTRINGW(IDS_HELP15);
						msgtext += __T("\n");
						msgtext += __T("%totalpl%  -  ");
						msgtext += WASABI_API_LNGSTRINGW(IDS_HELP16);
						msgtext += __T("\n");
						msgtext += __T("%kbps%  -  ");
						msgtext += WASABI_API_LNGSTRINGW(IDS_HELP17);
						msgtext += __T("\n");
						msgtext += __T("%khz%  -  ");
						msgtext += WASABI_API_LNGSTRINGW(IDS_HELP18);
						msgtext += __T("\n");
						msgtext += __T("%Settings.play_volume%  -  ");
						msgtext += WASABI_API_LNGSTRINGW(IDS_HELP19);
						msgtext += __T("\n");
						msgtext += __T("%shuffle%  -  ");
						msgtext += WASABI_API_LNGSTRINGW(IDS_HELP20);
						msgtext += __T("\n");
						msgtext += __T("%repeat%  -  ");
						msgtext += WASABI_API_LNGSTRINGW(IDS_HELP21);
						msgtext += __T("\n");

						MessageBoxEx(cfgwindow, msgtext.c_str(),
									 WASABI_API_LNGSTRINGW(IDS_INFORMATION),
									 MB_ICONINFORMATION, 0);
					}
					break;
				case IDC_BUTTON5:
					{
						CHOOSEFONT cf;
						ZeroMemory(&cf, sizeof(cf));
						cf.lStructSize = sizeof(cf);
						cf.hwndOwner = cfgwindow;					
						cf.rgbColors = Settings.text_color;
						cf.lpLogFont = &Settings.font;
						cf.Flags = CF_SCREENFONTS | CF_INITTOLOGFONTSTRUCT |
								   // added these styles to prevent fonts being listed
								   // which appear to have no effect when used (will
								   // need to investigate further but this is a decent
								   // safety blanket on things for the time being - dro).
								   CF_SCALABLEONLY | CF_NOOEMFONTS | CF_TTONLY;
						
						if (ChooseFont(&cf))
						{
							Settings.font = *cf.lpLogFont;
						}
					}
					break;
				case IDC_BUTTON6:
					{
						CHOOSECOLOR cc;                 // common dialog box structure 
						static COLORREF acrCustClr[16]; // array of custom colors 

						// Initialize CHOOSECOLOR 
						ZeroMemory(&cc, sizeof(cc));
						cc.lStructSize = sizeof(cc);
						cc.hwndOwner = cfgwindow;
						cc.lpCustColors = (LPDWORD) acrCustClr;
						cc.rgbResult = Settings.bgcolor;
						cc.Flags = CC_FULLOPEN | CC_RGBINIT;
						 
						if (ChooseColor(&cc)==TRUE) 
						{
							Settings.bgcolor = cc.rgbResult;
						}
					}
					break;

                case IDC_BUTTON9:
                    {
                        CHOOSECOLOR cc;                 // common dialog box structure 
                        static COLORREF acrCustClr[16]; // array of custom colors 

                        // Initialize CHOOSECOLOR 
                        ZeroMemory(&cc, sizeof(cc));
                        cc.lStructSize = sizeof(cc);
                        cc.hwndOwner = cfgwindow;
                        cc.lpCustColors = (LPDWORD) acrCustClr;
                        cc.rgbResult = Settings.bgcolor;
                        cc.Flags = CC_FULLOPEN | CC_RGBINIT;

                        if (ChooseColor(&cc)==TRUE) 
                        {
                            Settings.text_color = cc.rgbResult;
                        }
                    }
                    break;

				case IDC_CHECK2:
					{
						bool checked = (Button_GetCheck(GetDlgItem(hwnd, IDC_CHECK2)) == BST_CHECKED);
						EnableWindow(GetDlgItem(hwnd, IDC_CHECK4), checked);
						EnableWindow(GetDlgItem(hwnd, IDC_CHECK5), checked);
						if (SendMessage(GetDlgItem(hwnd, IDC_CHECK2), (UINT) BM_GETCHECK, 0, 0))
							SendMessage(GetDlgItem(hwnd, IDC_CHECK26), (UINT) BM_SETCHECK, 0, 0);		
					}
					break;
				case IDC_CHECK6:
					EnableWindow(GetDlgItem(hwnd, IDC_CHECK27), SendMessage(GetDlgItem(hwnd, IDC_CHECK6), 
						(UINT) BM_GETCHECK, 0, 0));
				case IDC_CHECK20:
					if (SendMessage(GetDlgItem(hwnd, IDC_CHECK20), (UINT) BM_GETCHECK, 0, 0))
						SendMessage(GetDlgItem(hwnd, IDC_CHECK21), (UINT) BM_SETCHECK, 0, 0);				
					break;
				case IDC_CHECK21:
					if (SendMessage(GetDlgItem(hwnd, IDC_CHECK21), (UINT) BM_GETCHECK, 0, 0))
						SendMessage(GetDlgItem(hwnd, IDC_CHECK20), (UINT) BM_SETCHECK, 0, 0);	
					break;
				case IDC_CHECK26:
					if (SendMessage(GetDlgItem(hwnd, IDC_CHECK26), (UINT) BM_GETCHECK, 0, 0))
					{
						SendMessage(GetDlgItem(hwnd, IDC_CHECK2), (UINT) BM_SETCHECK, 0, 0);	
						EnableWindow(GetDlgItem(hwnd, IDC_CHECK4), 0);
						EnableWindow(GetDlgItem(hwnd, IDC_CHECK5), 0);
					}
					break;
			}
			break;

		case WM_NOTIFY:
				switch (wParam)
				{
				case IDC_SYSLINK1:
					switch (((LPNMHDR)lParam)->code)
					{
					case NM_CLICK:
					case NM_RETURN:
						{
							ShellExecute(NULL, L"open", L"http://code.google.com/p/win7shell/", NULL, NULL, SW_SHOW);
						}
					}
					break;

				case IDC_SYSLINK2:
					switch (((LPNMHDR)lParam)->code)
					{
					case NM_CLICK:
					case NM_RETURN:
						{
							ShellExecute(NULL, L"open", L"http://code.google.com/p/win7shell/issues/entry", NULL, NULL, SW_SHOW);
						}
					}
					break;

				case IDC_SYSLINK3:
					switch (((LPNMHDR)lParam)->code)
					{
					case NM_CLICK:
					case NM_RETURN:
						{
							ShellExecute(NULL, L"open", L"http://code.google.com/p/win7shell/wiki/Translations", NULL, NULL, SW_SHOW);
						}
					}
					break;

				case IDC_SYSLINK4:
					switch (((LPNMHDR)lParam)->code)
					{
					case NM_CLICK:
					case NM_RETURN:
						{
							ShellExecute(NULL, L"open", L"mailto:atti86@gmail.com", NULL, NULL, SW_SHOW);
						}
					}
					break;

				case IDC_SYSLINK5:
					switch (((LPNMHDR)lParam)->code)
					{
					case NM_CLICK:
					case NM_RETURN:
						{
							ShellExecute(NULL, L"open", L"http://code.google.com/p/win7shell/wiki/Credits", NULL, NULL, SW_SHOW);
						}
					}
					break;

				case IDC_PCB1:
					if ((((LPNMHDR)lParam)->code) == BCN_HOTITEMCHANGE)
						SetWindowText(GetDlgItem(hwnd, IDC_STATIC29), getToolTip(1).c_str());
					break;

				case IDC_PCB2:
					if ((((LPNMHDR)lParam)->code) == BCN_HOTITEMCHANGE)
						SetWindowText(GetDlgItem(hwnd, IDC_STATIC29), 
						std::wstring(getToolTip(0) + __T(" / ") + getToolTip(2)).c_str());
					break;

				case IDC_PCB3:
					if ((((LPNMHDR)lParam)->code) == BCN_HOTITEMCHANGE)
						SetWindowText(GetDlgItem(hwnd, IDC_STATIC29), getToolTip(3).c_str());
					break;

				case IDC_PCB4:
					if ((((LPNMHDR)lParam)->code) == BCN_HOTITEMCHANGE)
						SetWindowText(GetDlgItem(hwnd, IDC_STATIC29), getToolTip(4).c_str());
					break;

				case IDC_PCB5:
					if ((((LPNMHDR)lParam)->code) == BCN_HOTITEMCHANGE)
						SetWindowText(GetDlgItem(hwnd, IDC_STATIC29), getToolTip(5).c_str());
					break;

				case IDC_PCB6:
					if ((((LPNMHDR)lParam)->code) == BCN_HOTITEMCHANGE)
						SetWindowText(GetDlgItem(hwnd, IDC_STATIC29), getToolTip(6).c_str());
					break;

				case IDC_PCB7:
					if ((((LPNMHDR)lParam)->code) == BCN_HOTITEMCHANGE)
						SetWindowText(GetDlgItem(hwnd, IDC_STATIC29), getToolTip(7).c_str());
					break;

				case IDC_PCB8:
					if ((((LPNMHDR)lParam)->code) == BCN_HOTITEMCHANGE)
						SetWindowText(GetDlgItem(hwnd, IDC_STATIC29), getToolTip(8).c_str());
					break;

				case IDC_PCB9:
					if ((((LPNMHDR)lParam)->code) == BCN_HOTITEMCHANGE)
						SetWindowText(GetDlgItem(hwnd, IDC_STATIC29), getToolTip(9).c_str());
					break;

				case IDC_PCB10:
					if ((((LPNMHDR)lParam)->code) == BCN_HOTITEMCHANGE)
						SetWindowText(GetDlgItem(hwnd, IDC_STATIC29), getToolTip(10).c_str());
					break;

				}
				break;

		case WM_MOUSEMOVE:
			SetWindowText(GetDlgItem(hwnd, IDC_STATIC29), WASABI_API_LNGSTRINGW(IDS_MAX_BUTTONS));
			break;

		default:
			return false;
	}

	return true;
}

void SetupJumpList()
{
	JumpList jl;
	if (jl.DeleteJumpList())
	{
        std::wstring bms = tools::getBMS(plugin.hwndParent);
		static wchar_t tmp1[128], tmp2[128], tmp3[128], tmp4[128], tmp5[128], tmp6[128];
		JumpList jl;
        if (!jl.CreateJumpList(tools::getShortPluginDllPath(plugin.hDllInstance),
			WASABI_API_LNGSTRINGW_BUF(IDS_WINAMP_PREFERENCES,tmp1,128),
			WASABI_API_LNGSTRINGW_BUF(IDS_PLAY_FROM_BEGINNING,tmp2,128),
			WASABI_API_LNGSTRINGW_BUF(IDS_RESUME_PLAYBACK,tmp3,128),
			WASABI_API_LNGSTRINGW_BUF(IDS_OPEN_FILE,tmp4,128),
			WASABI_API_LNGSTRINGW_BUF(IDS_BOOKMARKS,tmp5,128),
			WASABI_API_LNGSTRINGW_BUF(IDS_PLAYLISTS,tmp6,128),
			Settings.JLrecent, Settings.JLfrequent,
			Settings.JLtasks, Settings.JLbms, Settings.JLpl, bms))
		{
			MessageBoxEx(plugin.hwndParent,
						 WASABI_API_LNGSTRINGW(IDS_ERROR_CREATING_JUMP_LIST),
						 0, MB_ICONWARNING | MB_OK, 0);
		}
	}	
}

LRESULT CALLBACK KeyboardEvent (int nCode, WPARAM wParam, LPARAM lParam)
{
	if (!thumbshowing || wParam != WM_MOUSEWHEEL)
		return CallNextHookEx(hMouseHook,nCode,wParam,lParam);

	if ((short)((HIWORD(((MSLLHOOKSTRUCT*)lParam)->mouseData))) > 0)
	{		
		Settings.play_volume += 7;
		if (Settings.play_volume > 255)
			Settings.play_volume = 255;
		SendMessage(plugin.hwndParent,WM_WA_IPC,Settings.play_volume,IPC_SETVOLUME);
	}
	else
	{
		Settings.play_volume -= 7;
		if (Settings.play_volume < 0)
			Settings.play_volume = 0;
		SendMessage(plugin.hwndParent,WM_WA_IPC,Settings.play_volume,IPC_SETVOLUME);
	}

// 	RECT rect;
// 	GetWindowRect(plugin.hwndParent, &rect);
// 	Bitmap bmp(rect.right-rect.left-1, rect.bottom-rect.top-1, PixelFormat32bppPARGB);
// 	StringFormat sf(StringFormatFlagsNoWrap);
// 	sf.SetAlignment(StringAlignmentCenter);
// 	Font font(L"Segoe UI", 18);
// 	SolidBrush brush(Color::MakeARGB(200, 0, 0, 0));
// 	std::wstringstream ss;
// 	ss << L"Volume: " << ((Settings.play_volume * 100) /255) << L"%\n" << metadata.getMetadata(L"title");
// 	ss << L"\n" << metadata.getMetadata(L"artist");
// 
// 	Graphics gfx(&bmp);
// 	gfx.SetTextRenderingHint(TextRenderingHintAntiAlias);
// 
// 	gfx.DrawString(ss.str().c_str(), -1, &font, RectF(0, 0, (float)(rect.right-rect.left-1), (float)(rect.bottom-rect.top-1)), &sf, &brush);
// 	HBITMAP hbmp;
// 	bmp.GetHBITMAP(Color::Black, &hbmp);
// 
// 	POINT pt;
// 	pt.x = 0;
// 	pt.y = 0;
// 
// 	DwmSetIconicLivePreviewBitmap(plugin.hwndParent, hbmp, &pt, 0);
// 	DeleteObject(hbmp);
// 
// 	DwmInvalidateIconicBitmaps(plugin.hwndParent);

	return CallNextHookEx(hMouseHook,nCode,wParam,lParam);
}


// This is an export function called by winamp which returns this plugin info.
// We wrap the code in 'extern "C"' to ensure the export isn't mangled if used in a CPP file.
#ifdef __cplusplus
extern "C" {
#endif
	__declspec(dllexport) winampGeneralPurposePlugin * winampGetGeneralPurposePlugin() 
    {
		return &plugin;
	}

	__declspec( dllexport ) int winampUninstallPlugin(HINSTANCE hDllInst, HWND hwndDlg, int param) 
    {
		uninstall = true;
		if(MessageBox(hwndDlg,WASABI_API_LNGSTRINGW(IDS_UNINSTALL_PROMPT),
					  BuildPluginNameW(),
					  MB_YESNO)==IDYES)
        {
            std::wstring ini_path = tools::getWinampINIPath(plugin.hwndParent) + L"\\Plugins\\" + INIFILE_NAME;
            DeleteFile(ini_path.c_str());
			JumpList jl;
			jl.DeleteJumpList();
		}
		return GEN_PLUGIN_UNINSTALL_REBOOT;
	}
#ifdef __cplusplus
}
#endif

extern "C" int __declspec(dllexport) __stdcall pref() 
{
	HWND hwnd = tools::getWinampWindow();
	if (hwnd == 0)
		MessageBoxEx(0, L"Please start Winamp first", BuildPluginNameW(), MB_ICONWARNING, 0);
	else
	{
		SendMessage(hwnd,WM_WA_IPC,(WPARAM)-1,IPC_OPENPREFSTOPAGE);
		
		Sleep(600);
		HWND prefs = (HWND)SendMessage(hwnd,WM_WA_IPC,0,IPC_GETPREFSWND);
		if (IsWindow(prefs))
			SetForegroundWindow(prefs);
	}

	return 0;
}

extern "C" int __declspec(dllexport) __stdcall fromstart() 
{
	HWND hwnd = tools::getWinampWindow();
	if (hwnd == 0)
		MessageBoxEx(0, L"Please start Winamp first", BuildPluginNameW(), MB_ICONWARNING, 0);
	else
	{
		SendMessage(hwnd, WM_WA_IPC, (WPARAM)-1, IPC_SETPLAYLISTPOS);
		SendMessage(hwnd, WM_COMMAND, MAKEWPARAM(40045,0), 0);
	}

	return 0;
}

extern "C" int __declspec(dllexport) __stdcall resume() 
{
	HWND hwnd = tools::getWinampWindow();    
	if (hwnd == 0)
		MessageBoxEx(0, L"Please start Winamp first", BuildPluginNameW(), MB_ICONWARNING, 0);
	else
	{
		if (SendMessage(hwnd,WM_WA_IPC,0,IPC_ISPLAYING) == 1)
			return 0;
		
		std::wstring path;
        path = tools::getWinampINIPath(hwnd);

		if (path.empty())
			return -1;

		path += L"\\Plugins\\";
        path += INIFILE_NAME;

        sResumeSettings resume;
		SettingsManager SManager(path);
        SManager.ReadResume(resume);

		SendMessage(hwnd, WM_WA_IPC, resume.ResumePosition, IPC_SETPLAYLISTPOS);
		SendMessage(hwnd, WM_COMMAND, MAKEWPARAM(40045,0), 0);
		SendMessage(hwnd, WM_WA_IPC, resume.ResumeTime, IPC_JUMPTOTIME);
	}

	return 0;
}

extern "C" int __declspec(dllexport) __stdcall openfile() 
{
	HWND hwnd = tools::getWinampWindow();
	if (hwnd == 0)
		MessageBoxEx(0, L"Please start Winamp first", BuildPluginNameW(), MB_ICONWARNING, 0);
	else
		SendMessage(hwnd,WM_WA_IPC,(WPARAM)(HWND)0,IPC_OPENFILEBOX);

	return 0;
}