#ifndef tools_h__
#define tools_h__

#include <string>
#include <Windows.h>
#include <propvarutil.h>
#include <propkey.h>
#include <time.h>

namespace tools
{
    // Namespace variables
    std::wstring BM_path = L"";
    std::wstring INI_DIR_path = L"";
    std::wstring PLUG_DIR_path = L"";
    int ver = -1;

    // Functions declarations
    UINT GetWinampVersion(HWND winamp);
    std::wstring getBMS(HWND WinampWindow);
    std::wstring getShortPluginDllPath();
    std::wstring getWinampINIPath(HWND wnd);
    HRESULT __CreateShellLink(PCWSTR filename, PCWSTR pszTitle, IShellLink **ppsl);
    HWND getWinampWindow();
    std::wstring getInstallPath();
    std::wstring GetFileName(HWND hwnd);
    bool is_in_recent(std::wstring filename);
    HIMAGELIST prepareIcons();
    std::wstring SecToTime(int sec);

    // Function definitions
    std::wstring getBMS(HWND WinampWindow)
    {
        if(BM_path.empty())
        {
            if(GetWinampVersion(WinampWindow) < 0x5058)
            {
                char* bmfile=(char*)SendMessage(WinampWindow,WM_WA_IPC,0,IPC_ADDBOOKMARK);
                BM_path.resize(MAX_PATH);
                MultiByteToWideChar(CP_ACP, 0, bmfile, strlen(bmfile), &BM_path[0], MAX_PATH);
                BM_path.resize(wcslen(BM_path.c_str()));
            }
            else
            {
                BM_path = (wchar_t*)SendMessage(WinampWindow,WM_WA_IPC,0,IPC_ADDBOOKMARKW);
            }
        }

        std::wifstream is(BM_path.c_str());
        if (is.fail())
            return L"";

        std::wstring data;
        std::getline(is, data, L'\0');
        return data;
    }

    UINT GetWinampVersion(HWND winamp)
    {
        if(ver == -1){
            return (ver = (int)SendMessage(winamp,WM_WA_IPC,0,IPC_GETVERSION));
        }
        return ver;
    }
    
    std::wstring getWinampINIPath(HWND wnd)
    {
	    if(INI_DIR_path.empty())
	    {
		    if(IsWindow(wnd))
		    {
                if(tools::GetWinampVersion(wnd) < 0x5058)
			    {
				    char *dir=(char*)SendMessage(wnd,WM_WA_IPC,0,IPC_GETINIDIRECTORY);
				    INI_DIR_path.resize(MAX_PATH);
				    MultiByteToWideChar(CP_ACP, 0, dir, strlen(dir), &INI_DIR_path[0], MAX_PATH);
				    INI_DIR_path.resize(wcslen(INI_DIR_path.c_str()));
			    }
			    else
			    {
				    #ifndef IPC_GETINIDIRECTORYW
					    #define IPC_GETINIDIRECTORYW 1335
				    #endif
				    INI_DIR_path=(wchar_t*)SendMessage(wnd,WM_WA_IPC,0,IPC_GETINIDIRECTORYW);
			    }
			    return INI_DIR_path;
		    }
	    }
	    return INI_DIR_path;
    }

    int GetCurrentDay()
    {
        time_t date;
        date = time(NULL);
        return localtime(&date)->tm_mday;
    }

    std::wstring getShortPluginDllPath(HINSTANCE dll_instance)
    {
        if(PLUG_DIR_path.empty())
        {
            // get the shortfilename version to not complicate things
            // when generating a path to access the exported functions
            PLUG_DIR_path.reserve(MAX_PATH);
            PLUG_DIR_path.resize(MAX_PATH, '\0');
            GetModuleFileName(dll_instance, &PLUG_DIR_path[0], MAX_PATH);
            GetShortPathName(PLUG_DIR_path.c_str(), &PLUG_DIR_path[0], MAX_PATH);
            PLUG_DIR_path.resize(wcslen(PLUG_DIR_path.c_str()));
        }
        return PLUG_DIR_path;
    }

    HRESULT __CreateShellLink(PCWSTR filename, PCWSTR pszTitle, IShellLink **ppsl)
    {
        IShellLink *psl;
        HRESULT hr = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&psl));
        if (SUCCEEDED(hr))
        {
            // TODO optimise me
            std::wstring fname;
            fname.resize(MAX_PATH);
            GetModuleFileName(0, &fname[0], MAX_PATH);
            fname.resize(wcslen(fname.c_str()));

            std::wstring shortfname;
            shortfname.resize(MAX_PATH);			
            GetShortPathName(fname.c_str(), &shortfname[0], MAX_PATH);
            shortfname.resize(wcslen(shortfname.c_str()));

            fname.clear();
            fname.resize(MAX_PATH);
            if (GetShortPathName(filename, &fname[0], MAX_PATH) == 0)
            {
                fname = filename;
                fname.resize(wcslen(filename));
            }
            psl->SetIconLocation(shortfname.c_str(), 0);

            hr = psl->SetPath(shortfname.c_str());

            if (SUCCEEDED(hr))
            {
                hr = psl->SetArguments(fname.c_str());
                if (SUCCEEDED(hr))
                {
                    // The title property is required on Jump List items provided as an IShellLink
                    // instance.  This value is used as the display name in the Jump List.
                    IPropertyStore *pps;
                    hr = psl->QueryInterface(IID_PPV_ARGS(&pps));
                    if (SUCCEEDED(hr))
                    {
                        PROPVARIANT propvar;
                        hr = InitPropVariantFromString(pszTitle, &propvar);
                        if (SUCCEEDED(hr))
                        {
                            hr = pps->SetValue(PKEY_Title, propvar);
                            if (SUCCEEDED(hr))
                            {
                                hr = pps->Commit();
                                if (SUCCEEDED(hr))
                                {
                                    hr = psl->QueryInterface(IID_PPV_ARGS(ppsl));
                                }
                            }
                            PropVariantClear(&propvar);
                        }
                        pps->Release();
                    }
                }
            }
            else
            {
                hr = HRESULT_FROM_WIN32(GetLastError());
            }
            psl->Release();
        }
        return hr;
    }

    HWND getWinampWindow()
    {
        HWND hwnd = FindWindow(L"Winamp v1.x", NULL);
        if (hwnd != 0)
        {
            if (FindWindowEx(hwnd, NULL, L"WinampVis", NULL) == 0 )
            {
                hwnd = 0;
            }
        }

        if (hwnd == 0)
        {	
            std::wstring ipath = getInstallPath();
            if (!ipath.empty())					
            {
                ipath += L"\\Winamp.exe";
                ShellExecute(NULL, L"open", ipath.c_str(), NULL, NULL, SW_SHOW);
                int wait = 0;
                while (hwnd == 0 && wait < 24)
                {
                    Sleep(200);
                    wait++;
                    hwnd = FindWindow(L"Winamp v1.x", NULL);
                }
            }
        }
        return hwnd;
    }

    std::wstring getInstallPath()
    {
        std::wstring ret;	
        ret.reserve(255);
        ret.resize(255, '\0');
        HKEY hKey;
        LONG returnStatus;
        DWORD dwType=REG_SZ;
        DWORD dwSize=255;
        returnStatus = RegOpenKeyEx(HKEY_CURRENT_USER, L"Software\\Winamp", 0L,  KEY_READ, &hKey);
        if (returnStatus == ERROR_SUCCESS)
        {
            returnStatus = RegQueryValueEx(hKey, L"", NULL, &dwType,(LPBYTE)&ret[0], &dwSize);
            ret.resize(wcslen(ret.c_str()));
            if (returnStatus == ERROR_SUCCESS)
                return ret;
        }

        RegCloseKey(hKey);

        return L"";
    }


    std::wstring GetFileName(HWND hwnd)
    {
        IFileDialog *pfd;

        // CoCreate the dialog object.
        HRESULT hr = CoCreateInstance(CLSID_FileOpenDialog, 
            NULL, 
            CLSCTX_INPROC_SERVER, 
            IID_PPV_ARGS(&pfd));

        if (SUCCEEDED(hr))
        {
            // Show the dialog
            wchar_t tmp[128], tmp2[128], tmp3[128];
            COMDLG_FILTERSPEC rgSpec[] =
            { 			
                { WASABI_API_LNGSTRINGW_BUF(IDS_ALL_IMAGE_FORMATS,tmp,128),
                __T("*.bmp;*.dib;*.jpg;*.jpeg;*.gif;*.png;*.ico;*.tiff;*.tif;*.wmf;*.emf;*.exif")
                }
            };

            pfd->SetFileTypes(1, rgSpec);
            pfd->SetOkButtonLabel(WASABI_API_LNGSTRINGW_BUF(IDS_USE_AS_THUMB_BKGND,tmp2,128));
            pfd->SetTitle(WASABI_API_LNGSTRINGW_BUF(IDS_SELECT_IMAGE_FILE,tmp3,128));
            hr = pfd->Show(hwnd);

            if (SUCCEEDED(hr))
            {
                // Obtain the result of the user's interaction with the dialog.
                IShellItem *psiResult;
                hr = pfd->GetResult(&psiResult);

                if (SUCCEEDED(hr))
                {
                    wchar_t *w;
                    psiResult->GetDisplayName(SIGDN_FILESYSPATH, &w);
                    psiResult->Release();
                    std::wstring ret(w);
                    CoTaskMemFree(w);
                    return ret;
                }
            } 
            pfd->Release();
        }
        return __T("");
    }

    bool is_in_recent(std::wstring filename)
    {
        std::wstring path;
        path.resize(MAX_PATH);
        SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, 0, &path[0]);
        path.resize(wcslen(path.c_str()));

        std::wstring::size_type pos = filename.find_last_of(L"\\");
        if (pos != std::wstring::npos)
            filename.erase(0, pos+1);

        path += L"\\Microsoft\\Windows\\Recent\\" + filename + L".lnk";

        WIN32_FIND_DATA ffd;
        if (FindFirstFile(path.c_str(), &ffd) == INVALID_HANDLE_VALUE)
            return false;
        return true;
    }

    HIMAGELIST prepareIcons(HINSTANCE DllInstance)
    {
        InitCommonControls();

        HIMAGELIST himlIcons;  
        HICON hicon;  

        himlIcons = ImageList_Create(GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), ILC_COLOR32, 9, 0); 	

        for (int i = 0; i < 11; ++i)
        {
            hicon = LoadIcon(DllInstance, MAKEINTRESOURCE(IDI_ICON1+i)); 

            if (hicon == NULL)
                return NULL;		
            else
                ImageList_AddIcon(himlIcons, hicon);
        }

        return himlIcons;	
    }


    std::wstring SecToTime(int sec)
    {	
        int ZH = sec / 3600;
        int ZM = sec / 60 - ZH * 60;
        int ZS = sec - (ZH * 3600 + ZM * 60);

        std::wstringstream ss;
        if (ZH != 0)
        {
            if (ZH < 10)
                ss << "0" << ZH << ":";
            else
                ss << ZH << ":";
        }

        if (ZM < 10)
            ss << "0" << ZM << ":";
        else
            ss << ZM << ":";

        if (ZS < 10)
            ss << "0" << ZS;
        else
            ss << ZS;

        return ss.str();
    }
}
#endif // tools_h__
