#ifndef albumart_h__
#define albumart_h__

#include <string>
#include <gdiplus.h>
#include "api.h"
#include "gen_win7shell.h"

class AlbumArt
{
public:
	AlbumArt(api_memmgr* WASABI_API_MEMMGR, api_albumart* AGAVE_API_ALBUMART, const sSettings &settings);
	~AlbumArt() {};

	bool AlbumArt::getAA(std::wstring fname, Gdiplus::Bitmap *retimg, 
                         int width, int height, int& size) const;

private:
    api_memmgr* m_WASABI_API_MEMMGR;
    api_albumart* m_AGAVE_API_ALBUMART;
    const sSettings &m_settings;
};

#endif // albumart_h__