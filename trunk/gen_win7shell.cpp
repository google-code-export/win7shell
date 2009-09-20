#define WIN32_LEAN_AND_MEAN
#define ICONSIZEPX 50

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
#include <cctype>
#include <fstream>
#include <map>
#include "gen_win7shell.h"
#include "gen.h"
#include "wa_ipc.h"
#include "..\resource.h"
#include "api.h"
#include "VersionChecker.h"
#include "tabs.h"

using namespace Gdiplus;

const std::tstring cur_version(__T("0.82"));
UINT s_uTaskbarRestart=0;
WNDPROC lpWndProcOld = 0;
ITaskbarList3* pTBL = 0 ;
char playstate = -1, progressbarstate = -1;
int gCurPos = -2, gTotal = -2;
std::tstring W_INI;
sSettings Settings;
sSettings_strings Settings_strings;
sFontEx Settings_font;
BOOL cfgopen = false, btaskinited = false, overlaystate = false, gCheckTotal = true, RTL = false;
HWND cfgwindow = 0;
Bitmap *background;
bool AAFound = true;
std::tstring metadatacache;
std::tstring lastfilename;
std::tstring metadatatype;
HICON iPlay, iPause, iStop;
HIMAGELIST theicons;
ULONG_PTR gdiplusToken;
std::map<std::tstring, std::tstring> STRINGS;
	
// these are callback functions/events which will be called by Winamp
VOID CALLBACK TimerProc(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime);
LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
static INT_PTR CALLBACK cfgWndProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK TabHandler(HWND, UINT, WPARAM, LPARAM);

HIMAGELIST prepareIcons();
void updateToolbar(bool first);
void ReadSettings();
void WriteSettings();
int InitTaskbar();
void SetWindowAttr();
HBITMAP DrawThumbnail(int width, int height);
std::tstring GetWinampInfo(int slot);
std::tstring GetMetadata(std::tstring metadata);
std::tstring SearchDir(std::tstring path);
std::tstring MetaWord(std::tstring word);
std::tstring GetLine(int linenumber, bool &center);
void LoadStrings(std::tstring Path);
void FillStrings();
void getinistring(std::tstring what, std::tstring Path);
void SetStrings(HWND hwnd);

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
	char* INI_FILE = (char*)SendMessage(plugin.hwndParent,WM_WA_IPC,0,IPC_GETINIDIRECTORY);	
	int size = MultiByteToWideChar(CP_ACP, MB_COMPOSITE, INI_FILE, -1, NULL, 0);
	std::vector<TCHAR> w_inifile(size+22);
	MultiByteToWideChar(CP_ACP, MB_COMPOSITE, INI_FILE, -1, &w_inifile[0], size);
	W_INI = &w_inifile[0];
	FillStrings();

	WIN32_FIND_DATA ffd;
	if (FindFirstFile(std::wstring(W_INI + __T("\\plugins\\*win7shell.lng")).c_str(), &ffd))
		LoadStrings(W_INI + __T("\\plugins\\") + ffd.cFileName);
		
	W_INI += __T("\\plugins\\win7shell.ini");	

	Settings.VuMeter = false;
	ZeroMemory(&Settings, sizeof(Settings));
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

		std::ofstream of(&w_inifile[0], std::ios_base::out | std::ios_base::binary);	
		of.write("\xFF\xFE", 2);
		of.close();
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

	std::tstring deftext = __T("%c%%r%%title%‡%c%%r%%artist%‡‡Track: %curtime% / %totaltime%‡Volume:  %volume%");
	TCHAR text[1000];	
	GetPrivateProfileString(__T("win7shell"), __T("text"), deftext.c_str(), &text[0], 1000, W_INI.c_str());
	Settings_strings.Text = text;

	if (!Settings.DisableUpdates)
		if (SendMessage(plugin.hwndParent,WM_WA_IPC,0,IPC_INETAVAILABLE))
		{
			VersionChecker *vc = new VersionChecker();
			std::tstring news = vc->IsNewVersion(cur_version);
			if (news != __T(""))
			{
				news.erase(news.length()-1, 1);
				news = STRINGS.find(__T("newvers"))->second + __T("   ") + news;
				news += __T("\n\n") + STRINGS.find(__T("disableupdate"))->second + __T("\n");
				news += STRINGS.find(__T("opendownload"))->second;

				delete vc;
				if (MessageBoxEx(plugin.hwndParent, news.c_str(), __T("Win7 Taskbar Integration Plugin"), MB_ICONINFORMATION | MB_YESNO, 0) == IDYES)
					ShellExecute(NULL, L"open", L"http://code.google.com/p/win7shell/downloads/list", NULL, NULL, SW_SHOW);		
			}
		}

	MSG msg;
	while (PeekMessage(&msg, 0,0,0, PM_REMOVE))
	{
	TranslateMessage(&msg);
	DispatchMessage(&msg);
	}
	
	OSVERSIONINFOW pVI;
	ZeroMemory(&pVI, sizeof(pVI));
	pVI.dwOSVersionInfoSize = sizeof(pVI);
	GetVersionEx(&pVI);

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

	SetTimer(plugin.hwndParent, 6667, 50, TimerProc);

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
	WritePrivateProfileStruct(__T("win7shell"), __T("settings"), &Settings, sizeof(Settings), W_INI.c_str());
	WritePrivateProfileStruct(__T("win7shell"), __T("font"), &Settings_font, sizeof(Settings_font), W_INI.c_str());
	WritePrivateProfileString(__T("win7shell"), __T("bgpath"), Settings_strings.BGPath.c_str(), W_INI.c_str());	
	WritePrivateProfileString(__T("win7shell"), __T("text"), Settings_strings.Text.c_str(), W_INI.c_str());	

	if (background)
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

std::tstring GetMetadata(std::tstring metadata)
{
	int index = SendMessage(plugin.hwndParent,WM_WA_IPC,0,IPC_GETLISTLENGTH);
	if (index == 0)
		return __T("");
	index = SendMessage(plugin.hwndParent,WM_WA_IPC,0,IPC_GETLISTPOS);	
	std::tstring name = (wchar_t*)SendMessage(plugin.hwndParent,WM_WA_IPC,index,IPC_GETPLAYLISTFILEW); 
	
	if ((name == lastfilename) && (!metadatacache.empty()) && (metadata == metadatatype))
		return metadatacache;
		
	metadatatype = metadata;
	lastfilename = name;
	wchar_t value[512];
	extendedFileInfoStructW efs;
	efs.filename= name.c_str();
	efs.metadata=metadata.c_str();
	efs.ret=value;
	efs.retlen=512;
	SendMessage(plugin.hwndParent,WM_WA_IPC,(WPARAM)&efs,IPC_GET_EXTENDED_FILE_INFOW);
	
	name = value;
	if (metadata == __T("year"))
	{
		for (int i=0; i<name.length(); i++)
			if (!std::isdigit(name[i]))
				return __T("");
	}

	if ( (wcslen(value) == 0) && ((metadata == __T("title")) || (metadata == __T("artist"))) )
	{
		name = (wchar_t*) SendMessage(plugin.hwndParent,WM_WA_IPC,0,IPC_GET_PLAYING_TITLE);
		size_t pos = name.find_first_of('-');
		if (pos == std::tstring::npos)
			return std::tstring(__T(""));
		else
		{		
			if (metadata == __T("title"))
				name = std::tstring(name, pos+2, name.length()-(pos-2));
			else if (metadata == __T("artist"))
				name = std::tstring(name, 0, pos-1);
		}
	}
	else
		name = std::tstring(value);
	metadatacache = name;

	return name;
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

	if (word == __T("%title%"))
		return GetMetadata(__T("title"));

	if (word == __T("%artist%"))
		return GetMetadata(__T("artist"));

	if (word == __T("%year%"))
		return GetMetadata(__T("year"));

	if (word == __T("%album%"))
		return GetMetadata(__T("album"));

	if (word == __T("%volume%"))
	{
		std::wstringstream w;
		w << (IPC_GETVOLUME(plugin.hwndParent) * 100) / 255;
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
		w << SendMessage(plugin.hwndParent,WM_WA_IPC,0,IPC_GETLISTPOS)+1;
		return w.str();
	}

	if (word == __T("%totalpl%"))
	{
		std::wstringstream w;
		w << SendMessage(plugin.hwndParent,WM_WA_IPC,0,IPC_GETLISTLENGTH);
		return w.str();
	}

	if (word == __T("%track%"))	
		return GetMetadata(__T("track"));

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

	if (word.substr(0, 4) == __T("%color:"))
		return __T("‡color‡");
	

	return __T("");
}

HIMAGELIST prepareIcons()
{
	InitCommonControls();
	
	HIMAGELIST himlIcons;  
    HICON hicon;  
 
	himlIcons = ImageList_Create(GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), ILC_COLOR32, 6, 0); 	
		
	for (int i = 0; i < 6; ++i)
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

void updateToolbar(bool first)
{
	if (!Settings.Thumbnailbuttons)
		return;

	THUMBBUTTONMASK dwMask =  THB_BITMAP | THB_TOOLTIP;
	THUMBBUTTON thbButtons[5];

	thbButtons[0].dwMask = dwMask;
	thbButtons[0].iId = 0;
	thbButtons[0].iBitmap = 0;
	wcscpy(thbButtons[0].szTip, STRINGS.find(__T("previous"))->second.c_str());	


	thbButtons[1].dwMask = dwMask;
	thbButtons[1].iId = 1;

	int res = SendMessage(plugin.hwndParent,WM_WA_IPC,0,IPC_ISPLAYING);
	if (res == 1)
	{
		thbButtons[1].iBitmap = 4;
		wcscpy(thbButtons[1].szTip, STRINGS.find(__T("pause"))->second.c_str());	
	}
	else
	{
		thbButtons[1].iBitmap = 1;
		wcscpy(thbButtons[1].szTip, STRINGS.find(__T("play"))->second.c_str());			
	}

	thbButtons[2].dwMask = dwMask;
	thbButtons[2].iId = 2;
	thbButtons[2].iBitmap = 2;
	wcscpy(thbButtons[2].szTip, STRINGS.find(__T("stop"))->second.c_str());	

	thbButtons[3].dwMask = dwMask;
	thbButtons[3].iId = 3;
	thbButtons[3].iBitmap = 3;
	wcscpy(thbButtons[3].szTip, STRINGS.find(__T("next"))->second.c_str());	

	thbButtons[4].dwMask = dwMask;
	thbButtons[4].iId = 5;
	thbButtons[4].iBitmap = 5;
	wcscpy(thbButtons[4].szTip, STRINGS.find(__T("favorite"))->second.c_str());	

	if (first)
		pTBL->ThumbBarAddButtons(plugin.hwndParent, ARRAYSIZE(thbButtons), thbButtons);
	else
		pTBL->ThumbBarUpdateButtons(plugin.hwndParent, ARRAYSIZE(thbButtons), thbButtons);
}

std::tstring GetLine(int linenumber, bool &center)
{
	std::tstring::size_type pos = 0, open = -1;
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
			if (open != -1)
			{
				metaword = MetaWord(text.substr(open, pos-open+1));
				if (metaword == __T("‡center‡"))
				{
					metaword = __T("");
					center = true;
				}

				text.replace(open, pos-open+1, metaword);
				open = -1;
				pos = -1;
			}
			else
				open = pos;
			pos++;
		}
	}
	while (pos!=std::tstring::npos);

	return text;
}

HBITMAP DrawThumbnail(int width, int height)
{
	//Create background
	if (!background) 
		switch (Settings.Thumbnailbackground)
		{
		case 0: //transparent
			background = new Bitmap(width, height, PixelFormat32bppARGB);
			AAFound = false;
			break;

		case 1: //album art
			{
				std::tstring name = __T("");
				int index = SendMessage(plugin.hwndParent,WM_WA_IPC,0,IPC_GETLISTLENGTH);
				if (index != 0)
				{
					index = SendMessage(plugin.hwndParent,WM_WA_IPC,0,IPC_GETLISTPOS);
					name = (wchar_t*)SendMessage(plugin.hwndParent,WM_WA_IPC,index,IPC_GETPLAYLISTFILEW); 
								
					std::tstring::size_type pos = name.find_last_of('\\');			
					name = SearchDir(name.substr(0, pos));
				}
			
				if (!name.empty())
				{
					Image img(name.c_str(), true);
					if (img.GetType() != 0)
					{				
						double ratio = (double)img.GetHeight() / (double)(height);
						if (ratio == 0)
							ratio = 1;
						ratio = (double)img.GetWidth() / ratio;
						if (ratio > width)
							ratio = width;
						background = new Bitmap(width, height, PixelFormat32bppARGB);
						Graphics gfx(background);
						gfx.SetInterpolationMode(InterpolationModeHighQualityBicubic);
						if (Settings.AsIcon)
						{							
							gfx.DrawImage(&img, RectF(0, 0, ICONSIZEPX, ICONSIZEPX));
						}
						else
							gfx.DrawImage(&img, RectF((int)(((double)width-(double)ratio)/(double)2), 0, ratio, height));
						AAFound = true;
					}
					else
					{
						AAFound = false;
						if (Settings.AsIcon)
							background = new Bitmap(width, height, PixelFormat32bppARGB);
						else
							background = new Bitmap(plugin.hDllInstance, MAKEINTRESOURCE(IDB_WINAMP));
					}
				}
				else				
				{
					AAFound = false;
					if (Settings.AsIcon)
						background = new Bitmap(width, height, PixelFormat32bppARGB);
					else
						background = new Bitmap(plugin.hDllInstance, MAKEINTRESOURCE(IDB_WINAMP));
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
						gfx.DrawImage(&img, RectF(0, 0, width, height));
				}
				else
				{				
					if (Settings.AsIcon)
					{
						Bitmap tmp(plugin.hDllInstance, MAKEINTRESOURCE(IDB_BACKGROUND));
						background = new Bitmap(width, height, PixelFormat32bppARGB);
						Graphics gfx(background);
						gfx.DrawImage(&tmp, RectF(0, 0, ICONSIZEPX, ICONSIZEPX));
					}
					else
						background = new Bitmap(plugin.hDllInstance, MAKEINTRESOURCE(IDB_BACKGROUND));

					Settings.Thumbnailbackground = 2;
				}				
			}
			else
			{				
				background = new Bitmap(plugin.hDllInstance, MAKEINTRESOURCE(IDB_BACKGROUND));
				Settings.Thumbnailbackground = 2;
			}
			break;

		default: 
			background = new Bitmap(width, height, PixelFormat32bppARGB);
	}


	//Draw text
	HBITMAP retbmp;
	int lines = 1;

	int textheight, lineheight;
	Font font(GetDC(0), const_cast<LOGFONT*> (&Settings_font.font));
	StringFormat sf(StringFormatFlagsNoWrap);

	Graphics tmpgfx(background);					

	RectF rect;
	tmpgfx.MeasureString(__T("gytTFjKQ"), -1, &font, RectF(0, 0, width, height), &sf, &rect);
	lineheight = rect.GetBottom();

	for (int i = 0; i < Settings_strings.Text.length(); i++)
		if (Settings_strings.Text[i] == NEWLINE)
			lines++;

	if (Settings.Shrinkframe)
	{
		textheight = (lines * lineheight);
		if (textheight > height-2)
			textheight = height-2;
	}
	else
		textheight = height-2;

	Bitmap bmp(background->GetWidth(), textheight+2, PixelFormat32bppARGB);
	Graphics gfx(&bmp);
	gfx.SetInterpolationMode(InterpolationModeHighQualityBilinear);
	if (Settings.AsIcon)
		gfx.DrawImage(background, RectF(0, 0, background->GetWidth(), background->GetHeight()));
	else
		gfx.DrawImage(background, RectF(0, 0, background->GetWidth(), textheight+2));

	gfx.SetSmoothingMode(SmoothingModeAntiAlias);		
			
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


	int left = 3;
	if (Settings.AsIcon && AAFound)
		left = ICONSIZEPX + 4;

	for (int i=0; i < lines; i++)
	{
		bool center;
		std::tstring text = GetLine(i, center);
		sf.SetAlignment(static_cast<StringAlignment>(center));

		int yt = i * (lineheight-2) + 2;					
		int yb = yt + lineheight-2;

		if (drawshadow)
		{		
			RectF rect(left+1, yt, 197, yb);				
			gfx.DrawString(text.c_str(), -1, &font, rect, &sf, &bgcolor);					
		}

		RectF rect(left, yt-1, 196, yb-2);				
		gfx.DrawString(text.c_str(), -1, &font, rect, &sf, &fgcolor);
	}
	
	bmp.GetHBITMAP(Color::Black, &retbmp);

	return retbmp;
}

std::tstring SearchDir(std::tstring path)
{
	WIN32_FIND_DATA ffd;

	std::tstring defpath = path + __T("\\");
	defpath = GetMetadata(__T("album"));
	if (!defpath.empty())
	{
		defpath = path + __T("\\") + defpath + __T(".jpg");
		if ((FindFirstFile(defpath.c_str() , &ffd) != INVALID_HANDLE_VALUE))
			return defpath.c_str();
	}

	defpath = path + __T("\\AlbumArtSmall.jpg");
	if ((FindFirstFile(defpath.c_str() , &ffd) != INVALID_HANDLE_VALUE))
		return path + __T("\\AlbumArtSmall.jpg");

	defpath = path + __T("\\AlbumArt*");
	if ((FindFirstFile(defpath.c_str() , &ffd) != INVALID_HANDLE_VALUE))
		return path + __T("\\") + ffd.cFileName;

	defpath = path + __T("\\folder.jpg");
	if ((FindFirstFile(defpath.c_str() , &ffd) != INVALID_HANDLE_VALUE))
		return path + __T("\\folder.jpg");

	defpath = path + __T("\\cover.jpg");
	if ((FindFirstFile(defpath.c_str() , &ffd) != INVALID_HANDLE_VALUE))
		return path + __T("\\cover.jpg");

	defpath = path + __T("\\front.jpg");
	if ((FindFirstFile(defpath.c_str() , &ffd) != INVALID_HANDLE_VALUE))
		return path + __T("\\front.jpg");


	
	
	HANDLE file;

	file = FindFirstFile((path + L"\\*.*").c_str(), &ffd);
	path += L"\\";

	std::list<std::tstring> extensions;
	extensions.push_back(L".bmp");
	extensions.push_back(L".jpg");
	extensions.push_back(L".jpeg");
	extensions.push_back(L".png");
	extensions.push_back(L".gif");
	extensions.push_back(L".ico");
	extensions.push_back(L".exif");
	extensions.push_back(L".tiff");
	extensions.push_back(L".tif");
	extensions.push_back(L".wmf");
	extensions.push_back(L".emf");

	do
	{
		if (!(ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
		{
			std::tstring::size_type pos;
			std::tstring filename = ffd.cFileName;

			for (std::list<std::tstring>::iterator it = extensions.begin(); it != extensions.end(); it++)
			{
				pos =  filename.rfind(static_cast<std::tstring>(*it));
				
				if (pos != std::tstring::npos) 
				{					
					std::tstring::size_type sz = pos + (*it).length();
					if (filename.length() == sz)
						return path + filename;
				}
			}
		}
	}
	while (FindNextFile(file, &ffd) != 0);

	return L"";
}


LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_DWMSENDICONICTHUMBNAIL:
		{
			HBITMAP thumbnail = DrawThumbnail(HIWORD(lParam), LOWORD(lParam));			
			Sleep(60);

			HRESULT hr = DwmSetIconicThumbnail(plugin.hwndParent, thumbnail, 0);
			if (FAILED(hr))
			{
				MessageBoxEx(plugin.hwndParent, STRINGS.find(__T("errorthumb"))->second.c_str(), NULL, MB_ICONERROR, 0);
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
				DwmInvalidateIconicBitmaps(plugin.hwndParent);				
				break;
			case 1:
				{
					int res = SendMessage(plugin.hwndParent,WM_WA_IPC,0,IPC_ISPLAYING);
					if (res == 1)
					{
						SendMessage(plugin.hwndParent, WM_COMMAND, 40046, 0);			
						updateToolbar(false);
					}
					else
					{
						SendMessage(plugin.hwndParent, WM_COMMAND, 40045, 0);
						updateToolbar(false);
					}
				}
				break;
			case 2:
				SendMessage(plugin.hwndParent, WM_COMMAND, 40047, 0);
				playstate = 3; //stopped
				updateToolbar(false);
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
				DwmInvalidateIconicBitmaps(plugin.hwndParent);
				break;
			case 5:
				SendMessage(plugin.hwndParent,WM_WA_IPC,5,IPC_SETRATING);
			}
		break;

		case WM_WA_IPC:
			if ( (lParam == IPC_PLAYING_FILE) || (lParam == IPC_PLAYING_FILEW))
			{
				gCheckTotal = true;
				gCurPos = 0;

				if (Settings.Overlay)
					pTBL->SetOverlayIcon(plugin.hwndParent, ImageList_GetIcon(theicons, 1, 0), __T("Playing"));
				DwmInvalidateIconicBitmaps(plugin.hwndParent);
				updateToolbar(false);

				if (Settings.Add2RecentDocs)
				{
					int index = SendMessage(plugin.hwndParent,WM_WA_IPC,0,IPC_GETLISTPOS);
					std::tstring name = (wchar_t*)SendMessage(plugin.hwndParent,WM_WA_IPC,index,IPC_GETPLAYLISTFILEW); 
					SHAddToRecentDocs(SHARD_PATHW, name.c_str());
				}

				if (Settings.Thumbnailbackground == 1)
					if (background)
					{
						delete background;
						background = 0;
					}
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

VOID CALLBACK TimerProc(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime)
{
	if (Settings.VuMeter)
	{
		static int (*export_sa_get)(int channel) = (int(__cdecl *)(int)) SendMessage(plugin.hwndParent, WM_WA_IPC, 0, IPC_GETVUDATAFUNC);
		int audiodata = export_sa_get(0);

		if (audiodata <= 70)
			pTBL->SetProgressState(plugin.hwndParent, TBPF_NORMAL);
		else if (audiodata > 70 && audiodata < 160)
			pTBL->SetProgressState(plugin.hwndParent, TBPF_PAUSED);
		else
			pTBL->SetProgressState(plugin.hwndParent, TBPF_ERROR);

		pTBL->SetProgressValue(plugin.hwndParent, audiodata, 255);
		progressbarstate = -1;
	}

	if (Settings.RemoveTitle)					
		if (GetWindowTextLength(plugin.hwndParent) != 0)
			SetWindowText(plugin.hwndParent, __T(""));
	int ps = SendMessage(plugin.hwndParent,WM_WA_IPC,0,IPC_ISPLAYING);
	if (playstate == ps)
	{
		int cp = SendMessage(plugin.hwndParent,WM_WA_IPC,0,IPC_GETOUTPUTTIME);
		if (gCurPos == cp)
		{
			return;
		}
		gCurPos = cp;
	}

	if (gCheckTotal || gTotal <= 0)
	{
		gTotal = SendMessage(plugin.hwndParent,WM_WA_IPC,2,IPC_GETOUTPUTTIME);
		gCheckTotal = false;
	}

	playstate = ps;

	switch (playstate)
	{
		case 1: 
				if (!Settings_strings.Text.empty())
					DwmInvalidateIconicBitmaps(plugin.hwndParent);

				if (Settings.Overlay && !overlaystate)
				{
					pTBL->SetOverlayIcon(plugin.hwndParent, ImageList_GetIcon(theicons, 1, 0), 
						STRINGS.find(__T("playing"))->second.c_str());
					overlaystate = true;
				}					
				updateToolbar(false);

				if (Settings.Progressbar)
				{
					if (gCurPos == -1 || gTotal < 0)
					{ 
						if (Settings.Streamstatus)
						{
							if (progressbarstate != TBPF_INDETERMINATE)
							{
								pTBL->SetProgressState(plugin.hwndParent, TBPF_INDETERMINATE);
								progressbarstate = TBPF_INDETERMINATE;
							}
						}
						else						
						{
							if (progressbarstate != TBPF_NOPROGRESS)
							{
								pTBL->SetProgressState(plugin.hwndParent, TBPF_NOPROGRESS);
								progressbarstate = TBPF_NOPROGRESS;
							}							
						}
					}
					else
					{
						if (progressbarstate != TBPF_NORMAL)
						{
							pTBL->SetProgressState(plugin.hwndParent, TBPF_NORMAL);
							progressbarstate = TBPF_NORMAL;
						}
						
						pTBL->SetProgressValue(plugin.hwndParent, gCurPos, gTotal);	
					}
				}
			break;

		case 3: 
				updateToolbar(false);

				if (Settings.Overlay)
					pTBL->SetOverlayIcon(plugin.hwndParent, ImageList_GetIcon(theicons, 4, 0), 
					STRINGS.find(__T("paused"))->second.c_str());
				overlaystate = false;

				if (Settings.Progressbar)
				{
					if (progressbarstate != TBPF_PAUSED)
					{
						pTBL->SetProgressState(plugin.hwndParent, TBPF_PAUSED);
						progressbarstate = TBPF_PAUSED;
					}

					pTBL->SetProgressValue(plugin.hwndParent, gCurPos, gTotal);	

					if (SendMessage(plugin.hwndParent,WM_WA_IPC,1,IPC_GETOUTPUTTIME) == -1)
						pTBL->SetProgressValue(plugin.hwndParent, 100, 100);
				}
			break;

		default:

			if (Settings.Overlay)
				pTBL->SetOverlayIcon(plugin.hwndParent, ImageList_GetIcon(theicons, 2, 0), 
				STRINGS.find(__T("stopped"))->second.c_str());
			overlaystate = false;

			updateToolbar(false);

			if (Settings.Progressbar) 
			{
				if (Settings.Stoppedstatus)
				{
					if (progressbarstate != TBPF_ERROR)
					{
						pTBL->SetProgressState(plugin.hwndParent, TBPF_ERROR);
						progressbarstate = TBPF_ERROR;
					}
					pTBL->SetProgressValue(plugin.hwndParent, 100, 100);
				}
				else
					if (progressbarstate != TBPF_NOPROGRESS)
					{
						pTBL->SetProgressState(plugin.hwndParent, TBPF_NOPROGRESS);
						progressbarstate = TBPF_NOPROGRESS;
					}
			}
			break;
	}
}

void SetWindowAttr()
{
	DwmInvalidateIconicBitmaps(plugin.hwndParent);
	if (Settings.Thumbnailenabled)
	{
		BOOL b = TRUE;
		DwmSetWindowAttribute(plugin.hwndParent, DWMWA_HAS_ICONIC_BITMAP, &b, sizeof(b));
		DwmSetWindowAttribute(plugin.hwndParent, DWMWA_FORCE_ICONIC_REPRESENTATION, &b, sizeof(b));
	}
	else
	{
		BOOL b = FALSE;
		DwmSetWindowAttribute(plugin.hwndParent, DWMWA_HAS_ICONIC_BITMAP, &b, sizeof(b));
		DwmSetWindowAttribute(plugin.hwndParent, DWMWA_FORCE_ICONIC_REPRESENTATION, &b, sizeof(b));
	}
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

	//recentdocs
	SendMessage(GetDlgItem(hwnd, IDC_CHECK19), (UINT) BM_SETCHECK, Settings.Add2RecentDocs, 0);

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
	if (GetDlgItem(hwnd, IDC_CHECK19) != NULL)
		Settings.Add2RecentDocs = SendMessage(GetDlgItem(hwnd, IDC_CHECK19), (UINT) BM_GETCHECK, 0, 0);
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

	static bool bbutton;
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

			if (STRINGS.find(__T("ok"))!= STRINGS.end())
				SetWindowText(GetDlgItem(hwnd, IDC_BUTTON1), STRINGS.find(__T("ok"))->second.c_str());
			if (STRINGS.find(__T("cancel"))!= STRINGS.end())
				SetWindowText(GetDlgItem(hwnd, IDC_BUTTON2), STRINGS.find(__T("cancel"))->second.c_str());

			int Index = 0;
			if (STRINGS.find(__T("thumbtext"))!= STRINGS.end())
				Index = AddTab(TabCtrl, Tab1, const_cast<wchar_t*> (STRINGS.find(__T("thumbtext"))->second.c_str()), -1);
			else
				Index = AddTab(TabCtrl, Tab1, __T("Text"), -1);

			if (STRINGS.find(__T("background"))!= STRINGS.end())
				AddTab(TabCtrl, Tab2, const_cast<wchar_t*> (STRINGS.find(__T("background"))->second.c_str()), -1);
			else
				AddTab(TabCtrl, Tab2, __T("Background"), -1);

			if (STRINGS.find(__T("thumboptions"))!= STRINGS.end())
				AddTab(TabCtrl, Tab3, const_cast<wchar_t*> (STRINGS.find(__T("thumboptions"))->second.c_str()), -1);
			else
				AddTab(TabCtrl, Tab3, __T("Options"), -1);

			if (STRINGS.find(__T("taskicon"))!= STRINGS.end())
				AddTab(TabCtrl, Tab4, const_cast<wchar_t*> (STRINGS.find(__T("taskicon"))->second.c_str()), -1);
			else
				AddTab(TabCtrl, Tab4, __T("Taskbar Icon"), -1);

			if (STRINGS.find(__T("jumplist"))!= STRINGS.end())
				AddTab(TabCtrl, Tab5, const_cast<wchar_t*> (STRINGS.find(__T("jumplist"))->second.c_str()), -1);
			else
				AddTab(TabCtrl, Tab5, __T("Jump List"), -1);

			std::tstring cv;
			if (STRINGS.find(__T("plugdev"))!= STRINGS.end())
			{
				cv = STRINGS.find(__T("plugdev"))->second + __T(" v") + cur_version;
				AddTab(TabCtrl, Tab6, const_cast<wchar_t*> (cv.c_str()), -1);
			}
			else
			{
				cv = __T("Development v") + cur_version;
				AddTab(TabCtrl, Tab6, &cv[0], -1);
			}

			TabToFront(TabCtrl, Index);

			cfgopen = TRUE;
			bbutton = Settings.Thumbnailbuttons;
		}
		break;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
			case IDC_BUTTON1:
				DwmInvalidateIconicBitmaps(plugin.hwndParent);
				playstate = -1;
				cfgopen = FALSE;
				WriteSettings(Tab1);
				WriteSettings(Tab2);
				WriteSettings(Tab3);
				WriteSettings(Tab4);
				WriteSettings(Tab5);
				WriteSettings(Tab6);
				if (background)
				{
					delete background;
					background = 0;
				}

				if (!Settings.Progressbar)
					pTBL->SetProgressState(plugin.hwndParent, TBPF_NOPROGRESS);
	
				if (bbutton != Settings.Thumbnailbuttons )
					MessageBoxEx(cfgwindow, STRINGS.find(__T("afterrestart"))->second.c_str(),
					STRINGS.find(__T("applysettings"))->second.c_str(), MB_ICONWARNING, 0);

				if (!Settings.Overlay)
					pTBL->SetOverlayIcon(plugin.hwndParent, NULL, NULL);
				else
					playstate = -1;


				SetWindowAttr();
				DestroyWindow(hwnd);
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
HWND TabWindow;
int Index;

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
				case IDC_BUTTON7:
					{
						
					}
				case IDC_CHECK2:
					{
						bool checked = SendMessage(GetDlgItem(hwnd, IDC_CHECK2), (UINT) BM_GETCHECK, 0, 0);
						EnableWindow(GetDlgItem(hwnd, IDC_CHECK4), checked);
						EnableWindow(GetDlgItem(hwnd, IDC_CHECK5), checked);
						if (SendMessage(GetDlgItem(hwnd, IDC_CHECK2), (UINT) BM_GETCHECK, 0, 0))
							SendMessage(GetDlgItem(hwnd, IDC_CHECK26), (UINT) BM_SETCHECK, 0, 0);		
					}
					break;
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
				}
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

	getinistring(__T("newvers"), Path);
	getinistring(__T("disableupdate"), Path);
	getinistring(__T("opendownload"), Path);
	getinistring(__T("uninstall"), Path);
	getinistring(__T("uninstallhowto"), Path);
	getinistring(__T("info"), Path);
	getinistring(__T("config"), Path);
	getinistring(__T("on"), Path);
	getinistring(__T("off"), Path);
	getinistring(__T("previous"), Path);
	getinistring(__T("pause"), Path);
	getinistring(__T("play"), Path);
	getinistring(__T("stop"), Path);
	getinistring(__T("next"), Path);
	getinistring(__T("errorthumb"), Path);
	getinistring(__T("playing"), Path);
	getinistring(__T("paused"), Path);
	getinistring(__T("stopped"), Path);
	getinistring(__T("taskinitfail"), Path);
	getinistring(__T("plugindev"), Path);
	getinistring(__T("images"), Path);
	getinistring(__T("usethumbbg"), Path);
	getinistring(__T("selectimage"), Path);
	getinistring(__T("afterrestart"), Path);
	getinistring(__T("applysettings"), Path);
	getinistring(__T("help1"), Path);
	getinistring(__T("help2"), Path);
	getinistring(__T("help3"), Path);
	getinistring(__T("help5"), Path);
	getinistring(__T("help6"), Path);
	getinistring(__T("help7"), Path);
	getinistring(__T("help8"), Path);
	getinistring(__T("help9"), Path);
	getinistring(__T("help10"), Path);
	getinistring(__T("help11"), Path);
	getinistring(__T("help12"), Path);
	getinistring(__T("help13"), Path);
	getinistring(__T("help14"), Path);
	getinistring(__T("help15"), Path);
	getinistring(__T("help16"), Path);
	getinistring(__T("help17"), Path);
	getinistring(__T("help18"), Path);
	getinistring(__T("help19"), Path);
	getinistring(__T("help20"), Path);
	getinistring(__T("help21"), Path);	

	//GUI
	getinistring(__T("help"), Path);
	getinistring(__T("thumbbg"), Path);
	getinistring(__T("transparent"), Path);
	getinistring(__T("albumart"), Path);
	getinistring(__T("defbg"), Path);
	getinistring(__T("custombg"), Path);
	getinistring(__T("browse"), Path);
	getinistring(__T("aa"), Path);
	getinistring(__T("buttons"), Path);
	getinistring(__T("shrink"), Path);
	getinistring(__T("taskicon"), Path);
	getinistring(__T("progressbar"), Path);
	getinistring(__T("streaming"), Path);
	getinistring(__T("error"), Path);
	getinistring(__T("overlays"), Path);
	getinistring(__T("jumplist"), Path);
	getinistring(__T("addtorecent"), Path);
	getinistring(__T("bugslink"), Path);
	getinistring(__T("bugstext"), Path);
	getinistring(__T("sitelink"), Path);
	getinistring(__T("sitetext"), Path);
	getinistring(__T("noupdate"), Path);
	getinistring(__T("ok"), Path);
	getinistring(__T("cancel"), Path);	
	getinistring(__T("removetitle"), Path);
	getinistring(__T("selectfont"), Path);
	getinistring(__T("selectshadow"), Path);
	getinistring(__T("enable"), Path);
	getinistring(__T("plugdev"), Path);
	getinistring(__T("thumboptions"), Path);
	getinistring(__T("thumbtext"), Path);
	getinistring(__T("translink"), Path);
	getinistring(__T("transtext"), Path);
	getinistring(__T("favorite"), Path);
	getinistring(__T("vumeter"), Path);
	getinistring(__T("iconic"), Path);
	getinistring(__T("background"), Path);
}

BOOL CALLBACK EnumChildProc(HWND hwndChild, LPARAM lParam) 
{ 
	SetWindowLong(hwndChild, GWL_EXSTYLE, 0x00400000);
	return TRUE;
}

void SetStrings(HWND hwnd)
{
	if (RTL)
		EnumChildWindows(hwnd, EnumChildProc, 0);

	if (STRINGS.find(__T("help"))!= STRINGS.end())
		SetWindowText(GetDlgItem(hwnd, IDC_BUTTON4), STRINGS.find(__T("help"))->second.c_str());
	if (STRINGS.find(__T("thumbbg"))!= STRINGS.end())
		SetWindowText(GetDlgItem(hwnd, IDC_STATIC2), STRINGS.find(__T("thumbbg"))->second.c_str());
	if (STRINGS.find(__T("transparent"))!= STRINGS.end())
		SetWindowText(GetDlgItem(hwnd, IDC_RADIO1), STRINGS.find(__T("transparent"))->second.c_str());
	if (STRINGS.find(__T("albumart"))!= STRINGS.end())
		SetWindowText(GetDlgItem(hwnd, IDC_RADIO2), STRINGS.find(__T("albumart"))->second.c_str());
	if (STRINGS.find(__T("defbg"))!= STRINGS.end())
		SetWindowText(GetDlgItem(hwnd, IDC_RADIO3), STRINGS.find(__T("defbg"))->second.c_str());
	if (STRINGS.find(__T("custombg"))!= STRINGS.end())
		SetWindowText(GetDlgItem(hwnd, IDC_RADIO4), STRINGS.find(__T("custombg"))->second.c_str());
	if (STRINGS.find(__T("browse"))!= STRINGS.end())
		SetWindowText(GetDlgItem(hwnd, IDC_BUTTON3), STRINGS.find(__T("browse"))->second.c_str());	
	if (STRINGS.find(__T("aa"))!= STRINGS.end())
		SetWindowText(GetDlgItem(hwnd, IDC_CHECK8), STRINGS.find(__T("aa"))->second.c_str());
	if (STRINGS.find(__T("buttons"))!= STRINGS.end())
		SetWindowText(GetDlgItem(hwnd, IDC_CHECK6), STRINGS.find(__T("buttons"))->second.c_str());
	if (STRINGS.find(__T("shrink"))!= STRINGS.end())
		SetWindowText(GetDlgItem(hwnd, IDC_CHECK1), STRINGS.find(__T("shrink"))->second.c_str());
	if (STRINGS.find(__T("taskicon"))!= STRINGS.end())
		SetWindowText(GetDlgItem(hwnd, IDC_STATIC5), STRINGS.find(__T("taskicon"))->second.c_str());
	if (STRINGS.find(__T("progressbar"))!= STRINGS.end())
		SetWindowText(GetDlgItem(hwnd, IDC_CHECK2), STRINGS.find(__T("progressbar"))->second.c_str());
	if (STRINGS.find(__T("streaming"))!= STRINGS.end())
		SetWindowText(GetDlgItem(hwnd, IDC_CHECK4), STRINGS.find(__T("streaming"))->second.c_str());
	if (STRINGS.find(__T("error"))!= STRINGS.end())
		SetWindowText(GetDlgItem(hwnd, IDC_CHECK5), STRINGS.find(__T("error"))->second.c_str());
	if (STRINGS.find(__T("overlays"))!= STRINGS.end())
		SetWindowText(GetDlgItem(hwnd, IDC_CHECK3), STRINGS.find(__T("overlays"))->second.c_str());
	if (STRINGS.find(__T("jumplist"))!= STRINGS.end())
		SetWindowText(GetDlgItem(hwnd, IDC_STATIC6), STRINGS.find(__T("jumplist"))->second.c_str());
	if (STRINGS.find(__T("addtorecent"))!= STRINGS.end())
		SetWindowText(GetDlgItem(hwnd, IDC_CHECK19), STRINGS.find(__T("addtorecent"))->second.c_str());
	if (STRINGS.find(__T("bugslink"))!= STRINGS.end() && STRINGS.find(__T("bugstext"))!= STRINGS.end() )
	{
		std::tstring tmp = __T("<a>") + STRINGS.find(__T("bugslink"))->second + __T("</a>");
		tmp += __T(" - ") + STRINGS.find(__T("bugstext"))->second;
		SetWindowText(GetDlgItem(hwnd, IDC_SYSLINK2), tmp.c_str());
	}
	if (STRINGS.find(__T("sitelink"))!= STRINGS.end() && STRINGS.find(__T("sitetext"))!= STRINGS.end() )
	{
		std::tstring tmp = __T("<a>") + STRINGS.find(__T("sitelink"))->second + __T("</a>");
		tmp += __T(" - ") + STRINGS.find(__T("sitetext"))->second;
		SetWindowText(GetDlgItem(hwnd, IDC_SYSLINK1), tmp.c_str());
	}
	if (STRINGS.find(__T("translink"))!= STRINGS.end() && STRINGS.find(__T("transtext"))!= STRINGS.end() )
	{
		std::tstring tmp = __T("<a>") + STRINGS.find(__T("translink"))->second + __T("</a>");
		tmp += __T(" - ") + STRINGS.find(__T("transtext"))->second;
		SetWindowText(GetDlgItem(hwnd, IDC_SYSLINK3), tmp.c_str());
	}
	if (STRINGS.find(__T("noupdate"))!= STRINGS.end())
		SetWindowText(GetDlgItem(hwnd, IDC_CHECK9), STRINGS.find(__T("noupdate"))->second.c_str());
	
	if (STRINGS.find(__T("removetitle"))!= STRINGS.end())
		SetWindowText(GetDlgItem(hwnd, IDC_CHECK10), STRINGS.find(__T("removetitle"))->second.c_str());
	if (STRINGS.find(__T("selectfont"))!= STRINGS.end())
		SetWindowText(GetDlgItem(hwnd, IDC_BUTTON5), STRINGS.find(__T("selectfont"))->second.c_str());
	if (STRINGS.find(__T("enable"))!= STRINGS.end())
		SetWindowText(GetDlgItem(hwnd, IDC_CHECK7), STRINGS.find(__T("enable"))->second.c_str());
	if (STRINGS.find(__T("selectshadow"))!= STRINGS.end())
		SetWindowText(GetDlgItem(hwnd, IDC_BUTTON6), STRINGS.find(__T("selectshadow"))->second.c_str());
	if (STRINGS.find(__T("iconic"))!= STRINGS.end())
		SetWindowText(GetDlgItem(hwnd, IDC_CHECK25), STRINGS.find(__T("iconic"))->second.c_str());
	if (STRINGS.find(__T("vumeter"))!= STRINGS.end())
		SetWindowText(GetDlgItem(hwnd, IDC_CHECK26), STRINGS.find(__T("vumeter"))->second.c_str());


}

// This is an export function called by winamp which returns this plugin info.
// We wrap the code in 'extern "C"' to ensure the export isn't mangled if used in a CPP file.
extern "C" __declspec(dllexport) winampGeneralPurposePlugin * winampGetGeneralPurposePlugin() {
  return &plugin;
}
