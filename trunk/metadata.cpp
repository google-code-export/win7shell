#include <Windows.h>
#include <string>
#include <map>
#include <ctype.h>
#include "metadata.h"
#include "sdk/winamp/wa_ipc.h"

using namespace std;

MetaData::MetaData() : 
isFile(false), 
mfilename(L""), 
m_play_count(0)
{
}

void MetaData::setWinampWindow(HWND winampwindow)
{
    mhwnd = winampwindow;
}

bool MetaData::reset(std::wstring filename, bool force)
{
    if (force)
    {
        cache.clear();
        return true;
    }

    if (filename != mfilename)
    {
        cache.clear();
        mfilename = filename;
        return true;
    }	
    else
        return false;
}

std::wstring MetaData::getMetadata(std::wstring tag)
{
    if (mfilename.empty())
        return L"";

    if (!cache.empty() && cache.find(tag) != cache.end())
        return cache.find(tag)->second;

    /*wchar_t a[32];
    wsprintf(a,L"%s %d",tag.c_str(),cache.size());
    MessageBox(0,mfilename.c_str(),a,0);*/

    std::wstring ret;
    ret.reserve(512);
    ret.resize(512,0);

    extendedFileInfoStructW efs = {0};
    efs.filename= mfilename.c_str();
    efs.metadata=tag.c_str();
    efs.ret=&ret[0];	
    efs.retlen = 512;
    SendMessage(mhwnd,WM_WA_IPC,(WPARAM)&efs,IPC_GET_EXTENDED_FILE_INFOW);
    ret.resize(wcslen(&ret[0]));

    if (ret.empty())
    {
        if (tag == L"title" || tag == L"artist") 
        {
            ret = (wchar_t*) SendMessage(mhwnd,WM_WA_IPC,0,IPC_GET_PLAYING_TITLE);
            if (ret.length() == 0)
                return L"";

            size_t pos = ret.find_first_of('-');
            if (pos != std::wstring::npos && pos != 0)							
            {		
                if (tag == L"title")
                {
                    ret = std::wstring(ret, pos+1, ret.length()-(pos-2));
                    if (ret[0] == L' ')
                        ret.erase(0, 1);
                }
                else if (tag == L"artist")
                {
                    ret = std::wstring(ret, 0, pos);
                    pos--;
                    if (ret[pos] == L' ')
                        ret.erase(pos, 1);
                }
            }
            else
                if (tag == L"artist")
                    return L"";
        }
        else
            return L"";
    }

    if (tag == L"year")
    {
        for (std::wstring::size_type i=0; i<ret.length(); i++)
            if (!isdigit(ret[i]))
                return L"";
    }

    cache[tag] = ret;
    return ret;
}

std::wstring MetaData::getFileName() const
{
    return mfilename;
}

bool MetaData::CheckPlayCount()
{
    if (m_play_count > 50)
    {
        m_play_count = 0;
        return true;
    }
    else
    {
        ++m_play_count;
        return false;
    }

    return false;
}
