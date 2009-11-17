#include <string>
#include <gdiplus.h>

class AlbumArt
{
public:
	AlbumArt() {};
	~AlbumArt() {};

	bool AlbumArt::getAA(std::wstring fname, Gdiplus::Bitmap &retimg, int height);
};