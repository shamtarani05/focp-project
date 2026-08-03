#ifndef THEME_H
#define THEME_H

#include <windows.h>

namespace Theme {
    // Window
    const int WIN_W = 1100;
    const int WIN_H = 720;
    const int HEADER_H = 65;
    const int SIDEBAR_W = 230;
    const int STATUS_H = 32;

    // Colors
    const COLORREF Primary     = RGB(15, 23, 42);     // #0F172A Dark Slate/Navy
    const COLORREF PrimaryLight= RGB(30, 41, 59);     // #1E293B Slate Navy
    const COLORREF Secondary   = RGB(99, 102, 241);   // #6366F1 Indigo Accent
    const COLORREF Accent      = RGB(16, 185, 129);   // #10B981 Emerald Green Accent
    const COLORREF Bg          = RGB(248, 250, 252);  // #F8FAFC Clean Canvas
    const COLORREF Card        = RGB(255, 255, 255);  // Pure White Card
    const COLORREF CardBorder  = RGB(226, 232, 240);  // #E2E8F0 Border
    const COLORREF Text        = RGB(15, 23, 42);     // #0F172A Text Dark
    const COLORREF TextLight   = RGB(100, 116, 139);  // #64748B Text Muted
    const COLORREF TextWhite   = RGB(255, 255, 255);
    const COLORREF Success     = RGB(16, 185, 129);   // #10B981 Success
    const COLORREF Error       = RGB(239, 68, 68);    // #EF4444 Error
    const COLORREF Warning     = RGB(245, 158, 11);   // #F59E0B Warning
    const COLORREF SidebarBg   = RGB(15, 23, 42);     // #0F172A Sidebar
    const COLORREF SidebarSel  = RGB(99, 102, 241);   // #6366F1 Active item
    const COLORREF InputBg     = RGB(255, 255, 255);  // White input
    const COLORREF InputBorder = RGB(203, 213, 225);  // #CBD5E1
    const COLORREF Shadow      = RGB(226, 232, 240);
    const COLORREF Hover       = RGB(30, 41, 59);

    // Fonts
    const char* FontName      = "Segoe UI";
    const char* FontNameBold  = "Segoe UI";
    const char* FontNameMono  = "Consolas";
}

#endif
