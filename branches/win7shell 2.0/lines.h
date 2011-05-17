#ifndef lines_h__
#define lines_h__

#include <string>
#include <vector>
#include "gen_win7shell.h"
#include "metadata.h"

class lines
{
public:

    lines(const sSettings &Settings, MetaData &Metadata, const HWND WinampWnd);

    inline std::wstring::size_type GetNumberOfLines() const { return m_texts.size(); }
    
    inline std::wstring GetLineText(int index) const { return m_texts[index]; }
    inline linesettings GetLineSettings(int index) const { return m_linesettings[index]; }

    void Parse();

private:

    inline void lines::ProcessLine(int index);
    inline std::wstring MetaWord(const std::wstring &word);

    std::vector<std::wstring> m_texts;
    std::vector<linesettings> m_linesettings;
    MetaData &m_metadata;
    const sSettings &m_settings;
    const HWND m_hwnd;    
};

#endif // lines_h__