#define WIN32_LEAN_AND_MEAN
#define _SECURE_SCL 0
#define _CRT_SECURE_NO_DEPRECATE 1
#define _CRT_NONSTDC_NO_DEPRECATE 1
#define TAGLIB_STATIC

#define ICONSIZEPX 50
#define APPID L"Winamp"

// changes:
// * altered IID_MAIN to have a hidden style on creating so it doesn't cause gui issue on opening (noticed via VNC)
// * reworking of the getWinampIniPath(..) function to attempt to use Winamp's message api where possible instead of as a last resort
// -> this seems to resolve the issue where i wasn't able to see bookmarks (may help others as well)
// * ensured compatible with 5.58 (as 5.58 broke shared handling)
// * fixing prefs to remember the last selected page - done
// * prefs now used IPC_USE_UXTHEME_FUNC so is a 5.5+ plugin now (which is sensible for a unicode only plugin)
// * added a check on the cache - was causing sparodic crashes at times if not done
// * added in a version lock on the plugin to only work on 5.5+ installs
// * adding in code to use the newer unicode ini api for 5.58+ installs
// * adjusted version checker to handle testing with a newer verison ie 1.14 test when 1.13 is the current version
// * added in use of api_playlists* to get the installed playlists instead of using the xml reading option (need to ensure it's safe to release the file)
// * playlist and bookmark items now show the playlist/file they relate to in the tooltip for the item (mainly for debugging but looks ok)
// * changing path detection/handling to use correct strings from Winamp where possible

// changes from v1.14 Test 1
// * bit more code clean-up for the getting of the wasabi services
// * gone through and changed all of the dialogs to be in the correct tabbing order (one for koopa as/when he implements things)
// * tab control now is included in the tab listing which i think will help things out
// * added in automatic building of the lng file for the plugin and created a new guid for it (so should be able to start using it soon - yay)
// * added some missing language strings

// changes from v1.14 Test 2
// * wasted a load of time trying to work out why the cache handler is crashing in release mode but nfiw it crashes
// * releasing test build 2 with a debug msvcrt compiled into the code which seems to work (must be an overflow somewhere else but nfi)
// * %curpl% now returns 0 if there is no playlist items (seemed weird with it showing 1 or 0 when empty)
// * adding in handling to re-create the jumplist if explorer crashed and it needs to be re-added
// * adding items to the bookmark list will now cause an update of the jump list

// changes from v1.14 Test 3
// * seeing if the recent thing can be removed until a file is played instead of just on loading (seems a bit inconsistant the way it is)
// * fixed lang support to actually use the localised dialog resources now (doh)

// changes from v1.14 Test 4
// * added back in WM_MOUSEMOVE handler on the buttons config page so it'll correctly revert to the fallback text now
// * cleaned up project files to fit in a single folder so it's easier to pop into a development folder relative to the 'sdk' folder
// * removed DestroyIcon(..) call when the imagelist is built as per MSDN info (maybe this will resolve the random loss of icon issue?!?!)

// changes from v1.14 Test 5
// * fixed getToolTip(..) not having break's in all the correct places
// * fixed string id 34 not correctly showing the star symbol (alt code 9733)
// * changed the non-rated part with %rating2% to use the empty star instead of a dot as it's not too clear (though easy to revert if not liked)
// * fixed incorrect string mapping on the title formatting help message
// * altered implementation for the remove title feature so it'll pad things out to ensure all buttons will appear as required even under the 'basic' theme
// -> will now re-enable the text when the setting is altered as well when taskbar text scrolling is disabled
// * changed the prompt for player control buttons to allow Winamp to be restarted if yes is selected

// changes from v1.14 Test 6
// *tweaking the blank space padding from 5 to 7 characters to hopefully resolve osmosis's issues under basic theme...
// -> appears to work better now i've re-tested things on this version so will see what comes back..

// sort out opening prefs/ofd to be slightly delayed so that all of Winamp can be correctly started before they appear (affects modern skins)
// fix jump list to work better - is there aa limit on the number of items to be shown??

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
#include "../sdk/winamp/gen.h"
#include "../sdk/winamp/wa_ipc.h"
#include "resource.h"
#include "api.h"
#include "VersionChecker.h"
#include "tabs.h"
#include "metadata.h"
#include "jumplist.h"
#include "albumart.h"

using namespace Gdiplus;

sSettings Settings;
sSettings_strings Settings_strings;
sFontEx Settings_font;
int S_lowframerate = 0;
static UINT WM_TASKBARBUTTONCREATED;
const std::wstring cur_version(__T("1.14"));
const std::string cur_versionA("1.14 Test Build 7");
UINT s_uTaskbarRestart=0;
WNDPROC lpWndProcOld = 0;
ITaskbarList3* pTBL = 0;
char playstate = -1;
int gCurPos = -2, gTotal = -2, gPlayListPos = 0;
int volume = 0;
size_t step = 0;
std::wstring INI_DIR;
std::wstring BM_FILE;
std::wstring PLUG_DIR;
std::wstring W_INI;
BOOL cfgopen = false, btaskinited = false, totheleft = false;
bool thumbshowing = false, uninstall = false;
HWND cfgwindow = 0, ratewnd = 0;
Bitmap *background;
MetaData metadata;
HICON iPlay, iPause, iStop;
HIMAGELIST theicons;
ULONG_PTR gdiplusToken;
HHOOK hMouseHook;
int LastTab = 0;

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

//CALLBACKS
VOID CALLBACK TimerProc(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime);
VOID CALLBACK VUMeterProc(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime);
VOID CALLBACK RateTimerProc(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime);
LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
static INT_PTR CALLBACK cfgWndProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
static INT_PTR CALLBACK rateWndProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK TabHandler(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK KeyboardEvent(int nCode, WPARAM wParam, LPARAM lParam);

//THREAD PROCS
void CheckVersion(void *dummy);

HIMAGELIST prepareIcons();
void updateToolbar(bool first);
void ReadSettings();
void WriteSettings();
int InitTaskbar();
void SetWindowAttr();
HBITMAP DrawThumbnail(int width, int height, int force);
std::wstring SearchDir(std::wstring path);
std::wstring MetaWord(std::wstring word);
std::wstring GetLine(int linenumber, bool &center, bool &largefont, bool &forceleft, bool &shadow, bool &darkbox, bool &dontscroll);
inline void SetProgressState(TBPFLAG newstate);
std::wstring getToolTip(int button);
std::wstring getInstallPath();
HWND getWinampWindow();
std::wstring getWinampINIPath(HWND wnd);
std::wstring getBMS();
void DoJl();
HRESULT __CreateShellLink(PCWSTR filename, PCWSTR pszTitle, IShellLink **ppsl);
bool is_in_recent(std::wstring filename);
std::wstring getShortPluginDllPath();

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

int ver = -1;
UINT GetWinampVersion(HWND winamp){
	if(ver == -1){
		return (ver = (int)SendMessage(winamp,WM_WA_IPC,0,IPC_GETVERSION));
	}
	return ver;
}

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
	if(GetWinampVersion(plugin.hwndParent) < 0x5050){
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

			if (SetCurrentProcessExplicitAppUserModelID(APPID) != S_OK)
				MessageBoxEx(plugin.hwndParent,
							 WASABI_API_LNGSTRINGW(IDS_ERROR_SETTING_APPID),
							 BuildPluginNameW(),
							 MB_ICONWARNING | MB_OK, 0);

			W_INI = getWinampINIPath(plugin.hwndParent) + L"\\Plugins\\win7shell.ini";

			ZeroMemory(&Settings, sizeof(Settings));

			Settings.VuMeter = false;
			for (int i=0; i<16; i++)
				Settings.Buttons[i] = false;

			S_lowframerate = GetPrivateProfileInt(L"win7shell", L"lowframerate", 0, W_INI.c_str());

			if (!GetPrivateProfileStruct(__T("win7shell"), __T("settings"), &Settings, sizeof(Settings), W_INI.c_str()))
			{
				sSettings_old sold;
				ZeroMemory(&sold, sizeof(sSettings_old));
				if (GetPrivateProfileStruct(__T("win7shell"), __T("settings"), &sold, sizeof(sold), W_INI.c_str()))
				{
					Settings.Add2RecentDocs = sold.Add2RecentDocs;
					Settings.Antialias = sold.Antialias;
					Settings.AsIcon = sold.AsIcon;
					std::copy(sold.Buttons, sold.Buttons+16, Settings.Buttons);
					Settings.DisableUpdates = sold.DisableUpdates;
					Settings.ForceVersion = sold.ForceVersion;
					Settings.JLbms = sold.JLbms;
					Settings.JLfrequent = sold.JLfrequent;
					Settings.JLpl = true;
					Settings.JLrecent = sold.JLrecent;
					Settings.JLtasks = sold.JLtasks;
					Settings.Overlay = sold.Overlay;
					Settings.Progressbar = sold.Progressbar;
					Settings.RemoveTitle = sold.RemoveTitle;
					Settings.Revertto = sold.Revertto;
					Settings.Shrinkframe = sold.Shrinkframe;
					Settings.Stoppedstatus = sold.Stoppedstatus;
					Settings.Streamstatus = sold.Streamstatus;
					Settings.Thumbnailbackground = sold.Thumbnailbackground;
					Settings.Thumbnailbuttons = sold.Thumbnailbuttons;
					Settings.Thumbnailenabled = sold.Thumbnailenabled;
					Settings.Thumbnailpb = sold.Thumbnailpb;
					Settings.VuMeter = sold.VuMeter;
				}
				else			
				{
					sSettings_old2 sold;
					ZeroMemory(&sold, sizeof(sSettings_old2));
					if (GetPrivateProfileStruct(__T("win7shell"), __T("settings"), &sold, sizeof(sold), W_INI.c_str()))
					{
						Settings.Add2RecentDocs = sold.Add2RecentDocs;
						Settings.Antialias = sold.Antialias;
						Settings.AsIcon = sold.AsIcon;
						std::copy(sold.Buttons, sold.Buttons+16, Settings.Buttons);
						Settings.DisableUpdates = sold.DisableUpdates;
						Settings.ForceVersion = sold.ForceVersion;
						Settings.JLbms = sold.JLbms;
						Settings.JLfrequent = sold.JLfrequent;
						Settings.JLpl = true;
						Settings.JLrecent = sold.JLrecent;
						Settings.JLtasks = sold.JLtasks;
						Settings.Overlay = sold.Overlay;
						Settings.Progressbar = sold.Progressbar;
						Settings.RemoveTitle = sold.RemoveTitle;
						Settings.Revertto = sold.Revertto;
						Settings.Shrinkframe = sold.Shrinkframe;
						Settings.Stoppedstatus = sold.Stoppedstatus;
						Settings.Streamstatus = sold.Streamstatus;
						Settings.Thumbnailbackground = sold.Thumbnailbackground;
						Settings.Thumbnailbuttons = sold.Thumbnailbuttons;
						Settings.Thumbnailenabled = sold.Thumbnailenabled;
						Settings.Thumbnailpb = sold.Thumbnailpb;
						Settings.VuMeter = sold.VuMeter;
						Settings.VolumeControl = true;
					}
					else
					{
						Settings.Thumbnailbackground = 1;
						Settings.Thumbnailbuttons = true;
						Settings.Thumbnailenabled = true;
						Settings.Progressbar = true;
						Settings.Overlay = false;
						Settings.Streamstatus = true;
						Settings.Stoppedstatus = true;
						Settings.Add2RecentDocs = true;
						Settings.Shrinkframe = true;
						Settings.Antialias = true;
						Settings.DisableUpdates = false;
						Settings.RemoveTitle = false;
						Settings.AsIcon = false;
						Settings.VuMeter = false;
						Settings.Buttons[0] = true;
						Settings.Buttons[1] = true;
						Settings.Buttons[2] = true;
						Settings.Buttons[3] = true;
						Settings.Buttons[4] = true;
						Settings.Thumbnailpb = false;
						Settings.Revertto = 0;
						Settings.JLfrequent = true;
						Settings.JLrecent = true;
						Settings.JLtasks = true;
						Settings.JLbms = true;

						std::ofstream of(W_INI.c_str(), std::ios_base::out | std::ios_base::binary);	
						of.write("\xFF\xFE", 2);
						of.close();
					}
				}
			}

			if (Settings.JLrecent || Settings.JLfrequent || Settings.JLtasks || Settings.JLbms)
			{
				DoJl();
			}

			ZeroMemory(&Settings_font, sizeof(Settings_font));
			if (!GetPrivateProfileStruct(__T("win7shell"), __T("font"), &Settings_font, sizeof(Settings_font), W_INI.c_str()))
			{
				LOGFONT ft;
				wcsncpy(ft.lfFaceName, L"Segoe UI", 32);
				ft.lfHeight = 14;
				ft.lfStrikeOut = 0;
				ft.lfUnderline = 0;

				Settings_font.color = RGB(255,255,255);
				Settings_font.bgcolor = RGB(0,0,0);
				Settings_font.font = ft;
			}

			TCHAR path[MAX_PATH];
			GetPrivateProfileString(__T("win7shell"), __T("bgpath"), __T(""), &path[0], MAX_PATH, W_INI.c_str());
			Settings_strings.BGPath = path;

			std::wstring deftext = __T("%c%%curtime% / %totaltime%‡%c%%l%%artist%‡%c%%l%%title%‡%c%%album%‡%c%(#%curpl% of %totalpl%)");

			TCHAR text[1000];	
			GetPrivateProfileString(__T("win7shell"), __T("text"), deftext.c_str(), &text[0], 1000, W_INI.c_str());
			Settings_strings.Text = text;

			if (!Settings.DisableUpdates)
				if (SendMessage(plugin.hwndParent,WM_WA_IPC,0,IPC_INETAVAILABLE))
					_beginthread(CheckVersion, 0, NULL);

			MSG msg;
			while (PeekMessage(&msg, 0,0,0, PM_REMOVE))
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
			
			theicons = prepareIcons();

			if ( (!InitTaskbar()) && (Settings.Thumbnailbuttons) )
			{		
				HRESULT hr = pTBL->ThumbBarSetImageList(plugin.hwndParent, theicons);
				if (SUCCEEDED(hr))
					updateToolbar(true);
			}

			SetWindowAttr();

			GdiplusStartupInput gdiplusStartupInput;    

			GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

			WM_TASKBARBUTTONCREATED = RegisterWindowMessage(L"TaskbarButtonCreated");
			lpWndProcOld = (WNDPROC)SetWindowLongW(plugin.hwndParent,GWLP_WNDPROC,(LONG_PTR)WndProc);

			metadata.setWinampWindow(plugin.hwndParent);

			if (Settings.VuMeter)
				SetTimer(plugin.hwndParent, 6668, 50, VUMeterProc);

			int interval = (S_lowframerate == 1) ? 400 : 100;
			SetTimer(plugin.hwndParent, 6667, interval, TimerProc);		

			volume = IPC_GETVOLUME(plugin.hwndParent);

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
		sResume.ResumePosition = gPlayListPos;
		sResume.ResumeTime = gCurPos;

		DoJl();

		WritePrivateProfileStruct(__T("win7shell"), __T("resume"), &sResume, sizeof(sResume), W_INI.c_str());
	}

	delete background;

	if( pTBL != 0 )
    {
		pTBL->Release();
    }
	
	CoUninitialize();

	GdiplusShutdown(gdiplusToken);
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

std::wstring SecToTime(int sec)
{	
	int ZH = sec / 3600;
	int ZM = sec / 60 - ZH * 60;
	int ZS = sec - (ZH * 3600 + ZM * 60);

	std::wstringstream ss;
	if (ZH != 0)
	{
		if (ZH < 10)
			ss << "0" << ZH << ":";
		else
			ss << ZH << ":";
	}
	
	if (ZM < 10)
		ss << "0" << ZM << ":";
	else
		ss << ZM << ":";

	if (ZS < 10)
		ss << "0" << ZS;
	else
		ss << ZS;

	return ss.str();
}

std::wstring MetaWord(std::wstring word)
{
	if (word == __T("%curtime%"))
	{
		int res = SendMessage(plugin.hwndParent,WM_WA_IPC,0,IPC_GETOUTPUTTIME);
		if (res == -1)
			return __T("~");
		return SecToTime(res/1000);
	}

	if (word == __T("%timeleft%"))
	{
		int cur = SendMessage(plugin.hwndParent,WM_WA_IPC,0,IPC_GETOUTPUTTIME);
		int tot = SendMessage(plugin.hwndParent,WM_WA_IPC,1,IPC_GETOUTPUTTIME);
		if (cur == -1 || tot == -1)
			return __T("~");
		return SecToTime(tot - cur / 1000);
	}

	if (word == __T("%totaltime%"))
	{
		int res = SendMessage(plugin.hwndParent,WM_WA_IPC,1,IPC_GETOUTPUTTIME);
		if (res == -1)
			return __T("");
		return SecToTime(res);
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

	if (word == __T("%volume%"))
	{
		std::wstringstream w;
		w << (volume * 100) /255;
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
		w << (SendMessage(plugin.hwndParent,WM_WA_IPC,0,IPC_GETLISTLENGTH)?gPlayListPos+1:0);
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

	word.erase(0, 1);
	word.erase(word.length()-1, 1);
	return metadata.getMetadata(word);
}

HIMAGELIST prepareIcons()
{
	InitCommonControls();
	
	HIMAGELIST himlIcons;  
    HICON hicon;  
 
	himlIcons = ImageList_Create(GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), ILC_COLOR32, 9, 0); 	

	for (int i = 0; i < 11; ++i)
	{
		hicon = LoadIcon(plugin.hDllInstance, MAKEINTRESOURCE(IDI_ICON1+i)); 

		if (hicon == NULL)
			return NULL;		
		else
			ImageList_AddIcon(himlIcons, hicon);
		// Disabled this for Test 5 as msdn says it's not required if loading an icon via LoadIcon(..)
		//DestroyIcon(hicon);
	}

	return himlIcons;	
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
			button.dwMask = THB_BITMAP | THB_TOOLTIP;
			button.iId = i;

			if (i == 9 || i == 4)
			{
				button.dwMask = button.dwMask | THB_FLAGS;
				button.dwFlags = THBF_DISMISSONCLICK;
			}
			
			if (i == 1)
			{				
				if (playstate == 1)
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

	if (first)
		pTBL->ThumbBarAddButtons(plugin.hwndParent, thbButtons.size(), &thbButtons[0]);
	else
		pTBL->ThumbBarUpdateButtons(plugin.hwndParent, thbButtons.size(), &thbButtons[0]);
}

std::wstring GetLine(int linenumber, bool &center, bool &largefont, bool &forceleft, bool &shadow, bool &darkbox, bool &dontscroll)
{
	std::wstring::size_type pos = 0, open = std::wstring::npos;
	std::wstring text, metaword;
	center = 0;
	int count = 0;
	if (linenumber == 0)
		text = Settings_strings.Text.substr(pos, Settings_strings.Text.find_first_of(NEWLINE, pos));
	else
	{
		do 
		{
			pos = Settings_strings.Text.find_first_of(NEWLINE, pos+1);
			if (pos != std::wstring::npos)
				count++;
		}
		while ( (count < linenumber) && (pos != std::wstring::npos) );

		text = Settings_strings.Text.substr(pos+1, Settings_strings.Text.find_first_of(NEWLINE, pos+1)-pos-1);
	}

	pos = 0;
	do
	{
		pos = text.find_first_of('%', pos);		
		if (pos != std::wstring::npos)
		{
			if (open != std::wstring::npos)
			{
				metaword = MetaWord(text.substr(open, pos-open+1));
				if (metaword == __T("‡center‡"))
				{
					metaword = __T("");
					center = true;
				}
				if (metaword == __T("‡largefont‡"))
				{
					metaword = __T("");
					largefont = true;
				}
				if (metaword == __T("‡forceleft‡"))
				{
					metaword = __T("");
					forceleft = true;
				}
				if (metaword == __T("‡shadow‡"))
				{
					metaword = __T("");
					shadow = true;
				}
				if (metaword == __T("‡darkbox‡"))
				{
					metaword = __T("");
					darkbox = true;
				}
				if (metaword == __T("‡dontscroll‡"))
				{
					metaword = __T("");
					dontscroll = true;
				}

				text.replace(open, pos-open+1, metaword);
				open = std::wstring::npos;
				pos = std::wstring::npos;
			}
			else
				open = pos;
			pos++;
		}
	}
	while (pos!=std::wstring::npos);

	return text;
}

HBITMAP DrawThumbnail(int width, int height, int force)
{	
	int bg;
	(force == -1) ? bg = Settings.Thumbnailbackground : bg = force;

	//Create background
	if (!background)
		switch (bg)
		{
		case 0: //transparent
			background = new Bitmap(width, height, PixelFormat32bppPARGB);
			totheleft = true;
			break;

		case 1: //album art
			{
				ARGB32* cur_image = 0;
				int cur_w = 0, cur_h = 0;
				
				if (AGAVE_API_ALBUMART && AGAVE_API_ALBUMART->GetAlbumArt(metadata.getFileName().c_str(), L"cover", 
					&cur_w, &cur_h, &cur_image) == ALBUMART_SUCCESS)
				{
					BITMAPINFO bmi;
					ZeroMemory(&bmi, sizeof bmi);
					bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
					bmi.bmiHeader.biWidth = cur_w;
					bmi.bmiHeader.biHeight = -cur_h;
					bmi.bmiHeader.biPlanes = 1;
					bmi.bmiHeader.biBitCount = 32;
					bmi.bmiHeader.biCompression = BI_RGB;
					bmi.bmiHeader.biSizeImage = 0;
					bmi.bmiHeader.biXPelsPerMeter = 0;
					bmi.bmiHeader.biYPelsPerMeter = 0;
					bmi.bmiHeader.biClrUsed = 0;
					bmi.bmiHeader.biClrImportant = 0;

					Bitmap tmpbmp(&bmi, cur_image);
					background = new Bitmap(width, height, PixelFormat32bppPARGB);
					Graphics gfx(background);

					gfx.SetSmoothingMode(SmoothingModeNone);
					gfx.SetInterpolationMode(InterpolationModeHighQuality);
					gfx.SetPixelOffsetMode(PixelOffsetModeNone);
					gfx.SetCompositingQuality(CompositingQualityHighSpeed);

					if (Settings.AsIcon)
					{							
						gfx.DrawImage(&tmpbmp, RectF(0, 0, ICONSIZEPX, ICONSIZEPX));
					}
					else
					{
						float ratio = (float)cur_h / (float)(height);
						float widthD = (float)width;
						if (ratio == 0)
							ratio = 1;
						ratio = (float)cur_w / ratio;
						if (ratio > widthD)
							ratio = widthD;
						gfx.DrawImage(&tmpbmp, RectF(((widthD-ratio)/2.0f), 0, ratio, (float)height));
					}

					if (cur_image) 
						WASABI_API_MEMMGR->sysFree(cur_image); 

					totheleft = false;
				}
				else
				{
					AlbumArt AA;
					Bitmap tmpbmp(height, height, PixelFormat32bppPARGB);
					if (AA.getAA(metadata.getFileName(), tmpbmp, height))
					{                                       
						background = new Bitmap(width, height, PixelFormat32bppPARGB);
						Graphics gfx(background);
						gfx.SetInterpolationMode(InterpolationModeHighQualityBilinear);
						if (Settings.AsIcon)
						{                                                       
							gfx.DrawImage(&tmpbmp, RectF(0, 0, ICONSIZEPX, ICONSIZEPX));
						}
						else
							gfx.DrawImage(&tmpbmp, RectF(static_cast<REAL>(((double)width-(double)height)/(double)2),
							0,static_cast<REAL>(height),static_cast<REAL>(height)));        
						totheleft = false;
					}          
					else
						if (force != -1)
						{
							background = new Bitmap(width, height, PixelFormat32bppPARGB);					
							totheleft = true;
						}
						else
							DrawThumbnail(width, height, Settings.Revertto);
				}
			}
			break;

		case 2: //default
			if (Settings.AsIcon)
			{
				Bitmap tmp(plugin.hDllInstance, MAKEINTRESOURCE(IDB_BACKGROUND));
				background = new Bitmap(width, height, PixelFormat32bppPARGB);
				Graphics gfx(background);
				gfx.DrawImage(&tmp, RectF(0, 0, ICONSIZEPX, ICONSIZEPX));
				totheleft = true;
			}
			else
				background = new Bitmap(plugin.hDllInstance, MAKEINTRESOURCE(IDB_BACKGROUND));
			break;

		case 3: //custom
			if (!Settings_strings.BGPath.empty())
			{
				Image img(Settings_strings.BGPath.c_str(), true);
				if (img.GetType() != 0)
				{
					background = new Bitmap(width, height, PixelFormat32bppPARGB);
					Graphics gfx(background);
					gfx.SetInterpolationMode(InterpolationModeHighQualityBilinear);
					if (Settings.AsIcon)
						gfx.DrawImage(&img, RectF(0, 0, ICONSIZEPX, ICONSIZEPX));
					else
						gfx.DrawImage(&img, RectF(0, 0, static_cast<REAL>(width), static_cast<REAL>(height)));
					totheleft = false;
				}
				else
				{		
					if (force != -1)
					{
						totheleft = true;
						background = new Bitmap(width, height, PixelFormat32bppPARGB);	
					}
					else
						DrawThumbnail(width, height, Settings.Revertto);
				}				
			}
			else
			{				
				if (force != -1)
				{
					totheleft = true;
					background = new Bitmap(width, height, PixelFormat32bppPARGB);					
				}
				else
					DrawThumbnail(width, height, Settings.Revertto);
				Settings.Thumbnailbackground = 0;
			}
			break;

		default: 
			totheleft = true;
			background = new Bitmap(width, height, PixelFormat32bppPARGB);
	}

	if (force >= 0)
		return NULL;

	//Draw text
	HBITMAP retbmp;
	int lines = 1;
	HDC DC0 = GetDC(0);

	int textheight=0;
	Font font(DC0, const_cast<LOGFONT*> (&Settings_font.font));
	LONG oldsize = Settings_font.font.lfHeight;
	int x = -((Settings_font.font.lfHeight * 72) / GetDeviceCaps(DC0, LOGPIXELSY));
	x += 4;
	Settings_font.font.lfHeight = -MulDiv(x, GetDeviceCaps(DC0, LOGPIXELSY), 72);
	Font largefont(DC0, const_cast<LOGFONT*> (&Settings_font.font));
	Settings_font.font.lfHeight = oldsize;
	StringFormat sf(StringFormatFlagsNoWrap);

	Graphics tmpgfx(background);

	struct texts {
		std::wstring text;
		int height;
		bool center;
		bool largefont;
		bool forceleft;
		bool shadow;
		bool darkbox;
		bool dontscroll;
	};

	for (std::wstring::size_type i = 0; i < Settings_strings.Text.length(); i++)
		if (Settings_strings.Text[i] == NEWLINE)
			lines++;

	std::vector<texts> TX;
	RectF retrect;

	for (int i=0; i<lines; i++)
	{
		texts tx;
		ZeroMemory(&tx, sizeof(tx));
		tx.text = GetLine(i, tx.center, tx.largefont, tx.forceleft, tx.shadow, tx.darkbox, tx.dontscroll);
		if (tx.largefont)
			tmpgfx.MeasureString(tx.text.c_str(), -1, &largefont, RectF(0, 0, static_cast<REAL>(width), static_cast<REAL>(height)), &sf, &retrect);
		else
			tmpgfx.MeasureString(tx.text.c_str(), -1, &font, RectF(0, 0, static_cast<REAL>(width), static_cast<REAL>(height)), &sf, &retrect);

		if (retrect.GetBottom() == 0)
			tmpgfx.MeasureString(L"QWEXCyjM", -1, &font, RectF(0, 0, static_cast<REAL>(width), static_cast<REAL>(height)), &sf, &retrect);			
		
		tx.height = static_cast<int>(retrect.GetBottom());
		textheight += tx.height;
		TX.push_back(tx);
	}	

	if (!Settings.Shrinkframe)	
		textheight = height-2;
		
	if (Settings.Thumbnailpb)
		textheight += 25;

	if (textheight > height-2)
		textheight = height-2;

	Bitmap bmp(background->GetWidth(), textheight+2, PixelFormat32bppPARGB);
	Graphics gfx(&bmp);
	
	gfx.SetSmoothingMode(SmoothingModeNone);
	gfx.SetInterpolationMode(InterpolationModeNearestNeighbor);
	gfx.SetPixelOffsetMode(PixelOffsetModeNone);
	gfx.SetCompositingQuality(CompositingQualityHighSpeed);

	if (Settings.AsIcon)
		gfx.DrawImage(background, RectF(0, 0, static_cast<REAL>(background->GetWidth()), static_cast<REAL>(background->GetHeight())));
	else
		gfx.DrawImage(background, RectF(0, 0, static_cast<REAL>(background->GetWidth()), static_cast<REAL>(textheight+2)));

	if ( (Settings.Thumbnailbackground == 0) || (Settings.Thumbnailbackground == 1) )
		(Settings.Antialias) ? gfx.SetTextRenderingHint(TextRenderingHintAntiAlias) : gfx.SetTextRenderingHint(TextRenderingHintAntiAliasGridFit);
	else	
		(Settings.Antialias) ? gfx.SetTextRenderingHint(TextRenderingHintAntiAlias) : gfx.SetTextRenderingHint(TextRenderingHintClearTypeGridFit);		

	SolidBrush bgcolor(Color(GetRValue(Settings_font.bgcolor), GetGValue(Settings_font.bgcolor), GetBValue(Settings_font.bgcolor)));
	SolidBrush fgcolor(Color(GetRValue(Settings_font.color), GetGValue(Settings_font.color), GetBValue(Settings_font.color)));

	int heightsofar=0;
	for (int i=0; i < lines; i++)
	{
		sf.SetAlignment(StringAlignmentNear);
		int left = 2;
		if (Settings.AsIcon && !totheleft && !TX[i].forceleft)
			left = ICONSIZEPX + 4;

		if (i)
			heightsofar += TX[i-1].height;

		PointF point(static_cast<REAL>(left), static_cast<REAL>(heightsofar-1));
		if (TX[i].largefont)
			gfx.MeasureString(TX[i].text.c_str(), -1, &largefont, point, &sf, &retrect);
		else
			gfx.MeasureString(TX[i].text.c_str(), -1, &font, point, &sf, &retrect);

		bool longtext = false;
		if (left + retrect.Width - 1 > width && !TX[i].dontscroll)
		{
			TX[i].text += L"     ";
			size_t pos = (step > 9) ? (step - 9) % TX[i].text.length() : 0;
			std::wstring tmp = TX[i].text.substr(0, pos);
			TX[i].text.erase(0, pos);
			TX[i].text += tmp;
			longtext = true;
		}
		else
		{
			sf.SetAlignment(static_cast<StringAlignment>(TX[i].center));
			if (TX[i].center)
				if (Settings.AsIcon && !TX[i].forceleft && !totheleft)
					retrect.X = (((REAL)(196 - ICONSIZEPX) / (REAL)2) - ((REAL)retrect.Width / (REAL)2)) + ICONSIZEPX;
				else
				{
					retrect.X = 98 - ((REAL)retrect.Width / 2);
					if (retrect.X <= 0)
						retrect.X = (float)left;
				}
			if (Settings.AsIcon && retrect.X < (ICONSIZEPX + 4) && !totheleft && !TX[i].forceleft)
				retrect.X = ICONSIZEPX + 4;
		}

		if (TX[i].darkbox)
		{			
			SolidBrush boxbrush(Color::MakeARGB(120, GetRValue(Settings_font.bgcolor),
				GetGValue(Settings_font.bgcolor), GetBValue(Settings_font.bgcolor)));

			retrect.Y += 2;
			retrect.Height -= 1;
			gfx.FillRectangle(&boxbrush, retrect);
		}

		if (TX[i].shadow)
		{		
			PointF shadowpt(static_cast<REAL>(left+1), static_cast<REAL>(heightsofar));
			RectF shadowrect(retrect.X + 1, retrect.Y + 1, retrect.Width, retrect.Height);
			if (TX[i].largefont)
			{
				if (longtext)
					gfx.DrawString(TX[i].text.c_str(), -1, &largefont, shadowpt, &sf, &bgcolor);
				else
					gfx.DrawString(TX[i].text.c_str(), -1, &largefont, shadowrect, &sf, &bgcolor);					
			}
			else
			{
				if (longtext)
					gfx.DrawString(TX[i].text.c_str(), -1, &font, shadowpt, &sf, &bgcolor);
				else
					gfx.DrawString(TX[i].text.c_str(), -1, &font, shadowrect, &sf, &bgcolor);						
			}
		}

		if (TX[i].largefont)
		{
			if (longtext)
				gfx.DrawString(TX[i].text.c_str(), -1, &largefont, point, &sf, &fgcolor);
			else
				gfx.DrawString(TX[i].text.c_str(), -1, &largefont, retrect, &sf, &fgcolor);
		}
		else
		{
			if (longtext)
				gfx.DrawString(TX[i].text.c_str(), -1, &font, point, &sf, &fgcolor);
			else
				gfx.DrawString(TX[i].text.c_str(), -1, &font, retrect, &sf, &fgcolor);
		}
	}

	if (gTotal > 0 && gCurPos >0 && Settings.Thumbnailpb)
	{
		gfx.SetSmoothingMode(SmoothingModeAntiAlias);		
		int Y = bmp.GetHeight()-15;
		Pen p1(Color::White, 1);
		Pen p2(Color::MakeARGB(80, 0, 0, 0), 1);

		SolidBrush b1(Color::MakeARGB(150, 0, 0, 0));	
		SolidBrush b2(Color::MakeARGB(220, 255, 255, 255));
		RectF R1(44, (REAL)Y, 10, 9);
		RectF R2((REAL)width - 56, (REAL)Y, 10, 9);

		gfx.FillRectangle(&b1, 46, Y, width-94, 9);
		gfx.DrawLine(&p2, 46, Y+2, 46, Y+6);
		gfx.DrawLine(&p2, width-48, Y+2, width-48, Y+6);

		gfx.DrawArc(&p1, R1, -90.0f, -180.0f);
		gfx.DrawArc(&p1, R2, 90.0f, -180.0f);
		
		gfx.DrawLine(&p1, 48, Y, width-49, Y);
		gfx.DrawLine(&p1, 48, Y+9, width-49, Y+9);

		gfx.SetSmoothingMode(SmoothingModeDefault);
		gfx.FillRectangle(&b2, 48, Y+3, (gCurPos*(width-96))/gTotal, 4);
	}
	
	bmp.GetHBITMAP(Color::Black, &retbmp);

	ReleaseDC(0, DC0);

	return retbmp;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	if(message == WM_TASKBARBUTTONCREATED)
	{
		btaskinited = false;
		if ( (!InitTaskbar()) && (Settings.Thumbnailbuttons) )
		{		
			HRESULT hr = pTBL->ThumbBarSetImageList(plugin.hwndParent, theicons);
			if (SUCCEEDED(hr))
				updateToolbar(true);
		}
		DoJl();
		SetWindowAttr();
	}

	switch (message)
	{
	case WM_DWMSENDICONICLIVEPREVIEWBITMAP:
		{
			thumbshowing = true;
			RECT rect;
			GetWindowRect(plugin.hwndParent, &rect);
			Bitmap bmp(rect.right-rect.left-1, rect.bottom-rect.top-1, PixelFormat32bppPARGB);
			StringFormat sf(StringFormatFlagsNoWrap);
			sf.SetAlignment(StringAlignmentCenter);
			Font font(L"Segoe UI", 18);
			SolidBrush brush(Color::MakeARGB(200, 0, 0, 0));
			std::wstringstream ss;
			ss << L"Volume: " << ((volume * 100) /255) << L"%\n" << metadata.getMetadata(L"title");
			ss << L"\n" << metadata.getMetadata(L"artist");

			Graphics gfx(&bmp);
			gfx.SetTextRenderingHint(TextRenderingHintAntiAlias);

			gfx.DrawString(ss.str().c_str(), -1, &font, RectF(0, 0, (float)(rect.right-rect.left-1), (float)(rect.bottom-rect.top-1)), &sf, &brush);
			HBITMAP hbmp;
			bmp.GetHBITMAP(Color::Black, &hbmp);
			
			POINT pt;
			pt.x = 0;
			pt.y = 0;

			DwmSetIconicLivePreviewBitmap(plugin.hwndParent, hbmp, &pt, 0);
			DeleteObject(hbmp);
			return 0;
		}
		break;
	case WM_DWMSENDICONICTHUMBNAIL:
		{
			if (!thumbshowing)
			{
				thumbshowing = true;
				step  = 0;
			}

			HBITMAP thumbnail = DrawThumbnail(HIWORD(lParam), LOWORD(lParam), -1);	
			if (Settings.Shrinkframe)
				Sleep(50);

			HRESULT hr = DwmSetIconicThumbnail(plugin.hwndParent, thumbnail, 0);
			if (FAILED(hr))
			{
				MessageBoxEx(plugin.hwndParent,
							 WASABI_API_LNGSTRINGW(IDS_ERROR_SETTING_THUMBNAIL),
							 BuildPluginNameW(), MB_ICONERROR, 0);
				Settings.Thumbnailenabled = false;
				SetWindowAttr();
			}
			DeleteObject(thumbnail);
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
					gPlayListPos = SendMessage(plugin.hwndParent,WM_WA_IPC,0,IPC_GETLISTPOS);
					if (background)
					{
						delete background;
						background = 0;
					}
					if (playstate != 1)
						DwmInvalidateIconicBitmaps(plugin.hwndParent);
					int index = SendMessage(plugin.hwndParent,WM_WA_IPC,0,IPC_GETLISTPOS);
					wchar_t *p = (wchar_t*)SendMessage(plugin.hwndParent,WM_WA_IPC,index,IPC_GETPLAYLISTFILEW); 
					if (p != NULL)
						metadata.reset(p, false);
					break;
				}

			case 1:
				{
					int res = SendMessage(plugin.hwndParent,WM_WA_IPC,0,IPC_ISPLAYING);
					if (res == 1)
						SendMessage(plugin.hwndParent, WM_COMMAND, MAKEWPARAM(40046,0), 0);									
					else					
						SendMessage(plugin.hwndParent, WM_COMMAND, MAKEWPARAM(40045,0), 0);					
					playstate = (char)res;
				}
				break;

			case 2:
				SendMessage(plugin.hwndParent, WM_COMMAND, MAKEWPARAM(40047,0), 0);
				playstate = 3; //stopped
				break;

			case 3:
				{				
					SendMessage(plugin.hwndParent, WM_COMMAND, MAKEWPARAM(40048,0), 0);
					gPlayListPos = SendMessage(plugin.hwndParent,WM_WA_IPC,0,IPC_GETLISTPOS);
					if (background)
					{
						delete background;
						background = 0;
					}
					if (playstate != 1)
						DwmInvalidateIconicBitmaps(plugin.hwndParent);
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
					SetTimer(plugin.hwndParent, 6669, 5000, RateTimerProc);
					ShowWindow(ratewnd, SW_SHOW);
				}
				break;

			case 5:
				volume -= 25;
				if (volume < 0)
					volume = 0;
				SendMessage(plugin.hwndParent,WM_WA_IPC,volume,IPC_SETVOLUME);
				break;

			case 6:
				volume += 25;
				if (volume > 255)
					volume = 255;
				SendMessage(plugin.hwndParent,WM_WA_IPC,volume,IPC_SETVOLUME);
				break;

			case 7:
				SendMessage(plugin.hwndParent,WM_WA_IPC,(WPARAM)(HWND)0,IPC_OPENFILEBOX);
				break;

			case 8:
				static int lastvolume;
				if (volume == 0)
				{					
					volume = lastvolume;
					SendMessage(plugin.hwndParent,WM_WA_IPC,volume,IPC_SETVOLUME);
				}
				else
				{
					lastvolume = volume;
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

					gPlayListPos = SendMessage(plugin.hwndParent,WM_WA_IPC,0,IPC_GETLISTPOS);

					gTotal = SendMessage(plugin.hwndParent,WM_WA_IPC,2,IPC_GETOUTPUTTIME);
					gCurPos = 0;

					DwmInvalidateIconicBitmaps(plugin.hwndParent);

					playstate = 1;

					if ((Settings.JLrecent || Settings.JLfrequent) && !is_in_recent(filename))
					{
						std::wstring title(metadata.getMetadata(L"title") + L" - " + 
							metadata.getMetadata(L"artist"));

						if (gTotal > 0)
							title += L"  (" + SecToTime(gTotal/1000) + L")";						

						IShellLink * psl;
						SHARDAPPIDINFOLINK applink;						
						if (/*true_msg  && */__CreateShellLink(filename.c_str(), title.c_str(), &psl) == S_OK)
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

					if (Settings.Thumbnailbackground == 1)
					{
						if (background)
						{
							delete background;
							background = 0;
						}
					}

					step = 0;
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
									pTBL->SetOverlayIcon(plugin.hwndParent, icon,
														 WASABI_API_LNGSTRINGW_BUF(IDS_PLAYING,tmp,64));
									DestroyIcon(icon);
								}
								break;
							case 3:
								{
									HICON icon = ImageList_GetIcon(theicons, 2, 0);
									pTBL->SetOverlayIcon(plugin.hwndParent, icon,
														 WASABI_API_LNGSTRINGW_BUF(IDS_PAUSED,tmp,64));
									DestroyIcon(icon);
								}
								break;
							default:
								{
									HICON icon = ImageList_GetIcon(theicons, 3, 0);
									pTBL->SetOverlayIcon(plugin.hwndParent, icon,
														 WASABI_API_LNGSTRINGW_BUF(IDS_STOPPED,tmp,64));
									DestroyIcon(icon);
								}								
							}
						}
						break;

					case IPC_CB_MISC_VOLUME:
						volume = IPC_GETVOLUME(plugin.hwndParent);
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

	LRESULT ret = CallWindowProc(lpWndProcOld,hwnd,message,wParam,lParam);	 

	switch(message)
	{
		case WM_WA_IPC:
			switch (lParam)
			{
			case IPC_ADDBOOKMARK:
			case IPC_ADDBOOKMARKW:
				{
					if(wParam) DoJl();
				}
				break;
			}
			break;
	}

	return ret;
}

bool is_in_recent(std::wstring filename)
{
	std::wstring path;
	path.resize(MAX_PATH);
	SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, 0, &path[0]);
	path.resize(wcslen(path.c_str()));

	std::wstring::size_type pos = filename.find_last_of(L"\\");
	if (pos != std::wstring::npos)
		filename.erase(0, pos+1);

	path += L"\\Microsoft\\Windows\\Recent\\" + filename + L".lnk";

	WIN32_FIND_DATA ffd;
	if (FindFirstFile(path.c_str(), &ffd) == INVALID_HANDLE_VALUE)
		return false;
	return true;
}

inline void SetProgressState(TBPFLAG newstate)
{
	static TBPFLAG progressbarstate = TBPF_NOPROGRESS;
	if (newstate != progressbarstate)
	{
		pTBL->SetProgressState(plugin.hwndParent, newstate);
		progressbarstate = newstate;
	}
}

VOID CALLBACK RateTimerProc(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime)
{
	if (ratewnd != NULL)
		DestroyWindow(ratewnd);
	KillTimer(plugin.hwndParent, 6669);
}

VOID CALLBACK VUMeterProc(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime)
{
	if (Settings.VuMeter)
	{
		static int (*export_sa_get)(int channel) = (int(__cdecl *)(int)) SendMessage(plugin.hwndParent, WM_WA_IPC, 0, IPC_GETVUDATAFUNC);
		int audiodata = export_sa_get(0);

		if (playstate != 1 || audiodata == -1)
		{
			SetProgressState(TBPF_NOPROGRESS);
		}
		else
		{
			if (audiodata <= 150)
				SetProgressState(TBPF_NORMAL);
			else if (audiodata > 150 && audiodata < 210)
				SetProgressState(TBPF_PAUSED);
			else
				SetProgressState(TBPF_ERROR);		

			pTBL->SetProgressValue(plugin.hwndParent, audiodata, 255);
		}
	}
}

VOID CALLBACK TimerProc(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime)
{
	if (thumbshowing)
	{
		POINT pt;
		GetCursorPos(&pt);
		std::wstring name;
		name.resize(200);
		GetClassName(WindowFromPoint(pt), &name[0], 200);
		name.resize(wcslen(name.c_str()));
		if (name != L"TaskListThumbnailWnd" && name != L"MSTaskListWClass")
		{
			thumbshowing = false;			
		}

		DwmInvalidateIconicBitmaps(plugin.hwndParent);
	}

	step += (S_lowframerate == 1) ? 2 : 1;

	if (playstate == -1)
	{
 		WndProc(plugin.hwndParent, WM_WA_IPC, (WPARAM)L"", IPC_PLAYING_FILEW);
		WndProc(plugin.hwndParent, WM_WA_IPC, IPC_CB_MISC_STATUS, IPC_CB_MISC);
	}

	if (!(Settings.Progressbar || Settings.VuMeter))
		SetProgressState(TBPF_NOPROGRESS);	

	if (playstate == 1 || Settings.Thumbnailpb)
		if (gTotal <= 0)
			gTotal = SendMessage(plugin.hwndParent,WM_WA_IPC,1,IPC_GETOUTPUTTIME) * 1000;

	int cp = SendMessage(plugin.hwndParent,WM_WA_IPC,0,IPC_GETOUTPUTTIME);
	if (gCurPos == cp)
	{
		return;
	}
	gCurPos = cp;

	DwmInvalidateIconicBitmaps(plugin.hwndParent);

	switch (playstate)
	{
		case 1: 

			if (gCurPos == -1 || gTotal <= 0)
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
					if (gCurPos == -1 || gTotal <= 0)
					{ 
						if (Settings.Streamstatus)
							SetProgressState(TBPF_INDETERMINATE);
						else						
							SetProgressState(TBPF_NOPROGRESS);
					}
					else
					{
						SetProgressState(TBPF_NORMAL);
						pTBL->SetProgressValue(plugin.hwndParent, gCurPos, gTotal);	
					}
				}
			break;

		case 3: 
				if (Settings.Progressbar)
				{
					SetProgressState(TBPF_PAUSED);
					pTBL->SetProgressValue(plugin.hwndParent, gCurPos, gTotal);	

					if (SendMessage(plugin.hwndParent,WM_WA_IPC,1,IPC_GETOUTPUTTIME) == -1)
						pTBL->SetProgressValue(plugin.hwndParent, 100, 100);
				}
			break;

		default:

			if (Settings.Progressbar) 
			{
				if (Settings.Stoppedstatus)
				{
					SetProgressState(TBPF_ERROR);
					pTBL->SetProgressValue(plugin.hwndParent, 100, 100);
				}
				else
					SetProgressState(TBPF_NOPROGRESS);
			}
			break;
	}
}

void SetWindowAttr()
{
	DwmInvalidateIconicBitmaps(plugin.hwndParent);
	BOOL b = Settings.Thumbnailenabled;	
		
	DwmSetWindowAttribute(plugin.hwndParent, DWMWA_HAS_ICONIC_BITMAP, &b, sizeof(b));
	DwmSetWindowAttribute(plugin.hwndParent, DWMWA_FORCE_ICONIC_REPRESENTATION, &b, sizeof(b));	
}

int InitTaskbar()
{
	if (btaskinited)
		return 0;

	if ( (Settings.Progressbar) || (Settings.Overlay) || (Settings.Thumbnailbuttons) )
	{
		CoInitialize(0);
		HRESULT hr = ::CoCreateInstance(CLSID_TaskbarList, NULL, CLSCTX_INPROC_SERVER,
			IID_ITaskbarList, reinterpret_cast<void**>(&pTBL) );

		if (!SUCCEEDED(hr))
		{
			MessageBoxEx(plugin.hwndParent,
						 WASABI_API_LNGSTRINGW(IDS_TASKBAR_INITIALIZE_FAILURE),
						 BuildPluginNameW(), MB_ICONSTOP, 0);
			return -1;
		}

		hr = pTBL->HrInit();
		if (!SUCCEEDED(hr))
		{
			MessageBoxEx(plugin.hwndParent,
						 WASABI_API_LNGSTRINGW(IDS_TASKBAR_INITIALIZE_FAILURE),
						 BuildPluginNameW(), MB_ICONSTOP, 0);
			return -1;
		}

		btaskinited = true;
		return 0;
	}
	return 0;
}

void ReadSettings(HWND hwnd)
{
	//Aero peek settings
	switch (Settings.Thumbnailbackground)
	{
	case 0:
		SendMessage(GetDlgItem(hwnd, IDC_RADIO1), (UINT) BM_SETCHECK, BST_CHECKED , 0);
		break;
	case 1:
		SendMessage(GetDlgItem(hwnd, IDC_RADIO2), (UINT) BM_SETCHECK, BST_CHECKED , 0);
		break;
	case 2:
		SendMessage(GetDlgItem(hwnd, IDC_RADIO3), (UINT) BM_SETCHECK, BST_CHECKED , 0);
		break;
	case 3:
		SendMessage(GetDlgItem(hwnd, IDC_RADIO4), (UINT) BM_SETCHECK, BST_CHECKED , 0);
		break;
	}

	SendMessage(GetDlgItem(hwnd, IDC_COMBO1), CB_ADDSTRING, 0, (LPARAM)WASABI_API_LNGSTRINGW(IDS_TRANSPARENT));
	SendMessage(GetDlgItem(hwnd, IDC_COMBO1), CB_ADDSTRING, 0, (LPARAM)WASABI_API_LNGSTRINGW(IDS_ALBUM_ART));
	SendMessage(GetDlgItem(hwnd, IDC_COMBO1), CB_ADDSTRING, 0, (LPARAM)WASABI_API_LNGSTRINGW(IDS_DEFAULT_BACKGROUND));
	SendMessage(GetDlgItem(hwnd, IDC_COMBO1), CB_ADDSTRING, 0, (LPARAM)WASABI_API_LNGSTRINGW(IDS_CUSTOM_BACKGROUND));

	SendMessage(GetDlgItem(hwnd, IDC_COMBO1), CB_SETCURSEL, Settings.Revertto, 0);

	//Player control buttons
	SendMessage(GetDlgItem(hwnd, IDC_CHECK6), (UINT) BM_SETCHECK, Settings.Thumbnailbuttons, 0);

	//Progressbar
	SendMessage(GetDlgItem(hwnd, IDC_CHECK2), (UINT) BM_SETCHECK, Settings.Progressbar, 0);
	
	//Streamstatus
	SendMessage(GetDlgItem(hwnd, IDC_CHECK4), (UINT) BM_SETCHECK, Settings.Streamstatus, 0);

	//Stoppedstatus
	SendMessage(GetDlgItem(hwnd, IDC_CHECK5), (UINT) BM_SETCHECK, Settings.Stoppedstatus, 0);

	//Overlay
	SendMessage(GetDlgItem(hwnd, IDC_CHECK3), (UINT) BM_SETCHECK, Settings.Overlay, 0);

	//antialias
	SendMessage(GetDlgItem(hwnd, IDC_CHECK8), (UINT) BM_SETCHECK, Settings.Antialias, 0);
	
	//shrinkframe
	SendMessage(GetDlgItem(hwnd, IDC_CHECK1), (UINT) BM_SETCHECK, Settings.Shrinkframe, 0);

	//enable thumbnail
	SendMessage(GetDlgItem(hwnd, IDC_CHECK7), (UINT) BM_SETCHECK, Settings.Thumbnailenabled, 0);

	//disable updated
	SendMessage(GetDlgItem(hwnd, IDC_CHECK9), (UINT) BM_SETCHECK, Settings.DisableUpdates, 0);

	//remove winamp title
	SendMessage(GetDlgItem(hwnd, IDC_CHECK10), (UINT) BM_SETCHECK, Settings.RemoveTitle, 0);

	//Show image as icon
	SendMessage(GetDlgItem(hwnd, IDC_CHECK25), (UINT) BM_SETCHECK, Settings.AsIcon, 0);

	//VU Meter
	SendMessage(GetDlgItem(hwnd, IDC_CHECK26), (UINT) BM_SETCHECK, Settings.VuMeter, 0);

	//Thumbnail pb
	SendMessage(GetDlgItem(hwnd, IDC_CHECK29), (UINT) BM_SETCHECK, Settings.Thumbnailpb, 0);

	SendMessage(GetDlgItem(hwnd, IDC_CHECK30), (UINT) BM_SETCHECK, Settings.JLrecent, 0);
	SendMessage(GetDlgItem(hwnd, IDC_CHECK31), (UINT) BM_SETCHECK, Settings.JLfrequent, 0);
	SendMessage(GetDlgItem(hwnd, IDC_CHECK32), (UINT) BM_SETCHECK, Settings.JLtasks, 0);
	SendMessage(GetDlgItem(hwnd, IDC_CHECK33), (UINT) BM_SETCHECK, Settings.JLbms, 0);
	SendMessage(GetDlgItem(hwnd, IDC_CHECK34), (UINT) BM_SETCHECK, Settings.JLpl, 0);

	SendMessage(GetDlgItem(hwnd, IDC_CHECK35), (UINT) BM_SETCHECK, Settings.VolumeControl, 0);
	SendMessage(GetDlgItem(hwnd, IDC_CHECK36), (UINT) BM_SETCHECK, static_cast<WPARAM>(S_lowframerate), 0);

	SetWindowText(GetDlgItem(hwnd, IDC_EDIT2), Settings_strings.BGPath.c_str());

	std::wstring tmpbuf = Settings_strings.Text;
	std::wstring::size_type pos = std::wstring::npos;
	do 
	{
		pos = tmpbuf.find(__T("‡"));
		if (pos != std::wstring::npos)
			tmpbuf.replace(pos, 1, __T("\xD\xA"));
	}
	while (pos != std::wstring::npos);
	
	SetWindowText(GetDlgItem(hwnd, IDC_EDIT3), tmpbuf.c_str());
	
	EnableWindow(GetDlgItem(hwnd, IDC_CHECK4), Settings.Progressbar);
	EnableWindow(GetDlgItem(hwnd, IDC_CHECK5), Settings.Progressbar);
	EnableWindow(GetDlgItem(hwnd, IDC_CHECK27), Settings.Thumbnailbuttons);

	for (int i = IDC_PCB1; i <= IDC_PCB10; i++)
		SendMessage(GetDlgItem(hwnd, i), (UINT) BM_SETCHECK, Settings.Buttons[i-IDC_PCB1], 0);
}

void WriteSettings(HWND hwnd)
{
	//Checkboxes
	if (GetDlgItem(hwnd, IDC_CHECK3) != NULL)
		Settings.Overlay = (Button_GetCheck(GetDlgItem(hwnd, IDC_CHECK3)) == BST_CHECKED);
	if (GetDlgItem(hwnd, IDC_CHECK2) != NULL)
		Settings.Progressbar = (Button_GetCheck(GetDlgItem(hwnd, IDC_CHECK2)) == BST_CHECKED);
	if (GetDlgItem(hwnd, IDC_CHECK4) != NULL)
		Settings.Streamstatus = (Button_GetCheck(GetDlgItem(hwnd, IDC_CHECK4)) == BST_CHECKED);
	if (GetDlgItem(hwnd, IDC_CHECK5) != NULL)
		Settings.Stoppedstatus = (Button_GetCheck(GetDlgItem(hwnd, IDC_CHECK5)) == BST_CHECKED);
	if (GetDlgItem(hwnd, IDC_CHECK6) != NULL)
		Settings.Thumbnailbuttons = (Button_GetCheck(GetDlgItem(hwnd, IDC_CHECK6)) == BST_CHECKED);
	if (GetDlgItem(hwnd, IDC_CHECK7) != NULL)
		Settings.Thumbnailenabled = (Button_GetCheck(GetDlgItem(hwnd, IDC_CHECK7)) == BST_CHECKED);
	if (GetDlgItem(hwnd, IDC_CHECK8) != NULL)
		Settings.Antialias = (Button_GetCheck(GetDlgItem(hwnd, IDC_CHECK8)) == BST_CHECKED);
	if (GetDlgItem(hwnd, IDC_CHECK1) != NULL)
		Settings.Shrinkframe = (Button_GetCheck(GetDlgItem(hwnd, IDC_CHECK1)) == BST_CHECKED);
	if (GetDlgItem(hwnd, IDC_CHECK9) != NULL)
		Settings.DisableUpdates = (Button_GetCheck(GetDlgItem(hwnd, IDC_CHECK9)) == BST_CHECKED);
	if (GetDlgItem(hwnd, IDC_CHECK10) != NULL)
	{
		Settings.RemoveTitle = (Button_GetCheck(GetDlgItem(hwnd, IDC_CHECK10)) == BST_CHECKED);
		// do this all so that the title can be cleared or reset as required on-the-fly
		if(Settings.RemoveTitle)
		{
			SetWindowText(plugin.hwndParent,L"");
		}
		else
		{
			wchar_t tmp[1024] = {0};
			GetWindowText(plugin.hwndParent,tmp,1024);
			SetWindowText(plugin.hwndParent,tmp);
		}
	}
	if (GetDlgItem(hwnd, IDC_CHECK25) != NULL)
		Settings.AsIcon = (Button_GetCheck(GetDlgItem(hwnd, IDC_CHECK25)) == BST_CHECKED);
	if (GetDlgItem(hwnd, IDC_CHECK26) != NULL)
		Settings.VuMeter = (Button_GetCheck(GetDlgItem(hwnd, IDC_CHECK26)) == BST_CHECKED);
	if (GetDlgItem(hwnd, IDC_CHECK29) != NULL)
		Settings.Thumbnailpb = (Button_GetCheck(GetDlgItem(hwnd, IDC_CHECK29)) == BST_CHECKED);
	if (GetDlgItem(hwnd, IDC_CHECK30) != NULL)
		Settings.JLrecent = (Button_GetCheck(GetDlgItem(hwnd, IDC_CHECK30)) == BST_CHECKED);
	if (GetDlgItem(hwnd, IDC_CHECK31) != NULL)
		Settings.JLfrequent = (Button_GetCheck(GetDlgItem(hwnd, IDC_CHECK31)) == BST_CHECKED);
	if (GetDlgItem(hwnd, IDC_CHECK32) != NULL)
		Settings.JLtasks = (Button_GetCheck(GetDlgItem(hwnd, IDC_CHECK32)) == BST_CHECKED);
	if (GetDlgItem(hwnd, IDC_CHECK33) != NULL)
		Settings.JLbms = (Button_GetCheck(GetDlgItem(hwnd, IDC_CHECK33)) == BST_CHECKED);
	if (GetDlgItem(hwnd, IDC_CHECK34) != NULL)
		Settings.JLpl = (Button_GetCheck(GetDlgItem(hwnd, IDC_CHECK34)) == BST_CHECKED);
	if (GetDlgItem(hwnd, IDC_CHECK35) != NULL)
		Settings.VolumeControl = (Button_GetCheck(GetDlgItem(hwnd, IDC_CHECK35)) == BST_CHECKED);
	if (GetDlgItem(hwnd, IDC_CHECK36) != NULL)
		S_lowframerate = (Button_GetCheck(GetDlgItem(hwnd, IDC_CHECK36)) == BST_CHECKED);

	//Radiobuttons
	if (GetDlgItem(hwnd, IDC_RADIO1) != NULL)
	{
		if ( SendMessage(GetDlgItem(hwnd, IDC_RADIO1), (UINT) BM_GETCHECK, 0 , 0) )
			Settings.Thumbnailbackground = 0;
		else if ( SendMessage(GetDlgItem(hwnd, IDC_RADIO2), (UINT) BM_GETCHECK, 0 , 0) )
			Settings.Thumbnailbackground = 1;
		else if ( SendMessage(GetDlgItem(hwnd, IDC_RADIO3), (UINT) BM_GETCHECK, 0 , 0) )
			Settings.Thumbnailbackground = 2;
		else if ( SendMessage(GetDlgItem(hwnd, IDC_RADIO4), (UINT) BM_GETCHECK, 0 , 0) )
			Settings.Thumbnailbackground = 3;
	}

	if (GetDlgItem(hwnd, IDC_COMBO1) != NULL)
		Settings.Revertto = (char)SendMessage(GetDlgItem(hwnd, IDC_COMBO1), CB_GETCURSEL, 0, 0);

	if (GetDlgItem(hwnd, IDC_EDIT2) != NULL)
	{
		int size = GetWindowTextLength(GetDlgItem(hwnd, IDC_EDIT2));
		std::vector<TCHAR> buf1(size+1);
		GetWindowText(GetDlgItem(hwnd, IDC_EDIT2), &buf1[0], size+1);	
		Settings_strings.BGPath = &buf1[0];
	}

	if (GetDlgItem(hwnd, IDC_EDIT3) != NULL)
	{
		int size = GetWindowTextLength(GetDlgItem(hwnd, IDC_EDIT3));
		std::vector<TCHAR> buf2(size+1);
		GetWindowText(GetDlgItem(hwnd, IDC_EDIT3), &buf2[0], size+1);	
		Settings_strings.Text = &buf2[0];

		std::wstring::size_type pos = std::wstring::npos;
		do 
		{
			pos = Settings_strings.Text.find(__T("\xD\xA"));
			if (pos != std::wstring::npos)
				Settings_strings.Text.replace(pos, 2, __T("‡"));
		}
		while (pos != std::wstring::npos);
	}

	if (GetDlgItem(hwnd, IDC_PCB1) != NULL)
	{
		for (int i = IDC_PCB1; i <= IDC_PCB10; i++)
			Settings.Buttons[i-IDC_PCB1] = (Button_GetCheck(GetDlgItem(hwnd, i)) == BST_CHECKED);
	}
}

std::wstring GetFileName(HWND hwnd)
{
    IFileDialog *pfd;
    
    // CoCreate the dialog object.
    HRESULT hr = CoCreateInstance(CLSID_FileOpenDialog, 
                                  NULL, 
                                  CLSCTX_INPROC_SERVER, 
                                  IID_PPV_ARGS(&pfd));
    
    if (SUCCEEDED(hr))
    {
        // Show the dialog
		wchar_t tmp[128], tmp2[128], tmp3[128];
		COMDLG_FILTERSPEC rgSpec[] =
		{ 			
			{ WASABI_API_LNGSTRINGW_BUF(IDS_ALL_IMAGE_FORMATS,tmp,128),
			 __T("*.bmp;*.dib;*.jpg;*.jpeg;*.gif;*.png;*.ico;*.tiff;*.tif;*.wmf;*.emf;*.exif")
			}
		};

		pfd->SetFileTypes(1, rgSpec);
		pfd->SetOkButtonLabel(WASABI_API_LNGSTRINGW_BUF(IDS_USE_AS_THUMB_BKGND,tmp2,128));
		pfd->SetTitle(WASABI_API_LNGSTRINGW_BUF(IDS_SELECT_IMAGE_FILE,tmp3,128));
        hr = pfd->Show(hwnd);
        
        if (SUCCEEDED(hr))
        {
            // Obtain the result of the user's interaction with the dialog.
            IShellItem *psiResult;
            hr = pfd->GetResult(&psiResult);
            
            if (SUCCEEDED(hr))
            {
				wchar_t *w;
				psiResult->GetDisplayName(SIGDN_FILESYSPATH, &w);
                psiResult->Release();
				std::wstring ret(w);
				CoTaskMemFree(w);
				return ret;
            }
        } 
        pfd->Release();
    }
    return __T("");
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

void WritePrivateProfileInt(const wchar_t* name, int value){
	std::wstring tmp;
	tmp.resize(16);
	StringCchPrintf((LPWSTR)tmp.c_str(), 16, L"%d", value);
	WritePrivateProfileString(__T("win7shell"), name, tmp.c_str(), W_INI.c_str());
}

static INT_PTR CALLBACK cfgWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	static HWND TabCtrl;
	static HWND Tab1, Tab2, Tab3, Tab4, Tab5, Tab6;
	static sSettings Settings_backup;
	static sSettings_strings Settings_strings_backup;
	static sFontEx Settings_font_backup;
	static bool applypressed = false;
	static int lowframerate_backup;

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

			LastTab = GetPrivateProfileInt(L"win7shell", L"LastTab", 0, W_INI.c_str());
			TabToFront(TabCtrl, LastTab);

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
			DestroyIcon(okbut);
			DestroyIcon(applybut);
			DestroyIcon(cancelbut);

			cfgopen = TRUE;
		}
		break;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
			case IDC_BUTTON1:
				{
					cfgopen = FALSE;
					bool b1 = Settings.Thumbnailbuttons, b2 = false;
					bool Buttons[16]; 
					std::copy(Settings.Buttons, Settings.Buttons+16, Buttons);
					bool recent = Settings.JLrecent;
					bool frequent = Settings.JLfrequent;
					bool tasks = Settings.JLtasks;
					bool bms = Settings.JLbms;
					
					WriteSettings(Tab1);
					WriteSettings(Tab2);
					WriteSettings(Tab3);
					WriteSettings(Tab4);
					WriteSettings(Tab5);
					WriteSettings(Tab6);

					if (recent != Settings.JLrecent ||
						frequent != Settings.JLfrequent ||
						tasks != Settings.JLtasks ||
						bms != Settings.JLbms)
					{
						DoJl();					
					}

					SetWindowAttr();

					if (Settings.VuMeter)
						SetTimer(plugin.hwndParent, 6668, 50, VUMeterProc);
					else
						KillTimer(plugin.hwndParent, 6668);

					if (background)
					{
						delete background;
						background = 0;
					}

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
						pTBL->SetOverlayIcon(plugin.hwndParent, NULL, NULL);

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

					int interval = (S_lowframerate == 1) ? 400 : 100;
					SetTimer(plugin.hwndParent, 6667, interval, TimerProc);		

					WritePrivateProfileStruct(__T("win7shell"), __T("settings"), &Settings, sizeof(Settings), W_INI.c_str());
					WritePrivateProfileStruct(__T("win7shell"), __T("font"), &Settings_font, sizeof(Settings_font), W_INI.c_str());
					WritePrivateProfileString(__T("win7shell"), __T("bgpath"), Settings_strings.BGPath.c_str(), W_INI.c_str());
					WritePrivateProfileString(__T("win7shell"), __T("text"), Settings_strings.Text.c_str(), W_INI.c_str());
					std::wstringstream wss;
					wss << S_lowframerate;
					WritePrivateProfileString(L"win7shell", L"lowframerate", wss.str().c_str(), W_INI.c_str());
					WritePrivateProfileInt(__T("LastTab"), LastTab);
					DestroyWindow(hwnd);
				}
				break;
			case IDC_BUTTON2:
			case IDCANCEL:
				if (applypressed)
				{	
					applypressed = false;

					memcpy(&Settings, &Settings_backup, sizeof(Settings_backup));
					memcpy(&Settings_font, &Settings_font_backup, sizeof(Settings_font_backup));
					Settings_strings.BGPath = Settings_strings_backup.BGPath;
					Settings_strings.Text = Settings_strings_backup.Text;
					S_lowframerate = lowframerate_backup;

					DoJl();					

					SetWindowAttr();

					if (Settings.VuMeter)
						SetTimer(plugin.hwndParent, 6668, 50, VUMeterProc);
					else
						KillTimer(plugin.hwndParent, 6668);

					if (background)
					{
						delete background;
						background = 0;
					}

					if (Settings.Overlay)
						WndProc(plugin.hwndParent, WM_WA_IPC, IPC_CB_MISC_STATUS, IPC_CB_MISC);
					else
						pTBL->SetOverlayIcon(plugin.hwndParent, NULL, NULL);

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

					int interval = (S_lowframerate == 1) ? 400 : 100;
					SetTimer(plugin.hwndParent, 6667, interval, TimerProc);
				}

				cfgopen = FALSE;
				WritePrivateProfileInt(__T("LastTab"), LastTab);
				DestroyWindow(hwnd);
				break;	

			case IDC_BUTTON8:
				{					
					if (!applypressed)
					{					
						memcpy(&Settings_backup, &Settings, sizeof(Settings));
						memcpy(&Settings_font_backup, &Settings_font, sizeof(Settings_font));
						Settings_strings_backup.BGPath = Settings_strings.BGPath;
						Settings_strings_backup.Text = Settings_strings.Text;
						lowframerate_backup = S_lowframerate;
					}					

					applypressed = true;

					bool b1 = Settings.Thumbnailbuttons, b2 = false;
					bool Buttons[16]; 
					std::copy(Settings.Buttons, Settings.Buttons+16, Buttons);
					bool recent = Settings.JLrecent;
					bool frequent = Settings.JLfrequent;
					bool tasks = Settings.JLtasks;
					bool bms = Settings.JLbms;

					WriteSettings(Tab1);
					WriteSettings(Tab2);
					WriteSettings(Tab3);
					WriteSettings(Tab4);
					WriteSettings(Tab5);
					WriteSettings(Tab6);

					if (recent != Settings.JLrecent ||
						frequent != Settings.JLfrequent ||
						tasks != Settings.JLtasks ||
						bms != Settings.JLbms)
					{
						DoJl();					
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

					SetWindowAttr();

					if (Settings.VuMeter)
						SetTimer(plugin.hwndParent, 6668, 50, VUMeterProc);
					else
						KillTimer(plugin.hwndParent, 6668);

					if (background)
					{
						delete background;
						background = 0;
					}

					if (Settings.Overlay)
						WndProc(plugin.hwndParent, WM_WA_IPC, IPC_CB_MISC_STATUS, IPC_CB_MISC);
					else
						pTBL->SetOverlayIcon(plugin.hwndParent, NULL, NULL);

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

					int interval = (S_lowframerate == 1) ? 400 : 100;
					SetTimer(plugin.hwndParent, 6667, interval, TimerProc);
				}
				break;
		}
	break;

	case WM_NOTIFY:
		switch(((NMHDR *)lParam)->code)
        {
        case TCN_SELCHANGE: // Get currently selected tab window to front
				LastTab = SendMessage(TabCtrl, TCM_GETCURSEL, 0, 0);
                TabToFront(TabCtrl, -1);
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
		WritePrivateProfileInt(__T("LastTab"), LastTab);
		break;
	}
		
	return 0;
}

INT_PTR CALLBACK TabHandler(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	switch(Message)
	{
		case WM_INITDIALOG:			
			ReadSettings(hwnd);
			break;

		case WM_COMMAND:
			switch(LOWORD(wParam))
			{
				case IDC_BUTTON3:				
						SetWindowText(GetDlgItem(hwnd, IDC_EDIT2), GetFileName(cfgwindow).c_str());
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
						msgtext += __T("%volume%  -  ");
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
						cf.rgbColors = Settings_font.color;
						cf.lpLogFont = &Settings_font.font;
						cf.Flags = CF_EFFECTS | CF_SCREENFONTS | CF_INITTOLOGFONTSTRUCT;
						
						if (ChooseFont(&cf))
						{
							Settings_font.font = *cf.lpLogFont;
							Settings_font.color = cf.rgbColors;
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
						cc.rgbResult = Settings_font.bgcolor;
						cc.Flags = CC_FULLOPEN | CC_RGBINIT;
						 
						if (ChooseColor(&cc)==TRUE) 
						{
							Settings_font.bgcolor = cc.rgbResult;
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

std::wstring getInstallPath()
{
	std::wstring ret;	
	ret.reserve(255);
	ret.resize(255, '\0');
	HKEY hKey;
	LONG returnStatus;
	DWORD dwType=REG_SZ;
	DWORD dwSize=255;
	returnStatus = RegOpenKeyEx(HKEY_CURRENT_USER, L"Software\\Winamp", 0L,  KEY_READ, &hKey);
	if (returnStatus == ERROR_SUCCESS)
	{
		returnStatus = RegQueryValueEx(hKey, L"", NULL, &dwType,(LPBYTE)&ret[0], &dwSize);
		ret.resize(wcslen(ret.c_str()));
		if (returnStatus == ERROR_SUCCESS)
			return ret;
	}
	
	RegCloseKey(hKey);

	return L"";
}

HWND getWinampWindow()
{
	HWND hwnd = FindWindow(L"Winamp v1.x", NULL);
	if (hwnd != 0)
		if (FindWindowEx(hwnd, NULL, L"WinampVis", NULL) == 0 )
			hwnd = 0;

	if (hwnd == 0)
	{	
		std::wstring ipath = getInstallPath();
		if (!ipath.empty())					
		{
			ipath += L"\\Winamp.exe";
			ShellExecute(NULL, L"open", ipath.c_str(), NULL, NULL, SW_SHOW);
			int wait = 0;
			while (hwnd == 0 && wait < 24)
			{
				Sleep(200);
				wait++;
				hwnd = FindWindow(L"Winamp v1.x", NULL);
			}
		}
	}
	return hwnd;
}

void DoJl()
{
	JumpList jl;
	if (jl.DeleteJumpList())
	{
		std::wstring bms = getBMS();
		static wchar_t tmp1[128], tmp2[128], tmp3[128], tmp4[128], tmp5[128], tmp6[128];
		JumpList jl;
		if (!jl.CreateJumpList(getShortPluginDllPath(),
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

HRESULT __CreateShellLink(PCWSTR filename, PCWSTR pszTitle, IShellLink **ppsl)
{
	IShellLink *psl;
	HRESULT hr = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&psl));
	if (SUCCEEDED(hr))
	{
		// TODO optimise me
		std::wstring fname;
		fname.resize(MAX_PATH);
		GetModuleFileName(0, &fname[0], MAX_PATH);
		fname.resize(wcslen(fname.c_str()));

		std::wstring shortfname;
		shortfname.resize(MAX_PATH);			
		GetShortPathName(fname.c_str(), &shortfname[0], MAX_PATH);
		shortfname.resize(wcslen(shortfname.c_str()));

		fname.clear();
		fname.resize(MAX_PATH);
		if (GetShortPathName(filename, &fname[0], MAX_PATH) == 0)
		{
			fname = filename;
			fname.resize(wcslen(filename));
		}
		psl->SetIconLocation(shortfname.c_str(), 0);

		hr = psl->SetPath(shortfname.c_str());

		if (SUCCEEDED(hr))
		{
			hr = psl->SetArguments(fname.c_str());
			if (SUCCEEDED(hr))
			{
				// The title property is required on Jump List items provided as an IShellLink
				// instance.  This value is used as the display name in the Jump List.
				IPropertyStore *pps;
				hr = psl->QueryInterface(IID_PPV_ARGS(&pps));
				if (SUCCEEDED(hr))
				{
					PROPVARIANT propvar;
					hr = InitPropVariantFromString(pszTitle, &propvar);
					if (SUCCEEDED(hr))
					{
						hr = pps->SetValue(PKEY_Title, propvar);
						if (SUCCEEDED(hr))
						{
							hr = pps->Commit();
							if (SUCCEEDED(hr))
							{
								hr = psl->QueryInterface(IID_PPV_ARGS(ppsl));
							}
						}
						PropVariantClear(&propvar);
					}
					pps->Release();
				}
			}
		}
		else
		{
			hr = HRESULT_FROM_WIN32(GetLastError());
		}
		psl->Release();
	}
	return hr;
}

LRESULT CALLBACK KeyboardEvent (int nCode, WPARAM wParam, LPARAM lParam)
{
	if (!thumbshowing || wParam != WM_MOUSEWHEEL)
		return CallNextHookEx(hMouseHook,nCode,wParam,lParam);

	if ((short)((HIWORD(((MSLLHOOKSTRUCT*)lParam)->mouseData))) > 0)
	{		
		volume += 7;
		if (volume > 255)
			volume = 255;
		SendMessage(plugin.hwndParent,WM_WA_IPC,volume,IPC_SETVOLUME);
	}
	else
	{
		volume -= 7;
		if (volume < 0)
			volume = 0;
		SendMessage(plugin.hwndParent,WM_WA_IPC,volume,IPC_SETVOLUME);
	}

	RECT rect;
	GetWindowRect(plugin.hwndParent, &rect);
	Bitmap bmp(rect.right-rect.left-1, rect.bottom-rect.top-1, PixelFormat32bppPARGB);
	StringFormat sf(StringFormatFlagsNoWrap);
	sf.SetAlignment(StringAlignmentCenter);
	Font font(L"Segoe UI", 18);
	SolidBrush brush(Color::MakeARGB(200, 0, 0, 0));
	std::wstringstream ss;
	ss << L"Volume: " << ((volume * 100) /255) << L"%\n" << metadata.getMetadata(L"title");
	ss << L"\n" << metadata.getMetadata(L"artist");

	Graphics gfx(&bmp);
	gfx.SetTextRenderingHint(TextRenderingHintAntiAlias);

	gfx.DrawString(ss.str().c_str(), -1, &font, RectF(0, 0, (float)(rect.right-rect.left-1), (float)(rect.bottom-rect.top-1)), &sf, &brush);
	HBITMAP hbmp;
	bmp.GetHBITMAP(Color::Black, &hbmp);

	POINT pt;
	pt.x = 0;
	pt.y = 0;

	DwmSetIconicLivePreviewBitmap(plugin.hwndParent, hbmp, &pt, 0);
	DeleteObject(hbmp);

	DwmInvalidateIconicBitmaps(plugin.hwndParent);

	return CallNextHookEx(hMouseHook,nCode,wParam,lParam);
}

std::wstring getShortPluginDllPath()
{
	if(PLUG_DIR.empty())
	{
		// get the shortfilename version to not complicate things
		// when generating a path to access the exported functions
		PLUG_DIR.reserve(MAX_PATH);
		PLUG_DIR.resize(MAX_PATH, '\0');
		GetModuleFileName(plugin.hDllInstance, &PLUG_DIR[0], MAX_PATH);
		GetShortPathName(PLUG_DIR.c_str(), &PLUG_DIR[0], MAX_PATH);
		PLUG_DIR.resize(wcslen(PLUG_DIR.c_str()));
	}
	return PLUG_DIR;
}

std::wstring getWinampINIPath(HWND wnd)
{
	if(INI_DIR.empty())
	{
		if(IsWindow(wnd))
		{
			if(GetWinampVersion(wnd) < 0x5058)
			{
				char *dir=(char*)SendMessage(wnd,WM_WA_IPC,0,IPC_GETINIDIRECTORY);
				INI_DIR.resize(MAX_PATH);
				MultiByteToWideChar(CP_ACP, 0, dir, strlen(dir), &INI_DIR[0], MAX_PATH);
				INI_DIR.resize(wcslen(INI_DIR.c_str()));
			}
			else
			{
				#ifndef IPC_GETINIDIRECTORYW
					#define IPC_GETINIDIRECTORYW 1335
				#endif
				INI_DIR=(wchar_t*)SendMessage(wnd,WM_WA_IPC,0,IPC_GETINIDIRECTORYW);
			}
			return INI_DIR;
		}
	}
	return INI_DIR;
}

std::wstring getBMS()
{
	if(BM_FILE.empty())
	{
		if(GetWinampVersion(plugin.hwndParent) < 0x5058)
		{
			char* bmfile=(char*)SendMessage(plugin.hwndParent,WM_WA_IPC,0,IPC_ADDBOOKMARK);
			BM_FILE.resize(MAX_PATH);
			MultiByteToWideChar(CP_ACP, 0, bmfile, strlen(bmfile), &BM_FILE[0], MAX_PATH);
			BM_FILE.resize(wcslen(BM_FILE.c_str()));
		}
		else
		{
			BM_FILE = (wchar_t*)SendMessage(plugin.hwndParent,WM_WA_IPC,0,IPC_ADDBOOKMARKW);
		}
	}

	std::wifstream is(BM_FILE.c_str());
	if (is.fail())
		return L"";

	std::wstring data;
	std::getline(is, data, L'\0');
	return data;
}

// This is an export function called by winamp which returns this plugin info.
// We wrap the code in 'extern "C"' to ensure the export isn't mangled if used in a CPP file.
#ifdef __cplusplus
extern "C" {
#endif
	__declspec(dllexport) winampGeneralPurposePlugin * winampGetGeneralPurposePlugin() {
		return &plugin;
	}

	__declspec( dllexport ) int winampUninstallPlugin(HINSTANCE hDllInst, HWND hwndDlg, int param) {
		uninstall = true;
		if(MessageBox(hwndDlg,WASABI_API_LNGSTRINGW(IDS_UNINSTALL_PROMPT),
					  BuildPluginNameW(),
					  MB_YESNO)==IDYES){
			DeleteFile(W_INI.c_str());
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
	HWND hwnd = getWinampWindow();
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
	HWND hwnd = getWinampWindow();
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
	HWND hwnd = getWinampWindow();
	if (hwnd == 0)
		MessageBoxEx(0, L"Please start Winamp first", BuildPluginNameW(), MB_ICONWARNING, 0);
	else
	{
		if (SendMessage(hwnd,WM_WA_IPC,0,IPC_ISPLAYING) == 1)
			return 0;
		
		std::wstring path;
		path = getWinampINIPath(hwnd);

		if (path.empty())
			return -1;

		path += L"\\Plugins\\win7shell.ini";

		sResumeSettings sResume;
		ZeroMemory(&sResume, sizeof(sResume));
		if (GetPrivateProfileStruct(__T("win7shell"), __T("resume"), &sResume, sizeof(sResume), path.c_str()))
		{
			SendMessage(hwnd, WM_WA_IPC, sResume.ResumePosition, IPC_SETPLAYLISTPOS);
			SendMessage(hwnd, WM_COMMAND, MAKEWPARAM(40045,0), 0);
			SendMessage(hwnd, WM_WA_IPC, sResume.ResumeTime, IPC_JUMPTOTIME);
		}
	}

	return 0;
}

extern "C" int __declspec(dllexport) __stdcall openfile() 
{
	HWND hwnd = getWinampWindow();
	if (hwnd == 0)
		MessageBoxEx(0, L"Please start Winamp first", BuildPluginNameW(), MB_ICONWARNING, 0);
	else
		SendMessage(hwnd,WM_WA_IPC,(WPARAM)(HWND)0,IPC_OPENFILEBOX);

	return 0;
}