#include <windows.h>
#include <string>
#include <map>

class MetaData 
{
public: 
	void setWinampWindow(HWND winampwindow);
	bool reset(std::wstring, bool);
	std::wstring getMetadata(std::wstring tag);
	std::wstring getFileName();

private:
	HWND mhwnd;
	std::wstring mfilename;
	std::map<std::wstring, std::wstring> cache;
};
