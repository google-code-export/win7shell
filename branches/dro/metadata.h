#include <windows.h>
#include <string>
#include <map>

using namespace std;

class MetaData 
{
public: 
	MetaData() {isFile = false;}
	void setWinampWindow(HWND winampwindow);
	bool reset(std::wstring, bool);
	std::wstring getMetadata(std::wstring tag);
	std::wstring getFileName();
	bool isFile;

private:
	HWND mhwnd;
	//typedef std::map<std::wstring, std::wstring> cacheType;
	//typedef std::pair<std::wstring, std::wstring> cacheEntry;
    //cacheType cache;
	std::map<std::wstring, std::wstring> cache;
	std::wstring mfilename;
};