# National Bank Banking System - Complete Demo Guide
## FOCP Project Documentation

---

# TABLE OF CONTENTS

1. [Project Overview](#1-project-overview)
2. [File Structure & Purpose](#2-file-structure--purpose)
3. [FOCP Concepts Used](#3-focp-concepts-used)
4. [Feature-by-Feature Breakdown](#4-feature-by-feature-breakdown)
5. [Windows GUI Functions](#5-windows-gui-functions)
6. [Data Flow Examples](#6-data-flow-examples)
7. [Demo Script](#7-demo-script)

---

# 1. PROJECT OVERVIEW

This is a **Desktop Banking Application** built using:
- **Language**: C++ (Standard C++11)
- **GUI Library**: Windows GDI (Graphics Device Interface) - Built into Windows
- **Compiler**: MinGW G++ (GCC for Windows)

The application has **two modes**:
1. **Admin Mode** - Bank administrator manages accounts, views transactions, approves loans
2. **ATM Mode** - Customers perform banking operations (deposit, withdraw, transfer)

---

# 2. FILE STRUCTURE & PURPOSE

```
focp project/
│
├── src/                          # Source Code Directory
│   │
│   ├── main.cpp                  # APPLICATION ENTRY POINT
│   │                             # - Contains WinMain() function
│   │                             # - Includes gui.cpp to compile everything
│   │                             # - Starts the Windows message loop
│   │
│   ├── gui.cpp                   # MAIN GUI FILE (3,700+ lines)
│   │                             # - All screen painting (25+ screens)
│   │                             # - Button click handlers
│   │                             # - Window creation & management
│   │                             # - Event handling (mouse, keyboard)
│   │
│   ├── gui.h                     # GUI Header
│   │                             # - Screen enum definitions
│   │                             # - Function declarations
│   │
│   ├── gui_state.h               # Application State Structure
│   │                             # - AppState struct definition
│   │                             # - Screen enumeration
│   │
│   ├── banking.cpp               # BUSINESS LOGIC (132 lines)
│   │                             # - PIN encoding/decoding
│   │                             # - OTP generation
│   │                             # - Cash dispensing algorithm
│   │                             # - Daily withdrawal reset
│   │
│   ├── banking.h                 # Banking Function Declarations
│   │
│   ├── theme.h                   # VISUAL THEME
│   │                             # - Color definitions (RGB values)
│   │                             # - Layout constants (sidebar width, header height)
│   │
│   └── core/                     # Core Modules Directory
│       │
│       ├── types.h               # DATA STRUCTURES (225 lines)
│       │                         # - Account struct
│       │                         # - Transaction struct
│       │                         # - Loan struct
│       │                         # - CashNote struct
│       │                         # - Constants (limits, file paths)
│       │
│       ├── fileio.cpp            # FILE OPERATIONS (620 lines)
│       │                         # - Load/Save accounts
│       │                         # - Load/Save transactions
│       │                         # - Load/Save loans
│       │                         # - Audit logging
│       │                         # - Receipt generation
│       │                         # - Backup functionality
│       │
│       ├── fileio.h              # File I/O Function Declarations
│       │
│       ├── validation.cpp        # INPUT VALIDATION (191 lines)
│       │                         # - PIN validation
│       │                         # - Amount validation
│       │                         # - CNIC validation
│       │                         # - Account search functions
│       │
│       └── validation.h          # Validation Function Declarations
│
├── data/                         # DATA STORAGE (Text Files)
│   ├── accounts.txt              # All bank accounts
│   ├── transactions.txt          # Transaction history
│   ├── loans.txt                 # Loan records
│   ├── audit.txt                 # Admin action log
│   ├── cash_inventory.txt        # ATM cash denominations
│   └── reactivation_requests.txt # Frozen account requests
│
├── receipts/                     # Generated transaction receipts
│
├── build/                        # Compiled executable & backups
│
└── run.bat                       # Build & run script
```

---

# 3. FOCP CONCEPTS USED

## 3.1 VARIABLES & DATA TYPES

**File: `src/core/types.h`**

```
CONCEPT: Different data types store different kinds of information

int accountNo;        → Whole numbers (account number: 1001, 1002)
double balance;       → Decimal numbers (money: 25000.50)
string name;          → Text (customer name: "Ahmed Khan")
bool isActive;        → True/False (account active or not)
```

**Where Used:**
- `Account` struct has `int accountNo`, `double balance`, `string name`
- `Transaction` struct has `double amount`, `string type`
- `Loan` struct has `double interestRate`, `int termMonths`

---

## 3.2 ARRAYS & VECTORS

**File: `src/core/types.h` and `src/gui.cpp`**

```
CONCEPT: Store multiple items of same type

FIXED ARRAY (size known at compile time):
const int DENOMINATIONS[] = {5000, 1000, 500};   → ATM note types

DYNAMIC ARRAY/VECTOR (size changes at runtime):
vector<Account> accounts;        → List of all bank accounts
vector<Transaction> transactions; → List of all transactions
```

**Where Used:**
- `vector<Account>` stores all accounts loaded from file
- `vector<Transaction>` stores transaction history
- `vector<Loan>` stores all loan records
- `vector<HWND>` stores GUI control handles
- `DENOMINATIONS[]` array for ATM cash notes

---

## 3.3 LOOPS

**File: `src/core/validation.cpp`, `src/core/fileio.cpp`**

```
CONCEPT: Repeat code multiple times

FOR LOOP - When you know how many times:
for (int i = 0; i < accounts.size(); i++) {
    // Process each account
}

RANGE-BASED FOR - Modern C++ style:
for (auto& acc : accounts) {
    // Process each account
}

WHILE LOOP - Until condition is false:
while (getline(file, line)) {
    // Read each line from file
}
```

**Where Used:**
- `validation.cpp` line 52-55: Loop through account types to validate
- `fileio.cpp` line 54-78: While loop to read accounts from file
- `gui.cpp` line 930-948: For loop to draw ATM menu cards
- `banking.cpp` line 62-75: Loop through denominations for cash dispensing

---

## 3.4 CONDITIONAL STATEMENTS

**File: `src/core/validation.cpp`, `src/gui.cpp`**

```
CONCEPT: Make decisions in code

IF-ELSE:
if (pin.length() != 4) {
    return false;  // PIN must be 4 digits
} else {
    return true;
}

SWITCH-CASE:
switch (g.screen) {
    case SCR_LOGIN: PaintLoginScreen(); break;
    case SCR_ATM_MENU: PaintATMMenu(); break;
    case SCR_WITHDRAW: PaintWithdraw(); break;
}
```

**Where Used:**
- `validation.cpp` line 7-12: If statement to check PIN length
- `gui.cpp` line 1825-1848: Switch statement for screen painting
- `gui.cpp` line 2580-2639: Multiple if statements for withdrawal validation

---

## 3.5 FUNCTIONS

**File: All .cpp files**

```
CONCEPT: Reusable blocks of code

FUNCTION WITH RETURN VALUE:
bool isValidPIN(const string& pin) {
    if (pin.length() != 4) return false;
    return true;
}

FUNCTION WITHOUT RETURN (void):
void saveAccounts(const vector<Account>& accounts) {
    // Save to file
}

FUNCTION WITH DEFAULT PARAMETER:
void SetStatus(const string& msg, int type = 0) {
    // type defaults to 0 if not provided
}
```

**Where Used:**
- `validation.cpp`: 15+ validation functions
- `fileio.cpp`: Load/Save functions for each data type
- `banking.cpp`: PIN encoding, OTP generation, cash dispensing
- `gui.cpp`: 50+ functions for painting screens, handling clicks

---

## 3.6 STRUCTURES (struct)

**File: `src/core/types.h`**

```
CONCEPT: Group related data together

struct Account {
    int accountNo;      // Account number
    string name;        // Customer name
    double balance;     // Current balance
    string pinHash;     // Encoded PIN
    string status;      // active/frozen/locked
};
```

**All Structures:**
1. `Account` - Bank account data (line 10-55)
2. `Transaction` - Transaction record (line 57-89)
3. `Loan` - Loan information (line 91-120)
4. `CashNote` - ATM cash denomination (line 122-130)
5. `AuditEntry` - Admin action log (line 132-139)
6. `ReactivationRequest` - Account unlock request (line 141-156)

---

## 3.7 CLASSES & OOP

**File: `src/core/types.h`**

```
CONCEPT: Structures with private data and public methods

class TransactionManager {
private:
    vector<Transaction> transactions;  // PRIVATE - cannot access directly

public:
    void setTransactions(...);         // PUBLIC - can call from outside
    vector<Transaction> filter(...);   // PUBLIC - query method
};
```

**OOP Concepts Used:**
- **Encapsulation**: Private data members in TransactionManager class
- **Constructors**: Default and parameterized constructors in all structs
- **Member Functions**: `isActive()`, `canWithdraw()`, `deposit()` in Account struct

---

## 3.8 FILE I/O

**File: `src/core/fileio.cpp`**

```
CONCEPT: Read from and write to files

READING FROM FILE:
ifstream file("data/accounts.txt");    // Open for reading
string line;
while (getline(file, line)) {          // Read line by line
    // Process line
}
file.close();

WRITING TO FILE:
ofstream file("data/accounts.txt");    // Open for writing
file << accountNo << "|" << name << "|" << balance << "\n";
file.close();

APPENDING TO FILE:
ofstream file("data/audit.txt", ios::app);  // Append mode
file << timestamp << "|" << action << "\n";
file.close();
```

**Where Used:**
- `loadAccounts()` - Read accounts from accounts.txt
- `saveAccounts()` - Write accounts to accounts.txt
- `loadTransactions()` / `saveTransactions()` - Transaction history
- `logAudit()` - Append to audit log
- `generateReceipt()` - Create receipt file

---

## 3.9 STRING MANIPULATION

**File: `src/core/fileio.cpp`, `src/banking.cpp`**

```
CONCEPT: Work with text data

PARSING (splitting string):
string line = "1001|Ahmed|50000.00";
stringstream ss(line);
string field;
getline(ss, field, '|');  // Gets "1001"
getline(ss, field, '|');  // Gets "Ahmed"

CONCATENATION (joining strings):
string result = "Account " + to_string(accNo) + " created";

SUBSTRING:
string date = dateTime.substr(0, 10);  // First 10 characters
```

**Where Used:**
- `fileio.cpp` line 54-78: Parse pipe-delimited account data
- `banking.cpp` line 8-25: Build encoded PIN string character by character
- `gui.cpp` line 2777-2780: Build status message with amount

---

## 3.10 POINTERS (Windows Handles)

**File: `src/gui.cpp`**

```
CONCEPT: Variables that store memory addresses

WINDOW HANDLES (HWND):
HWND gHwnd = NULL;           // Pointer to main window
HWND button = CreateWindow(...);  // Returns pointer to button

GDI HANDLES:
HFONT hFontTitle;            // Pointer to font object
HBRUSH hBrushPrimary;        // Pointer to brush object
HDC hdc;                     // Pointer to device context (drawing surface)
```

**Where Used:**
- All GUI controls are accessed via HWND pointers
- Drawing uses HDC (Handle to Device Context)
- Fonts, brushes, pens are all pointer-based handles

---

# 4. FEATURE-BY-FEATURE BREAKDOWN

## FEATURE 1: USER LOGIN (ATM)

**Files Involved:**
- `gui.cpp` - Login screen painting & button handling
- `validation.cpp` - PIN validation
- `banking.cpp` - PIN decoding
- `fileio.cpp` - Load accounts from file

**FOCP Concepts:**
- `if-else` statements for validation
- `for` loop to search for account
- `string` comparison for PIN check
- `struct` to store account data

**How It Works:**

```
Step 1: User enters Account Number and PIN
        ↓
Step 2: gui.cpp receives button click (BTN_DO_LOGIN)
        ↓
Step 3: GetEditText() retrieves input from text boxes
        ↓
Step 4: loadAccounts() reads all accounts from file
        ↓
Step 5: findAccountIndex() searches for account number
        ↓
Step 6: decodePIN() converts stored PIN to original
        ↓
Step 7: Compare entered PIN with decoded PIN
        ↓
Step 8: If match → Login success, go to ATM Menu
        If wrong → Increment pinAttempts, show error
        If 3 wrong → Lock account
```

**Code Location:** `gui.cpp` lines 2301-2340

---

## FEATURE 2: ADMIN LOGIN

**Files Involved:**
- `gui.cpp` - Login screen & validation

**FOCP Concepts:**
- `const string` for password storage
- `if` statement for comparison
- `string` comparison

**How It Works:**

```
Step 1: Admin enters password
        ↓
Step 2: Compare with hardcoded password "SAA@Bank#2026"
        ↓
Step 3: If match → Set g.isAdmin = true, go to Dashboard
        If wrong → Show error message
```

**Password Location:** `gui.cpp` line 24
```cpp
const string ADMIN_PASSWORD = "SAA@Bank#2026";
```

---

## FEATURE 3: ACCOUNT CREATION (Admin)

**Files Involved:**
- `gui.cpp` - Form handling
- `validation.cpp` - Input validation
- `banking.cpp` - PIN encoding
- `fileio.cpp` - Save to file

**FOCP Concepts:**
- Multiple `if` statements for validation chain
- `struct` initialization
- `vector.push_back()` to add new account
- File writing with `ofstream`

**How It Works:**

```
Step 1: Admin fills form (Name, CNIC, Type, Balance, PIN)
        ↓
Step 2: Validate each field:
        - isValidName() - Only letters and spaces
        - isValidCNIC() - 13 digits with dashes
        - isValidAccountType() - "savings" or "current"
        - isValidAmount() - Positive number
        - isValidPIN() - Exactly 4 digits
        ↓
Step 3: Generate unique account number
        ↓
Step 4: encodePIN() - Convert PIN to encoded form
        ↓
Step 5: Create Account struct with all data
        ↓
Step 6: accounts.push_back(newAccount)
        ↓
Step 7: saveAccounts() - Write all accounts to file
        ↓
Step 8: logAudit() - Record admin action
```

**Code Location:** `gui.cpp` lines 2338-2380

---

## FEATURE 4: CASH DEPOSIT

**Files Involved:**
- `gui.cpp` - Deposit screen & handling
- `validation.cpp` - Amount validation
- `fileio.cpp` - Save account, save transaction

**FOCP Concepts:**
- `double` arithmetic (`balance += amount`)
- `struct` member access
- Function calls chain
- String formatting with `stringstream`

**How It Works:**

```
Step 1: User enters amount (e.g., 5000)
        ↓
Step 2: Validate amount:
        - isNumericDecimal() - Valid number
        - isPositiveAmount() - Greater than 0
        - isMultipleOf500() - Must be 500, 1000, 1500, etc.
        - Maximum limit check (Rs. 1,000,000)
        ↓
Step 3: Update balance:
        acc.balance += amount;  // Add to balance
        ↓
Step 4: saveAccounts() - Persist change to file
        ↓
Step 5: Create Transaction record:
        txn.type = "deposit";
        txn.amount = 5000;
        txn.resultingBalance = newBalance;
        ↓
Step 6: saveTransactions() - Add to history
        ↓
Step 7: generateReceipt() - Create receipt file
        ↓
Step 8: ShowReceipt() - Display receipt overlay
```

**Code Location:** `gui.cpp` lines 2686-2722

---

## FEATURE 5: CASH WITHDRAWAL

**Files Involved:**
- `gui.cpp` - Withdrawal screen & handling
- `validation.cpp` - Amount validation, balance check
- `banking.cpp` - Cash dispensing algorithm
- `fileio.cpp` - Save account, transaction, cash inventory

**FOCP Concepts:**
- Multiple validation `if` statements
- `for` loop in cash dispensing
- `double` arithmetic (`balance -= amount`)
- Array manipulation for cash notes

**How It Works:**

```
Step 1: User enters amount (e.g., 2000)
        ↓
Step 2: Validation chain:
        - isMultipleOf500() - Amount divisible by 500
        - Maximum Rs. 100,000 per transaction
        - acc.balance >= amount (sufficient balance)
        - acc.status == "active" (not frozen/locked)
        ↓
Step 3: Check daily limit:
        - Load today's withdrawals
        - dailyWithdrawn + amount <= 50000
        ↓
Step 4: Check ATM cash:
        - loadCashInventory()
        - getTotalCashInATM() >= amount
        ↓
Step 5: Dispense cash:
        - dispenseCash() algorithm tries to give exact notes
        - Uses greedy algorithm: 5000 → 1000 → 500
        ↓
Step 6: Update account:
        acc.balance -= amount;
        acc.dailyWithdrawn += amount;
        ↓
Step 7: Save all changes to files
        ↓
Step 8: Show receipt overlay
```

**Cash Dispensing Algorithm:** `banking.cpp` lines 55-90
```
For amount = 7500:
  - 5000 notes: 7500 / 5000 = 1 note, remaining = 2500
  - 1000 notes: 2500 / 1000 = 2 notes, remaining = 500
  - 500 notes: 500 / 500 = 1 note, remaining = 0
  Result: 1×5000 + 2×1000 + 1×500 = 7500 ✓
```

---

## FEATURE 6: FUND TRANSFER

**Files Involved:**
- `gui.cpp` - Transfer screen & OTP screen
- `validation.cpp` - Account validation
- `banking.cpp` - OTP generation
- `fileio.cpp` - Save both accounts

**FOCP Concepts:**
- Two-step process (OTP for large transfers)
- Finding receiver account in vector
- Updating two accounts simultaneously
- Random number generation for OTP

**How It Works:**

```
Step 1: User enters target account and amount
        ↓
Step 2: Validate:
        - Target account exists
        - Target != sender (can't transfer to self)
        - Target account is active
        - Sender has sufficient balance
        ↓
Step 3: Check if OTP required:
        if (amount >= 50000) {
            Generate OTP (4-digit random)
            Show OTP screen
            Wait for verification
        }
        ↓
Step 4: Execute transfer:
        sender.balance -= amount;
        receiver.balance += amount;
        ↓
Step 5: Create transaction record (for sender)
        ↓
Step 6: Save both accounts
        ↓
Step 7: Show receipt
```

**OTP Generation:** `banking.cpp` line 45-50
```cpp
string generateOTP() {
    srand((unsigned)time(0));
    int otp = 1000 + rand() % 9000;  // 1000-9999
    return to_string(otp);
}
```

---

## FEATURE 7: PIN SECURITY

**Files Involved:**
- `banking.cpp` - PIN encoding/decoding
- `validation.cpp` - PIN format validation
- `gui.cpp` - PIN change screen

**FOCP Concepts:**
- Character-by-character string processing
- Modulo arithmetic for encoding
- `for` loop through string

**How PIN Encoding Works:**

```
Original PIN: "1234"
Encoding: Each digit + 7, then mod 10

Digit '1': (1 + 7) % 10 = 8
Digit '2': (2 + 7) % 10 = 9
Digit '3': (3 + 7) % 10 = 0
Digit '4': (4 + 7) % 10 = 1

Encoded PIN: "8901" (stored in file)
```

**Decoding (reverse):**
```
Encoded: "8901"
Decoding: Each digit + 3, then mod 10 (reverse of +7)

Digit '8': (8 + 3) % 10 = 1
Digit '9': (9 + 3) % 10 = 2
Digit '0': (0 + 3) % 10 = 3
Digit '1': (1 + 3) % 10 = 4

Original PIN: "1234"
```

**Code Location:** `banking.cpp` lines 8-30

---

## FEATURE 8: LOAN MANAGEMENT

**Files Involved:**
- `gui.cpp` - Loan application & admin approval screens
- `fileio.cpp` - Save/load loans
- `types.h` - Loan struct with calculations

**FOCP Concepts:**
- Mathematical formula for monthly payment
- Struct member functions
- Status workflow (pending → approved → active)

**Loan Calculation:**
```
Loan Amount: Rs. 100,000
Interest Rate: 5% per year
Term: 12 months

Total Repayment = Amount × (1 + Rate × Term / 1200)
                = 100,000 × (1 + 5 × 12 / 1200)
                = 100,000 × 1.05
                = Rs. 105,000

Monthly Payment = 105,000 / 12 = Rs. 8,750
```

---

## FEATURE 9: TRANSACTION RECEIPT

**Files Involved:**
- `gui.cpp` - Receipt overlay painting
- `fileio.cpp` - Receipt file generation

**FOCP Concepts:**
- GDI drawing functions
- Struct data display
- String formatting

**Receipt Overlay Components:**
```
┌─────────────────────────────────┐
│     Transaction Receipt         │  ← Header (green background)
│     National Bank ATM           │
├─────────────────────────────────┤
│ Type:          Cash Withdrawal  │
│ Transaction ID: TXN445620       │
│ Account:       1001 - Ahmed     │
│ Amount:        Rs. 5,000.00     │
│ Balance:       Rs. 45,000.00    │
│ Date/Time:     2026-08-12 14:30 │
├─────────────────────────────────┤
│   Thank you for banking!        │
│        [Close Receipt]          │  ← Button
└─────────────────────────────────┘
```

---

## FEATURE 10: DAILY LIMITS & RESETS

**Files Involved:**
- `banking.cpp` - Reset logic
- `types.h` - Constants

**FOCP Concepts:**
- Date string comparison
- Conditional reset
- Global constants

**How Daily Reset Works:**
```cpp
void resetDailyWithdrawals(vector<Account>& accounts) {
    string today = getCurrentDateStr();  // "2026-08-12"

    for (auto& acc : accounts) {
        if (acc.lastWithdrawalDate != today) {
            // New day, reset counter
            acc.dailyWithdrawn = 0;
            acc.lastWithdrawalDate = today;
        }
    }
}
```

**Constants:** `types.h` line 201
```cpp
const double DAILY_WITHDRAWAL_LIMIT = 50000.0;  // Rs. 50,000 per day
```

---

# 5. WINDOWS GUI FUNCTIONS

## 5.1 Window Creation

**File:** `gui.cpp`

```cpp
// Main window
HWND hMainWnd = CreateWindowEx(
    0,                              // Extended style
    "NationalBankWindow",           // Window class name
    "National Bank - Banking System", // Window title
    WS_OVERLAPPEDWINDOW,            // Window style
    100, 100, 1200, 700,            // x, y, width, height
    NULL, NULL, hInstance, NULL     // Parent, menu, instance, param
);

// Edit box (text input)
HWND editBox = CreateWindow(
    "EDIT", "",                     // Class, initial text
    WS_CHILD | WS_VISIBLE | WS_BORDER,
    x, y, width, height,
    gHwnd, (HMENU)controlId, NULL, NULL
);

// Button
HWND button = CreateWindow(
    "BUTTON", "Click Me",
    WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,  // Owner-draw for custom look
    x, y, width, height,
    gHwnd, (HMENU)buttonId, NULL, NULL
);
```

## 5.2 Drawing Functions

**File:** `gui.cpp`

```cpp
// Draw text
TextOut(hdc, x, y, text, length);
DrawText(hdc, text, -1, &rect, DT_CENTER | DT_VCENTER);

// Draw shapes
FillRect(hdc, &rect, brush);           // Filled rectangle
RoundRect(hdc, x1, y1, x2, y2, rx, ry); // Rounded rectangle
Ellipse(hdc, x1, y1, x2, y2);          // Circle/ellipse

// Draw lines
MoveToEx(hdc, x1, y1, NULL);           // Start point
LineTo(hdc, x2, y2);                   // End point

// Colors
SetTextColor(hdc, RGB(255, 255, 255)); // White text
SetBkMode(hdc, TRANSPARENT);           // Transparent background
```

## 5.3 Font Creation

**File:** `gui.cpp`

```cpp
hFontTitle = CreateFont(
    26,                    // Height (points)
    0, 0, 0,               // Width, escapement, orientation
    FW_BOLD,               // Weight (bold)
    FALSE, FALSE, FALSE,   // Italic, underline, strikeout
    DEFAULT_CHARSET,       // Character set
    0, 0, 0, 0,            // Output precision, clip, quality, pitch
    "Segoe UI"             // Font name
);
```

## 5.4 Event Handling

**File:** `gui.cpp`

```cpp
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
        case WM_PAINT:        // Window needs repainting
            BeginPaint(...);
            PaintScreen(...);
            EndPaint(...);
            break;

        case WM_COMMAND:      // Button clicked
            int buttonId = LOWORD(wp);
            HandleCommand(buttonId);
            break;

        case WM_LBUTTONDOWN:  // Mouse left click
            int x = LOWORD(lp);
            int y = HIWORD(lp);
            // Handle click at (x, y)
            break;

        case WM_DESTROY:      // Window closing
            PostQuitMessage(0);
            break;
    }
    return DefWindowProc(hwnd, msg, wp, lp);
}
```

---

# 6. DATA FLOW EXAMPLES

## Example 1: Complete Withdrawal Flow

```
USER ACTION                          CODE EXECUTION
───────────────────────────────────────────────────────────────
1. User on Withdraw screen           PaintATMWithdraw() displays form

2. Types "5000" in amount box        Text stored in edit control

3. Clicks "Withdraw Cash"            WM_COMMAND received
                                     ↓
                                     HandleCommand(BTN_DO_WITHDRAW)
                                     ↓
4. Get input                         amtStr = GetEditText(EDT_AMT)
                                     amt = atof(amtStr.c_str())

5. Validate amount                   isMultipleOf500(5000) → true
                                     5000 <= 100000 → true

6. Check account                     acc.status == "active" → true
                                     acc.balance >= 5000 → true

7. Check daily limit                 resetDailyWithdrawals()
                                     dailyWithdrawn + 5000 <= 50000 → true

8. Check ATM cash                    loadCashInventory()
                                     getTotalCashInATM() >= 5000 → true

9. Dispense cash                     dispenseCash(inventory, 5000)
                                     (deducts 1×5000 note from inventory)

10. Update balance                   acc.balance -= 5000
                                     acc.dailyWithdrawn += 5000

11. Save to file                     saveAccounts(accounts)

12. Create transaction               txn.type = "withdrawal"
                                     txn.amount = 5000
                                     transactions.push_back(txn)
                                     saveTransactions()

13. Generate receipt file            generateReceipt(txn, acc)
                                     (creates receipts/TXN_1001_2026...txt)

14. Log action                       logAudit("Withdrawal", "...")

15. Show receipt                     ShowReceipt(txn, acc)
                                     ↓
                                     Sets gShowReceipt = true
                                     Creates BTN_CLOSE_RECEIPT button
                                     InvalidateRect() triggers repaint
                                     ↓
                                     PaintScreen() → PaintReceiptOverlay()
```

## Example 2: Data File Format

**accounts.txt:**
```
1001|Ahmed Khan|42301-1234567-1|current|45000.00|active|8901|0|5000.00|2026-08-12|2026-07-15 10:30:00
1002|Sara Ali|42301-7654321-2|savings|120000.00|active|1234|0|0.00|2026-08-12|2026-07-20 14:15:00
```

**Fields (pipe-separated):**
```
Position | Field              | Example
---------|--------------------|---------
1        | Account Number     | 1001
2        | Name               | Ahmed Khan
3        | CNIC               | 42301-1234567-1
4        | Account Type       | current
5        | Balance            | 45000.00
6        | Status             | active
7        | Encoded PIN        | 8901 (actual: 1234)
8        | Failed PIN Attempts| 0
9        | Daily Withdrawn    | 5000.00
10       | Last Withdrawal Date| 2026-08-12
11       | Creation Date      | 2026-07-15 10:30:00
```

---

# 7. DEMO SCRIPT

## Step 1: Admin Login (30 seconds)
- Open application
- Click "Bank Administrator"
- Enter password: `SAA@Bank#2026`
- Show admin dashboard

## Step 2: Create Account (1 minute)
- Click "Create Account"
- Fill form:
  - Name: Test User
  - CNIC: 12345-1234567-1
  - Type: Savings
  - Balance: 50000
  - PIN: 1234
- Click Create
- Show success message

## Step 3: Customer Login (30 seconds)
- Logout
- Click "ATM Customer"
- Enter Account Number and PIN: 1234
- Show ATM Menu

## Step 4: Check Balance (15 seconds)
- Click "ATM Balance"
- Show current balance

## Step 5: Deposit (30 seconds)
- Click "Deposit"
- Enter amount: 10000
- Click Deposit
- Show receipt overlay
- Click Close Receipt

## Step 6: Withdraw (45 seconds)
- Click "Withdraw"
- Enter amount: 5000
- Click Withdraw
- Show receipt with:
  - Transaction ID
  - Amount
  - New Balance
  - Date/Time
- Click Close Receipt

## Step 7: Transfer (1 minute)
- Click "Transfer"
- Enter target account number
- Enter amount: 60000 (triggers OTP)
- Show OTP screen
- Enter OTP
- Show transfer receipt

## Step 8: Mini Statement (30 seconds)
- Click "Mini Statement"
- Show last 15 transactions
- Point out color coding (green=deposit, red=withdrawal)

## Step 9: Admin Features (1 minute)
- Login as admin
- Show "View All Accounts"
- Show "Transaction History"
- Show "Audit Log"

---

# SUMMARY: FOCP CONCEPTS CHECKLIST

| Concept | File | Line Numbers |
|---------|------|--------------|
| Variables & Data Types | types.h | 10-55 |
| Arrays (fixed) | types.h | 206-213 |
| Vectors (dynamic) | fileio.cpp | 50, 80 |
| For Loop | validation.cpp | 52-55 |
| While Loop | fileio.cpp | 54 |
| If-Else | validation.cpp | 7-12 |
| Switch-Case | gui.cpp | 1825-1848 |
| Functions | All files | Throughout |
| Structures | types.h | 10-156 |
| Classes | types.h | 161-188 |
| File Input | fileio.cpp | 50-78 |
| File Output | fileio.cpp | 80-100 |
| String Operations | banking.cpp | 8-30 |
| Pointers (Handles) | gui.cpp | 165-168 |

---

**END OF DOCUMENTATION**
