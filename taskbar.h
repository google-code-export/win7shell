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
    void SetWindowAttr(bool enable, bool flip);

private:
    HWND mWinampWnd;
    TBPFLAG progressbarstate;

    ITaskbarList3* pTBL;
};

#endif // taskbar_h__