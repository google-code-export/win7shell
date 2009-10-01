#include <objectarray.h>
#include <shobjidl.h>
#include <propkey.h>
#include <propvarutil.h>
#include <knownfolders.h>
#include <shlobj.h>
#include <string>
#include "jumplist.h"

JumpList::JumpList()
{
	hr = CoCreateInstance(CLSID_DestinationList, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pcdl));
}

JumpList::~JumpList()
{
	pcdl->Release();
}


// Creates a CLSID_ShellLink to insert into the Tasks section of the Jump List.  This type of Jump
// List item allows the specification of an explicit command line to execute the task.
HRESULT JumpList::_CreateShellLink(PCWSTR pszArguments, PCWSTR pszTitle, IShellLink **ppsl, int iconindex)
{
	IShellLink *psl;
	HRESULT hr = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&psl));
	if (SUCCEEDED(hr))
	{
		psl->SetIconLocation(path.c_str(), iconindex);
		hr = psl->SetPath(L"rundll32.exe");
		if (SUCCEEDED(hr))
		{
			hr = psl->SetArguments(pszArguments);
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

bool JumpList::CreateJumpList(std::wstring pluginpath, std::wstring pref, std::wstring openfile,
							  bool recent, bool frequent, bool tasks)
{
	path = pluginpath;
	s1 = pref;
	s2 = openfile;

	UINT cMinSlots;
	IObjectArray *poaRemoved;
	hr = pcdl->BeginList(&cMinSlots, IID_PPV_ARGS(&poaRemoved));
	if (SUCCEEDED(hr))
	{
		if (recent)
			pcdl->AppendKnownCategory(KDC_RECENT);

		if (frequent)
			pcdl->AppendKnownCategory(KDC_FREQUENT);
		
		if (tasks)
		hr = _AddTasksToList(pcdl);

		if (SUCCEEDED(hr))
		{
			// Commit the list-building transaction.
			hr = pcdl->CommitList();
		}
	}
	poaRemoved->Release();

	return (hr == S_OK);
}

bool JumpList::DeleteJumpList()
{
	return (pcdl->DeleteList(NULL) == S_OK);
}

HRESULT JumpList::_AddTasksToList(ICustomDestinationList *pcdl)
{
	IObjectCollection *poc;
	HRESULT hr = CoCreateInstance(CLSID_EnumerableObjectCollection, NULL, CLSCTX_INPROC, IID_PPV_ARGS(&poc));
	if (SUCCEEDED(hr))
	{
		std::wstring args;
		args = path + L",_pref@0";;

		IShellLink * psl;
		hr = _CreateShellLink(args.c_str(), s1.c_str(), &psl, 9);
		if (SUCCEEDED(hr))
		{
			hr = poc->AddObject(psl);
			psl->Release();
		}

		args = path + L",_openfile@0";

		if (SUCCEEDED(hr))
		{
			hr = _CreateShellLink(args.c_str(), s2.c_str(), &psl, 8);
			if (SUCCEEDED(hr))
			{
				hr = poc->AddObject(psl);
				psl->Release();
			}
		}

		if (SUCCEEDED(hr))
		{
			IObjectArray * poa;
			hr = poc->QueryInterface(IID_PPV_ARGS(&poa));
			if (SUCCEEDED(hr))
			{
				// Add the tasks to the Jump List. Tasks always appear in the canonical "Tasks"
				// category that is displayed at the bottom of the Jump List, after all other
				// categories.
				hr = pcdl->AddUserTasks(poa);
				poa->Release();
			}
		}
		poc->Release();
	}
	return hr;
}