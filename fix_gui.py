#!/usr/bin/env python3
"""Fix compilation errors in gui.cpp"""
import re

path = r"C:\Users\Sham\Desktop\fiver\focp project\src\gui.cpp"
with open(path, "r", encoding="utf-8") as f:
    code = f.read()

# 1. Move struct definitions before vector declarations
# Remove the forward-declared structs and vectors section, replace with proper ordering
old_section = """// --- Control Management ---
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
};"""

new_section = """// --- Control Management ---
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
static HWND gHwnd = NULL;"""

code = code.replace(old_section, new_section)

# 2. Fix MakeEdit calls missing pwd=false - add explicit false
# Pattern: MakeEdit(EDT_XXX, ..., edH);  where last arg before ) is edH or a number (int)
# These need , false) added
# Actually simpler: all calls with exactly 5 args need a 6th false
# But some calls with 6 args (including true) are fine
# The MakeEdit signature is: MakeEdit(int id, int x, int y, int w, int h, bool pwd = false)
# So 5 args is fine with default... but the issue was the overload conflict
# Since we removed the gui.h declarations, the default param should work now.
# Actually wait - we need to double check. The calls are:
# MakeEdit(EDT_NAME, sx + 170, formY + 2, edW, edH);  -- 5 args, OK with default
# MakeEdit(EDT_PIN, sx + 170, formY + gap*4 + 2, edW, edH, true); -- 6 args, OK
# Since gui.h no longer declares the conflicting overload, this should be fine.

# 3. Fix loadAccounts() calls - need argument
code = code.replace(
    'g.accounts = loadAccounts() ? g.accounts : vector<Account>();',
    'loadAccounts(g.accounts);'
)
code = code.replace(
    'g.transactions = loadTransactions() ? g.transactions : vector<Transaction>();',
    'loadTransactions(g.transactions);'
)

# 4. Fix loadCashInventory assignment
code = code.replace(
    'g.inventory = loadCashInventory(g.inventory);',
    'loadCashInventory(g.inventory);'
)

# 5. Fix PaintAdminDash duplicate load calls
old_dash = """    g.accounts = loadAccounts() ? g.accounts : vector<Account>();
    loadAccounts(g.accounts);
    g.transactions = loadTransactions() ? g.transactions : vector<Transaction>();
    loadTransactions(g.transactions);"""
new_dash = """    loadAccounts(g.accounts);
    loadTransactions(g.transactions);"""
code = code.replace(old_dash, new_dash)

with open(path, "w", encoding="utf-8") as f:
    f.write(code)

print("Fixed gui.cpp")
