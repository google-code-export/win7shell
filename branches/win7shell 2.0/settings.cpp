#include <Windows.h>
#include <string>
#include <vector>
#include <sstream>
#include <windowsx.h>
#include <Commctrl.h>
#include "settings.h"
#include "gen_win7shell.h"
#include "api.h"
#include "resource.h"

using namespace std;

int SettingsManager::GetInt(wstring key, int default_value)
{
    return GetPrivateProfileIntW(currentSection.c_str(), key.c_str(), default_value, mPath.c_str());
}

bool SettingsManager::GetBool(wstring key, bool default_value)
{
    return (bool)GetPrivateProfileIntW(currentSection.c_str(), key.c_str(), default_value, mPath.c_str());
}

wstring SettingsManager::GetString(wstring key, wstring default_value, size_t max_size)
{
    wstring buffer;
    buffer.resize(max_size);
    GetPrivateProfileStringW(currentSection.c_str(), key.c_str(), default_value.c_str(),
        &buffer[0], max_size, mPath.c_str());
    buffer.resize(wcslen(buffer.c_str()));
    return wstring(buffer);
}

bool SettingsManager::WriteInt(wstring key, int value)
{
    wstringstream s;
    s << value;
    return (bool)WritePrivateProfileStringW(currentSection.c_str(), key.c_str(), s.str().c_str(), mPath.c_str());
}

bool SettingsManager::WriteBool(wstring key, bool value)
{
    wstring s;
    s = (value ? L"1" : L"0");
    return (bool)WritePrivateProfileStringW(currentSection.c_str(), key.c_str(), s.c_str(), mPath.c_str());
}

bool SettingsManager::WriteString(wstring key, wstring value)
{
    return (bool)WritePrivateProfileStringW(currentSection.c_str(), key.c_str(), value.c_str(), mPath.c_str());
}

bool SettingsManager::ReadSettings(sSettings &Destination_struct)
{
    bool result(true);
    currentSection = SECTION_NAME_GENERAL;
    ZeroMemory(&Destination_struct, sizeof(Destination_struct));

    // Read all values from .ini file to Destination_struct
    Destination_struct.Add2RecentDocs = GetBool(L"Add2RecentDocs", true);
    Destination_struct.Antialias = GetBool(L"AntiAlias", true);
    Destination_struct.AsIcon = GetBool(L"AsIcon", true);
    Destination_struct.BGPath = GetString(L"BGPath", L"", MAX_PATH);    
    Destination_struct.DisableUpdates = GetBool(L"DisableUpdates", false);
    Destination_struct.ForceVersion = GetBool(L"ForceVersion", false);
    Destination_struct.JLbms = GetBool(L"JLBookMarks", true);
    Destination_struct.JLfrequent = GetBool(L"JLFrequent", true);
    Destination_struct.JLpl = GetBool(L"JLPlayList", true);
    Destination_struct.JLrecent = GetBool(L"JLRecent", true);
    Destination_struct.JLtasks = GetBool(L"JLTasks", true);
    Destination_struct.Overlay = GetBool(L"IconOverlay", true);
    Destination_struct.Progressbar = GetBool(L"ProgressBar", true);
    Destination_struct.RemoveTitle = GetBool(L"RemoveTitle", false);
    Destination_struct.Revertto = GetInt(L"RevertTo", BG_TRANSPARENT);
    Destination_struct.Shrinkframe = GetBool(L"ShrinkFrame", false);
    Destination_struct.Stoppedstatus = GetBool(L"StoppedStatusOn", true);
    Destination_struct.Streamstatus = GetBool(L"StreamStatusOn", true);
    Destination_struct.Text = GetString(L"Text", 
        L"%c%%s%%curpl% of %totalpl%. ‡%c%%s%%title%‡%c%%s%%artist%‡‡%c%%s%%curtime%/%totaltime%‡%c%%s%Track #: %track%        Volume: %volume%%");
    Destination_struct.Thumbnailbackground = GetInt(L"ThumbnailBackGround", BG_ALBUMART);
    Destination_struct.Thumbnailbuttons = GetBool(L"ThumbnailButtons", true);
    Destination_struct.Thumbnailenabled = GetBool(L"ThumbnailEnabled", true);
    Destination_struct.Thumbnailpb = GetBool(L"ThumbnailPB", false);
    Destination_struct.VolumeControl = GetBool(L"VolumeControl", false);
    Destination_struct.VuMeter = GetBool(L"VUMeter", false);   
    Destination_struct.LowFrameRate = GetBool(L"LowFrameRate", false);
    Destination_struct.LastTab = GetInt(L"LastTab", 0);
    Destination_struct.LastUpdateCheck = GetInt(L"LastUpdateCheck", 0);
    Destination_struct.IconSize = GetInt(L"IconSize", 50);
    Destination_struct.IconPosition = GetInt(L"IconPosition", IP_UPPERLEFT);
    
    // Decoding bool[16] Buttons
    wstring Buttons = GetString(L"Buttons", L"1111100000000000");
    for (int i = 0; i!= 16; ++i)
    {
        Destination_struct.Buttons[i] = (Buttons[i] == '1' ? true : false);
    }

    if (!GetBool(L"OldSettings", false))
    {
        ReadOldSettings(Destination_struct);
        WriteBool(L"OldSettings", true);
    }

    // Read font
    currentSection = SECTION_NAME_FONT;

    if (!GetPrivateProfileStructW(SECTION_NAME_FONT, L"font", &Destination_struct.font, 
        sizeof(Destination_struct.font), mPath.c_str()))
    {
        LOGFONT ft = {};
        wcsncpy_s(ft.lfFaceName, L"Segoe UI", 32);
        ft.lfHeight = -14;
        ft.lfWeight = FW_NORMAL;
        Destination_struct.font = ft;
    }

    Destination_struct.text_color = GetInt(L"color", RGB(255,255,255));
    Destination_struct.bgcolor = GetInt(L"bgcolor", RGB(0,0,0));

    return result;
}

bool SettingsManager::WriteSettings(const sSettings &Source_struct)
{
    int result = 1;
    currentSection = SECTION_NAME_GENERAL;

    // Write values form Source_struct to .ini file
    result = result & (int)WriteBool(L"Add2RecentDocs", Source_struct.Add2RecentDocs);
    result = result & (int)WriteBool(L"AntiAlias", Source_struct.Antialias);
    result = result & (int)WriteBool(L"AsIcon", Source_struct.AsIcon);
    result = result & (int)WriteString(L"BGPath", Source_struct.BGPath);
    result = result & (int)WriteBool(L"DisableUpdates", Source_struct.DisableUpdates);
    result = result & (int)WriteBool(L"ForceVersion", Source_struct.ForceVersion);
    result = result & (int)WriteBool(L"JLBookMarks", Source_struct.JLbms);
    result = result & (int)WriteBool(L"JLFrequent", Source_struct.JLfrequent);
    result = result & (int)WriteBool(L"JLPlayList", Source_struct.JLpl);
    result = result & (int)WriteBool(L"JLRecent", Source_struct.JLrecent);
    result = result & (int)WriteBool(L"JLTasks", Source_struct.JLtasks);
    result = result & (int)WriteBool(L"IconOverlay", Source_struct.Overlay);
    result = result & (int)WriteBool(L"ProgressBar", Source_struct.Progressbar);
    result = result & (int)WriteBool(L"RemoveTitle", Source_struct.RemoveTitle);
    result = result & (int)WriteInt(L"RevertTo", Source_struct.Revertto);
    result = result & (int)WriteBool(L"ShrinkFrame", Source_struct.Shrinkframe);
    result = result & (int)WriteBool(L"StoppedStatusOn", Source_struct.Stoppedstatus);
    result = result & (int)WriteBool(L"StreamStatusOn", Source_struct.Streamstatus);
    result = result & (int)WriteString(L"Text", Source_struct.Text);
    result = result & (int)WriteInt(L"ThumbnailBackGround", Source_struct.Thumbnailbackground);
    result = result & (int)WriteBool(L"ThumbnailButtons", Source_struct.Thumbnailbuttons);
    result = result & (int)WriteBool(L"ThumbnailEnabled", Source_struct.Thumbnailenabled);
    result = result & (int)WriteBool(L"ThumbnailPB", Source_struct.Thumbnailpb);
    result = result & (int)WriteBool(L"VolumeControl", Source_struct.VolumeControl);
    result = result & (int)WriteBool(L"VUMeter", Source_struct.VuMeter);
    result = result & (int)WriteBool(L"LowFrameRate", Source_struct.LowFrameRate);
    result = result & (int)WriteInt(L"LastTab", Source_struct.LastTab);
    result = result & (int)WriteInt(L"LastUpdateCheck", Source_struct.LastUpdateCheck);
    result = result & (int)WriteInt(L"IconSize", Source_struct.IconSize);
    result = result & (int)WriteInt(L"IconPosition", Source_struct.IconPosition);

    // Encoding bool[16] Buttons
    wstringstream s;
    for (int i = 0; i != 16; ++i)
    {
        s << Source_struct.Buttons[i];
    }
    result = result & (int)WriteString(L"Buttons", s.str());

    // Font
    currentSection = SECTION_NAME_FONT;
    result = result & (int)WritePrivateProfileStructW(SECTION_NAME_FONT, L"font", (LPVOID)(&Source_struct.font), 
        sizeof(Source_struct.font), mPath.c_str());
    result = result * (int)WriteInt(L"color", Source_struct.text_color);
    result = result * (int)WriteInt(L"bgcolor", Source_struct.bgcolor);
    
    return (result == 1);
}

bool SettingsManager::ReadResume(sResumeSettings &resume)
{
    ZeroMemory(&resume, sizeof(resume));
    currentSection = SECTION_NAME_RESUME;
    resume.ResumePosition = GetInt(L"ResumePosition", 0);
    resume.ResumeTime = GetInt(L"ResumeTime", 0);

    return true;
}

bool SettingsManager::WriteResume(sResumeSettings &resume)
{
    int result = 0;
    currentSection = SECTION_NAME_RESUME;
    result = result & (int)WriteInt(L"ResumePosition", resume.ResumePosition);
    result = result & (int)WriteInt(L"ResumeTime", resume.ResumeTime);
    
    return (result == 1);
}

void SettingsManager::WriteSettings_ToForm(HWND hwnd, HWND WinampWnd, const sSettings &Settings)
{
    //Aero peek settings
    switch (Settings.Thumbnailbackground)
    {
    case BG_TRANSPARENT:
        SendMessage(GetDlgItem(hwnd, IDC_RADIO1), (UINT) BM_SETCHECK, BST_CHECKED , 0);
        break;
    case BG_ALBUMART:
        SendMessage(GetDlgItem(hwnd, IDC_RADIO2), (UINT) BM_SETCHECK, BST_CHECKED , 0);
        break;
    case BG_CUSTOM:
        SendMessage(GetDlgItem(hwnd, IDC_RADIO3), (UINT) BM_SETCHECK, BST_CHECKED , 0);
        break;
    }

    switch (Settings.IconPosition)
    {
    case IP_UPPERLEFT:
        SendMessage(GetDlgItem(hwnd, IDC_RADIO4), (UINT) BM_SETCHECK, BST_CHECKED , 0);
        break;
    case IP_LOWERLEFT:
        SendMessage(GetDlgItem(hwnd, IDC_RADIO7), (UINT) BM_SETCHECK, BST_CHECKED , 0);
        break;
    case IP_UPPERRIGHT:
        SendMessage(GetDlgItem(hwnd, IDC_RADIO6), (UINT) BM_SETCHECK, BST_CHECKED , 0);
        break;
    case IP_LOWERRIGHT:
        SendMessage(GetDlgItem(hwnd, IDC_RADIO8), (UINT) BM_SETCHECK, BST_CHECKED , 0);
        break;
    }

    SendMessage(GetDlgItem(hwnd, IDC_COMBO1), CB_ADDSTRING, 0, (LPARAM)WASABI_API_LNGSTRINGW(IDS_TRANSPARENT));
    SendMessage(GetDlgItem(hwnd, IDC_COMBO1), CB_ADDSTRING, 0, (LPARAM)WASABI_API_LNGSTRINGW(IDS_ALBUM_ART));
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
    SendMessage(GetDlgItem(hwnd, IDC_CHECK36), (UINT) BM_SETCHECK, static_cast<WPARAM>(Settings.LowFrameRate), 0);

    //Trackbar
    SendMessage(GetDlgItem(hwnd, IDC_SLIDER1), TBM_SETRANGEMAX, TRUE, 100);
    SendMessage(GetDlgItem(hwnd, IDC_SLIDER1), TBM_SETRANGEMIN, TRUE, 1);
    SendMessage(GetDlgItem(hwnd, IDC_SLIDER1), TBM_SETPOS, TRUE, Settings.IconSize);
    
    wstringstream size;
    size << "Icon size (" << SendMessage(GetDlgItem(hwnd, IDC_SLIDER1), TBM_GETPOS, NULL, NULL) << "%)";
    SetWindowTextW(GetDlgItem(hwnd, IDC_ICONSIZE), size.str().c_str());

    SetWindowText(GetDlgItem(hwnd, IDC_EDIT2), Settings.BGPath.c_str());

    std::wstring tmpbuf = Settings.Text;
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
}

void SettingsManager::ReadOldSettings( sSettings & Destination_struct )
{
    struct sSettings_old 
    {
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

    sSettings_old old_settings = {};

    if (!GetPrivateProfileStruct(__T("win7shell"), __T("settings"), &old_settings, sizeof(old_settings), mPath.c_str()))
    {
        return;
    }
    else
    {        
        Destination_struct.Add2RecentDocs = old_settings.Add2RecentDocs;
        Destination_struct.Antialias = old_settings.Antialias;
        Destination_struct.AsIcon = old_settings.AsIcon;
        std::copy(old_settings.Buttons, old_settings.Buttons+16, Destination_struct.Buttons);
        Destination_struct.DisableUpdates = old_settings.DisableUpdates;
        Destination_struct.ForceVersion = old_settings.ForceVersion;
        Destination_struct.JLbms = old_settings.JLbms;
        Destination_struct.JLfrequent = old_settings.JLfrequent;
        Destination_struct.JLpl = true;
        Destination_struct.JLrecent = old_settings.JLrecent;
        Destination_struct.JLtasks = old_settings.JLtasks;
        Destination_struct.Overlay = old_settings.Overlay;
        Destination_struct.Progressbar = old_settings.Progressbar;
        Destination_struct.RemoveTitle = old_settings.RemoveTitle;
        Destination_struct.Revertto = old_settings.Revertto;
        Destination_struct.Shrinkframe = old_settings.Shrinkframe;
        Destination_struct.Stoppedstatus = old_settings.Stoppedstatus;
        Destination_struct.Streamstatus = old_settings.Streamstatus;
        Destination_struct.Thumbnailbackground = old_settings.Thumbnailbackground;
        Destination_struct.Thumbnailbuttons = old_settings.Thumbnailbuttons;
        Destination_struct.Thumbnailenabled = old_settings.Thumbnailenabled;
        Destination_struct.Thumbnailpb = old_settings.Thumbnailpb;
        Destination_struct.VuMeter = old_settings.VuMeter;
    }
}

bool SettingsManager::WriteButtons(std::vector<int> &tba)
{
    if (tba.size() > 7)
    {
        tba.resize(7);
    }

    std::wstringstream button_TextStream;

    for (int i = 0; i != tba.size(); ++i)
    {        
        button_TextStream << tba[i] << ",";
    }
    std::wstring button_Text = button_TextStream.str();
    button_Text.erase(button_Text.length()-1, 1);

    return WritePrivateProfileString(SECTION_NAME_GENERAL, L"ThumbButtons", button_Text.c_str(), mPath.c_str());
}

bool SettingsManager::ReadButtons( std::vector<int> &tba )
{
    std::wstring text;
    text.resize(100);
    if (!GetPrivateProfileString(SECTION_NAME_GENERAL, L"ThumbButtons", L"1076,1077,1078,1079,1084", &text[0], 99, mPath.c_str()))
    {
        return false;
    }
    text.resize(wcslen(&text[0]));

    tba.clear();
    std::wstring::size_type pos = std::wstring::npos;
    do
    {
        pos = text.find_first_of(L',');
        std::wstringstream buffer;
        buffer << text.substr(0, pos);
        int code;
        buffer >> code;
        if (code == -1)
        {
            continue;
        }
        tba.push_back(code);
        text.erase(0, pos+1);
    } 
    while (pos != std::wstring::npos);

    return true;
}
