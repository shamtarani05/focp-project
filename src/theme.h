#ifndef THEME_H
#define THEME_H

#include <windows.h>

namespace Theme {
    // Window
    const int WIN_W = 1050;
    const int WIN_H = 700;
    const int HEADER_H = 60;
    const int SIDEBAR_W = 220;
    const int STATUS_H = 30;

    // Colors
    const COLORREF Primary     = RGB(20, 30, 65);
    const COLORREF PrimaryLight= RGB(30, 50, 100);
    const COLORREF Secondary   = RGB(33, 150, 243);
    const COLORREF Accent      = RGB(255, 193, 7);
    const COLORREF Bg          = RGB(235, 240, 248);
    const COLORREF Card        = RGB(255, 255, 255);
    const COLORREF CardBorder  = RGB(210, 220, 235);
    const COLORREF Text        = RGB(40, 40, 50);
    const COLORREF TextLight   = RGB(140, 150, 165);
    const COLORREF TextWhite   = RGB(255, 255, 255);
    const COLORREF Success     = RGB(46, 160, 67);
    const COLORREF Error       = RGB(220, 53, 69);
    const COLORREF Warning     = RGB(255, 165, 0);
    const COLORREF SidebarBg   = RGB(16, 24, 48);
    const COLORREF SidebarSel  = RGB(33, 150, 243);
    const COLORREF InputBg     = RGB(248, 250, 252);
    const COLORREF InputBorder = RGB(180, 195, 215);
    const COLORREF Shadow      = RGB(190, 200, 215);
    const COLORREF Hover       = RGB(40, 80, 160);

    // Fonts
    const char* FontName      = "Segoe UI";
    const char* FontNameBold  = "Segoe UI";
    const char* FontNameMono  = "Consolas";
}

#endif
