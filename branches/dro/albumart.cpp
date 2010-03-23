#include <windows.h>

/*#include <tbytevector.h>
#include <mpegfile.h>
#include <attachedpictureframe.h>
#include <string>
#include <gdiplus.h>
#include <apetag.h>*/

#include "albumart.h"

//using namespace TagLib;

bool AlbumArt::getAA(std::wstring fname, Gdiplus::Bitmap &retimg, int height)
{
	/*MPEG::File f(fname.c_str());

	if (f.isOpen())
	{
		APE::Tag *ape = f.APETag();

		if (ape)
		{
			for(APE::ItemListMap::ConstIterator it = ape->itemListMap().begin();
				it != ape->itemListMap().end(); it++)
			{
				int type = (*it).second.type();	
				if (type == 1)
				{
					HGLOBAL hBuffer = ::GlobalAlloc(GMEM_MOVEABLE, (*it).second.value().size());

					if(hBuffer)
					{
						void* pBuffer = ::GlobalLock(hBuffer);
						if(pBuffer)
						{
							int size = (*it).second.value().size();
							std::vector<char> data(size);							
							
							memcpy(&data[0], (*it).second.value().data(), size);
							for (std::vector<char>::iterator it = data.begin();
								it != data.end(); it++)							
								if ((*it) == 0)
								{
									data.erase(data.begin(), it+1);
									break;
								}							

							CopyMemory(pBuffer, &data[0], data.size());
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
		}
	}*/
	
	return false;
}