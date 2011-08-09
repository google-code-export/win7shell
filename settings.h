#ifndef settings_h__
#define settings_h__

#include <Windows.h>
#include <string>
#include <sstream>
#include <vector>
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

    static void WriteSettings_ToForm(HWND hwnd, HWND WinampWnd, const sSettings &Settings);

    bool WriteButtons(std::vector<int> &tba);
    bool ReadButtons(std::vector<int> &tba);

private:
    int GetInt(wstring key, int default_value);
    bool GetBool(wstring key, bool default_value);
    wstring GetString(wstring key, wstring default_value, size_t max_size = 1024);

    bool WriteInt(wstring key, int value);
    bool WriteBool(wstring key, bool value);
    bool WriteString(wstring key, wstring value);
    void ReadOldSettings(sSettings & Destination_struct);
    wstring mPath;
    wstring currentSection;
};

#endif // settings_h__