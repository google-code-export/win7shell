#define WIN32_LEAN_AND_MEAN

#pragma once

#include <windows.h>
#include <commctrl.h>
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
#include "resource.h"
#include "api.h"
#include "VersionChecker.h"

using namespace Gdiplus;

const std::tstring cur_version(__T("0.71"));
UINT s_uTaskbarRestart=0;
WNDPROC lpWndProcOld = 0;
ITaskbarList3* pTBL = 0 ;
char playstate = -1, progressbarstate = -1;
int gCurPos = -2, gTotal = -2;
std::tstring W_INI;
sSettings Settings;
sSettings_strings Settings_strings;
BOOL cfgopen = false, btaskinited = false, overlaystate = false, gCheckTotal = true;
HWND cfgwindow = 0;
Bitmap *background;
bool bgloaded = false;
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

HIMAGELIST prepareIcons();
void updateToolbar(bool first);
void LOADSETTINGS();
void SAVESETTINGS();
void ENABLECONTROLS(HWND hwnd, BOOL enable, BOOL upper);		
int INITTASKBAR();
void SETWINDOWATTR();
HBITMAP DRAWTHUMBNAIL(int width, int height);
std::tstring GetWinampInfo(int slot);
std::tstring GetMetadata(std::tstring metadata);
std::tstring SearchDir(std::tstring path);
std::tstring MetaWord(std::tstring word);
std::tstring GetLine(int linenumber, bool &center, bool &redshadow);
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
	LoadStrings(W_INI + __T("\\plugins\\win7shell.lng"));
	W_INI += __T("\\plugins\\win7shell.ini");		

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
		Settings.Boldfont = false;
		Settings.Add2RecentDocs = true;
		Settings.FontSize = 14;
		Settings.Shrinkframe = true;
		Settings.Antialis = true;
		Settings.ForceVersion = false;
		Settings.DisableUpdates = false;

		std::ofstream of(&w_inifile[0], std::ios_base::out | std::ios_base::binary);	
		of.write("\xFF\xFE", 2);
		of.close();
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

	if (!Settings.ForceVersion)
		if ((pVI.dwMajorVersion != 6) || (pVI.dwMinorVersion != 1))
		{
			std::tstring msg = STRINGS.find(__T("wrongwindows"))->second + __T("\n\n") + STRINGS.find(__T("installanyways"))->second;
			if ( MessageBoxEx(plugin.hwndParent, msg.c_str(), 
				STRINGS.find(__T("wrongostitle"))->second.c_str(), MB_ICONSTOP | MB_YESNO, 0) == IDYES )
			{
				msg = STRINGS.find(__T("uninstall"))->second + __T("\n\n") + STRINGS.find(__T("uninstallhowto"))->second;
				MessageBoxEx(plugin.hwndParent, msg.c_str(), STRINGS.find(__T("info"))->second.c_str(), MB_ICONINFORMATION, 0);
				Settings.ForceVersion = true;
			}
			else
				return -1;
		}	

	theicons = prepareIcons();

	if ( (!INITTASKBAR()) && (Settings.Thumbnailbuttons) )
	{
		HRESULT hr = pTBL->ThumbBarSetImageList(plugin.hwndParent, theicons);
		if (SUCCEEDED(hr))
			updateToolbar(true);
	}

	SETWINDOWATTR();

	GdiplusStartupInput gdiplusStartupInput;    

	GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

	if (IsWindowUnicode(plugin.hwndParent))
		lpWndProcOld = (WNDPROC)SetWindowLongW(plugin.hwndParent,GWL_WNDPROC,(LONG)WndProc);
	else
		lpWndProcOld = (WNDPROC)SetWindowLongA(plugin.hwndParent,GWL_WNDPROC,(LONG)WndProc);

	SetTimer(plugin.hwndParent, 6667, 100, TimerProc);

	return 0;
}

void config() 
{
	if ((cfgopen) && (cfgwindow != NULL))
	{
		SetForegroundWindow(cfgwindow);
		return;
	}
	cfgwindow = CreateDialogParam(plugin.hDllInstance,MAKEINTRESOURCE(IDD_DIALOG1),plugin.hwndParent,cfgWndProc,0);
	SetWindowText(cfgwindow, STRINGS.find(__T("config"))->second.c_str());
	ShowWindow(cfgwindow,SW_SHOW);
}
 
void quit() 
{	
	WritePrivateProfileStruct(__T("win7shell"), __T("settings"), &Settings, sizeof(Settings), W_INI.c_str());
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

	if (word == __T("%r%"))
		return __T("‡red‡");

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

	return __T("");
}

HIMAGELIST prepareIcons()
{
	InitCommonControls();
	
	HIMAGELIST himlIcons;  
    HICON hicon;  
 
	himlIcons = ImageList_Create(GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), ILC_COLOR32, 5, 0); 	
		
	for (int i = 0; i < 5; ++i)
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
	THUMBBUTTON thbButtons[4];

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

	if (first)
		pTBL->ThumbBarAddButtons(plugin.hwndParent, ARRAYSIZE(thbButtons), thbButtons);
	else
		pTBL->ThumbBarUpdateButtons(plugin.hwndParent, ARRAYSIZE(thbButtons), thbButtons);
}

std::tstring GetLine(int linenumber, bool &center, bool &redshadow)
{
	std::tstring::size_type pos = 0, open = -1;
	std::tstring text, metaword;
	center = 0;
	redshadow = 0;
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
				else if (metaword == __T("‡red‡"))
				{
					metaword = __T("");
					redshadow = true;
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

HBITMAP DRAWTHUMBNAIL(int width, int height)
{
	//Create background
	if (!bgloaded) 
		switch (Settings.Thumbnailbackground)
		{
		case 0: //transparent
			background = new Bitmap(width, height, PixelFormat32bppARGB);
			bgloaded = true;
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
						gfx.DrawImage(&img, RectF((int)(((double)width-(double)ratio)/(double)2), 0, ratio, height));
					}
					else
						background = new Bitmap(plugin.hDllInstance, MAKEINTRESOURCE(IDB_WINAMP));
				}
				else				
					background = new Bitmap(plugin.hDllInstance, MAKEINTRESOURCE(IDB_WINAMP));
				bgloaded = true;
			}
			break;

		case 2: //default
			background = new Bitmap(plugin.hDllInstance, MAKEINTRESOURCE(IDB_BACKGROUND));
			bgloaded = true;
			break;

		case 3: //custom
			if (!Settings_strings.BGPath.empty())
			{
				Image img(Settings_strings.BGPath.c_str(), true);
				if (img.GetType() != 0)
				{
					background = new Bitmap(width, height, PixelFormat32bppRGB);
					Graphics gfx(background);
					gfx.SetInterpolationMode(InterpolationModeHighQualityBilinear);
					gfx.DrawImage(&img, RectF(0, 0, width, height));
				}
				else
				{				
					background = new Bitmap(plugin.hDllInstance, MAKEINTRESOURCE(IDB_BACKGROUND));
					Settings.Thumbnailbackground = 2;
				}				
			}
			else
			{				
				background = new Bitmap(plugin.hDllInstance, MAKEINTRESOURCE(IDB_BACKGROUND));
				Settings.Thumbnailbackground = 2;
			}
			bgloaded = true;
			break;
	}


	//Draw text
	HBITMAP retbmp;
	int lines = 1;

	int weight = 0, textheight, lineheight, linespacing = 1;
	Settings.Boldfont ? weight = FontStyleBold : weight = 0;
	Font font(L"Segoe UI", Settings.FontSize, weight, UnitPixel);
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
		textheight = (lines * lineheight) + ((lines-1) * linespacing); 
		if (textheight > height-5)
			textheight = height-5;
	}
	else
		textheight = height-5;

	Bitmap bmp(background->GetWidth(), textheight+5, PixelFormat32bppARGB);
	Graphics gfx(&bmp);
	gfx.SetInterpolationMode(InterpolationModeHighQualityBilinear);
	gfx.DrawImage(background, RectF(0, 0, background->GetWidth(), textheight+5));

	gfx.SetSmoothingMode(SmoothingModeAntiAlias);		
			
	if ( (Settings.Thumbnailbackground == 0) || (Settings.Thumbnailbackground == 1) )
		(Settings.Antialis) ? gfx.SetTextRenderingHint(TextRenderingHintAntiAlias) : gfx.SetTextRenderingHint(TextRenderingHintAntiAliasGridFit);
	else	
		(Settings.Antialis) ? gfx.SetTextRenderingHint(TextRenderingHintAntiAlias) : gfx.SetTextRenderingHint(TextRenderingHintClearTypeGridFit);		

	
	for (int i=0; i < lines; i++)
	{
		bool center, redshadow;
		std::tstring text = GetLine(i, center, redshadow);
		sf.SetAlignment(static_cast<StringAlignment>(center));

		int yt = i * (lineheight-2) + 5;
		//yt += (i+1) * linespacing;				
		int yb = yt + lineheight-2;

		if (!Settings.Thumbnailbackground == 0)
		{		
			SolidBrush *textb;
			if (redshadow) 
				textb = new SolidBrush(Color::Red);
			else
				textb = new SolidBrush(Color::Black);

			RectF rect(5, yt, 197, yb);				
			gfx.DrawString(text.c_str(), -1, &font, rect, &sf, textb);		
			delete textb;
		}

		SolidBrush textb(Color::White);
		RectF rect(4, yt-1, 196, yb-2);				
		gfx.DrawString(text.c_str(), -1, &font, rect, &sf, &textb);	
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
			HBITMAP thumbnail = DRAWTHUMBNAIL(HIWORD(lParam), LOWORD(lParam));			
			Sleep(50);

			HRESULT hr = DwmSetIconicThumbnail(plugin.hwndParent, thumbnail, 0);
			if (FAILED(hr))
			{
				MessageBoxEx(plugin.hwndParent, STRINGS.find(__T("errorthumb"))->second.c_str(), NULL, MB_ICONERROR, 0);
				SETWINDOWATTR();
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
				if (bgloaded)
				{
					delete background;
					bgloaded = false;
				}
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
				if (bgloaded)
				{
					delete background;
					bgloaded = false;
				}
				DwmInvalidateIconicBitmaps(plugin.hwndParent);
				break;
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
					if (bgloaded)
					{
						delete background;
						bgloaded = false;
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
					if (gCurPos == -1)
					{ 
						if (Settings.Streamstatus)
							if (progressbarstate != TBPF_INDETERMINATE)
							{
								pTBL->SetProgressState(plugin.hwndParent, TBPF_INDETERMINATE);
								progressbarstate = TBPF_INDETERMINATE;
							}
						else						
							if (progressbarstate != TBPF_NOPROGRESS)
							{
								pTBL->SetProgressState(plugin.hwndParent, TBPF_NOPROGRESS);
								progressbarstate = TBPF_NOPROGRESS;
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

void SETWINDOWATTR()
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

int INITTASKBAR()
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

void READSETTINGS(HWND hwnd)
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

	//bold
	SendMessage(GetDlgItem(hwnd, IDC_CHECK18), (UINT) BM_SETCHECK, Settings.Boldfont, 0);
	
	//recentdocs
	SendMessage(GetDlgItem(hwnd, IDC_CHECK19), (UINT) BM_SETCHECK, Settings.Add2RecentDocs, 0);

	//antialias
	SendMessage(GetDlgItem(hwnd, IDC_CHECK8), (UINT) BM_SETCHECK, Settings.Antialis, 0);
	
	//shrinkframe
	SendMessage(GetDlgItem(hwnd, IDC_CHECK1), (UINT) BM_SETCHECK, Settings.Shrinkframe, 0);

	//enable thumbnail
	SendMessage(GetDlgItem(hwnd, IDC_CHECK7), (UINT) BM_SETCHECK, Settings.Thumbnailenabled, 0);

	//disable updated
	SendMessage(GetDlgItem(hwnd, IDC_CHECK9), (UINT) BM_SETCHECK, Settings.DisableUpdates, 0);

	//fontsize
	std::wstringstream w;
	w << Settings.FontSize;
	SetWindowText(GetDlgItem(hwnd, IDC_EDIT1), w.str().c_str());

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

	std::tstring cv = STRINGS.find(__T("plugindev"))->second + __T("  ") + cur_version;
	SetWindowText(GetDlgItem(hwnd, IDC_STATIC3), cv.c_str());
}

void WRITESETTINGS(HWND hwnd)
{
	//Checkboxes
	Settings.Overlay = SendMessage(GetDlgItem(hwnd, IDC_CHECK3), (UINT) BM_GETCHECK, 0, 0);
	Settings.Progressbar = SendMessage(GetDlgItem(hwnd, IDC_CHECK2), (UINT) BM_GETCHECK, 0, 0);
	Settings.Streamstatus = SendMessage(GetDlgItem(hwnd, IDC_CHECK4), (UINT) BM_GETCHECK, 0, 0);
	Settings.Stoppedstatus = SendMessage(GetDlgItem(hwnd, IDC_CHECK5), (UINT) BM_GETCHECK, 0, 0);
	Settings.Thumbnailbuttons = SendMessage(GetDlgItem(hwnd, IDC_CHECK6), (UINT) BM_GETCHECK, 0, 0);

	Settings.Thumbnailenabled = SendMessage(GetDlgItem(hwnd, IDC_CHECK7), (UINT) BM_GETCHECK, 0, 0);
	Settings.Boldfont = SendMessage(GetDlgItem(hwnd, IDC_CHECK18), (UINT) BM_GETCHECK, 0, 0);
	Settings.Add2RecentDocs = SendMessage(GetDlgItem(hwnd, IDC_CHECK19), (UINT) BM_GETCHECK, 0, 0);
	Settings.Antialis = SendMessage(GetDlgItem(hwnd, IDC_CHECK8), (UINT) BM_GETCHECK, 0, 0);
	Settings.Shrinkframe = SendMessage(GetDlgItem(hwnd, IDC_CHECK1), (UINT) BM_GETCHECK, 0, 0);
	Settings.DisableUpdates = SendMessage(GetDlgItem(hwnd, IDC_CHECK9), (UINT) BM_GETCHECK, 0, 0);

	//Radiobuttons
	if ( SendMessage(GetDlgItem(hwnd, IDC_RADIO1), (UINT) BM_GETCHECK, 0 , 0) )
		Settings.Thumbnailbackground = 0;
	else if ( SendMessage(GetDlgItem(hwnd, IDC_RADIO2), (UINT) BM_GETCHECK, 0 , 0) )
		Settings.Thumbnailbackground = 1;
	else if ( SendMessage(GetDlgItem(hwnd, IDC_RADIO3), (UINT) BM_GETCHECK, 0 , 0) )
		Settings.Thumbnailbackground = 2;
	else if ( SendMessage(GetDlgItem(hwnd, IDC_RADIO4), (UINT) BM_GETCHECK, 0 , 0) )
		Settings.Thumbnailbackground = 3;

	wchar_t wc[4];	
	GetWindowText(GetDlgItem(hwnd, IDC_EDIT1), wc, 3);
	Settings.FontSize = _wtoi(wc);
	if (Settings.FontSize == 0)
		Settings.FontSize = 14;

	int size = GetWindowTextLength(GetDlgItem(hwnd, IDC_EDIT2));
	std::vector<TCHAR> buf1(size+1);
	GetWindowText(GetDlgItem(hwnd, IDC_EDIT2), &buf1[0], size+1);	
	Settings_strings.BGPath = &buf1[0];

	size = GetWindowTextLength(GetDlgItem(hwnd, IDC_EDIT3));
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

void ENABLECONTROLS(HWND hwnd, BOOL enable, BOOL upper)
{
	if (upper)
	{			
		EnableWindow(GetDlgItem(hwnd, IDC_STATIC1), enable);
		EnableWindow(GetDlgItem(hwnd, IDC_STATIC2), enable);
		EnableWindow(GetDlgItem(hwnd, IDC_BUTTON4), enable);
		EnableWindow(GetDlgItem(hwnd, IDC_BUTTON3), enable);
		EnableWindow(GetDlgItem(hwnd, IDC_EDIT1), enable);
		EnableWindow(GetDlgItem(hwnd, IDC_EDIT2), enable);
		EnableWindow(GetDlgItem(hwnd, IDC_EDIT3), enable);
		EnableWindow(GetDlgItem(hwnd, IDC_RADIO1), enable);
		EnableWindow(GetDlgItem(hwnd, IDC_RADIO2), enable);
		EnableWindow(GetDlgItem(hwnd, IDC_RADIO3), enable);
		EnableWindow(GetDlgItem(hwnd, IDC_RADIO4), enable);
		EnableWindow(GetDlgItem(hwnd, IDC_RADIO1), enable);
		EnableWindow(GetDlgItem(hwnd, IDC_STATIC1), enable);
		EnableWindow(GetDlgItem(hwnd, IDC_CHECK8), enable);
		EnableWindow(GetDlgItem(hwnd, IDC_CHECK18), enable);
		EnableWindow(GetDlgItem(hwnd, IDC_CHECK1), enable);
		EnableWindow(GetDlgItem(hwnd, IDC_STATIC2), enable);
		EnableWindow(GetDlgItem(hwnd, IDC_STATIC12), enable);

	}
	else
	{
		EnableWindow(GetDlgItem(hwnd, IDC_CHECK4), enable);
		EnableWindow(GetDlgItem(hwnd, IDC_CHECK5), enable);		
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
	static bool bbutton;
	switch(msg)
	{
	case WM_INITDIALOG:
		{			
			READSETTINGS(hwnd);
			ENABLECONTROLS(hwnd, SendMessage(GetDlgItem(hwnd, IDC_CHECK7), (UINT) BM_GETCHECK, 0 , 0), true);
			ENABLECONTROLS(hwnd, SendMessage(GetDlgItem(hwnd, IDC_CHECK2), (UINT) BM_GETCHECK, 0, 0), false);
			SetStrings(hwnd);
			cfgopen = TRUE;
			bbutton = Settings.Thumbnailbuttons;
		}
		break;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
			case IDC_BUTTON1:
				cfgopen = FALSE;
				WRITESETTINGS(hwnd);
				if (bgloaded)
				{
					delete background;
					bgloaded = false;
				}

				if (!Settings.Progressbar)
					pTBL->SetProgressState(plugin.hwndParent, TBPF_NOPROGRESS);
	
				if (bbutton != Settings.Thumbnailbuttons )
					MessageBoxEx(plugin.hwndParent, STRINGS.find(__T("afterrestart"))->second.c_str(),
					STRINGS.find(__T("applysettings"))->second.c_str(), MB_ICONWARNING, 0);

				if (!Settings.Overlay)
					pTBL->SetOverlayIcon(plugin.hwndParent, NULL, NULL);
				else
					playstate = -1;


				SETWINDOWATTR();
				DwmInvalidateIconicBitmaps(plugin.hwndParent);
				DestroyWindow(hwnd);
				break;
			case IDC_BUTTON2:
				cfgopen = FALSE;
				DestroyWindow(hwnd);
				break;
			case IDC_BUTTON3:				
					SetWindowText(GetDlgItem(hwnd, IDC_EDIT2), GetFileName(hwnd).c_str());
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
					msgtext += __T("%r%  -  ") + STRINGS.find(__T("help4"))->second + __T("\n");
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
						
					MessageBoxEx(plugin.hwndParent, msgtext.c_str(), STRINGS.find(__T("info"))->second.c_str(), MB_ICONINFORMATION, 0);
				}
				break;
			case IDC_CHECK7:
				ENABLECONTROLS(hwnd, SendMessage(GetDlgItem(hwnd, IDC_CHECK7), (UINT) BM_GETCHECK, 0 , 0), true);
				break;
			case IDC_CHECK2:
				ENABLECONTROLS(hwnd, SendMessage(GetDlgItem(hwnd, IDC_CHECK2), (UINT) BM_GETCHECK, 0, 0), false);
				break;
			case IDC_CHECK20:
				if (SendMessage(GetDlgItem(hwnd, IDC_CHECK20), (UINT) BM_GETCHECK, 0, 0))
					SendMessage(GetDlgItem(hwnd, IDC_CHECK21), (UINT) BM_SETCHECK, 0, 0);				
				break;
			case IDC_CHECK21:
				if (SendMessage(GetDlgItem(hwnd, IDC_CHECK21), (UINT) BM_GETCHECK, 0, 0))
					SendMessage(GetDlgItem(hwnd, IDC_CHECK20), (UINT) BM_SETCHECK, 0, 0);	
				break;
		}
		break;

	case WM_NOTIFY:
		if (wParam == IDC_SYSLINK1)
			switch (((LPNMHDR)lParam)->code)
			{
			case NM_CLICK:
			case NM_RETURN:
				{
					ShellExecute(NULL, L"open", L"http://code.google.com/p/win7shell/", NULL, NULL, SW_SHOW);
				}
			}
		else
			switch (((LPNMHDR)lParam)->code)
			{
			case NM_CLICK:
			case NM_RETURN:
				{
					ShellExecute(NULL, L"open", L"http://code.google.com/p/win7shell/issues/entry", NULL, NULL, SW_SHOW);
				}
			}
		break;

	case WM_CLOSE:
		cfgopen = FALSE;
		DestroyWindow(hwnd);
		break;
	}
		
	return 0;
}

void FillStrings()
{
	STRINGS[__T("newvers")] = __T("New version available");
	STRINGS[__T("disableupdate")] = __T("You can disable checking for updates in the plugin configuration");
	STRINGS[__T("opendownload")] = __T("Do you want to open the download page?");
	STRINGS[__T("wrongwindows")] = __T("Wrong Windows™ version detected. This plugin is targeting Windows™ 7 only.");
	STRINGS[__T("installanyways")] = __T("Do you want to install it anyways?");
	STRINGS[__T("wrongostitle")] = __T("Wrong OS detected");
	STRINGS[__T("uninstall")] = __T("You won't see this question again, if you want to uninstall it, go to");
	STRINGS[__T("uninstallhowto")] = __T("'Winamp Preferences -> Plug-ins -> General Purpose -> Windows 7 Taskbar Integration' and uninstall.");
	STRINGS[__T("info")] = __T("Information");
	STRINGS[__T("config")] = __T("Configure Plugin");
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
	STRINGS[__T("help4")] = __T("The line that contains this is drawn with red shadow");
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
	getinistring(__T("newvers"), Path);
	getinistring(__T("disableupdate"), Path);
	getinistring(__T("opendownload"), Path);
	getinistring(__T("wrongwindows"), Path);
	getinistring(__T("installanyways"), Path);
	getinistring(__T("wrongostitle"), Path);
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
	getinistring(__T("help4"), Path);
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
	getinistring(__T("aerothumb"), Path);
	getinistring(__T("textonthumb"), Path);
	getinistring(__T("help"), Path);
	getinistring(__T("thumbbg"), Path);
	getinistring(__T("transparent"), Path);
	getinistring(__T("albumart"), Path);
	getinistring(__T("defbg"), Path);
	getinistring(__T("custombg"), Path);
	getinistring(__T("browse"), Path);
	getinistring(__T("fontsz"), Path);
	getinistring(__T("bold"), Path);
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
}


void SetStrings(HWND hwnd)
{
	if (STRINGS.find(__T("aerothumb")) != STRINGS.end() )
	{
		std::tstring tmp = __T("       ") + STRINGS.find(__T("aerothumb"))->second;
		SetWindowText(GetDlgItem(hwnd, IDC_STATIC4), tmp.c_str());
	}
	if (STRINGS.find(__T("textonthumb")) != STRINGS.end())
		SetWindowText(GetDlgItem(hwnd, IDC_STATIC1), STRINGS.find(__T("textonthumb"))->second.c_str());
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
	if (STRINGS.find(__T("fontsz"))!= STRINGS.end())
		SetWindowText(GetDlgItem(hwnd, IDC_STATIC12), STRINGS.find(__T("fontsz"))->second.c_str());
	if (STRINGS.find(__T("bold"))!= STRINGS.end())
		SetWindowText(GetDlgItem(hwnd, IDC_CHECK18), STRINGS.find(__T("bold"))->second.c_str());
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
	if (STRINGS.find(__T("noupdate"))!= STRINGS.end())
		SetWindowText(GetDlgItem(hwnd, IDC_CHECK9), STRINGS.find(__T("noupdate"))->second.c_str());
	if (STRINGS.find(__T("ok"))!= STRINGS.end())
		SetWindowText(GetDlgItem(hwnd, IDC_BUTTON1), STRINGS.find(__T("ok"))->second.c_str());
	if (STRINGS.find(__T("cancel"))!= STRINGS.end())
		SetWindowText(GetDlgItem(hwnd, IDC_BUTTON2), STRINGS.find(__T("cancel"))->second.c_str());


}

// This is an export function called by winamp which returns this plugin info.
// We wrap the code in 'extern "C"' to ensure the export isn't mangled if used in a CPP file.
extern "C" __declspec(dllexport) winampGeneralPurposePlugin * winampGetGeneralPurposePlugin() {
  return &plugin;
}
