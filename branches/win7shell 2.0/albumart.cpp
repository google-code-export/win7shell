#include <windows.h>
#include <string>
#include <GdiPlus.h>

/*#include <tbytevector.h>
#include <mpegfile.h>
#include <attachedpictureframe.h>
#include <string>
#include <gdiplus.h>
#include <apetag.h>*/

#include "albumart.h"
#include "gen_win7shell.h"

//using namespace TagLib;
using namespace Gdiplus;

AlbumArt::AlbumArt( api_memmgr* WASABI_API_MEMMGR, api_albumart* AGAVE_API_ALBUMART ) :
m_WASABI_API_MEMMGR(WASABI_API_MEMMGR),
m_AGAVE_API_ALBUMART(AGAVE_API_ALBUMART)
{
}

bool AlbumArt::getAA(std::wstring fname, Gdiplus::Bitmap *retimg, 
                     int width, int height, int icon, int iconsize) const
{
    ARGB32* cur_image = 0;
    int cur_w = 0, cur_h = 0;

    if (m_AGAVE_API_ALBUMART && m_AGAVE_API_ALBUMART->GetAlbumArt(fname.c_str(), L"cover", 
        &cur_w, &cur_h, &cur_image) == ALBUMART_SUCCESS)
    {
        BITMAPINFO bmi;
        ZeroMemory(&bmi, sizeof bmi);
        bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bmi.bmiHeader.biWidth = cur_w;
        bmi.bmiHeader.biHeight = -cur_h;
        bmi.bmiHeader.biPlanes = 1;
        bmi.bmiHeader.biBitCount = 32;
        bmi.bmiHeader.biCompression = BI_RGB;
        bmi.bmiHeader.biSizeImage = 0;
        bmi.bmiHeader.biXPelsPerMeter = 0;
        bmi.bmiHeader.biYPelsPerMeter = 0;
        bmi.bmiHeader.biClrUsed = 0;
        bmi.bmiHeader.biClrImportant = 0;

        Bitmap tmpbmp(&bmi, cur_image);
        Graphics gfx(retimg);

        gfx.SetSmoothingMode(SmoothingModeNone);
        gfx.SetInterpolationMode(InterpolationModeHighQuality);
        gfx.SetPixelOffsetMode(PixelOffsetModeNone);
        gfx.SetCompositingQuality(CompositingQualityHighSpeed);

        if (icon >= 0)
        {            
            int iconleft = 0, icontop = 0;

            switch (icon)
            {
            case IP_LOWERLEFT:
                icontop = height - iconsize - 2;
                break;
            case IP_UPPERRIGHT:
                iconleft = width - iconsize - 2;
                break;
            case IP_LOWERRIGHT:
                iconleft = width - iconsize - 2;
                icontop = height - iconsize - 2;
                break;
            }

            // Draw icon shadow
            gfx.SetSmoothingMode(SmoothingModeAntiAlias);
            gfx.FillRectangle(&SolidBrush(Color::MakeARGB(110, 0, 0, 0)),
                static_cast<REAL>(iconleft + 1), static_cast<REAL>(icontop + 1), 
                static_cast<REAL>(iconsize + 1), static_cast<REAL>(iconsize + 1));

            // Draw icon
            gfx.SetSmoothingMode(SmoothingModeNone);
            gfx.DrawImage(&tmpbmp, 
                RectF(static_cast<REAL>(iconleft), static_cast<REAL>(icontop), 
                static_cast<REAL>(iconsize), static_cast<REAL>(iconsize)));
        }
        else
        {
            float ratio = (float)cur_h / (float)(height);
            float widthD = (float)width;
            if (ratio == 0)
                ratio = 1;
            ratio = (float)cur_w / ratio;
            if (ratio > widthD)
                ratio = widthD;
            gfx.DrawImage(&tmpbmp, RectF(((widthD-ratio)/2.0f), 0, ratio, (float)height));
        }

        if (cur_image) 
            WASABI_API_MEMMGR->sysFree(cur_image);

        return true;
    }

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
