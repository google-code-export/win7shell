#include <Windows.h>
#include <ObjBase.h>
#include <dwmapi.h>
#include <fstream>

#include "taskbar.h"

iTaskBar::iTaskBar() :
mWinampWnd(NULL),
pTBL(NULL),
progressbarstate(TBPF_NOPROGRESS)
{    
    CoInitialize(0);
}

iTaskBar::~iTaskBar()
{
    if (pTBL != NULL)
        pTBL->Release();

    CoUninitialize();
}

bool iTaskBar::Reset(HWND WinampWnd)
{
    mWinampWnd = WinampWnd;

    if (pTBL != NULL)
        pTBL->Release();

    HRESULT hr = ::CoCreateInstance(CLSID_TaskbarList, NULL, CLSCTX_INPROC_SERVER,
        IID_ITaskbarList, reinterpret_cast<void**>(&pTBL) );

    if (!SUCCEEDED(hr))
        return false;
        
    hr = pTBL->HrInit();
    if (!SUCCEEDED(hr))
        return false;

    return true;
}

HRESULT iTaskBar::SetImageList(HIMAGELIST ImageList)
{
    return pTBL->ThumbBarSetImageList(mWinampWnd, ImageList);
}

HRESULT iTaskBar::ThumbBarUpdateButtons( std::vector<THUMBBUTTON>& thbButtons, bool first )
{
    if (first)
        return pTBL->ThumbBarAddButtons(mWinampWnd, thbButtons.size(), &thbButtons[0]);
    else
        return pTBL->ThumbBarUpdateButtons(mWinampWnd, thbButtons.size(), &thbButtons[0]);
}

void iTaskBar::SetIconOverlay( HICON icon, std::wstring text )
{
   pTBL->SetOverlayIcon(mWinampWnd, icon, text.c_str());
}

void iTaskBar::SetWindowAttr(bool enable, bool flip, bool peek)
{
    bool enabled = true;
    DwmInvalidateIconicBitmaps(mWinampWnd);
    DwmSetWindowAttribute(mWinampWnd, DWMWA_HAS_ICONIC_BITMAP, &enabled, sizeof(int));
    DwmSetWindowAttribute(mWinampWnd, DWMWA_FORCE_ICONIC_REPRESENTATION, &enabled, sizeof(int));

    enabled = peek;
    DwmSetWindowAttribute(mWinampWnd, DWMWA_DISALLOW_PEEK, &peek, sizeof(int));
    
    if (flip)
    {
        DWMFLIP3DWINDOWPOLICY flip_policy;
        flip_policy = flip ? DWMFLIP3D_EXCLUDEBELOW : DWMFLIP3D_DEFAULT;
        DwmSetWindowAttribute(mWinampWnd, DWMWA_FLIP3D_POLICY, &flip_policy, sizeof(int));
    }
}
