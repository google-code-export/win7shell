#ifndef settings_h__
#define settings_h__

#include <Windows.h>
#include <string>
#include <sstream>
#include "gen_win7shell.h"

using namespace std;

class SettingsManager
{
public:
    SettingsManager(wstring Path) : mPath(Path), currentSection(SECTION_NAME_GENERAL) {};

    bool ReadSettings(sSettings &Destination_struct);
    bool WriteSettings(const sSettings &Source_struct);

    bool ReadResume(sResumeSettings &resume);
    bool WriteResume(sResumeSettings &resume);

    static void ReadSettings_FromForm(HWND hwnd, HWND WinampWnd, sSettings &Settings);
    static void WriteSettings_ToForm(HWND hwnd, HWND WinampWnd, const sSettings &Settings);

private:
    int GetInt(wstring key, int default_value);
    bool GetBool(wstring key, bool default_value);
    wstring GetString(wstring key, wstring default_value, size_t max_size = 1024);

    bool WriteInt(wstring key, int value);
    bool WriteBool(wstring key, bool value);
    bool WriteString(wstring key, wstring value);

    wstring mPath;
    wstring currentSection;
};

#endif // settings_h__