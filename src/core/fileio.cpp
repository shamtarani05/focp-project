#include "core/fileio.h"
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

    const char* files[] = {
        ACCOUNTS_FILE.c_str(),
        TRANSACTIONS_FILE.c_str(),
        AUDIT_FILE.c_str(),
        LOANS_FILE.c_str(),
        CASH_FILE.c_str()
    };
    for (int i = 0; i < 5; i++) {
        ifstream check(files[i]);
        if (!check.is_open()) {
            ofstream f(files[i]);
            f.close();
        }
        check.close();
    }
    return true;
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
        getline(ss, temp, '|'); acc.accountNo = stoi(temp);
        getline(ss, acc.name, '|');
        getline(ss, acc.cnic, '|');
        getline(ss, acc.accountType, '|');
        getline(ss, temp, '|'); acc.balance = stod(temp);
        getline(ss, acc.status, '|');
        getline(ss, acc.pinHash, '|');
        getline(ss, temp, '|'); acc.pinAttempts = stoi(temp);
        getline(ss, temp, '|'); acc.dailyWithdrawn = stod(temp);
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
        getline(ss, temp, '|'); t.accountNo = stoi(temp);
        getline(ss, t.type, '|');
        getline(ss, temp, '|'); t.amount = stod(temp);
        getline(ss, t.dateTime, '|');
        getline(ss, temp, '|'); t.resultingBalance = stod(temp);
        getline(ss, t.details, '|');
        transactions.push_back(t);
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
        getline(ss, temp, '|'); loan.loanId = stoi(temp);
        getline(ss, temp, '|'); loan.accountNo = stoi(temp);
        getline(ss, temp, '|'); loan.amount = stod(temp);
        getline(ss, temp, '|'); loan.interestRate = stod(temp);
        getline(ss, temp, '|'); loan.termMonths = stoi(temp);
        getline(ss, temp, '|'); loan.monthlyPayment = stod(temp);
        getline(ss, loan.status, '|');
        getline(ss, loan.applicationDate, '|');
        getline(ss, temp, '|'); loan.remainingAmount = stod(temp);
        getline(ss, temp, '|'); loan.monthsPaid = stoi(temp);
        loans.push_back(loan);
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
        getline(ss, temp, '|'); note.denomination = stoi(temp);
        getline(ss, temp, '|'); note.count = stoi(temp);
        inventory.push_back(note);
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
    return ok;
}
