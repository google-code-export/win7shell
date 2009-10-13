#define WIN32_LEAN_AND_MEAN
#define _SECURE_SCL 0
#define _CRT_SECURE_NO_DEPRECATE 1
#define _CRT_NONSTDC_NO_DEPRECATE 1
#define TAGLIB_STATIC

#define ICONSIZEPX 50
#define APPID L"Winamp"

#pragma once

#include <windows.h>
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
#include <fstream>
#include <map>
#include "gen_win7shell.h"
#include "gen.h"
#include "wa_ipc.h"
#include "resource.h"
#include "api.h"
#include "VersionChecker.h"
#include "tabs.h"
#include "metadata.h"
#include "jumplist.h"
#include "albumart.h"

using namespace Gdiplus;

const std::tstring cur_version(__T("1.05"));
UINT s_uTaskbarRestart=0;
WNDPROC lpWndProcOld = 0;
ITaskbarList3* pTBL = 0 ;
char playstate = -1;
int gCurPos = -2, gTotal = -2, volume = 0, gPlayListPos = 0;
std::tstring W_INI;
sSettings Settings;
sSettings_strings Settings_strings;
sFontEx Settings_font;
BOOL cfgopen = false, btaskinited = false, RTL = false, translationfound = false, totheleft = false;
HWND cfgwindow = 0;
Bitmap *background;
MetaData metadata;
HICON iPlay, iPause, iStop;
HIMAGELIST theicons;
ULONG_PTR gdiplusToken;
std::map<std::tstring, std::tstring> STRINGS;
	
// these are callback functions/events which will be called by Winamp
VOID CALLBACK TimerProc(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime);
VOID CALLBACK VUMeterProc(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime);
LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
static INT_PTR CALLBACK cfgWndProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK TabHandler(HWND, UINT, WPARAM, LPARAM);
void CheckVersion( void *dummy );

HIMAGELIST prepareIcons();
void updateToolbar(bool first);
void ReadSettings();
void WriteSettings();
int InitTaskbar();
void SetWindowAttr();
HBITMAP DrawThumbnail(int width, int height, int force);
std::tstring SearchDir(std::tstring path);
std::tstring MetaWord(std::tstring word);
std::tstring GetLine(int linenumber, bool &center, bool &largefont, bool &forceleft);
void LoadStrings(std::tstring Path);
void FillStrings();
void getinistring(std::tstring what, std::tstring Path);
void SetStrings(HWND hwnd);
inline void SetProgressState(TBPFLAG newstate);
std::wstring getToolTip(int button);
std::wstring getString(std::wstring keyword, std::wstring def = L"");
std::wstring getInstallPath();
HWND getWinampWindow();
std::wstring getWinampINIPath();
std::wstring getBMS();
void DoJl();

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


// event functions follow
int init() 
{
	if (SetCurrentProcessExplicitAppUserModelID(APPID) != S_OK)
		MessageBoxEx(plugin.hwndParent, __T("Error setting AppID. This might affect certain features."), 0, MB_ICONWARNING | MB_OK, 0);	

	std::wstring pluginpath;
	pluginpath.reserve(MAX_PATH);
	pluginpath.resize(MAX_PATH, '\0');
	GetModuleFileName(plugin.hDllInstance, &pluginpath[0], MAX_PATH);
	pluginpath.resize(wcslen(pluginpath.c_str()));

	GetShortPathName(pluginpath.c_str(), &pluginpath[0], pluginpath.length());
	pluginpath.resize(wcslen(pluginpath.c_str()));

	W_INI = getWinampINIPath() + L"\\Plugins\\";
	
	FillStrings();

	WIN32_FIND_DATA ffd;
	if (FindFirstFile(std::wstring(W_INI + __T("*win7shell.lng")).c_str(), &ffd) != INVALID_HANDLE_VALUE)
	{
		LoadStrings(W_INI + ffd.cFileName);
		translationfound = true;
	}

	W_INI += __T("win7shell.ini");	

	ZeroMemory(&Settings, sizeof(Settings));

	Settings.VuMeter = false;
	for (int i=0; i<16; i++)
		Settings.Buttons[i] = false;

	if (!GetPrivateProfileStruct(__T("win7shell"), __T("settings"), &Settings, sizeof(Settings), W_INI.c_str()))
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

	if (Settings.JLrecent || Settings.JLfrequent || Settings.JLtasks || Settings.JLbms)
	{
		std::wstring bms;
		bms = getBMS();		

		JumpList jl;
		if (!jl.CreateJumpList(pluginpath, getString(__T("wapref"), __T("Winamp preferences")), 
			getString(__T("fromstart"), __T("Play from beginning")), getString(__T("resumeplay"), __T("Resume playback")), 
			getString(__T("openfile"), __T("Open file")), getString(__T("bookmark"), __T("Bookmarks")),
			Settings.JLrecent, Settings.JLfrequent, Settings.JLtasks, Settings.JLbms, bms) )
			MessageBoxEx(plugin.hwndParent, getString(__T("jumplisterror"), __T("Error creating jump list.")).c_str(), 0, MB_ICONWARNING | MB_OK, 0);
	}

	ZeroMemory(&Settings_font, sizeof(Settings_font));
	if (!GetPrivateProfileStruct(__T("win7shell"), __T("font"), &Settings_font, sizeof(Settings_font), W_INI.c_str()))
	{
		LOGFONT ft;
		wcscpy(ft.lfFaceName, L"Segoe UI");
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

	std::tstring deftext = __T("%c%%title%‡%c%%artist%‡‡Track: %curtime% / %totaltime%‡Volume:  %volume%");
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

	if (IsWindowUnicode(plugin.hwndParent))
		lpWndProcOld = (WNDPROC)SetWindowLongW(plugin.hwndParent,GWL_WNDPROC,(LONG)WndProc);
	else
		lpWndProcOld = (WNDPROC)SetWindowLongA(plugin.hwndParent,GWL_WNDPROC,(LONG)WndProc);

	metadata.setWinampWindow(plugin.hwndParent);

	if (Settings.VuMeter)
		SetTimer(plugin.hwndParent, 6668, 50, VUMeterProc);

	SetTimer(plugin.hwndParent, 6667, 300, TimerProc);		

	volume = IPC_GETVOLUME(plugin.hwndParent);

	return 0;
}

void config()
{
	if ((cfgopen) && (cfgwindow != NULL))
	{
		SetForegroundWindow(cfgwindow);
		return;
	}
	cfgwindow = CreateDialogParam(plugin.hDllInstance,MAKEINTRESOURCE(IID_MAIN),plugin.hwndParent,cfgWndProc,0);
	SetWindowText(cfgwindow, STRINGS.find(__T("config"))->second.c_str());

	RECT rc;
	int screenWidth = GetSystemMetrics(SM_CXSCREEN);
	int screenHeight = GetSystemMetrics(SM_CYSCREEN);
	GetWindowRect(cfgwindow, &rc);
	SetWindowPos(cfgwindow, 0, (screenWidth - rc.right)/2,
		(screenHeight - rc.bottom)/2, 0, 0, SWP_NOZORDER|SWP_NOSIZE);

	ShowWindow(cfgwindow,SW_SHOW);
}
 
void quit() 
{	
	sResumeSettings sResume;
	sResume.ResumePosition = gPlayListPos;
	sResume.ResumeTime = gCurPos;

	DoJl();

	WritePrivateProfileStruct(__T("win7shell"), __T("settings"), &Settings, sizeof(Settings), W_INI.c_str());
	WritePrivateProfileStruct(__T("win7shell"), __T("font"), &Settings_font, sizeof(Settings_font), W_INI.c_str());
	WritePrivateProfileStruct(__T("win7shell"), __T("resume"), &sResume, sizeof(sResume), W_INI.c_str());
	WritePrivateProfileString(__T("win7shell"), __T("bgpath"), Settings_strings.BGPath.c_str(), W_INI.c_str());	
	WritePrivateProfileString(__T("win7shell"), __T("text"), Settings_strings.Text.c_str(), W_INI.c_str());	

	delete background;

	if( pTBL != 0 )
    {
		pTBL->Release();
    }
	
	CoUninitialize();

	if (IsWindowUnicode(plugin.hwndParent))
		SetWindowLongPtrW(plugin.hwndParent, GWLP_WNDPROC, (LONG_PTR)lpWndProcOld);
	else 
		SetWindowLongPtrA(plugin.hwndParent, GWLP_WNDPROC, (LONG_PTR)lpWndProcOld);
	
	GdiplusShutdown(gdiplusToken);
} 

void CheckVersion( void *dummy )
{
	VersionChecker *vc = new VersionChecker();
	std::tstring news = vc->IsNewVersion(cur_version);
	if (news != __T(""))
	{
		news.erase(news.length()-1, 1);
		news = STRINGS.find(__T("newvers"))->second + __T("   ") + news;
		news += __T("\n\n") + STRINGS.find(__T("disableupdate"))->second + __T("\n");
		news += STRINGS.find(__T("opendownload"))->second;

		if (MessageBoxEx(plugin.hwndParent, news.c_str(), __T("Win7 Taskbar Integration Plugin"), 
			MB_ICONINFORMATION | MB_YESNO | MB_SETFOREGROUND, 0) == IDYES)
			ShellExecute(NULL, L"open", L"http://code.google.com/p/win7shell/", NULL, NULL, SW_SHOW);		
	}
	delete vc;
}

std::tstring SecToTime(int sec)
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

std::tstring MetaWord(std::tstring word)
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
			return std::tstring(__T(""));
		std::wstringstream w;
		w << inf;
		return w.str();
	}

	if (word == __T("%khz%"))
	{
		int inf=SendMessage(plugin.hwndParent,WM_WA_IPC,0,IPC_GETINFO);
		if (inf == NULL)
			return std::tstring(__T(""));
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

	if (word == __T("%volume%"))
	{
		std::wstringstream w;
		w << (volume * 100) /255;
		return w.str();
	}

	if (word == __T("%shuffle%"))
	{
		if (SendMessage(plugin.hwndParent,WM_WA_IPC,0,IPC_GET_SHUFFLE))
			return STRINGS.find(__T("on"))->second;
		else return STRINGS.find(__T("off"))->second;
	}

	if (word == __T("%repeat%"))
	{
		if (SendMessage(plugin.hwndParent,WM_WA_IPC,0,IPC_GET_REPEAT))
			return STRINGS.find(__T("on"))->second;
		else return STRINGS.find(__T("off"))->second;
	}

	if (word == __T("%curpl%"))
	{
		std::wstringstream w;
		w << gPlayListPos+1;
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
		int cur_pos=SendMessage(plugin.hwndParent,WM_WA_IPC,0,IPC_GETLISTPOS)+1;
		std::wstringstream w;
		int x = SendMessage(plugin.hwndParent,WM_WA_IPC,0,IPC_GETRATING);
		if (word == __T("%rating1%"))
			w << x;
		else
			for (int i=0; i < x; i++)
				w << __T("\u2605");
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
		
	for (int i = 0; i < 10; ++i)
	{
		hicon = LoadIcon(plugin.hDllInstance, MAKEINTRESOURCE(IDI_ICON1+i)); 

		if (hicon == NULL)
			return NULL;		
		else
			ImageList_AddIcon(himlIcons, hicon);
		DestroyIcon(hicon);
	}

	return himlIcons;	
}

std::wstring getToolTip(int button)
{
	switch (button)
	{
	case 0:
		return getString(__T("play"));
		break;
	case 1:
		return getString(__T("previous"));
		break;
	case 2:
		return getString(__T("pause"));
		break;
	case 3:
		return getString(__T("stop"));
		break;
	case 4:
		return getString(__T("next"));
		break;
	case 5:
		return getString(__T("favorite"));
		break;
	case 6:
		return getString(__T("voldown"), __T("Volume -10%"));
		break;
	case 7:
		return getString(__T("volup"), __T("Volume +10%"));
		break;
	case 8:
		return getString(__T("openfile"), __T("Open File"));
		break;
	case 9:
		return getString(__T("mute"), __T("Mute"));
	default:
		return L"";
	}
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
			
			if (i == 1)
			{				
				if (playstate == 1)
				{
					button.iBitmap = 2;
					wcscpy(button.szTip, getToolTip(2).c_str());
				}
				else
				{
					button.iBitmap = 0;
					wcscpy(button.szTip, getToolTip(0).c_str());
				}
			}
			else
			{
				button.iBitmap = i+1;
				wcscpy(button.szTip, getToolTip(i+1).c_str());
			}
			
			thbButtons.push_back(button);
		}	

	if (first)
		pTBL->ThumbBarAddButtons(plugin.hwndParent, thbButtons.size(), &thbButtons[0]);
	else
		pTBL->ThumbBarUpdateButtons(plugin.hwndParent, thbButtons.size(), &thbButtons[0]);
}

std::tstring GetLine(int linenumber, bool &center, bool &largefont, bool &forceleft)
{
	std::tstring::size_type pos = 0, open = std::tstring::npos;
	std::tstring text, metaword;
	center = 0;
	int count = 0;
	if (linenumber == 0)
		text = Settings_strings.Text.substr(pos, Settings_strings.Text.find_first_of(NEWLINE, pos));
	else
	{
		do 
		{
			pos = Settings_strings.Text.find_first_of(NEWLINE, pos+1);
			if (pos != std::tstring::npos)
				count++;
		}
		while ( (count < linenumber) && (pos != std::tstring::npos) );

		text = Settings_strings.Text.substr(pos+1, Settings_strings.Text.find_first_of(NEWLINE, pos+1)-pos-1);
	}

	pos = 0;
	do
	{
		pos = text.find_first_of('%', pos);		
		if (pos != std::tstring::npos)
		{
			if (open != std::tstring::npos)
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

				text.replace(open, pos-open+1, metaword);
				open = std::tstring::npos;
				pos = std::tstring::npos;
			}
			else
				open = pos;
			pos++;
		}
	}
	while (pos!=std::tstring::npos);

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
			background = new Bitmap(width, height, PixelFormat32bppARGB);
			totheleft = true;
			break;

		case 1: //album art
			{
				AlbumArt AA;
				Bitmap tmpbmp(height, height, PixelFormat32bppARGB);
				if (AA.getAA(metadata.getFileName(), metadata.getMetadata(L"album"), tmpbmp, height))
				{					
					background = new Bitmap(width, height, PixelFormat32bppARGB);
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
				{
					if (force != -1)
					{
						background = new Bitmap(width, height, PixelFormat32bppARGB);					
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
				background = new Bitmap(width, height, PixelFormat32bppARGB);
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
					background = new Bitmap(width, height, PixelFormat32bppARGB);
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
						background = new Bitmap(width, height, PixelFormat32bppARGB);	
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
					background = new Bitmap(width, height, PixelFormat32bppARGB);					
				}
				else
					DrawThumbnail(width, height, Settings.Revertto);
				Settings.Thumbnailbackground = 0;
			}
			break;

		default: 
			totheleft = true;
			background = new Bitmap(width, height, PixelFormat32bppARGB);
	}

	if (force > 0)
		return NULL;

	//Draw text
	HBITMAP retbmp;
	int lines = 1;

	int textheight=0;
	Font font(GetDC(0), const_cast<LOGFONT*> (&Settings_font.font));
	LONG oldsize = Settings_font.font.lfHeight;
	int x = -((Settings_font.font.lfHeight * 72) / GetDeviceCaps(GetDC(0), LOGPIXELSY));
	x += 4;
	Settings_font.font.lfHeight = -MulDiv(x, GetDeviceCaps(GetDC(0), LOGPIXELSY), 72);
	Font largefont(GetDC(0), const_cast<LOGFONT*> (&Settings_font.font));
	Settings_font.font.lfHeight = oldsize;
	StringFormat sf(StringFormatFlagsNoWrap);

	Graphics tmpgfx(background);		

	Pen p(Color::MakeARGB(1, 255, 255, 255), 1);
	tmpgfx.DrawLine(&p, 0, 0, 1, 1);

	struct texts {
		std::wstring text;
		int height;
		bool center;
		bool largefont;
		bool forceleft;
	};

	for (std::tstring::size_type i = 0; i < Settings_strings.Text.length(); i++)
		if (Settings_strings.Text[i] == NEWLINE)
			lines++;

	std::vector<texts> TX;

	for (int i=0; i<lines; i++)
	{
		texts tx;
		ZeroMemory(&tx, sizeof(tx));
		tx.text = GetLine(i, tx.center, tx.largefont, tx.forceleft);
		RectF rect;
		if (tx.largefont)
			tmpgfx.MeasureString(tx.text.c_str(), -1, &largefont, RectF(0, 0, static_cast<REAL>(width), static_cast<REAL>(height)), &sf, &rect);
		else
			tmpgfx.MeasureString(tx.text.c_str(), -1, &font, RectF(0, 0, static_cast<REAL>(width), static_cast<REAL>(height)), &sf, &rect);

		if (rect.GetBottom() == 0)
			tmpgfx.MeasureString(L"QWEXCyjM", -1, &font, RectF(0, 0, static_cast<REAL>(width), static_cast<REAL>(height)), &sf, &rect);			
		
		tx.height = static_cast<int>(rect.GetBottom());
		textheight += tx.height;
		TX.push_back(tx);
	}	

	if (!Settings.Shrinkframe)	
		textheight = height-2;
		
	if (Settings.Thumbnailpb)
		textheight += 25;

	if (textheight > height-2)
		textheight = height-2;

	Bitmap bmp(background->GetWidth(), textheight+2, PixelFormat32bppARGB);
	Graphics gfx(&bmp);
	gfx.SetInterpolationMode(InterpolationModeHighQualityBilinear);
	if (Settings.AsIcon)
		gfx.DrawImage(background, RectF(0, 0, static_cast<REAL>(background->GetWidth()), static_cast<REAL>(background->GetHeight())));
	else
		gfx.DrawImage(background, RectF(0, 0, static_cast<REAL>(background->GetWidth()), static_cast<REAL>(textheight+2)));

	gfx.SetSmoothingMode(SmoothingModeNone);		
			
	if ( (Settings.Thumbnailbackground == 0) || (Settings.Thumbnailbackground == 1) )
		(Settings.Antialias) ? gfx.SetTextRenderingHint(TextRenderingHintAntiAlias) : gfx.SetTextRenderingHint(TextRenderingHintAntiAliasGridFit);
	else	
		(Settings.Antialias) ? gfx.SetTextRenderingHint(TextRenderingHintAntiAlias) : gfx.SetTextRenderingHint(TextRenderingHintClearTypeGridFit);		

	bool drawshadow = true;
	SolidBrush bgcolor(Color(GetRValue(Settings_font.bgcolor), GetGValue(Settings_font.bgcolor), GetBValue(Settings_font.bgcolor)));
	SolidBrush fgcolor(Color(GetRValue(Settings_font.color), GetGValue(Settings_font.color), GetBValue(Settings_font.color)));
	if (GetRValue(Settings_font.bgcolor) == 255 &&
		GetGValue(Settings_font.bgcolor) == 255 &&
		GetBValue(Settings_font.bgcolor) == 255)
		drawshadow = false;

	int heightsofar=0;
	for (int i=0; i < lines; i++)
	{
		sf.SetAlignment(static_cast<StringAlignment>(TX[i].center));

		int left = 3;
		if (Settings.AsIcon && !totheleft && !TX[i].forceleft)
			left = ICONSIZEPX + 4;

		if (i)
			heightsofar += TX[i-1].height;		

		if (drawshadow)
		{		
			RectF rect(static_cast<REAL>(left+1), static_cast<REAL>(heightsofar), 197, static_cast<REAL>(heightsofar+TX[i].height));		
			if (TX[i].largefont)
				gfx.DrawString(TX[i].text.c_str(), -1, &largefont, rect, &sf, &bgcolor);					
			else
				gfx.DrawString(TX[i].text.c_str(), -1, &font, rect, &sf, &bgcolor);					
		}

		RectF rect(static_cast<REAL>(left), static_cast<REAL>(heightsofar-1), 196, static_cast<REAL>(heightsofar+TX[i].height-2));				
		if (TX[i].largefont)
			gfx.DrawString(TX[i].text.c_str(), -1, &largefont, rect, &sf, &fgcolor);
		else
			gfx.DrawString(TX[i].text.c_str(), -1, &font, rect, &sf, &fgcolor);
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

	return retbmp;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_DWMSENDICONICTHUMBNAIL:
		{
			HBITMAP thumbnail = DrawThumbnail(HIWORD(lParam), LOWORD(lParam), -1);			
			Sleep(50);

			HRESULT hr = DwmSetIconicThumbnail(plugin.hwndParent, thumbnail, 0);
			if (FAILED(hr))
			{
				MessageBoxEx(plugin.hwndParent, STRINGS.find(__T("errorthumb"))->second.c_str(), NULL, MB_ICONERROR, 0);
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
				SendMessage(plugin.hwndParent, WM_COMMAND, 40044, 0);
				if (background)
				{
					delete background;
					background = 0;
				}
				if (Settings.RemoveTitle)
					SetWindowText(plugin.hwndParent, __T(""));
				break;
			case 1:
				{
					int res = SendMessage(plugin.hwndParent,WM_WA_IPC,0,IPC_ISPLAYING);
					if (res == 1)
						SendMessage(plugin.hwndParent, WM_COMMAND, 40046, 0);									
					else					
						SendMessage(plugin.hwndParent, WM_COMMAND, 40045, 0);					
					playstate = res;
				}
				break;
			case 2:
				SendMessage(plugin.hwndParent, WM_COMMAND, 40047, 0);
				playstate = 3; //stopped
				break;
			case 3:
				SendMessage(plugin.hwndParent, WM_COMMAND, 40048, 0);
				if (background)
				{
					delete background;
					background = 0;
				}
				if (Settings.RemoveTitle)
					SetWindowText(plugin.hwndParent, __T(""));
				break;
			case 4:
				SendMessage(plugin.hwndParent,WM_WA_IPC,5,IPC_SETRATING);
				DwmInvalidateIconicBitmaps(plugin.hwndParent);
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

			}
		break;

		case WM_WA_IPC:
			switch (lParam)
			{
			case IPC_PLAYING_FILEW:
				{
					std::wstring filename = (wchar_t*)wParam;
					if (filename.empty())
					{
						int index = SendMessage(plugin.hwndParent,WM_WA_IPC,0,IPC_GETLISTPOS);
						filename = (wchar_t*)SendMessage(plugin.hwndParent,WM_WA_IPC,index,IPC_GETPLAYLISTFILEW); 
					}
					
					metadata.reset(filename.c_str(), false);

					gPlayListPos = SendMessage(plugin.hwndParent,WM_WA_IPC,0,IPC_GETLISTPOS);

					gTotal = SendMessage(plugin.hwndParent,WM_WA_IPC,2,IPC_GETOUTPUTTIME);
					gCurPos = 0;

					DwmInvalidateIconicBitmaps(plugin.hwndParent);
					
					playstate = 1;

					if ( (Settings.JLrecent || Settings.JLfrequent) && metadata.isFile )
					{
						
						SHAddToRecentDocs(SHARD_PATHW, filename.c_str());
					}

					if (Settings.Thumbnailbackground == 1)
						if (background)
						{
							delete background;
							background = 0;
						}
				}
				break;				

			case IPC_CB_MISC:
				switch (wParam)
				{
					case IPC_CB_MISC_STATUS:
						playstate = SendMessage(plugin.hwndParent,WM_WA_IPC,0,IPC_ISPLAYING);

						updateToolbar(false);

						if (Settings.Overlay)
							switch (playstate)
							{
							case 1:
								pTBL->SetOverlayIcon(plugin.hwndParent, ImageList_GetIcon(theicons, 0, 0), 
									STRINGS.find(__T("playing"))->second.c_str());
								break;
							case 3:
								pTBL->SetOverlayIcon(plugin.hwndParent, ImageList_GetIcon(theicons, 2, 0), 
									STRINGS.find(__T("paused"))->second.c_str());
								break;
							default:
								pTBL->SetOverlayIcon(plugin.hwndParent, ImageList_GetIcon(theicons, 3, 0), 
									STRINGS.find(__T("stopped"))->second.c_str());
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
				SendMessage(plugin.hwndParent, WM_COMMAND, 40001, 0);
			break;

		default:
			return CallWindowProc(lpWndProcOld,hwnd,message,wParam,lParam);	 
	}

	return CallWindowProc(lpWndProcOld,hwnd,message,wParam,lParam);	 
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
	if (playstate == -1)
	{
 		WndProc(plugin.hwndParent, WM_WA_IPC, (WPARAM)L"", IPC_PLAYING_FILEW);
		WndProc(plugin.hwndParent, WM_WA_IPC, IPC_CB_MISC_STATUS, IPC_CB_MISC);
	}

	if (Settings.RemoveTitle)					
		if (GetWindowTextLength(plugin.hwndParent) != 0)
			SetWindowText(plugin.hwndParent, __T(""));

	if (!(Settings.Progressbar || Settings.VuMeter))
		SetProgressState(TBPF_NOPROGRESS);	

	int cp = SendMessage(plugin.hwndParent,WM_WA_IPC,0,IPC_GETOUTPUTTIME);
	if (gCurPos == cp)
	{
		return;
	}
	gCurPos = cp;

	if (playstate == 1 || Settings.Thumbnailpb)
		if (gTotal <= 0)
			gTotal = SendMessage(plugin.hwndParent,WM_WA_IPC,2,IPC_GETOUTPUTTIME);

	static unsigned char count1=0;
	if (count1 == 2)
	{
		DwmInvalidateIconicBitmaps(plugin.hwndParent);
		count1 = 0;
	}
	else
		count1++;

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
			MessageBoxEx(plugin.hwndParent, STRINGS.find(__T("taskinitfail"))->second.c_str(), NULL, MB_ICONSTOP, 0);
			return -1;
		}

		hr = pTBL->HrInit();
		if (!SUCCEEDED(hr))
		{
			MessageBoxEx(plugin.hwndParent, STRINGS.find(__T("taskinitfail"))->second.c_str(), NULL, MB_ICONSTOP, 0);
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

	SendMessage(GetDlgItem(hwnd, IDC_COMBO1), CB_ADDSTRING, 0, (LPARAM)getString(L"transparent", L"Transparent").c_str());
	SendMessage(GetDlgItem(hwnd, IDC_COMBO1), CB_ADDSTRING, 0, (LPARAM)getString(L"albumart", L"Album art").c_str());
	SendMessage(GetDlgItem(hwnd, IDC_COMBO1), CB_ADDSTRING, 0, (LPARAM)getString(L"defbg", L"Default").c_str());
	SendMessage(GetDlgItem(hwnd, IDC_COMBO1), CB_ADDSTRING, 0, (LPARAM)getString(L"custombg", L"Custom").c_str());

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

	SetWindowText(GetDlgItem(hwnd, IDC_EDIT2), Settings_strings.BGPath.c_str());

	std::tstring tmpbuf = Settings_strings.Text;
	std::tstring::size_type pos = std::tstring::npos;
	do 
	{
		pos = tmpbuf.find(__T("‡"));
		if (pos != std::tstring::npos)
			tmpbuf.replace(pos, 1, __T("\xD\xA"));
	}
	while (pos != std::tstring::npos);
	
	SetWindowText(GetDlgItem(hwnd, IDC_EDIT3), tmpbuf.c_str());
	
	EnableWindow(GetDlgItem(hwnd, IDC_CHECK4), Settings.Progressbar);
	EnableWindow(GetDlgItem(hwnd, IDC_CHECK5), Settings.Progressbar);
	EnableWindow(GetDlgItem(hwnd, IDC_CHECK27), Settings.Thumbnailbuttons);

	for (int i = IDC_PCB1; i <= IDC_PCB9; i++)
		SendMessage(GetDlgItem(hwnd, i), (UINT) BM_SETCHECK, Settings.Buttons[i-IDC_PCB1], 0);		
}

void WriteSettings(HWND hwnd)
{
	//Checkboxes
	if (GetDlgItem(hwnd, IDC_CHECK3) != NULL)
		Settings.Overlay = SendMessage(GetDlgItem(hwnd, IDC_CHECK3), (UINT) BM_GETCHECK, 0, 0);
	if (GetDlgItem(hwnd, IDC_CHECK2) != NULL)
		Settings.Progressbar = SendMessage(GetDlgItem(hwnd, IDC_CHECK2), (UINT) BM_GETCHECK, 0, 0);
	if (GetDlgItem(hwnd, IDC_CHECK4) != NULL)
		Settings.Streamstatus = SendMessage(GetDlgItem(hwnd, IDC_CHECK4), (UINT) BM_GETCHECK, 0, 0);
	if (GetDlgItem(hwnd, IDC_CHECK5) != NULL)
		Settings.Stoppedstatus = SendMessage(GetDlgItem(hwnd, IDC_CHECK5), (UINT) BM_GETCHECK, 0, 0);
	if (GetDlgItem(hwnd, IDC_CHECK6) != NULL)
		Settings.Thumbnailbuttons = SendMessage(GetDlgItem(hwnd, IDC_CHECK6), (UINT) BM_GETCHECK, 0, 0);
	if (GetDlgItem(hwnd, IDC_CHECK7) != NULL)
		Settings.Thumbnailenabled = SendMessage(GetDlgItem(hwnd, IDC_CHECK7), (UINT) BM_GETCHECK, 0, 0);	
	if (GetDlgItem(hwnd, IDC_CHECK8) != NULL)
		Settings.Antialias = SendMessage(GetDlgItem(hwnd, IDC_CHECK8), (UINT) BM_GETCHECK, 0, 0);
	if (GetDlgItem(hwnd, IDC_CHECK1) != NULL)
		Settings.Shrinkframe = SendMessage(GetDlgItem(hwnd, IDC_CHECK1), (UINT) BM_GETCHECK, 0, 0);
	if (GetDlgItem(hwnd, IDC_CHECK9) != NULL)
		Settings.DisableUpdates = SendMessage(GetDlgItem(hwnd, IDC_CHECK9), (UINT) BM_GETCHECK, 0, 0);
	if (GetDlgItem(hwnd, IDC_CHECK10) != NULL)
		Settings.RemoveTitle = SendMessage(GetDlgItem(hwnd, IDC_CHECK10), (UINT) BM_GETCHECK, 0, 0);
	if (GetDlgItem(hwnd, IDC_CHECK25) != NULL)
		Settings.AsIcon = SendMessage(GetDlgItem(hwnd, IDC_CHECK25), (UINT) BM_GETCHECK, 0, 0);
	if (GetDlgItem(hwnd, IDC_CHECK26) != NULL)
		Settings.VuMeter = SendMessage(GetDlgItem(hwnd, IDC_CHECK26), (UINT) BM_GETCHECK, 0, 0);
	if (GetDlgItem(hwnd, IDC_CHECK29) != NULL)
		Settings.Thumbnailpb = SendMessage(GetDlgItem(hwnd, IDC_CHECK29), (UINT) BM_GETCHECK, 0, 0);
	if (GetDlgItem(hwnd, IDC_CHECK30) != NULL)
		Settings.JLrecent = SendMessage(GetDlgItem(hwnd, IDC_CHECK30), (UINT) BM_GETCHECK, 0, 0);
	if (GetDlgItem(hwnd, IDC_CHECK31) != NULL)
		Settings.JLfrequent = SendMessage(GetDlgItem(hwnd, IDC_CHECK31), (UINT) BM_GETCHECK, 0, 0);
	if (GetDlgItem(hwnd, IDC_CHECK32) != NULL)
		Settings.JLtasks = SendMessage(GetDlgItem(hwnd, IDC_CHECK32), (UINT) BM_GETCHECK, 0, 0);
	if (GetDlgItem(hwnd, IDC_CHECK33) != NULL)
		Settings.JLbms = SendMessage(GetDlgItem(hwnd, IDC_CHECK33), (UINT) BM_GETCHECK, 0, 0);

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
		Settings.Revertto = SendMessage(GetDlgItem(hwnd, IDC_COMBO1), CB_GETCURSEL, 0, 0);

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

		std::tstring::size_type pos = std::tstring::npos;
		do 
		{
			pos = Settings_strings.Text.find(__T("\xD\xA"));
			if (pos != std::tstring::npos)
				Settings_strings.Text.replace(pos, 2, __T("‡"));
		}
		while (pos != std::tstring::npos);
	}

	if (GetDlgItem(hwnd, IDC_PCB1) != NULL)
	{
		for (int i = IDC_PCB1; i <= IDC_PCB9; i++)
			Settings.Buttons[i-IDC_PCB1] = SendMessage(GetDlgItem(hwnd, i), (UINT) BM_GETCHECK, 0 , 0);
	}
}

std::tstring GetFileName(HWND hwnd)
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
		COMDLG_FILTERSPEC rgSpec[] =
		{ 			
			{ STRINGS.find(__T("images"))->second.c_str(), __T("*.bmp;*.dib;*.jpg;*.jpeg;*.gif;*.png;*.ico;*.tiff;*.tif;*.wmf;*.emf;*.exif")} 
		};

		pfd->SetFileTypes(1, rgSpec);
		pfd->SetOkButtonLabel(STRINGS.find(__T("usethumbbg"))->second.c_str());
		pfd->SetTitle(STRINGS.find(__T("selectimage"))->second.c_str());
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
				std::tstring ret(w);
				CoTaskMemFree(w);
				return ret;
            }
        } 
        pfd->Release();
    }
    return __T("");
}

static INT_PTR CALLBACK cfgWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	static HWND TabCtrl;
	static HWND Tab1, Tab2, Tab3, Tab4, Tab5, Tab6;

	switch(msg)
	{
	case WM_INITDIALOG:
		{			
			Tab1 = CreateDialog(plugin.hDllInstance, MAKEINTRESOURCE(IID_TAB1), hwnd, &TabHandler);
			Tab2 = CreateDialog(plugin.hDllInstance, MAKEINTRESOURCE(IID_TAB2), hwnd, &TabHandler);
			Tab3 = CreateDialog(plugin.hDllInstance, MAKEINTRESOURCE(IID_TAB3), hwnd, &TabHandler);
			Tab4 = CreateDialog(plugin.hDllInstance, MAKEINTRESOURCE(IID_TAB4), hwnd, &TabHandler);
			Tab5 = CreateDialog(plugin.hDllInstance, MAKEINTRESOURCE(IID_TAB5), hwnd, &TabHandler);
			Tab6 = CreateDialog(plugin.hDllInstance, MAKEINTRESOURCE(IID_TAB6), hwnd, &TabHandler);

			TabCtrl = GetDlgItem(hwnd, IDC_TAB);

			if (translationfound)
			{
				SetWindowText(GetDlgItem(hwnd, IDC_BUTTON1), getString(__T("ok")).c_str());
				SetWindowText(GetDlgItem(hwnd, IDC_BUTTON2), getString(__T("cancel")).c_str());
			}			

			int Index = 0;
			Index = AddTab(TabCtrl, Tab1, const_cast<wchar_t*> (getString(__T("thumbtext"), __T("Text")).c_str()), -1);

			AddTab(TabCtrl, Tab2, const_cast<wchar_t*> (getString(__T("background"), __T("Background")).c_str()), -1);
			AddTab(TabCtrl, Tab3, const_cast<wchar_t*> (getString(__T("thumboptions"), __T("Options")).c_str()), -1);
			AddTab(TabCtrl, Tab4, const_cast<wchar_t*> (getString(__T("taskicon"), __T("Taskbar Icon")).c_str()), -1);
			AddTab(TabCtrl, Tab5, const_cast<wchar_t*> (getString(__T("jumplist"), __T("Jump List")).c_str()), -1);

			std::tstring cv;
			cv = getString(__T("plugdev"), __T("Development")) + __T(" v") + cur_version;

			AddTab(TabCtrl, Tab6, const_cast<wchar_t*> (cv.c_str()), -1);

			TabToFront(TabCtrl, Index);

			for (int i=0; i<9; i++)
			{
				SendMessage(GetDlgItem(Tab3, IDC_PCB1+i), BM_SETIMAGE, IMAGE_ICON, (LPARAM)ImageList_GetIcon(theicons, i+1, 0));
			}

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
						MessageBoxEx(cfgwindow, STRINGS.find(__T("afterrestart"))->second.c_str(),
							STRINGS.find(__T("applysettings"))->second.c_str(), MB_ICONWARNING, 0);					
					}

					if (Settings.Overlay)
						WndProc(plugin.hwndParent, WM_WA_IPC, IPC_CB_MISC_STATUS, IPC_CB_MISC);
					else
						pTBL->SetOverlayIcon(plugin.hwndParent, NULL, NULL);

					DestroyWindow(hwnd);
				}
				break;
			case IDC_BUTTON2:
				cfgopen = FALSE;
				DestroyWindow(hwnd);
				break;	
		}
	break;

	case WM_NOTIFY:
		switch(((NMHDR *)lParam)->code)
        {
        case TCN_SELCHANGE: // Get currently selected tab window to front
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
		break;
	}
		
	return 0;
}

INT_PTR CALLBACK TabHandler(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	switch(Message)
	{
		case WM_INITDIALOG:			
			SetStrings(hwnd);
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
						std::tstring msgtext = STRINGS.find(__T("help1"))->second + __T("\n\n");
						msgtext += STRINGS.find(__T("help2"))->second + __T("\n\n");
						msgtext += __T("%c%  -  ") + STRINGS.find(__T("help3"))->second + __T("\n");
						msgtext += __T("%l%  -  ") + STRINGS.find(__T("help22"))->second + __T("\n");
						msgtext += __T("%f%  -  ") + STRINGS.find(__T("help23"))->second + __T("\n");
						msgtext += __T("%title%  -  ") + STRINGS.find(__T("help5"))->second + __T("\n");
						msgtext += __T("%artist%  -  ") + STRINGS.find(__T("help6"))->second + __T("\n");
						msgtext += __T("%album%  -    ") + STRINGS.find(__T("help7"))->second + __T("\n");
						msgtext += __T("%year%  -  ") + STRINGS.find(__T("help8"))->second + __T("\n");
						msgtext += __T("%track%  -  ") + STRINGS.find(__T("help9"))->second + __T("\n");
						msgtext += __T("%rating1%  -  ") + STRINGS.find(__T("help10"))->second + __T("\n");
						msgtext += __T("%rating2%  -  ") + STRINGS.find(__T("help11"))->second + __T("\n");
						msgtext += __T("%curtime%  -  ") + STRINGS.find(__T("help12"))->second + __T("\n");
						msgtext += __T("%timeleft%  -  ") + STRINGS.find(__T("help13"))->second + __T("\n");
						msgtext += __T("%totaltime%  -  ") + STRINGS.find(__T("help14"))->second + __T("\n");
						msgtext += __T("%curpl%  -  ") + STRINGS.find(__T("help15"))->second + __T("\n");
						msgtext += __T("%totalpl%  -  ") + STRINGS.find(__T("help16"))->second + __T("\n");
						msgtext += __T("%kbps%  -  ") + STRINGS.find(__T("help17"))->second + __T("\n");
						msgtext += __T("%khz%  -  ") + STRINGS.find(__T("help18"))->second + __T("\n");
						msgtext += __T("%volume%  -  ") + STRINGS.find(__T("help19"))->second + __T("\n");
						msgtext += __T("%shuffle%  -  ") + STRINGS.find(__T("help20"))->second + __T("\n");
						msgtext += __T("%repeat%  -  ") + STRINGS.find(__T("help21"))->second + __T("\n");
							
						MessageBoxEx(cfgwindow, msgtext.c_str(), 
							STRINGS.find(__T("info"))->second.c_str(), MB_ICONINFORMATION, 0);

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
						bool checked = SendMessage(GetDlgItem(hwnd, IDC_CHECK2), (UINT) BM_GETCHECK, 0, 0);
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

				}
				break;

		case WM_MOUSEMOVE:
			SetWindowText(GetDlgItem(hwnd, IDC_STATIC29), __T(""));
			break;

		default:
			return false;
	}

	return true;
}

void FillStrings()
{
	STRINGS[__T("newvers")] = __T("New version available");
	STRINGS[__T("disableupdate")] = __T("You can disable checking for updates in the plugin configuration");
	STRINGS[__T("opendownload")] = __T("Do you want to open the download page?");
	STRINGS[__T("config")] = __T("Configure Plugin");
	STRINGS[__T("info")] = __T("Information");
	STRINGS[__T("on")] = __T("ON");
	STRINGS[__T("off")] = __T("OFF");
	STRINGS[__T("previous")] = __T("Previous");
	STRINGS[__T("pause")] = __T("Pause");
	STRINGS[__T("play")] = __T("Play");
	STRINGS[__T("stop")] = __T("Stop");
	STRINGS[__T("next")] = __T("Next");
	STRINGS[__T("errorthumb")] = __T("Error setting thumbnail, switching feature off");
	STRINGS[__T("playing")] = __T("Playing");
	STRINGS[__T("paused")] = __T("Paused");
	STRINGS[__T("stopped")] = __T("Stopped");
	STRINGS[__T("taskinitfail")] = __T("Unable to initialize Windows 7 taskbar interface");
	STRINGS[__T("plugindev")] = __T("Plug-in Development  -  Current Version: ");
	STRINGS[__T("images")] = __T("All supported image formats");
	STRINGS[__T("usethumbbg")] = __T("Use as thumbnail background");
	STRINGS[__T("selectimage")] = __T("Select an image file");
	STRINGS[__T("afterrestart")] = __T("Modifying player control buttons will only take effect after restarting Winamp.");
	STRINGS[__T("applysettings")] = __T("Applying Settings");
	STRINGS[__T("help1")] = __T("The text you enter in this editbox will be drawn on the thumbnail. ")
		__T("You can write as many lines as you want. If the 'shrink thumbnail' option is enabled, ")
		__T("the frame will be resized depending on the number of lines in this editbox.");
	STRINGS[__T("help2")] = __T("In order to create dynamic text, you can use a set of ");
		__T("metawords, which if you include in the text, they will be replaced by the actual data taken from Winamp. Here are these words: ");
	STRINGS[__T("help3")] = __T("The line that contains this is drawn centered");
	STRINGS[__T("help5")] = __T("Title of currently playing media");
	STRINGS[__T("help6")] = __T("Artist of currently playing media");
	STRINGS[__T("help7")] = __T("Name of album, empty if not found");
	STRINGS[__T("help8")] = __T("Release year of the album, empty if not found");
	STRINGS[__T("help9")] = __T("Track number");
	STRINGS[__T("help10")] = __T("Current song rating in digits (e.g. 4)");
	STRINGS[__T("help11")] = __T("Current song rating in stars (e.g. \u2605\u2605\u2605\u2605 )");
	STRINGS[__T("help12")] = __T("Current playback time");
	STRINGS[__T("help13")] = __T("Time left from current song");
	STRINGS[__T("help14")] = __T("Total playback time");
	STRINGS[__T("help15")] = __T("Current playlist position");
	STRINGS[__T("help16")] = __T("Total number of tracks in current playlist");
	STRINGS[__T("help17")] = __T("Bitrate in kb/s");
	STRINGS[__T("help18")] = __T("Samplerate in kilohertz");
	STRINGS[__T("help19")] = __T("Current volume in percentage ('%' sign not included)");
	STRINGS[__T("help20")] = __T("Shuffle state (ON / OFF)");
	STRINGS[__T("help21")] = __T("Repeat state (ON / OFF)");
	STRINGS[__T("help22")] = __T("The line that contains this will be drawn with larger font");
	STRINGS[__T("help23")] = __T("The line that contains this will be drawn to the left of the thumbnail");
	STRINGS[__T("favorite")] = __T("Favorite");
}

void getinistring(std::tstring what, std::tstring Path)
{
	std::vector<TCHAR> buffer(1000);
	int sz = GetPrivateProfileString(__T("win7shell"), what.c_str(), __T(""), &buffer[0], 1000, Path.c_str());
	if (sz != 0)
		STRINGS[what] = std::tstring(buffer.begin(), buffer.begin()+sz);	
}

void LoadStrings(std::tstring Path)
{
	RTL = GetPrivateProfileInt(__T("win7shell"), __T("RTL"), 0, Path.c_str());

	std::wstring values;
	values.reserve(10000);
	values.resize(10000);
	int sz = GetPrivateProfileSection(L"win7shell", &values[0], 10000, Path.c_str());
	values.resize(sz);
	
	std::wstring::size_type pos = 0;
	while (pos != std::wstring::npos)
	{
		pos = values.find_first_of(L'\0');
		if (pos != std::wstring::npos)
		{
			std::wstring kvpair = values.substr(0, pos);
			values.erase(0, pos+1);

			std::wstring::size_type separator = kvpair.find_first_of('=');
			if (separator != std::wstring::npos)
			{
				STRINGS[kvpair.substr(0, separator)] = kvpair.substr(separator+1);
			}
		}
	}
}

BOOL CALLBACK EnumChildProc(HWND hwndChild, LPARAM lParam) 
{ 
	DWORD dwStyle;
	dwStyle = GetWindowLong(hwndChild, GWL_EXSTYLE);

	SetWindowLong(hwndChild, GWL_EXSTYLE, dwStyle | WS_EX_RTLREADING);
	return TRUE;
}

void SetStrings(HWND hwnd)
{
	if (!translationfound)
		return;

	if (RTL)
		EnumChildWindows(hwnd, EnumChildProc, 0);

	if (!getString(__T("help")).empty())
		SetWindowText(GetDlgItem(hwnd, IDC_BUTTON4), getString(__T("help")).c_str());
	if (!getString(__T("thumbbg")).empty())
		SetWindowText(GetDlgItem(hwnd, IDC_STATIC2), getString(__T("thumbbg")).c_str());
	if (!getString(__T("transparent")).empty())
		SetWindowText(GetDlgItem(hwnd, IDC_RADIO1), getString(__T("transparent")).c_str());
	if (!getString(__T("albumart")).empty())
		SetWindowText(GetDlgItem(hwnd, IDC_RADIO2), getString(__T("albumart")).c_str());
	if (!getString(__T("defbg")).empty())
		SetWindowText(GetDlgItem(hwnd, IDC_RADIO3), getString(__T("defbg")).c_str());
	if (!getString(__T("custombg")).empty())
		SetWindowText(GetDlgItem(hwnd, IDC_RADIO4), getString(__T("custombg")).c_str());
	if (!getString(__T("browse")).empty())
		SetWindowText(GetDlgItem(hwnd, IDC_BUTTON3), getString(__T("browse")).c_str());
	if (!getString(__T("aa")).empty())
		SetWindowText(GetDlgItem(hwnd, IDC_CHECK8), getString(__T("aa")).c_str());
	if (!getString(__T("buttons")).empty())
		SetWindowText(GetDlgItem(hwnd, IDC_CHECK6), getString(__T("buttons")).c_str());
	if (!getString(__T("shrink")).empty())
		SetWindowText(GetDlgItem(hwnd, IDC_CHECK1), getString(__T("shrink")).c_str());
	if (!getString(__T("taskicon")).empty())
		SetWindowText(GetDlgItem(hwnd, IDC_STATIC5), getString(__T("taskicon")).c_str());
	if (!getString(__T("progressbar")).empty())
		SetWindowText(GetDlgItem(hwnd, IDC_CHECK2), getString(__T("progressbar")).c_str());
	if (!getString(__T("streaming")).empty())
		SetWindowText(GetDlgItem(hwnd, IDC_CHECK4), getString(__T("streaming")).c_str());
	if (!getString(__T("error")).empty())
		SetWindowText(GetDlgItem(hwnd, IDC_CHECK5), getString(__T("error")).c_str());
	if (!getString(__T("overlays")).empty())
		SetWindowText(GetDlgItem(hwnd, IDC_CHECK3), getString(__T("overlays")).c_str());
	if (!getString(__T("jumplist")).empty())
		SetWindowText(GetDlgItem(hwnd, IDC_STATIC6), getString(__T("jumplist")).c_str());
	if (!getString(__T("bugslink")).empty() && !getString(__T("bugstext")).empty())
		SetWindowText(GetDlgItem(hwnd, IDC_SYSLINK2), 
			std::wstring(__T("<a>") + getString(__T("bugslink")) + __T("</a> ") + getString(__T("bugstext"))).c_str());
	if (!getString(__T("sitelink")).empty() && !getString(__T("sitetext")).empty())
		SetWindowText(GetDlgItem(hwnd, IDC_SYSLINK1), 
			std::wstring(__T("<a>") + getString(__T("sitelink")) + __T("</a>  ") + getString(__T("sitetext"))).c_str());
	if (!getString(__T("translink")).empty() && !getString(__T("transtext")).empty())
		SetWindowText(GetDlgItem(hwnd, IDC_SYSLINK3), 
			std::wstring(__T("<a>") + getString(__T("translink")) + __T("</a>  ") + getString(__T("transtext"))).c_str());
	if (!getString(__T("creditslink")).empty() && !getString(__T("creditstext")).empty())
		SetWindowText(GetDlgItem(hwnd, IDC_SYSLINK5), 
			std::wstring(__T("<a>") + getString(__T("creditslink")) + __T("</a>  ") + getString(__T("creditstext"))).c_str());
	if (!getString(__T("noupdate")).empty())
		SetWindowText(GetDlgItem(hwnd, IDC_CHECK9), getString(__T("noupdate")).c_str());
	if (!getString(__T("removetitle")).empty())
		SetWindowText(GetDlgItem(hwnd, IDC_CHECK10), getString(__T("removetitle")).c_str());
	if (!getString(__T("selectfont")).empty())
		SetWindowText(GetDlgItem(hwnd, IDC_BUTTON5), getString(__T("selectfont")).c_str());
	if (!getString(__T("enable")).empty())
		SetWindowText(GetDlgItem(hwnd, IDC_CHECK7), getString(__T("enable")).c_str());
	if (!getString(__T("selectshadow")).empty())
		SetWindowText(GetDlgItem(hwnd, IDC_BUTTON6), getString(__T("selectshadow")).c_str());
	if (!getString(__T("iconic")).empty())
		SetWindowText(GetDlgItem(hwnd, IDC_CHECK25), getString(__T("iconic")).c_str());
	if (!getString(__T("vumeter")).empty())
		SetWindowText(GetDlgItem(hwnd, IDC_CHECK26), getString(__T("vumeter")).c_str());
	if (!getString(__T("addfav")).empty())
		SetWindowText(GetDlgItem(hwnd, IDC_CHECK27), getString(__T("addfav")).c_str());
	if (!getString(__T("author")).empty())
		SetWindowText(GetDlgItem(hwnd, IDC_SYSLINK4), 
			std::wstring(getString(__T("author")) + __T(": Magyari Attila <a>atti86@gmail.com</a>")).c_str());
	if (!getString(__T("thumbpb")).empty())
		SetWindowText(GetDlgItem(hwnd, IDC_CHECK29), getString(__T("thumbpb")).c_str());

	if (!getString(__T("jlrecent")).empty())
		SetWindowText(GetDlgItem(hwnd, IDC_CHECK30), getString(__T("jlrecent")).c_str());
	if (!getString(__T("jlfrequent")).empty())
		SetWindowText(GetDlgItem(hwnd, IDC_CHECK31), getString(__T("jlfrequent")).c_str());
	if (!getString(__T("jltasks")).empty())
		SetWindowText(GetDlgItem(hwnd, IDC_CHECK32), getString(__T("jltasks")).c_str());
	if (!getString(__T("revert")).empty())
		SetWindowText(GetDlgItem(hwnd, IDC_STATIC30), getString(__T("revert")).c_str());
	if (!getString(__T("jlnote")).empty())
		SetWindowText(GetDlgItem(hwnd, IDC_STATIC31), getString(__T("jlnote")).c_str());
	if (!getString(__T("jlchoice")).empty())
		SetWindowText(GetDlgItem(hwnd, IDC_STATIC32), getString(__T("jlchoice")).c_str());
	if (!getString(__T("bookmark")).empty())
		SetWindowText(GetDlgItem(hwnd, IDC_CHECK33), getString(__T("bookmark")).c_str());
}

inline std::wstring getString(std::wstring keyword, std::wstring def)
{
	if (STRINGS.find(keyword) != STRINGS.end())
		return STRINGS.find(keyword)->second;
	else 
		return def;
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
		std::wstring pluginpath;
		pluginpath.reserve(MAX_PATH);
		pluginpath.resize(MAX_PATH, '\0');
		GetModuleFileName(plugin.hDllInstance, &pluginpath[0], MAX_PATH);
		pluginpath.resize(wcslen(pluginpath.c_str()));

		GetShortPathName(pluginpath.c_str(), &pluginpath[0], pluginpath.length());
		pluginpath.resize(wcslen(pluginpath.c_str()));

		std::wstring bms = getBMS();
		JumpList jl;
		if (!jl.CreateJumpList(pluginpath, getString(__T("wapref"), __T("Winamp preferences")), getString(__T("fromstart"), __T("Play from beginning")),
			getString(__T("resumeplay"), __T("Resume playback")), getString(__T("openfile"), __T("Open file")), getString(__T("bookmark"), __T("Bookmarks")),
			Settings.JLrecent, Settings.JLfrequent, Settings.JLtasks, Settings.JLbms, bms) )
			MessageBoxEx(plugin.hwndParent, getString(__T("jumplisterror"), __T("Error creating jump list.")).c_str(), 0, MB_ICONWARNING | MB_OK, 0);
	}	
}

inline bool parsePath(std::wstring &thepath)
{
	std::wstring::size_type pos1 = std::wstring::npos, pos2 = std::wstring::npos;
	
	pos1 = thepath.find_first_of(L'{');
	if (pos1 != std::wstring::npos)
	{
		pos2 = thepath.find_first_of(L'}', pos1+1);
		if (pos2 != std::wstring::npos)
		{
			std::wstring re;
			int clsid;

			re = thepath.substr(pos1+1, pos2 - pos1 - 1);
			std::wstringstream ss;
			ss << re;
			ss >> clsid;

			re.clear();
			re.resize(MAX_PATH);
			SHGetSpecialFolderPath(0, &re[0], clsid, 0);
			re.resize(wcslen(re.c_str()));	

			thepath.replace(pos1, pos2 - pos1 + 1, re);
			return true;
		}
	}

	pos1 = thepath.find_first_of(L'%');
	if (pos1 != std::wstring::npos)
	{
		pos2 = thepath.find_first_of(L'%', pos1+1);
		if (pos2 != std::wstring::npos)
		{
			std::wstring re;
			re = thepath.substr(pos1+1, pos2 - pos1 - 1);
			re = _wgetenv(re.c_str());

			thepath.replace(pos1, pos2 - pos1 + 1, re);
			return true;
		}
	}

	return false;
}

std::wstring getWinampINIPath()
{	
	std::wstring wini;
	wini.resize(MAX_PATH);
	GetPrivateProfileString(L"Winamp", L"inidir", L"", &wini[0], MAX_PATH, std::wstring(getInstallPath() + L"\\paths.ini").c_str());
	wini.resize(wcslen(wini.c_str()));
	
	while (parsePath(wini));

	if (wini.empty())
		return getInstallPath();
	
	return wini;
}

std::wstring getBMS()
{		
	std::wstring path = getWinampINIPath() + L"\\Winamp.bm";		
	std::wifstream is(path.c_str());
	if (is.fail())
		return L"";

	std::wstring data;
	std::getline(is, data, L'\0');
	return data;
}

// This is an export function called by winamp which returns this plugin info.
// We wrap the code in 'extern "C"' to ensure the export isn't mangled if used in a CPP file.
extern "C" __declspec(dllexport) winampGeneralPurposePlugin * winampGetGeneralPurposePlugin() {
  return &plugin;
}

extern "C" int __declspec(dllexport) __stdcall pref() 
{
	HWND hwnd = getWinampWindow();
	if (hwnd == 0)
		MessageBoxEx(0, L"Please start Winamp first", 0, MB_ICONWARNING, 0);
	else
	{
		SendMessage(hwnd,WM_WA_IPC,-1,IPC_OPENPREFSTOPAGE);
		
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
		MessageBoxEx(0, L"Please start Winamp first", 0, MB_ICONWARNING, 0);
	else
	{
		SendMessage(hwnd, WM_WA_IPC, -1, IPC_SETPLAYLISTPOS);
		SendMessage(hwnd, WM_COMMAND, 40045, 0);

	}

	return 0;
}

extern "C" int __declspec(dllexport) __stdcall resume() 
{
	HWND hwnd = getWinampWindow();
	if (hwnd == 0)
		MessageBoxEx(0, L"Please start Winamp first", 0, MB_ICONWARNING, 0);
	else
	{
		if (SendMessage(hwnd,WM_WA_IPC,0,IPC_ISPLAYING) == 1)
			return 0;
		
		std::wstring path;
		path = getWinampINIPath();

		if (path.empty())
			return -1;

		path += L"\\Plugins\\win7shell.ini";

		sResumeSettings sResume;
		ZeroMemory(&sResume, sizeof(sResume));
		if (GetPrivateProfileStruct(__T("win7shell"), __T("resume"), &sResume, sizeof(sResume), path.c_str()))
		{
			SendMessage(hwnd, WM_WA_IPC, sResume.ResumePosition, IPC_SETPLAYLISTPOS);
			SendMessage(hwnd, WM_COMMAND, 40045, 0);
			SendMessage(hwnd, WM_WA_IPC, sResume.ResumeTime, IPC_JUMPTOTIME);
		}		
	}

	return 0;
}

extern "C" int __declspec(dllexport) __stdcall openfile() 
{
	HWND hwnd = getWinampWindow();
	if (hwnd == 0)
		MessageBoxEx(0, L"Please start Winamp first", 0, MB_ICONWARNING, 0);
	else
		SendMessage(hwnd,WM_WA_IPC,(WPARAM)(HWND)0,IPC_OPENFILEBOX);

	return 0;
}

