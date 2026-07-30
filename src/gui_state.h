#ifndef BANKING_GUI_MAIN_H
#define BANKING_GUI_MAIN_H

#include <windows.h>
#include <commctrl.h>
#include <string>
#include <vector>
#include "core/types.h"

using namespace std;

enum Screen {
    SCR_LOGIN,
    SCR_ADMIN_LOGIN,
    SCR_ADMIN_DASH,
    SCR_ADMIN_CREATE,
    SCR_ADMIN_VIEW,
    SCR_ADMIN_SEARCH,
    SCR_ADMIN_FREEZE,
    SCR_ADMIN_UNFREEZE,
    SCR_ADMIN_UNLOCK,
    SCR_ADMIN_TXNS,
    SCR_ADMIN_SEARCH_TXN,
    SCR_ADMIN_AUDIT,
    SCR_ADMIN_LOANS,
    SCR_ADMIN_CASH,
    SCR_ADMIN_DAILY,
    SCR_ATM_LOGIN,
    SCR_ATM_MENU,
    SCR_ATM_BALANCE,
    SCR_ATM_DEPOSIT,
    SCR_ATM_WITHDRAW,
    SCR_ATM_TRANSFER,
    SCR_ATM_MINISTATE,
    SCR_ATM_CHANGEPIN,
    SCR_ATM_INFO
};

struct AppState {
    Screen screen;
    Screen prevScreen;

    vector<Account> accounts;
    vector<Transaction> transactions;
    vector<Loan> loans;
    vector<CashNote> inventory;

    int currentAccIdx;
    bool isAdmin;

    HWND hMainWnd;
    vector<HWND> childControls;

    int sidebarHover;
    int msgTimer;

    string statusMsg;
    int statusType;
};

void SwitchScreen(AppState& state, Screen newScreen);
void AddControl(AppState& state, HWND hwnd);
void ClearControls(AppState& state);
void SetStatus(AppState& state, const string& msg, int type);

#endif
