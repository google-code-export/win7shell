#ifndef taskbar_h__
#define taskbar_h__

#include <Windows.h>
#include <Shobjidl.h>
#include <string>
#include <vector>

class iTaskBar
{
public:
    iTaskBar();
    ~iTaskBar();

    bool Reset(HWND WinampWnd);
    HRESULT SetImageList(HIMAGELIST ImageList);
    HRESULT ThumbBarUpdateButtons(std::vector<THUMBBUTTON>& buttons, bool first);
    void SetProgressState(TBPFLAG newstate);
    void SetIconOverlay(HICON icon, std::wstring text);
    void SetProgressValue(ULONGLONG completed, ULONGLONG total);    
    void SetWindowAttr(bool enable, bool flip, bool peek);

private:
    HWND mWinampWnd;
    TBPFLAG progressbarstate;

    ITaskbarList3* pTBL;
};

inline void iTaskBar::SetProgressValue( ULONGLONG completed, ULONGLONG total )
{
    pTBL->SetProgressValue(mWinampWnd, completed, total);
}

inline void iTaskBar::SetProgressState( TBPFLAG newstate )
{
    if (newstate != progressbarstate)
    {
        pTBL->SetProgressState(mWinampWnd, newstate);
        progressbarstate = newstate;
    }
}

#endif // taskbar_h__