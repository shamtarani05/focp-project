#ifndef GUI_H
#define GUI_H

#include <windows.h>
#include <string>
#include <vector>
#include "core/types.h"

using namespace std;

enum Screen {
    SCR_LOGIN, SCR_ADMIN_LOGIN, SCR_ADMIN_DASH, SCR_ADMIN_CREATE,
    SCR_ADMIN_VIEW, SCR_ADMIN_SEARCH, SCR_ADMIN_FREEZE, SCR_ADMIN_UNFREEZE,
    SCR_ADMIN_UNLOCK, SCR_ADMIN_TXNS, SCR_ADMIN_SEARCH_TXN, SCR_ADMIN_AUDIT,
    SCR_ADMIN_LOANS, SCR_ADMIN_CASH, SCR_ADMIN_DAILY,
    SCR_ATM_LOGIN, SCR_ATM_MENU, SCR_ATM_BALANCE, SCR_ATM_DEPOSIT,
    SCR_ATM_WITHDRAW, SCR_ATM_TRANSFER, SCR_ATM_MINISTATE,
    SCR_ATM_CHANGEPIN, SCR_ATM_INFO
};

struct AppState {
    Screen screen;
    vector<Account> accounts;
    vector<Transaction> transactions;
    vector<Loan> loans;
    vector<CashNote> inventory;
    int currentAccIdx;
    bool isAdmin;
    HWND hMainWnd;
    vector<HWND> childControls;
    string statusMsg;
    int statusType;
    string otpCode;
};

#endif
