#ifndef renderer_h__
#define renderer_h__

#include <windows.h>
#include <GdiPlus.h>
#include <vector>
#include "gen_win7shell.h"
#include "metadata.h"
#include "albumart.h"

class renderer
{
public:
    renderer(const sSettings& Settings, MetaData &metadata, const AlbumArt &AA, HWND WinampWnd);
    ~renderer();

    HBITMAP GetThumbnail();
    inline void ClearBackground();
    void ClearCustomBackground();
    void ThumbnailPopup();

    inline void SetDimensions(int new_w, int new_h) 
    {
        width = new_w;
        height = new_h;
    }

private:
    ULONG_PTR gdiplusToken;
    Gdiplus::Image *custom_img;
    Gdiplus::Bitmap *background;

    const sSettings &m_Settings;
    MetaData &m_metadata;
    const AlbumArt &m_albumart;    

    int width, height, m_iconwidth, m_iconheight;
    int m_textpause;
    HWND m_hwnd;
    std::vector<int> m_textpositions;
    bool no_icon;
    bool fail;
    bool scroll_block;
};

#endif // renderer_h__
