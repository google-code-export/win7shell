#include <objectarray.h>
#include <shobjidl.h>
#include <propkey.h>
#include <propvarutil.h>
#include <knownfolders.h>
#include <shlobj.h>
#include <string>

class JumpList
{
public:
	JumpList();
	~JumpList();

	HRESULT _CreateShellLink(PCWSTR pszArguments, PCWSTR pszTitle, IShellLink **ppsl, int iconindex);
	HRESULT _AddTasksToList(ICustomDestinationList *pcdl);
	bool CreateJumpList(std::wstring pluginpath, std::wstring pref, std::wstring openfile,
		bool recent, bool frequent, bool tasks);
	bool DeleteJumpList();

private:
	ICustomDestinationList *pcdl;
	HRESULT hr;
	std::wstring path, s1, s2;
};

