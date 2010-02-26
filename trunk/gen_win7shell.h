// gen_win7shell.h
#ifndef gen_myplugin_h
//---------------------------------------------------------------------------
#define gen_myplugin_h
#include <windows.h>
#include <string>
 
 
// plugin version (don't touch this)
#define GPPHDR_VER 0x10
 
// plugin name/title (change this to something you like)
#define PLUGIN_NAME "Windows 7 Taskbar Integration v1.13"

#ifdef _UNICODE
#define tstring wstring
#else
#define tstring string
#endif

#ifdef  _UNICODE
typedef wchar_t TCHAR;
#define __T(x) L ## x
#else
typedef char TCHAR;
#define __T(x) x
#endif

#define NEWLINE __T('‡')

struct sSettings_strings {
	std::tstring BGPath;
	std::tstring Text;
};
 
struct sSettings_old {
	bool Thumbnailenabled;
	char Thumbnailbackground;
	bool Thumbnailbuttons;
	bool Progressbar;
	bool Streamstatus;
	bool Stoppedstatus;
	bool Overlay;
	bool Add2RecentDocs;
	bool Antialias;
	bool Shrinkframe;
	bool ForceVersion;
	bool DisableUpdates;
	bool RemoveTitle;
	bool AsIcon;
	bool VuMeter;
	bool Buttons[16];
	bool Thumbnailpb;
	bool JLrecent;
	bool JLfrequent;
	bool JLtasks;
	bool JLbms;
	char Revertto;
};

struct sSettings_old2 {
	bool Thumbnailenabled;
	char Thumbnailbackground;
	bool Thumbnailbuttons;
	bool Progressbar;
	bool Streamstatus;
	bool Stoppedstatus;
	bool Overlay;
	bool Add2RecentDocs;
	bool Antialias;
	bool Shrinkframe;
	bool ForceVersion;
	bool DisableUpdates;
	bool RemoveTitle;
	bool AsIcon;
	bool VuMeter;
	bool Buttons[16];
	bool Thumbnailpb;
	bool JLrecent;
	bool JLfrequent;
	bool JLtasks;
	bool JLbms;
	bool JLpl;
	char Revertto;
};

struct sSettings {
	bool Thumbnailenabled;
	char Thumbnailbackground;
	bool Thumbnailbuttons;
	bool Progressbar;
	bool Streamstatus;
	bool Stoppedstatus;
	bool Overlay;
	bool Add2RecentDocs;
	bool Antialias;
	bool Shrinkframe;
	bool ForceVersion;
	bool DisableUpdates;
	bool RemoveTitle;
	bool AsIcon;
	bool VuMeter;
	bool Buttons[16];
	bool Thumbnailpb;
	bool JLrecent;
	bool JLfrequent;
	bool JLtasks;
	bool JLbms;
	bool JLpl;
	bool VolumeControl;
	char Revertto;
};

struct sResumeSettings {
	int ResumeTime;
	int ResumePosition;
};

struct sFontEx {
	LOGFONT font;
	DWORD color;
	DWORD bgcolor;
};

#endif
