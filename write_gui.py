#!/usr/bin/env python3
"""Generate gui.cpp - the complete Win32 GUI for National Bank"""
import os

code = r'''#include "gui.h"
#include "theme.h"
#include "banking.h"
#include "core/fileio.h"
#include "core/validation.h"
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cmath>

using namespace Theme;

// --- Control IDs ---
enum CtrlID {
    BTN_SIDEBAR = 9000,
    BTN_LOGOUT = 9100,
    BTN_HEADER = 9200,
    ID_EDIT_BASE = 10000,
    BTN_BASE = 11000,
    ID_LABEL_BASE = 12000,
    ID_TABLE_BASE = 13000,
};

// --- Control ID ranges per screen ---
enum EditID {
    EDT_ACCNO = ID_EDIT_BASE,
    EDT_PIN, EDT_NAME, EDT_CNIC, EDT_TYPE, EDT_BALANCE,
    EDT_NEWPIN, EDT_CONFIRMPIN,
    EDT_AMT, EDT_TARGET, EDT_SEARCH,
    EDT_SEARCH_TXN_ACC, EDT_SEARCH_TXN_TYPE,
    EDT_SEARCH_TXN_FROM, EDT_SEARCH_TXN_TO,
    EDT_SEARCH_TXN_MIN, EDT_SEARCH_TXN_MAX,
    EDT_LOAN_AMT, EDT_LOAN_TERM, EDT_LOAN_ACC,
    EDT_TRANSFER_TARGET, EDT_TRANSFER_AMT,
    EDT_OTP,
};

enum BtnID {
    BTN_ADMIN_CREATE = BTN_BASE,
    BTN_ADMIN_VIEW, BTN_ADMIN_SEARCH, BTN_ADMIN_FREEZE,
    BTN_ADMIN_UNFREEZE, BTN_ADMIN_UNLOCK, BTN_ADMIN_TXNS,
    BTN_ADMIN_SEARCH_TXN, BTN_ADMIN_AUDIT, BTN_ADMIN_LOANS,
    BTN_ADMIN_CASH, BTN_ADMIN_DAILY,
    BTN_ATM_DEPOSIT, BTN_ATM_WITHDRAW, BTN_ATM_TRANSFER,
    BTN_ATM_BALANCE, BTN_ATM_MINISTATE, BTN_ATM_CHANGEPIN,
    BTN_ATM_INFO, BTN_BACK,
    BTN_DO_CREATE, BTN_DO_LOGIN, BTN_DO_ADMIN_LOGIN,
    BTN_DO_SEARCH, BTN_DO_FREEZE, BTN_DO_UNFREEZE, BTN_DO_UNLOCK,
    BTN_DO_DEPOSIT, BTN_DO_WITHDRAW, BTN_DO_TRANSFER, BTN_DO_CHANGEPIN,
    BTN_DO_SEARCH_TXN, BTN_DO_LOAN, BTN_DO_OTP,
    BTN_DO_BACKUP,
    BTN_CLEAR_CREATE, BTN_CLEAR_SEARCH,
};

// --- GDI handles ---
static HFONT hFontTitle, hFontHeading, hFontNormal, hFontSmall, hFontBold, hFontMono;
static HBRUSH hBrushPrimary, hBrushCard, hBrushBg, hBrushSidebar, hBrushAccent;
static HPEN hPenCardBorder, hPenShadow;
static HBRUSH hBrushHover;
static bool gdiInit = false;

static void InitGDI() {
    if (gdiInit) return;
    gdiInit = true;
    hFontTitle   = CreateFont(28,0,0,0,FW_BOLD,0,0,0,0,0,0,0,0,"Segoe UI");
    hFontHeading = CreateFont(20,0,0,0,FW_BOLD,0,0,0,0,0,0,0,0,"Segoe UI");
    hFontBold    = CreateFont(16,0,0,0,FW_BOLD,0,0,0,0,0,0,0,0,"Segoe UI");
    hFontNormal  = CreateFont(15,0,0,0,0,0,0,0,0,0,0,0,0,"Segoe UI");
    hFontSmall   = CreateFont(13,0,0,0,0,0,0,0,0,0,0,0,0,"Segoe UI");
    hFontMono    = CreateFont(14,0,0,0,0,0,0,0,0,0,0,0,0,"Consolas");

    hBrushPrimary  = CreateSolidBrush(Primary);
    hBrushCard     = CreateSolidBrush(Card);
    hBrushBg       = CreateSolidBrush(Bg);
    hBrushSidebar  = CreateSolidBrush(SidebarBg);
    hBrushAccent   = CreateSolidBrush(Accent);
    hBrushHover    = CreateSolidBrush(Hover);
    hPenCardBorder = CreatePen(PS_SOLID, 1, CardBorder);
    hPenShadow     = CreatePen(PS_SOLID, 1, Shadow);
}

static void DestroyFonts() {
    DeleteObject(hFontTitle); DeleteObject(hFontHeading);
    DeleteObject(hFontNormal); DeleteObject(hFontSmall);
    DeleteObject(hFontBold); DeleteObject(hFontMono);
}

// --- GDI Drawing Helpers ---
static void RoundRect2(HDC hdc, int x, int y, int w, int h, int r, HBRUSH br, HPEN pen) {
    HPEN oldPen = (HPEN)SelectObject(hdc, pen);
    HBRUSH oldBr = (HBRUSH)SelectObject(hdc, br);
    RoundRect(hdc, x, y, x+w, y+h, r, r);
    SelectObject(hdc, oldPen);
    SelectObject(hdc, oldBr);
}

static void Txt(HDC hdc, const char* s, int x, int y, HFONT f, COLORREF clr) {
    SetTextColor(hdc, clr);
    SetBkMode(hdc, TRANSPARENT);
    HFONT old = (HFONT)SelectObject(hdc, f);
    TextOut(hdc, x, y, s, (int)strlen(s));
    SelectObject(hdc, old);
}

static void TxtCenter(HDC hdc, const char* s, int x, int y, int w, int h, HFONT f, COLORREF clr) {
    SetTextColor(hdc, clr);
    SetBkMode(hdc, TRANSPARENT);
    HFONT old = (HFONT)SelectObject(hdc, f);
    RECT rc = {x, y, x+w, y+h};
    DrawText(hdc, s, -1, &rc, DT_CENTER|DT_VCENTER|DT_SINGLELINE);
    SelectObject(hdc, old);
}

static void GradientHeader(HDC hdc, RECT rc) {
    for (int i = 0; i < HEADER_H; i++) {
        int r = GetRValue(Primary) + (GetRValue(PrimaryLight)-GetRValue(Primary))*i/HEADER_H;
        int g = GetGValue(Primary) + (GetGValue(PrimaryLight)-GetGValue(Primary))*i/HEADER_H;
        int b = GetBValue(Primary) + (GetBValue(PrimaryLight)-GetBValue(Primary))*i/HEADER_H;
        HPEN ln = CreatePen(PS_SOLID, 1, RGB(r,g,b));
        HPEN old = (HPEN)SelectObject(hdc, ln);
        MoveToEx(hdc, 0, i, NULL);
        LineTo(hdc, rc.right, i);
        SelectObject(hdc, old);
        DeleteObject(ln);
    }
}

static void DrawShadow(HDC hdc, int x, int y, int w, int h) {
    HPEN old = (HPEN)SelectObject(hdc, hPenShadow);
    HBRUSH oldBr = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));
    for (int i = 1; i <= 3; i++) {
        RECT r = {x+i, y+i, x+w+i, y+h+i};
        Rectangle(hdc, r.left, r.top, r.right, r.bottom);
    }
    SelectObject(hdc, old);
    SelectObject(hdc, oldBr);
}

// --- Sidebar ---
struct SidebarItem {
    const char* label;
    Screen screen;
    bool adminOnly;
};

static const SidebarItem sidebarItems[] = {
    {"Dashboard",       SCR_ADMIN_DASH,       true},
    {"Create Account",  SCR_ADMIN_CREATE,     true},
    {"View Accounts",   SCR_ADMIN_VIEW,       true},
    {"Search Account",  SCR_ADMIN_SEARCH,     true},
    {"Freeze Account",  SCR_ADMIN_FREEZE,     true},
    {"Unfreeze Acct",   SCR_ADMIN_UNFREEZE,   true},
    {"Unlock Account",  SCR_ADMIN_UNLOCK,     true},
    {"Transactions",    SCR_ADMIN_TXNS,       true},
    {"Search Txns",     SCR_ADMIN_SEARCH_TXN, true},
    {"Audit Log",       SCR_ADMIN_AUDIT,      true},
    {"Loans",           SCR_ADMIN_LOANS,      true},
    {"Cash Inventory",  SCR_ADMIN_CASH,       true},
    {"Daily Report",    SCR_ADMIN_DAILY,      true},
    {"",                SCR_LOGIN,            false},
    {"ATM Menu",        SCR_ATM_MENU,         false},
    {"ATM Balance",     SCR_ATM_BALANCE,      false},
    {"Deposit",         SCR_ATM_DEPOSIT,      false},
    {"Withdraw",        SCR_ATM_WITHDRAW,     false},
    {"Transfer",        SCR_ATM_TRANSFER,     false},
    {"Mini Statement",  SCR_ATM_MINISTATE,    false},
    {"Change PIN",      SCR_ATM_CHANGEPIN,    false},
    {"ATM Info",        SCR_ATM_INFO,         false},
};
static const int NUM_SIDEBAR_ITEMS = sizeof(sidebarItems)/sizeof(sidebarItems[0]);

static void DrawSidebar(HDC hdc, RECT rc, AppState& s) {
    RECT sbRc = {0, HEADER_H, SIDEBAR_W, rc.bottom};
    FillRect(hdc, &sbRc, hBrushSidebar);

    // Bank logo area
    Txt(hdc, "NATIONAL BANK", 20, HEADER_H + 12, hFontBold, Accent);
    Txt(hdc, "Banking System", 20, HEADER_H + 32, hFontSmall, RGB(120,140,170));

    int yStart = HEADER_H + 65;
    int itemH = 36;

    int startIdx = 0;
    int count = 0;
    if (s.isAdmin) {
        startIdx = 0;
        count = 13;
    } else {
        startIdx = 14;
        count = NUM_SIDEBAR_ITEMS - 14;
    }

    for (int i = 0; i < count; i++) {
        int idx = startIdx + i;
        int y = yStart + i * itemH;
        bool selected = (s.screen == sidebarItems[idx].screen);

        if (selected) {
            RECT selRc = {0, y, SIDEBAR_W, y + itemH};
            FillRect(hdc, &selRc, hBrushSidebar);

            HPEN accentPen = CreatePen(PS_SOLID, 3, Accent);
            HPEN old = (HPEN)SelectObject(hdc, accentPen);
            MoveToEx(hdc, 0, y+4, NULL);
            LineTo(hdc, 0, y+itemH-4);
            SelectObject(hdc, old);
            DeleteObject(accentPen);
        }

        RECT itemRc = {8, y+2, SIDEBAR_W-8, y+itemH-2};
        if (selected) {
            HBRUSH selBg = CreateSolidBrush(RGB(30,50,100));
            FillRect(hdc, &itemRc, selBg);
            DeleteObject(selBg);
        }

        Txt(hdc, sidebarItems[idx].label, 24, y + 9, hFontNormal,
            selected ? RGB(255,255,255) : RGB(160,175,200));
    }
}

static int SidebarHitTest(LPARAM lp, AppState& s) {
    int mx = LOWORD(lp);
    int my = HIWORD(lp);
    if (mx >= SIDEBAR_W || my <= HEADER_H) return -1;

    int yStart = HEADER_H + 65;
    int itemH = 36;

    int startIdx = s.isAdmin ? 0 : 14;
    int count = s.isAdmin ? 13 : (NUM_SIDEBAR_ITEMS - 14);

    for (int i = 0; i < count; i++) {
        int y = yStart + i * itemH;
        if (my >= y && my < y + itemH) {
            return startIdx + i;
        }
    }
    return -1;
}

static void DrawHeader(HDC hdc, RECT rc, AppState& s) {
    RECT hdrRc = {0, 0, rc.right, HEADER_H};
    GradientHeader(hdc, hdrRc);

    Txt(hdc, "National Bank", SIDEBAR_W + 20, 16, hFontTitle, RGB(255,255,255));

    const char* user = s.isAdmin ? "Admin Panel" : "ATM Portal";
    Txt(hdc, user, rc.right - 150, 20, hFontNormal, RGB(180,200,230));
}

static void DrawStatus(HDC hdc, RECT rc) {
    RECT stRc = {0, rc.bottom - STATUS_H, rc.right, rc.bottom};
    FillRect(hdc, &stRc, hBrushPrimary);
    Txt(hdc, "National Bank v2.0  |  FOCP Group Project  |  All Rights Reserved",
        10, rc.bottom - STATUS_H + 7, hFontSmall, RGB(140,160,190));
}

// --- Card / Panel helpers ---
static void DrawCard(HDC hdc, int x, int y, int w, int h) {
    DrawShadow(hdc, x, y, w, h);
    RoundRect2(hdc, x, y, w, h, 10, hBrushCard, hPenCardBorder);
}

// --- Control Management ---
static AppState g;
static vector<EditInfo> gEdits;
static vector<ButtonInfo> gButtons;
static vector<LabelInfo> gLabels;
static HWND gHwnd = NULL;

struct EditInfo {
    int id;
    HWND hwnd;
};
struct ButtonInfo {
    int id;
    HWND hwnd;
    int color;
};
struct LabelInfo {
    int id;
    HWND hwnd;
};

static void ClearControls() {
    for (auto& e : gEdits) if (e.hwnd) DestroyWindow(e.hwnd);
    for (auto& b : gButtons) if (b.hwnd) DestroyWindow(b.hwnd);
    for (auto& l : gLabels) if (l.hwnd) DestroyWindow(l.hwnd);
    gEdits.clear();
    gButtons.clear();
    gLabels.clear();
    g.childControls.clear();
}

static HWND MakeBtn(int id, const char* text, int x, int y, int w, int h, int color) {
    HWND hw = CreateWindow("BUTTON", text,
        WS_CHILD|WS_VISIBLE|BS_OWNERDRAW,
        x, y, w, h, gHwnd, (HMENU)(LONG_PTR)id, NULL, NULL);
    SetWindowLongPtr(hw, GWL_USERDATA, color);
    ButtonInfo bi = {id, hw, color};
    gButtons.push_back(bi);
    return hw;
}

static HWND MakeEdit(int id, int x, int y, int w, int h, bool pwd) {
    DWORD style = WS_CHILD|WS_VISIBLE|WS_BORDER|ES_AUTOHSCROLL;
    if (pwd) style |= ES_PASSWORD;
    HWND hw = CreateWindow("EDIT", "",
        style, x, y, w, h, gHwnd, (HMENU)(LONG_PTR)id, NULL, NULL);
    SendMessage(hw, WM_SETFONT, (WPARAM)hFontNormal, TRUE);
    EditInfo ei = {id, hw};
    gEdits.push_back(ei);
    return hw;
}

static HWND MakeLabel(const char* text, int x, int y, int w, int h, int fontType) {
    DWORD style = WS_CHILD|WS_VISIBLE|SS_LEFT;
    HWND hw = CreateWindow("STATIC", text,
        style, x, y, w, h, gHwnd, (HMENU)(LONG_PTR)(ID_LABEL_BASE + (int)gLabels.size()), NULL, NULL);
    HFONT f = hFontNormal;
    if (fontType == 1) f = hFontBold;
    else if (fontType == 2) f = hFontSmall;
    SendMessage(hw, WM_SETFONT, (WPARAM)f, TRUE);
    LabelInfo li = {(int)gLabels.size(), hw};
    gLabels.push_back(li);
    return hw;
}

static string GetEditText(int id) {
    for (auto& e : gEdits) {
        if (e.id == id) {
            char buf[256] = {0};
            GetWindowText(e.hwnd, buf, 255);
            return string(buf);
        }
    }
    return "";
}

static void SetStatus(const string& msg, int type) {
    g.statusMsg = msg;
    g.statusType = type;
    if (gHwnd) InvalidateRect(gHwnd, NULL, FALSE);
}

// --- Refresh display ---
static void RefreshScreen() {
    if (gHwnd) InvalidateRect(gHwnd, NULL, TRUE);
}

// ============================
// SCREEN PAINTERS
// ============================

static void PaintLoginScreen(HDC hdc, RECT rc) {
    int cx = SIDEBAR_W + (rc.right - SIDEBAR_W) / 2;
    int cy = HEADER_H + (rc.bottom - HEADER_H - STATUS_H) / 2;

    DrawCard(hdc, cx - 200, cy - 160, 400, 320);
    TxtCenter(hdc, "National Bank", cx - 200, cy - 140, 400, 40, hFontTitle, Primary);
    TxtCenter(hdc, "Welcome to our Banking System", cx - 200, cy - 95, 400, 25, hFontSmall, TextLight);

    HPEN linePen = CreatePen(PS_SOLID, 1, CardBorder);
    HPEN old = (HPEN)SelectObject(hdc, linePen);
    MoveToEx(hdc, cx - 160, cy - 65, NULL);
    LineTo(hdc, cx + 160, cy - 65);
    SelectObject(hdc, old);
    DeleteObject(linePen);

    TxtCenter(hdc, "Select your portal:", cx - 200, cy - 55, 400, 25, hFontBold, Text);
}

static void PaintAdminDash(HDC hdc, RECT rc) {
    int sx = SIDEBAR_W + 25;
    int sy = HEADER_H + 25;
    int cw = rc.right - SIDEBAR_W - 50;
    int ch = rc.bottom - HEADER_H - STATUS_H - 50;

    Txt(hdc, "Dashboard Overview", sx, sy, hFontHeading, Primary);

    HPEN linePen = CreatePen(PS_SOLID, 2, Accent);
    HPEN old = (HPEN)SelectObject(hdc, linePen);
    MoveToEx(hdc, sx, sy + 30, NULL);
    LineTo(hdc, sx + 220, sy + 30);
    SelectObject(hdc, old);
    DeleteObject(linePen);

    g.accounts = loadAccounts() ? g.accounts : vector<Account>();
    loadAccounts(g.accounts);
    g.transactions = loadTransactions() ? g.transactions : vector<Transaction>();
    loadTransactions(g.transactions);

    int totalAcc = (int)g.accounts.size();
    int activeAcc = 0;
    double totalBal = 0;
    for (auto& a : g.accounts) {
        if (a.status == "active") activeAcc++;
        totalBal += a.balance;
    }
    int totalTxn = (int)g.transactions.size();

    struct CardData { const char* title; const char* value; COLORREF clr; };
    CardData cards[] = {
        {"Total Accounts", to_string(totalAcc).c_str(), Secondary},
        {"Active Accounts", to_string(activeAcc).c_str(), Success},
        {"Total Balance", "", Accent},
        {"Transactions", to_string(totalTxn).c_str(), RGB(156,39,176)},
    };

    stringstream balSS;
    balSS << "Rs. " << fixed << setprecision(0) << totalBal;
    string balStr = balSS.str();
    cards[2].value = balStr.c_str();

    int cardW = (cw - 45) / 4;
    for (int i = 0; i < 4; i++) {
        int cx = sx + i * (cardW + 15);
        int cy = sy + 50;
        DrawCard(hdc, cx, cy, cardW, 100);

        RECT iconRc = {cx + 10, cy + 10, cx + 50, cy + 50};
        HBRUSH iconBr = CreateSolidBrush(cards[i].clr);
        HPEN iconPen = CreatePen(PS_SOLID, 0, cards[i].clr);
        HPEN oldP = (HPEN)SelectObject(hdc, iconPen);
        HBRUSH oldB = (HBRUSH)SelectObject(hdc, iconBr);
        RoundRect(hdc, iconRc.left, iconRc.top, iconRc.right, iconRc.bottom, 8, 8);
        SelectObject(hdc, oldP);
        SelectObject(hdc, oldB);
        DeleteObject(iconBr);
        DeleteObject(iconPen);

        Txt(hdc, cards[i].title, cx + 60, cy + 15, hFontSmall, TextLight);
        Txt(hdc, cards[i].value, cx + 60, cy + 42, hFontHeading, Text);
    }

    int recentY = sy + 175;
    Txt(hdc, "Recent Transactions", sx, recentY, hFontBold, Text);
    recentY += 30;

    DrawCard(hdc, sx, recentY, cw, ch - 145);

    // Table header
    RECT tblHdr = {sx + 10, recentY + 5, sx + cw - 10, recentY + 30};
    HBRUSH tblHdrBr = CreateSolidBrush(RGB(240,243,248));
    FillRect(hdc, &tblHdr, tblHdrBr);
    DeleteObject(tblHdrBr);

    int colX[] = {sx+15, sx+100, sx+220, sx+370, sx+500, sx+620};
    const char* colH[] = {"ID", "Account", "Type", "Amount", "Date", "Balance"};
    for (int i = 0; i < 6; i++)
        Txt(hdc, colH[i], colX[i], recentY + 10, hFontBold, TextLight);

    int rowY = recentY + 35;
    int shown = min((int)g.transactions.size(), 10);
    for (int i = shown - 1; i >= 0; i--) {
        if (rowY > recentY + ch - 170) break;
        auto& t = g.transactions[i];
        Txt(hdc, t.transactionID.c_str(), colX[0], rowY, hFontMono, Text);
        Txt(hdc, to_string(t.accountNo).c_str(), colX[1], rowY, hFontNormal, Text);
        Txt(hdc, t.type.c_str(), colX[2], rowY, hFontNormal, Text);

        stringstream amtSS;
        amtSS << "Rs. " << fixed << setprecision(2) << t.amount;
        COLORREF amtClr = (t.type == "deposit") ? Success : (t.type == "withdrawal" ? Error : Secondary);
        Txt(hdc, amtSS.str().c_str(), colX[3], rowY, hFontNormal, amtClr);

        string dt = t.dateTime.substr(0, 10);
        Txt(hdc, dt.c_str(), colX[4], rowY, hFontSmall, TextLight);

        stringstream balSS2;
        balSS2 << "Rs. " << fixed << setprecision(2) << t.resultingBalance;
        Txt(hdc, balSS2.str().c_str(), colX[5], rowY, hFontNormal, Text);
        rowY += 25;

        HPEN sepPen = CreatePen(PS_SOLID, 1, RGB(235,240,248));
        HPEN oldSep = (HPEN)SelectObject(hdc, sepPen);
        MoveToEx(hdc, sx + 15, rowY - 3, NULL);
        LineTo(hdc, sx + cw - 15, rowY - 3);
        SelectObject(hdc, oldSep);
        DeleteObject(sepPen);
    }
}

static void PaintCreateAccount(HDC hdc, RECT rc) {
    int sx = SIDEBAR_W + 30;
    int sy = HEADER_H + 30;
    int cw = min(500, (int)(rc.right - SIDEBAR_W - 60));

    Txt(hdc, "Create New Account", sx, sy, hFontHeading, Primary);
    HPEN linePen = CreatePen(PS_SOLID, 2, Accent);
    HPEN old = (HPEN)SelectObject(hdc, linePen);
    MoveToEx(hdc, sx, sy + 28, NULL);
    LineTo(hdc, sx + 250, sy + 28);
    SelectObject(hdc, old);
    DeleteObject(linePen);

    int formY = sy + 50;
    int lblX = sx + 15;
    int edtX = sx + 170;
    int edtW = cw - 200;

    DrawCard(hdc, sx - 10, formY - 10, cw + 20, 400);

    const char* labels[] = {"Full Name:", "CNIC:", "Account Type:", "Initial Balance:", "PIN:", "Confirm PIN:"};
    for (int i = 0; i < 6; i++) {
        Txt(hdc, labels[i], lblX, formY + 10 + i * 48, hFontBold, Text);
    }
}

static void PaintViewAccounts(HDC hdc, RECT rc) {
    int sx = SIDEBAR_W + 25;
    int sy = HEADER_H + 25;
    int cw = rc.right - SIDEBAR_W - 50;
    int ch = rc.bottom - HEADER_H - STATUS_H - 50;

    Txt(hdc, "All Accounts", sx, sy, hFontHeading, Primary);
    HPEN linePen = CreatePen(PS_SOLID, 2, Accent);
    HPEN old = (HPEN)SelectObject(hdc, linePen);
    MoveToEx(hdc, sx, sy + 28, NULL);
    LineTo(hdc, sx + 160, sy + 28);
    SelectObject(hdc, old);
    DeleteObject(linePen);

    DrawCard(hdc, sx, sy + 45, cw, ch - 55);

    RECT tblHdr = {sx + 10, sy + 50, sx + cw - 10, sy + 78};
    HBRUSH tblHdrBr = CreateSolidBrush(RGB(240,243,248));
    FillRect(hdc, &tblHdr, tblHdrBr);
    DeleteObject(tblHdrBr);

    int colX[] = {sx+15, sx+90, sx+200, sx+330, sx+450, sx+560, sx+670};
    const char* colH[] = {"Acct #", "Name", "CNIC", "Type", "Balance", "Status", "Created"};
    for (int i = 0; i < 7; i++)
        Txt(hdc, colH[i], colX[i], sy + 57, hFontBold, TextLight);

    int rowY = sy + 85;
    for (size_t i = 0; i < g.accounts.size(); i++) {
        if (rowY > sy + ch - 30) break;
        auto& a = g.accounts[i];

        if (i % 2 == 0) {
            RECT altRc = {sx+5, rowY-3, sx+cw-5, rowY+22};
            HBRUSH altBr = CreateSolidBrush(RGB(248,250,252));
            FillRect(hdc, &altRc, altBr);
            DeleteObject(altBr);
        }

        Txt(hdc, to_string(a.accountNo).c_str(), colX[0], rowY, hFontMono, Text);
        Txt(hdc, a.name.substr(0,15).c_str(), colX[1], rowY, hFontNormal, Text);
        Txt(hdc, a.cnic.c_str(), colX[2], rowY, hFontSmall, TextLight);
        Txt(hdc, a.accountType.c_str(), colX[3], rowY, hFontNormal, Text);

        stringstream ss;
        ss << "Rs. " << fixed << setprecision(2) << a.balance;
        Txt(hdc, ss.str().c_str(), colX[4], rowY, hFontNormal, Text);

        COLORREF stClr = (a.status=="active") ? Success : (a.status=="frozen" ? Warning : Error);
        Txt(hdc, a.status.c_str(), colX[5], rowY, hFontBold, stClr);
        Txt(hdc, a.creationDate.substr(0,10).c_str(), colX[6], rowY, hFontSmall, TextLight);

        rowY += 28;

        HPEN sepPen = CreatePen(PS_SOLID, 1, RGB(235,240,248));
        HPEN oldSep = (HPEN)SelectObject(hdc, sepPen);
        MoveToEx(hdc, sx+15, rowY-5, NULL);
        LineTo(hdc, sx+cw-15, rowY-5);
        SelectObject(hdc, oldSep);
        DeleteObject(sepPen);
    }

    stringstream info;
    info << "Total: " << g.accounts.size() << " accounts";
    Txt(hdc, info.str().c_str(), sx + cw - 200, sy + ch - 25, hFontSmall, TextLight);
}

static void PaintSearchAccount(HDC hdc, RECT rc) {
    int sx = SIDEBAR_W + 30;
    int sy = HEADER_H + 30;
    int cw = min(450, (int)(rc.right - SIDEBAR_W - 60));

    Txt(hdc, "Search Account", sx, sy, hFontHeading, Primary);
    HPEN linePen = CreatePen(PS_SOLID, 2, Accent);
    HPEN old = (HPEN)SelectObject(hdc, linePen);
    MoveToEx(hdc, sx, sy + 28, NULL);
    LineTo(hdc, sx + 180, sy + 28);
    SelectObject(hdc, old);
    DeleteObject(linePen);

    DrawCard(hdc, sx - 10, sy + 45, cw + 20, 70);
    Txt(hdc, "Account #:", sx + 5, sy + 65, hFontBold, Text);

    DrawCard(hdc, sx - 10, sy + 130, cw + 20, 300);
}

static void PaintATMMenu(HDC hdc, RECT rc) {
    int sx = SIDEBAR_W + 30;
    int sy = HEADER_H + 30;

    Txt(hdc, "ATM Services", sx, sy, hFontHeading, Primary);
    HPEN linePen = CreatePen(PS_SOLID, 2, Accent);
    HPEN old = (HPEN)SelectObject(hdc, linePen);
    MoveToEx(hdc, sx, sy + 28, NULL);
    LineTo(hdc, sx + 160, sy + 28);
    SelectObject(hdc, old);
    DeleteObject(linePen);

    Txt(hdc, "Select a service from the sidebar or buttons below", sx, sy + 40, hFontSmall, TextLight);

    int cw = rc.right - SIDEBAR_W - 60;
    int ch = rc.bottom - HEADER_H - STATUS_H - sy - 40;
    DrawCard(hdc, sx - 10, sy + 65, cw + 20, ch - 10);

    struct ATMCard { const char* title; const char* desc; COLORREF clr; int scr; };
    ATMCard cards[] = {
        {"Check Balance", "View your current account balance", Success, SCR_ATM_BALANCE},
        {"Deposit Cash", "Deposit money into your account", Secondary, SCR_ATM_DEPOSIT},
        {"Withdraw Cash", "Withdraw money from your account", Error, SCR_ATM_WITHDRAW},
        {"Transfer Funds", "Send money to another account", RGB(156,39,176), SCR_ATM_TRANSFER},
        {"Mini Statement", "View recent transactions", RGB(0,150,136), SCR_ATM_MINISTATE},
        {"Change PIN", "Update your security PIN", RGB(255,87,34), SCR_ATM_CHANGEPIN},
    };

    int cardW = (cw - 30) / 3;
    int cardH = 110;
    for (int i = 0; i < 6; i++) {
        int col = i % 3;
        int row = i / 3;
        int cx = sx + col * (cardW + 15);
        int cy = sy + 80 + row * (cardH + 20);

        DrawCard(hdc, cx, cy, cardW, cardH);

        HBRUSH dotBr = CreateSolidBrush(cards[i].clr);
        HBRUSH oldB = (HBRUSH)SelectObject(hdc, dotBr);
        HPEN oldP = (HPEN)SelectObject(hdc, GetStockObject(NULL_PEN));
        Ellipse(hdc, cx + 15, cy + 15, cx + 35, cy + 35);
        SelectObject(hdc, oldB);
        SelectObject(hdc, oldP);
        DeleteObject(dotBr);

        Txt(hdc, cards[i].title, cx + 45, cy + 18, hFontBold, Text);
        Txt(hdc, cards[i].desc, cx + 15, cy + 50, hFontSmall, TextLight);
    }
}

static void PaintATMBalance(HDC hdc, RECT rc) {
    int sx = SIDEBAR_W + 30;
    int sy = HEADER_H + 30;
    int cw = min(450, (int)(rc.right - SIDEBAR_W - 60));

    Txt(hdc, "Account Balance", sx, sy, hFontHeading, Primary);
    HPEN linePen = CreatePen(PS_SOLID, 2, Accent);
    HPEN old = (HPEN)SelectObject(hdc, linePen);
    MoveToEx(hdc, sx, sy + 28, NULL);
    LineTo(hdc, sx + 180, sy + 28);
    SelectObject(hdc, old);
    DeleteObject(linePen);

    DrawCard(hdc, sx - 10, sy + 45, cw + 20, 70);
    Txt(hdc, "Account #:", sx + 5, sy + 65, hFontBold, Text);

    DrawCard(hdc, sx - 10, sy + 130, cw + 20, 180);
    if (g.currentAccIdx >= 0 && g.currentAccIdx < (int)g.accounts.size()) {
        auto& a = g.accounts[g.currentAccIdx];
        Txt(hdc, "Account Holder:", sx + 10, sy + 150, hFontSmall, TextLight);
        Txt(hdc, a.name.c_str(), sx + 150, sy + 148, hFontBold, Text);

        Txt(hdc, "Account Type:", sx + 10, sy + 180, hFontSmall, TextLight);
        Txt(hdc, a.accountType.c_str(), sx + 150, sy + 178, hFontNormal, Text);

        Txt(hdc, "Status:", sx + 10, sy + 210, hFontSmall, TextLight);
        COLORREF stClr = (a.status=="active") ? Success : Error;
        Txt(hdc, a.status.c_str(), sx + 150, sy + 208, hFontBold, stClr);

        Txt(hdc, "Available Balance:", sx + 10, sy + 250, hFontBold, Text);
        stringstream ss;
        ss << "Rs. " << fixed << setprecision(2) << a.balance;
        Txt(hdc, ss.str().c_str(), sx + 170, sy + 246, hFontTitle, Primary);
    }
}

static void PaintATMWithdraw(HDC hdc, RECT rc) {
    int sx = SIDEBAR_W + 30;
    int sy = HEADER_H + 30;
    int cw = min(450, (int)(rc.right - SIDEBAR_W - 60));

    Txt(hdc, "Withdraw Cash", sx, sy, hFontHeading, Primary);
    HPEN linePen = CreatePen(PS_SOLID, 2, Accent);
    HPEN old = (HPEN)SelectObject(hdc, linePen);
    MoveToEx(hdc, sx, sy + 28, NULL);
    LineTo(hdc, sx + 180, sy + 28);
    SelectObject(hdc, old);
    DeleteObject(linePen);

    DrawCard(hdc, sx - 10, sy + 45, cw + 20, 70);
    Txt(hdc, "Account #:", sx + 5, sy + 65, hFontBold, Text);

    DrawCard(hdc, sx - 10, sy + 130, cw + 20, 140);
    Txt(hdc, "Amount:", sx + 5, sy + 150, hFontBold, Text);

    if (g.currentAccIdx >= 0 && g.currentAccIdx < (int)g.accounts.size()) {
        stringstream ss;
        ss << "Daily Limit: Rs. " << fixed << setprecision(0) << DAILY_WITHDRAWAL_LIMIT
           << "  |  Withdrawn: Rs. " << g.accounts[g.currentAccIdx].dailyWithdrawn;
        Txt(hdc, ss.str().c_str(), sx, sy + 190, hFontSmall, TextLight);
    }
}

static void PaintATMDeposit(HDC hdc, RECT rc) {
    int sx = SIDEBAR_W + 30;
    int sy = HEADER_H + 30;
    int cw = min(450, (int)(rc.right - SIDEBAR_W - 60));

    Txt(hdc, "Deposit Cash", sx, sy, hFontHeading, Primary);
    HPEN linePen = CreatePen(PS_SOLID, 2, Accent);
    HPEN old = (HPEN)SelectObject(hdc, linePen);
    MoveToEx(hdc, sx, sy + 28, NULL);
    LineTo(hdc, sx + 150, sy + 28);
    SelectObject(hdc, old);
    DeleteObject(linePen);

    DrawCard(hdc, sx - 10, sy + 45, cw + 20, 70);
    Txt(hdc, "Account #:", sx + 5, sy + 65, hFontBold, Text);

    DrawCard(hdc, sx - 10, sy + 130, cw + 20, 100);
    Txt(hdc, "Amount:", sx + 5, sy + 150, hFontBold, Text);
}

static void PaintATMTransfer(HDC hdc, RECT rc) {
    int sx = SIDEBAR_W + 30;
    int sy = HEADER_H + 30;
    int cw = min(450, (int)(rc.right - SIDEBAR_W - 60));

    Txt(hdc, "Transfer Funds", sx, sy, hFontHeading, Primary);
    HPEN linePen = CreatePen(PS_SOLID, 2, Accent);
    HPEN old = (HPEN)SelectObject(hdc, linePen);
    MoveToEx(hdc, sx, sy + 28, NULL);
    LineTo(hdc, sx + 180, sy + 28);
    SelectObject(hdc, old);
    DeleteObject(linePen);

    DrawCard(hdc, sx - 10, sy + 45, cw + 20, 70);
    Txt(hdc, "From Account:", sx + 5, sy + 65, hFontBold, Text);

    DrawCard(hdc, sx - 10, sy + 130, cw + 20, 200);
    Txt(hdc, "To Account #:", sx + 5, sy + 150, hFontBold, Text);
    Txt(hdc, "Amount:", sx + 5, sy + 198, hFontBold, Text);
}

static void PaintATMMiniState(HDC hdc, RECT rc) {
    int sx = SIDEBAR_W + 25;
    int sy = HEADER_H + 25;
    int cw = rc.right - SIDEBAR_W - 50;
    int ch = rc.bottom - HEADER_H - STATUS_H - 50;

    Txt(hdc, "Mini Statement", sx, sy, hFontHeading, Primary);
    HPEN linePen = CreatePen(PS_SOLID, 2, Accent);
    HPEN old = (HPEN)SelectObject(hdc, linePen);
    MoveToEx(hdc, sx, sy + 28, NULL);
    LineTo(hdc, sx + 180, sy + 28);
    SelectObject(hdc, old);
    DeleteObject(linePen);

    DrawCard(hdc, sx, sy + 45, cw, ch - 55);

    RECT tblHdr = {sx + 10, sy + 50, sx + cw - 10, sy + 78};
    HBRUSH tblHdrBr = CreateSolidBrush(RGB(240,243,248));
    FillRect(hdc, &tblHdr, tblHdrBr);
    DeleteObject(tblHdrBr);

    int colX[] = {sx+15, sx+130, sx+260, sx+400, sx+530};
    const char* colH[] = {"ID", "Type", "Amount", "Date", "Balance"};
    for (int i = 0; i < 5; i++)
        Txt(hdc, colH[i], colX[i], sy + 57, hFontBold, TextLight);

    int rowY = sy + 85;
    int accNo = -1;
    if (g.currentAccIdx >= 0 && g.currentAccIdx < (int)g.accounts.size())
        accNo = g.accounts[g.currentAccIdx].accountNo;

    vector<Transaction> filtered;
    if (accNo >= 0) {
        for (auto& t : g.transactions)
            if (t.accountNo == accNo) filtered.push_back(t);
    }

    int shown = min((int)filtered.size(), 15);
    for (int i = shown - 1; i >= 0; i--) {
        if (rowY > sy + ch - 30) break;
        auto& t = filtered[i];
        Txt(hdc, t.transactionID.c_str(), colX[0], rowY, hFontMono, Text);
        Txt(hdc, t.type.c_str(), colX[1], rowY, hFontNormal, Text);

        stringstream ss;
        ss << "Rs. " << fixed << setprecision(2) << t.amount;
        COLORREF clr = (t.type=="deposit") ? Success : Error;
        Txt(hdc, ss.str().c_str(), colX[2], rowY, hFontNormal, clr);

        Txt(hdc, t.dateTime.substr(0,10).c_str(), colX[3], rowY, hFontSmall, TextLight);

        stringstream bs;
        bs << "Rs. " << fixed << setprecision(2) << t.resultingBalance;
        Txt(hdc, bs.str().c_str(), colX[4], rowY, hFontNormal, Text);
        rowY += 25;

        HPEN sepPen = CreatePen(PS_SOLID, 1, RGB(235,240,248));
        HPEN oldSep = (HPEN)SelectObject(hdc, sepPen);
        MoveToEx(hdc, sx+15, rowY-3, NULL);
        LineTo(hdc, sx+cw-15, rowY-3);
        SelectObject(hdc, oldSep);
        DeleteObject(sepPen);
    }
}

static void PaintATMChangePIN(HDC hdc, RECT rc) {
    int sx = SIDEBAR_W + 30;
    int sy = HEADER_H + 30;
    int cw = min(450, (int)(rc.right - SIDEBAR_W - 60));

    Txt(hdc, "Change PIN", sx, sy, hFontHeading, Primary);
    HPEN linePen = CreatePen(PS_SOLID, 2, Accent);
    HPEN old = (HPEN)SelectObject(hdc, linePen);
    MoveToEx(hdc, sx, sy + 28, NULL);
    LineTo(hdc, sx + 130, sy + 28);
    SelectObject(hdc, old);
    DeleteObject(linePen);

    DrawCard(hdc, sx - 10, sy + 45, cw + 20, 240);
    Txt(hdc, "Current PIN:", sx + 5, sy + 65, hFontBold, Text);
    Txt(hdc, "New PIN:", sx + 5, sy + 113, hFontBold, Text);
    Txt(hdc, "Confirm New PIN:", sx + 5, sy + 161, hFontBold, Text);
}

static void PaintATMInfo(HDC hdc, RECT rc) {
    int sx = SIDEBAR_W + 30;
    int sy = HEADER_H + 30;
    int cw = min(500, (int)(rc.right - SIDEBAR_W - 60));

    Txt(hdc, "ATM Information", sx, sy, hFontHeading, Primary);
    HPEN linePen = CreatePen(PS_SOLID, 2, Accent);
    HPEN old = (HPEN)SelectObject(hdc, linePen);
    MoveToEx(hdc, sx, sy + 28, NULL);
    LineTo(hdc, sx + 180, sy + 28);
    SelectObject(hdc, old);
    DeleteObject(linePen);

    DrawCard(hdc, sx - 10, sy + 45, cw + 20, 300);

    Txt(hdc, "Machine ID: ATM-NB-001", sx + 10, sy + 65, hFontNormal, Text);
    Txt(hdc, "Location: Main Branch, Downtown", sx + 10, sy + 95, hFontNormal, Text);
    Txt(hdc, "Status: Online", sx + 10, sy + 125, hFontBold, Success);

    Txt(hdc, "Cash Available:", sx + 10, sy + 165, hFontBold, Text);
    int y = sy + 195;
    for (auto& c : g.inventory) {
        stringstream ss;
        ss << c.denomination << " x " << c.count << " notes";
        Txt(hdc, ss.str().c_str(), sx + 25, y, hFontMono, Text);
        y += 22;
    }
}

static void PaintAdminFreeze(HDC hdc, RECT rc) {
    int sx = SIDEBAR_W + 30;
    int sy = HEADER_H + 30;
    int cw = min(450, (int)(rc.right - SIDEBAR_W - 60));

    Txt(hdc, "Freeze Account", sx, sy, hFontHeading, Primary);
    HPEN linePen = CreatePen(PS_SOLID, 2, Accent);
    HPEN old = (HPEN)SelectObject(hdc, linePen);
    MoveToEx(hdc, sx, sy + 28, NULL);
    LineTo(hdc, sx + 170, sy + 28);
    SelectObject(hdc, old);
    DeleteObject(linePen);

    DrawCard(hdc, sx - 10, sy + 45, cw + 20, 70);
    Txt(hdc, "Account #:", sx + 5, sy + 65, hFontBold, Text);
    Txt(hdc, "Freezing an account disables all ATM transactions.", sx, sy + 130, hFontSmall, Error);
}

static void PaintAdminUnfreeze(HDC hdc, RECT rc) {
    int sx = SIDEBAR_W + 30;
    int sy = HEADER_H + 30;
    int cw = min(450, (int)(rc.right - SIDEBAR_W - 60));

    Txt(hdc, "Unfreeze Account", sx, sy, hFontHeading, Primary);
    HPEN linePen = CreatePen(PS_SOLID, 2, Accent);
    HPEN old = (HPEN)SelectObject(hdc, linePen);
    MoveToEx(hdc, sx, sy + 28, NULL);
    LineTo(hdc, sx + 200, sy + 28);
    SelectObject(hdc, old);
    DeleteObject(linePen);

    DrawCard(hdc, sx - 10, sy + 45, cw + 20, 70);
    Txt(hdc, "Account #:", sx + 5, sy + 65, hFontBold, Text);
    Txt(hdc, "Reactivate a frozen account for full access.", sx, sy + 130, hFontSmall, Success);
}

static void PaintAdminUnlock(HDC hdc, RECT rc) {
    int sx = SIDEBAR_W + 30;
    int sy = HEADER_H + 30;
    int cw = min(450, (int)(rc.right - SIDEBAR_W - 60));

    Txt(hdc, "Unlock Account", sx, sy, hFontHeading, Primary);
    HPEN linePen = CreatePen(PS_SOLID, 2, Accent);
    HPEN old = (HPEN)SelectObject(hdc, linePen);
    MoveToEx(hdc, sx, sy + 28, NULL);
    LineTo(hdc, sx + 180, sy + 28);
    SelectObject(hdc, old);
    DeleteObject(linePen);

    DrawCard(hdc, sx - 10, sy + 45, cw + 20, 70);
    Txt(hdc, "Account #:", sx + 5, sy + 65, hFontBold, Text);
    Txt(hdc, "Reset PIN attempts and restore locked accounts.", sx, sy + 130, hFontSmall, Secondary);
}

static void PaintAdminTxns(HDC hdc, RECT rc) {
    int sx = SIDEBAR_W + 25;
    int sy = HEADER_H + 25;
    int cw = rc.right - SIDEBAR_W - 50;
    int ch = rc.bottom - HEADER_H - STATUS_H - 50;

    Txt(hdc, "Transaction History", sx, sy, hFontHeading, Primary);
    HPEN linePen = CreatePen(PS_SOLID, 2, Accent);
    HPEN old = (HPEN)SelectObject(hdc, linePen);
    MoveToEx(hdc, sx, sy + 28, NULL);
    LineTo(hdc, sx + 220, sy + 28);
    SelectObject(hdc, old);
    DeleteObject(linePen);

    DrawCard(hdc, sx, sy + 45, cw, ch - 55);

    RECT tblHdr = {sx + 10, sy + 50, sx + cw - 10, sy + 78};
    HBRUSH tblHdrBr = CreateSolidBrush(RGB(240,243,248));
    FillRect(hdc, &tblHdr, tblHdrBr);
    DeleteObject(tblHdrBr);

    int colX[] = {sx+15, sx+120, sx+240, sx+370, sx+500, sx+620};
    const char* colH[] = {"ID", "Account", "Type", "Amount", "Date", "Balance"};
    for (int i = 0; i < 6; i++)
        Txt(hdc, colH[i], colX[i], sy + 57, hFontBold, TextLight);

    int rowY = sy + 85;
    int shown = min((int)g.transactions.size(), 20);
    for (int i = shown - 1; i >= 0; i--) {
        if (rowY > sy + ch - 30) break;
        auto& t = g.transactions[i];
        Txt(hdc, t.transactionID.c_str(), colX[0], rowY, hFontMono, Text);
        Txt(hdc, to_string(t.accountNo).c_str(), colX[1], rowY, hFontNormal, Text);
        Txt(hdc, t.type.c_str(), colX[2], rowY, hFontNormal, Text);

        stringstream ss;
        ss << "Rs. " << fixed << setprecision(2) << t.amount;
        COLORREF clr = (t.type=="deposit") ? Success : (t.type=="withdrawal" ? Error : Secondary);
        Txt(hdc, ss.str().c_str(), colX[3], rowY, hFontNormal, clr);
        Txt(hdc, t.dateTime.substr(0,10).c_str(), colX[4], rowY, hFontSmall, TextLight);

        stringstream bs;
        bs << "Rs. " << fixed << setprecision(2) << t.resultingBalance;
        Txt(hdc, bs.str().c_str(), colX[5], rowY, hFontNormal, Text);
        rowY += 25;

        HPEN sepPen = CreatePen(PS_SOLID, 1, RGB(235,240,248));
        HPEN oldSep = (HPEN)SelectObject(hdc, sepPen);
        MoveToEx(hdc, sx+15, rowY-3, NULL);
        LineTo(hdc, sx+cw-15, rowY-3);
        SelectObject(hdc, oldSep);
        DeleteObject(sepPen);
    }
}

static void PaintAdminSearchTxn(HDC hdc, RECT rc) {
    int sx = SIDEBAR_W + 30;
    int sy = HEADER_H + 30;
    int cw = min(550, (int)(rc.right - SIDEBAR_W - 60));

    Txt(hdc, "Search Transactions", sx, sy, hFontHeading, Primary);
    HPEN linePen = CreatePen(PS_SOLID, 2, Accent);
    HPEN old = (HPEN)SelectObject(hdc, linePen);
    MoveToEx(hdc, sx, sy + 28, NULL);
    LineTo(hdc, sx + 220, sy + 28);
    SelectObject(hdc, old);
    DeleteObject(linePen);

    DrawCard(hdc, sx - 10, sy + 45, cw + 20, 200);
    Txt(hdc, "Account #:", sx + 5, sy + 65, hFontBold, Text);
    Txt(hdc, "Type:", sx + 250, sy + 65, hFontBold, Text);
    Txt(hdc, "From Date:", sx + 5, sy + 113, hFontBold, Text);
    Txt(hdc, "To Date:", sx + 250, sy + 113, hFontBold, Text);
    Txt(hdc, "Min Amount:", sx + 5, sy + 161, hFontBold, Text);
    Txt(hdc, "Max Amount:", sx + 250, sy + 161, hFontBold, Text);

    DrawCard(hdc, sx - 10, sy + 260, cw + 20, 300);
}

static void PaintAdminAudit(HDC hdc, RECT rc) {
    int sx = SIDEBAR_W + 25;
    int sy = HEADER_H + 25;
    int cw = rc.right - SIDEBAR_W - 50;
    int ch = rc.bottom - HEADER_H - STATUS_H - 50;

    Txt(hdc, "Audit Log", sx, sy, hFontHeading, Primary);
    HPEN linePen = CreatePen(PS_SOLID, 2, Accent);
    HPEN old = (HPEN)SelectObject(hdc, linePen);
    MoveToEx(hdc, sx, sy + 28, NULL);
    LineTo(hdc, sx + 120, sy + 28);
    SelectObject(hdc, old);
    DeleteObject(linePen);

    DrawCard(hdc, sx, sy + 45, cw, ch - 55);

    RECT tblHdr = {sx + 10, sy + 50, sx + cw - 10, sy + 78};
    HBRUSH tblHdrBr = CreateSolidBrush(RGB(240,243,248));
    FillRect(hdc, &tblHdr, tblHdrBr);
    DeleteObject(tblHdrBr);

    Txt(hdc, "Timestamp", sx + 15, sy + 57, hFontBold, TextLight);
    Txt(hdc, "Action", sx + 230, sy + 57, hFontBold, TextLight);
    Txt(hdc, "Details", sx + 420, sy + 57, hFontBold, TextLight);

    vector<AuditEntry> entries = loadAuditEntries();
    int rowY = sy + 85;
    int shown = min((int)entries.size(), 20);
    for (int i = (int)entries.size() - 1; i >= (int)entries.size() - shown && i >= 0; i--) {
        if (rowY > sy + ch - 30) break;
        auto& e = entries[i];
        Txt(hdc, e.timestamp.substr(0,19).c_str(), sx + 15, rowY, hFontMono, TextLight);
        Txt(hdc, e.action.substr(0,20).c_str(), sx + 230, rowY, hFontBold, Text);
        Txt(hdc, e.details.substr(0,40).c_str(), sx + 420, rowY, hFontSmall, TextLight);
        rowY += 25;

        HPEN sepPen = CreatePen(PS_SOLID, 1, RGB(235,240,248));
        HPEN oldSep = (HPEN)SelectObject(hdc, sepPen);
        MoveToEx(hdc, sx+15, rowY-3, NULL);
        LineTo(hdc, sx+cw-15, rowY-3);
        SelectObject(hdc, oldSep);
        DeleteObject(sepPen);
    }
}

static void PaintAdminLoans(HDC hdc, RECT rc) {
    int sx = SIDEBAR_W + 25;
    int sy = HEADER_H + 25;
    int cw = rc.right - SIDEBAR_W - 50;
    int ch = rc.bottom - HEADER_H - STATUS_H - 50;

    Txt(hdc, "Loan Management", sx, sy, hFontHeading, Primary);
    HPEN linePen = CreatePen(PS_SOLID, 2, Accent);
    HPEN old = (HPEN)SelectObject(hdc, linePen);
    MoveToEx(hdc, sx, sy + 28, NULL);
    LineTo(hdc, sx + 200, sy + 28);
    SelectObject(hdc, old);
    DeleteObject(linePen);

    DrawCard(hdc, sx - 10, sy + 40, cw + 20, 90);
    Txt(hdc, "Account #:", sx + 5, sy + 60, hFontBold, Text);
    Txt(hdc, "Amount:", sx + 5, sy + 90, hFontBold, Text);

    DrawCard(hdc, sx, sy + 145, cw, ch - 160);

    RECT tblHdr = {sx + 10, sy + 150, sx + cw - 10, sy + 178};
    HBRUSH tblHdrBr = CreateSolidBrush(RGB(240,243,248));
    FillRect(hdc, &tblHdr, tblHdrBr);
    DeleteObject(tblHdrBr);

    int colX[] = {sx+15, sx+80, sx+180, sx+300, sx+420, sx+540};
    const char* colH[] = {"ID", "Account", "Amount", "Monthly", "Status", "Paid"};
    for (int i = 0; i < 6; i++)
        Txt(hdc, colH[i], colX[i], sy + 157, hFontBold, TextLight);

    int rowY = sy + 185;
    for (size_t i = 0; i < g.loans.size(); i++) {
        if (rowY > sy + ch - 20) break;
        auto& l = g.loans[i];
        Txt(hdc, to_string(l.loanId).c_str(), colX[0], rowY, hFontMono, Text);
        Txt(hdc, to_string(l.accountNo).c_str(), colX[1], rowY, hFontNormal, Text);
        stringstream ss; ss << "Rs. " << fixed << setprecision(0) << l.amount;
        Txt(hdc, ss.str().c_str(), colX[2], rowY, hFontNormal, Text);
        stringstream ms; ms << "Rs. " << fixed << setprecision(0) << l.monthlyPayment;
        Txt(hdc, ms.str().c_str(), colX[3], rowY, hFontNormal, Text);
        COLORREF clr = (l.status=="active") ? Warning : Success;
        Txt(hdc, l.status.c_str(), colX[4], rowY, hFontBold, clr);
        stringstream ps; ps << l.monthsPaid << "/" << l.termMonths;
        Txt(hdc, ps.str().c_str(), colX[5], rowY, hFontNormal, Text);
        rowY += 25;

        HPEN sepPen = CreatePen(PS_SOLID, 1, RGB(235,240,248));
        HPEN oldSep = (HPEN)SelectObject(hdc, sepPen);
        MoveToEx(hdc, sx+15, rowY-3, NULL);
        LineTo(hdc, sx+cw-15, rowY-3);
        SelectObject(hdc, oldSep);
        DeleteObject(sepPen);
    }
}

static void PaintAdminCash(HDC hdc, RECT rc) {
    int sx = SIDEBAR_W + 30;
    int sy = HEADER_H + 30;
    int cw = min(500, (int)(rc.right - SIDEBAR_W - 60));

    Txt(hdc, "ATM Cash Inventory", sx, sy, hFontHeading, Primary);
    HPEN linePen = CreatePen(PS_SOLID, 2, Accent);
    HPEN old = (HPEN)SelectObject(hdc, linePen);
    MoveToEx(hdc, sx, sy + 28, NULL);
    LineTo(hdc, sx + 220, sy + 28);
    SelectObject(hdc, old);
    DeleteObject(linePen);

    DrawCard(hdc, sx - 10, sy + 45, cw + 20, 250);

    int rowY = sy + 65;
    Txt(hdc, "Denomination", sx + 15, rowY, hFontBold, TextLight);
    Txt(hdc, "Count", sx + 200, rowY, hFontBold, TextLight);
    Txt(hdc, "Total", sx + 330, rowY, hFontBold, TextLight);
    rowY += 30;

    double grandTotal = 0;
    for (auto& c : g.inventory) {
        HPEN sepPen = CreatePen(PS_SOLID, 1, RGB(235,240,248));
        HPEN oldSep = (HPEN)SelectObject(hdc, sepPen);
        MoveToEx(hdc, sx+15, rowY, NULL);
        LineTo(hdc, sx+cw-15, rowY);
        SelectObject(hdc, oldSep);
        DeleteObject(sepPen);

        Txt(hdc, ("Rs. " + to_string(c.denomination)).c_str(), sx + 15, rowY + 5, hFontNormal, Text);
        Txt(hdc, to_string(c.count).c_str(), sx + 200, rowY + 5, hFontNormal, Text);
        double tot = c.denomination * c.count;
        grandTotal += tot;
        stringstream ss; ss << "Rs. " << fixed << setprecision(0) << tot;
        Txt(hdc, ss.str().c_str(), sx + 330, rowY + 5, hFontNormal, Text);
        rowY += 30;
    }

    rowY += 10;
    HPEN linePen2 = CreatePen(PS_SOLID, 2, Primary);
    HPEN old2 = (HPEN)SelectObject(hdc, linePen2);
    MoveToEx(hdc, sx + 15, rowY, NULL);
    LineTo(hdc, sx + cw - 15, rowY);
    SelectObject(hdc, old2);
    DeleteObject(linePen2);

    Txt(hdc, "Total Cash:", sx + 15, rowY + 10, hFontBold, Primary);
    stringstream ts; ts << "Rs. " << fixed << setprecision(0) << grandTotal;
    Txt(hdc, ts.str().c_str(), sx + 150, rowY + 8, hFontTitle, Primary);
}

static void PaintAdminDaily(HDC hdc, RECT rc) {
    int sx = SIDEBAR_W + 30;
    int sy = HEADER_H + 30;
    int cw = min(500, (int)(rc.right - SIDEBAR_W - 60));

    Txt(hdc, "Daily Summary Report", sx, sy, hFontHeading, Primary);
    HPEN linePen = CreatePen(PS_SOLID, 2, Accent);
    HPEN old = (HPEN)SelectObject(hdc, linePen);
    MoveToEx(hdc, sx, sy + 28, NULL);
    LineTo(hdc, sx + 240, sy + 28);
    SelectObject(hdc, old);
    DeleteObject(linePen);

    DrawCard(hdc, sx - 10, sy + 45, cw + 20, 350);

    string today = getCurrentDateStr();
    Txt(hdc, ("Date: " + today).c_str(), sx + 10, sy + 65, hFontBold, Text);

    double totalDeposits = 0, totalWithdrawals = 0, totalTransfers = 0;
    int depCount = 0, witCount = 0, trfCount = 0;
    for (auto& t : g.transactions) {
        if (t.dateTime.substr(0, 10) == today) {
            if (t.type == "deposit") { totalDeposits += t.amount; depCount++; }
            else if (t.type == "withdrawal") { totalWithdrawals += t.amount; witCount++; }
            else if (t.type == "transfer") { totalTransfers += t.amount; trfCount++; }
        }
    }

    int y = sy + 105;
    struct Metric { const char* label; double value; int count; COLORREF clr; };
    Metric metrics[] = {
        {"Total Deposits", totalDeposits, depCount, Success},
        {"Total Withdrawals", totalWithdrawals, witCount, Error},
        {"Total Transfers", totalTransfers, trfCount, Secondary},
    };

    for (int i = 0; i < 3; i++) {
        Txt(hdc, metrics[i].label, sx + 15, y, hFontBold, TextLight);
        stringstream ss; ss << "Rs. " << fixed << setprecision(2) << metrics[i].value;
        Txt(hdc, ss.str().c_str(), sx + 15, y + 22, hFontHeading, metrics[i].clr);
        stringstream cs; cs << metrics[i].count << " transactions";
        Txt(hdc, cs.str().c_str(), sx + 250, y + 25, hFontSmall, TextLight);
        y += 65;

        if (i < 2) {
            HPEN sepPen = CreatePen(PS_SOLID, 1, RGB(235,240,248));
            HPEN oldSep = (HPEN)SelectObject(hdc, sepPen);
            MoveToEx(hdc, sx+15, y-10, NULL);
            LineTo(hdc, sx+cw-25, y-10);
            SelectObject(hdc, oldSep);
            DeleteObject(sepPen);
        }
    }
}

static void PaintOTPScreen(HDC hdc, RECT rc) {
    int sx = SIDEBAR_W + 30;
    int sy = HEADER_H + 30;
    int cw = min(400, (int)(rc.right - SIDEBAR_W - 60));

    Txt(hdc, "OTP Verification", sx, sy, hFontHeading, Primary);
    HPEN linePen = CreatePen(PS_SOLID, 2, Accent);
    HPEN old = (HPEN)SelectObject(hdc, linePen);
    MoveToEx(hdc, sx, sy + 28, NULL);
    LineTo(hdc, sx + 180, sy + 28);
    SelectObject(hdc, old);
    DeleteObject(linePen);

    DrawCard(hdc, sx - 10, sy + 45, cw + 20, 200);
    Txt(hdc, "Enter OTP sent to your device:", sx + 5, sy + 65, hFontBold, Text);
    Txt(hdc, g.otpCode.c_str(), sx + 100, sy + 100, hFontTitle, Accent);
    Txt(hdc, "OTP:", sx + 5, sy + 145, hFontBold, Text);
}

// ============================
// MAIN PAINT DISPATCHER
// ============================
static void PaintScreen(HDC hdc, RECT rc) {
    FillRect(hdc, &rc, hBrushBg);
    DrawHeader(hdc, rc, g);

    if (g.screen != SCR_LOGIN && g.screen != SCR_ADMIN_LOGIN) {
        DrawSidebar(hdc, rc, g);
    }

    switch (g.screen) {
        case SCR_LOGIN: PaintLoginScreen(hdc, rc); break;
        case SCR_ADMIN_LOGIN: PaintLoginScreen(hdc, rc); break;
        case SCR_ADMIN_DASH: PaintAdminDash(hdc, rc); break;
        case SCR_ADMIN_CREATE: PaintCreateAccount(hdc, rc); break;
        case SCR_ADMIN_VIEW: PaintViewAccounts(hdc, rc); break;
        case SCR_ADMIN_SEARCH: PaintSearchAccount(hdc, rc); break;
        case SCR_ADMIN_FREEZE: PaintAdminFreeze(hdc, rc); break;
        case SCR_ADMIN_UNFREEZE: PaintAdminUnfreeze(hdc, rc); break;
        case SCR_ADMIN_UNLOCK: PaintAdminUnlock(hdc, rc); break;
        case SCR_ADMIN_TXNS: PaintAdminTxns(hdc, rc); break;
        case SCR_ADMIN_SEARCH_TXN: PaintAdminSearchTxn(hdc, rc); break;
        case SCR_ADMIN_AUDIT: PaintAdminAudit(hdc, rc); break;
        case SCR_ADMIN_LOANS: PaintAdminLoans(hdc, rc); break;
        case SCR_ADMIN_CASH: PaintAdminCash(hdc, rc); break;
        case SCR_ADMIN_DAILY: PaintAdminDaily(hdc, rc); break;
        case SCR_ATM_LOGIN: PaintLoginScreen(hdc, rc); break;
        case SCR_ATM_MENU: PaintATMMenu(hdc, rc); break;
        case SCR_ATM_BALANCE: PaintATMBalance(hdc, rc); break;
        case SCR_ATM_DEPOSIT: PaintATMDeposit(hdc, rc); break;
        case SCR_ATM_WITHDRAW: PaintATMWithdraw(hdc, rc); break;
        case SCR_ATM_TRANSFER: PaintATMTransfer(hdc, rc); break;
        case SCR_ATM_MINISTATE: PaintATMMiniState(hdc, rc); break;
        case SCR_ATM_CHANGEPIN: PaintATMChangePIN(hdc, rc); break;
        case SCR_ATM_INFO: PaintATMInfo(hdc, rc); break;
    }

    if (!g.statusMsg.empty()) {
        RECT stRc = {SIDEBAR_W + 10, rc.bottom - STATUS_H - 25, rc.right - 10, rc.bottom - STATUS_H - 5};
        HBRUSH stBr = CreateSolidBrush(g.statusType == 1 ? Success : (g.statusType == 2 ? Error : Secondary));
        RoundRect2(hdc, stRc.left, stRc.top, stRc.right - stRc.left, stRc.bottom - stRc.top, 6, stBr, (HPEN)GetStockObject(NULL_PEN));
        DeleteObject(stBr);
        Txt(hdc, g.statusMsg.c_str(), stRc.left + 10, stRc.top + 4, hFontNormal, RGB(255,255,255));
    }

    DrawStatus(hdc, rc);
}

// ============================
// SCREEN TRANSITIONS + CONTROLS
// ============================

static void GoToScreen(Screen scr);

static void CreateScreenControls() {
    ClearControls();
    RECT rc;
    GetClientRect(gHwnd, &rc);
    int sx = SIDEBAR_W + 30;
    int sy = HEADER_H + 80;
    int edW = 250;
    int edH = 28;
    int btnW = 160;
    int btnH = 38;
    int gap = 48;

    switch (g.screen) {
    case SCR_LOGIN:
    case SCR_ADMIN_LOGIN:
    case SCR_ATM_LOGIN: {
        int cx = SIDEBAR_W + (rc.right - SIDEBAR_W) / 2;
        int cy = HEADER_H + (rc.bottom - HEADER_H - STATUS_H) / 2;
        MakeBtn(BTN_DO_ADMIN_LOGIN, "Bank Administrator", cx - 180, cy - 20, 170, 42, 0x001E32A0);
        MakeBtn(BTN_DO_LOGIN, "ATM Customer", cx + 10, cy - 20, 170, 42, 0x0043A047);
        break;
    }
    case SCR_ADMIN_DASH: {
        MakeBtn(BTN_DO_BACKUP, "Backup Data", sx + 500, HEADER_H + 30, 140, 36, 0x001E32A0);
        break;
    }
    case SCR_ADMIN_CREATE: {
        int formY = HEADER_H + 80;
        MakeLabel("Full Name:", sx + 5, formY + 5, 150, 22, 1);
        MakeEdit(EDT_NAME, sx + 170, formY + 2, edW, edH);
        MakeLabel("CNIC (XXXXX-XXXXXXX-X):", sx + 5, formY + gap + 5, 190, 22, 1);
        MakeEdit(EDT_CNIC, sx + 200, formY + gap + 2, edW - 30, edH);
        MakeLabel("Account Type (savings/current):", sx + 5, formY + gap*2 + 5, 220, 22, 1);
        MakeEdit(EDT_TYPE, sx + 230, formY + gap*2 + 2, edW - 60, edH);
        MakeLabel("Initial Balance:", sx + 5, formY + gap*3 + 5, 150, 22, 1);
        MakeEdit(EDT_BALANCE, sx + 170, formY + gap*3 + 2, edW, edH);
        MakeLabel("PIN (4 digits):", sx + 5, formY + gap*4 + 5, 150, 22, 1);
        MakeEdit(EDT_PIN, sx + 170, formY + gap*4 + 2, edW, edH, true);
        MakeLabel("Confirm PIN:", sx + 5, formY + gap*5 + 5, 150, 22, 1);
        MakeEdit(EDT_CONFIRMPIN, sx + 170, formY + gap*5 + 2, edW, edH, true);
        MakeBtn(BTN_DO_CREATE, "Create Account", sx + 30, formY + gap*6 + 15, btnW, btnH, 0x0043A047);
        MakeBtn(BTN_CLEAR_CREATE, "Clear Form", sx + 210, formY + gap*6 + 15, btnW, btnH, 0x00808080);
        break;
    }
    case SCR_ADMIN_VIEW: break;
    case SCR_ADMIN_SEARCH: {
        MakeLabel("Account #:", sx + 5, sy + 5, 150, 22, 1);
        MakeEdit(EDT_SEARCH, sx + 115, sy + 2, edW, edH);
        MakeBtn(BTN_DO_SEARCH, "Search", sx + 380, sy, 100, btnH, 0x001E32A0);
        break;
    }
    case SCR_ADMIN_FREEZE: {
        MakeLabel("Account #:", sx + 5, sy + 5, 150, 22, 1);
        MakeEdit(EDT_SEARCH, sx + 115, sy + 2, edW, edH);
        MakeBtn(BTN_DO_FREEZE, "Freeze Account", sx + 30, sy + gap + 10, btnW, btnH, 0x004527A0);
        break;
    }
    case SCR_ADMIN_UNFREEZE: {
        MakeLabel("Account #:", sx + 5, sy + 5, 150, 22, 1);
        MakeEdit(EDT_SEARCH, sx + 115, sy + 2, edW, edH);
        MakeBtn(BTN_DO_UNFREEZE, "Unfreeze Account", sx + 30, sy + gap + 10, btnW, btnH, 0x0043A047);
        break;
    }
    case SCR_ADMIN_UNLOCK: {
        MakeLabel("Account #:", sx + 5, sy + 5, 150, 22, 1);
        MakeEdit(EDT_SEARCH, sx + 115, sy + 2, edW, edH);
        MakeBtn(BTN_DO_UNLOCK, "Unlock Account", sx + 30, sy + gap + 10, btnW, btnH, 0x00FF8F00);
        break;
    }
    case SCR_ADMIN_TXNS: break;
    case SCR_ADMIN_SEARCH_TXN: {
        MakeLabel("Account #:", sx + 5, sy + 5, 120, 22, 1);
        MakeEdit(EDT_SEARCH_TXN_ACC, sx + 130, sy + 2, 120, edH);
        MakeLabel("Type:", sx + 270, sy + 5, 60, 22, 1);
        MakeEdit(EDT_SEARCH_TXN_TYPE, sx + 330, sy + 2, 120, edH);
        MakeLabel("From:", sx + 5, sy + gap + 5, 80, 22, 1);
        MakeEdit(EDT_SEARCH_TXN_FROM, sx + 90, sy + gap + 2, 120, edH);
        MakeLabel("To:", sx + 230, sy + gap + 5, 40, 22, 1);
        MakeEdit(EDT_SEARCH_TXN_TO, sx + 270, sy + gap + 2, 120, edH);
        MakeLabel("Min:", sx + 420, sy + gap + 5, 50, 22, 1);
        MakeEdit(EDT_SEARCH_TXN_MIN, sx + 470, sy + gap + 2, 80, edH);
        MakeLabel("Max:", sx + 5, sy + gap*2 + 5, 50, 22, 1);
        MakeEdit(EDT_SEARCH_TXN_MAX, sx + 60, sy + gap*2 + 2, 80, edH);
        MakeBtn(BTN_DO_SEARCH_TXN, "Search", sx + 160, sy + gap*2, 100, btnH, 0x001E32A0);
        break;
    }
    case SCR_ADMIN_AUDIT: break;
    case SCR_ADMIN_LOANS: {
        MakeLabel("Account #:", sx + 5, sy + 5, 150, 22, 1);
        MakeEdit(EDT_LOAN_ACC, sx + 115, sy + 2, edW, edH);
        MakeLabel("Loan Amount:", sx + 5, sy + gap + 5, 150, 22, 1);
        MakeEdit(EDT_LOAN_AMT, sx + 115, sy + gap + 2, edW, edH);
        MakeLabel("Term (months):", sx + 5, sy + gap*2 + 5, 150, 22, 1);
        MakeEdit(EDT_LOAN_TERM, sx + 115, sy + gap*2 + 2, edW, edH);
        MakeBtn(BTN_DO_LOAN, "Approve Loan", sx + 30, sy + gap*3 + 10, btnW, btnH, 0x00FF8F00);
        break;
    }
    case SCR_ADMIN_CASH: break;
    case SCR_ADMIN_DAILY: break;
    case SCR_ATM_MENU: break;
    case SCR_ATM_BALANCE: {
        MakeLabel("Account #:", sx + 5, sy + 5, 150, 22, 1);
        MakeEdit(EDT_ACCNO, sx + 115, sy + 2, edW, edH);
        MakeLabel("PIN:", sx + 5, sy + gap + 5, 150, 22, 1);
        MakeEdit(EDT_PIN, sx + 115, sy + gap + 2, edW, edH, true);
        MakeBtn(BTN_DO_LOGIN, "View Balance", sx + 30, sy + gap*2 + 10, btnW, btnH, 0x001E32A0);
        break;
    }
    case SCR_ATM_DEPOSIT: {
        MakeLabel("Account #:", sx + 5, sy + 5, 150, 22, 1);
        MakeEdit(EDT_ACCNO, sx + 115, sy + 2, edW, edH);
        MakeLabel("Amount:", sx + 5, sy + gap + 5, 150, 22, 1);
        MakeEdit(EDT_AMT, sx + 115, sy + gap + 2, edW, edH);
        MakeBtn(BTN_DO_DEPOSIT, "Deposit", sx + 30, sy + gap*2 + 10, btnW, btnH, 0x0043A047);
        break;
    }
    case SCR_ATM_WITHDRAW: {
        MakeLabel("Amount:", sx + 5, sy + 5, 150, 22, 1);
        MakeEdit(EDT_AMT, sx + 115, sy + 2, edW, edH);
        MakeBtn(BTN_DO_WITHDRAW, "Withdraw", sx + 30, sy + gap + 10, btnW, btnH, 0x00DC3545);
        break;
    }
    case SCR_ATM_TRANSFER: {
        MakeLabel("To Account #:", sx + 5, sy + 5, 150, 22, 1);
        MakeEdit(EDT_TRANSFER_TARGET, sx + 150, sy + 2, edW, edH);
        MakeLabel("Amount:", sx + 5, sy + gap + 5, 150, 22, 1);
        MakeEdit(EDT_TRANSFER_AMT, sx + 150, sy + gap + 2, edW, edH);
        MakeBtn(BTN_DO_TRANSFER, "Transfer", sx + 30, sy + gap*2 + 10, btnW, btnH, 0x009C27B0);
        break;
    }
    case SCR_ATM_MINISTATE: break;
    case SCR_ATM_CHANGEPIN: {
        MakeLabel("Current PIN:", sx + 5, sy + 5, 150, 22, 1);
        MakeEdit(EDT_PIN, sx + 170, sy + 2, edW, edH, true);
        MakeLabel("New PIN:", sx + 5, sy + gap + 5, 150, 22, 1);
        MakeEdit(EDT_NEWPIN, sx + 170, sy + gap + 2, edW, edH, true);
        MakeLabel("Confirm New PIN:", sx + 5, sy + gap*2 + 5, 150, 22, 1);
        MakeEdit(EDT_CONFIRMPIN, sx + 170, sy + gap*2 + 2, edW, edH, true);
        MakeBtn(BTN_DO_CHANGEPIN, "Change PIN", sx + 30, sy + gap*3 + 10, btnW, btnH, 0x00FF5722);
        break;
    }
    case SCR_ATM_INFO: break;
    }

    InvalidateRect(gHwnd, NULL, TRUE);
}

// ============================
// COMMAND HANDLER
// ============================

static void HandleCommand(int id) {
    if (id == BTN_LOGOUT) {
        g.currentAccIdx = -1;
        GoToScreen(SCR_LOGIN);
        return;
    }

    if (id >= BTN_SIDEBAR && id < BTN_SIDEBAR + 100) {
        int idx = id - BTN_SIDEBAR;
        if (idx >= 0 && idx < NUM_SIDEBAR_ITEMS) {
            GoToScreen(sidebarItems[idx].screen);
        }
        return;
    }

    switch (id) {
    case BTN_DO_ADMIN_LOGIN:
        GoToScreen(SCR_ADMIN_LOGIN);
        break;

    case BTN_DO_LOGIN: {
        string accStr = GetEditText(EDT_ACCNO);
        string pin = GetEditText(EDT_PIN);
        if (accStr.empty() || pin.empty()) {
            SetStatus("Please enter Account # and PIN", 2);
            break;
        }
        int accNo = atoi(accStr.c_str());
        loadAccounts(g.accounts);
        int idx = findAccountIndex(accNo, g.accounts);
        if (idx < 0) {
            SetStatus("Account not found", 2);
            break;
        }
        if (g.accounts[idx].status == "frozen") {
            SetStatus("Account is frozen. Contact admin.", 2);
            break;
        }
        if (g.accounts[idx].status == "locked") {
            SetStatus("Account is locked. Too many failed attempts.", 2);
            break;
        }
        string stored = decodePIN(g.accounts[idx].pinHash);
        if (pin != stored) {
            g.accounts[idx].pinAttempts++;
            if (g.accounts[idx].pinAttempts >= MAX_PIN_ATTEMPTS) {
                g.accounts[idx].status = "locked";
                saveAccounts(g.accounts);
                SetStatus("Account locked after 3 failed attempts", 2);
            } else {
                saveAccounts(g.accounts);
                stringstream ss;
                ss << "Wrong PIN. " << MAX_PIN_ATTEMPTS - g.accounts[idx].pinAttempts << " attempts left";
                SetStatus(ss.str(), 2);
            }
            break;
        }
        g.accounts[idx].pinAttempts = 0;
        saveAccounts(g.accounts);
        g.currentAccIdx = idx;
        g.isAdmin = false;
        logAudit("ATM Login", "Account " + to_string(accNo) + " logged in");
        loadTransactions(g.transactions);
        GoToScreen(SCR_ATM_MENU);
        break;
    }

    case BTN_DO_CREATE: {
        string name = GetEditText(EDT_NAME);
        string cnic = GetEditText(EDT_CNIC);
        string type = GetEditText(EDT_TYPE);
        string balStr = GetEditText(EDT_BALANCE);
        string pin = GetEditText(EDT_PIN);
        string pin2 = GetEditText(EDT_CONFIRMPIN);

        if (!isValidName(name)) { SetStatus("Invalid name (letters and spaces only)", 2); break; }
        if (!isValidCNIC(cnic)) { SetStatus("Invalid CNIC format (XXXXX-XXXXXXX-X)", 2); break; }
        if (!isValidAccountType(type)) { SetStatus("Type must be 'savings' or 'current'", 2); break; }
        if (balStr.empty()) { SetStatus("Enter initial balance", 2); break; }
        double bal = atof(balStr.c_str());
        if (!isPositiveAmount(bal)) { SetStatus("Balance must be positive", 2); break; }
        if (!isValidPIN(pin)) { SetStatus("PIN must be 4 digits", 2); break; }
        if (pin != pin2) { SetStatus("PINs do not match", 2); break; }

        loadAccounts(g.accounts);
        Account acc;
        acc.accountNo = getNextAccountNo(g.accounts);
        acc.name = name;
        acc.cnic = cnic;
        acc.accountType = type;
        acc.balance = bal;
        acc.status = "active";
        acc.pinHash = encodePIN(pin);
        acc.pinAttempts = 0;
        acc.dailyWithdrawn = 0;
        acc.creationDate = getCurrentDateTimeStr();
        acc.lastWithdrawalDate = getCurrentDateStr();

        g.accounts.push_back(acc);
        saveAccounts(g.accounts);
        appendAccount(acc);

        logAudit("Account Created", "Account " + to_string(acc.accountNo) + " created for " + name);
        stringstream ss;
        ss << "Account #" << acc.accountNo << " created successfully!";
        SetStatus(ss.str(), 1);
        ClearControls();
        break;
    }

    case BTN_DO_SEARCH: {
        string accStr = GetEditText(EDT_SEARCH);
        if (accStr.empty()) { SetStatus("Enter account number", 2); break; }
        int accNo = atoi(accStr.c_str());
        loadAccounts(g.accounts);
        int idx = findAccountIndex(accNo, g.accounts);
        if (idx < 0) { SetStatus("Account not found", 2); break; }
        g.currentAccIdx = idx;
        stringstream ss;
        ss << "Found: " << g.accounts[idx].name << " | Rs. " << fixed << setprecision(2) << g.accounts[idx].balance;
        SetStatus(ss.str(), 1);
        break;
    }

    case BTN_DO_FREEZE: {
        string accStr = GetEditText(EDT_SEARCH);
        if (accStr.empty()) { SetStatus("Enter account number", 2); break; }
        int accNo = atoi(accStr.c_str());
        loadAccounts(g.accounts);
        int idx = findAccountIndex(accNo, g.accounts);
        if (idx < 0) { SetStatus("Account not found", 2); break; }
        g.accounts[idx].status = "frozen";
        saveAccounts(g.accounts);
        logAudit("Account Frozen", "Account " + to_string(accNo) + " frozen");
        SetStatus("Account frozen successfully", 1);
        break;
    }

    case BTN_DO_UNFREEZE: {
        string accStr = GetEditText(EDT_SEARCH);
        if (accStr.empty()) { SetStatus("Enter account number", 2); break; }
        int accNo = atoi(accStr.c_str());
        loadAccounts(g.accounts);
        int idx = findAccountIndex(accNo, g.accounts);
        if (idx < 0) { SetStatus("Account not found", 2); break; }
        g.accounts[idx].status = "active";
        saveAccounts(g.accounts);
        logAudit("Account Unfrozen", "Account " + to_string(accNo) + " unfrozen");
        SetStatus("Account unfrozen successfully", 1);
        break;
    }

    case BTN_DO_UNLOCK: {
        string accStr = GetEditText(EDT_SEARCH);
        if (accStr.empty()) { SetStatus("Enter account number", 2); break; }
        int accNo = atoi(accStr.c_str());
        loadAccounts(g.accounts);
        int idx = findAccountIndex(accNo, g.accounts);
        if (idx < 0) { SetStatus("Account not found", 2); break; }
        g.accounts[idx].status = "active";
        g.accounts[idx].pinAttempts = 0;
        saveAccounts(g.accounts);
        logAudit("Account Unlocked", "Account " + to_string(accNo) + " unlocked, PIN reset");
        SetStatus("Account unlocked, PIN attempts reset", 1);
        break;
    }

    case BTN_DO_DEPOSIT: {
        string accStr = GetEditText(EDT_ACCNO);
        string amtStr = GetEditText(EDT_AMT);
        if (accStr.empty() || amtStr.empty()) { SetStatus("Fill in all fields", 2); break; }
        int accNo = atoi(accStr.c_str());
        double amt = atof(amtStr.c_str());
        if (!isValidAmount(amt)) { SetStatus("Invalid amount", 2); break; }
        loadAccounts(g.accounts);
        int idx = findAccountIndex(accNo, g.accounts);
        if (idx < 0) { SetStatus("Account not found", 2); break; }
        g.accounts[idx].balance += amt;
        saveAccounts(g.accounts);

        Transaction txn;
        txn.transactionID = generateTransactionID(g.transactions);
        txn.accountNo = accNo;
        txn.type = "deposit";
        txn.amount = amt;
        txn.dateTime = getCurrentDateTimeStr();
        txn.resultingBalance = g.accounts[idx].balance;
        txn.details = "Cash deposit";
        g.transactions.push_back(txn);
        saveTransactions(g.transactions);
        appendTransaction(txn);
        generateReceipt(txn, g.accounts[idx]);

        logAudit("Deposit", "Account " + to_string(accNo) + " deposited Rs. " + to_string(amt));
        stringstream ss;
        ss << "Deposited Rs. " << fixed << setprecision(2) << amt << " successfully";
        SetStatus(ss.str(), 1);
        ClearControls();
        break;
    }

    case BTN_DO_WITHDRAW: {
        if (g.currentAccIdx < 0 || g.currentAccIdx >= (int)g.accounts.size()) {
            SetStatus("No account selected", 2);
            break;
        }
        string amtStr = GetEditText(EDT_AMT);
        if (amtStr.empty()) { SetStatus("Enter amount", 2); break; }
        double amt = atof(amtStr.c_str());
        if (!isValidAmount(amt)) { SetStatus("Invalid amount", 2); break; }
        auto& acc = g.accounts[g.currentAccIdx];
        if (acc.balance < amt) { SetStatus("Insufficient balance", 2); break; }
        if (acc.dailyWithdrawn + amt > DAILY_WITHDRAWAL_LIMIT) {
            SetStatus("Exceeds daily withdrawal limit (Rs. 50,000)", 2);
            break;
        }
        if (!dispenseCash(g.inventory, amt)) {
            SetStatus("ATM has insufficient cash", 2);
            break;
        }
        acc.balance -= amt;
        acc.dailyWithdrawn += amt;
        saveAccounts(g.accounts);

        Transaction txn;
        txn.transactionID = generateTransactionID(g.transactions);
        txn.accountNo = acc.accountNo;
        txn.type = "withdrawal";
        txn.amount = amt;
        txn.dateTime = getCurrentDateTimeStr();
        txn.resultingBalance = acc.balance;
        txn.details = "ATM cash withdrawal";
        g.transactions.push_back(txn);
        saveTransactions(g.transactions);
        appendTransaction(txn);
        generateReceipt(txn, acc);

        logAudit("Withdrawal", "Account " + to_string(acc.accountNo) + " withdrew Rs. " + to_string(amt));
        stringstream ss;
        ss << "Withdrawn Rs. " << fixed << setprecision(2) << amt << " successfully";
        SetStatus(ss.str(), 1);
        ClearControls();
        break;
    }

    case BTN_DO_TRANSFER: {
        if (g.currentAccIdx < 0 || g.currentAccIdx >= (int)g.accounts.size()) {
            SetStatus("No account selected", 2);
            break;
        }
        string targetStr = GetEditText(EDT_TRANSFER_TARGET);
        string amtStr = GetEditText(EDT_TRANSFER_AMT);
        if (targetStr.empty() || amtStr.empty()) { SetStatus("Fill in all fields", 2); break; }
        int targetAcc = atoi(targetStr.c_str());
        double amt = atof(amtStr.c_str());
        if (!isValidAmount(amt)) { SetStatus("Invalid amount", 2); break; }
        int senderIdx = g.currentAccIdx;
        int receiverIdx = findAccountIndex(targetAcc, g.accounts);
        if (receiverIdx < 0) { SetStatus("Target account not found", 2); break; }
        if (senderIdx == receiverIdx) { SetStatus("Cannot transfer to same account", 2); break; }
        if (g.accounts[senderIdx].balance < amt) { SetStatus("Insufficient balance", 2); break; }

        if (amt >= OTP_THRESHOLD) {
            g.otpCode = generateOTP();
            stringstream ss;
            ss << "OTP " << g.otpCode << " generated for large transfer";
            SetStatus(ss.str(), 0);
            break;
        }

        g.accounts[senderIdx].balance -= amt;
        g.accounts[receiverIdx].balance += amt;
        saveAccounts(g.accounts);

        Transaction txn;
        txn.transactionID = generateTransactionID(g.transactions);
        txn.accountNo = g.accounts[senderIdx].accountNo;
        txn.type = "transfer";
        txn.amount = amt;
        txn.dateTime = getCurrentDateTimeStr();
        txn.resultingBalance = g.accounts[senderIdx].balance;
        txn.details = "Transfer to " + to_string(targetAcc);
        g.transactions.push_back(txn);
        saveTransactions(g.transactions);
        appendTransaction(txn);
        generateReceipt(txn, g.accounts[senderIdx]);

        logAudit("Transfer", "Account " + to_string(g.accounts[senderIdx].accountNo) +
                 " transferred Rs. " + to_string(amt) + " to " + to_string(targetAcc));
        stringstream ss;
        ss << "Transferred Rs. " << fixed << setprecision(2) << amt << " successfully";
        SetStatus(ss.str(), 1);
        ClearControls();
        break;
    }

    case BTN_DO_CHANGEPIN: {
        if (g.currentAccIdx < 0 || g.currentAccIdx >= (int)g.accounts.size()) {
            SetStatus("No account selected", 2);
            break;
        }
        string curPin = GetEditText(EDT_PIN);
        string newPin = GetEditText(EDT_NEWPIN);
        string confPin = GetEditText(EDT_CONFIRMPIN);
        if (curPin.empty() || newPin.empty() || confPin.empty()) {
            SetStatus("Fill in all fields", 2);
            break;
        }
        string stored = decodePIN(g.accounts[g.currentAccIdx].pinHash);
        if (curPin != stored) { SetStatus("Current PIN is incorrect", 2); break; }
        if (!isValidPIN(newPin)) { SetStatus("PIN must be 4 digits", 2); break; }
        if (newPin != confPin) { SetStatus("New PINs do not match", 2); break; }
        g.accounts[g.currentAccIdx].pinHash = encodePIN(newPin);
        saveAccounts(g.accounts);
        logAudit("PIN Changed", "Account " + to_string(g.accounts[g.currentAccIdx].accountNo) + " PIN changed");
        SetStatus("PIN changed successfully", 1);
        ClearControls();
        break;
    }

    case BTN_DO_SEARCH_TXN: {
        loadTransactions(g.transactions);
        string accStr = GetEditText(EDT_SEARCH_TXN_ACC);
        string type = GetEditText(EDT_SEARCH_TXN_TYPE);
        string from = GetEditText(EDT_SEARCH_TXN_FROM);
        string to = GetEditText(EDT_SEARCH_TXN_TO);
        string minS = GetEditText(EDT_SEARCH_TXN_MIN);
        string maxS = GetEditText(EDT_SEARCH_TXN_MAX);

        int accNo = accStr.empty() ? -1 : atoi(accStr.c_str());
        double minAmt = minS.empty() ? -1 : atof(minS.c_str());
        double maxAmt = maxS.empty() ? -1 : atof(maxS.c_str());

        g.transactions = filterTransactions(g.transactions, accNo, type, from, to, minAmt, maxAmt);
        stringstream ss;
        ss << "Found " << g.transactions.size() << " matching transactions";
        SetStatus(ss.str(), 1);
        break;
    }

    case BTN_DO_LOAN: {
        string accStr = GetEditText(EDT_LOAN_ACC);
        string amtStr = GetEditText(EDT_LOAN_AMT);
        string termStr = GetEditText(EDT_LOAN_TERM);
        if (accStr.empty() || amtStr.empty() || termStr.empty()) {
            SetStatus("Fill in all fields", 2);
            break;
        }
        int accNo = atoi(accStr.c_str());
        double amt = atof(amtStr.c_str());
        int term = atoi(termStr.c_str());
        loadAccounts(g.accounts);
        int idx = findAccountIndex(accNo, g.accounts);
        if (idx < 0) { SetStatus("Account not found", 2); break; }
        if (!isValidLoanAmount(amt, g.accounts[idx].balance)) {
            SetStatus("Loan amount exceeds 50% of balance", 2);
            break;
        }

        double rate = 5.0;
        double monthly = (amt * (1 + rate * term / 1200.0)) / term;

        Loan loan;
        loan.loanId = getNextLoanID(g.loans);
        loan.accountNo = accNo;
        loan.amount = amt;
        loan.interestRate = rate;
        loan.termMonths = term;
        loan.monthlyPayment = monthly;
        loan.status = "active";
        loan.applicationDate = getCurrentDateTimeStr();
        loan.remainingAmount = amt;
        loan.monthsPaid = 0;

        g.loans.push_back(loan);
        saveLoans(g.loans);
        appendLoan(loan);

        g.accounts[idx].balance += amt;
        saveAccounts(g.accounts);

        logAudit("Loan Approved", "Loan " + to_string(loan.loanId) + " of Rs. " + to_string(amt) + " for account " + to_string(accNo));
        stringstream ss;
        ss << "Loan #" << loan.loanId << " approved! Rs. " << fixed << setprecision(0) << amt;
        SetStatus(ss.str(), 1);
        break;
    }

    case BTN_DO_BACKUP: {
        if (backupData()) {
            logAudit("Backup", "System data backup created");
            SetStatus("Backup created successfully", 1);
        } else {
            SetStatus("Backup failed", 2);
        }
        break;
    }

    case BTN_CLEAR_CREATE:
        ClearControls();
        CreateScreenControls();
        break;
    }
}

static void GoToScreen(Screen scr) {
    g.screen = scr;
    g.statusMsg.clear();
    CreateScreenControls();
    RefreshScreen();
}

// ============================
// Owner-drawn button painting
// ============================
static void DrawBtnRaw(DRAWITEMSTRUCT* di, int customColor) {
    HDC hdc = di->hDC;
    RECT rc = di->rcItem;
    bool pressed = di->itemState & ODS_SELECTED;
    bool focused = di->itemState & ODS_FOCUS;

    COLORREF bgClr = (customColor != 0) ? (COLORREF)customColor : RGB(100,100,120);
    if (pressed) {
        int r = max(0, GetRValue(bgClr) - 30);
        int g2 = max(0, GetGValue(bgClr) - 30);
        int b = max(0, GetBValue(bgClr) - 30);
        bgClr = RGB(r, g2, b);
    }

    HBRUSH br = CreateSolidBrush(bgClr);
    HPEN pen = CreatePen(PS_SOLID, 1, RGB(max(0,GetRValue(bgClr)-20), max(0,GetGValue(bgClr)-20), max(0,GetBValue(bgClr)-20)));
    HPEN oldPen = (HPEN)SelectObject(hdc, pen);
    HBRUSH oldBr = (HBRUSH)SelectObject(hdc, br);
    RoundRect(hdc, rc.left, rc.top, rc.right, rc.bottom, 8, 8);
    SelectObject(hdc, oldPen);
    SelectObject(hdc, oldBr);
    DeleteObject(br);
    DeleteObject(pen);

    char txt[256] = {0};
    GetWindowText(di->hwndItem, txt, 255);

    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, RGB(255,255,255));
    HFONT oldFont = (HFONT)SelectObject(hdc, hFontBold);
    DrawText(hdc, txt, -1, &rc, DT_CENTER|DT_VCENTER|DT_SINGLELINE);
    SelectObject(hdc, oldFont);

    if (focused) {
        HPEN focusPen = CreatePen(PS_DOT, 1, RGB(255,255,255));
        HPEN oldFP = (HPEN)SelectObject(hdc, focusPen);
        HBRUSH oldFB = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));
        DrawFocusRect(hdc, &rc);
        SelectObject(hdc, oldFP);
        SelectObject(hdc, oldFB);
        DeleteObject(focusPen);
    }
}

// ============================
// WINDOW PROCEDURE
// ============================
static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch(msg) {
    case WM_CREATE:
        gHwnd = hwnd;
        InitGDI();
        return 0;

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        RECT rc;
        GetClientRect(hwnd, &rc);
        PaintScreen(hdc, rc);
        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_LBUTTONDOWN: {
        int hit = SidebarHitTest(lp, g);
        if (hit >= 0) {
            Screen target = sidebarItems[hit].screen;
            GoToScreen(target);
            logAudit("Navigation", string("Sidebar -> ") + sidebarItems[hit].label);
        }
        return 0;
    }

    case WM_MOUSEMOVE: {
        int mx = LOWORD(lp);
        int my = HIWORD(lp);
        if (mx < SIDEBAR_W && my > HEADER_H) {
            SetCursor(LoadCursor(NULL, IDC_HAND));
        } else {
            SetCursor(LoadCursor(NULL, IDC_ARROW));
        }
        return 0;
    }

    case WM_SETCURSOR: {
        POINT pt;
        GetCursorPos(&pt);
        ScreenToClient(hwnd, &pt);
        if (pt.x < SIDEBAR_W && pt.y > HEADER_H) {
            SetCursor(LoadCursor(NULL, IDC_HAND));
            return TRUE;
        }
        return DefWindowProc(hwnd, msg, wp, lp);
    }

    case WM_COMMAND:
        HandleCommand((int)LOWORD(wp));
        return 0;

    case WM_DRAWITEM: {
        DRAWITEMSTRUCT* di = (DRAWITEMSTRUCT*)lp;
        if (di->CtlType == ODT_BUTTON) {
            int clr = (int)GetWindowLongPtr(di->hwndItem, GWL_USERDATA);
            DrawBtnRaw(di, clr);
        }
        return TRUE;
    }

    case WM_CTLCOLOREDIT: {
        HDC hdc2 = (HDC)wp;
        SetTextColor(hdc2, RGB(40,40,50));
        SetBkColor(hdc2, RGB(248,250,252));
        static HBRUSH hBr = NULL;
        if (hBr) DeleteObject(hBr);
        hBr = CreateSolidBrush(RGB(248,250,252));
        return (LRESULT)hBr;
    }

    case WM_CTLCOLORSTATIC: {
        HDC hdc2 = (HDC)wp;
        SetTextColor(hdc2, RGB(40,40,50));
        SetBkMode(hdc2, TRANSPARENT);
        return (LRESULT)GetStockObject(NULL_BRUSH);
    }

    case WM_ERASEBKGND:
        return 1;

    case WM_DESTROY:
        DestroyFonts();
        DeleteObject(hBrushPrimary); DeleteObject(hBrushCard);
        DeleteObject(hBrushBg); DeleteObject(hBrushSidebar);
        DeleteObject(hBrushAccent); DeleteObject(hBrushHover);
        DeleteObject(hPenCardBorder); DeleteObject(hPenShadow);
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hwnd, msg, wp, lp);
}

// ============================
// WINMAIN
// ============================
int WINAPI WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, int) {
    g.screen = SCR_LOGIN;
    g.currentAccIdx = -1;
    g.isAdmin = false;

    loadAccounts(g.accounts);
    loadTransactions(g.transactions);
    loadLoans(g.loans);
    g.inventory = loadCashInventory(g.inventory);
    if (g.inventory.empty()) initCashInventory(g.inventory);

    WNDCLASSEX wc = {};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW|CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInst;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
    wc.lpszClassName = "NationalBankGUI";
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.hIconSm = LoadIcon(NULL, IDI_APPLICATION);
    RegisterClassEx(&wc);

    int screenW = GetSystemMetrics(SM_CXSCREEN);
    int screenH = GetSystemMetrics(SM_CYSCREEN);
    int wx = (screenW - WIN_W) / 2;
    int wy = (screenH - WIN_H) / 2;

    HWND hwnd = CreateWindowEx(
        0, "NationalBankGUI", "National Bank - Banking System",
        WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX,
        wx, wy, WIN_W, WIN_H,
        NULL, NULL, hInst, NULL);

    if (!hwnd) return 1;

    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);

    gHwnd = hwnd;
    CreateScreenControls();

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        if (IsDialogMessage(hwnd, &msg)) continue;
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return (int)msg.wParam;
}
'''

outpath = os.path.join(os.path.dirname(os.path.abspath(__file__)), "src", "gui.cpp")
with open(outpath, "w", encoding="utf-8") as f:
    f.write(code)

print(f"Written {len(code)} bytes to {outpath}")
