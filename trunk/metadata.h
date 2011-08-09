#ifndef metadata_h__
#define metadata_h__

#include <windows.h>
#include <string>
#include <map>

using namespace std;

class MetaData 
{
public: 	
    MetaData();
	void setWinampWindow(HWND winampwindow);
	bool reset(std::wstring, bool);
	std::wstring getMetadata(std::wstring tag);
	std::wstring getFileName() const;
    bool CheckPlayCount();

private:
	HWND mhwnd;
	//typedef std::map<std::wstring, std::wstring> cacheType;
	//typedef std::pair<std::wstring, std::wstring> cacheEntry;
    //cacheType cache;
	std::map<std::wstring, std::wstring> cache;
	std::wstring mfilename;
    bool isFile;
    int m_play_count;    
};

#endif // metadata_h__