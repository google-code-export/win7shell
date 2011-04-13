#ifndef jumplist_h__
#define jumplist_h__

#include <objectarray.h>
#include <shobjidl.h>
#include <propkey.h>
#include <propvarutil.h>
#include <knownfolders.h>
#include <shlobj.h>
#include <string>
#include <map>

class JumpList
{
public:
	JumpList();
	~JumpList();

	HRESULT _CreateShellLink(PCWSTR pszArguments, PCWSTR pszTitle, IShellLink **ppsl, int iconindex, bool WA);
	bool _IsItemInArray(std::wstring path, IObjectArray *poaRemoved);
	HRESULT _AddTasksToList();
	HRESULT _AddCategoryToList();
	HRESULT _AddCategoryToList2();
	bool CreateJumpList(std::wstring pluginpath, std::wstring pref, std::wstring fromstart, 
		std::wstring resume, std::wstring openfile, std::wstring bookmarks, std::wstring pltext, bool recent, 
		bool frequent, bool tasks, bool addbm, bool playlist, const std::wstring bms);
	bool DeleteJumpList();

private:
	ICustomDestinationList *pcdl;
	IObjectCollection *poc;
	HRESULT hr;
	std::wstring path, s1, s2, s3, s4, s5, s6;
};

#endif // jumplist_h__
