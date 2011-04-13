#include <Windows.h>
#include <string>
#include <vector>
#include <gdiplus.h>
#include "renderer.h"
#include "gen_win7shell.h"
#include "albumart.h"
#include "lines.h"

using namespace Gdiplus;

renderer::renderer(const sSettings& m_Settings, MetaData &metadata, const AlbumArt &AA, HWND WinampWnd) : 
m_Settings(m_Settings),
m_metadata(metadata),
m_albumart(AA),
background(NULL),
fail(false),
no_icon(false),
m_hwnd(WinampWnd)
{
    GdiplusStartupInput gdiplusStartupInput;   
    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
}

renderer::~renderer()
{
    ClearBackground();
    GdiplusShutdown(gdiplusToken);
}

HBITMAP renderer::GetThumbnail(int width, int height)
{
    //Calculate icon size
    int iconsize = height;

    if (m_Settings.AsIcon)
    {
        if (width < iconsize)
            iconsize = width;
        iconsize -= 2;
        iconsize = (m_Settings.IconSize * iconsize) / 100;
    }

    // Draw background if not valid
    if (!background)
    {      
        no_icon = false;
        bool tempfail = fail;
        fail = false;
        background = new Bitmap(width, height, PixelFormat32bppPARGB);

        switch (tempfail ? m_Settings.Revertto : m_Settings.Thumbnailbackground)
	    {
	    case BG_TRANSPARENT:
	        no_icon = true;
	        break;
	
	    case BG_ALBUMART:
	        {
               // get album art
	           if (!m_albumart.getAA(m_metadata.getFileName(), background, 
                   width, height, m_Settings.AsIcon ? m_Settings.IconPosition : -1, iconsize) &&
                        m_Settings.Revertto != BG_ALBUMART)
	           {
                   // fallback
	               fail = true;
                   ClearBackground();
                   GetThumbnail(width, height);
	           }
	        }
	        break;

        case BG_CUSTOM:
            if (!m_Settings.BGPath.empty())
            {
                Image img(m_Settings.BGPath.c_str(), true);
                if (img.GetType() != 0)
                {
                    Graphics gfx(background);
                    gfx.SetInterpolationMode(InterpolationModeHighQualityBilinear);
                    if (m_Settings.AsIcon)
                    {
                        int iconleft = 0, icontop = 0;
                        
                        switch (m_Settings.IconPosition)
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
                        gfx.DrawImage(&img, 
                            RectF(static_cast<REAL>(iconleft), static_cast<REAL>(icontop), 
                            static_cast<REAL>(iconsize), static_cast<REAL>(iconsize)));
                    }
                    else
                    {
                        gfx.SetSmoothingMode(SmoothingModeNone);
                        gfx.DrawImage(&img, RectF(0, 0, static_cast<REAL>(width), static_cast<REAL>(height)));
                    }
                }
                else if (m_Settings.Revertto != BG_CUSTOM)
                {		
                    fail = true;
                    ClearBackground();
                    GetThumbnail(width, height);
                }				
            }
            else if (m_Settings.Revertto != BG_CUSTOM)
            {		
                fail = true;
                ClearBackground();
                GetThumbnail(width, height);
            }
            break;	

        default:
            {
                fail = true;
                ClearBackground();
                GetThumbnail(width, height);                
            }
            break;
	    }
    }

    Bitmap *canvas = background->Clone(0, 0, background->GetWidth(), background->GetHeight(), PixelFormat32bppPARGB);
    Graphics gfx(canvas);     

    if (m_Settings.Text.empty())
    {
        Pen p(Color::MakeARGB(1, 255, 255, 255), 0.1);
        gfx.DrawLine(&p, 0, 0, 1, 1);
    }
    else
    {
	    // Draw text
	    static lines text_parser(m_Settings, m_metadata, m_hwnd);
	    text_parser.Parse();

        if (m_textpositions.empty())
        {
            m_textpositions.resize(text_parser.GetNumberOfLines(), 0);
            m_textpause.resize(text_parser.GetNumberOfLines(), 0);
        }
        else
        {
            for (int i = 0; i != m_textpositions.size(); ++i)
            {
                if (m_textpause[i] == 0)
                {
                    m_textpositions[i] -= 2;
                }
            }
        }
	
	    // Setup fonts
	    REAL textheight = 0;
	    HDC h_gfx = gfx.GetHDC();
	    
	    Font font(h_gfx, &m_Settings.font);
	
	    LOGFONT large_font;
	    ZeroMemory(&large_font, sizeof(large_font));
	    memcpy(&large_font, &m_Settings.font, sizeof(m_Settings.font));
	
	    LONG large_size = -((m_Settings.font.lfHeight * 72) / GetDeviceCaps(h_gfx, LOGPIXELSY));
	    large_size += 4;
	    large_font.lfHeight = -MulDiv(large_size, GetDeviceCaps(h_gfx, LOGPIXELSY), 72);
	    Font largefont(h_gfx, &large_font);
	
	    gfx.ReleaseHDC(h_gfx);
	
	    SolidBrush bgcolor(Color(GetRValue(m_Settings.bgcolor), GetGValue(m_Settings.bgcolor), GetBValue(m_Settings.bgcolor)));
	    SolidBrush fgcolor(Color(GetRValue(m_Settings.text_color), GetGValue(m_Settings.text_color), GetBValue(m_Settings.text_color)));
		
	    StringFormat sf(StringFormatFlagsNoWrap);
        const int text_space = 28;
	
	    for (std::size_t text_index = 0; text_index != text_parser.GetNumberOfLines(); ++text_index)
	    {
	        RectF ret_rect;
	        std::wstring current_text = text_parser.GetLineText(text_index);
            linesettings current_settings = text_parser.GetLineSettings(text_index);
	     
	        // Measure size
	        gfx.SetTextRenderingHint(TextRenderingHintAntiAlias);
	        if (current_settings.largefont)
	            gfx.MeasureString(current_text.c_str(), -1, &largefont, RectF(0, 0, 2000, 1000), &sf, &ret_rect);
	        else
	            gfx.MeasureString(current_text.c_str(), -1, &font, RectF(0, 0, 2000, 1000), &sf, &ret_rect);
	
	        if (ret_rect.GetBottom() == 0)
	        {
	            if (current_text.empty())
	            {
	                gfx.MeasureString(L"QWEXCyjM", -1, &font, RectF(0, 0, static_cast<REAL>(width), static_cast<REAL>(height)), &sf, &ret_rect);			
	            }
	            else
	            {
	                gfx.MeasureString(L"QWEXCyjM", -1, &Font(L"Segoe UI", 14), RectF(0, 0, static_cast<REAL>(width), static_cast<REAL>(height)), &sf, &ret_rect);			
	            }
	        }
	
		    Bitmap text_bitmap(ret_rect.GetRight(), ret_rect.GetBottom() - 1, PixelFormat32bppPARGB);
		    Graphics text_gfx(&text_bitmap);
	
            // Graphics setup
	        text_gfx.SetSmoothingMode(SmoothingModeHighSpeed);
	        text_gfx.SetInterpolationMode(InterpolationModeNearestNeighbor);
	        text_gfx.SetPixelOffsetMode(PixelOffsetModeNone);
	        text_gfx.SetCompositingQuality(CompositingQualityHighQuality);
	        
            (m_Settings.Antialias) ? text_gfx.SetTextRenderingHint(TextRenderingHintAntiAliasGridFit) : 
	                                 text_gfx.SetTextRenderingHint(TextRenderingHintSingleBitPerPixelGridFit);

            // Draw box if needed
            if (current_settings.darkbox)
            {
	            SolidBrush boxbrush(Color::MakeARGB(120, GetRValue(m_Settings.bgcolor),
	                GetGValue(m_Settings.bgcolor), GetBValue(m_Settings.bgcolor)));
	
	            //ret_rect.Y += 1;
	            ret_rect.Height += 2;
	            text_gfx.FillRectangle(&boxbrush, ret_rect);
            }

            text_gfx.SetTextContrast(120);

	        // Draw text to offscreen surface
            text_gfx.DrawString(current_text.c_str(), -1, 
                current_settings.largefont ? &largefont : &font, 
                PointF(2, -1), &bgcolor);
		    
		    text_gfx.DrawString(current_text.c_str(), -1, 
	            current_settings.largefont ? &largefont : &font, 
	            PointF(0, -2), &fgcolor);

            // Calculate text position
            int X = 0, CX = width;

            if (m_Settings.AsIcon && !no_icon)
            {
                if ( (m_Settings.IconPosition == IP_UPPERLEFT || 
                    m_Settings.IconPosition == IP_LOWERLEFT) &&
                    !current_settings.forceleft )
                {
                    X += iconsize + 5;
                    CX = width - X;
                }
                else if ( m_Settings.IconPosition == IP_UPPERRIGHT || 
                    m_Settings.IconPosition == IP_LOWERRIGHT )
                {
                    CX = width - iconsize - 5;
                }
            }

            gfx.SetClip(RectF(X, 0, CX, width), CombineModeReplace);

            // Draw text bitmap to final bitmap
            if (text_bitmap.GetWidth() > CX + 2 && !current_settings.dontscroll)// && m_textpause[text_index] == 0)
            {
                // Draw scrolling text
                int left = m_textpositions[text_index];
                int bmp_width = (int)text_bitmap.GetWidth();
                int bmp_height = (int)text_bitmap.GetHeight();

                if (left + bmp_width < 0)
                {
                    // reset text
                    m_textpositions[text_index] = text_space;
                    left = text_space;
                }   

                if (left == 0 && m_textpause[text_index] == 0)
                {
                    m_textpause[text_index] = 60; // delay; in steps
                }

                if (left + bmp_width >= CX)
                {
                    gfx.DrawImage(&text_bitmap, X, (int)textheight, -left, 0, CX, bmp_height, UnitPixel);
                }
                else
                {
                    gfx.DrawImage(&text_bitmap, X, (int)textheight, -left, 0, bmp_width + left,
                                  bmp_height, UnitPixel);

                    gfx.DrawImage(&text_bitmap, X + text_space + 2 + bmp_width + left, (int)textheight, 0, 0,
                        -left, bmp_height, UnitPixel);
                }
            }
            else
            {
                // Draw non-scrolling text
                if (current_settings.center)
                {
                    // Center text
                    int newleft = X + ((CX / 2) - (text_bitmap.GetWidth() / 2));
                    gfx.DrawImage(&text_bitmap, newleft, (int)textheight, 0, 0, text_bitmap.GetWidth(), text_bitmap.GetHeight(), UnitPixel);          
                }
                else
                {
                    gfx.DrawImage(&text_bitmap, X, (int)textheight, 0, 0, text_bitmap.GetWidth(), text_bitmap.GetHeight(), UnitPixel);          
                }
            }

            gfx.ResetClip();

            if (m_textpause[text_index] > 0)
            {
                --m_textpause[text_index];
            }
	
	        textheight += text_bitmap.GetHeight();
	    }

	    if (!m_Settings.Shrinkframe)	
	        textheight = height-2;
	
	    if (m_Settings.Thumbnailpb)
	        textheight += 25;
	
	    if (textheight > height-2)
	        textheight = height-2;
	}

    // Draw progressbar

    if (m_Settings.Thumbnailpb && m_Settings.play_total > 0)// && m_Settings.play_current >0)
    {
        gfx.SetSmoothingMode(SmoothingModeAntiAlias);		
        int Y = canvas->GetHeight()-15;
        Pen p1(Color::White, 1);
        Pen p2(Color::MakeARGB(80, 0, 0, 0), 1);

        SolidBrush b1(Color::MakeARGB(150, 0, 0, 0));	
        SolidBrush b2(Color::MakeARGB(220, 255, 255, 255));

        RectF R1(44, (REAL)Y, 10, 9);
        RectF R2((REAL)width - 56, (REAL)Y, 10, 9);

        gfx.FillRectangle(&b1, 46, Y, width-94, 9);
        gfx.DrawLine(&p2, 46, Y+2, 46, Y+6);
        gfx.DrawLine(&p2, width-48, Y+2, width-48, Y+6);

        gfx.DrawArc(&p1, R1, -90.0f, -180.0f);
        gfx.DrawArc(&p1, R2, 90.0f, -180.0f);

        gfx.DrawLine(&p1, 48, Y, width-49, Y);
        gfx.DrawLine(&p1, 48, Y+9, width-49, Y+9);

        gfx.SetSmoothingMode(SmoothingModeDefault);
        gfx.FillRectangle(&b2, 48, Y+3, (m_Settings.play_current*(width-96))/m_Settings.play_total, 4);
    }

    // finalize / garbage
    

    HBITMAP retbmp;    
    canvas->GetHBITMAP(NULL, &retbmp);
    delete canvas;
    return retbmp;
}

void renderer::ClearBackground()
{
    if (background)
    {
        delete background;
        background = NULL;
    }
}

void renderer::ThumbnailPopup()
{
    m_textpositions.clear();
    m_textpause.clear();
}