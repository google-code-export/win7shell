#include <windows.h>

#include <tbytevector.h>
#include <mpegfile.h>
#include <id3v2tag.h>
#include <id3v2frame.h>
#include <id3v2header.h>
#include <attachedpictureframe.h>
#include <string>
#include <gdiplus.h>

#include "albumart.h"

using namespace TagLib;

bool AlbumArt::getAA(std::wstring fname, std::wstring album, Gdiplus::Bitmap &retimg, int height)
{
	MPEG::File f(fname.c_str());

	if (f.isOpen())
	{
		ID3v2::Tag *id3v2tag = f.ID3v2Tag();

		TagLib::ID3v2::FrameList l = f.ID3v2Tag()->frameListMap()["APIC"];

		if (!l.isEmpty())
		{
			TagLib::ID3v2::AttachedPictureFrame *picture = static_cast<TagLib::ID3v2::AttachedPictureFrame*>(l.front());
			TagLib::ByteVector pictureData  = picture->picture();

			HGLOBAL hBuffer = ::GlobalAlloc(GMEM_MOVEABLE, pictureData.size());

			if(hBuffer)
			{
				void* pBuffer = ::GlobalLock(hBuffer);
				if(pBuffer)
				{
					CopyMemory(pBuffer, pictureData.data(), pictureData.size());
					IStream* pStream = NULL;

					if(::CreateStreamOnHGlobal(hBuffer,FALSE,&pStream) == S_OK)
					{
						Gdiplus::Image img(pStream);

						pStream->Release();
						if(img.GetLastStatus() == 0)
						{
							Gdiplus::Graphics gfx(&retimg);
							gfx.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
							gfx.SetSmoothingMode(Gdiplus::SmoothingModeNone);
							gfx.DrawImage(&img, 0, 0, height, height);

							::GlobalUnlock(hBuffer);
							::GlobalFree(hBuffer);

							return true;
						}
					}
					::GlobalUnlock(hBuffer);
				}
				::GlobalFree(hBuffer);    
			}
		}
	}

	
	std::wstring name = fname;
	std::wstring::size_type pos = name.find_last_of('\\');	
	if (pos == std::wstring::npos)
		return false;

	name = SearchDir(name.substr(0, pos), album);
	if (name.empty())
		return false;

	Gdiplus::Image img(name.c_str());
	if (img.GetLastStatus() != 0)
		return false;

	Gdiplus::Graphics gfx(&retimg);	
	
	gfx.DrawImage(&img, 0, 0, height, height);

	return true;
}

std::wstring AlbumArt::SearchDir(std::wstring path, std::wstring album)
{
	WIN32_FIND_DATA ffd;

	std::wstring defpath = path + L"\\";
	defpath = album;
	if (!defpath.empty())
	{
		defpath = path + L"\\" + defpath + L".jpg";
		if ((FindFirstFile(defpath.c_str() , &ffd) != INVALID_HANDLE_VALUE))
			return defpath.c_str();
	}

	defpath = path + L"\\folder.jpg";
	if ((FindFirstFile(defpath.c_str() , &ffd) != INVALID_HANDLE_VALUE))
		return path + L"\\folder.jpg";

	defpath = path + L"\\folder.gif";
	if ((FindFirstFile(defpath.c_str() , &ffd) != INVALID_HANDLE_VALUE))
		return path + L"\\folder.gif";

	defpath = path + L"\\folder.png";
	if ((FindFirstFile(defpath.c_str() , &ffd) != INVALID_HANDLE_VALUE))
		return path + L"\\folder.png";

	defpath = path + L"\\cover.jpg";
	if ((FindFirstFile(defpath.c_str() , &ffd) != INVALID_HANDLE_VALUE))
		return path + L"\\cover.jpg";

	defpath = path + L"\\front.jpg";
	if ((FindFirstFile(defpath.c_str() , &ffd) != INVALID_HANDLE_VALUE))
		return path + L"\\front.jpg";	

	defpath = path + L"\\cover.gif";
	if ((FindFirstFile(defpath.c_str() , &ffd) != INVALID_HANDLE_VALUE))
		return path + L"\\cover.gif";

	defpath = path + L"\\front.gif";
	if ((FindFirstFile(defpath.c_str() , &ffd) != INVALID_HANDLE_VALUE))
		return path + L"\\front.gif";

	defpath = path + L"\\cover.png";
	if ((FindFirstFile(defpath.c_str() , &ffd) != INVALID_HANDLE_VALUE))
		return path + L"\\cover.png";

	defpath = path + L"\\front.png";
	if ((FindFirstFile(defpath.c_str() , &ffd) != INVALID_HANDLE_VALUE))
		return path + L"\\front.png";

	defpath = path + L"\\AlbumArtSmall.jpg";
	if ((FindFirstFile(defpath.c_str() , &ffd) != INVALID_HANDLE_VALUE))
		return path + L"\\AlbumArtSmall.jpg";

	defpath = path + L"\\AlbumArt*";
	if ((FindFirstFile(defpath.c_str() , &ffd) != INVALID_HANDLE_VALUE))
		return path + L"\\" + ffd.cFileName;

	return L"";
}