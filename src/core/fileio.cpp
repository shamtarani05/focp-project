#include "fileio.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <direct.h>
#include <iomanip>
#include <ctime>

using namespace std;

bool ensureDirectoryExists(const string& dir) {
    _mkdir(dir.c_str());
    return true;
}

bool ensureDataFilesExist() {
    ensureDirectoryExists(DATA_DIR);
    ensureDirectoryExists(RECEIPTS_DIR);
    ensureDirectoryExists(LOGS_DIR);

    for (int i = 0; i < NUM_DATA_FILES; i++) {
        ifstream check(REQUIRED_DATA_FILES[i]);
        if (!check.is_open()) {
            ofstream f(REQUIRED_DATA_FILES[i]);
            f.close();
        }
        check.close();
    }
    return true;
}

static int safeStoi(const string& s, int defaultVal = 0) {
    if (s.empty()) return defaultVal;
    try {
        return stoi(s);
    } catch (...) {
        return defaultVal;
    }
}

static double safeStod(const string& s, double defaultVal = 0.0) {
    if (s.empty()) return defaultVal;
    try {
        return stod(s);
    } catch (...) {
        return defaultVal;
    }
}

bool loadAccounts(vector<Account>& accounts) {
    accounts.clear();
    ifstream file(ACCOUNTS_FILE);
    if (!file.is_open()) return false;

    string line;
    while (getline(file, line)) {
        if (line.empty()) continue;
        stringstream ss(line);
        Account acc;
        string temp;
        getline(ss, temp, '|'); acc.accountNo = safeStoi(temp);
        if (acc.accountNo <= 0) continue; // Skip invalid record
        getline(ss, acc.name, '|');
        getline(ss, acc.cnic, '|');
        getline(ss, acc.accountType, '|');
        getline(ss, temp, '|'); acc.balance = safeStod(temp);
        getline(ss, acc.status, '|');
        getline(ss, acc.pinHash, '|');
        getline(ss, temp, '|'); acc.pinAttempts = safeStoi(temp);
        getline(ss, temp, '|'); acc.dailyWithdrawn = safeStod(temp);
        getline(ss, acc.lastWithdrawalDate, '|');
        getline(ss, acc.creationDate, '|');
        accounts.push_back(acc);
    }
    file.close();
    return true;
}


bool saveAccounts(const vector<Account>& accounts) {
    ofstream file(ACCOUNTS_FILE);
    if (!file.is_open()) return false;

    for (const auto& acc : accounts) {
        file << acc.accountNo << "|" << acc.name << "|" << acc.cnic << "|"
             << acc.accountType << "|" << fixed << setprecision(2) << acc.balance << "|"
             << acc.status << "|" << acc.pinHash << "|" << acc.pinAttempts << "|"
             << fixed << setprecision(2) << acc.dailyWithdrawn << "|"
             << acc.lastWithdrawalDate << "|" << acc.creationDate << "\n";
    }
    file.close();
    return true;
}

bool appendAccount(const Account& acc) {
    ofstream file(ACCOUNTS_FILE, ios::app);
    if (!file.is_open()) return false;

    file << acc.accountNo << "|" << acc.name << "|" << acc.cnic << "|"
         << acc.accountType << "|" << fixed << setprecision(2) << acc.balance << "|"
         << acc.status << "|" << acc.pinHash << "|" << acc.pinAttempts << "|"
         << fixed << setprecision(2) << acc.dailyWithdrawn << "|"
         << acc.lastWithdrawalDate << "|" << acc.creationDate << "\n";
    file.close();
    return true;
}

bool updateAccountInFile(const Account& acc) {
    vector<Account> accounts;
    loadAccounts(accounts);
    for (auto& a : accounts) {
        if (a.accountNo == acc.accountNo) { a = acc; break; }
    }
    return saveAccounts(accounts);
}

bool deleteAccountFromFile(int accountNo) {
    vector<Account> accounts;
    loadAccounts(accounts);
    vector<Account> remaining;
    for (const auto& a : accounts) {
        if (a.accountNo != accountNo) remaining.push_back(a);
    }
    return saveAccounts(remaining);
}

bool loadTransactions(vector<Transaction>& transactions) {
    transactions.clear();
    ifstream file(TRANSACTIONS_FILE);
    if (!file.is_open()) return false;

    string line;
    while (getline(file, line)) {
        if (line.empty()) continue;
        stringstream ss(line);
        Transaction t;
        string temp;
        getline(ss, t.transactionID, '|');
        getline(ss, temp, '|'); t.accountNo = safeStoi(temp);
        getline(ss, t.type, '|');
        getline(ss, temp, '|'); t.amount = safeStod(temp);
        getline(ss, t.dateTime, '|');
        getline(ss, temp, '|'); t.resultingBalance = safeStod(temp);
        getline(ss, t.details, '|');
        if (!t.transactionID.empty()) transactions.push_back(t);
    }
    file.close();
    return true;
}

bool saveTransactions(const vector<Transaction>& transactions) {
    ofstream file(TRANSACTIONS_FILE);
    if (!file.is_open()) return false;

    for (const auto& t : transactions) {
        file << t.transactionID << "|" << t.accountNo << "|" << t.type << "|"
             << fixed << setprecision(2) << t.amount << "|" << t.dateTime << "|"
             << fixed << setprecision(2) << t.resultingBalance << "|" << t.details << "\n";
    }
    file.close();
    return true;
}

bool appendTransaction(const Transaction& t) {
    ofstream file(TRANSACTIONS_FILE, ios::app);
    if (!file.is_open()) return false;

    file << t.transactionID << "|" << t.accountNo << "|" << t.type << "|"
         << fixed << setprecision(2) << t.amount << "|" << t.dateTime << "|"
         << fixed << setprecision(2) << t.resultingBalance << "|" << t.details << "\n";
    file.close();
    return true;
}

bool loadLoans(vector<Loan>& loans) {
    loans.clear();
    ifstream file(LOANS_FILE);
    if (!file.is_open()) return false;

    string line;
    while (getline(file, line)) {
        if (line.empty()) continue;
        stringstream ss(line);
        Loan loan;
        string temp;
        getline(ss, temp, '|'); loan.loanId = safeStoi(temp);
        getline(ss, temp, '|'); loan.accountNo = safeStoi(temp);
        getline(ss, temp, '|'); loan.amount = safeStod(temp);
        getline(ss, temp, '|'); loan.interestRate = safeStod(temp);
        getline(ss, temp, '|'); loan.termMonths = safeStoi(temp);
        getline(ss, temp, '|'); loan.monthlyPayment = safeStod(temp);
        getline(ss, loan.status, '|');
        getline(ss, loan.applicationDate, '|');
        getline(ss, temp, '|'); loan.remainingAmount = safeStod(temp);
        getline(ss, temp, '|'); loan.monthsPaid = safeStoi(temp);
        if (loan.loanId > 0) loans.push_back(loan);
    }
    file.close();
    return true;
}

bool saveLoans(const vector<Loan>& loans) {
    ofstream file(LOANS_FILE);
    if (!file.is_open()) return false;

    for (const auto& loan : loans) {
        file << loan.loanId << "|" << loan.accountNo << "|"
             << fixed << setprecision(2) << loan.amount << "|"
             << loan.interestRate << "|" << loan.termMonths << "|"
             << fixed << setprecision(2) << loan.monthlyPayment << "|"
             << loan.status << "|" << loan.applicationDate << "|"
             << fixed << setprecision(2) << loan.remainingAmount << "|"
             << loan.monthsPaid << "\n";
    }
    file.close();
    return true;
}

bool appendLoan(const Loan& loan) {
    vector<Loan> loans;
    loadLoans(loans);
    loans.push_back(loan);
    return saveLoans(loans);
}

bool updateLoanInFile(const Loan& loan) {
    vector<Loan> loans;
    loadLoans(loans);
    for (auto& l : loans) {
        if (l.loanId == loan.loanId) { l = loan; break; }
    }
    return saveLoans(loans);
}

bool loadCashInventory(vector<CashNote>& inventory) {
    inventory.clear();
    ifstream file(CASH_FILE);
    if (!file.is_open()) return false;

    string line;
    while (getline(file, line)) {
        if (line.empty()) continue;
        stringstream ss(line);
        CashNote note;
        string temp;
        getline(ss, temp, '|'); note.denomination = safeStoi(temp);
        getline(ss, temp, '|'); note.count = safeStoi(temp);
        if (note.denomination >= 500) inventory.push_back(note);
    }
    file.close();
    return true;
}

bool saveCashInventory(const vector<CashNote>& inventory) {
    ofstream file(CASH_FILE);
    if (!file.is_open()) return false;

    for (const auto& note : inventory) {
        file << note.denomination << "|" << note.count << "\n";
    }
    file.close();
    return true;
}

bool appendAuditEntry(const AuditEntry& entry) {
    ensureDirectoryExists(DATA_DIR);
    ofstream file(AUDIT_FILE, ios::app);
    if (!file.is_open()) return false;

    file << entry.timestamp << "|" << entry.action << "|" << entry.details << "\n";
    file.close();
    return true;
}

vector<AuditEntry> loadAuditEntries() {
    vector<AuditEntry> entries;
    ifstream file(AUDIT_FILE);
    if (!file.is_open()) return entries;

    string line;
    while (getline(file, line)) {
        if (line.empty()) continue;
        stringstream ss(line);
        AuditEntry entry;
        getline(ss, entry.timestamp, '|');
        getline(ss, entry.action, '|');
        getline(ss, entry.details, '|');
        entries.push_back(entry);
    }
    file.close();
    return entries;
}

bool generateReceipt(const Transaction& t, const Account& acc) {
    ensureDirectoryExists(RECEIPTS_DIR);

    string filename = RECEIPTS_DIR + "/receipt_" + t.transactionID + ".txt";
    ofstream file(filename);
    if (!file.is_open()) return false;

    file << "=============================================\n";
    file << "        NATIONAL BANK - TRANSACTION RECEIPT   \n";
    file << "=============================================\n";
    file << "  Transaction ID  : " << t.transactionID << "\n";
    file << "  Date/Time       : " << t.dateTime << "\n";
    file << "---------------------------------------------\n";
    file << "  Account Number  : " << t.accountNo << "\n";
    file << "  Account Holder  : " << acc.name << "\n";
    file << "---------------------------------------------\n";
    file << "  Type            : " << t.type << "\n";
    file << "  Amount          : Rs. " << fixed << setprecision(2) << t.amount << "\n";
    file << "  Resulting Bal   : Rs. " << fixed << setprecision(2) << t.resultingBalance << "\n";
    file << "---------------------------------------------\n";
    if (!t.details.empty()) {
        file << "  Details         : " << t.details << "\n";
        file << "---------------------------------------------\n";
    }
    file << "  Thank you for banking with us!\n";
    file << "=============================================\n";

    file.close();
    return true;
}

bool backupData() {
    ensureDirectoryExists("backup");
    time_t now = time(0);
    tm* ltm = localtime(&now);
    stringstream ss;
    ss << "backup/backup_" << setfill('0') << setw(4) << (1900 + ltm->tm_year)
       << setw(2) << (1 + ltm->tm_mon) << setw(2) << ltm->tm_mday
       << "_" << setw(2) << ltm->tm_hour << setw(2) << ltm->tm_min;
    string backupDir = ss.str();
    _mkdir(backupDir.c_str());

    auto copyFile = [](const string& src, const string& dest) {
        ifstream in(src, ios::binary);
        if (!in.is_open()) return false;
        ofstream out(dest, ios::binary);
        out << in.rdbuf();
        return true;
    };

    bool ok = true;
    ok &= copyFile(ACCOUNTS_FILE, backupDir + "/accounts.txt");
    ok &= copyFile(TRANSACTIONS_FILE, backupDir + "/transactions.txt");
    ok &= copyFile(AUDIT_FILE, backupDir + "/audit.txt");
    ok &= copyFile(LOANS_FILE, backupDir + "/loans.txt");
    ok &= copyFile(CASH_FILE, backupDir + "/cash_inventory.txt");
    ok &= copyFile(REACTIVATION_FILE, backupDir + "/reactivation_requests.txt");
    return ok;
}

// ============================
// REACTIVATION REQUEST I/O
// ============================

bool loadReactivationRequests(vector<ReactivationRequest>& requests) {
    requests.clear();
    ifstream file(REACTIVATION_FILE);
    if (!file.is_open()) return false;

    string line;
    while (getline(file, line)) {
        if (line.empty()) continue;
        stringstream ss(line);
        ReactivationRequest req;
        string temp;
        getline(ss, temp, '|'); req.requestId  = safeStoi(temp);
        getline(ss, temp, '|'); req.accountNo  = safeStoi(temp);
        getline(ss, req.name,     '|');
        getline(ss, req.cnic,     '|');
        getline(ss, req.reason,   '|');
        getline(ss, req.dateTime, '|');
        getline(ss, req.status,   '|');
        if (req.requestId > 0) requests.push_back(req);
    }
    file.close();
    return true;
}

bool saveReactivationRequests(const vector<ReactivationRequest>& requests) {
    ofstream file(REACTIVATION_FILE);
    if (!file.is_open()) return false;

    for (const auto& req : requests) {
        file << req.requestId << "|" << req.accountNo << "|" << req.name << "|"
             << req.cnic << "|" << req.reason << "|" << req.dateTime << "|" << req.status << "\n";
    }
    file.close();
    return true;
}

bool appendReactivationRequest(const ReactivationRequest& req) {
    ensureDirectoryExists(DATA_DIR);
    ofstream file(REACTIVATION_FILE, ios::app);
    if (!file.is_open()) return false;

    file << req.requestId << "|" << req.accountNo << "|" << req.name << "|"
         << req.cnic << "|" << req.reason << "|" << req.dateTime << "|" << req.status << "\n";
    file.close();
    return true;
}

// ============================
// ENHANCED FILE HANDLING & CSV EXPORT
// ============================

static string escapeCSV(const string& s) {
    if (s.find(',') != string::npos || s.find('"') != string::npos || s.find('\n') != string::npos) {
        string res = "\"";
        for (char c : s) {
            if (c == '"') res += "\"\"";
            else res += c;
        }
        res += "\"";
        return res;
    }
    return s;
}

bool exportAccountsCSV(const string& filename) {
    vector<Account> accounts;
    if (!loadAccounts(accounts)) return false;

    ofstream file(filename);
    if (!file.is_open()) return false;

    file << "AccountNo,Name,CNIC,AccountType,Balance,Status,PinAttempts,DailyWithdrawn,LastWithdrawalDate,CreationDate\n";
    for (const auto& acc : accounts) {
        file << acc.accountNo << ","
             << escapeCSV(acc.name) << ","
             << escapeCSV(acc.cnic) << ","
             << escapeCSV(acc.accountType) << ","
             << fixed << setprecision(2) << acc.balance << ","
             << escapeCSV(acc.status) << ","
             << acc.pinAttempts << ","
             << fixed << setprecision(2) << acc.dailyWithdrawn << ","
             << escapeCSV(acc.lastWithdrawalDate) << ","
             << escapeCSV(acc.creationDate) << "\n";
    }
    file.close();
    return true;
}

bool exportTransactionsCSV(const string& filename) {
    vector<Transaction> transactions;
    if (!loadTransactions(transactions)) return false;

    ofstream file(filename);
    if (!file.is_open()) return false;

    file << "TransactionID,AccountNo,Type,Amount,DateTime,ResultingBalance,Details\n";
    for (const auto& t : transactions) {
        file << escapeCSV(t.transactionID) << ","
             << t.accountNo << ","
             << escapeCSV(t.type) << ","
             << fixed << setprecision(2) << t.amount << ","
             << escapeCSV(t.dateTime) << ","
             << fixed << setprecision(2) << t.resultingBalance << ","
             << escapeCSV(t.details) << "\n";
    }
    file.close();
    return true;
}

bool exportAuditEntriesCSV(const string& filename) {
    vector<AuditEntry> entries = loadAuditEntries();
    ofstream file(filename);
    if (!file.is_open()) return false;

    file << "Timestamp,Action,Details\n";
    for (const auto& entry : entries) {
        file << escapeCSV(entry.timestamp) << ","
             << escapeCSV(entry.action) << ","
             << escapeCSV(entry.details) << "\n";
    }
    file.close();
    return true;
}

bool exportAccountStatementTXT(const Account& acc, const vector<Transaction>& txns, const string& filepath) {
    ensureDirectoryExists(RECEIPTS_DIR);
    ofstream file(filepath);
    if (!file.is_open()) return false;

    file << "=================================================================\n";
    file << "                 NATIONAL BANK - ACCOUNT STATEMENT               \n";
    file << "=================================================================\n";
    file << "  Account Number : " << acc.accountNo << "\n";
    file << "  Account Holder : " << acc.name << "\n";
    file << "  CNIC           : " << acc.cnic << "\n";
    file << "  Account Type   : " << acc.accountType << "\n";
    file << "  Current Balance: Rs. " << fixed << setprecision(2) << acc.balance << "\n";
    file << "  Status         : " << acc.status << "\n";
    file << "-----------------------------------------------------------------\n";
    file << "  TRANSACTION HISTORY:\n";
    file << "  " << left << setw(14) << "TXN ID" 
         << setw(20) << "Date/Time" 
         << setw(12) << "Type" 
         << right << setw(12) << "Amount" 
         << setw(14) << "Balance" << "\n";
    file << "  ---------------------------------------------------------------\n";

    double totalDeposits = 0;
    double totalWithdrawals = 0;
    int count = 0;

    for (const auto& t : txns) {
        if (t.accountNo == acc.accountNo) {
            file << "  " << left << setw(14) << t.transactionID
                 << setw(20) << t.dateTime
                 << setw(12) << t.type
                 << right << setw(12) << fixed << setprecision(2) << t.amount
                 << setw(14) << fixed << setprecision(2) << t.resultingBalance << "\n";
            count++;
            if (t.type == "deposit" || t.type == "transfer_in") {
                totalDeposits += t.amount;
            } else if (t.type == "withdraw" || t.type == "transfer_out") {
                totalWithdrawals += t.amount;
            }
        }
    }

    file << "-----------------------------------------------------------------\n";
    file << "  Total Transactions : " << count << "\n";
    file << "  Total Deposits     : Rs. " << fixed << setprecision(2) << totalDeposits << "\n";
    file << "  Total Withdrawals  : Rs. " << fixed << setprecision(2) << totalWithdrawals << "\n";
    file << "=================================================================\n";
    file.close();
    return true;
}

bool writeSystemLog(const string& level, const string& message) {
    ensureDirectoryExists(LOGS_DIR);
    ofstream file(LOGS_DIR + "/system.log", ios::app);
    if (!file.is_open()) return false;

    time_t now = time(0);
    tm* ltm = localtime(&now);
    file << "[" << setfill('0') << setw(4) << (1900 + ltm->tm_year) << "-"
         << setw(2) << (1 + ltm->tm_mon) << "-"
         << setw(2) << ltm->tm_mday << " "
         << setw(2) << ltm->tm_hour << ":"
         << setw(2) << ltm->tm_min << ":"
         << setw(2) << ltm->tm_sec << "] ["
         << level << "] " << message << "\n";
    file.close();
    return true;
}

bool restoreBackup(const string& backupDir) {
    auto copyFile = [](const string& src, const string& dest) {
        ifstream in(src, ios::binary);
        if (!in.is_open()) return false;
        ofstream out(dest, ios::binary);
        out << in.rdbuf();
        return true;
    };

    ensureDirectoryExists(DATA_DIR);
    bool ok = true;
    ok &= copyFile(backupDir + "/accounts.txt", ACCOUNTS_FILE);
    ok &= copyFile(backupDir + "/transactions.txt", TRANSACTIONS_FILE);
    ok &= copyFile(backupDir + "/audit.txt", AUDIT_FILE);
    ok &= copyFile(backupDir + "/loans.txt", LOANS_FILE);
    ok &= copyFile(backupDir + "/cash_inventory.txt", CASH_FILE);
    ok &= copyFile(backupDir + "/reactivation_requests.txt", REACTIVATION_FILE);
    return ok;
}

bool verifyDataIntegrity(string& reportOutput) {
    ensureDirectoryExists(LOGS_DIR);
    stringstream ss;
    ss << "=== DATA INTEGRITY CHECK REPORT ===\n";

    // Check accounts
    vector<Account> accounts;
    if (!loadAccounts(accounts)) {
        ss << "[FAIL] Failed to open accounts file: " << ACCOUNTS_FILE << "\n";
    } else {
        ss << "[OK] Accounts loaded successfully. Count: " << accounts.size() << "\n";
        for (const auto& a : accounts) {
            if (a.balance < 0) {
                ss << "[WARNING] Account #" << a.accountNo << " has negative balance: Rs. " << a.balance << "\n";
            }
        }
    }

    // Check transactions
    vector<Transaction> transactions;
    if (!loadTransactions(transactions)) {
        ss << "[FAIL] Failed to open transactions file: " << TRANSACTIONS_FILE << "\n";
    } else {
        ss << "[OK] Transactions loaded successfully. Count: " << transactions.size() << "\n";
    }

    // Check loans
    vector<Loan> loans;
    if (!loadLoans(loans)) {
        ss << "[FAIL] Failed to open loans file: " << LOANS_FILE << "\n";
    } else {
        ss << "[OK] Loans loaded successfully. Count: " << loans.size() << "\n";
    }

    // Check audit entries
    vector<AuditEntry> audit = loadAuditEntries();
    ss << "[OK] Audit entries count: " << audit.size() << "\n";

    reportOutput = ss.str();

    ofstream logFile(LOGS_DIR + "/integrity_report.txt");
    if (logFile.is_open()) {
        logFile << reportOutput;
        logFile.close();
    }

    return true;
}

