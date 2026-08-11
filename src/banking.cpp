#include "banking.h"
#include "core/fileio.h"
#include <sstream>
#include <iomanip>
#include <cstdlib>
#include <ctime>

string encodePIN(const string& pin) {
    string encoded = "";
    for (char c : pin) {
        int val = (c - '0' + 7) % 10;
        encoded += to_string(val);
    }
    return encoded;
}

string decodePIN(const string& encoded) {
    string decoded = "";
    for (char c : encoded) {
        int val = (c - '0' + 3) % 10;
        decoded += to_string(val);
    }
    return decoded;
}

string generateOTP() {
    srand((unsigned)time(0));
    int otp = 1000 + rand() % 9000;
    return to_string(otp);
}

string getTimestamp() { return getCurrentDateTimeStr(); }

string getCurrentDateTimeStr() {
    time_t now = time(0);
    tm* ltm = localtime(&now);
    stringstream ss;
    ss << setfill('0') << setw(4) << (1900 + ltm->tm_year) << "-"
       << setw(2) << (1 + ltm->tm_mon) << "-"
       << setw(2) << ltm->tm_mday << " "
       << setw(2) << ltm->tm_hour << ":"
       << setw(2) << ltm->tm_min << ":"
       << setw(2) << ltm->tm_sec;
    return ss.str();
}

string getCurrentDateStr() {
    time_t now = time(0);
    tm* ltm = localtime(&now);
    stringstream ss;
    ss << setfill('0') << setw(4) << (1900 + ltm->tm_year) << "-"
       << setw(2) << (1 + ltm->tm_mon) << "-"
       << setw(2) << ltm->tm_mday;
    return ss.str();
}

void logAudit(const string& action, const string& details) {
    AuditEntry entry;
    entry.timestamp = getCurrentDateTimeStr();
    entry.action = action;
    entry.details = details;
    appendAuditEntry(entry);
    writeSystemLog("AUDIT", action + ": " + details);
}

void initCashInventory(vector<CashNote>& inventory) {
    inventory.clear();
    const int defaultCounts[NUM_DENOMINATIONS] = {100, 200, 300, 500};
    for (int i = 0; i < NUM_DENOMINATIONS; i++) {
        inventory.push_back(CashNote(DENOMINATIONS[i], defaultCounts[i]));
    }
    saveCashInventory(inventory);
}

bool dispenseCash(vector<CashNote>& inventory, double amount) {
    int remaining = (int)amount;
    for (size_t i = 0; i < inventory.size(); i++) {
        int denom = inventory[i].denomination;
        int needed = remaining / denom;
        int give = (needed < inventory[i].count) ? needed : inventory[i].count;
        remaining -= give * denom;
        inventory[i].count -= give;
    }
    if (remaining > 0) return false;
    saveCashInventory(inventory);
    return true;
}

double calculateProfit(double balance, double rate, int days) {
    return balance * rate * days / 365.0;
}

void resetDailyWithdrawals(vector<Account>& accounts) {
    string today = getCurrentDateStr();
    for (auto& acc : accounts) {
        if (acc.lastWithdrawalDate != today) {
            acc.dailyWithdrawn = 0;
            acc.lastWithdrawalDate = today;
        }
    }
    saveAccounts(accounts);
}

vector<Transaction> filterTransactions(const vector<Transaction>& transactions,
                                       int accountNo, const string& type,
                                       const string& startDate, const string& endDate,
                                       double minAmount, double maxAmount) {
    TransactionManager manager(transactions);
    return manager.filter(accountNo, type, startDate, endDate, minAmount, maxAmount);
}

int getNextAccountNo(const vector<Account>& accounts) {
    int mx = 1000;
    for (const auto& a : accounts) if (a.accountNo > mx) mx = a.accountNo;
    return mx + 1;
}

string generateTransactionID(const vector<Transaction>& txns) {
    int seed = (int)time(0) + (int)txns.size();
    stringstream ss;
    ss << "TXN" << setfill('0') << setw(6) << (seed % 1000000);
    return ss.str();
}

int getNextLoanID(const vector<Loan>& loans) {
    int mx = 0;
    for (const auto& l : loans) if (l.loanId > mx) mx = l.loanId;
    return mx + 1;
}
