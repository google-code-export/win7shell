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

void iTaskBar::SetProgressState( TBPFLAG newstate )
{
    if (newstate != progressbarstate)
    {
        pTBL->SetProgressState(mWinampWnd, newstate);
        progressbarstate = newstate;
    }
}

HRESULT iTaskBar::SetImageList(HIMAGELIST ImageList)
{
    return pTBL->ThumbBarSetImageList(mWinampWnd, ImageList);
}

HRESULT iTaskBar::ThumbBarUpdateButtons( std::vector<THUMBBUTTON>& thbButtons, bool first )
{
//     THUMBBUTTON *button;
//     button = new THUMBBUTTON[2];
// 
//     ZeroMemory(&button[0], sizeof(button));
//     button[0].dwMask = THB_TOOLTIP;
//     button[0].iId = 0;
//     button[0].hIcon = NULL;
//     button[0].iBitmap = NULL;
//     wcscpy(button[0].szTip, L"Text");
// 
//     ZeroMemory(&button[1], sizeof(button));
//     button[1].dwMask = THB_TOOLTIP;
//     button[1].iId = 0;
//     button[1].hIcon = NULL;
//     button[1].iBitmap = NULL;
//     wcscpy(button[1].szTip, L"Text");
// 
//     if (first)
//         return pTBL->ThumbBarAddButtons(WinampWnd, 2, button);
//     else
//         return pTBL->ThumbBarUpdateButtons(WinampWnd, 2, button);


    if (first)
        return pTBL->ThumbBarAddButtons(mWinampWnd, thbButtons.size(), &thbButtons[0]);
    else
        return pTBL->ThumbBarUpdateButtons(mWinampWnd, thbButtons.size(), &thbButtons[0]);
// 
//     THUMBBUTTONMASK dwMask = THB_BITMAP | THB_TOOLTIP | THB_FLAGS;
// 
//     THUMBBUTTON thbButtons[2];
//     thbButtons[0].dwMask = dwMask;
//     thbButtons[0].iId = 0;
//     thbButtons[0].iBitmap = 0;
//     thbButtons[0].szTip = TEXT("Button 1");
//     thbButtons[0].dwFlags = THBF_DISMISSONCLICK;
// 
//     dwMask = THB_BITMAP | THB_TOOLTIP;
//     thbButtons[1].dwMask = dwMask;
//     thbButtons[1].iId = 1;
//     thbButtons[1].iBitmap = 1;
//     thbButtons[1].szTip = TEXT("Button 2");
// 
//             // Attach the toolbar to the thumbnail.
//             pTBL->ThumbBarAddButtons(WinampWnd, ARRAYSIZE(thbButtons), &thbButtons);
//        
//         pTBL->Release();
// 
}

void iTaskBar::SetIconOverlay( HICON icon, std::wstring text )
{
   pTBL->SetOverlayIcon(mWinampWnd, icon, text.c_str());
}

void iTaskBar::SetProgressValue( ULONGLONG completed, ULONGLONG total )
{
    pTBL->SetProgressValue(mWinampWnd, completed, total);
}

void iTaskBar::SetWindowAttr(bool enable, bool flip)
{
    bool enabled = enable;    
    DwmInvalidateIconicBitmaps(mWinampWnd);
    DwmSetWindowAttribute(mWinampWnd, DWMWA_HAS_ICONIC_BITMAP, &enabled, sizeof(int));
    DwmSetWindowAttribute(mWinampWnd, DWMWA_FORCE_ICONIC_REPRESENTATION, &enabled, sizeof(int));
    DwmSetWindowAttribute(mWinampWnd, DWMWA_DISALLOW_PEEK, &enabled, sizeof(int));
    
    if (flip)
    {
        DWMFLIP3DWINDOWPOLICY flip_policy;
        flip_policy = flip ? DWMFLIP3D_EXCLUDEBELOW : DWMFLIP3D_DEFAULT;
        DwmSetWindowAttribute(mWinampWnd, DWMWA_FLIP3D_POLICY, &flip_policy, sizeof(int));
    }
}
