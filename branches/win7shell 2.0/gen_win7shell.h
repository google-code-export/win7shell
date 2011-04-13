// gen_win7shell.h
#ifndef gen_myplugin_h
//---------------------------------------------------------------------------
#define gen_myplugin_h
#include <windows.h>
#include <string>
 
// plugin name/title
#define PLUGIN_NAME "Windows 7 Taskbar Integration v1.14 Test Build 2"

#ifdef  _UNICODE
typedef wchar_t TCHAR;
#define __T(x) L ## x
#else
typedef char TCHAR;
#define __T(x) x
#endif

#define NEWLINE __T('‡')

struct sResumeSettings 
{
    int ResumeTime;
    int ResumePosition;
};

struct sSettings 
{
    // thumbnail
	bool Thumbnailenabled;
	int Thumbnailbackground;
	bool Thumbnailbuttons;
	bool Progressbar;
	bool Streamstatus;
	bool Stoppedstatus;
	bool Overlay;
    int Revertto;
    std::wstring BGPath;
    std::wstring Text;
    
	bool Add2RecentDocs;
	bool Antialias;
	bool Shrinkframe;
	bool ForceVersion;
	bool DisableUpdates;
	bool RemoveTitle;
	bool VuMeter;
	bool Buttons[16];
	bool Thumbnailpb;

    // icon settings
	bool AsIcon;
    int IconSize;
    int IconPosition;

    // jumplist
	bool JLrecent;
	bool JLfrequent;
	bool JLtasks;
	bool JLbms;
	bool JLpl;

    // misc
    bool VolumeControl;
    bool LowFrameRate;
    int LastTab;
    int LastUpdateCheck;

    // playback info
    int play_current;
    int play_total;
    int play_playlistpos;
    int play_volume;


    // font settings
    LOGFONT font;
    DWORD text_color;
    DWORD bgcolor;
};

struct linesettings
{
    int height;
    bool center;
    bool largefont;
    bool forceleft;
    bool shadow;
    bool darkbox;
    bool dontscroll;
    bool iscalculated;
};

enum 
{ 
    BG_TRANSPARENT, 
    BG_ALBUMART, 
    BG_CUSTOM
};

enum
{
    IP_UPPERLEFT,
    IP_LOWERLEFT,
    IP_UPPERRIGHT,
    IP_LOWERRIGHT
};

#define SECTION_NAME_GENERAL L"general"
#define SECTION_NAME_FONT L"font"
#define SECTION_NAME_RESUME L"resume"

#define PLAYSTATE_PLAYING 1
#define PLAYSTATE_PAUSED 3
#define PLAYSTATE_NOTPLAYING 0

#endif
