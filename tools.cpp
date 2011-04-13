#include <string>
#include <Windows.h>

namespace tools
{
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
}