#include <string>
#include <gdiplus.h>

class AlbumArt
{
public:
	AlbumArt() {};
	~AlbumArt() {};

	bool AlbumArt::getAA(std::wstring fname, std::wstring album, Gdiplus::Bitmap &retimg, int height);

private:
	std::wstring SearchDir(std::wstring path, std::wstring album);
};