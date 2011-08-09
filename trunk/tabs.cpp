/*

2009 Killboy/SND

*/

#include <windows.h>
#include <commctrl.h> // Tabs
#include <shlwapi.h>  // DllGetVersion
#include "sdk/winamp/wa_ipc.h"

// Flags for EnableThemeDialogTexture
#define ETDT_DISABLE       0x00000001
#define ETDT_ENABLE        0x00000002
#define ETDT_USETABTEXTURE 0x00000004
#define ETDT_ENABLETAB     (ETDT_ENABLE | ETDT_USETABTEXTURE)

wchar_t PropName[] = L"TabCtrlCurWnd";

int  AddTab(HWND TabWindow, HWND Window, wchar_t * Caption, int Index);
bool RemoveTab(HWND TabWindow, int Index);
bool TabCleanup(HWND TabWindow);
bool SetTabIcon(HWND TabWindow, int Index, HICON Icon);
int  TabToFront(HWND TabWindow, int Index);

int AddTab(HWND TabWindow, HWND Window, wchar_t * Caption, int Index, HWND winamp){
TC_ITEM TabData;
RECT TabRect;

    if(Index == -1)
        Index = SendMessage(TabWindow, TCM_GETITEMCOUNT, 0, 0);

    TabData.mask = TCIF_TEXT | TCIF_PARAM;
    TabData.pszText = Caption;
    TabData.cchTextMax = wcslen(Caption) + 1;
    TabData.lParam = (LPARAM)Window;

    Index = SendMessage(TabWindow, TCM_INSERTITEM, Index, (LPARAM)&TabData);
    if(Index != -1)
    {
        GetWindowRect(TabWindow, &TabRect);
        MapWindowPoints(HWND_DESKTOP, GetParent(TabWindow), (POINT *)&TabRect, 2);
        SendMessage(TabWindow, TCM_ADJUSTRECT, false, (LPARAM)&TabRect);
        TabRect.right  -= TabRect.left; // .right  == width
        TabRect.bottom -= TabRect.top;  // .bottom == heigth

        SetWindowPos(Window, HWND_BOTTOM, TabRect.left, TabRect.top, TabRect.right, TabRect.bottom, SWP_HIDEWINDOW);
    
        SetProp(TabWindow, PropName, (HANDLE)Window);

        SendMessage(winamp,WM_WA_IPC,(WPARAM)Window,IPC_USE_UXTHEME_FUNC);
    }

    return Index;
}

bool RemoveTab(HWND TabWindow, int Index){
TC_ITEM TabData;
HIMAGELIST ImageList;
int CurIndex;
int Count;

    TabData.mask = TCIF_IMAGE | TCIF_PARAM;

    if(SendMessage(TabWindow, TCM_GETITEM, Index, (LPARAM)&TabData))
    {
        CurIndex = SendMessage(TabWindow, TCM_GETCURSEL, 0, 0);

        if(SendMessage(TabWindow, TCM_DELETEITEM, Index, 0))
        {
            if(TabData.iImage != -1){
                SendMessage(TabWindow, TCM_REMOVEIMAGE, TabData.iImage, 0);
            }

            Count = SendMessage(TabWindow, TCM_GETITEMCOUNT, 0, 0);
            if(!Count){
                ShowWindow((HWND)TabData.lParam, SW_HIDE);
                ImageList = (HIMAGELIST)SendMessage(TabWindow, TCM_GETIMAGELIST, 0, 0);
                if(ImageList){
                    ImageList_Destroy(ImageList);
                    SendMessage(TabWindow, TCM_SETIMAGELIST, 0, 0);
                }
                RemoveProp(TabWindow, PropName);
            }
            else if(Index == CurIndex){ // Deleting the tab currently visible
                if(Index == Count) Index--; // Last tab
                TabToFront(TabWindow, Index);
            }
            return true;
        }
    }
    return false;
}

bool TabCleanup(HWND TabWindow){
int Count;
bool Result = true;

    Count = SendMessage(TabWindow, TCM_GETITEMCOUNT, 0, 0);

    for(int i = 1; i <= Count; i++){
        Result = RemoveTab(TabWindow, Count-i) && Result;
    }

    return Result;
}

bool SetTabIcon(HWND TabWindow, int Index, HICON Icon){
TC_ITEM TabData;
HIMAGELIST ImageList;

    ImageList = (HIMAGELIST)SendMessage(TabWindow, TCM_GETIMAGELIST, 0, 0);

    if(!ImageList){
        ImageList = ImageList_Create(16, 16, ILC_COLOR32 | ILC_MASK, 0, 1);
        SendMessage(TabWindow, TCM_SETIMAGELIST, 0, (LPARAM)ImageList);
    }

    TabData.mask = TCIF_IMAGE;

    if(SendMessage(TabWindow, TCM_GETITEM, Index, (LPARAM)&TabData)){
        if(Icon){ // Index -1 == AddIcon
            TabData.iImage = ImageList_ReplaceIcon(ImageList, TabData.iImage, Icon);
        }
        else if(TabData.iImage != -1){
            SendMessage(TabWindow, TCM_REMOVEIMAGE, TabData.iImage, 0);
            TabData.iImage = -1;
        }

        if(SendMessage(TabWindow, TCM_SETITEM, Index, (LPARAM)&TabData)){
            return true;
        }
    }

    return false;
}

int TabToFront(HWND TabWindow, int Index){
TC_ITEM TabData;

    if(Index == -1)
        Index = SendMessage(TabWindow, TCM_GETCURSEL, 0, 0);

    TabData.mask = TCIF_PARAM;

    if(SendMessage(TabWindow, TCM_GETITEM, Index, (LPARAM)&TabData)){
        ShowWindow((HWND)GetProp(TabWindow, PropName), SW_HIDE);
        SetWindowPos((HWND)TabData.lParam, HWND_TOP, 0, 0, 0, 0, SWP_SHOWWINDOW | SWP_NOMOVE | SWP_NOSIZE);
        SetProp(TabWindow, PropName, (HANDLE)TabData.lParam);
        SendMessage(TabWindow, TCM_SETCURSEL, Index, 0);
        return Index;
    }

    return -1;
}