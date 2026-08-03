#include "gui.h"
#include "theme.h"
#include "banking.h"
#include "core/fileio.h"
#include "core/validation.h"
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cmath>


using namespace Theme;

const string ADMIN_PASSWORD = "admin123";

// ============================
// VOICE ASSISTANT
// ============================
struct SpeakData {
    string text;
};

static DWORD WINAPI SpeakThread(LPVOID lpParam) {
    SpeakData* data = (SpeakData*)lpParam;
    string escaped = data->text;
    delete data;

    size_t pos = 0;
    while ((pos = escaped.find("'", pos)) != string::npos) {
        escaped.insert(pos, "'");
        pos += 2;
    }

    string cmd = "powershell -NoProfile -Command \"Add-Type -AssemblyName System.Speech; "
                 "$s=New-Object System.Speech.Synthesis.SpeechSynthesizer; "
                 "$s.Speak('" + escaped + "')\"";

    SECURITY_ATTRIBUTES sa = { sizeof(SECURITY_ATTRIBUTES), NULL, TRUE };
    PROCESS_INFORMATION pi = {};
    STARTUPINFO si = {};
    si.cb = sizeof(STARTUPINFO);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    CreateProcess(NULL, &cmd[0], &sa, &sa, TRUE,
                  CREATE_NO_WINDOW, NULL, NULL, &si, &pi);
    if (pi.hProcess) {
        WaitForSingleObject(pi.hProcess, 8000);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }
    return 0;
}

static void SpeakText(const string& text) {
    SpeakData* data = new SpeakData();
    data->text = text;
    CreateThread(NULL, 0, SpeakThread, data, 0, NULL);
}

static string ScreenGuide(Screen screen) {
    switch (screen) {
        case SCR_LOGIN:      return "Welcome to National Bank. Please select Bank Administrator for admin panel, or ATM Customer for ATM services.";
        case SCR_ADMIN_LOGIN:return "Admin login. Please enter your administrator password.";
        case SCR_ADMIN_DASH: return "Admin dashboard. View account overview, recent transactions, and system statistics.";
        case SCR_ADMIN_CREATE:return "Create a new bank account. Fill in name, CNIC, account type, balance, and PIN.";
        case SCR_ADMIN_VIEW: return "View all registered bank accounts with their details and status.";
        case SCR_ADMIN_SEARCH:return "Search for a specific account by account number.";
        case SCR_ADMIN_FREEZE:return "Freeze an account to disable all ATM transactions.";
        case SCR_ADMIN_UNFREEZE:return "Unfreeze a previously frozen account.";
        case SCR_ADMIN_UNLOCK:return "Unlock a locked account and reset PIN attempts.";
        case SCR_ADMIN_TXNS: return "View complete transaction history for all accounts.";
        case SCR_ADMIN_SEARCH_TXN:return "Search transactions by account number, type, date range, or amount.";
        case SCR_ADMIN_AUDIT: return "View system audit log with all administrative actions.";
        case SCR_ADMIN_LOANS: return "Loan management. Approve new loans and view existing loans.";
        case SCR_ADMIN_CASH: return "View ATM cash inventory and denomination counts.";
        case SCR_ADMIN_DAILY:return "View daily summary report of deposits, withdrawals, and transfers.";
        case SCR_ATM_LOGIN:  return "ATM login. Enter your account number and PIN to access ATM services.";
        case SCR_ATM_MENU:   return "ATM main menu. Choose from balance inquiry, deposit, withdraw, transfer, mini statement, or change PIN.";
        case SCR_ATM_BALANCE:return "Check your account balance by entering your account number and PIN.";
        case SCR_ATM_DEPOSIT:return "Deposit cash into your account. Enter account number and amount.";
        case SCR_ATM_WITHDRAW:return "Withdraw cash from your account. Enter the amount to withdraw.";
        case SCR_ATM_TRANSFER:return "Transfer funds to another account. Enter target account number and amount.";
        case SCR_ATM_MINISTATE:return "View your recent transactions and mini statement.";
        case SCR_ATM_CHANGEPIN:return "Change your account PIN. Enter current PIN, new PIN, and confirm.";
        case SCR_ATM_INFO:  return "ATM information screen showing machine details and cash availability.";
        case SCR_OTP:       return "OTP verification. A one time password has been sent for large transfer verification. Please enter the OTP code.";
        default: return "";
    }
}

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
    hFontTitle   = CreateFont(26,0,0,0,FW_BOLD,0,0,0,0,0,0,0,0,"Segoe UI");
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
    HBRUSH br = CreateSolidBrush(Primary);
    FillRect(hdc, &rc, br);
    DeleteObject(br);
    
    // Bottom accent border line
    HPEN borderPen = CreatePen(PS_SOLID, 1, RGB(30, 41, 59));
    HPEN old = (HPEN)SelectObject(hdc, borderPen);
    MoveToEx(hdc, 0, rc.bottom - 1, NULL);
    LineTo(hdc, rc.right, rc.bottom - 1);
    SelectObject(hdc, old);
    DeleteObject(borderPen);
}

static void DrawShadow(HDC hdc, int x, int y, int w, int h) {
    // Subtle outer border glow
    HPEN shadowPen = CreatePen(PS_SOLID, 1, RGB(226, 232, 240));
    HBRUSH oldBr = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));
    HPEN oldPen = (HPEN)SelectObject(hdc, shadowPen);
    RoundRect(hdc, x-1, y-1, x+w+1, y+h+1, 14, 14);
    SelectObject(hdc, oldPen);
    SelectObject(hdc, oldBr);
    DeleteObject(shadowPen);
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

static int gHoverSidebarIdx = -1;

static void DrawSidebar(HDC hdc, RECT rc, AppState& s) {
    RECT sbRc = {0, HEADER_H, SIDEBAR_W, rc.bottom};
    FillRect(hdc, &sbRc, hBrushSidebar);

    // Right subtle divider line
    HPEN divPen = CreatePen(PS_SOLID, 1, RGB(30, 41, 59));
    HPEN oldDiv = (HPEN)SelectObject(hdc, divPen);
    MoveToEx(hdc, SIDEBAR_W - 1, HEADER_H, NULL);
    LineTo(hdc, SIDEBAR_W - 1, rc.bottom);
    SelectObject(hdc, oldDiv);
    DeleteObject(divPen);

    // Sidebar Category Title
    Txt(hdc, s.isAdmin ? "ADMINISTRATION" : "ATM SERVICES", 20, HEADER_H + 18, hFontSmall, RGB(100, 116, 139));

    int yStart = HEADER_H + 42;
    int itemH = 38;

    int startIdx = s.isAdmin ? 0 : 14;
    int count = s.isAdmin ? 13 : (NUM_SIDEBAR_ITEMS - 14);

    for (int i = 0; i < count; i++) {
        int idx = startIdx + i;
        int y = yStart + i * itemH;
        bool selected = (s.screen == sidebarItems[idx].screen);
        bool hovered = (idx == gHoverSidebarIdx);

        RECT itemRc = {12, y, SIDEBAR_W - 12, y + itemH - 4};
        if (selected) {
            HBRUSH selBg = CreateSolidBrush(RGB(30, 41, 59));
            HPEN selPen = CreatePen(PS_SOLID, 1, RGB(51, 65, 85));
            RoundRect2(hdc, itemRc.left, itemRc.top, itemRc.right - itemRc.left, itemRc.bottom - itemRc.top, 12, selBg, selPen);
            DeleteObject(selBg);
            DeleteObject(selPen);

            // Active pill bar
            HBRUSH pillBr = CreateSolidBrush(Secondary);
            HPEN oldP = (HPEN)SelectObject(hdc, GetStockObject(NULL_PEN));
            HBRUSH oldB = (HBRUSH)SelectObject(hdc, pillBr);
            RoundRect(hdc, 12, y + 4, 16, y + itemH - 8, 4, 4);
            SelectObject(hdc, oldP);
            SelectObject(hdc, oldB);
            DeleteObject(pillBr);
        } else if (hovered) {
            HBRUSH hovBg = CreateSolidBrush(RGB(24, 34, 53));
            HPEN hovPen = CreatePen(PS_SOLID, 1, RGB(40, 53, 72));
            RoundRect2(hdc, itemRc.left, itemRc.top, itemRc.right - itemRc.left, itemRc.bottom - itemRc.top, 12, hovBg, hovPen);
            DeleteObject(hovBg);
            DeleteObject(hovPen);
        }

        Txt(hdc, sidebarItems[idx].label, 26, y + 8, (selected || hovered) ? hFontBold : hFontNormal,
            selected ? RGB(255,255,255) : (hovered ? RGB(226,232,240) : RGB(148,163,184)));
    }
}

static int SidebarHitTest(LPARAM lp, AppState& s) {
    int mx = LOWORD(lp);
    int my = HIWORD(lp);
    if (mx >= SIDEBAR_W || my <= HEADER_H) return -1;

    int yStart = HEADER_H + 42;
    int itemH = 38;

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

    // Bank Icon badge
    HBRUSH iconBr = CreateSolidBrush(Accent);
    HPEN oldP = (HPEN)SelectObject(hdc, GetStockObject(NULL_PEN));
    HBRUSH oldB = (HBRUSH)SelectObject(hdc, iconBr);
    RoundRect(hdc, 20, 14, 55, 49, 10, 10);
    SelectObject(hdc, oldP);
    SelectObject(hdc, oldB);
    DeleteObject(iconBr);

    TxtCenter(hdc, "NB", 20, 14, 35, 35, hFontBold, RGB(255,255,255));

    Txt(hdc, "NATIONAL BANK", 68, 14, hFontHeading, RGB(255,255,255));
    Txt(hdc, "Secure Banking Platform", 68, 38, hFontSmall, RGB(148,163,184));

    // Show current user/mode label (left-aligned after bank name)
    const char* modeLabel = s.isAdmin ? "Admin Portal" :
                            (s.currentAccIdx >= 0 ? "ATM Session" : "");
    if (modeLabel[0] != '\0') {
        HBRUSH modeBg = CreateSolidBrush(RGB(30,41,59));
        HPEN modePen = CreatePen(PS_SOLID, 1, RGB(51,65,85));
        RoundRect2(hdc, 280, 20, 120, 26, 8, modeBg, modePen);
        DeleteObject(modeBg);
        DeleteObject(modePen);
        TxtCenter(hdc, modeLabel, 280, 20, 120, 26, hFontSmall, RGB(148,163,184));
    }
}

static void DrawStatus(HDC hdc, RECT rc, AppState& s) {
    RECT stRc = {0, rc.bottom - STATUS_H, rc.right, rc.bottom};
    FillRect(hdc, &stRc, hBrushPrimary);

    HPEN linePen = CreatePen(PS_SOLID, 1, RGB(30, 41, 59));
    HPEN old = (HPEN)SelectObject(hdc, linePen);
    MoveToEx(hdc, 0, stRc.top, NULL);
    LineTo(hdc, rc.right, stRc.top);
    SelectObject(hdc, old);
    DeleteObject(linePen);

    // Status Message if present
    if (!s.statusMsg.empty()) {
        COLORREF dotClr = (s.statusType == 1) ? Success : (s.statusType == 2 ? Error : Secondary);
        HBRUSH dotBr = CreateSolidBrush(dotClr);
        HPEN oldP = (HPEN)SelectObject(hdc, GetStockObject(NULL_PEN));
        HBRUSH oldB = (HBRUSH)SelectObject(hdc, dotBr);
        Ellipse(hdc, 15, stRc.top + 12, 23, stRc.top + 20);
        SelectObject(hdc, oldP);
        SelectObject(hdc, oldB);
        DeleteObject(dotBr);

        Txt(hdc, s.statusMsg.c_str(), 30, stRc.top + 7, hFontBold, dotClr);
    } else {
        Txt(hdc, "National Bank  |  Ready", 15, stRc.top + 7, hFontSmall, RGB(148,163,184));
    }

    Txt(hdc, "System v2.5", rc.right - 100, stRc.top + 7, hFontSmall, RGB(148,163,184));
}

// --- Card / Panel helpers ---
static void DrawCard(HDC hdc, int x, int y, int w, int h) {
    DrawShadow(hdc, x, y, w, h);
    RoundRect2(hdc, x, y, w, h, 10, hBrushCard, hPenCardBorder);
}

// --- Control Management ---
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

static AppState g;
static vector<EditInfo> gEdits;
static vector<ButtonInfo> gButtons;
static vector<LabelInfo> gLabels;
static HWND gHwnd = NULL;

static void ClearControls() {
    for (auto& e : gEdits) if (e.hwnd) DestroyWindow(e.hwnd);
    for (auto& b : gButtons) if (b.hwnd) DestroyWindow(b.hwnd);
    for (auto& l : gLabels) if (l.hwnd) DestroyWindow(l.hwnd);
    gEdits.clear();
    gButtons.clear();
    gLabels.clear();
    g.childControls.clear();
}

static WNDPROC gOldBtnProc = NULL;

static LRESULT CALLBACK BtnSubclassProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_MOUSEMOVE: {
        if (!GetProp(hwnd, "HOVER")) {
            SetProp(hwnd, "HOVER", (HANDLE)1);
            TRACKMOUSEEVENT tme = { sizeof(tme), TME_LEAVE, hwnd, 0 };
            TrackMouseEvent(&tme);
            InvalidateRect(hwnd, NULL, FALSE);
        }
        break;
    }
    case WM_MOUSELEAVE: {
        RemoveProp(hwnd, "HOVER");
        InvalidateRect(hwnd, NULL, FALSE);
        break;
    }
    case WM_SETCURSOR: {
        SetCursor(LoadCursor(NULL, IDC_HAND));
        return TRUE;
    }
    }
    return CallWindowProc(gOldBtnProc, hwnd, msg, wp, lp);
}

static HWND MakeBtn(int id, const char* text, int x, int y, int w, int h, int color) {
    HWND hw = CreateWindow("BUTTON", text,
        WS_CHILD|WS_VISIBLE|BS_OWNERDRAW,
        x, y, w, h, gHwnd, (HMENU)(LONG_PTR)id, NULL, NULL);
    SetWindowLongPtr(hw, GWL_USERDATA, color);
    if (!gOldBtnProc) {
        gOldBtnProc = (WNDPROC)GetWindowLongPtr(hw, GWLP_WNDPROC);
    }
    SetWindowLongPtr(hw, GWLP_WNDPROC, (LONG_PTR)BtnSubclassProc);
    ButtonInfo bi = {id, hw, color};
    gButtons.push_back(bi);
    return hw;
}

static HWND MakeEdit(int id, int x, int y, int w, int h, bool pwd = false) {
    DWORD style = WS_CHILD|WS_VISIBLE|ES_AUTOHSCROLL;
    if (pwd) style |= ES_PASSWORD;
    HWND hw = CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", "",
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
    int cx = rc.right / 2;
    int cy = HEADER_H + (rc.bottom - HEADER_H - STATUS_H) / 2;

    DrawCard(hdc, cx - 220, cy - 140, 440, 280);
    TxtCenter(hdc, "NATIONAL BANK", cx - 220, cy - 115, 440, 32, hFontTitle, Primary);
    TxtCenter(hdc, "Welcome to the Digital Banking Platform", cx - 220, cy - 80, 440, 20, hFontSmall, TextLight);

    HPEN linePen = CreatePen(PS_SOLID, 1, CardBorder);
    HPEN old = (HPEN)SelectObject(hdc, linePen);
    MoveToEx(hdc, cx - 170, cy - 50, NULL);
    LineTo(hdc, cx + 170, cy - 50);
    SelectObject(hdc, old);
    DeleteObject(linePen);

    TxtCenter(hdc, "Select your portal to log in:", cx - 220, cy - 35, 440, 22, hFontBold, Text);
}

static void PaintATMLoginScreen(HDC hdc, RECT rc) {
    int cx = rc.right / 2;
    int cy = HEADER_H + (rc.bottom - HEADER_H - STATUS_H) / 2;

    DrawCard(hdc, cx - 220, cy - 140, 440, 280);
    TxtCenter(hdc, "ATM Customer Login", cx - 220, cy - 115, 440, 32, hFontTitle, Primary);
    TxtCenter(hdc, "Enter your account number and PIN to access ATM services", cx - 220, cy - 80, 440, 20, hFontSmall, TextLight);

    HPEN linePen = CreatePen(PS_SOLID, 1, CardBorder);
    HPEN old = (HPEN)SelectObject(hdc, linePen);
    MoveToEx(hdc, cx - 170, cy - 50, NULL);
    LineTo(hdc, cx + 170, cy - 50);
    SelectObject(hdc, old);
    DeleteObject(linePen);
}

static void PaintAdminLoginScreen(HDC hdc, RECT rc) {
    int cx = rc.right / 2;
    int cy = HEADER_H + (rc.bottom - HEADER_H - STATUS_H) / 2;

    DrawCard(hdc, cx - 220, cy - 140, 440, 280);
    TxtCenter(hdc, "Administrator Login", cx - 220, cy - 115, 440, 32, hFontTitle, Primary);
    TxtCenter(hdc, "Enter administrator password to access management system", cx - 220, cy - 80, 440, 20, hFontSmall, TextLight);

    HPEN linePen = CreatePen(PS_SOLID, 1, CardBorder);
    HPEN old = (HPEN)SelectObject(hdc, linePen);
    MoveToEx(hdc, cx - 170, cy - 50, NULL);
    LineTo(hdc, cx + 170, cy - 50);
    SelectObject(hdc, old);
    DeleteObject(linePen);
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

    loadAccounts(g.accounts);
    loadTransactions(g.transactions);

    int totalAcc = (int)g.accounts.size();
    int activeAcc = 0;
    double totalBal = 0;
    for (auto& a : g.accounts) {
        if (a.status == "active") activeAcc++;
        totalBal += a.balance;
    }
    int totalTxn = (int)g.transactions.size();

    string strTotalAcc = to_string(totalAcc);
    string strActiveAcc = to_string(activeAcc);
    string strTotalTxn = to_string(totalTxn);
    stringstream balSS;
    balSS << "Rs. " << fixed << setprecision(0) << totalBal;
    string strTotalBal = balSS.str();

    struct CardData { string title; string value; COLORREF clr; };
    CardData cards[] = {
        {"Total Accounts", strTotalAcc, Secondary},
        {"Active Accounts", strActiveAcc, Success},
        {"Total Balance", strTotalBal, Accent},
        {"Transactions", strTotalTxn, RGB(156,39,176)},
    };

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

        Txt(hdc, cards[i].title.c_str(), cx + 60, cy + 15, hFontSmall, TextLight);
        Txt(hdc, cards[i].value.c_str(), cx + 60, cy + 42, hFontHeading, Text);
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
    int sy = HEADER_H + 25;
    int cw = min(560, (int)(rc.right - SIDEBAR_W - 60));

    Txt(hdc, "Create New Account", sx, sy, hFontHeading, Primary);
    Txt(hdc, "Fill in the form below to register a new bank account", sx, sy + 28, hFontSmall, TextLight);
    HPEN linePen = CreatePen(PS_SOLID, 2, Accent);
    HPEN old = (HPEN)SelectObject(hdc, linePen);
    MoveToEx(hdc, sx, sy + 46, NULL);
    LineTo(hdc, sx + 260, sy + 46);
    SelectObject(hdc, old);
    DeleteObject(linePen);

    int cardY = sy + 58;
    int cardH = 6 * 52 + 70;
    DrawCard(hdc, sx, cardY, cw, cardH);
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
    int sy = HEADER_H + 25;
    int cw = min(560, (int)(rc.right - SIDEBAR_W - 60));

    Txt(hdc, "Search Account", sx, sy, hFontHeading, Primary);
    Txt(hdc, "Look up any account by its number", sx, sy + 28, hFontSmall, TextLight);
    HPEN linePen = CreatePen(PS_SOLID, 2, Accent);
    HPEN old = (HPEN)SelectObject(hdc, linePen);
    MoveToEx(hdc, sx, sy + 46, NULL);
    LineTo(hdc, sx + 190, sy + 46);
    SelectObject(hdc, old);
    DeleteObject(linePen);

    // Search input card
    DrawCard(hdc, sx, sy + 58, cw, 80);

    // Results card
    int resultY = sy + 155;
    int resultH = rc.bottom - HEADER_H - STATUS_H - resultY + HEADER_H - 15;
    DrawCard(hdc, sx, resultY, cw, max(resultH, 180));
    int idx = -1;
    if (g.lastSearchAccNo > 0) {
        idx = findAccountIndex(g.lastSearchAccNo, g.accounts);
    }
    if (idx < 0) {
        TxtCenter(hdc, "Enter an account number above and click Search", sx, resultY, cw, 180, hFontSmall, TextLight);
    } else {
        auto& a = g.accounts[idx];
        int y = resultY + 25;
        int lx = sx + 30, vx = sx + 200;
        Txt(hdc, "Account Number:",  lx, y,      hFontSmall, TextLight);
        Txt(hdc, to_string(a.accountNo).c_str(), vx, y, hFontBold, Text);
        Txt(hdc, "Name:",             lx, y+32,   hFontSmall, TextLight);
        Txt(hdc, a.name.c_str(),       vx, y+32,   hFontBold, Text);
        Txt(hdc, "CNIC:",             lx, y+64,   hFontSmall, TextLight);
        Txt(hdc, a.cnic.c_str(),       vx, y+64,   hFontNormal, TextLight);
        Txt(hdc, "Account Type:",     lx, y+96,   hFontSmall, TextLight);
        Txt(hdc, a.accountType.c_str(), vx, y+96,  hFontNormal, Text);
        Txt(hdc, "Balance:",          lx, y+128,  hFontSmall, TextLight);
        stringstream ss; ss << "Rs. " << fixed << setprecision(2) << a.balance;
        Txt(hdc, ss.str().c_str(),    vx, y+126,  hFontHeading, Success);
        Txt(hdc, "Status:",           lx, y+165,  hFontSmall, TextLight);
        COLORREF sc = (a.status=="active") ? Success : (a.status=="frozen" ? Warning : Error);
        Txt(hdc, a.status.c_str(),    vx, y+163,  hFontBold, sc);
    }
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
    int sx = SIDEBAR_W + 35;
    int sy = HEADER_H + 25;
    int cw = min(500, (int)(rc.right - SIDEBAR_W - 70));

    Txt(hdc, "Account Balance", sx, sy, hFontHeading, Primary);
    HPEN linePen = CreatePen(PS_SOLID, 2, Accent);
    HPEN old = (HPEN)SelectObject(hdc, linePen);
    MoveToEx(hdc, sx, sy + 28, NULL);
    LineTo(hdc, sx + 180, sy + 28);
    SelectObject(hdc, old);
    DeleteObject(linePen);

    DrawCard(hdc, sx, sy + 45, cw, 220);
    if (g.currentAccIdx >= 0 && g.currentAccIdx < (int)g.accounts.size()) {
        auto& a = g.accounts[g.currentAccIdx];
        Txt(hdc, "Account Holder:", sx + 25, sy + 70, hFontSmall, TextLight);
        Txt(hdc, a.name.c_str(), sx + 160, sy + 68, hFontBold, Text);

        Txt(hdc, "Account Type:", sx + 25, sy + 105, hFontSmall, TextLight);
        Txt(hdc, a.accountType.c_str(), sx + 160, sy + 103, hFontNormal, Text);

        Txt(hdc, "Account Status:", sx + 25, sy + 140, hFontSmall, TextLight);
        COLORREF stClr = (a.status=="active") ? Success : Error;
        Txt(hdc, a.status.c_str(), sx + 160, sy + 138, hFontBold, stClr);

        Txt(hdc, "Available Balance:", sx + 25, sy + 185, hFontBold, Text);
        stringstream ss;
        ss << "Rs. " << fixed << setprecision(2) << a.balance;
        Txt(hdc, ss.str().c_str(), sx + 180, sy + 180, hFontTitle, Success);
    }
}

static void PaintATMWithdraw(HDC hdc, RECT rc) {
    int sx = SIDEBAR_W + 30;
    int sy = HEADER_H + 25;
    int cw = min(520, (int)(rc.right - SIDEBAR_W - 60));

    Txt(hdc, "Withdraw Cash", sx, sy, hFontHeading, Primary);
    Txt(hdc, "Enter the amount you wish to withdraw", sx, sy + 28, hFontSmall, TextLight);
    HPEN linePen = CreatePen(PS_SOLID, 2, Accent);
    HPEN old = (HPEN)SelectObject(hdc, linePen);
    MoveToEx(hdc, sx, sy + 46, NULL);
    LineTo(hdc, sx + 175, sy + 46);
    SelectObject(hdc, old);
    DeleteObject(linePen);

    int cardY = sy + 58;
    DrawCard(hdc, sx, cardY, cw, 145);
    Txt(hdc, "Amount (Rs.):", sx + 25, cardY + 28, hFontBold, Text);

    if (g.currentAccIdx >= 0 && g.currentAccIdx < (int)g.accounts.size()) {
        stringstream ss;
        ss << "Daily Limit: Rs. " << fixed << setprecision(0) << DAILY_WITHDRAWAL_LIMIT
           << "   |   Withdrawn Today: Rs. " << fixed << setprecision(0) << g.accounts[g.currentAccIdx].dailyWithdrawn;
        Txt(hdc, ss.str().c_str(), sx + 25, cardY + 155, hFontSmall, TextLight);
    }
}

static void PaintATMDeposit(HDC hdc, RECT rc) {
    int sx = SIDEBAR_W + 30;
    int sy = HEADER_H + 25;
    int cw = min(520, (int)(rc.right - SIDEBAR_W - 60));

    Txt(hdc, "Deposit Cash", sx, sy, hFontHeading, Primary);
    Txt(hdc, "Add funds to your account", sx, sy + 28, hFontSmall, TextLight);
    HPEN linePen = CreatePen(PS_SOLID, 2, Accent);
    HPEN old = (HPEN)SelectObject(hdc, linePen);
    MoveToEx(hdc, sx, sy + 46, NULL);
    LineTo(hdc, sx + 145, sy + 46);
    SelectObject(hdc, old);
    DeleteObject(linePen);

    int cardY = sy + 58;
    DrawCard(hdc, sx, cardY, cw, 145);
    Txt(hdc, "Amount (Rs.):", sx + 25, cardY + 28, hFontBold, Text);
    Txt(hdc, "Enter the amount in Pakistani Rupees", sx + 25, cardY + 155, hFontSmall, TextLight);
}

static void PaintATMTransfer(HDC hdc, RECT rc) {
    int sx = SIDEBAR_W + 30;
    int sy = HEADER_H + 25;
    int cw = min(520, (int)(rc.right - SIDEBAR_W - 60));

    Txt(hdc, "Transfer Funds", sx, sy, hFontHeading, Primary);
    Txt(hdc, "Send money to another account", sx, sy + 28, hFontSmall, TextLight);
    HPEN linePen = CreatePen(PS_SOLID, 2, Accent);
    HPEN old = (HPEN)SelectObject(hdc, linePen);
    MoveToEx(hdc, sx, sy + 46, NULL);
    LineTo(hdc, sx + 180, sy + 46);
    SelectObject(hdc, old);
    DeleteObject(linePen);

    int cardY = sy + 58;
    DrawCard(hdc, sx, cardY, cw, 195);
    Txt(hdc, "Target Account #:", sx + 25, cardY + 28, hFontBold, Text);
    Txt(hdc, "Amount (Rs.):",     sx + 25, cardY + 78, hFontBold, Text);
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
    int sy = HEADER_H + 25;
    int cw = min(520, (int)(rc.right - SIDEBAR_W - 60));

    Txt(hdc, "Change PIN", sx, sy, hFontHeading, Primary);
    Txt(hdc, "Update your 4-digit security PIN", sx, sy + 28, hFontSmall, TextLight);
    HPEN linePen = CreatePen(PS_SOLID, 2, Accent);
    HPEN old = (HPEN)SelectObject(hdc, linePen);
    MoveToEx(hdc, sx, sy + 46, NULL);
    LineTo(hdc, sx + 130, sy + 46);
    SelectObject(hdc, old);
    DeleteObject(linePen);

    int cardY = sy + 58;
    DrawCard(hdc, sx, cardY, cw, 245);
    Txt(hdc, "Current PIN:",     sx + 25, cardY + 28, hFontBold, Text);
    Txt(hdc, "New PIN:",         sx + 25, cardY + 78, hFontBold, Text);
    Txt(hdc, "Confirm New PIN:", sx + 25, cardY + 128, hFontBold, Text);
    Txt(hdc, "All PINs must be exactly 4 digits", sx + 25, cardY + 255, hFontSmall, TextLight);
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

static void PaintAdminActionScreen(HDC hdc, RECT rc,
    const char* title, const char* subtitle, const char* note, COLORREF noteClr) {
    int sx = SIDEBAR_W + 30;
    int sy = HEADER_H + 25;
    int cw = min(520, (int)(rc.right - SIDEBAR_W - 60));

    Txt(hdc, title, sx, sy, hFontHeading, Primary);
    Txt(hdc, subtitle, sx, sy + 28, hFontSmall, TextLight);
    HPEN linePen = CreatePen(PS_SOLID, 2, Accent);
    HPEN old = (HPEN)SelectObject(hdc, linePen);
    MoveToEx(hdc, sx, sy + 46, NULL);
    LineTo(hdc, sx + (int)(strlen(title) * 10.5f), sy + 46);
    SelectObject(hdc, old);
    DeleteObject(linePen);

    int cardY = sy + 58;
    DrawCard(hdc, sx, cardY, cw, 145);

    Txt(hdc, "Account Number:", sx + 25, cardY + 28, hFontBold, Text);

    // Warning/info note below card
    HBRUSH noteBg = CreateSolidBrush(RGB(254,252,232));
    HPEN notePen = CreatePen(PS_SOLID, 1, RGB(253,230,138));
    RoundRect2(hdc, sx, cardY + 160, cw, 44, 8, noteBg, notePen);
    DeleteObject(noteBg);
    DeleteObject(notePen);
    Txt(hdc, note, sx + 15, cardY + 172, hFontSmall, noteClr);
}

static void PaintAdminFreeze(HDC hdc, RECT rc) {
    PaintAdminActionScreen(hdc, rc,
        "Freeze Account",
        "Disable all ATM transactions for an account",
        "Warning: Freezing blocks all deposits, withdrawals and transfers.",
        RGB(180, 83, 9));
}

static void PaintAdminUnfreeze(HDC hdc, RECT rc) {
    PaintAdminActionScreen(hdc, rc,
        "Unfreeze Account",
        "Restore access to a previously frozen account",
        "This will reactivate the account and allow all ATM transactions.",
        RGB(5, 122, 85));
}

static void PaintAdminUnlock(HDC hdc, RECT rc) {
    PaintAdminActionScreen(hdc, rc,
        "Unlock Account",
        "Reset PIN attempt counter and restore locked accounts",
        "This resets failed PIN attempts to 0, allowing the customer to log in again.",
        RGB(30, 64, 175));
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
    int sy = HEADER_H + 25;
    int cw = rc.right - SIDEBAR_W - 60;
    int ch = rc.bottom - HEADER_H - STATUS_H - 50;

    Txt(hdc, "Search Transactions", sx, sy, hFontHeading, Primary);
    Txt(hdc, "Filter the transaction log by account, type, date or amount", sx, sy + 28, hFontSmall, TextLight);
    HPEN linePen = CreatePen(PS_SOLID, 2, Accent);
    HPEN old = (HPEN)SelectObject(hdc, linePen);
    MoveToEx(hdc, sx, sy + 46, NULL);
    LineTo(hdc, sx + 240, sy + 46);
    SelectObject(hdc, old);
    DeleteObject(linePen);

    // Filter card
    DrawCard(hdc, sx, sy + 58, cw, 160);
    int fY = sy + 58;
    int col1X = sx + 20, col2X = sx + cw/2 + 10;
    int col1V = sx + 130, col2V = sx + cw/2 + 120;
    Txt(hdc, "Account #:",  col1X, fY + 25, hFontBold, TextLight);
    Txt(hdc, "Type:",       col2X, fY + 25, hFontBold, TextLight);
    Txt(hdc, "From Date:",  col1X, fY + 75, hFontBold, TextLight);
    Txt(hdc, "To Date:",    col2X, fY + 75, hFontBold, TextLight);
    Txt(hdc, "Min Amount:", col1X, fY + 125, hFontBold, TextLight);
    Txt(hdc, "Max Amount:", col2X, fY + 125, hFontBold, TextLight);

    // Results card
    int resY = sy + 235;
    DrawCard(hdc, sx, resY, cw, ch - resY + HEADER_H + 15);
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
    int sx = SIDEBAR_W + 30;
    int sy = HEADER_H + 25;
    int cw = rc.right - SIDEBAR_W - 60;
    int ch = rc.bottom - HEADER_H - STATUS_H - 50;

    Txt(hdc, "Loan Management", sx, sy, hFontHeading, Primary);
    Txt(hdc, "Approve new loans and review existing loan records", sx, sy + 28, hFontSmall, TextLight);
    HPEN linePen = CreatePen(PS_SOLID, 2, Accent);
    HPEN old = (HPEN)SelectObject(hdc, linePen);
    MoveToEx(hdc, sx, sy + 46, NULL);
    LineTo(hdc, sx + 200, sy + 46);
    SelectObject(hdc, old);
    DeleteObject(linePen);

    // Approve loan form card
    DrawCard(hdc, sx, sy + 58, cw, 175);
    int fY = sy + 58;
    Txt(hdc, "Approve New Loan", sx + 20, fY + 15, hFontBold, Text);
    Txt(hdc, "Account #:",    sx + 20, fY + 52, hFontSmall, TextLight);
    Txt(hdc, "Loan Amount:",  sx + 20, fY + 102, hFontSmall, TextLight);
    Txt(hdc, "Term (months):",sx + 20, fY + 152, hFontSmall, TextLight);

    // Results card
    int resY = sy + 250;
    int cardH2 = ch - resY + HEADER_H + 15;
    DrawCard(hdc, sx, resY, cw, max(cardH2, 80));


    // Loans table inside results card
    int tblHdrY = resY + 5;
    RECT tblHdr = {sx + 10, tblHdrY, sx + cw - 10, tblHdrY + 28};
    HBRUSH tblHdrBr = CreateSolidBrush(RGB(240,243,248));
    FillRect(hdc, &tblHdr, tblHdrBr);
    DeleteObject(tblHdrBr);

    int colX[] = {sx+15, sx+80, sx+200, sx+330, sx+455, sx+560};
    const char* colH[] = {"ID", "Account", "Amount", "Monthly", "Status", "Paid"};
    for (int i = 0; i < 6; i++)
        Txt(hdc, colH[i], colX[i], tblHdrY + 8, hFontBold, TextLight);

    int rowY = tblHdrY + 35;
    for (size_t i = 0; i < g.loans.size(); i++) {
        if (rowY > resY + ch - resY + HEADER_H + 10) break;
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
    int cx = rc.right / 2;
    int cy = HEADER_H + (rc.bottom - HEADER_H - STATUS_H) / 2;
    int cw = 440, ch = 260;
    int cardX = cx - cw/2, cardY = cy - ch/2;

    DrawCard(hdc, cardX, cardY, cw, ch);
    TxtCenter(hdc, "OTP Verification", cardX, cardY + 20, cw, 30, hFontHeading, Primary);
    TxtCenter(hdc, "A one-time code has been generated for your transfer", cardX, cardY + 54, cw, 22, hFontSmall, TextLight);

    HPEN linePen = CreatePen(PS_SOLID, 1, CardBorder);
    HPEN old = (HPEN)SelectObject(hdc, linePen);
    MoveToEx(hdc, cardX + 30, cardY + 80, NULL);
    LineTo(hdc, cardX + cw - 30, cardY + 80);
    SelectObject(hdc, old);
    DeleteObject(linePen);

    // Show OTP prominently
    TxtCenter(hdc, g.otpCode.c_str(), cardX, cardY + 90, cw, 50, hFontTitle, Accent);
    TxtCenter(hdc, "Your OTP code (for testing)", cardX, cardY + 142, cw, 18, hFontSmall, TextLight);

    Txt(hdc, "Enter OTP:", cardX + 30, cardY + 175, hFontBold, Text);
}

// ============================
// MAIN PAINT DISPATCHER
// ============================
static void PaintScreen(HDC hdc, RECT rc) {
    FillRect(hdc, &rc, hBrushBg);
    DrawHeader(hdc, rc, g);

    if (g.screen != SCR_LOGIN && g.screen != SCR_ADMIN_LOGIN && g.screen != SCR_ATM_LOGIN) {
        DrawSidebar(hdc, rc, g);
    }

    switch (g.screen) {
        case SCR_LOGIN: PaintLoginScreen(hdc, rc); break;
        case SCR_ADMIN_LOGIN: PaintAdminLoginScreen(hdc, rc); break;
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
        case SCR_ATM_LOGIN: PaintATMLoginScreen(hdc, rc); break;
        case SCR_ATM_MENU: PaintATMMenu(hdc, rc); break;
        case SCR_ATM_BALANCE: PaintATMBalance(hdc, rc); break;
        case SCR_ATM_DEPOSIT: PaintATMDeposit(hdc, rc); break;
        case SCR_ATM_WITHDRAW: PaintATMWithdraw(hdc, rc); break;
        case SCR_ATM_TRANSFER: PaintATMTransfer(hdc, rc); break;
        case SCR_ATM_MINISTATE: PaintATMMiniState(hdc, rc); break;
        case SCR_ATM_CHANGEPIN: PaintATMChangePIN(hdc, rc); break;
        case SCR_ATM_INFO: PaintATMInfo(hdc, rc); break;
        case SCR_OTP: PaintOTPScreen(hdc, rc); break;
    }

    DrawStatus(hdc, rc, g);
}

// ============================
// SCREEN TRANSITIONS + CONTROLS
// ============================

static void GoToScreen(Screen scr);

static void CreateScreenControls() {
    ClearControls();
    RECT rc;
    GetClientRect(gHwnd, &rc);
    int sx = SIDEBAR_W + 35;
    int sy = HEADER_H + 25;
    int edH = 34;

    // Header buttons (Logout / Backup)
    if (g.isAdmin && g.screen != SCR_ADMIN_LOGIN && g.screen != SCR_LOGIN) {
        MakeBtn(BTN_DO_BACKUP, "Backup Data", rc.right - 245, 14, 115, 36, Primary);
        MakeBtn(BTN_LOGOUT,    "Logout",      rc.right - 115, 14, 95,  36, Error);
    } else if (!g.isAdmin && g.currentAccIdx >= 0 && g.screen != SCR_ATM_LOGIN && g.screen != SCR_LOGIN) {
        MakeBtn(BTN_LOGOUT,    "Logout",      rc.right - 115, 14, 95,  36, Error);
    }

    switch (g.screen) {
    case SCR_LOGIN: {
        int cx = rc.right / 2;
        int cy = HEADER_H + (rc.bottom - HEADER_H - STATUS_H) / 2;
        MakeBtn(BTN_DO_ADMIN_LOGIN, "Bank Administrator", cx - 170, cy + 10, 340, 46, Primary);
        MakeBtn(BTN_DO_LOGIN,       "ATM Customer",       cx - 170, cy + 68, 340, 46, Success);
        break;
    }
    case SCR_ATM_LOGIN: {
        int cx = rc.right / 2;
        int cy = HEADER_H + (rc.bottom - HEADER_H - STATUS_H) / 2;
        MakeLabel("Account Number:", cx - 170, cy - 25, 140, 24, 1);
        MakeEdit(EDT_ACCNO,           cx - 20,  cy - 30, 190, 34);
        MakeLabel("PIN:",            cx - 170, cy + 25, 140, 24, 1);
        MakeEdit(EDT_PIN,             cx - 20,  cy + 20, 190, 34, true);
        MakeBtn(BTN_DO_LOGIN,        "Login",  cx - 170, cy + 80, 160, 42, Success);
        MakeBtn(BTN_BACK,            "Back",   cx + 10,  cy + 80, 160, 42, RGB(100, 116, 139));
        break;
    }
    case SCR_ADMIN_LOGIN: {
        int cx = rc.right / 2;
        int cy = HEADER_H + (rc.bottom - HEADER_H - STATUS_H) / 2;
        MakeLabel("Password:", cx - 170, cy - 10, 130, 24, 1);
        MakeEdit(EDT_PIN,      cx - 30,  cy - 16, 200, 34, true);
        MakeBtn(BTN_DO_ADMIN_LOGIN, "Login", cx - 170, cy + 60, 160, 42, Primary);
        MakeBtn(BTN_BACK,           "Back",  cx + 10,  cy + 60, 160, 42, RGB(100, 116, 139));
        break;
    }
    case SCR_ADMIN_DASH: break;
    case SCR_ADMIN_CREATE: {
        int cw = min(560, (int)(rc.right - SIDEBAR_W - 60));
        int cardY = sy + 58;
        int lblX = sx + 30;
        int edtX = sx + 200;
        int edtW = cw - 230;
        int stepY = 52;

        MakeLabel("Full Name:",     lblX, cardY + 24,           160, 24, 1);
        MakeEdit(EDT_NAME,           edtX, cardY + 20,           edtW, edH);
        MakeLabel("CNIC:",           lblX, cardY + 24 + stepY,   160, 24, 1);
        MakeEdit(EDT_CNIC,           edtX, cardY + 20 + stepY,   edtW, edH);
        MakeLabel("Account Type:",   lblX, cardY + 24 + stepY*2, 160, 24, 1);
        MakeEdit(EDT_TYPE,           edtX, cardY + 20 + stepY*2, edtW, edH);
        MakeLabel("Initial Balance:",lblX, cardY + 24 + stepY*3, 160, 24, 1);
        MakeEdit(EDT_BALANCE,        edtX, cardY + 20 + stepY*3, edtW, edH);
        MakeLabel("PIN (4 digits):", lblX, cardY + 24 + stepY*4, 160, 24, 1);
        MakeEdit(EDT_PIN,            edtX, cardY + 20 + stepY*4, edtW, edH, true);
        MakeLabel("Confirm PIN:",    lblX, cardY + 24 + stepY*5, 160, 24, 1);
        MakeEdit(EDT_CONFIRMPIN,     edtX, cardY + 20 + stepY*5, edtW, edH, true);

        MakeBtn(BTN_DO_CREATE,    "Create Account", edtX,       cardY + 24 + stepY*6, 160, 42, Success);
        MakeBtn(BTN_CLEAR_CREATE, "Clear",          edtX + 175, cardY + 24 + stepY*6, 110, 42, RGB(100,116,139));
        break;
    }
    case SCR_ADMIN_VIEW: break;
    case SCR_ADMIN_SEARCH: {
        int cardY = sy + 58;
        MakeLabel("Account #:", sx + 25, cardY + 24, 120, 24, 1);
        MakeEdit(EDT_SEARCH, sx + 150, cardY + 20, 250, 36);
        MakeBtn(BTN_DO_SEARCH, "Search", sx + 415, cardY + 19, 115, 38, Primary);
        break;
    }
    case SCR_ADMIN_FREEZE: {
        int cardY = sy + 58;
        MakeEdit(EDT_SEARCH, sx + 185, cardY + 22, 220, 36);
        MakeBtn(BTN_DO_FREEZE, "Freeze Account", sx + 185, cardY + 75, 150, 40, Error);
        break;
    }
    case SCR_ADMIN_UNFREEZE: {
        int cardY = sy + 58;
        MakeEdit(EDT_SEARCH, sx + 185, cardY + 22, 220, 36);
        MakeBtn(BTN_DO_UNFREEZE, "Unfreeze", sx + 185, cardY + 75, 150, 40, Success);
        break;
    }
    case SCR_ADMIN_UNLOCK: {
        int cardY = sy + 58;
        MakeEdit(EDT_SEARCH, sx + 185, cardY + 22, 220, 36);
        MakeBtn(BTN_DO_UNLOCK, "Unlock Account", sx + 185, cardY + 75, 150, 40, Warning);
        break;
    }
    case SCR_ADMIN_TXNS: break;
    case SCR_ADMIN_SEARCH_TXN: {
        int cw = rc.right - SIDEBAR_W - 60;
        int fY = sy + 58;
        int col1X = sx + 20, col2X = sx + cw/2 + 10;
        int col1V = sx + 130, col2V = sx + cw/2 + 120;
        MakeEdit(EDT_SEARCH_TXN_ACC,  col1V, fY + 18, 140, 32);
        MakeEdit(EDT_SEARCH_TXN_TYPE, col2V, fY + 18, 140, 32);
        MakeEdit(EDT_SEARCH_TXN_FROM, col1V, fY + 68, 140, 32);
        MakeEdit(EDT_SEARCH_TXN_TO,   col2V, fY + 68, 140, 32);
        MakeEdit(EDT_SEARCH_TXN_MIN,  col1V, fY + 118, 100, 32);
        MakeEdit(EDT_SEARCH_TXN_MAX,  col2V, fY + 118, 100, 32);
        MakeBtn(BTN_DO_SEARCH_TXN, "Search", sx + cw - 145, fY + 115, 125, 36, Primary);
        break;
    }
    case SCR_ADMIN_AUDIT: break;
    case SCR_ADMIN_LOANS: {
        int fY = sy + 58;
        MakeEdit(EDT_LOAN_ACC,  sx + 160, fY + 44,  210, 32);
        MakeEdit(EDT_LOAN_AMT,  sx + 160, fY + 94,  210, 32);
        MakeEdit(EDT_LOAN_TERM, sx + 160, fY + 144, 110, 32);
        MakeBtn(BTN_DO_LOAN, "Approve Loan", sx + 290, fY + 140, 150, 38, Warning);
        break;
    }
    case SCR_ADMIN_CASH: break;
    case SCR_ADMIN_DAILY: break;
    case SCR_ATM_MENU: break;
    case SCR_ATM_BALANCE: break;
    case SCR_ATM_DEPOSIT: {
        int cardY = sy + 58;
        MakeEdit(EDT_AMT,      sx + 175, cardY + 22, 220, 36);
        MakeBtn(BTN_DO_DEPOSIT, "Deposit Cash", sx + 175, cardY + 75, 135, 40, Success);
        MakeBtn(BTN_BACK,       "Back",         sx + 320, cardY + 75, 90, 40, RGB(100,116,139));
        break;
    }
    case SCR_ATM_WITHDRAW: {
        int cardY = sy + 58;
        MakeEdit(EDT_AMT,      sx + 175, cardY + 22, 220, 36);
        MakeBtn(BTN_DO_WITHDRAW, "Withdraw Cash", sx + 175, cardY + 75, 140, 40, Error);
        MakeBtn(BTN_BACK,        "Back",          sx + 325, cardY + 75, 90, 40, RGB(100,116,139));
        break;
    }
    case SCR_ATM_TRANSFER: {
        int cardY = sy + 58;
        MakeEdit(EDT_TRANSFER_TARGET, sx + 185, cardY + 22, 220, 36);
        MakeEdit(EDT_TRANSFER_AMT,    sx + 185, cardY + 72, 220, 36);
        MakeBtn(BTN_DO_TRANSFER, "Transfer Funds", sx + 185, cardY + 126, 140, 40, Secondary);
        MakeBtn(BTN_BACK,        "Back",           sx + 335, cardY + 126, 90, 40, RGB(100,116,139));
        break;
    }
    case SCR_ATM_MINISTATE: break;
    case SCR_ATM_CHANGEPIN: {
        int cardY = sy + 58;
        MakeEdit(EDT_PIN,        sx + 185, cardY + 22,  210, 36, true);
        MakeEdit(EDT_NEWPIN,     sx + 185, cardY + 72,  210, 36, true);
        MakeEdit(EDT_CONFIRMPIN, sx + 185, cardY + 122, 210, 36, true);
        MakeBtn(BTN_DO_CHANGEPIN, "Change PIN", sx + 185, cardY + 176, 135, 40, Warning);
        MakeBtn(BTN_BACK,         "Back",       sx + 330, cardY + 176, 90, 40, RGB(100,116,139));
        break;
    }
    case SCR_ATM_INFO: break;
    case SCR_OTP: {
        int cx = rc.right / 2;
        int cy = HEADER_H + (rc.bottom - HEADER_H - STATUS_H) / 2;
        int cw2 = 440, ch2 = 270;
        int cardX = cx - cw2/2, cardY = cy - ch2/2;
        MakeEdit(EDT_OTP,       cardX + 140, cardY + 164, 250, 36);
        MakeBtn(BTN_DO_OTP, "Verify OTP", cardX + 140, cardY + 212, 130, 40, Primary);
        MakeBtn(BTN_BACK,   "Cancel",     cardX + 280, cardY + 212, 110, 40, RGB(100, 116, 139));
        break;
    }
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
        if (g.screen == SCR_LOGIN || g.screen == SCR_ATM_LOGIN) {
            GoToScreen(SCR_ADMIN_LOGIN);
        } else {
            string pw = GetEditText(EDT_PIN);
            if (pw == ADMIN_PASSWORD) {
                g.isAdmin = true;
                g.currentAccIdx = -1;
                loadAccounts(g.accounts);
                loadTransactions(g.transactions);
                loadLoans(g.loans);
                loadCashInventory(g.inventory);
                logAudit("Admin Login", "Administrator logged in");
                SpeakText("Welcome to the admin panel.");
                GoToScreen(SCR_ADMIN_DASH);
            } else {
                SetStatus("Wrong admin password", 2);
                SpeakText("Wrong password. Please try again.");
            }
        }
        break;

    case BTN_DO_LOGIN: {
        if (g.screen == SCR_LOGIN) {
            GoToScreen(SCR_ATM_LOGIN);
            break;
        }
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
        SpeakText("Login successful. Welcome " + g.accounts[idx].name + ".");
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

        logAudit("Account Created", "Account " + to_string(acc.accountNo) + " created for " + name);
        stringstream ss;
        ss << "Account #" << acc.accountNo << " created successfully!";
        SetStatus(ss.str(), 1);
        SpeakText("Account number " + to_string(acc.accountNo) + " created successfully for " + name + ".");
        ClearControls();
        break;
    }

    case BTN_DO_SEARCH: {
        string accStr = GetEditText(EDT_SEARCH);
        if (accStr.empty()) { SetStatus("Enter account number", 2); break; }
        int accNo = atoi(accStr.c_str());
        loadAccounts(g.accounts);
        int idx = findAccountIndex(accNo, g.accounts);
        if (idx < 0) { 
            g.lastSearchAccNo = -1;
            SetStatus("Account not found", 2); 
            InvalidateRect(gHwnd, NULL, TRUE);
            break; 
        }
        g.lastSearchAccNo = accNo;
        g.currentAccIdx = idx;
        stringstream ss;
        ss << "Found: " << g.accounts[idx].name << " | Rs. " << fixed << setprecision(2) << g.accounts[idx].balance;
        SetStatus(ss.str(), 1);
        InvalidateRect(gHwnd, NULL, TRUE);
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
        SpeakText("Account " + to_string(accNo) + " has been frozen.");
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
        SpeakText("Account " + to_string(accNo) + " has been unfrozen.");
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
        SpeakText("Account " + to_string(accNo) + " has been unlocked.");
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
        generateReceipt(txn, g.accounts[idx]);

        logAudit("Deposit", "Account " + to_string(accNo) + " deposited Rs. " + to_string(amt));
        stringstream ss;
        ss << "Deposited Rs. " << fixed << setprecision(2) << amt << " successfully";
        SetStatus(ss.str(), 1);
        SpeakText("Deposit successful. " + to_string((int)amt) + " rupees deposited.");
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
        generateReceipt(txn, acc);

        logAudit("Withdrawal", "Account " + to_string(acc.accountNo) + " withdrew Rs. " + to_string(amt));
        stringstream ss;
        ss << "Withdrawn Rs. " << fixed << setprecision(2) << amt << " successfully";
        SetStatus(ss.str(), 1);
        SpeakText("Please collect your cash. Withdrawal successful.");
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
            g.pendingTransferTarget = targetAcc;
            g.pendingTransferAmount = amt;
            GoToScreen(SCR_OTP);
            return;
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
        generateReceipt(txn, g.accounts[senderIdx]);

        logAudit("Transfer", "Account " + to_string(g.accounts[senderIdx].accountNo) +
                 " transferred Rs. " + to_string(amt) + " to " + to_string(targetAcc));
        stringstream ss;
        ss << "Transferred Rs. " << fixed << setprecision(2) << amt << " successfully";
        SetStatus(ss.str(), 1);
        SpeakText("Transfer successful.");
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
        SpeakText("PIN changed successfully.");
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

        g.accounts[idx].balance += amt;
        saveAccounts(g.accounts);

        logAudit("Loan Approved", "Loan " + to_string(loan.loanId) + " of Rs. " + to_string(amt) + " for account " + to_string(accNo));
        stringstream ss;
        ss << "Loan #" << loan.loanId << " approved! Rs. " << fixed << setprecision(0) << amt;
        SetStatus(ss.str(), 1);
        SpeakText("Loan approved. " + to_string((int)amt) + " rupees credited to account.");
        break;
    }

    case BTN_BACK:
        GoToScreen(SCR_LOGIN);
        break;

    case BTN_DO_BACKUP: {
        if (backupData()) {
            logAudit("Backup", "System data backup created");
            SetStatus("Backup created successfully", 1);
        } else {
            SetStatus("Backup failed", 2);
        }
        break;
    }

    case BTN_DO_OTP: {
        string otpInput = GetEditText(EDT_OTP);
        if (otpInput.empty()) {
            SetStatus("Enter OTP code", 2);
            break;
        }
        if (otpInput != g.otpCode) {
            SetStatus("Wrong OTP code", 2);
            break;
        }
        int senderIdx = g.currentAccIdx;
        int targetAcc = g.pendingTransferTarget;
        double amt = g.pendingTransferAmount;
        int receiverIdx = findAccountIndex(targetAcc, g.accounts);
        if (receiverIdx < 0 || senderIdx < 0) {
            SetStatus("Transfer invalid, please try again", 2);
            GoToScreen(SCR_ATM_MENU);
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
        generateReceipt(txn, g.accounts[senderIdx]);

        logAudit("Transfer (OTP)", "Account " + to_string(g.accounts[senderIdx].accountNo) +
                 " transferred Rs. " + to_string(amt) + " to " + to_string(targetAcc));
        stringstream ss;
        ss << "Transferred Rs. " << fixed << setprecision(2) << amt << " successfully";
        SetStatus(ss.str(), 1);
        GoToScreen(SCR_ATM_MENU);
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
    string guide = ScreenGuide(scr);
    if (!guide.empty()) SpeakText(guide);
}

// ============================
// Owner-drawn button painting
// ============================
static void DrawBtnRaw(DRAWITEMSTRUCT* di, int customColor) {
    HDC hdc = di->hDC;
    RECT rc = di->rcItem;
    bool pressed = di->itemState & ODS_SELECTED;
    bool focused = di->itemState & ODS_FOCUS;
    bool hovered = (GetProp(di->hwndItem, "HOVER") != NULL);

    COLORREF bgClr = (customColor != 0) ? (COLORREF)customColor : Secondary;
    if (pressed) {
        int r = max(0, GetRValue(bgClr) - 35);
        int g2 = max(0, GetGValue(bgClr) - 35);
        int b = max(0, GetBValue(bgClr) - 35);
        bgClr = RGB(r, g2, b);
    } else if (hovered) {
        int r = min(255, GetRValue(bgClr) + 25);
        int g2 = min(255, GetGValue(bgClr) + 25);
        int b = min(255, GetBValue(bgClr) + 25);
        bgClr = RGB(r, g2, b);
    }

    HBRUSH br = CreateSolidBrush(bgClr);
    HPEN pen = CreatePen(PS_SOLID, 1, hovered ? RGB(255,255,255) : bgClr);
    HPEN oldPen = (HPEN)SelectObject(hdc, pen);
    HBRUSH oldBr = (HBRUSH)SelectObject(hdc, br);
    RoundRect(hdc, rc.left, rc.top, rc.right, rc.bottom, 20, 20);
    SelectObject(hdc, oldPen);
    SelectObject(hdc, oldBr);
    DeleteObject(br);
    DeleteObject(pen);

    char txt[256] = {0};
    GetWindowText(di->hwndItem, txt, 255);

    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, RGB(255,255,255));
    HFONT oldFont = (HFONT)SelectObject(hdc, hFontBold);
    RECT txtRc = rc;
    if (pressed) {
        txtRc.top += 1;
        txtRc.left += 1;
    }
    DrawText(hdc, txt, -1, &txtRc, DT_CENTER|DT_VCENTER|DT_SINGLELINE);
    SelectObject(hdc, oldFont);

    if (focused) {
        HPEN focusPen = CreatePen(PS_DOT, 1, RGB(255,255,255));
        HPEN oldFP = (HPEN)SelectObject(hdc, focusPen);
        HBRUSH oldFB = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));
        RoundRect(hdc, rc.left + 2, rc.top + 2, rc.right - 2, rc.bottom - 2, 16, 16);
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
            int hit = SidebarHitTest(lp, g);
            if (hit != gHoverSidebarIdx) {
                gHoverSidebarIdx = hit;
                InvalidateRect(hwnd, NULL, FALSE);
            }
            SetCursor(LoadCursor(NULL, IDC_HAND));
        } else {
            if (gHoverSidebarIdx != -1) {
                gHoverSidebarIdx = -1;
                InvalidateRect(hwnd, NULL, FALSE);
            }
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

    case WM_SIZE: {
        if (wp != SIZE_MINIMIZED) {
            CreateScreenControls();
            InvalidateRect(hwnd, NULL, TRUE);
        }
        return 0;
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
        static HBRUSH hEditBr = CreateSolidBrush(RGB(248,250,252));
        return (LRESULT)hEditBr;
    }

    case WM_CTLCOLORSTATIC: {
        HDC hdc2 = (HDC)wp;
        SetTextColor(hdc2, RGB(40,40,50));
        SetBkMode(hdc2, TRANSPARENT);
        return (LRESULT)GetStockObject(HOLLOW_BRUSH);
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
    loadCashInventory(g.inventory);
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
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, WIN_W, WIN_H,
        NULL, NULL, hInst, NULL);

    if (!hwnd) return 1;

    ShowWindow(hwnd, SW_SHOWMAXIMIZED);
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
