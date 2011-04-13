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

    /*__forceinline*/ HBITMAP GetThumbnail(int width, int height);
    inline void ClearBackground();
    void ThumbnailPopup();

private:
    ULONG_PTR gdiplusToken;
    bool fail;

    const sSettings &m_Settings;
    MetaData &m_metadata;
    const AlbumArt &m_albumart;
    Gdiplus::Bitmap *background;

    HWND m_hwnd;
    std::vector<int> m_textpositions;
    std::vector<int> m_textpause;
    bool no_icon;
};

#endif // renderer_h__
