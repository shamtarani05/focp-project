#include "gui.h"
#include "theme.h"
#include "banking.h"
#include "core/fileio.h"
#include "core/validation.h"
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cmath>

// Check if SAPI is available (MSVC with Windows SDK)
#if defined(_MSC_VER) || defined(HAS_SAPI)
#define USE_SAPI 1
#include <initguid.h>
#include <sapi.h>
#include <ole2.h>
#else
#define USE_SAPI 0
#endif


using namespace Theme;

const string ADMIN_PASSWORD = "SAA@Bank#2026";

// ============================
// VOICE ASSISTANT (Native Windows SAPI)
// ============================
#if USE_SAPI
static ISpVoice* g_pVoice = NULL;
static bool g_voiceInit = false;

static void InitVoice() {
    if (!g_voiceInit) {
        CoInitialize(NULL);
        HRESULT hr = CoCreateInstance(CLSID_SpVoice, NULL, CLSCTX_ALL, IID_ISpVoice, (void**)&g_pVoice);
        if (FAILED(hr)) g_pVoice = NULL;
        g_voiceInit = true;
    }
}

static void StopCurrentSpeech() {
    InitVoice();
    if (g_pVoice) {
        // SPF_PURGEBEFORESPEAK immediately stops/purges any ongoing or queued speech
        g_pVoice->Speak(NULL, SPF_PURGEBEFORESPEAK, NULL);
    }
}

static void SpeakText(const string& text) {
    if (text.empty()) return;
    InitVoice();
    if (g_pVoice) {
        int wlen = MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, NULL, 0);
        if (wlen > 0) {
            wstring wtext(wlen, 0);
            MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, &wtext[0], wlen);
            // SPF_ASYNC | SPF_PURGEBEFORESPEAK instantly cancels old speech and speaks new text in 0ms!
            g_pVoice->Speak(wtext.c_str(), SPF_ASYNC | SPF_PURGEBEFORESPEAK, NULL);
        }
    }
}
#else
// Stub implementations when SAPI is not available (MinGW)
static void InitVoice() {}
static void StopCurrentSpeech() {}
static void SpeakText(const string& text) { (void)text; }
#endif

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
        case SCR_ATM_REPAY_LOAN: return "Loan repayment. Pay your monthly loan installment or pay off your loan balance.";
        case SCR_ATM_APPLY_LOAN: return "Apply for a loan. Request a new loan application for bank administrator review.";
        case SCR_OTP:       return "OTP verification. A one time password has been sent for large transfer verification. Please enter the OTP code.";
        case SCR_CONTACT_ADMIN: return "Contact Administrator. Fill in your details and reason to request account reactivation.";
        case SCR_ADMIN_REQUESTS: return "Reactivation Requests. Review and approve or reject account reactivation requests from customers.";
        case SCR_FORGOT_PIN: return "Forgot PIN. Enter your account number, full name, CNIC, and new PIN to reset your PIN.";
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
    EDT_REASON,
    EDT_REPAY_AMT,
    EDT_APPLY_LOAN_AMT,
    EDT_APPLY_LOAN_TERM,
};

enum BtnID {
    BTN_ADMIN_CREATE = BTN_BASE,
    BTN_ADMIN_VIEW, BTN_ADMIN_SEARCH, BTN_ADMIN_FREEZE,
    BTN_ADMIN_UNFREEZE, BTN_ADMIN_UNLOCK, BTN_ADMIN_TXNS,
    BTN_ADMIN_SEARCH_TXN, BTN_ADMIN_AUDIT, BTN_ADMIN_LOANS,
    BTN_ADMIN_CASH, BTN_ADMIN_DAILY,
    BTN_ATM_DEPOSIT, BTN_ATM_WITHDRAW, BTN_ATM_TRANSFER,
    BTN_ATM_BALANCE, BTN_ATM_MINISTATE, BTN_ATM_CHANGEPIN,
    BTN_ATM_INFO, BTN_ATM_PAY_LOAN, BTN_ATM_APPLY_LOAN, BTN_BACK,
    BTN_DO_CREATE, BTN_DO_LOGIN, BTN_DO_ADMIN_LOGIN,
    BTN_DO_SEARCH, BTN_DO_FREEZE, BTN_DO_UNFREEZE, BTN_DO_UNLOCK,
    BTN_DO_DEPOSIT, BTN_DO_WITHDRAW, BTN_DO_TRANSFER, BTN_DO_CHANGEPIN,
    BTN_DO_SEARCH_TXN, BTN_DO_LOAN, BTN_DO_OTP,
    BTN_DO_BACKUP,
    BTN_CLEAR_CREATE, BTN_CLEAR_SEARCH,
    BTN_DO_CONTACT_ADMIN,
    BTN_ADMIN_REQUESTS,
    BTN_CONTACT_ADMIN_SCREEN,
    BTN_FORGOT_PIN_SCREEN,
    BTN_DO_FORGOT_PIN,
    BTN_NOTIF_REQUESTS,
    BTN_DO_REPAY_INSTALLMENT,
    BTN_DO_REPAY_CUSTOM,
    BTN_DO_REPAY_FULL,
    BTN_DO_APPLY_LOAN,
    BTN_CLOSE_RECEIPT,
    BTN_VERIFY_ACCOUNT,
};

const int BTN_DYNAMIC_APPROVE_BASE = 20000;
const int BTN_DYNAMIC_REJECT_BASE  = 21000;
const int BTN_DYNAMIC_LOAN_APPROVE_BASE = 22000;
const int BTN_DYNAMIC_LOAN_REJECT_BASE  = 23000;

// --- GDI handles ---
static HFONT hFontTitle, hFontHeading, hFontNormal, hFontSmall, hFontBold, hFontMono;
static HBRUSH hBrushPrimary, hBrushCard, hBrushBg, hBrushSidebar, hBrushAccent;
static HPEN hPenCardBorder, hPenShadow;
static HBRUSH hBrushHover;
static bool gdiInit = false;

// --- Receipt overlay state ---
static bool gShowReceipt = false;
static Transaction gLastTxn;
static string gLastAccName;
static int gLastAccNo = 0;

// --- Transfer verification state ---
static bool gTransferVerified = false;
static int gVerifiedTargetAccNo = 0;
static string gVerifiedTargetName;

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
    {"Transactions",    SCR_ADMIN_TXNS,       true},
    {"Audit Log",       SCR_ADMIN_AUDIT,      true},
    {"Loans",           SCR_ADMIN_LOANS,      true},
    {"Cash Inventory",  SCR_ADMIN_CASH,       true},
    {"Daily Report",    SCR_ADMIN_DAILY,      true},
    {"Reactivation Req",SCR_ADMIN_REQUESTS,   true},
    {"",                SCR_LOGIN,            false},
    {"ATM Menu",        SCR_ATM_MENU,         false},
    {"ATM Balance",     SCR_ATM_BALANCE,      false},
    {"Deposit",         SCR_ATM_DEPOSIT,      false},
    {"Withdraw",        SCR_ATM_WITHDRAW,     false},
    {"Transfer",        SCR_ATM_TRANSFER,     false},
    {"Mini Statement",  SCR_ATM_MINISTATE,    false},
    {"Change PIN",      SCR_ATM_CHANGEPIN,    false},
    {"Pay Loan",        SCR_ATM_REPAY_LOAN,   false},
    {"Apply Loan",      SCR_ATM_APPLY_LOAN,   false},
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

    int startIdx = s.isAdmin ? 0 : 13;
    int count = s.isAdmin ? 12 : (NUM_SIDEBAR_ITEMS - 13);

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
    if (s.screen == SCR_LOGIN || s.screen == SCR_ADMIN_LOGIN || s.screen == SCR_ATM_LOGIN || s.screen == SCR_CONTACT_ADMIN || s.screen == SCR_FORGOT_PIN) {
        return -1;
    }
    int mx = LOWORD(lp);
    int my = HIWORD(lp);
    if (mx >= SIDEBAR_W || my <= HEADER_H) return -1;

    int yStart = HEADER_H + 42;
    int itemH = 38;

    int startIdx = s.isAdmin ? 0 : 13;
    int count = s.isAdmin ? 12 : (NUM_SIDEBAR_ITEMS - 13);

    for (int i = 0; i < count; i++) {
        int y = yStart + i * itemH;
        if (my >= y && my < y + itemH) {
            return startIdx + i;
        }
    }
    return -1;
}

static void DrawBellIcon(HDC hdc, int x, int y, COLORREF color) {
    HPEN pen = CreatePen(PS_SOLID, 2, color);
    HBRUSH brush = CreateSolidBrush(color);
    HPEN oldPen = (HPEN)SelectObject(hdc, pen);
    HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, brush);

    int cx = x + 10;
    int cy = y + 10;

    // Top handle loop
    HBRUSH nullBr = (HBRUSH)GetStockObject(NULL_BRUSH);
    SelectObject(hdc, nullBr);
    Ellipse(hdc, cx - 2, cy - 9, cx + 2, cy - 5);
    SelectObject(hdc, brush);

    // Main Bell body polygon
    POINT pts[6] = {
        {cx - 3, cy - 5},
        {cx + 3, cy - 5},
        {cx + 6, cy + 2},
        {cx + 8, cy + 5},
        {cx - 8, cy + 5},
        {cx - 6, cy + 2}
    };
    Polygon(hdc, pts, 6);

    // Bottom clapper ball
    Ellipse(hdc, cx - 2, cy + 5, cx + 2, cy + 9);

    SelectObject(hdc, oldPen);
    SelectObject(hdc, oldBrush);
    DeleteObject(pen);
    DeleteObject(brush);
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
    bool isPublicScreen = (s.screen == SCR_LOGIN || s.screen == SCR_ADMIN_LOGIN || s.screen == SCR_ATM_LOGIN || s.screen == SCR_CONTACT_ADMIN || s.screen == SCR_FORGOT_PIN);
    const char* modeLabel = (s.isAdmin && !isPublicScreen) ? "Admin Portal" :
                            (!s.isAdmin && s.currentAccIdx >= 0 && !isPublicScreen ? "ATM Session" : "");
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
static WNDPROC gOldEditProc = NULL;

static LRESULT CALLBACK EditSubclassProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    if (msg == WM_LBUTTONDOWN) {
        SetFocus(hwnd);
    }
    return CallWindowProc(gOldEditProc, hwnd, msg, wp, lp);
}

static LRESULT CALLBACK BtnSubclassProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_MOUSEMOVE: {
        if (!GetProp(hwnd, "HOVER")) {
            SetProp(hwnd, "HOVER", (HANDLE)1);
            TRACKMOUSEEVENT tme = { sizeof(tme), TME_LEAVE, hwnd, 0 };
            TrackMouseEvent(&tme);
            InvalidateRect(hwnd, NULL, FALSE);

            char btnText[128] = {0};
            GetWindowText(hwnd, btnText, 127);
            if (strlen(btnText) > 0) {
                SpeakText(btnText);
            }
        }
        break;
    }
    case WM_MOUSELEAVE: {
        RemoveProp(hwnd, "HOVER");
        InvalidateRect(hwnd, NULL, FALSE);
        StopCurrentSpeech();
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
        WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_OWNERDRAW,
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
    DWORD style = WS_CHILD|WS_VISIBLE|WS_TABSTOP|ES_AUTOHSCROLL;
    if (pwd) style |= ES_PASSWORD;
    HWND hw = CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", "",
        style, x, y, w, h, gHwnd, (HMENU)(LONG_PTR)id, NULL, NULL);
    SendMessage(hw, WM_SETFONT, (WPARAM)hFontNormal, TRUE);
    if (!gOldEditProc) {
        gOldEditProc = (WNDPROC)GetWindowLongPtr(hw, GWLP_WNDPROC);
    }
    SetWindowLongPtr(hw, GWLP_WNDPROC, (LONG_PTR)EditSubclassProc);
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

static void SetEditText(int id, const string& txt) {
    for (auto& e : gEdits) {
        if (e.id == id) {
            SetWindowText(e.hwnd, txt.c_str());
            return;
        }
    }
}

static void SetStatus(const string& msg, int type) {
    g.statusMsg = msg;
    g.statusType = type;
    if (gHwnd) InvalidateRect(gHwnd, NULL, FALSE);
}

// --- Receipt overlay control ---
static void ShowReceipt(const Transaction& txn, const Account& acc) {
    gLastTxn = txn;
    gLastAccNo = acc.accountNo;
    gLastAccName = acc.name;
    gShowReceipt = true;

    // Hide existing controls so they don't show under the overlay
    ClearControls();

    // Create close button centered at bottom of receipt card
    if (gHwnd) {
        RECT rc;
        GetClientRect(gHwnd, &rc);
        // Match the positioning from PaintReceiptOverlay
        int cw = 420, ch = 420;
        int contentWidth = rc.right - SIDEBAR_W;
        int contentHeight = rc.bottom - HEADER_H - STATUS_H;
        int cx = SIDEBAR_W + contentWidth / 2;
        int cy = HEADER_H + contentHeight / 2;
        int cardY = cy - ch / 2;
        // Button at bottom of card
        MakeBtn(BTN_CLOSE_RECEIPT, "Close Receipt", cx - 70, cardY + ch - 50, 140, 40, Primary);
    }

    if (gHwnd) InvalidateRect(gHwnd, NULL, TRUE);
}

static void CreateScreenControls(); // Forward declaration

static void CloseReceipt() {
    gShowReceipt = false;
    // Remove the close button by recreating screen controls
    ClearControls();
    CreateScreenControls();
    if (gHwnd) InvalidateRect(gHwnd, NULL, TRUE);
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

    DrawCard(hdc, cx - 220, cy - 150, 440, 385);
    TxtCenter(hdc, "ATM Customer Login", cx - 220, cy - 125, 440, 32, hFontTitle, Primary);
    TxtCenter(hdc, "Enter your account number and PIN to access ATM services", cx - 220, cy - 90, 440, 20, hFontSmall, TextLight);

    HPEN linePen = CreatePen(PS_SOLID, 1, CardBorder);
    HPEN old = (HPEN)SelectObject(hdc, linePen);
    MoveToEx(hdc, cx - 170, cy - 60, NULL);
    LineTo(hdc, cx + 170, cy - 60);
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
    for (int i = (int)g.transactions.size() - 1; i >= 0 && i >= (int)g.transactions.size() - shown; i--) {
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
        {"Pay Loan", "Repay loan installments or payoff", RGB(245,158,11), SCR_ATM_REPAY_LOAN},
        {"Apply Loan", "Apply for a new bank loan", RGB(16,185,129), SCR_ATM_APPLY_LOAN},
    };

    int cardW = (cw - 30) / 3;
    int cardH = 110;
    for (int i = 0; i < 8; i++) {
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

    if (!gTransferVerified) {
        // Phase 1: Enter account number to verify
        DrawCard(hdc, sx, cardY, cw, 140);
        Txt(hdc, "Step 1: Verify Recipient", sx + 25, cardY + 18, hFontBold, Secondary);
        Txt(hdc, "Target Account #:", sx + 25, cardY + 55, hFontNormal, Text);
        Txt(hdc, "Enter account number and click Verify", sx + 25, cardY + 110, hFontSmall, TextLight);
    } else {
        // Phase 2: Show verified name, enter amount
        DrawCard(hdc, sx, cardY, cw, 250);
        Txt(hdc, "Step 2: Enter Amount", sx + 25, cardY + 18, hFontBold, Secondary);

        // Show verified recipient info
        Txt(hdc, "Recipient:", sx + 25, cardY + 55, hFontNormal, TextLight);
        Txt(hdc, gVerifiedTargetName.c_str(), sx + 150, cardY + 55, hFontBold, Success);

        Txt(hdc, "Account #:", sx + 25, cardY + 85, hFontNormal, TextLight);
        Txt(hdc, to_string(gVerifiedTargetAccNo).c_str(), sx + 150, cardY + 85, hFontNormal, Text);

        // Separator
        HPEN sepPen = CreatePen(PS_SOLID, 1, RGB(220, 220, 220));
        HPEN oldSep = (HPEN)SelectObject(hdc, sepPen);
        MoveToEx(hdc, sx + 20, cardY + 115, NULL);
        LineTo(hdc, sx + cw - 20, cardY + 115);
        SelectObject(hdc, oldSep);
        DeleteObject(sepPen);

        Txt(hdc, "Amount (Rs.):", sx + 25, cardY + 135, hFontBold, Text);
        Txt(hdc, "Amount must be multiple of Rs. 500", sx + 25, cardY + 210, hFontSmall, TextLight);
    }
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
    for (int i = (int)filtered.size() - 1; i >= 0 && i >= (int)filtered.size() - shown; i--) {
        if (rowY > sy + ch - 30) break;
        auto& t = filtered[i];
        Txt(hdc, t.transactionID.c_str(), colX[0], rowY, hFontMono, Text);
        Txt(hdc, t.type.c_str(), colX[1], rowY, hFontNormal, Text);

        stringstream ss;
        ss << "Rs. " << fixed << setprecision(2) << t.amount;
        COLORREF clr = (t.type == "deposit") ? Success : (t.type == "withdrawal" ? Error : Secondary);
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

// ============================
// ATM LOAN REPAYMENT SCREEN
// ============================
static void PaintATMRepayLoan(HDC hdc, RECT rc) {
    int sx = SIDEBAR_W + 30;
    int sy = HEADER_H + 25;
    int cw = min(580, (int)(rc.right - SIDEBAR_W - 60));

    Txt(hdc, "Loan Repayment", sx, sy, hFontHeading, Primary);
    Txt(hdc, "Pay your monthly loan installment or pay off your loan balance", sx, sy + 28, hFontSmall, TextLight);
    HPEN linePen = CreatePen(PS_SOLID, 2, Accent);
    HPEN old = (HPEN)SelectObject(hdc, linePen);
    MoveToEx(hdc, sx, sy + 46, NULL);
    LineTo(hdc, sx + 180, sy + 46);
    SelectObject(hdc, old);
    DeleteObject(linePen);

    int cardY = sy + 58;

    if (g.currentAccIdx < 0 || g.currentAccIdx >= (int)g.accounts.size()) {
        DrawCard(hdc, sx, cardY, cw, 150);
        Txt(hdc, "Please log in to access loan repayment.", sx + 30, cardY + 50, hFontBold, Text);
        return;
    }

    int accNo = g.accounts[g.currentAccIdx].accountNo;
    loadLoans(g.loans);

    int activeLoanIdx = -1;
    for (size_t i = 0; i < g.loans.size(); i++) {
        if (g.loans[i].accountNo == accNo && (g.loans[i].status == "active" || g.loans[i].status == "approved") && g.loans[i].remainingAmount > 0.01) {
            activeLoanIdx = (int)i;
            break;
        }
    }

    if (activeLoanIdx < 0) {
        DrawCard(hdc, sx, cardY, cw, 160);
        Txt(hdc, "No Active Loans Found", sx + 30, cardY + 30, hFontBold, Primary);
        Txt(hdc, "You do not have any active outstanding loan balance to repay at this time.", sx + 30, cardY + 65, hFontNormal, TextLight);
        return;
    }

    auto& loan = g.loans[activeLoanIdx];

    // Card 1: Loan Overview
    DrawCard(hdc, sx, cardY, cw, 180);
    Txt(hdc, "Active Loan Details", sx + 25, cardY + 20, hFontBold, Primary);

    stringstream s1, s2, s3, s4;
    s1 << "Loan ID: #" << loan.loanId << "   |   Original Amount: Rs. " << fixed << setprecision(2) << loan.amount;
    Txt(hdc, s1.str().c_str(), sx + 25, cardY + 55, hFontNormal, Text);

    s2 << "Monthly Installment: Rs. " << fixed << setprecision(2) << loan.monthlyPayment;
    Txt(hdc, s2.str().c_str(), sx + 25, cardY + 85, hFontBold, RGB(217, 119, 6));

    s3 << "Remaining Balance: Rs. " << fixed << setprecision(2) << loan.remainingAmount;
    Txt(hdc, s3.str().c_str(), sx + 25, cardY + 115, hFontTitle, Error);

    s4 << "Repayment Progress: " << loan.monthsPaid << " / " << loan.termMonths << " months paid";
    Txt(hdc, s4.str().c_str(), sx + 25, cardY + 150, hFontSmall, TextLight);

    // Card 2: Repayment Options
    int card2Y = cardY + 195;
    DrawCard(hdc, sx, card2Y, cw, 205);
    Txt(hdc, "Make a Payment", sx + 25, card2Y + 18, hFontBold, Primary);

    Txt(hdc, "Quick Option:", sx + 25, card2Y + 60, hFontBold, Text);
    Txt(hdc, "Custom Repayment (Rs.):", sx + 25, card2Y + 112, hFontBold, Text);

    stringstream sBal;
    sBal << "Your Account Balance: Rs. " << fixed << setprecision(2) << g.accounts[g.currentAccIdx].balance;
    Txt(hdc, sBal.str().c_str(), sx + 25, card2Y + 162, hFontSmall, Success);
}

// ============================
// ATM APPLY LOAN SCREEN
// ============================
static void PaintATMApplyLoan(HDC hdc, RECT rc) {
    int sx = SIDEBAR_W + 30;
    int sy = HEADER_H + 25;
    int cw = min(580, (int)(rc.right - SIDEBAR_W - 60));

    Txt(hdc, "Apply for a Loan", sx, sy, hFontHeading, Primary);
    Txt(hdc, "Request a new loan application - bank administrator will review your request", sx, sy + 28, hFontSmall, TextLight);
    HPEN linePen = CreatePen(PS_SOLID, 2, Accent);
    HPEN old = (HPEN)SelectObject(hdc, linePen);
    MoveToEx(hdc, sx, sy + 46, NULL);
    LineTo(hdc, sx + 200, sy + 46);
    SelectObject(hdc, old);
    DeleteObject(linePen);

    int cardY = sy + 58;

    if (g.currentAccIdx < 0 || g.currentAccIdx >= (int)g.accounts.size()) {
        DrawCard(hdc, sx, cardY, cw, 150);
        Txt(hdc, "Please log in to apply for a loan.", sx + 30, cardY + 50, hFontBold, Text);
        return;
    }

    int accNo = g.accounts[g.currentAccIdx].accountNo;
    loadLoans(g.loans);

    int activeLoanIdx = -1;
    int pendingLoanIdx = -1;

    for (size_t i = 0; i < g.loans.size(); i++) {
        if (g.loans[i].accountNo == accNo) {
            if ((g.loans[i].status == "active" || g.loans[i].status == "approved") && g.loans[i].remainingAmount > 0.01) {
                activeLoanIdx = (int)i;
                break;
            } else if (g.loans[i].status == "pending") {
                pendingLoanIdx = (int)i;
            }
        }
    }

    if (pendingLoanIdx >= 0) {
        auto& pLoan = g.loans[pendingLoanIdx];
        DrawCard(hdc, sx, cardY, cw, 260);
        Txt(hdc, "Loan Application Pending Review", sx + 30, cardY + 25, hFontBold, Warning);
        
        stringstream ps1, ps2;
        ps1 << "Loan ID: #" << pLoan.loanId << "   |   Requested Amount: Rs. " << fixed << setprecision(2) << pLoan.amount;
        Txt(hdc, ps1.str().c_str(), sx + 30, cardY + 60, hFontNormal, Text);

        ps2 << "Term: " << pLoan.termMonths << " months   |   Est. Monthly Payment: Rs. " << fixed << setprecision(2) << pLoan.monthlyPayment;
        Txt(hdc, ps2.str().c_str(), sx + 30, cardY + 88, hFontNormal, TextLight);

        Txt(hdc, "Status: Pending Administrator Review", sx + 30, cardY + 116, hFontBold, Warning);

        HBRUSH noteBg = CreateSolidBrush(RGB(254, 243, 199));
        HPEN notePen = CreatePen(PS_SOLID, 1, RGB(252, 211, 77));
        RoundRect2(hdc, sx + 25, cardY + 146, cw - 50, 42, 8, noteBg, notePen);
        DeleteObject(noteBg);
        DeleteObject(notePen);
        TxtCenter(hdc, "Your loan application has been submitted and is currently awaiting administrator review.", sx + 25, cardY + 157, cw - 50, 20, hFontSmall, RGB(180, 83, 9));
        return;
    }

    if (activeLoanIdx >= 0) {
        DrawCard(hdc, sx, cardY, cw, 170);
        Txt(hdc, "Active Loan Exists", sx + 30, cardY + 30, hFontBold, Primary);
        Txt(hdc, "You currently have an active loan. Please repay your active loan before applying for a new one.", sx + 30, cardY + 65, hFontNormal, TextLight);
        return;
    }

    // Form to apply for a loan
    DrawCard(hdc, sx, cardY, cw, 235);
    Txt(hdc, "Apply for a New Loan", sx + 25, cardY + 20, hFontBold, Primary);

    Txt(hdc, "Requested Amount (Rs.):", sx + 25, cardY + 62, hFontBold, Text);
    Txt(hdc, "Term (Months):",           sx + 25, cardY + 112, hFontBold, Text);

    stringstream sLimit;
    sLimit << "Maximum Eligible Loan (3x Balance): Rs. " << fixed << setprecision(2) << (g.accounts[g.currentAccIdx].balance * 3.0);
    Txt(hdc, sLimit.str().c_str(), sx + 25, cardY + 148, hFontSmall, Success);
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

// ============================
// FORGOT PIN SCREEN (ATM side) - Standalone Page
// ============================
static void PaintForgotPIN(HDC hdc, RECT rc) {
    int cx = rc.right / 2;
    int cy = HEADER_H + (rc.bottom - HEADER_H - STATUS_H) / 2;
    int cw = 640;
    int ch = 440;
    int cardX = cx - cw / 2;
    int cardY = cy - ch / 2;

    // Title & Subtitle centered above card
    TxtCenter(hdc, "Forgot / Reset PIN", 0, cardY - 60, rc.right, 30, hFontHeading, Primary);
    TxtCenter(hdc, "Verify your Full Name and CNIC to set a new 4-digit PIN for your account", 0, cardY - 30, rc.right, 20, hFontSmall, TextLight);

    // Card Container
    DrawCard(hdc, cardX, cardY, cw, ch);

    // Field labels inside card
    Txt(hdc, "Account Number:", cardX + 40, cardY + 28,  hFontBold, Text);
    Txt(hdc, "Full Name:",      cardX + 40, cardY + 78,  hFontBold, Text);
    Txt(hdc, "CNIC:",           cardX + 40, cardY + 128, hFontBold, Text);
    Txt(hdc, "New PIN:",        cardX + 40, cardY + 178, hFontBold, Text);
    Txt(hdc, "Confirm PIN:",    cardX + 40, cardY + 228, hFontBold, Text);

    // Info note box inside card bottom
    HBRUSH noteBg = CreateSolidBrush(RGB(239, 246, 255));
    HPEN notePen = CreatePen(PS_SOLID, 1, RGB(147, 197, 253));
    RoundRect2(hdc, cardX + 30, cardY + 355, cw - 60, 48, 8, noteBg, notePen);
    DeleteObject(noteBg);
    DeleteObject(notePen);
    TxtCenter(hdc, "Enter your registered CNIC and Full Name to reset your PIN securely.", cardX + 30, cardY + 369, cw - 60, 20, hFontSmall, RGB(30, 64, 175));
}

// ============================
// CONTACT ADMIN SCREEN (ATM side) - Standalone Page
// ============================
static void PaintContactAdmin(HDC hdc, RECT rc) {
    int cx = rc.right / 2;
    int cy = HEADER_H + (rc.bottom - HEADER_H - STATUS_H) / 2;
    int cw = 620;
    int ch = 410;
    int cardX = cx - cw / 2;
    int cardY = cy - ch / 2;

    // Title & Subtitle centered above card
    TxtCenter(hdc, "Request Account Reactivation", 0, cardY - 60, rc.right, 30, hFontHeading, Primary);
    TxtCenter(hdc, "Fill in your details and reason - the bank administrator will review your request", 0, cardY - 30, rc.right, 20, hFontSmall, TextLight);

    // Card Container
    DrawCard(hdc, cardX, cardY, cw, ch);

    // Field labels inside card
    Txt(hdc, "Account Number:", cardX + 40, cardY + 30,  hFontBold, Text);
    Txt(hdc, "Full Name:",      cardX + 40, cardY + 80,  hFontBold, Text);
    Txt(hdc, "CNIC:",           cardX + 40, cardY + 130, hFontBold, Text);
    Txt(hdc, "Reason:",         cardX + 40, cardY + 180, hFontBold, Text);

    // Info note box inside card bottom
    HBRUSH noteBg = CreateSolidBrush(RGB(239, 246, 255));
    HPEN notePen = CreatePen(PS_SOLID, 1, RGB(147, 197, 253));
    RoundRect2(hdc, cardX + 30, cardY + 315, cw - 60, 48, 8, noteBg, notePen);
    DeleteObject(noteBg);
    DeleteObject(notePen);
    TxtCenter(hdc, "Your request will be submitted directly to the bank administrator for review.", cardX + 30, cardY + 329, cw - 60, 20, hFontSmall, RGB(30, 64, 175));
}

// ============================
// ADMIN REACTIVATION REQUESTS SCREEN
// ============================
static void PaintAdminRequests(HDC hdc, RECT rc) {
    int sx = SIDEBAR_W + 25;
    int sy = HEADER_H + 25;
    int cw = rc.right - SIDEBAR_W - 50;
    int ch = rc.bottom - HEADER_H - STATUS_H - 50;

    Txt(hdc, "Reactivation Requests", sx, sy, hFontHeading, Primary);
    Txt(hdc, "Review and approve customer account reactivation requests", sx, sy + 28, hFontSmall, TextLight);
    HPEN linePen = CreatePen(PS_SOLID, 2, Accent);
    HPEN old = (HPEN)SelectObject(hdc, linePen);
    MoveToEx(hdc, sx, sy + 46, NULL);
    LineTo(hdc, sx + 270, sy + 46);
    SelectObject(hdc, old);
    DeleteObject(linePen);

    DrawCard(hdc, sx, sy + 58, cw, ch - 68);

    // Table header
    RECT tblHdr = {sx + 10, sy + 63, sx + cw - 10, sy + 93};
    HBRUSH tblHdrBr = CreateSolidBrush(RGB(240, 243, 248));
    FillRect(hdc, &tblHdr, tblHdrBr);
    DeleteObject(tblHdrBr);

    int colX[] = {sx + 15, sx + 65, sx + 180, sx + 320, sx + 435, sx + 550, sx + 665};
    const char* colH[] = {"#", "Acct", "Name", "CNIC", "Date", "Status", "Actions"};
    for (int i = 0; i < 7; i++)
        Txt(hdc, colH[i], colX[i], sy + 70, hFontBold, TextLight);

    int rowY = sy + 100;
    int shown = min((int)g.reactivationRequests.size(), 14);
    for (int i = (int)g.reactivationRequests.size() - 1; i >= (int)g.reactivationRequests.size() - shown && i >= 0; i--) {
        if (rowY > sy + ch - 30) break;
        auto& req = g.reactivationRequests[i];

        // Row separator
        HPEN rowPen = CreatePen(PS_SOLID, 1, RGB(241, 245, 249));
        HPEN rOld = (HPEN)SelectObject(hdc, rowPen);
        MoveToEx(hdc, sx + 10, rowY + 24, NULL);
        LineTo(hdc, sx + cw - 10, rowY + 24);
        SelectObject(hdc, rOld);
        DeleteObject(rowPen);

        Txt(hdc, to_string(req.requestId).c_str(), colX[0], rowY, hFontMono, Text);
        Txt(hdc, to_string(req.accountNo).c_str(), colX[1], rowY, hFontNormal, Text);

        // Truncate name if too long
        string dispName = req.name.length() > 14 ? req.name.substr(0, 13) + "." : req.name;
        Txt(hdc, dispName.c_str(), colX[2], rowY, hFontNormal, Text);
        Txt(hdc, req.cnic.c_str(), colX[3], rowY, hFontSmall, Text);

        string dispDate = req.dateTime.length() >= 10 ? req.dateTime.substr(0, 10) : req.dateTime;
        Txt(hdc, dispDate.c_str(), colX[4], rowY, hFontSmall, TextLight);

        // Color-coded status
        COLORREF stClr = (req.status == "pending") ? RGB(217, 119, 6) :
                         (req.status == "approved") ? Success : Error;
        Txt(hdc, req.status.c_str(), colX[5], rowY, hFontBold, stClr);

        rowY += 32;
    }

    // Empty state
    if (g.reactivationRequests.empty()) {
        Txt(hdc, "No reactivation requests found.", sx + 25, sy + 105, hFontNormal, TextLight);
    }
}

static void PaintAdminTxns(HDC hdc, RECT rc) {
    int sx = SIDEBAR_W + 25;
    int sy = HEADER_H + 20;
    int cw = rc.right - SIDEBAR_W - 50;
    int ch = rc.bottom - HEADER_H - STATUS_H - 35;

    Txt(hdc, "Transaction History", sx, sy, hFontHeading, Primary);
    Txt(hdc, "Filter and view complete transaction log", sx, sy + 25, hFontSmall, TextLight);
    HPEN linePen = CreatePen(PS_SOLID, 2, Accent);
    HPEN old = (HPEN)SelectObject(hdc, linePen);
    MoveToEx(hdc, sx, sy + 44, NULL);
    LineTo(hdc, sx + 220, sy + 44);
    SelectObject(hdc, old);
    DeleteObject(linePen);

    // 1. Search / Filter Card (Top)
    int fY = sy + 52;
    int filterH = 145;
    DrawCard(hdc, sx, fY, cw, filterH);

    int col1X = sx + 20, col2X = sx + cw/2 + 10;
    Txt(hdc, "Account #:",  col1X, fY + 20, hFontBold, TextLight);
    Txt(hdc, "Type:",       col2X, fY + 20, hFontBold, TextLight);
    Txt(hdc, "From Date:",  col1X, fY + 60, hFontBold, TextLight);
    Txt(hdc, "To Date:",    col2X, fY + 60, hFontBold, TextLight);
    Txt(hdc, "Min Amount:", col1X, fY + 100, hFontBold, TextLight);
    Txt(hdc, "Max Amount:", col2X, fY + 100, hFontBold, TextLight);

    // 2. Transaction Results Card (Bottom)
    int tableY = fY + filterH + 15;
    int tableH = max(200, ch - (tableY - sy));
    DrawCard(hdc, sx, tableY, cw, tableH);

    RECT tblHdr = {sx + 10, tableY + 10, sx + cw - 10, tableY + 38};
    HBRUSH tblHdrBr = CreateSolidBrush(RGB(240,243,248));
    FillRect(hdc, &tblHdr, tblHdrBr);
    DeleteObject(tblHdrBr);

    int colX[] = {sx+15, sx+120, sx+240, sx+370, sx+500, sx+620};
    const char* colH[] = {"ID", "Account", "Type", "Amount", "Date", "Balance"};
    for (int i = 0; i < 6; i++)
        Txt(hdc, colH[i], colX[i], tableY + 16, hFontBold, TextLight);

    int rowY = tableY + 45;
    if (g.transactions.empty()) {
        Txt(hdc, "No transactions found matching criteria.", sx + 20, rowY + 5, hFontNormal, TextLight);
    } else {
        int shown = min((int)g.transactions.size(), 30);
        for (int i = (int)g.transactions.size() - 1; i >= 0 && i >= (int)g.transactions.size() - shown; i--) {
            if (rowY > tableY + tableH - 30) break;
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
}

static void PaintAdminSearchTxn(HDC hdc, RECT rc) {
    PaintAdminTxns(hdc, rc);
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
    int sy = HEADER_H + 20;
    int cw = rc.right - SIDEBAR_W - 50;
    int ch = rc.bottom - HEADER_H - STATUS_H - 40;

    Txt(hdc, "Loan Applications & Management", sx, sy, hFontHeading, Primary);
    Txt(hdc, "Review and approve or reject customer loan requests", sx, sy + 25, hFontSmall, TextLight);
    HPEN linePen = CreatePen(PS_SOLID, 2, Accent);
    HPEN old = (HPEN)SelectObject(hdc, linePen);
    MoveToEx(hdc, sx, sy + 44, NULL);
    LineTo(hdc, sx + 220, sy + 44);
    SelectObject(hdc, old);
    DeleteObject(linePen);

    // Single full-height Card for Loans table
    int tableY = sy + 52;
    int tableH = max(250, ch - 52);
    DrawCard(hdc, sx, tableY, cw, tableH);

    // Table Header
    RECT tblHdr = {sx + 10, tableY + 10, sx + cw - 10, tableY + 42};
    HBRUSH tblHdrBr = CreateSolidBrush(RGB(240, 243, 248));
    FillRect(hdc, &tblHdr, tblHdrBr);
    DeleteObject(tblHdrBr);

    int actionsX = sx + 615;
    int colX[] = {sx + 20, sx + 75, sx + 175, sx + 295, sx + 420, sx + 525, actionsX};
    const char* colH[] = {"ID", "Account", "Amount", "Monthly", "Status", "Paid", "Actions"};
    for (int i = 0; i < 7; i++)
        Txt(hdc, colH[i], colX[i], tableY + 18, hFontBold, TextLight);

    int rowY = tableY + 50;
    int rowH = 36;

    if (g.loans.empty()) {
        Txt(hdc, "No loan applications found.", sx + 20, rowY + 10, hFontNormal, TextLight);
    } else {
        for (size_t i = 0; i < g.loans.size(); i++) {
            if (rowY + rowH > tableY + tableH - 10) break;
            auto& l = g.loans[i];

            Txt(hdc, to_string(l.loanId).c_str(), colX[0], rowY + 8, hFontMono, Text);
            Txt(hdc, to_string(l.accountNo).c_str(), colX[1], rowY + 8, hFontNormal, Text);
            stringstream ss; ss << "Rs. " << fixed << setprecision(0) << l.amount;
            Txt(hdc, ss.str().c_str(), colX[2], rowY + 8, hFontNormal, Text);
            stringstream ms; ms << "Rs. " << fixed << setprecision(0) << l.monthlyPayment;
            Txt(hdc, ms.str().c_str(), colX[3], rowY + 8, hFontNormal, Text);

            COLORREF clr = (l.status == "pending") ? RGB(217, 119, 6) :
                           (l.status == "active" || l.status == "approved") ? Success :
                           (l.status == "completed") ? Primary : Error;
            Txt(hdc, l.status.c_str(), colX[4], rowY + 8, hFontBold, clr);

            stringstream ps; ps << l.monthsPaid << "/" << l.termMonths;
            Txt(hdc, ps.str().c_str(), colX[5], rowY + 8, hFontNormal, Text);

            HPEN sepPen = CreatePen(PS_SOLID, 1, RGB(235, 240, 248));
            HPEN oldSep = (HPEN)SelectObject(hdc, sepPen);
            MoveToEx(hdc, sx + 15, rowY + rowH - 2, NULL);
            LineTo(hdc, sx + cw - 15, rowY + rowH - 2);
            SelectObject(hdc, oldSep);
            DeleteObject(sepPen);

            rowY += rowH;
        }
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
// RECEIPT OVERLAY
// ============================
static void PaintReceiptOverlay(HDC hdc, RECT rc) {
    if (!gShowReceipt) return;

    // Dark overlay covering the content area (not sidebar/header)
    HBRUSH overlayBr = CreateSolidBrush(RGB(30, 30, 30));
    RECT overlayRc = {SIDEBAR_W, HEADER_H, rc.right, rc.bottom - STATUS_H};
    FillRect(hdc, &overlayRc, overlayBr);
    DeleteObject(overlayBr);

    SetBkMode(hdc, TRANSPARENT);

    // Calculate card position - center in content area (excluding sidebar)
    int cw = 420, ch = 420;
    int contentWidth = rc.right - SIDEBAR_W;
    int contentHeight = rc.bottom - HEADER_H - STATUS_H;
    int cx = SIDEBAR_W + contentWidth / 2;
    int cy = HEADER_H + contentHeight / 2;
    int cardX = cx - cw / 2;
    int cardY = cy - ch / 2;

    // Card shadow
    HBRUSH shadowBr = CreateSolidBrush(RGB(0, 0, 0));
    RECT shadowRc = {cardX + 6, cardY + 6, cardX + cw + 6, cardY + ch + 6};
    FillRect(hdc, &shadowRc, shadowBr);
    DeleteObject(shadowBr);

    // White card background with border
    HBRUSH cardBr = CreateSolidBrush(RGB(255, 255, 255));
    HPEN borderPen = CreatePen(PS_SOLID, 1, RGB(200, 200, 200));
    HPEN oldPen = (HPEN)SelectObject(hdc, borderPen);
    HBRUSH oldBr = (HBRUSH)SelectObject(hdc, cardBr);
    RoundRect(hdc, cardX, cardY, cardX + cw, cardY + ch, 16, 16);
    SelectObject(hdc, oldPen);
    SelectObject(hdc, oldBr);
    DeleteObject(cardBr);
    DeleteObject(borderPen);

    // Green header bar
    HBRUSH headerBr = CreateSolidBrush(Success);
    RECT headerRc = {cardX + 1, cardY + 1, cardX + cw - 1, cardY + 65};
    FillRect(hdc, &headerRc, headerBr);
    DeleteObject(headerBr);

    // Receipt title
    TxtCenter(hdc, "Transaction Receipt", cardX, cardY + 15, cw, 26, hFontHeading, RGB(255, 255, 255));
    TxtCenter(hdc, "National Bank ATM", cardX, cardY + 42, cw, 18, hFontSmall, RGB(220, 255, 220));

    // Transaction details - properly aligned
    int y = cardY + 85;
    int lx = cardX + 30;           // Label X position
    int vx = cardX + 150;          // Value X position
    int lineH = 38;

    // Transaction type
    string txnTypeLabel = "Transaction";
    COLORREF typeClr = Text;
    if (gLastTxn.type == "withdrawal") { txnTypeLabel = "Cash Withdrawal"; typeClr = Error; }
    else if (gLastTxn.type == "deposit") { txnTypeLabel = "Cash Deposit"; typeClr = Success; }
    else if (gLastTxn.type == "transfer") { txnTypeLabel = "Fund Transfer"; typeClr = Secondary; }

    Txt(hdc, "Type:", lx, y, hFontBold, TextLight);
    Txt(hdc, txnTypeLabel.c_str(), vx, y, hFontBold, typeClr);
    y += lineH;

    Txt(hdc, "Txn ID:", lx, y, hFontBold, TextLight);
    Txt(hdc, gLastTxn.transactionID.c_str(), vx, y, hFontMono, Text);
    y += lineH;

    Txt(hdc, "Account:", lx, y, hFontBold, TextLight);
    string accStr = to_string(gLastAccNo) + " - " + gLastAccName;
    Txt(hdc, accStr.c_str(), vx, y, hFontNormal, Text);
    y += lineH;

    Txt(hdc, "Amount:", lx, y, hFontBold, TextLight);
    stringstream amtSs;
    amtSs << "Rs. " << fixed << setprecision(2) << gLastTxn.amount;
    Txt(hdc, amtSs.str().c_str(), vx, y, hFontHeading, typeClr);
    y += lineH;

    Txt(hdc, "Balance:", lx, y, hFontBold, TextLight);
    stringstream balSs;
    balSs << "Rs. " << fixed << setprecision(2) << gLastTxn.resultingBalance;
    Txt(hdc, balSs.str().c_str(), vx, y, hFontBold, Success);
    y += lineH;

    Txt(hdc, "Date/Time:", lx, y, hFontBold, TextLight);
    Txt(hdc, gLastTxn.dateTime.c_str(), vx, y, hFontNormal, TextLight);
    y += lineH;

    if (!gLastTxn.details.empty()) {
        Txt(hdc, "Details:", lx, y, hFontBold, TextLight);
        Txt(hdc, gLastTxn.details.c_str(), vx, y, hFontNormal, TextLight);
    }

    // Separator line
    HPEN sepPen = CreatePen(PS_SOLID, 1, RGB(220, 220, 220));
    oldPen = (HPEN)SelectObject(hdc, sepPen);
    MoveToEx(hdc, cardX + 25, cardY + ch - 70, NULL);
    LineTo(hdc, cardX + cw - 25, cardY + ch - 70);
    SelectObject(hdc, oldPen);
    DeleteObject(sepPen);
}

// ============================
// MAIN PAINT DISPATCHER
// ============================
static void PaintScreen(HDC hdc, RECT rc) {
    FillRect(hdc, &rc, hBrushBg);
    DrawHeader(hdc, rc, g);

    if (g.screen != SCR_LOGIN && g.screen != SCR_ADMIN_LOGIN && g.screen != SCR_ATM_LOGIN && g.screen != SCR_CONTACT_ADMIN && g.screen != SCR_FORGOT_PIN) {
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
        case SCR_ATM_REPAY_LOAN: PaintATMRepayLoan(hdc, rc); break;
        case SCR_ATM_APPLY_LOAN: PaintATMApplyLoan(hdc, rc); break;
        case SCR_OTP: PaintOTPScreen(hdc, rc); break;
        case SCR_CONTACT_ADMIN: PaintContactAdmin(hdc, rc); break;
        case SCR_FORGOT_PIN: PaintForgotPIN(hdc, rc); break;
        case SCR_ADMIN_REQUESTS: PaintAdminRequests(hdc, rc); break;
    }

    // Draw receipt overlay if visible
    if (gShowReceipt) {
        PaintReceiptOverlay(hdc, rc);
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

    // Header buttons (Logout / Backup + Notification Bell)
    bool isPublicScreen = (g.screen == SCR_LOGIN || g.screen == SCR_ADMIN_LOGIN || g.screen == SCR_ATM_LOGIN || g.screen == SCR_CONTACT_ADMIN || g.screen == SCR_FORGOT_PIN);
    if (g.isAdmin && !isPublicScreen) {
        MakeBtn(BTN_NOTIF_REQUESTS, "",            rc.right - 280, 14, 42, 36, RGB(30,41,59));
        MakeBtn(BTN_DO_BACKUP,     "Backup Data", rc.right - 225, 14, 115, 36, Primary);
        MakeBtn(BTN_LOGOUT,        "Logout",      rc.right - 100, 14, 85,  36, Error);
    } else if (!g.isAdmin && g.currentAccIdx >= 0 && !isPublicScreen) {
        MakeBtn(BTN_LOGOUT,        "Logout",      rc.right - 100, 14, 85,  36, Error);
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
        MakeLabel("Account Number:", cx - 170, cy - 35, 140, 24, 1);
        MakeEdit(EDT_ACCNO,           cx - 20,  cy - 40, 190, 34);
        MakeLabel("PIN:",            cx - 170, cy + 15, 140, 24, 1);
        MakeEdit(EDT_PIN,             cx - 20,  cy + 10, 190, 34, true);
        MakeBtn(BTN_DO_LOGIN,        "Login",  cx - 170, cy + 65, 160, 40, Success);
        MakeBtn(BTN_BACK,            "Back",   cx + 10,  cy + 65, 160, 40, RGB(100, 116, 139));
        MakeBtn(BTN_FORGOT_PIN_SCREEN, "Forgot PIN?", cx - 170, cy + 118, 340, 36, Primary);
        MakeBtn(BTN_CONTACT_ADMIN_SCREEN, "Account Blocked? Contact Admin", cx - 170, cy + 162, 340, 36, Warning);

        if (g.pendingReactivationAccNo > 0) {
            SetEditText(EDT_ACCNO, to_string(g.pendingReactivationAccNo));
        }
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
    case SCR_ADMIN_TXNS:
    case SCR_ADMIN_SEARCH_TXN: {
        int cw = rc.right - SIDEBAR_W - 50;
        int fY = sy + 52;
        int col1V = sx + 120, col2V = sx + cw/2 + 110;
        MakeEdit(EDT_SEARCH_TXN_ACC,  col1V, fY + 14, 140, 30);
        MakeEdit(EDT_SEARCH_TXN_TYPE, col2V, fY + 14, 140, 30);
        MakeEdit(EDT_SEARCH_TXN_FROM, col1V, fY + 54, 140, 30);
        MakeEdit(EDT_SEARCH_TXN_TO,   col2V, fY + 54, 140, 30);
        MakeEdit(EDT_SEARCH_TXN_MIN,  col1V, fY + 94, 110, 30);
        MakeEdit(EDT_SEARCH_TXN_MAX,  col2V, fY + 94, 110, 30);
        MakeBtn(BTN_DO_SEARCH_TXN, "Search", sx + cw - 240, fY + 93, 105, 32, Primary);
        MakeBtn(BTN_CLEAR_SEARCH,  "Reset",  sx + cw - 125, fY + 93, 105, 32, RGB(100,116,139));
        break;
    }
    case SCR_ADMIN_AUDIT: break;
    case SCR_ADMIN_LOANS: {
        loadLoans(g.loans);
        int tableY = sy + 52;
        int rowY = tableY + 50;
        int rowH = 36;
        int cw = rc.right - SIDEBAR_W - 50;
        int ch = rc.bottom - HEADER_H - STATUS_H - 40;
        int tableH = max(250, ch - 52);
        int actionsX = sx + 615;

        for (size_t i = 0; i < g.loans.size(); i++) {
            if (rowY + rowH > tableY + tableH - 10) break;
            auto& l = g.loans[i];
            if (l.status == "pending") {
                MakeBtn(BTN_DYNAMIC_LOAN_APPROVE_BASE + (int)i, "Approve", actionsX, rowY + 5, 68, 25, Success);
                MakeBtn(BTN_DYNAMIC_LOAN_REJECT_BASE + (int)i, "Reject",  actionsX + 74, rowY + 5, 68, 25, Error);
            }
            rowY += rowH;
        }
        break;
    }
    case SCR_ADMIN_CASH: break;
    case SCR_ADMIN_DAILY: break;
    case SCR_ATM_MENU: break; // Card clicks handled in WM_LBUTTONDOWN
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
        if (!gTransferVerified) {
            // Phase 1: Enter account number to verify
            MakeEdit(EDT_TRANSFER_TARGET, sx + 185, cardY + 48, 220, 36);
            MakeBtn(BTN_VERIFY_ACCOUNT, "Verify Account", sx + 185, cardY + 95, 140, 38, Success);
            MakeBtn(BTN_BACK, "Back", sx + 335, cardY + 95, 90, 38, RGB(100,116,139));
        } else {
            // Phase 2: Enter amount and transfer
            MakeEdit(EDT_TRANSFER_AMT, sx + 185, cardY + 128, 220, 36);
            MakeBtn(BTN_DO_TRANSFER, "Send Money", sx + 185, cardY + 175, 130, 40, Secondary);
            MakeBtn(BTN_BACK, "Cancel", sx + 325, cardY + 175, 100, 40, RGB(100,116,139));
        }
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
    case SCR_ATM_REPAY_LOAN: {
        int cardY = sy + 58;
        int cw = min(580, (int)(rc.right - SIDEBAR_W - 60));
        bool hasActiveLoan = false;
        if (g.currentAccIdx >= 0 && g.currentAccIdx < (int)g.accounts.size()) {
            int accNo = g.accounts[g.currentAccIdx].accountNo;
            loadLoans(g.loans);
            for (const auto& l : g.loans) {
                if (l.accountNo == accNo && (l.status == "active" || l.status == "approved") && l.remainingAmount > 0.01) {
                    hasActiveLoan = true;
                    break;
                }
            }
        }
        if (hasActiveLoan) {
            int card2Y = cardY + 195;
            MakeBtn(BTN_DO_REPAY_INSTALLMENT, "Pay Monthly Installment", sx + 210, card2Y + 52, 210, 36, Primary);
            MakeEdit(EDT_REPAY_AMT, sx + 210, card2Y + 104, 115, 34);
            MakeBtn(BTN_DO_REPAY_CUSTOM, "Pay Amount", sx + 335, card2Y + 104, 105, 34, Success);
            MakeBtn(BTN_DO_REPAY_FULL, "Pay Full Loan", sx + 450, card2Y + 104, 105, 34, Warning);
            MakeBtn(BTN_BACK, "Back", sx + cw - 95, card2Y + 155, 80, 34, RGB(100, 116, 139));
        } else {
            MakeBtn(BTN_BACK, "Back", sx + 30, cardY + 105, 100, 36, RGB(100, 116, 139));
        }
        break;
    }
    case SCR_ATM_APPLY_LOAN: {
        int cardY = sy + 58;
        int cw = min(580, (int)(rc.right - SIDEBAR_W - 60));
        int pendingLoanIdx = -1;
        int activeLoanIdx = -1;
        if (g.currentAccIdx >= 0 && g.currentAccIdx < (int)g.accounts.size()) {
            int accNo = g.accounts[g.currentAccIdx].accountNo;
            loadLoans(g.loans);
            for (size_t i = 0; i < g.loans.size(); i++) {
                if (g.loans[i].accountNo == accNo) {
                    if ((g.loans[i].status == "active" || g.loans[i].status == "approved") && g.loans[i].remainingAmount > 0.01) {
                        activeLoanIdx = (int)i;
                        break;
                    } else if (g.loans[i].status == "pending") {
                        pendingLoanIdx = (int)i;
                    }
                }
            }
        }
        if (pendingLoanIdx >= 0) {
            MakeBtn(BTN_BACK, "Back", sx + 25, cardY + 204, 100, 36, RGB(100, 116, 139));
        } else if (activeLoanIdx >= 0) {
            MakeBtn(BTN_BACK, "Back", sx + 30, cardY + 110, 100, 36, RGB(100, 116, 139));
        } else {
            MakeEdit(EDT_APPLY_LOAN_AMT, sx + 210, cardY + 54, 210, 34);
            MakeEdit(EDT_APPLY_LOAN_TERM, sx + 210, cardY + 104, 115, 34);
            MakeBtn(BTN_DO_APPLY_LOAN, "Submit Loan Application", sx + 25, cardY + 178, 200, 38, Success);
            MakeBtn(BTN_BACK, "Back", sx + 240, cardY + 178, 90, 38, RGB(100, 116, 139));
        }
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
    case SCR_CONTACT_ADMIN: {
        int cx = rc.right / 2;
        int cy = HEADER_H + (rc.bottom - HEADER_H - STATUS_H) / 2;
        int cw = 620;
        int ch = 410;
        int cardX = cx - cw / 2;
        int cardY = cy - ch / 2;

        int edtX = cardX + 210;
        int edtW = 360;
        MakeEdit(EDT_ACCNO,  edtX, cardY + 24,  edtW, 34);
        MakeEdit(EDT_NAME,   edtX, cardY + 74,  edtW, 34);
        MakeEdit(EDT_CNIC,   edtX, cardY + 124, edtW, 34);
        MakeEdit(EDT_REASON, edtX, cardY + 174, edtW, 34);
        MakeBtn(BTN_DO_CONTACT_ADMIN, "Submit Request", edtX,       cardY + 235, 170, 42, Primary);
        MakeBtn(BTN_BACK,             "Back",           edtX + 185, cardY + 235, 120, 42, RGB(100,116,139));
        break;
    }
    case SCR_FORGOT_PIN: {
        int cx = rc.right / 2;
        int cy = HEADER_H + (rc.bottom - HEADER_H - STATUS_H) / 2;
        int cw = 640;
        int ch = 440;
        int cardX = cx - cw / 2;
        int cardY = cy - ch / 2;

        int edtX = cardX + 210;
        int edtW = 380;
        MakeEdit(EDT_ACCNO,      edtX, cardY + 24,  edtW, 34);
        MakeEdit(EDT_NAME,       edtX, cardY + 74,  edtW, 34);
        MakeEdit(EDT_CNIC,       edtX, cardY + 124, edtW, 34);
        MakeEdit(EDT_NEWPIN,     edtX, cardY + 174, edtW, 34, true);
        MakeEdit(EDT_CONFIRMPIN, edtX, cardY + 224, edtW, 34, true);

        if (g.pendingReactivationAccNo > 0) {
            SetEditText(EDT_ACCNO, to_string(g.pendingReactivationAccNo));
        }

        MakeBtn(BTN_DO_FORGOT_PIN, "Reset PIN", edtX,       cardY + 285, 170, 42, Success);
        MakeBtn(BTN_BACK,          "Back",      edtX + 185, cardY + 285, 120, 42, RGB(100,116,139));
        break;
    }
    case SCR_ADMIN_REQUESTS: {
        int colX[] = {sx + 15, sx + 65, sx + 180, sx + 320, sx + 435, sx + 550, sx + 665};
        int rowY = sy + 100;
        int shown = min((int)g.reactivationRequests.size(), 14);
        int startI = (int)g.reactivationRequests.size() - 1;
        int endI   = (int)g.reactivationRequests.size() - shown;
        for (int i = startI; i >= endI && i >= 0; i--) {
            if (rowY > rc.bottom - STATUS_H - 50) break;
            auto& req = g.reactivationRequests[i];
            if (req.status == "pending") {
                int btnApprId = BTN_DYNAMIC_APPROVE_BASE + i;
                int btnRejId  = BTN_DYNAMIC_REJECT_BASE + i;
                MakeBtn(btnApprId, "Approve", colX[6], rowY - 4, 70, 24, Success);
                MakeBtn(btnRejId,  "Reject",  colX[6] + 76, rowY - 4, 60, 24, Error);
            }
            rowY += 32;
        }
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
        g.isAdmin = false;
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

    // Handle the "Contact Admin" navigation button (appears on ATM login for locked/frozen)
    if (id == BTN_CONTACT_ADMIN_SCREEN) {
        GoToScreen(SCR_CONTACT_ADMIN);
        return;
    }

    // Handle the "Forgot PIN?" navigation button
    if (id == BTN_FORGOT_PIN_SCREEN) {
        string accStr = GetEditText(EDT_ACCNO);
        if (!accStr.empty() && isNumeric(accStr)) {
            g.pendingReactivationAccNo = atoi(accStr.c_str());
        }
        GoToScreen(SCR_FORGOT_PIN);
        return;
    }

    // Handle admin notification bell — navigate to reactivation requests
    if (id == BTN_NOTIF_REQUESTS) {
        loadReactivationRequests(g.reactivationRequests);
        GoToScreen(SCR_ADMIN_REQUESTS);
        return;
    }

    // Handle dynamic approve buttons
    if (id >= BTN_DYNAMIC_APPROVE_BASE && id < BTN_DYNAMIC_APPROVE_BASE + 1000) {
        int reqIdx = id - BTN_DYNAMIC_APPROVE_BASE;
        loadReactivationRequests(g.reactivationRequests);
        if (reqIdx >= 0 && reqIdx < (int)g.reactivationRequests.size()) {
            auto& req = g.reactivationRequests[reqIdx];
            req.status = "approved";
            // Auto-unlock the referenced account
            loadAccounts(g.accounts);
            int aIdx = findAccountIndex(req.accountNo, g.accounts);
            if (aIdx >= 0) {
                g.accounts[aIdx].status = "active";
                g.accounts[aIdx].pinAttempts = 0;
                saveAccounts(g.accounts);
            }
            saveReactivationRequests(g.reactivationRequests);
            logAudit("Reactivation Approved", "Request " + to_string(req.requestId) +
                     " for account " + to_string(req.accountNo) + " approved");
            SetStatus("Request approved. Account " + to_string(req.accountNo) + " is now active.", 1);
            SpeakText("Reactivation request approved. Account is now active.");
            GoToScreen(SCR_ADMIN_REQUESTS);
        }
        return;
    }

    // Handle dynamic loan approve buttons
    if (id >= BTN_DYNAMIC_LOAN_APPROVE_BASE && id < BTN_DYNAMIC_LOAN_APPROVE_BASE + 1000) {
        int loanIdx = id - BTN_DYNAMIC_LOAN_APPROVE_BASE;
        loadLoans(g.loans);
        if (loanIdx >= 0 && loanIdx < (int)g.loans.size()) {
            auto& loan = g.loans[loanIdx];
            loan.status = "active";
            
            // Credit account balance
            loadAccounts(g.accounts);
            int aIdx = findAccountIndex(loan.accountNo, g.accounts);
            if (aIdx >= 0) {
                g.accounts[aIdx].balance += loan.amount;
                saveAccounts(g.accounts);
            }
            saveLoans(g.loans);

            // Record transaction
            loadTransactions(g.transactions);
            Transaction txn;
            txn.transactionID = generateTransactionID(g.transactions);
            txn.accountNo = loan.accountNo;
            txn.type = "deposit";
            txn.amount = loan.amount;
            txn.dateTime = getCurrentDateTimeStr();
            txn.resultingBalance = (aIdx >= 0) ? g.accounts[aIdx].balance : loan.amount;
            txn.details = "Loan #" + to_string(loan.loanId) + " approved";
            g.transactions.push_back(txn);
            saveTransactions(g.transactions);

            logAudit("Loan Approved", "Loan ID #" + to_string(loan.loanId) + " approved for account " + to_string(loan.accountNo));
            SetStatus("Loan ID #" + to_string(loan.loanId) + " for account " + to_string(loan.accountNo) + " approved successfully!", 1);
            SpeakText("Loan application approved successfully.");
            GoToScreen(SCR_ADMIN_LOANS);
        }
        return;
    }

    // Handle dynamic loan reject buttons
    if (id >= BTN_DYNAMIC_LOAN_REJECT_BASE && id < BTN_DYNAMIC_LOAN_REJECT_BASE + 1000) {
        int loanIdx = id - BTN_DYNAMIC_LOAN_REJECT_BASE;
        loadLoans(g.loans);
        if (loanIdx >= 0 && loanIdx < (int)g.loans.size()) {
            auto& loan = g.loans[loanIdx];
            loan.status = "rejected";
            saveLoans(g.loans);

            logAudit("Loan Rejected", "Loan ID #" + to_string(loan.loanId) + " rejected for account " + to_string(loan.accountNo));
            SetStatus("Loan ID #" + to_string(loan.loanId) + " rejected.", 2);
            SpeakText("Loan application rejected.");
            GoToScreen(SCR_ADMIN_LOANS);
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
                loadReactivationRequests(g.reactivationRequests);
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
        RECT rc;
        GetClientRect(gHwnd, &rc);
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
            g.pendingReactivationAccNo = accNo;
            SetStatus("Account is frozen. Click 'Account Blocked? Contact Admin' below.", 2);
            break;
        }
        if (g.accounts[idx].status == "locked") {
            g.pendingReactivationAccNo = accNo;
            SetStatus("Account is locked. Click 'Account Blocked? Contact Admin' below.", 2);
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

        if (!isValidName(name)) { SetStatus("Invalid name (letters and spaces only, min 2 chars)", 2); break; }
        if (!isValidCNIC(cnic)) { SetStatus("Invalid CNIC format (XXXXX-XXXXXXX-X)", 2); break; }
        if (!isValidAccountType(type)) { SetStatus("Type must be 'savings' or 'current'", 2); break; }
        if (balStr.empty() || !isNumericDecimal(balStr)) { SetStatus("Enter a valid initial deposit amount", 2); break; }
        double bal = atof(balStr.c_str());
        if (bal < 500.0) { SetStatus("Minimum initial deposit is Rs. 500.00", 2); break; }
        if (bal > 10000000.0) { SetStatus("Initial deposit exceeds maximum allowed limit (Rs. 10,000,000)", 2); break; }
        if (!isValidPIN(pin)) { SetStatus("PIN must be exactly 4 numeric digits", 2); break; }
        if (pin != pin2) { SetStatus("PIN confirmation does not match", 2); break; }

        loadAccounts(g.accounts);
        if (findAccountByCNIC(cnic, g.accounts) >= 0) {
            SetStatus("An account with this CNIC already exists!", 2);
            break;
        }

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
        if (accStr.empty() || !isNumeric(accStr)) { SetStatus("Enter a valid numeric account number", 2); break; }
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
        if (accStr.empty() || !isNumeric(accStr)) { SetStatus("Enter a valid numeric account number", 2); break; }
        int accNo = atoi(accStr.c_str());
        loadAccounts(g.accounts);
        int idx = findAccountIndex(accNo, g.accounts);
        if (idx < 0) { SetStatus("Account not found", 2); break; }
        if (g.accounts[idx].status == "frozen") { SetStatus("Account is already frozen", 2); break; }
        g.accounts[idx].status = "frozen";
        saveAccounts(g.accounts);
        logAudit("Account Frozen", "Account " + to_string(accNo) + " frozen");
        SetStatus("Account frozen successfully", 1);
        SpeakText("Account " + to_string(accNo) + " has been frozen.");
        break;
    }

    case BTN_DO_UNFREEZE: {
        string accStr = GetEditText(EDT_SEARCH);
        if (accStr.empty() || !isNumeric(accStr)) { SetStatus("Enter a valid numeric account number", 2); break; }
        int accNo = atoi(accStr.c_str());
        loadAccounts(g.accounts);
        int idx = findAccountIndex(accNo, g.accounts);
        if (idx < 0) { SetStatus("Account not found", 2); break; }
        if (g.accounts[idx].status == "active") { SetStatus("Account is already active", 2); break; }
        g.accounts[idx].status = "active";
        saveAccounts(g.accounts);
        logAudit("Account Unfrozen", "Account " + to_string(accNo) + " unfrozen");
        SetStatus("Account unfrozen successfully", 1);
        SpeakText("Account " + to_string(accNo) + " has been unfrozen.");
        break;
    }

    case BTN_DO_UNLOCK: {
        string accStr = GetEditText(EDT_SEARCH);
        if (accStr.empty() || !isNumeric(accStr)) { SetStatus("Enter a valid numeric account number", 2); break; }
        int accNo = atoi(accStr.c_str());
        loadAccounts(g.accounts);
        int idx = findAccountIndex(accNo, g.accounts);
        if (idx < 0) { SetStatus("Account not found", 2); break; }
        if (g.accounts[idx].status == "active" && g.accounts[idx].pinAttempts == 0) {
            SetStatus("Account is already active and unlocked", 1);
            break;
        }
        g.accounts[idx].status = "active";
        g.accounts[idx].pinAttempts = 0;
        saveAccounts(g.accounts);
        logAudit("Account Unlocked", "Account " + to_string(accNo) + " unlocked, PIN reset");
        SetStatus("Account unlocked, PIN attempts reset", 1);
        SpeakText("Account " + to_string(accNo) + " has been unlocked.");
        break;
    }

    case BTN_DO_CONTACT_ADMIN: {
        string accStr  = GetEditText(EDT_ACCNO);
        string name    = GetEditText(EDT_NAME);
        string cnic    = GetEditText(EDT_CNIC);
        string reason  = GetEditText(EDT_REASON);

        if (accStr.empty() || !isNumeric(accStr)) { SetStatus("Please enter a valid numeric account number", 2); break; }
        if (!isValidName(name))   { SetStatus("Please enter your full name (letters and spaces only)", 2); break; }
        if (!isValidCNIC(cnic))   { SetStatus("Please enter a valid CNIC (XXXXX-XXXXXXX-X)", 2); break; }
        if (reason.length() < 5)  { SetStatus("Please describe the reason for reactivation (at least 5 characters)", 2); break; }

        int accNo = atoi(accStr.c_str());
        if (accNo <= 0) { SetStatus("Invalid account number", 2); break; }

        loadAccounts(g.accounts);
        int accIdx = findAccountIndex(accNo, g.accounts);
        if (accIdx < 0) { SetStatus("Account not found. Check your account number.", 2); break; }

        // Verify CNIC matches registered account details
        string cleanedInputCnic, cleanedRegCnic;
        for (char c : cnic) if (c != '-' && c != ' ') cleanedInputCnic += c;
        for (char c : g.accounts[accIdx].cnic) if (c != '-' && c != ' ') cleanedRegCnic += c;
        if (cleanedInputCnic != cleanedRegCnic) {
            SetStatus("CNIC does not match the registered account details!", 2);
            break;
        }

        // Verify Full Name matches registered account details exactly (case-insensitive & trimmed)
        auto trimStr = [](const string& s) {
            size_t start = s.find_first_not_of(" \t\r\n");
            if (start == string::npos) return string("");
            size_t end = s.find_last_not_of(" \t\r\n");
            return s.substr(start, end - start + 1);
        };

        string trimmedInput = trimStr(name);
        string trimmedReg   = trimStr(g.accounts[accIdx].name);

        bool nameMatch = (trimmedInput.length() == trimmedReg.length());
        if (nameMatch) {
            for (size_t i = 0; i < trimmedInput.length(); i++) {
                if (tolower((unsigned char)trimmedInput[i]) != tolower((unsigned char)trimmedReg[i])) {
                    nameMatch = false;
                    break;
                }
            }
        }

        if (!nameMatch) {
            SetStatus("Full Name does not match the registered account holder!", 2);
            break;
        }

        if (g.accounts[accIdx].status == "active") {
            SetStatus("This account is already active!", 1);
            break;
        }

        loadReactivationRequests(g.reactivationRequests);
        if (hasPendingReactivationRequest(accNo, g.reactivationRequests)) {
            SetStatus("A reactivation request for this account is already pending review.", 2);
            break;
        }

        ReactivationRequest req;
        req.requestId = (int)g.reactivationRequests.size() + 1;
        req.accountNo = accNo;
        req.name      = g.accounts[accIdx].name; // store official account name
        req.cnic      = cnic;
        req.reason    = reason;
        req.dateTime  = getCurrentDateTimeStr();
        req.status    = "pending";

        appendReactivationRequest(req);
        g.reactivationRequests.push_back(req);

        logAudit("Reactivation Request", "Account " + to_string(accNo) +
                 " submitted reactivation request #" + to_string(req.requestId));
        SetStatus("Request submitted! The admin will review it shortly.", 1);
        SpeakText("Your reactivation request has been submitted. Please wait for admin approval.");
        GoToScreen(SCR_ATM_LOGIN);
        break;
    }

    case BTN_DO_FORGOT_PIN: {
        string accStr     = GetEditText(EDT_ACCNO);
        string name       = GetEditText(EDT_NAME);
        string cnic       = GetEditText(EDT_CNIC);
        string newPin     = GetEditText(EDT_NEWPIN);
        string confirmPin = GetEditText(EDT_CONFIRMPIN);

        if (accStr.empty() || !isNumeric(accStr)) { SetStatus("Please enter a valid numeric account number", 2); break; }
        if (!isValidName(name))   { SetStatus("Please enter your full name (letters and spaces only)", 2); break; }
        if (!isValidCNIC(cnic))   { SetStatus("Please enter a valid CNIC (XXXXX-XXXXXXX-X)", 2); break; }
        if (!isValidPIN(newPin))  { SetStatus("New PIN must be exactly 4 numeric digits", 2); break; }
        if (newPin != confirmPin) { SetStatus("New PIN confirmation does not match!", 2); break; }

        int accNo = atoi(accStr.c_str());
        if (accNo <= 0) { SetStatus("Invalid account number", 2); break; }

        loadAccounts(g.accounts);
        int accIdx = findAccountIndex(accNo, g.accounts);
        if (accIdx < 0) { SetStatus("Account not found. Check your account number.", 2); break; }

        // Verify CNIC matches registered account details
        string cleanedInputCnic, cleanedRegCnic;
        for (char c : cnic) if (c != '-' && c != ' ') cleanedInputCnic += c;
        for (char c : g.accounts[accIdx].cnic) if (c != '-' && c != ' ') cleanedRegCnic += c;
        if (cleanedInputCnic != cleanedRegCnic) {
            SetStatus("CNIC does not match the registered account details!", 2);
            break;
        }

        // Verify Full Name matches registered account details (case-insensitive & trimmed)
        auto trimStr = [](const string& s) {
            size_t start = s.find_first_not_of(" \t\r\n");
            if (start == string::npos) return string("");
            size_t end = s.find_last_not_of(" \t\r\n");
            return s.substr(start, end - start + 1);
        };

        string trimmedInput = trimStr(name);
        string trimmedReg   = trimStr(g.accounts[accIdx].name);

        bool nameMatch = (trimmedInput.length() == trimmedReg.length());
        if (nameMatch) {
            for (size_t i = 0; i < trimmedInput.length(); i++) {
                if (tolower((unsigned char)trimmedInput[i]) != tolower((unsigned char)trimmedReg[i])) {
                    nameMatch = false;
                    break;
                }
            }
        }

        if (!nameMatch) {
            SetStatus("Full Name does not match the registered account holder!", 2);
            break;
        }

        // Reset PIN and reactivate account
        g.accounts[accIdx].pinHash = encodePIN(newPin);
        g.accounts[accIdx].pinAttempts = 0;
        g.accounts[accIdx].status = "active";
        saveAccounts(g.accounts);

        logAudit("Forgot PIN Reset", "Account " + to_string(accNo) + " PIN reset successfully");
        SetStatus("PIN reset successfully! You can now log in with your new PIN.", 1);
        SpeakText("PIN reset successfully. You can now log in with your new PIN.");

        g.pendingReactivationAccNo = accNo;
        GoToScreen(SCR_ATM_LOGIN);
        break;
    }

    case BTN_DO_DEPOSIT: {
        if (g.currentAccIdx < 0 || g.currentAccIdx >= (int)g.accounts.size()) {
            SetStatus("No account selected", 2);
            break;
        }
        string amtStr = GetEditText(EDT_AMT);
        if (amtStr.empty() || !isNumericDecimal(amtStr)) { SetStatus("Enter a valid deposit amount", 2); break; }

        double amt = atof(amtStr.c_str());
        if (!isValidAmount(amt)) { SetStatus("Deposit amount must be positive", 2); break; }
        if (!isMultipleOf500(amt)) { SetStatus("Deposit amount must be at least Rs. 500 and in multiples of 500 (e.g. 500, 1000, 1500)", 2); break; }
        if (amt > 1000000.0) { SetStatus("Maximum single deposit limit is Rs. 1,000,000", 2); break; }

        loadAccounts(g.accounts);
        loadTransactions(g.transactions);
        auto& acc = g.accounts[g.currentAccIdx];
        if (acc.status != "active") { SetStatus("Account is frozen or locked. Cannot deposit.", 2); break; }

        acc.balance += amt;
        saveAccounts(g.accounts);

        Transaction txn;
        txn.transactionID = generateTransactionID(g.transactions);
        txn.accountNo = acc.accountNo;
        txn.type = "deposit";
        txn.amount = amt;
        txn.dateTime = getCurrentDateTimeStr();
        txn.resultingBalance = acc.balance;
        txn.details = "Cash deposit";
        g.transactions.push_back(txn);
        saveTransactions(g.transactions);
        generateReceipt(txn, acc);

        logAudit("Deposit", "Account " + to_string(acc.accountNo) + " deposited Rs. " + to_string(amt));
        stringstream ss;
        ss << "Deposited Rs. " << fixed << setprecision(2) << amt << " successfully";
        SetStatus(ss.str(), 1);
        SpeakText("Deposit successful. " + to_string((int)amt) + " rupees deposited.");
        ShowReceipt(txn, acc);
        break;
    }

    case BTN_DO_WITHDRAW: {
        if (g.currentAccIdx < 0 || g.currentAccIdx >= (int)g.accounts.size()) {
            SetStatus("No account selected", 2);
            break;
        }
        string amtStr = GetEditText(EDT_AMT);
        if (amtStr.empty() || !isNumericDecimal(amtStr)) { SetStatus("Enter a valid withdrawal amount", 2); break; }
        double amt = atof(amtStr.c_str());
        if (!isValidAmount(amt)) { SetStatus("Withdrawal amount must be positive", 2); break; }
        if (!isMultipleOf500(amt)) { SetStatus("Withdrawal amount must be at least Rs. 500 and in multiples of 500 (e.g. 500, 1000, 1500)", 2); break; }
        if (amt > 100000.0) { SetStatus("Maximum single withdrawal limit is Rs. 100,000", 2); break; }

        loadAccounts(g.accounts);
        loadTransactions(g.transactions);
        auto& acc = g.accounts[g.currentAccIdx];
        if (acc.status != "active") { SetStatus("Account is not active", 2); break; }
        if (acc.balance < amt) { SetStatus("Insufficient account balance", 2); break; }

        resetDailyWithdrawals(g.accounts);
        if (acc.dailyWithdrawn + amt > DAILY_WITHDRAWAL_LIMIT) {
            SetStatus("Exceeds daily withdrawal limit (Rs. 50,000)", 2);
            break;
        }

        loadCashInventory(g.inventory);
        double totalAtmCash = getTotalCashInATM(g.inventory);
        if (totalAtmCash < amt) {
            stringstream ss;
            ss << "ATM cash short. Total available: Rs. " << fixed << setprecision(0) << totalAtmCash;
            SetStatus(ss.str(), 2);
            break;
        }

        if (!dispenseCash(g.inventory, amt)) {
            SetStatus("ATM cannot dispense exact note combination for this amount", 2);
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
        ShowReceipt(txn, acc);
        break;
    }

    case BTN_VERIFY_ACCOUNT: {
        string targetStr = GetEditText(EDT_TRANSFER_TARGET);
        if (targetStr.empty()) {
            SetStatus("Enter target account number", 2);
            break;
        }
        if (!isNumeric(targetStr)) {
            SetStatus("Enter a valid account number", 2);
            break;
        }

        int targetAcc = atoi(targetStr.c_str());
        loadAccounts(g.accounts);
        int receiverIdx = findAccountIndex(targetAcc, g.accounts);

        if (receiverIdx < 0) {
            SetStatus("Account not found", 2);
            break;
        }
        if (receiverIdx == g.currentAccIdx) {
            SetStatus("Cannot transfer to your own account", 2);
            break;
        }
        if (g.accounts[receiverIdx].status != "active") {
            SetStatus("Target account is not active", 2);
            break;
        }

        // Verification successful - save target info
        gTransferVerified = true;
        gVerifiedTargetAccNo = targetAcc;
        gVerifiedTargetName = g.accounts[receiverIdx].name;

        SetStatus("Account verified: " + gVerifiedTargetName, 1);
        SpeakText("Account verified. Recipient is " + gVerifiedTargetName);

        // Refresh screen to show phase 2
        ClearControls();
        CreateScreenControls();
        RefreshScreen();
        break;
    }

    case BTN_DO_TRANSFER: {
        if (g.currentAccIdx < 0 || g.currentAccIdx >= (int)g.accounts.size()) {
            SetStatus("No account selected", 2);
            break;
        }
        if (!gTransferVerified) {
            SetStatus("Please verify target account first", 2);
            break;
        }

        string amtStr = GetEditText(EDT_TRANSFER_AMT);
        if (amtStr.empty()) { SetStatus("Enter transfer amount", 2); break; }
        if (!isNumericDecimal(amtStr)) { SetStatus("Enter a valid transfer amount", 2); break; }

        int targetAcc = gVerifiedTargetAccNo;  // Use verified account
        double amt = atof(amtStr.c_str());
        if (!isValidAmount(amt)) { SetStatus("Transfer amount must be positive", 2); break; }
        if (!isMultipleOf500(amt)) { SetStatus("Transfer amount must be at least Rs. 500 and in multiples of 500 (e.g. 500, 1000, 1500)", 2); break; }
        if (amt > 1000000.0) { SetStatus("Maximum single transfer limit is Rs. 1,000,000", 2); break; }

        int senderIdx = g.currentAccIdx;
        loadAccounts(g.accounts);
        int receiverIdx = findAccountIndex(targetAcc, g.accounts);
        if (receiverIdx < 0) { SetStatus("Target account not found", 2); break; }
        if (senderIdx == receiverIdx) { SetStatus("Cannot transfer funds to your own account", 2); break; }
        if (g.accounts[receiverIdx].status != "active") { SetStatus("Target account is frozen or locked", 2); break; }
        if (g.accounts[senderIdx].balance < amt) { SetStatus("Insufficient account balance", 2); break; }

        if (amt >= OTP_THRESHOLD) {
            g.otpCode = generateOTP();
            g.pendingTransferTarget = targetAcc;
            g.pendingTransferAmount = amt;
            GoToScreen(SCR_OTP);
            return;
        }

        loadAccounts(g.accounts);
        loadTransactions(g.transactions);
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
        txn.details = "Transfer to Account #" + to_string(targetAcc);
        g.transactions.push_back(txn);
        saveTransactions(g.transactions);
        generateReceipt(txn, g.accounts[senderIdx]);

        logAudit("Transfer", "Account " + to_string(g.accounts[senderIdx].accountNo) +
                 " transferred Rs. " + to_string(amt) + " to " + to_string(targetAcc));
        stringstream ss;
        ss << "Transferred Rs. " << fixed << setprecision(2) << amt << " to " << gVerifiedTargetName;
        SetStatus(ss.str(), 1);
        SpeakText("Transfer successful. Money sent to " + gVerifiedTargetName);

        // Reset verification state after successful transfer
        gTransferVerified = false;
        gVerifiedTargetAccNo = 0;
        gVerifiedTargetName.clear();

        ShowReceipt(txn, g.accounts[senderIdx]);
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
        if (!isValidPIN(newPin)) { SetStatus("PIN must be exactly 4 numeric digits", 2); break; }
        if (newPin == curPin) { SetStatus("New PIN cannot be identical to current PIN", 2); break; }
        if (newPin != confPin) { SetStatus("New PIN confirmation does not match", 2); break; }

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

        int accNo = (accStr.empty() || !isNumeric(accStr)) ? -1 : atoi(accStr.c_str());
        double minAmt = (minS.empty() || !isNumericDecimal(minS)) ? -1 : atof(minS.c_str());
        double maxAmt = (maxS.empty() || !isNumericDecimal(maxS)) ? -1 : atof(maxS.c_str());

        g.transactions = filterTransactions(g.transactions, accNo, type, from, to, minAmt, maxAmt);
        stringstream ss;
        ss << "Found " << g.transactions.size() << " matching transactions";
        SetStatus(ss.str(), 1);
        if (gHwnd) InvalidateRect(gHwnd, NULL, TRUE);
        break;
    }

    case BTN_CLEAR_SEARCH: {
        SetEditText(EDT_SEARCH_TXN_ACC, "");
        SetEditText(EDT_SEARCH_TXN_TYPE, "");
        SetEditText(EDT_SEARCH_TXN_FROM, "");
        SetEditText(EDT_SEARCH_TXN_TO, "");
        SetEditText(EDT_SEARCH_TXN_MIN, "");
        SetEditText(EDT_SEARCH_TXN_MAX, "");
        loadTransactions(g.transactions);
        SetStatus("Transaction search filters cleared", 1);
        if (gHwnd) InvalidateRect(gHwnd, NULL, TRUE);
        break;
    }

    case BTN_DO_LOAN: {
        string accStr = GetEditText(EDT_LOAN_ACC);
        string amtStr = GetEditText(EDT_LOAN_AMT);
        string termStr = GetEditText(EDT_LOAN_TERM);
        if (accStr.empty()) {
            SetStatus("Enter an account number", 2);
            break;
        }
        if (!isNumeric(accStr)) { SetStatus("Enter a valid numeric account number", 2); break; }

        int accNo = atoi(accStr.c_str());

        loadAccounts(g.accounts);
        int idx = findAccountIndex(accNo, g.accounts);
        if (idx < 0) { SetStatus("Account not found", 2); break; }
        if (g.accounts[idx].status != "active") { SetStatus("Account is not active", 2); break; }

        loadLoans(g.loans);

        // Check if there is a pending loan application for this account
        int pendingIdx = -1;
        for (size_t i = 0; i < g.loans.size(); i++) {
            if (g.loans[i].accountNo == accNo && g.loans[i].status == "pending") {
                pendingIdx = (int)i;
                break;
            }
        }

        if (pendingIdx >= 0) {
            // Approve the pending loan application!
            auto& loan = g.loans[pendingIdx];
            loan.status = "active";
            
            g.accounts[idx].balance += loan.amount;
            saveAccounts(g.accounts);
            saveLoans(g.loans);

            loadTransactions(g.transactions);
            Transaction txn;
            txn.transactionID = generateTransactionID(g.transactions);
            txn.accountNo = accNo;
            txn.type = "deposit";
            txn.amount = loan.amount;
            txn.dateTime = getCurrentDateTimeStr();
            txn.resultingBalance = g.accounts[idx].balance;
            txn.details = "Loan #" + to_string(loan.loanId) + " approved";
            g.transactions.push_back(txn);
            saveTransactions(g.transactions);

            logAudit("Loan Approved", "Loan ID #" + to_string(loan.loanId) + " approved for account " + to_string(accNo));
            SetStatus("Loan ID #" + to_string(loan.loanId) + " for account " + to_string(accNo) + " approved successfully!", 1);
            SpeakText("Loan application approved successfully.");
            GoToScreen(SCR_ADMIN_LOANS);
            break;
        }

        // Check if account already has an active ongoing loan
        bool hasActive = false;
        for (const auto& l : g.loans) {
            if (l.accountNo == accNo && (l.status == "active" || l.status == "approved") && l.remainingAmount > 0.01) {
                hasActive = true;
                break;
            }
        }
        if (hasActive) {
            SetStatus("This account already has an active ongoing loan", 2);
            break;
        }

        if (amtStr.empty() || termStr.empty()) {
            SetStatus("Fill in amount and term fields to issue a new loan", 2);
            break;
        }
        if (!isNumericDecimal(amtStr)) { SetStatus("Enter a valid loan amount", 2); break; }
        if (!isNumeric(termStr)) { SetStatus("Enter a valid loan term in months", 2); break; }

        double amt = atof(amtStr.c_str());
        int term = atoi(termStr.c_str());

        if (term < 1 || term > 60) { SetStatus("Loan term must be between 1 and 60 months", 2); break; }

        if (!isValidLoanAmount(amt, g.accounts[idx].balance)) {
            SetStatus("Loan amount cannot exceed 3x current account balance", 2);
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

        loadTransactions(g.transactions);
        Transaction txn;
        txn.transactionID = generateTransactionID(g.transactions);
        txn.accountNo = accNo;
        txn.type = "deposit";
        txn.amount = amt;
        txn.dateTime = getCurrentDateTimeStr();
        txn.resultingBalance = g.accounts[idx].balance;
        txn.details = "New Loan #" + to_string(loan.loanId) + " issued";
        g.transactions.push_back(txn);
        saveTransactions(g.transactions);

        logAudit("Loan Created", "Loan ID #" + to_string(loan.loanId) + " created for account " + to_string(accNo));
        SetStatus("Loan issued and account credited successfully", 1);
        SpeakText("Loan created successfully.");
        GoToScreen(SCR_ADMIN_LOANS);
        break;
    }

    case BTN_DO_REPAY_INSTALLMENT:
    case BTN_DO_REPAY_CUSTOM:
    case BTN_DO_REPAY_FULL: {
        if (g.currentAccIdx < 0 || g.currentAccIdx >= (int)g.accounts.size()) {
            SetStatus("No account logged in", 2);
            break;
        }
        int accNo = g.accounts[g.currentAccIdx].accountNo;
        loadLoans(g.loans);
        int loanIdx = -1;
        for (size_t i = 0; i < g.loans.size(); i++) {
            if (g.loans[i].accountNo == accNo && (g.loans[i].status == "active" || g.loans[i].status == "approved") && g.loans[i].remainingAmount > 0.01) {
                loanIdx = (int)i;
                break;
            }
        }
        if (loanIdx < 0) {
            SetStatus("No active loan found to repay", 2);
            break;
        }

        double payAmt = 0;
        if (id == BTN_DO_REPAY_INSTALLMENT) {
            payAmt = min(g.loans[loanIdx].monthlyPayment, g.loans[loanIdx].remainingAmount);
        } else if (id == BTN_DO_REPAY_FULL) {
            payAmt = g.loans[loanIdx].remainingAmount;
        } else {
            string amtStr = GetEditText(EDT_REPAY_AMT);
            if (amtStr.empty() || !isNumericDecimal(amtStr)) {
                SetStatus("Please enter a valid numeric amount to pay", 2);
                break;
            }
            payAmt = atof(amtStr.c_str());
        }

        if (payAmt <= 0) {
            SetStatus("Repayment amount must be greater than zero", 2);
            break;
        }

        if (payAmt > g.loans[loanIdx].remainingAmount + 0.01) {
            payAmt = g.loans[loanIdx].remainingAmount;
        }

        if (g.accounts[g.currentAccIdx].balance < payAmt) {
            SetStatus("Insufficient account balance to make this loan payment", 2);
            break;
        }

        // Deduct payment amount from customer account balance
        g.accounts[g.currentAccIdx].balance -= payAmt;
        saveAccounts(g.accounts);

        // Update Loan record
        g.loans[loanIdx].remainingAmount -= payAmt;
        g.loans[loanIdx].monthsPaid += 1;
        if (g.loans[loanIdx].remainingAmount <= 0.01) {
            g.loans[loanIdx].remainingAmount = 0.0;
            g.loans[loanIdx].status = "completed";
        }
        updateLoanInFile(g.loans[loanIdx]);
        saveLoans(g.loans);

        // Record Transaction & Receipt
        Transaction txn;
        txn.transactionID = generateTransactionID(g.transactions);
        txn.accountNo = accNo;
        txn.type = "loan_payment";
        txn.amount = payAmt;
        txn.dateTime = getCurrentDateTimeStr();
        txn.resultingBalance = g.accounts[g.currentAccIdx].balance;
        txn.details = "Repayment for Loan #" + to_string(g.loans[loanIdx].loanId);
        g.transactions.push_back(txn);
        saveTransactions(g.transactions);
        generateReceipt(txn, g.accounts[g.currentAccIdx]);

        logAudit("Loan Repayment", "Account " + to_string(accNo) + " paid Rs. " + to_string(payAmt) + " towards Loan #" + to_string(g.loans[loanIdx].loanId));

        stringstream ssMsg;
        ssMsg << "Loan payment of Rs. " << fixed << setprecision(2) << payAmt << " processed successfully!";
        SetStatus(ssMsg.str(), 1);
        SpeakText("Loan payment processed successfully.");

        GoToScreen(SCR_ATM_REPAY_LOAN);
        break;
    }

    case BTN_DO_APPLY_LOAN: {
        if (g.currentAccIdx < 0 || g.currentAccIdx >= (int)g.accounts.size()) {
            SetStatus("No account logged in", 2);
            break;
        }
        int accNo = g.accounts[g.currentAccIdx].accountNo;
        string amtStr = GetEditText(EDT_APPLY_LOAN_AMT);
        string termStr = GetEditText(EDT_APPLY_LOAN_TERM);

        if (amtStr.empty() || termStr.empty()) {
            SetStatus("Please fill in both loan amount and term in months", 2);
            break;
        }
        if (!isNumericDecimal(amtStr)) { SetStatus("Enter a valid numeric loan amount", 2); break; }
        if (!isNumeric(termStr)) { SetStatus("Enter a valid loan term in months", 2); break; }

        double amt = atof(amtStr.c_str());
        int term = atoi(termStr.c_str());

        if (amt <= 0) { SetStatus("Loan amount must be greater than zero", 2); break; }
        if (!isMultipleOf500(amt)) { SetStatus("Loan amount must be at least Rs. 500 and in multiples of 500 (e.g. 500, 1000, 1500)", 2); break; }
        if (term < 1 || term > 60) { SetStatus("Loan term must be between 1 and 60 months", 2); break; }

        loadLoans(g.loans);
        if (hasActiveLoan(accNo, g.loans)) {
            SetStatus("This account already has an active or pending loan application", 2);
            break;
        }

        if (!isValidLoanAmount(amt, g.accounts[g.currentAccIdx].balance)) {
            stringstream ssErr;
            ssErr << "Requested loan exceeds limit! Maximum eligible loan is Rs. "
                  << fixed << setprecision(2) << (g.accounts[g.currentAccIdx].balance * 3.0);
            SetStatus(ssErr.str(), 2);
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
        loan.status = "pending";
        loan.applicationDate = getCurrentDateTimeStr();
        loan.remainingAmount = amt;
        loan.monthsPaid = 0;

        g.loans.push_back(loan);
        saveLoans(g.loans);

        logAudit("Loan Application", "Account " + to_string(accNo) + " requested loan #" +
                 to_string(loan.loanId) + " of Rs. " + to_string(amt));
        stringstream ss;
        ss << "Loan application #" << loan.loanId << " submitted! Awaiting admin review.";
        SetStatus(ss.str(), 1);
        SpeakText("Loan application submitted for admin approval.");

        GoToScreen(SCR_ATM_APPLY_LOAN);
        break;
    }

    case BTN_BACK:
        if (gShowReceipt) {
            CloseReceipt();
        } else if (g.screen == SCR_CONTACT_ADMIN || g.screen == SCR_FORGOT_PIN) {
            GoToScreen(SCR_ATM_LOGIN);
        } else if (g.screen == SCR_ADMIN_LOGIN || g.screen == SCR_ATM_LOGIN) {
            g.currentAccIdx = -1;
            g.isAdmin = false;
            GoToScreen(SCR_LOGIN);
        } else if (g.isAdmin) {
            GoToScreen(SCR_ADMIN_DASH);
        } else if (g.currentAccIdx >= 0) {
            GoToScreen(SCR_ATM_MENU);
        } else {
            g.isAdmin = false;
            GoToScreen(SCR_LOGIN);
        }
        break;

    case BTN_CLOSE_RECEIPT:
        CloseReceipt();
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

        loadAccounts(g.accounts);
        loadTransactions(g.transactions);

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
        SpeakText("Transfer successful.");
        GoToScreen(SCR_ATM_TRANSFER);
        ShowReceipt(txn, g.accounts[senderIdx]);
        break;
    }

    case BTN_CLEAR_CREATE:
        ClearControls();
        CreateScreenControls();
        break;

    // ATM Menu card buttons
    case BTN_ATM_BALANCE:
        GoToScreen(SCR_ATM_BALANCE);
        break;
    case BTN_ATM_DEPOSIT:
        GoToScreen(SCR_ATM_DEPOSIT);
        break;
    case BTN_ATM_WITHDRAW:
        GoToScreen(SCR_ATM_WITHDRAW);
        break;
    case BTN_ATM_TRANSFER:
        GoToScreen(SCR_ATM_TRANSFER);
        break;
    case BTN_ATM_MINISTATE:
        GoToScreen(SCR_ATM_MINISTATE);
        break;
    case BTN_ATM_CHANGEPIN:
        GoToScreen(SCR_ATM_CHANGEPIN);
        break;
    case BTN_ATM_PAY_LOAN:
        GoToScreen(SCR_ATM_REPAY_LOAN);
        break;
    case BTN_ATM_APPLY_LOAN:
        GoToScreen(SCR_ATM_APPLY_LOAN);
        break;
    }
}

static void GoToScreen(Screen scr) {
    // Close any open receipt overlay when navigating
    gShowReceipt = false;

    // Reset transfer verification when leaving or entering transfer screen
    if (g.screen == SCR_ATM_TRANSFER || scr == SCR_ATM_TRANSFER) {
        gTransferVerified = false;
        gVerifiedTargetAccNo = 0;
        gVerifiedTargetName.clear();
    }

    if (scr == SCR_LOGIN) {
        g.isAdmin = false;
    }
    g.screen = scr;
    g.statusMsg.clear();
    if (scr == SCR_ADMIN_TXNS || scr == SCR_ADMIN_SEARCH_TXN || scr == SCR_ATM_MINISTATE) {
        loadTransactions(g.transactions);
    }
    // Keep reactivation request list fresh for the notification badge
    // and reload fully when admin opens the requests screen
    if (g.isAdmin && scr != SCR_LOGIN && scr != SCR_ATM_LOGIN) {
        loadReactivationRequests(g.reactivationRequests);
    }
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

    int id = GetWindowLong(di->hwndItem, GWL_ID);

    // Pre-fill button window background with parent container background (Primary for header buttons)
    // so the pixels outside RoundRect curves match the surrounding background perfectly with zero white corners!
    bool isHeaderBtn = (id == BTN_NOTIF_REQUESTS || id == BTN_DO_BACKUP || id == BTN_LOGOUT);
    COLORREF parentBgClr = isHeaderBtn ? Primary : Bg;
    HBRUSH parentBr = CreateSolidBrush(parentBgClr);
    FillRect(hdc, &rc, parentBr);
    DeleteObject(parentBr);

    // Custom notification bell button rendering
    if (id == BTN_NOTIF_REQUESTS) {
        COLORREF bgClr = RGB(30, 41, 59);
        if (pressed) {
            bgClr = RGB(15, 23, 42);
        } else if (hovered) {
            bgClr = RGB(51, 65, 85);
        }

        HBRUSH br = CreateSolidBrush(bgClr);
        HPEN pen = CreatePen(PS_SOLID, 1, hovered ? RGB(99, 102, 241) : RGB(51, 65, 85));
        HPEN oldPen = (HPEN)SelectObject(hdc, pen);
        HBRUSH oldBr = (HBRUSH)SelectObject(hdc, br);
        RoundRect(hdc, rc.left, rc.top, rc.right, rc.bottom, 12, 12);
        SelectObject(hdc, oldPen);
        SelectObject(hdc, oldBr);
        DeleteObject(br);
        DeleteObject(pen);

        int pendingCount = 0;
        for (auto& req : g.reactivationRequests)
            if (req.status == "pending") pendingCount++;

        COLORREF bellClr = (pendingCount > 0) ? RGB(251, 191, 36) : (hovered ? RGB(255, 255, 255) : RGB(148, 163, 184));
        DrawBellIcon(hdc, rc.left + 11, rc.top + 8, bellClr);

        if (pendingCount > 0) {
            int rx = rc.right - 15;
            int ry = rc.top - 2;
            HBRUSH badgeBr = CreateSolidBrush(RGB(239, 68, 68));
            HPEN badgePen = CreatePen(PS_SOLID, 1, RGB(255, 255, 255));
            HPEN oldP2 = (HPEN)SelectObject(hdc, badgePen);
            HBRUSH oldB2 = (HBRUSH)SelectObject(hdc, badgeBr);
            Ellipse(hdc, rx, ry, rx + 16, ry + 16);
            SelectObject(hdc, oldP2);
            SelectObject(hdc, oldB2);
            DeleteObject(badgeBr);
            DeleteObject(badgePen);

            string countStr = pendingCount > 9 ? "9+" : to_string(pendingCount);
            SetBkMode(hdc, TRANSPARENT);
            SetTextColor(hdc, RGB(255, 255, 255));
            HFONT oldFont = (HFONT)SelectObject(hdc, hFontSmall);
            RECT bRc = {rx, ry, rx + 16, ry + 16};
            DrawText(hdc, countStr.c_str(), -1, &bRc, DT_CENTER|DT_VCENTER|DT_SINGLELINE);
            SelectObject(hdc, oldFont);
        }
        return;
    }

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
    RoundRect(hdc, rc.left, rc.top, rc.right, rc.bottom, isHeaderBtn ? 12 : 20, isHeaderBtn ? 12 : 20);
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

    // Skip white dotted focus box for header buttons to keep clean modern look
    if (focused && !isHeaderBtn) {
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

        // Double-buffering offscreen memory DC for 100% flicker-free rendering
        HDC memDC = CreateCompatibleDC(hdc);
        HBITMAP memBM = CreateCompatibleBitmap(hdc, rc.right, rc.bottom);
        HBITMAP oldBM = (HBITMAP)SelectObject(memDC, memBM);

        PaintScreen(memDC, rc);

        // Atomic Blit of offscreen buffer to screen
        BitBlt(hdc, ps.rcPaint.left, ps.rcPaint.top,
               ps.rcPaint.right - ps.rcPaint.left,
               ps.rcPaint.bottom - ps.rcPaint.top,
               memDC, ps.rcPaint.left, ps.rcPaint.top, SRCCOPY);

        SelectObject(memDC, oldBM);
        DeleteObject(memBM);
        DeleteDC(memDC);

        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_LBUTTONDOWN: {
        int mx = LOWORD(lp);
        int my = HIWORD(lp);

        // Sidebar hit test
        int hit = SidebarHitTest(lp, g);
        if (hit >= 0) {
            Screen target = sidebarItems[hit].screen;
            GoToScreen(target);
            logAudit("Navigation", string("Sidebar -> ") + sidebarItems[hit].label);
            return 0;
        }

        // ATM Menu card hit test
        if (g.screen == SCR_ATM_MENU) {
            RECT rc;
            GetClientRect(hwnd, &rc);
            int menuSx = SIDEBAR_W + 30;
            int menuSy = HEADER_H + 30;
            int cw = rc.right - SIDEBAR_W - 60;
            int cardW = (cw - 30) / 3;
            int cardH = 110;

            Screen cardScreens[] = {
                SCR_ATM_BALANCE, SCR_ATM_DEPOSIT, SCR_ATM_WITHDRAW,
                SCR_ATM_TRANSFER, SCR_ATM_MINISTATE, SCR_ATM_CHANGEPIN,
                SCR_ATM_REPAY_LOAN, SCR_ATM_APPLY_LOAN
            };

            for (int i = 0; i < 8; i++) {
                int col = i % 3;
                int row = i / 3;
                int cx = menuSx + col * (cardW + 15);
                int cy = menuSy + 80 + row * (cardH + 20);

                if (mx >= cx && mx < cx + cardW && my >= cy && my < cy + cardH) {
                    GoToScreen(cardScreens[i]);
                    return 0;
                }
            }
        }

        return 0;
    }

    case WM_MOUSEMOVE: {
        int mx = LOWORD(lp);
        int my = HIWORD(lp);
        bool hasSidebar = (g.screen != SCR_LOGIN && g.screen != SCR_ADMIN_LOGIN && g.screen != SCR_ATM_LOGIN && g.screen != SCR_CONTACT_ADMIN);

        // Check if over ATM menu cards
        bool overATMCard = false;
        if (g.screen == SCR_ATM_MENU) {
            RECT rc;
            GetClientRect(hwnd, &rc);
            int menuSx = SIDEBAR_W + 30;
            int menuSy = HEADER_H + 30;
            int cw = rc.right - SIDEBAR_W - 60;
            int cardW = (cw - 30) / 3;
            int cardH = 110;

            for (int i = 0; i < 8; i++) {
                int col = i % 3;
                int row = i / 3;
                int cx = menuSx + col * (cardW + 15);
                int cy = menuSy + 80 + row * (cardH + 20);

                if (mx >= cx && mx < cx + cardW && my >= cy && my < cy + cardH) {
                    overATMCard = true;
                    break;
                }
            }
        }

        if (overATMCard) {
            SetCursor(LoadCursor(NULL, IDC_HAND));
        } else if (hasSidebar && mx < SIDEBAR_W && my > HEADER_H) {
            int hit = SidebarHitTest(lp, g);
            if (hit != gHoverSidebarIdx) {
                gHoverSidebarIdx = hit;
                RECT sbRc = {0, HEADER_H, SIDEBAR_W, 2000};
                InvalidateRect(hwnd, &sbRc, FALSE); // Invalidate sidebar only for butter-smooth hover
                if (hit >= 0 && hit < (int)(sizeof(sidebarItems)/sizeof(sidebarItems[0]))) {
                    SpeakText(sidebarItems[hit].label);
                }
            }
            SetCursor(LoadCursor(NULL, IDC_HAND));
        } else {
            if (gHoverSidebarIdx != -1) {
                gHoverSidebarIdx = -1;
                RECT sbRc = {0, HEADER_H, SIDEBAR_W, 2000};
                InvalidateRect(hwnd, &sbRc, FALSE);
            }
            SetCursor(LoadCursor(NULL, IDC_ARROW));
        }
        return 0;
    }

    case WM_SETCURSOR: {
        POINT pt;
        GetCursorPos(&pt);
        ScreenToClient(hwnd, &pt);

        // Check ATM menu cards
        if (g.screen == SCR_ATM_MENU) {
            RECT rc;
            GetClientRect(hwnd, &rc);
            int menuSx = SIDEBAR_W + 30;
            int menuSy = HEADER_H + 30;
            int cw = rc.right - SIDEBAR_W - 60;
            int cardW = (cw - 30) / 3;
            int cardH = 110;

            for (int i = 0; i < 8; i++) {
                int col = i % 3;
                int row = i / 3;
                int cx = menuSx + col * (cardW + 15);
                int cy = menuSy + 80 + row * (cardH + 20);

                if (pt.x >= cx && pt.x < cx + cardW && pt.y >= cy && pt.y < cy + cardH) {
                    SetCursor(LoadCursor(NULL, IDC_HAND));
                    return TRUE;
                }
            }
        }

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
