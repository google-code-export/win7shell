#ifndef albumart_h__
#define albumart_h__

#include <string>
#include <gdiplus.h>
#include "api.h"

class AlbumArt
{
public:
	AlbumArt(api_memmgr* WASABI_API_MEMMGR, api_albumart* AGAVE_API_ALBUMART);
	~AlbumArt() {};

	bool AlbumArt::getAA(std::wstring fname, Gdiplus::Bitmap *retimg, 
                         int width, int height, int icon, int& size) const;

private:
    api_memmgr* m_WASABI_API_MEMMGR;
    api_albumart* m_AGAVE_API_ALBUMART;
};

#endif // albumart_h__