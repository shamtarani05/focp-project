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

    // Default & Parameterized Constructors (OOP Concept)
    Account() : accountNo(0), balance(0), status("active"),
                pinAttempts(0), dailyWithdrawn(0) {}

    Account(int accNo, string n, string c, string type, double bal)
        : accountNo(accNo), name(n), cnic(c), accountType(type), balance(bal),
          status("active"), pinAttempts(0), dailyWithdrawn(0) {}

    // OOP Member Functions
    bool isActive() const { return status == "active"; }
    bool isFrozen() const { return status == "frozen"; }
    bool isLocked() const { return status == "locked"; }

    bool canWithdraw(double amount) const {
        return isActive() && amount > 0 && balance >= amount;
    }

    void deposit(double amount) {
        if (amount > 0) balance += amount;
    }

    bool withdraw(double amount) {
        if (canWithdraw(amount)) {
            balance -= amount;
            dailyWithdrawn += amount;
            return true;
        }
        return false;
    }

    void resetPinAttempts() { pinAttempts = 0; }
    void incrementPinAttempts() { pinAttempts++; }
};

struct Transaction {
    string transactionID;
    int accountNo;
    string type;
    double amount;
    string dateTime;
    double resultingBalance;
    string details;

    // Default & Parameterized Constructors (OOP Concept)
    Transaction() : accountNo(0), amount(0), resultingBalance(0) {}

    Transaction(string id, int accNo, string t, double amt, string dt, double resBal, string det = "")
        : transactionID(id), accountNo(accNo), type(t), amount(amt),
          dateTime(dt), resultingBalance(resBal), details(det) {}

    // OOP Member Functions
    bool isDeposit() const { return type == "deposit"; }
    bool isWithdrawal() const { return type == "withdrawal"; }
    bool isTransfer() const { return type == "transfer"; }

    bool matchesFilter(int filterAcc, const string& filterType,
                       const string& startDate, const string& endDate,
                       double minAmt, double maxAmt) const {
        if (filterAcc != -1 && accountNo != filterAcc) return false;
        if (!filterType.empty() && type != filterType) return false;
        if (!startDate.empty() && dateTime.substr(0, 10) < startDate) return false;
        if (!endDate.empty() && dateTime.substr(0, 10) > endDate) return false;
        if (minAmt >= 0 && amount < minAmt) return false;
        if (maxAmt >= 0 && amount > maxAmt) return false;
        return true;
    }
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

    // Constructors
    Loan() : loanId(0), accountNo(0), amount(0), interestRate(5.0),
             termMonths(0), monthlyPayment(0), remainingAmount(0), monthsPaid(0) {}

    Loan(int id, int accNo, double amt, int term, double rate = 5.0)
        : loanId(id), accountNo(accNo), amount(amt), interestRate(rate),
          termMonths(term), remainingAmount(amt), monthsPaid(0), status("pending") {
        monthlyPayment = (term > 0) ? (amt * (1 + rate * term / 1200.0)) / term : 0;
    }

    // OOP Member Functions
    bool isApproved() const { return status == "approved" || status == "active"; }
    bool isPending() const { return status == "pending"; }
    
    double calculateTotalRepayment() const {
        return amount * (1.0 + interestRate * termMonths / 1200.0);
    }
};

struct CashNote {
    int denomination;
    int count;

    CashNote() : denomination(0), count(0) {}
    CashNote(int d, int c) : denomination(d), count(c) {}

    double totalValue() const { return denomination * count; }
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
    ReactivationRequest(int reqId, int accNo, string n, string c, string r, string dt)
        : requestId(reqId), accountNo(accNo), name(n), cnic(c), reason(r), dateTime(dt), status("pending") {}

    bool isPending() const { return status == "pending"; }
    bool isApproved() const { return status == "approved"; }
};

// --- OOP Class Definitions for Core Banking Services ---

// Class demonstrating Encapsulation (Private Data Members, Public Member Methods)
class TransactionManager {
private:
    vector<Transaction> transactions;

public:
    TransactionManager() {}
    TransactionManager(const vector<Transaction>& txns) : transactions(txns) {}

    void setTransactions(const vector<Transaction>& txns) {
        transactions = txns;
    }

    const vector<Transaction>& getTransactions() const {
        return transactions;
    }

    vector<Transaction> filter(int accountNo, const string& type,
                               const string& startDate, const string& endDate,
                               double minAmount, double maxAmount) const {
        vector<Transaction> result;
        for (const auto& t : transactions) {
            if (t.matchesFilter(accountNo, type, startDate, endDate, minAmount, maxAmount)) {
                result.push_back(t);
            }
        }
        return result;
    }
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

const int DENOMINATIONS[]   = {5000, 1000, 500};
const int NUM_DENOMINATIONS = 3;

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
