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

AlbumArt::AlbumArt( api_memmgr* WASABI_API_MEMMGR, api_albumart* AGAVE_API_ALBUMART, const sSettings &settings ) :
m_WASABI_API_MEMMGR(WASABI_API_MEMMGR),
m_AGAVE_API_ALBUMART(AGAVE_API_ALBUMART),
m_settings(settings)
{
}

bool AlbumArt::getAA(std::wstring fname, Gdiplus::Bitmap *retimg, 
                     int width, int height, int& iconsize) const
{
    ARGB32* cur_image = 0;
    int cur_w = 0, cur_h = 0;

    if (m_AGAVE_API_ALBUMART && m_AGAVE_API_ALBUMART->GetAlbumArt(fname.c_str(), L"cover", 
        &cur_w, &cur_h, &cur_image) == ALBUMART_SUCCESS)
    {
        BITMAPINFO bmi = {};
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

        //Calculate Alpha blend based on Transparency
        float fBlend = (100-m_settings.BG_Transparency)/100.0;

        ColorMatrix BitmapMatrix =	{     
            1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 0.0f, fBlend, 0.0f,
            0.0f, 0.0f, 0.0f, 0.0f, 1.0f
        };

        gfx.SetSmoothingMode(SmoothingModeNone);
        gfx.SetInterpolationMode(InterpolationModeHighQuality);
        gfx.SetPixelOffsetMode(PixelOffsetModeNone);
        gfx.SetCompositingQuality(CompositingQualityHighSpeed);
       
        int iconleft = 0, icontop = 0;

        switch (m_settings.IconPosition)
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

        float new_height = 0;
        float new_width = 0;
        float anchor = height;

        if (!m_settings.AsIcon)
        {
            if (width < height)
            {
                anchor = width;
            }
        }
        else
        {
            anchor = iconsize;
        }

        if (cur_w > cur_h)
        {
            new_height = (float)cur_h / (float)cur_w * (float)anchor;
            new_height -= 2;
            new_width = anchor;

            if (new_height > height)
            {
                new_width = (float)cur_w / (float)cur_h * (float)anchor;
                new_width -= 2;
                new_height = anchor;
            }
        }
        else
        {
            new_width = (float)cur_w / (float)cur_h * (float)anchor;
            new_width -= 2;
            new_height = anchor;

            if (new_width > width)
            {
                new_height = (float)cur_h / (float)cur_w * (float)anchor;
                new_height -= 2;
                new_width = anchor;
            }
        }

        iconsize = new_width;

        // Draw icon shadow
        if (!m_settings.AsIcon)
        {
            iconleft = (width / 2) - (new_width / 2);            
        }        
        else
        {
            gfx.SetSmoothingMode(SmoothingModeAntiAlias);
            gfx.FillRectangle(&SolidBrush(Color::MakeARGB(110 - m_settings.BG_Transparency, 0, 0, 0)),
                static_cast<REAL>(iconleft + 1), static_cast<REAL>(icontop + 1), 
                static_cast<REAL>(new_width + 1), static_cast<REAL>(new_height + 1));
        }

        // Draw icon
        gfx.SetSmoothingMode(SmoothingModeNone);

        if (m_settings.BG_Transparency == 0)
        {
            gfx.DrawImage(&tmpbmp, 
                RectF(static_cast<REAL>(iconleft), static_cast<REAL>(icontop), 
                static_cast<REAL>(new_width), static_cast<REAL>(new_height)));
        }
        else
        {
            ImageAttributes ImgAttr;
            ImgAttr.SetColorMatrix(&BitmapMatrix, 
                ColorMatrixFlagsDefault, 
                ColorAdjustTypeBitmap);

            gfx.DrawImage(&tmpbmp, 
                RectF(static_cast<REAL>(iconleft), static_cast<REAL>(icontop), 
                static_cast<REAL>(new_width), static_cast<REAL>(new_height)),
                0, 0, cur_w, cur_h,
                UnitPixel, &ImgAttr);
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
