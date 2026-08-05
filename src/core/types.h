#ifndef TYPES_H
#define TYPES_H

#include <string>
#include <vector>
#include <ctime>

using namespace std;

struct Account {
    int accountNo;
    string name;
    string cnic;
    string accountType;
    double balance;
    string status;
    string pinHash;
    int pinAttempts;
    double dailyWithdrawn;
    string lastWithdrawalDate;
    string creationDate;

    Account() : accountNo(0), balance(0), status("active"),
                pinAttempts(0), dailyWithdrawn(0) {}
};

struct Transaction {
    string transactionID;
    int accountNo;
    string type;
    double amount;
    string dateTime;
    double resultingBalance;
    string details;

    Transaction() : accountNo(0), amount(0), resultingBalance(0) {}
};

struct Loan {
    int loanId;
    int accountNo;
    double amount;
    double interestRate;
    int termMonths;
    double monthlyPayment;
    string status;
    string applicationDate;
    double remainingAmount;
    int monthsPaid;

    Loan() : loanId(0), accountNo(0), amount(0), interestRate(5.0),
             termMonths(0), monthlyPayment(0), remainingAmount(0), monthsPaid(0) {}
};

struct CashNote {
    int denomination;
    int count;

    CashNote() : denomination(0), count(0) {}
    CashNote(int d, int c) : denomination(d), count(c) {}
};

struct AuditEntry {
    string timestamp;
    string action;
    string details;

    AuditEntry() {}
    AuditEntry(string t, string a, string d) : timestamp(t), action(a), details(d) {}
};

struct ReactivationRequest {
    int    requestId;
    int    accountNo;
    string name;
    string cnic;
    string reason;
    string dateTime;
    string status;  // pending / approved / rejected

    ReactivationRequest() : requestId(0), accountNo(0), status("pending") {}
};

const string DATA_DIR   = "data";
const string RECEIPTS_DIR = "receipts";
const string LOGS_DIR   = "logs";

const string ACCOUNTS_FILE      = DATA_DIR + "/accounts.txt";
const string TRANSACTIONS_FILE  = DATA_DIR + "/transactions.txt";
const string AUDIT_FILE         = DATA_DIR + "/audit.txt";
const string LOANS_FILE         = DATA_DIR + "/loans.txt";
const string CASH_FILE          = DATA_DIR + "/cash_inventory.txt";
const string REACTIVATION_FILE  = DATA_DIR + "/reactivation_requests.txt";

const double DAILY_WITHDRAWAL_LIMIT = 50000.0;
const double SAVINGS_INTEREST_RATE  = 0.05;
const double OTP_THRESHOLD          = 50000.0;
const int    MAX_PIN_ATTEMPTS       = 3;

const int DENOMINATIONS[]   = {5000, 1000, 500, 100};
const int NUM_DENOMINATIONS = 4;

const string VALID_ACCOUNT_TYPES[] = {"savings", "current"};
const int    NUM_ACCOUNT_TYPES     = 2;

const string VALID_STATUSES[] = {"active", "frozen", "locked"};
const int    NUM_STATUSES     = 3;

const string REQUIRED_DATA_FILES[] = {
    DATA_DIR + "/accounts.txt",
    DATA_DIR + "/transactions.txt",
    DATA_DIR + "/audit.txt",
    DATA_DIR + "/loans.txt",
    DATA_DIR + "/cash_inventory.txt",
    DATA_DIR + "/reactivation_requests.txt"
};
const int NUM_DATA_FILES = 6;

#endif
