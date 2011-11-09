#define WIN32_LEAN_AND_MEAN
#define _SECURE_SCL 0
#define _CRT_SECURE_NO_DEPRECATE 1
#define _CRT_NONSTDC_NO_DEPRECATE 1
#define TAGLIB_STATIC

#define ICONSIZEPX 50
#define INIFILE_NAME L"win7shell.ini"
#define PREF_PAGE "Taskbar Integration"
#define PREF_PAGEW L"Taskbar Integration"
#define NR_BUTTONS 15

#include <windows.h>
#include <windowsx.h>

#include <string>
#include <vector>
#include <list>
#include <sstream>
#include <fstream>
#include <map>
#include <algorithm>
#include <cctype> 

#include <commctrl.h>
#include <commdlg.h>
#include <dwmapi.h>
#include <shlobj.h>
#include <shellapi.h>
#include <strsafe.h>
#include <shobjidl.h>
#include <gdiplus.h>
#include <process.h>
#include <cctype>
#include <ctime>

#include "gen_win7shell.h"
#include "sdk/winamp/gen.h"
#include "sdk/winamp/wa_ipc.h"
#include "sdk/Agave/Language/lang.h"
#include "ipc_pe.h"
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

UINT WM_TASKBARBUTTONCREATED;
const std::wstring cur_version(__T("2.0"));
const std::string cur_versionA("2.0");
const std::wstring AppID(L"Winamp");

WNDPROC lpWndProcOld = 0;
bool thumbshowing = false, 
     uninstall = false;
HWND ratewnd = 0;
HIMAGELIST theicons = NULL;
HHOOK hMouseHook = NULL;
sSettings Settings = {};
std::vector<int> TButtons;
iTaskBar *taskbar = NULL;
MetaData metadata;
renderer* thumbnaildrawer = NULL;
void* prefDlgPtr = NULL;

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
inline VOID CALLBACK TimerProc(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime);
LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK cfgWndProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK rateWndProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK KeyboardEvent(int nCode, WPARAM wParam, LPARAM lParam);

LRESULT CALLBACK TabHandler1(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK TabHandler2(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK TabHandler3(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK TabHandler4(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK TabHandler5(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK TabHandler6(HWND, UINT, WPARAM, LPARAM);

// THREAD PROCS
void CheckVersion(void *dummy);

inline void updateToolbar(bool first);
inline void SetupJumpList();
inline void AddStringtoList(HWND window, int control_ID);

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
    if(tools::GetWinampVersion(plugin.hwndParent) < 0x5050)
    {
        MessageBoxA(plugin.hwndParent,"This plug-in requires Winamp v5.5 and up for it to work.\t\n"
                                      "Please upgrade your Winamp client to be able to use this.",
                                      plugin.description,MB_OK|MB_ICONINFORMATION);
        return GEN_INIT_FAILURE;
    }
    else
    {
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
            if (SetCurrentProcessExplicitAppUserModelID(AppID.c_str()) != S_OK)
                MessageBoxEx(plugin.hwndParent,
                             WASABI_API_LNGSTRINGW(IDS_ERROR_SETTING_APPID),
                             BuildPluginNameW(),
                             MB_ICONWARNING | MB_OK, 0);

            // Read Settings into struct
            std::wstring SettingsFile = tools::getWinampINIPath(plugin.hwndParent) + L"\\Plugins\\" + INIFILE_NAME;
            {
                SettingsManager SManager(SettingsFile);
                SManager.ReadSettings(Settings);
                SManager.ReadButtons(TButtons);
            }

            // Create jumplist
            SetupJumpList();

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

           // Create integrated preferences dialogs
           if(tools::GetWinampVersion(plugin.hwndParent) < 0x5053)
           {
               char* prefDlgNamePtr = new char[strlen(WASABI_API_LNGSTRING(IDS_PREF_PAGE_NAME))];
               strcpy((char*)prefDlgNamePtr, WASABI_API_LNGSTRING(IDS_PREF_PAGE_NAME));     

               prefsDlgRec* prefsRec = 0;
               prefsRec = new prefsDlgRec();
               prefsRec->hInst = plugin.hDllInstance;
               prefsRec->dlgID = IID_MAIN;
               prefsRec->name = (char*)prefDlgNamePtr;
               prefsRec->where = 0;
               prefsRec->proc = cfgWndProc;
               SendMessage(plugin.hwndParent,WM_WA_IPC,(WPARAM)prefsRec,IPC_ADD_PREFS_DLG);
               prefDlgPtr = prefsRec;
           }
           else
           {
               wchar_t* prefDlgNamePtr = new wchar_t[wcslen(WASABI_API_LNGSTRINGW(IDS_PREF_PAGE_NAME))];
               wcscpy((wchar_t*)prefDlgNamePtr, WASABI_API_LNGSTRINGW(IDS_PREF_PAGE_NAME));               

               prefsDlgRecW* prefsRec = 0;
               prefsRec = new prefsDlgRecW();
               prefsRec->hInst = plugin.hDllInstance;
               prefsRec->dlgID = IID_MAIN;
               prefsRec->name = (wchar_t*)prefDlgNamePtr;
               prefsRec->where = 0;
               prefsRec->proc = cfgWndProc;
               SendMessage(plugin.hwndParent,WM_WA_IPC,(WPARAM)prefsRec,IPC_ADD_PREFS_DLGW);
               prefDlgPtr = prefsRec;                 
           }

           // Timers, settings, icons
           if (Settings.VuMeter)
               SetTimer(plugin.hwndParent, 6668, 50, TimerProc);

           SetTimer(plugin.hwndParent, 6667, Settings.LowFrameRate ? 400 : 100, TimerProc);     
           SetTimer(plugin.hwndParent, 6672, 10000, TimerProc);

           Settings.play_volume = IPC_GETVOLUME(plugin.hwndParent);

           theicons = tools::prepareIcons(plugin.hDllInstance);   
           metadata.setWinampWindow(plugin.hwndParent);

           // update shuffle and repeat
           Settings.state_shuffle = SendMessage(plugin.hwndParent,WM_WA_IPC,0,IPC_GET_SHUFFLE);

           Settings.state_repeat = SendMessage(plugin.hwndParent,WM_WA_IPC,0,IPC_GET_REPEAT);
           if (Settings.state_repeat == 1 && SendMessage(plugin.hwndParent,WM_WA_IPC,0,IPC_GET_MANUALPLADVANCE) == 1)
           {
               Settings.state_repeat = 2;
           }

           // Create the taskbar interface
           taskbar = new iTaskBar();
           if (!taskbar->Reset(plugin.hwndParent))
           {
               MessageBoxEx(plugin.hwndParent,
                   WASABI_API_LNGSTRINGW(IDS_TASKBAR_INITIALIZE_FAILURE),
                   BuildPluginNameW(), MB_ICONSTOP, 0);
           }
           else if (Settings.Thumbnailbuttons)
           {
               taskbar->SetWindowAttr(Settings.Thumbnailenabled, Settings.VolumeControl, Settings.disallow_peek);
               if SUCCEEDED(taskbar->SetImageList(theicons))
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
            static AlbumArt albumart(WASABI_API_MEMMGR, AGAVE_API_ALBUMART, Settings);
            thumbnaildrawer = new renderer(Settings, metadata, albumart, plugin.hwndParent);            

            int index = SendMessage(plugin.hwndParent,WM_WA_IPC,0,IPC_GETLISTPOS);
            wchar_t *p = (wchar_t*)SendMessage(plugin.hwndParent,WM_WA_IPC,index,IPC_GETPLAYLISTFILEW); 
            WndProc(plugin.hwndParent, WM_WA_IPC, (WPARAM)p, IPC_PLAYING_FILEW);

            return GEN_INIT_SUCCESS;
        }
    }
    return GEN_INIT_FAILURE;
}

void config()
{
    SendMessage(plugin.hwndParent,WM_WA_IPC,(WPARAM)prefDlgPtr,IPC_OPENPREFSTOPAGE);
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
        SManager.WriteSettings(Settings);
        SManager.WriteResume(sResume);
        SManager.WriteButtons(TButtons);
    }

    delete taskbar;
    delete prefDlgPtr;
    delete thumbnaildrawer;
} 

void CheckVersion(void *dummy)
{
    std::wstring current_version = cur_version;
    if (current_version.length() < 4)
    {
        current_version += '0';
    }

    VersionChecker vc;
    std::wstring news = vc.IsNewVersion(cur_version);
    std::wstring vermsg = L"";
    wchar_t tmp[256], tmp2[256], tmp3[256];
    std::list<std::pair<std::wstring, std::wstring>> message_list;

    // Check for new version
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
    
    // Check for server messages
    int last_message_on_server = Settings.LastMessage;
    int first_message = vc.GetMessages(message_list, last_message_on_server);
    if (first_message != -1 && Settings.LastMessage < first_message)
    {
        for (std::list<std::pair<std::wstring, std::wstring>>::const_iterator iter = message_list.begin();
            iter != message_list.end();
            ++iter)
        {
            UINT flag = (*iter).second.empty() ? 0 : MB_OKCANCEL;

            int re = MessageBox(plugin.hwndParent, (*iter).first.c_str(), L"Taskbar Integration Message", 
                MB_TASKMODAL | MB_SETFOREGROUND | flag);
            if (flag !=0 && re == IDOK)
            {
                ShellExecute(NULL, L"open", (*iter).second.c_str(), NULL, NULL, SW_SHOW);
            }
        }

        Settings.LastMessage = last_message_on_server;
    }
}

void updateToolbar(bool first)
{
    if (!Settings.Thumbnailbuttons)
        return;

    std::vector<THUMBBUTTON> thbButtons;
    for (int i = 0; i!= TButtons.size(); ++i)
    {
        THUMBBUTTON button = {};
        button.dwMask = THB_BITMAP | THB_TOOLTIP;
        button.iId = TButtons[i];
        button.hIcon = NULL;        

        if (button.iId == TB_RATE || 
            button.iId == TB_STOPAFTER ||
            button.iId == TB_DELETE || 
            button.iId == TB_OPENEXPLORER || 
            button.iId == TB_JTFE)
        {
            button.dwMask = button.dwMask | THB_FLAGS;
            button.dwFlags = THBF_DISMISSONCLICK;
        }

        if (button.iId == TB_PLAYPAUSE)
        {				
            button.iBitmap = tools::getBitmap(button.iId, Settings.play_state == PLAYSTATE_PLAYING ? 1 : 0);
            wcsncpy(button.szTip, tools::getToolTip(TB_PLAYPAUSE, Settings.play_state).c_str(), 260);
        } 

        else if (button.iId == TB_REPEAT)
        {
            button.iBitmap = tools::getBitmap(button.iId, Settings.state_repeat);
            wcsncpy(button.szTip, tools::getToolTip(TB_REPEAT, Settings.state_repeat).c_str(), 260);
        } 

        else if (button.iId == TB_SHUFFLE)
        {
            button.iBitmap = tools::getBitmap(button.iId, Settings.play_state == Settings.state_shuffle);
            wcsncpy(button.szTip, tools::getToolTip(TB_SHUFFLE, Settings.state_shuffle).c_str(), 260);
        }

        else
        {
            button.iBitmap = tools::getBitmap(button.iId, 0);
            wcsncpy(button.szTip, tools::getToolTip(button.iId, 0).c_str(), 260);
        }

        thbButtons.push_back(button);
    }    

    if (!SUCCEEDED(taskbar->ThumbBarUpdateButtons(thbButtons, first)))
    {
        MessageBoxEx(plugin.hwndParent, WASABI_API_LNGSTRINGW(IDS_BUTTONS_ERROR), L"Taskbar Error",
            MB_ICONWARNING, 0);
    }
}



LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (message == WM_COMMAND && LOWORD(wParam) == 40023 ||
        message == WM_KEYDOWN && wParam == 's')
    {
        Settings.state_shuffle = SendMessage(plugin.hwndParent,WM_WA_IPC,0,IPC_GET_SHUFFLE);
        Settings.state_shuffle = !Settings.state_shuffle;
        updateToolbar(false);
    }

    switch (message)
    {

    case WM_DWMSENDICONICTHUMBNAIL:
        {
            // drawit            
            if (!thumbshowing)
            {
                thumbnaildrawer->SetDimensions(HIWORD(lParam), LOWORD(lParam));
                HBITMAP thumbnail = thumbnaildrawer->GetThumbnail();
                
                //HRESULT hr = DwmSetIconicThumbnail(plugin.hwndParent, thumbnail, false);

                thumbnaildrawer->ThumbnailPopup();
                SetTimer(plugin.hwndParent, 6670, Settings.LowFrameRate ? 41 : 21, TimerProc);                 
                thumbshowing = true;
                DeleteObject(thumbnail);
            }
            return 0;
        }
        break;

    case WM_COMMAND:
        if (HIWORD(wParam) == THBN_CLICKED)
            switch (LOWORD(wParam))
            {
            case TB_PREVIOUS:
                {
                    /*SetTimer(plugin.hwndParent, 6670, 40, TimerProc);*/

                    SendMessage(plugin.hwndParent, WM_COMMAND, MAKEWPARAM(40044,0), 0);
                    Settings.play_playlistpos = SendMessage(plugin.hwndParent,WM_WA_IPC,0,IPC_GETLISTPOS);

                    if (Settings.Thumbnailbackground == BG_ALBUMART)
                    {
                        thumbnaildrawer->ClearBackground();

                        if (Settings.play_state != PLAYSTATE_PLAYING)
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

                    thumbnaildrawer->ThumbnailPopup();

                    return 0;
                    break;
                }

            case TB_PLAYPAUSE:
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
                    Settings.play_state = (char)res;
                }
                return 0;
                break;

            case TB_STOP:
                SendMessage(plugin.hwndParent, WM_COMMAND, MAKEWPARAM(40047,0), 0);
                Settings.play_state = PLAYSTATE_NOTPLAYING; 
                return 0;
                break;

            case TB_NEXT:
                {				
                    SendMessage(plugin.hwndParent, WM_COMMAND, MAKEWPARAM(40048,0), 0);
                    Settings.play_playlistpos = SendMessage(plugin.hwndParent,WM_WA_IPC,0,IPC_GETLISTPOS);

                    if (Settings.Thumbnailbackground == BG_ALBUMART)
                    {
                        thumbnaildrawer->ClearBackground();

                        if (Settings.play_state != PLAYSTATE_PLAYING)
                        {
                            DwmInvalidateIconicBitmaps(plugin.hwndParent);
                        }                        
                    }

                    int index = SendMessage(plugin.hwndParent,WM_WA_IPC,0,IPC_GETLISTPOS);
                    wchar_t *p = (wchar_t*)SendMessage(plugin.hwndParent,WM_WA_IPC,index,IPC_GETPLAYLISTFILEW); 
                    if (p != NULL)
                        metadata.reset(p, false);

                    thumbnaildrawer->ThumbnailPopup();
                }
                return 0;
                break;

            case TB_RATE:
                {
                    ratewnd = WASABI_API_CREATEDIALOGW(IDD_RATEDLG,plugin.hwndParent,rateWndProc);

                    RECT rc;
                    POINT point;						
                    GetCursorPos(&point);
                    GetWindowRect(ratewnd, &rc);
                    MoveWindow(ratewnd, point.x - 155, point.y - 15, rc.right - rc.left, rc.bottom - rc.top, false);	
                    SetTimer(plugin.hwndParent, 6669, 5000, TimerProc);
                    ShowWindow(ratewnd, SW_SHOW);
                }
                return 0;
                break;

            case TB_VOLDOWN:
                Settings.play_volume -= 25;
                if (Settings.play_volume < 0)
                    Settings.play_volume = 0;
                SendMessage(plugin.hwndParent,WM_WA_IPC,Settings.play_volume,IPC_SETVOLUME);
                return 0;
                break;

            case TB_VOLUP:
                Settings.play_volume += 25;
                if (Settings.play_volume > 255)
                    Settings.play_volume = 255;
                SendMessage(plugin.hwndParent,WM_WA_IPC,Settings.play_volume,IPC_SETVOLUME);
                return 0;
                break;

            case TB_OPENFILE:
                SendMessage(plugin.hwndParent,WM_WA_IPC,(WPARAM)(HWND)0,IPC_OPENFILEBOX);
                return 0;
                break;

            case TB_MUTE:
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
                return 0;
                break;

            case TB_STOPAFTER:
                SendMessage(plugin.hwndParent, WM_COMMAND, MAKEWPARAM(40157,0), 0);
                return 0;
                break;

            case TB_REPEAT:
                // get
                Settings.state_repeat = SendMessage(plugin.hwndParent,WM_WA_IPC,0,IPC_GET_REPEAT);
                if (Settings.state_repeat == 1 && SendMessage(plugin.hwndParent,WM_WA_IPC,0,IPC_GET_MANUALPLADVANCE) == 1)
                {
                    Settings.state_repeat = 2;
                }

                ++Settings.state_repeat;

                if (Settings.state_repeat > 2)
                {
                    Settings.state_repeat = 0;
                }

                SendMessage(plugin.hwndParent, WM_WA_IPC, Settings.state_repeat >= 1 ? 1 : 0, IPC_SET_REPEAT);
                SendMessage(plugin.hwndParent, WM_WA_IPC, Settings.state_repeat == 2 ? 1 : 0, IPC_SET_MANUALPLADVANCE);            
                updateToolbar(false);
                return 0;
                break;

            case TB_SHUFFLE:
                Settings.state_shuffle = SendMessage(plugin.hwndParent,WM_WA_IPC,0,IPC_GET_SHUFFLE);
                Settings.state_shuffle = !Settings.state_shuffle;
                SendMessage(plugin.hwndParent, WM_WA_IPC, Settings.state_shuffle, IPC_SET_SHUFFLE);
                updateToolbar(false);
                return 0;
                break;

            case TB_JTFE:                
                ShowWindow(plugin.hwndParent, SW_SHOWNORMAL);
                SetForegroundWindow(plugin.hwndParent);                
                SendMessage(plugin.hwndParent, WM_COMMAND, 40194, 0);
                return 0;
                break;

            case TB_OPENEXPLORER:
                {
                    WIN32_FIND_DATAW data;
	                if (FindFirstFile(metadata.getFileName().c_str(), &data) != INVALID_HANDLE_VALUE)
	                {
                        ITEMIDLIST *pidl = ILCreateFromPath(metadata.getFileName().c_str());
                        if(pidl) 
                        {
                            SHOpenFolderAndSelectItems(pidl,0,0,0);
                            ILFree(pidl);
                        }
	                }     
                }
                return 0;
                break;

            case TB_DELETE:
                {
                    SHFILEOPSTRUCTW fileop = {};
                    std::wstring path(metadata.getFileName().c_str());
                    path += L'\0';
	                
	                fileop.hwnd = NULL;
	                fileop.wFunc = FO_DELETE;                    
	                fileop.pFrom = path.c_str();
	                fileop.pTo = L"";
	                fileop.fFlags = FOF_ALLOWUNDO | FOF_FILESONLY;
	                fileop.hNameMappings = NULL;
	                fileop.fAnyOperationsAborted = false;
	                fileop.lpszProgressTitle = L"";

                    int saved_play_state = Settings.play_state;
                    SendMessage(plugin.hwndParent, WM_COMMAND, MAKEWPARAM(40047,0), 0);
                    Settings.play_state = PLAYSTATE_NOTPLAYING; 
	
	                if (SHFileOperation(&fileop) == 0)
                    {
                        SendMessage((HWND)SendMessage(plugin.hwndParent, WM_WA_IPC, IPC_GETWND_PE, IPC_GETWND),
                            WM_WA_IPC, IPC_PE_DELETEINDEX, SendMessage(plugin.hwndParent,WM_WA_IPC,0,IPC_GETLISTPOS));
                    }

                    if (saved_play_state == PLAYSTATE_PLAYING)
                    {
                        SendMessage(plugin.hwndParent, WM_COMMAND, MAKEWPARAM(40045,0), 0);
                    }
                    
                    return 0;
                }
                break;
            }
        break;

        case WM_WA_IPC:
            switch (lParam)
            {
            case IPC_PLAYING_FILEW:
                {
                    if (wParam == 0)
                    {
                        return 0;
                    }

                    std::wstring filename(L"");
                    try
                    {
                        filename = (wchar_t*)wParam;
                    }
                    catch (...)
                    {
                        return 0;
                    }
                    
                    // TODO - not sure on this one
                    //int true_msg = true;
                    if (filename.empty())
                    {
                        int index = SendMessage(plugin.hwndParent,WM_WA_IPC,0,IPC_GETLISTPOS);
                        wchar_t *p = (wchar_t*)SendMessage(plugin.hwndParent,WM_WA_IPC,index,IPC_GETPLAYLISTFILEW); 
                        if (p != NULL)
                        {
                            filename = p;
                        }
                        //true_msg = false;
                    }

                    metadata.reset(filename, false);
                    if (metadata.CheckPlayCount())
                    {
                        JumpList JL(AppID);
                        JL.CleanJumpList();
                    }

                    Settings.play_playlistpos = SendMessage(plugin.hwndParent,WM_WA_IPC,0,IPC_GETLISTPOS);
                    Settings.play_total = SendMessage(plugin.hwndParent,WM_WA_IPC,2,IPC_GETOUTPUTTIME);
                    Settings.play_current = 0;

                    if (Settings.Thumbnailbackground == BG_ALBUMART)
                    {
                        thumbnaildrawer->ClearBackground();
                    }

                    DwmInvalidateIconicBitmaps(plugin.hwndParent);
                    thumbnaildrawer->ThumbnailPopup();

                    Settings.play_state = SendMessage(plugin.hwndParent,WM_WA_IPC,0,IPC_ISPLAYING);

                    if ((Settings.JLrecent || Settings.JLfrequent) && !tools::is_in_recent(filename))
                    {
                        std::wstring title(metadata.getMetadata(L"title") + L" - " + 
                            metadata.getMetadata(L"artist"));

                        if (Settings.play_total > 0)
                            title += L"  (" + tools::SecToTime(Settings.play_total/1000) + L")";

                        IShellLink * psl;
                        SHARDAPPIDINFOLINK applink;
                        if (/*true_msg  && */
                            tools::__CreateShellLink(filename.c_str(), title.c_str(), &psl) == S_OK &&
                            Settings.play_state == PLAYSTATE_PLAYING &&
                            Settings.Add2RecentDocs)
                        {
                            time_t rawtime;
                            time (&rawtime);
                            std::wstring timestr = _wctime(&rawtime);
                            psl->SetDescription(timestr.c_str());
                            applink.psl = psl;
                            applink.pszAppID = AppID.c_str();
                            SHAddToRecentDocs(SHARD_LINK, psl);
                            psl->Release();
                        }
                    }
               
                }
                break;

            case IPC_CB_MISC:
                switch (wParam)
                {
                    case IPC_CB_MISC_STATUS:
                        Settings.play_state = (char)SendMessage(plugin.hwndParent,WM_WA_IPC,0,IPC_ISPLAYING);
                    
                        updateToolbar(false);

                        if (Settings.Overlay)
                        {
                            wchar_t tmp[64] = {0};
                            switch (Settings.play_state)
                            {
                            case 1:
                                {
                                    HICON icon = ImageList_GetIcon(theicons, tools::getBitmap(TB_PLAYPAUSE, 0), 0);
                                    taskbar->SetIconOverlay(icon, WASABI_API_LNGSTRINGW_BUF(IDS_PLAYING,tmp,64));
                                    DestroyIcon(icon);
                                }
                                break;
                            case 3:
                                {
                                    HICON icon = ImageList_GetIcon(theicons, tools::getBitmap(TB_PLAYPAUSE, 1), 0);
                                    taskbar->SetIconOverlay(icon, WASABI_API_LNGSTRINGW_BUF(IDS_PAUSED,tmp,64));
                                    DestroyIcon(icon);
                                }
                                break;
                            default:
                                {
                                    HICON icon = ImageList_GetIcon(theicons, tools::getBitmap(TB_STOP, 1), 0);
                                    taskbar->SetIconOverlay(icon, WASABI_API_LNGSTRINGW_BUF(IDS_PAUSED,tmp,64));
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
                for(int i = IDC_PCB1; i <= IDC_PCB15; i++)
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
        if (taskbar->Reset(plugin.hwndParent))
        {
            if (Settings.Thumbnailbuttons)
                if SUCCEEDED(taskbar->SetImageList(theicons))
                    updateToolbar(true);
        }         
        else
        {
            MessageBoxEx(plugin.hwndParent,
                WASABI_API_LNGSTRINGW(IDS_TASKBAR_INITIALIZE_FAILURE),
                BuildPluginNameW(), MB_ICONSTOP, 0);
        }

        SetupJumpList();
        taskbar->SetWindowAttr(Settings.Thumbnailenabled, Settings.VolumeControl, Settings.disallow_peek);
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

                if (Settings.play_state != PLAYSTATE_PLAYING || audiodata == -1)
                {
                    taskbar->SetProgressState(TBPF_NOPROGRESS);
                }
                else
                {
                    if (audiodata <= 150)
                        taskbar->SetProgressState(TBPF_NORMAL);
                    else if (audiodata > 150 && audiodata < 210)
                        taskbar->SetProgressState(TBPF_PAUSED);
                    else
                        taskbar->SetProgressState(TBPF_ERROR);		

                    taskbar->SetProgressValue(audiodata, 255);			
                }
            }
            break;
        }

    case 6669: //rate wnd
        {
            if (ratewnd != NULL)
            {
                DestroyWindow(ratewnd);
            }

            KillTimer(plugin.hwndParent, 6669);
            break;
        }

    case 6670: //scroll redraw
        {
            HBITMAP thumbnail = thumbnaildrawer->GetThumbnail();

            HRESULT hr = DwmSetIconicThumbnail(plugin.hwndParent, thumbnail, 0);
            if (FAILED(hr))
            {
                KillTimer(plugin.hwndParent, 6670);
                MessageBoxEx(plugin.hwndParent,
                             WASABI_API_LNGSTRINGW(IDS_ERROR_SETTING_THUMBNAIL),
                             BuildPluginNameW(), MB_ICONERROR, 0);
                Settings.Thumbnailenabled = false;
                taskbar->SetWindowAttr(Settings.Thumbnailenabled, Settings.VolumeControl, Settings.disallow_peek);
            }

//             static HDC dest = GetDC(0);//FindWindow(L"calc", NULL));
//             HDC hdcMem = CreateCompatibleDC(dest);
//             SelectObject(hdcMem, thumbnail);
// 
//             Beep(3000, 30);
// 
//             if (BitBlt( 
//                 dest, //destination device context
//                 50, 50, //x,y location on destination
//                 200, 120, //width,height of source bitmap
//                 hdcMem, //source bitmap device context
//                 0, 0, //start x,y on source bitmap
//                 SRCCOPY) == 0) //blit method
//             {                
//                 std::wstringstream s;
//                 s << "\nBitBlt error: " << GetLastError() << "\n";
//                 OutputDebugString(s.str().c_str());
//                 Beep(1000, 100);
//             }

            DeleteObject(thumbnail);
            break;
        }

    case 6672: // check thumbshowing just in case - every 10 sec
        {
            // update repeat state
            int current_repeat_state = SendMessage(plugin.hwndParent,WM_WA_IPC,0,IPC_GET_REPEAT);
            if (current_repeat_state == 1 && SendMessage(plugin.hwndParent,WM_WA_IPC,0,IPC_GET_MANUALPLADVANCE) == 1)
            {
                current_repeat_state = 2;
            }
            if (current_repeat_state != Settings.state_repeat)
            {
                Settings.state_repeat = current_repeat_state;
                updateToolbar(false);
            }            

            CheckThumbShowing();
            break;
        }

    case 6667: // main timer
        {
            if (thumbshowing)
            {
                CheckThumbShowing();
            }
        
            if (Settings.play_state == -1)
            {
                WndProc(plugin.hwndParent, WM_WA_IPC, (WPARAM)L"", IPC_PLAYING_FILEW);
                WndProc(plugin.hwndParent, WM_WA_IPC, IPC_CB_MISC_STATUS, IPC_CB_MISC);
            }
        
            if (!(Settings.Progressbar || Settings.VuMeter))
                taskbar->SetProgressState(TBPF_NOPROGRESS);	
        
            if (Settings.play_state == PLAYSTATE_PLAYING || Settings.Thumbnailpb)
                if (Settings.play_total <= 0)
                    Settings.play_total = SendMessage(plugin.hwndParent,WM_WA_IPC,1,IPC_GETOUTPUTTIME) * 1000;
        
            int cp = SendMessage(plugin.hwndParent,WM_WA_IPC,0,IPC_GETOUTPUTTIME);
            if (Settings.play_current == cp)
            {
                return;
            }
            Settings.play_current = cp;
        
            switch (Settings.play_state)
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
                        {
                            count2++;
                        }
                    }
        
                        if (Settings.Progressbar)
                        {					
                            if (Settings.play_current == -1 || Settings.play_total <= 0)
                            {
                                if (Settings.Streamstatus)
                                {
                                    taskbar->SetProgressState(TBPF_INDETERMINATE);
                                }
                                else						
                                {
                                    taskbar->SetProgressState(TBPF_NOPROGRESS);
                                }
                            }
                            else
                            {
                                taskbar->SetProgressState(TBPF_NORMAL);
                                taskbar->SetProgressValue(Settings.play_current, Settings.play_total);
                            }
                        }
                    break;
        
                case 3: 
                        if (Settings.Progressbar)
                        {
                            taskbar->SetProgressState(TBPF_PAUSED);
                            taskbar->SetProgressValue(Settings.play_current, Settings.play_total);	
        
                            if (SendMessage(plugin.hwndParent,WM_WA_IPC,1,IPC_GETOUTPUTTIME) == -1)
                            {
                                taskbar->SetProgressValue(100, 100);
                            }
                        }
                    break;
        
                default:
        
                    if (Settings.Progressbar) 
                    {
                        if (Settings.Stoppedstatus)
                        {
                            taskbar->SetProgressState(TBPF_ERROR);
                            taskbar->SetProgressValue(100, 100);
                        }
                        else
                        {
                            taskbar->SetProgressState(TBPF_NOPROGRESS);
                        }
                    }
                    break;
            }
        }
        break;
    }
}

LRESULT CALLBACK rateWndProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
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

LRESULT CALLBACK cfgWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    static HWND TabCtrl;
    static HWND Tab1, Tab2, Tab3, Tab4, Tab5, Tab6;

    switch(msg)
    {
    case WM_INITDIALOG:
        {	
            // Gets rid of WM_GETDLGCODE endless message loop
            SetWindowLong(hwnd, GWL_EXSTYLE, WS_EX_CONTROLPARENT);

            // Create tabs
            Tab1 = WASABI_API_CREATEDIALOGW(IID_TAB1, hwnd, TabHandler1);            
            Tab2 = WASABI_API_CREATEDIALOGW(IID_TAB2, hwnd, TabHandler2);
            Tab3 = WASABI_API_CREATEDIALOGW(IID_TAB3, hwnd, TabHandler3);
            Tab4 = WASABI_API_CREATEDIALOGW(IID_TAB4, hwnd, TabHandler4);
            Tab5 = WASABI_API_CREATEDIALOGW(IID_TAB5, hwnd, TabHandler5);
            Tab6 = WASABI_API_CREATEDIALOGW(IID_TAB6, hwnd, TabHandler6);
 
            TabCtrl = GetDlgItem(hwnd, IDC_TAB);
 
            // Add tabs to window
            AddTab(TabCtrl, Tab1, WASABI_API_LNGSTRINGW(IDS_TEXT), -1, plugin.hwndParent);
            AddTab(TabCtrl, Tab2, WASABI_API_LNGSTRINGW(IDS_BACKGROUND), -1, plugin.hwndParent);
            AddTab(TabCtrl, Tab3, WASABI_API_LNGSTRINGW(IDS_OPTIONS), -1, plugin.hwndParent);
            AddTab(TabCtrl, Tab4, WASABI_API_LNGSTRINGW(IDS_TASKBAR_ICON), -1, plugin.hwndParent);
            AddTab(TabCtrl, Tab5, WASABI_API_LNGSTRINGW(IDS_JUMPLIST), -1, plugin.hwndParent);           
            AddTab(TabCtrl, Tab6, WASABI_API_LNGSTRINGW(IDS_DEVELOPMENT), -1, plugin.hwndParent);
            
            TabToFront(TabCtrl, Settings.LastTab);
        }

        return FALSE;
    break;

    case WM_NOTIFY:
        switch(((NMHDR *)lParam)->code)
        {
        case TCN_SELCHANGE: // Get currently selected tab window to front
                Settings.LastTab = SendMessage(TabCtrl, TCM_GETCURSEL, 0, 0);
                TabToFront(TabCtrl, -1);
                break;
        
        default:
                return false;
        }
        return 0;
    break;		

    case WM_CLOSE:
        TabCleanup(TabCtrl);        
        DestroyWindow(Tab1);
        DestroyWindow(Tab2);
        DestroyWindow(Tab3);
        DestroyWindow(Tab4);
        DestroyWindow(Tab5);
        DestroyWindow(Tab6);
        return 0;
        break;
    }
          
    return 0;
}

LRESULT CALLBACK TabHandler1(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
    switch(Message)
    {
    case WM_INITDIALOG:
        SettingsManager::WriteSettings_ToForm(hwnd, plugin.hwndParent, Settings);
        return 0;
        break;

    case WM_PAINT:
        {  
            std::vector<POINT> points;

            {
	            RECT rect;
	            POINT point;
	
	            // button1
	            GetWindowRect(GetDlgItem(hwnd, IDC_BUTTON9), &rect);    
	
	            // top right
	            point.x = rect.right;
	            point.y = rect.top;
	            points.push_back(point); //0
	
	            // bottom right
	            point.y = rect.bottom;
	            points.push_back(point); //1
	
	            // button2
	            GetWindowRect(GetDlgItem(hwnd, IDC_BUTTON6), &rect);
	
	            // top right
	            point.x = rect.right;
	            point.y = rect.top;
	            points.push_back(point); //2
	
	            // bottom right
	            point.y = rect.bottom;
	            points.push_back(point); //3
	
	            // editbox
	            GetWindowRect(GetDlgItem(hwnd, IDC_EDIT3), &rect);
	
	            // bottom right
	            point.x = rect.right;
	            point.y = rect.bottom;
	            points.push_back(point); //4
	
	            MapWindowPoints(NULL, hwnd, &points[0], points.size());
            }

            PAINTSTRUCT paint = {};
            HDC hdc = BeginPaint(hwnd, &paint);

            {
                HBRUSH brush;
                brush = CreateSolidBrush(Settings.text_color);
                HPEN pen;
                pen = CreatePen(PS_SOLID, 1, RGB(0, 0, 0));

                SelectObject(hdc, brush);
                SelectObject(hdc, pen);

                RoundRect(hdc, points[0].x + 5, points[0].y + 2, points[4].x - 1, points[1].y - 2, 2, 2);
                
                DeleteObject(brush);
                DeleteObject(pen);
            }

            {
                HBRUSH brush;
                brush = CreateSolidBrush(Settings.bgcolor);
                HPEN pen;
                pen = CreatePen(PS_SOLID, 1, RGB(0, 0, 0));

                SelectObject(hdc, brush);
                SelectObject(hdc, pen);

                RoundRect(hdc, points[2].x + 5, points[2].y + 2, points[4].x - 1, points[3].y - 2, 2, 2);

                DeleteObject(brush);
                DeleteObject(pen);
            }
            
            EndPaint(hwnd, &paint);
        }
        break;

    case WM_COMMAND:
        {
            switch(LOWORD(wParam))
            {
            case IDC_EDIT3:
                {
                    if (HIWORD(wParam) == EN_CHANGE)
                    {
                        int size = GetWindowTextLength(GetDlgItem(hwnd, IDC_EDIT3));
                        std::vector<wchar_t> buf(size+1);
                        GetWindowText(GetDlgItem(hwnd, IDC_EDIT3), &buf[0], size+1);	
                        Settings.Text = &buf[0];

                        std::wstring::size_type pos = std::wstring::npos;
                        do 
                        {
                            pos = Settings.Text.find(__T("\xD\xA"));
                            if (pos != std::wstring::npos)
                                Settings.Text.replace(pos, 2, __T("‡"));
                        }
                        while (pos != std::wstring::npos);

                        thumbnaildrawer->ThumbnailPopup();
                    }
                }
            return 0;
            break;

            case IDC_BUTTON_HELP:
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

                    MessageBoxEx((HWND)SendMessage(plugin.hwndParent,WM_WA_IPC,0,IPC_GETPREFSWND), msgtext.c_str(),
                                    WASABI_API_LNGSTRINGW(IDS_INFORMATION),
                                    MB_ICONINFORMATION | MB_SETFOREGROUND, 0);
                }
            return 0;
            break;

            case IDC_DEFAULT:
                {
                    std::wstring deftext = 
                        L"%c%%s%%curpl% of %totalpl%. ‡%c%%s%%title%‡%c%%s%%artist%‡‡%c%%s%%curtime%/%totaltime%‡%c%%s%Track #: %track%        Volume: %volume%%";
                    std::wstring::size_type pos = std::wstring::npos;
                    do 
                    {
                        pos = deftext.find(__T("‡"));
                        if (pos != std::wstring::npos)
                            deftext.replace(pos, 1, L"\xD\xA");
                    }
                    while (pos != std::wstring::npos);

                    SetWindowText(GetDlgItem(hwnd, IDC_EDIT3), deftext.c_str());                    
                    SendMessage(hwnd, WM_COMMAND, MAKEWPARAM(IDC_EDIT3, EN_CHANGE), NULL);
                }
            return 0;
            break;

            case IDC_BUTTON5:
                {
                    CHOOSEFONT cf = {};
                    cf.lStructSize = sizeof(cf);
                    cf.hwndOwner = (HWND)SendMessage(plugin.hwndParent,WM_WA_IPC,0,IPC_GETPREFSWND);					
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
            return 0;
            break;

            case IDC_BUTTON6:
                {
                    CHOOSECOLOR cc = {};             // common dialog box structure 
                    static COLORREF acrCustClr[16]; // array of custom colors 

                    srand(Settings.play_playlistpos ^ Settings.play_current ^ Settings.play_total);
                    for (int i = 0; i != 16; ++i)
                    {
                        acrCustClr[i] = RGB(rand() % 0xFF, rand() % 0xFF, rand() % 0xFF);
                    }                    

                    // Initialize CHOOSECOLOR 
                    cc.lStructSize = sizeof(cc);
                    cc.hwndOwner = (HWND)SendMessage(plugin.hwndParent,WM_WA_IPC,0,IPC_GETPREFSWND);
                    cc.lpCustColors = (LPDWORD) acrCustClr;
                    cc.rgbResult = Settings.bgcolor;
                    cc.Flags = CC_FULLOPEN | CC_RGBINIT;
                         
                    if (ChooseColor(&cc)==TRUE) 
                    {
                        Settings.bgcolor = cc.rgbResult;
                        InvalidateRect(hwnd, NULL, false);
                    }
                }
            return 0;
            break;

            case IDC_BUTTON9:
                {
                    CHOOSECOLOR cc = {};            // common dialog box structure 
                    static COLORREF acrCustClr[16]; // array of custom colors 

                    srand(Settings.play_playlistpos ^ Settings.play_current ^ Settings.play_total);
                    for (int i = 0; i != 16; ++i)
                    {
                        acrCustClr[i] = RGB(rand() % 0xFF, rand() % 0xFF, rand() % 0xFF);
                    } 

                    // Initialize CHOOSECOLOR 
                    cc.lStructSize = sizeof(cc);
                    cc.hwndOwner = (HWND)SendMessage(plugin.hwndParent,WM_WA_IPC,0,IPC_GETPREFSWND);
                    cc.lpCustColors = (LPDWORD) acrCustClr;
                    cc.rgbResult = Settings.text_color;
                    cc.Flags = CC_FULLOPEN | CC_RGBINIT;

                    if (ChooseColor(&cc)==TRUE) 
                    {
                        Settings.text_color = cc.rgbResult;
                        InvalidateRect(hwnd, NULL, false);
                    }
                }
            return 0;
            break;

            case IDC_CHECK8:
                Settings.Antialias = (Button_GetCheck(GetDlgItem(hwnd, IDC_CHECK8)) == BST_CHECKED);
            return 0;
            break;

            case IDC_CHECK1:
                Settings.Shrinkframe = (Button_GetCheck(GetDlgItem(hwnd, IDC_CHECK1)) == BST_CHECKED);
            return 0;
            break;

            case IDC_CHECK36:
                Settings.LowFrameRate = (Button_GetCheck(GetDlgItem(hwnd, IDC_CHECK36)) == BST_CHECKED);
                SetTimer(plugin.hwndParent, 6667, Settings.LowFrameRate ? 400 : 100, TimerProc);
            return 0;
            break;
            }
        }        
        break;
    }

    return CallWindowProc(cfgWndProc, GetParent(hwnd), Message, wParam, lParam);
}

LRESULT CALLBACK TabHandler2(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
    switch(Message)
    {
    case WM_INITDIALOG:
        SettingsManager::WriteSettings_ToForm(hwnd, plugin.hwndParent, Settings);
        SendMessage(GetDlgItem(hwnd, IDC_EDIT2), EM_SETREADONLY, TRUE, NULL);
        SendMessage(GetDlgItem(hwnd, IDC_SLIDER1), TBM_SETRANGE, FALSE, MAKELPARAM(30, 100));
        SendMessage(GetDlgItem(hwnd, IDC_SLIDER_TRANSPARENCY), TBM_SETRANGE, FALSE, MAKELPARAM(0, 100));

        switch (Settings.IconPosition)
        {
        case IP_UPPERLEFT:
            SetWindowText(GetDlgItem(hwnd, IDC_ICONPOS), WASABI_API_LNGSTRINGW(IDS_ICON_POSITION_TL));
            break;
        case IP_UPPERRIGHT:
            SetWindowText(GetDlgItem(hwnd, IDC_ICONPOS), WASABI_API_LNGSTRINGW(IDS_ICON_POSITION_TR));
            break;
        case IP_LOWERLEFT:
            SetWindowText(GetDlgItem(hwnd, IDC_ICONPOS), WASABI_API_LNGSTRINGW(IDS_ICON_POSITION_BL));
            break;
        case IP_LOWERRIGHT:
            SetWindowText(GetDlgItem(hwnd, IDC_ICONPOS), WASABI_API_LNGSTRINGW(IDS_ICON_POSITION_BR));
            break;
        }

        return 0;
        break;

    case WM_HSCROLL:
        {
            wstringstream size;
            int value = SendMessage(GetDlgItem(hwnd, IDC_SLIDER1), TBM_GETPOS, NULL, NULL);
            size << "Icon size (" << value << "%)";
            SetWindowTextW(GetDlgItem(hwnd, IDC_ICONSIZE), size.str().c_str());
            Settings.IconSize = value;

            size.str(L"");
            value = SendMessage(GetDlgItem(hwnd, IDC_SLIDER_TRANSPARENCY), TBM_GETPOS, NULL, NULL);
            size << value << "%";
            SetWindowTextW(GetDlgItem(hwnd, IDC_TRANSPARENCY_PERCENT), size.str().c_str());
            Settings.BG_Transparency = value;

            thumbnaildrawer->ClearCustomBackground();
            thumbnaildrawer->ClearBackground();
        }
        break;

    case WM_COMMAND:
        {
            switch(LOWORD(wParam))
            {
            case IDC_RADIO1:
            case IDC_RADIO2:
            case IDC_RADIO3:
                {
                    if ( SendMessage(GetDlgItem(hwnd, IDC_RADIO1), (UINT) BM_GETCHECK, 0 , 0) )
                        Settings.Thumbnailbackground = BG_TRANSPARENT;
                    else if ( SendMessage(GetDlgItem(hwnd, IDC_RADIO2), (UINT) BM_GETCHECK, 0 , 0) )
                        Settings.Thumbnailbackground = BG_ALBUMART;
                    else if ( SendMessage(GetDlgItem(hwnd, IDC_RADIO3), (UINT) BM_GETCHECK, 0 , 0) )
                    {
                        Settings.Thumbnailbackground = BG_CUSTOM;                    
                        if (GetWindowTextLength(GetDlgItem(hwnd, IDC_EDIT2)) == 0)
                        {
                            SendMessage(GetDlgItem(hwnd, IDC_BUTTON3), BM_CLICK, 0, 0);
                        }
                    }

                    thumbnaildrawer->ClearBackground();
                    thumbnaildrawer->ClearCustomBackground();
                    thumbnaildrawer->ThumbnailPopup();
                }
            return 0;
            break;

            case IDC_EDIT2:
                {
                    int size = GetWindowTextLength(GetDlgItem(hwnd, IDC_EDIT2));
                    std::vector<wchar_t> buf1(size+1);
                    GetWindowText(GetDlgItem(hwnd, IDC_EDIT2), &buf1[0], size+1);	
                    Settings.BGPath = &buf1[0];                    
                }
            return 0;
            break;

            case IDC_BUTTON3:
                {
                    std::wstring filename = tools::GetFileName((HWND)SendMessage(plugin.hwndParent,WM_WA_IPC,0,IPC_GETPREFSWND));
                    if (!filename.empty())
                    {
                        SetWindowText(GetDlgItem(hwnd, IDC_EDIT2), filename.c_str());
                        thumbnaildrawer->ClearBackground();
                        thumbnaildrawer->ClearCustomBackground();
                    }
                    else if (Settings.BGPath.empty())
                    {
                        SendMessage(GetDlgItem(hwnd, IDC_RADIO2), (UINT) BM_SETCHECK, BST_CHECKED , 0);
                        SendMessage(GetDlgItem(hwnd, IDC_RADIO3), (UINT) BM_SETCHECK, BST_UNCHECKED , 0);
                    }
                }

            return 0;
            break;

            case IDC_COMBO1:
                Settings.Revertto = (char)SendMessage(GetDlgItem(hwnd, IDC_COMBO1), CB_GETCURSEL, 0, 0);
                thumbnaildrawer->ClearBackground();
                thumbnaildrawer->ClearCustomBackground();
                thumbnaildrawer->ThumbnailPopup();
            return 0;
            break;

            case IDC_CHECK25:
                Settings.AsIcon = (Button_GetCheck(GetDlgItem(hwnd, IDC_CHECK25)) == BST_CHECKED);
                thumbnaildrawer->ClearBackground();
                thumbnaildrawer->ClearCustomBackground();
                thumbnaildrawer->ThumbnailPopup();
            return 0;
            break;

            case IDC_RADIO4:
            case IDC_RADIO6:
            case IDC_RADIO7:
            case IDC_RADIO8:
                {
                    if ( SendMessage(GetDlgItem(hwnd, IDC_RADIO4), (UINT) BM_GETCHECK, 0 , 0) )
                    {
                        SetWindowText(GetDlgItem(hwnd, IDC_ICONPOS), WASABI_API_LNGSTRINGW(IDS_ICON_POSITION_TL));
                        Settings.IconPosition = IP_UPPERLEFT;
                    }
                    else if ( SendMessage(GetDlgItem(hwnd, IDC_RADIO7), (UINT) BM_GETCHECK, 0 , 0) )
                    {
                        SetWindowText(GetDlgItem(hwnd, IDC_ICONPOS), WASABI_API_LNGSTRINGW(IDS_ICON_POSITION_BL));
                        Settings.IconPosition = IP_LOWERLEFT;
                    }
                    else if ( SendMessage(GetDlgItem(hwnd, IDC_RADIO6), (UINT) BM_GETCHECK, 0 , 0) )
                    {
                        SetWindowText(GetDlgItem(hwnd, IDC_ICONPOS), WASABI_API_LNGSTRINGW(IDS_ICON_POSITION_TR));
                        Settings.IconPosition = IP_UPPERRIGHT;
                    }
                    else if ( SendMessage(GetDlgItem(hwnd, IDC_RADIO8), (UINT) BM_GETCHECK, 0 , 0) )
                    {
                        SetWindowText(GetDlgItem(hwnd, IDC_ICONPOS), WASABI_API_LNGSTRINGW(IDS_ICON_POSITION_BR));
                        Settings.IconPosition = IP_LOWERRIGHT;
                    }                    

                    thumbnaildrawer->ClearBackground();
                    thumbnaildrawer->ClearCustomBackground();
                    thumbnaildrawer->ThumbnailPopup();
                }
            return 0;
            break;

            } // switch
        }
        break;
    }

    return CallWindowProc(cfgWndProc, GetParent(hwnd), Message, wParam, lParam);
}

LRESULT CALLBACK TabHandler3(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
    switch(Message)
    {
    case WM_INITDIALOG:
        {
            // Reset buttons
            for(int i = IDC_PCB1; i <= IDC_PCB15; i++)
            {
                SendMessage(GetDlgItem(hwnd, i), BM_SETCHECK, BST_UNCHECKED, NULL);
            }

	        SettingsManager::WriteSettings_ToForm(hwnd, plugin.hwndParent, Settings);      

            HWND list = GetDlgItem(hwnd, IDC_LIST1);
            for (int i = 0; i < TButtons.size(); ++i)
            {
                int index = SendMessage(list, LB_ADDSTRING, NULL, (LPARAM)tools::getToolTip(TButtons[i], -1).c_str());
                SendMessage(list, LB_SETITEMDATA, index, TButtons[i]);
                SendMessage(GetDlgItem(hwnd, TButtons[i]), BM_SETCHECK, BST_CHECKED, NULL);
            }
	        
	        // Set button icons
	        for (int i = 0; i < NR_BUTTONS; i++)
	        {
                HICON icon = ImageList_GetIcon(theicons, tools::getBitmap(TB_PREVIOUS+i, i == 10 ? 1 : 0), 0);
                SendMessage(GetDlgItem(hwnd, IDC_PCB1+i), BM_SETIMAGE, IMAGE_ICON, (LPARAM)icon);           
	            DestroyIcon(icon);
	        }
	
	        HICON up_icon = (HICON)LoadImage(plugin.hDllInstance, MAKEINTRESOURCE(IDI_ICON7), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
	        HICON down_icon = (HICON)LoadImage(plugin.hDllInstance, MAKEINTRESOURCE(IDI_ICON8), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
	
	        if (up_icon != NULL && down_icon != NULL)
	        {
	            SendMessage(GetDlgItem(hwnd, IDC_UPBUTT), BM_SETIMAGE, IMAGE_ICON, (LPARAM)up_icon);
	            SendMessage(GetDlgItem(hwnd, IDC_DOWNBUTT), BM_SETIMAGE, IMAGE_ICON, (LPARAM)down_icon);
	        }
	
	        DestroyIcon(up_icon);
	        DestroyIcon(down_icon);
        }
    return 0;
    break;

    case WM_MOUSEMOVE:
        SetWindowText(GetDlgItem(hwnd, IDC_STATIC29), WASABI_API_LNGSTRINGW(IDS_MAX_BUTTONS));
        break;

    case WM_NOTIFY:
            switch (wParam)
            {   
            case IDC_PCB1:
            case IDC_PCB2:
            case IDC_PCB3:
            case IDC_PCB4:
            case IDC_PCB5:
            case IDC_PCB6:
            case IDC_PCB7:
            case IDC_PCB8:
            case IDC_PCB9:
            case IDC_PCB10:
            case IDC_PCB11:
            case IDC_PCB12:
            case IDC_PCB13:
            case IDC_PCB14:
            case IDC_PCB15:
                if ((((LPNMHDR)lParam)->code) == BCN_HOTITEMCHANGE)
                    SetWindowText(GetDlgItem(hwnd, IDC_STATIC29), tools::getToolTip(wParam).c_str());
                break;
            }

    break;    

    case WM_COMMAND:
        {
            switch(LOWORD(wParam))
            {
            case IDC_BUTTON_RESTART:
                    PostMessage(plugin.hwndParent,WM_WA_IPC,0,IPC_RESTARTWINAMP);
                break;

            case IDC_UPBUTT:
                {
                    HWND list = GetDlgItem(hwnd, IDC_LIST1);
                    int index = SendMessage(list, LB_GETCURSEL, NULL, NULL);
                    if (index == 0 || index == -1)
                    {
                        return 0;
                    }

                    int data = SendMessage(list, LB_GETITEMDATA, index, NULL);
                    SendMessage(list, LB_DELETESTRING, index, NULL);
                    index = SendMessage(list, LB_INSERTSTRING, index-1, (LPARAM)tools::getToolTip(data).c_str());
                    SendMessage(list, LB_SETITEMDATA, index, data);
                    SendMessage(list, LB_SETCURSEL, index, NULL);

                    // Populate buttons structure
                    TButtons.clear();
                    for (int i = 0; i != ListBox_GetCount(list); ++i)
                    {
                        TButtons.push_back(ListBox_GetItemData(list, i));
                    }

                    // Show note
                    ShowWindow(GetDlgItem(hwnd, IDC_NOTEGROUP), SW_SHOW);
                    ShowWindow(GetDlgItem(hwnd, IDC_STATIC_RESTART), SW_SHOW);
                    ShowWindow(GetDlgItem(hwnd, IDC_BUTTON_RESTART), SW_SHOW);
                }
                break;

            case IDC_DOWNBUTT:
                {                    
                    HWND list = GetDlgItem(hwnd, IDC_LIST1);
                    int index = SendMessage(list, LB_GETCURSEL, NULL, NULL);
                    if (index == ListBox_GetCount(list)-1 || index == -1)
                    {
                        return 0;
                    }

                    int data = SendMessage(list, LB_GETITEMDATA, index, NULL);
                    SendMessage(list, LB_DELETESTRING, index, NULL);
                    index = SendMessage(list, LB_INSERTSTRING, index+1, (LPARAM)tools::getToolTip(data).c_str());
                    SendMessage(list, LB_SETITEMDATA, index, data);
                    SendMessage(list, LB_SETCURSEL, index, NULL);

                    // Populate buttons structure
                    TButtons.clear();
                    for (int i = 0; i != ListBox_GetCount(list); ++i)
                    {
                        TButtons.push_back(ListBox_GetItemData(list, i));
                    }

                    // Show note
                    ShowWindow(GetDlgItem(hwnd, IDC_NOTEGROUP), SW_SHOW);
                    ShowWindow(GetDlgItem(hwnd, IDC_STATIC_RESTART), SW_SHOW);
                    ShowWindow(GetDlgItem(hwnd, IDC_BUTTON_RESTART), SW_SHOW);
                }
                break;

            case IDC_LIST1:
                {
                    int index = SendMessage(GetDlgItem(hwnd, IDC_LIST1), LB_GETCURSEL, NULL, NULL);
                    int data = SendMessage(GetDlgItem(hwnd, IDC_LIST1), LB_GETITEMDATA, index, NULL);
                    std::wstringstream w;
                    w << data;
                    OutputDebugString(w.str().c_str());
                }
                break;

            case IDC_CHECK6:
                Settings.Thumbnailbuttons = (Button_GetCheck(GetDlgItem(hwnd, IDC_CHECK6)) == BST_CHECKED);
                EnableWindow(GetDlgItem(hwnd, IDC_CHECK27), SendMessage(GetDlgItem(hwnd, IDC_CHECK6), (UINT) BM_GETCHECK, 0, 0));
            return 0;
            break;

            case IDC_CHECK7:
                Settings.Thumbnailenabled = (Button_GetCheck(GetDlgItem(hwnd, IDC_CHECK7)) == BST_CHECKED);
                taskbar->SetWindowAttr(Settings.Thumbnailenabled, Settings.VolumeControl, Settings.disallow_peek);
            return 0;
            break;

            case IDC_CHECK10:
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
            return 0;
            break;

            case IDC_CHECK29:
                Settings.Thumbnailpb = (Button_GetCheck(GetDlgItem(hwnd, IDC_CHECK29)) == BST_CHECKED);
            return 0;
            break;        

            case IDC_CHECK_AEROPEEK:
                Settings.disallow_peek = (Button_GetCheck(GetDlgItem(hwnd, IDC_CHECK_AEROPEEK)) == BST_CHECKED);
                taskbar->SetWindowAttr(true, Settings.VolumeControl, Settings.disallow_peek);
            return 0;
            break;

            case IDC_PCB1:
            case IDC_PCB2:
            case IDC_PCB3:
            case IDC_PCB4:
            case IDC_PCB5:
            case IDC_PCB6:
            case IDC_PCB7:
            case IDC_PCB8:
            case IDC_PCB9:
            case IDC_PCB10:
            case IDC_PCB11:
            case IDC_PCB12:
            case IDC_PCB13:
            case IDC_PCB14:
            case IDC_PCB15:
                {
                    AddStringtoList(hwnd, LOWORD(wParam));
                }

            return 0;
            break;
                    
            }
        }
        break;
    }

    return CallWindowProc(cfgWndProc, GetParent(hwnd), Message, wParam, lParam);
}

LRESULT CALLBACK TabHandler4(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
    switch(Message)
    {
    case WM_INITDIALOG:
        SettingsManager::WriteSettings_ToForm(hwnd, plugin.hwndParent, Settings);
        return 0;
        break;

    case WM_COMMAND:
        {
            switch(LOWORD(wParam))
            {
            case IDC_CHECK3:
                Settings.Overlay = (Button_GetCheck(GetDlgItem(hwnd, IDC_CHECK3)) == BST_CHECKED);
                if (Settings.Overlay)
                    WndProc(plugin.hwndParent, WM_WA_IPC, IPC_CB_MISC_STATUS, IPC_CB_MISC);
                else
                    taskbar->SetIconOverlay(NULL, L"");
            return 0;
            break;
            
            case IDC_CHECK2:
                {                
                    Settings.Progressbar = (Button_GetCheck(GetDlgItem(hwnd, IDC_CHECK2)) == BST_CHECKED);
                    
                    EnableWindow(GetDlgItem(hwnd, IDC_CHECK4), Settings.Progressbar);
                    EnableWindow(GetDlgItem(hwnd, IDC_CHECK5), Settings.Progressbar);

                    if (Settings.Progressbar)
                    {
                        SendMessage(GetDlgItem(hwnd, IDC_CHECK26), (UINT) BM_SETCHECK, 0, 0);	
                        Settings.VuMeter = false;
                    }
                }         
            return 0;
            break;
                
            case IDC_CHECK4:
                Settings.Streamstatus = (Button_GetCheck(GetDlgItem(hwnd, IDC_CHECK4)) == BST_CHECKED);
            return 0;
            break;
                  
            case IDC_CHECK5:
                Settings.Stoppedstatus = (Button_GetCheck(GetDlgItem(hwnd, IDC_CHECK5)) == BST_CHECKED);
            return 0;
            break;

            case IDC_CHECK26:
                Settings.VuMeter = (Button_GetCheck(GetDlgItem(hwnd, IDC_CHECK26)) == BST_CHECKED);

                if (Settings.VuMeter)
                {
                    SendMessage(GetDlgItem(hwnd, IDC_CHECK2), (UINT) BM_SETCHECK, 0, 0);	
                    EnableWindow(GetDlgItem(hwnd, IDC_CHECK4), 0);
                    EnableWindow(GetDlgItem(hwnd, IDC_CHECK5), 0);
                    Settings.Progressbar = false;
                }

                if (Settings.VuMeter)
                {
                    SetTimer(plugin.hwndParent, 6668, 50, TimerProc);
                }
                else
                {
                    KillTimer(plugin.hwndParent, 6668);
                }
            return 0;
            break;
            }

            case IDC_CHECK35:
                Settings.VolumeControl = (Button_GetCheck(GetDlgItem(hwnd, IDC_CHECK35)) == BST_CHECKED);

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
                return 0;
                break;
        }
        break;
    }

    return CallWindowProc(cfgWndProc, GetParent(hwnd), Message, wParam, lParam);
}

LRESULT CALLBACK TabHandler5(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
    switch(Message)
    {
    case WM_INITDIALOG:
        SettingsManager::WriteSettings_ToForm(hwnd, plugin.hwndParent, Settings);
        return 0;
        break;

    case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
            case IDC_CLEARALL:
                {
	                wchar_t path[MAX_PATH];
	                SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, SHGFP_TYPE_CURRENT, path);
	                std::wstring filepath(path);
	                filepath += L"\\Microsoft\\Windows\\Recent\\AutomaticDestinations\\879d567ffa1f5b9f.automaticDestinations-ms";
	                if (DeleteFile(filepath.c_str()) != 0)
	                {
	                    EnableWindow(GetDlgItem(hwnd, IDC_CLEARALL), false);
	                }
                }
                return 0;
                break;

            case IDC_CHECK_A2R:
                Settings.Add2RecentDocs = !(Button_GetCheck(GetDlgItem(hwnd, IDC_CHECK_A2R)) == BST_CHECKED);
                return 0;
                break;

            case IDC_CHECK30:
            case IDC_CHECK31:
            case IDC_CHECK32:
            case IDC_CHECK33:
            case IDC_CHECK34:
                Settings.JLpl = (Button_GetCheck(GetDlgItem(hwnd, IDC_CHECK34)) == BST_CHECKED);
                Settings.JLbms = (Button_GetCheck(GetDlgItem(hwnd, IDC_CHECK33)) == BST_CHECKED);
                Settings.JLtasks = (Button_GetCheck(GetDlgItem(hwnd, IDC_CHECK32)) == BST_CHECKED);
                Settings.JLfrequent = (Button_GetCheck(GetDlgItem(hwnd, IDC_CHECK31)) == BST_CHECKED);
                Settings.JLrecent = (Button_GetCheck(GetDlgItem(hwnd, IDC_CHECK30)) == BST_CHECKED);

                if ( Settings.JLbms      || 
                     Settings.JLpl       ||
                     Settings.JLtasks    ||
                     Settings.JLfrequent ||
                     Settings.JLrecent )
                {
                    SetupJumpList();
                }
                else
                {
                    Settings.JLrecent = true;
                    Button_SetCheck(GetDlgItem(hwnd, IDC_CHECK30), BST_CHECKED);
                    SetupJumpList();
                }
            return 0;
            break;
            }
        }        
        break;
    }

    return CallWindowProc(cfgWndProc, GetParent(hwnd), Message, wParam, lParam);
}

LRESULT CALLBACK TabHandler6(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
    switch(Message)
    {
    case WM_INITDIALOG:
        {
            SettingsManager::WriteSettings_ToForm(hwnd, plugin.hwndParent, Settings);
            std::wstring version = BuildPluginNameW();
            SetWindowText(GetDlgItem(hwnd, IDC_ABOUTGROUP), version.c_str());
        }
        
        return 0;
        break;

    case WM_COMMAND:
        {
            switch(LOWORD(wParam))
            {
            case IDC_CHECK9:
                Settings.DisableUpdates = (Button_GetCheck(GetDlgItem(hwnd, IDC_CHECK9)) == BST_CHECKED);
            return 0;
            break;
            }
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

            case IDC_SYSLINK6:
                switch (((LPNMHDR)lParam)->code)
                {
                case NM_CLICK:
                case NM_RETURN:
                    {
                        ShellExecute(NULL, L"open", L"http://code.google.com/p/win7shell/wiki/Donate", NULL, NULL, SW_SHOW);
                    }
                }
                break;
            }
            break;
    }

    return CallWindowProc(cfgWndProc, GetParent(hwnd), Message, wParam, lParam);
}

void AddStringtoList(HWND window, int control_ID)
{
    HWND button = GetDlgItem(window, control_ID);
    HWND list = GetDlgItem(window, IDC_LIST1);
    bool checked = !Button_GetCheck(button);

    // Populate list
    if (checked)
    {
        if (ListBox_GetCount(list) > 0)
        {
            int index = SendMessage(list, LB_FINDSTRINGEXACT, -1, (LPARAM)tools::getToolTip(control_ID).c_str());
            if (index != LB_ERR)
            {
                ListBox_DeleteString(list, index);
            }
        }
    }
    else
    {
        if (ListBox_GetCount(list) == 0)
        {
            int index = SendMessage(list, LB_ADDSTRING, NULL, (LPARAM)tools::getToolTip(control_ID).c_str());
            SendMessage(list, LB_SETITEMDATA, index, control_ID);
        }
        else
        {
            if (ListBox_GetCount(list) >= 7)
            {
                Button_SetCheck(button, BST_UNCHECKED);
                std::wstring max7 = WASABI_API_LNGSTRINGW(IDS_MAX_BUTTONS);
                std::transform(max7.begin(), max7.end(), max7.begin(), (int(*)(int)) std::toupper);
                SetWindowText(GetDlgItem(window, IDC_STATIC29), max7.c_str());
            }
            else
            if (SendMessage(list, LB_FINDSTRINGEXACT, -1, (LPARAM)tools::getToolTip(control_ID).c_str()) == LB_ERR) //no duplicate
            {
                int index = SendMessage(list, LB_ADDSTRING, NULL, (LPARAM)tools::getToolTip(control_ID).c_str());
                ListBox_SetItemData(list, index, control_ID);
            }
        }
    }

    // Populate buttons structure
    TButtons.clear();
    for (int i = 0; i != ListBox_GetCount(list); ++i)
    {
        TButtons.push_back(ListBox_GetItemData(list, i));
    }

    // Show note
    ShowWindow(GetDlgItem(window, IDC_NOTEGROUP), SW_SHOW);
    ShowWindow(GetDlgItem(window, IDC_STATIC_RESTART), SW_SHOW);
    ShowWindow(GetDlgItem(window, IDC_BUTTON_RESTART), SW_SHOW);
}

void SetupJumpList()
{
    JumpList jl(AppID);

    jl.CleanJumpList();

    if (jl.DeleteJumpList())
    {
        std::wstring bms = tools::getBMS(plugin.hwndParent);

        if (Settings.JLbms      || 
            Settings.JLfrequent ||
            Settings.JLpl       ||
            Settings.JLrecent   || 
            Settings.JLtasks)
        {            
            static wchar_t tmp1[128], tmp2[128], tmp3[128], tmp4[128], tmp5[128], tmp6[128];
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
        if(MessageBoxEx(hwndDlg,WASABI_API_LNGSTRINGW(IDS_UNINSTALL_PROMPT),
                      BuildPluginNameW(), MB_YESNO, 0)==IDYES)
        {
            std::wstring ini_path = tools::getWinampINIPath(plugin.hwndParent) + L"\\Plugins\\" + INIFILE_NAME;
            DeleteFile(ini_path.c_str());
            JumpList jl(AppID);
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
    if (!IsWindow(hwnd))
        MessageBoxEx(0, L"Please start Winamp first", BuildPluginNameW(), MB_ICONWARNING, 0);
    else
    {
        if (SendMessage(hwnd,WM_WA_IPC,0,IPC_ISPLAYING) == 1)
            return 0;
        
        std::wstring path;
        path = tools::getWinampINIPath(hwnd) + L"\\Plugins\\" + INIFILE_NAME;

        if (path.empty())
            return -1;

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