#include "validation.h"
#include <algorithm>
#include <cctype>

using namespace std;

bool isValidPIN(const string& pin) {
    if (pin.length() != 4) return false;
    for (char c : pin) {
        if (c < '0' || c > '9') return false;
    }
    return true;
}

bool isValidAmount(double amount) {
    return amount > 0;
}

bool isPositiveAmount(double amount) {
    return amount > 0;
}

bool isUniqueAccountNo(int accountNo, const vector<Account>& accounts) {
    for (const auto& acc : accounts) {
        if (acc.accountNo == accountNo) return false;
    }
    return true;
}

bool isValidCNIC(const string& cnic) {
    if (cnic.empty()) return false;
    string cleaned;
    for (char c : cnic) {
        if (c != '-' && c != ' ') cleaned += c;
    }
    if (cleaned.length() != 13) return false;
    for (char c : cleaned) {
        if (c < '0' || c > '9') return false;
    }
    return true;
}

bool isValidName(const string& name) {
    if (name.empty() || name.length() < 2) return false;
    for (char c : name) {
        if (!isalpha(c) && c != ' ' && c != '.' && c != '-') return false;
    }
    return true;
}

bool isValidAccountType(const string& type) {
    for (int i = 0; i < NUM_ACCOUNT_TYPES; i++) {
        if (type == VALID_ACCOUNT_TYPES[i]) return true;
    }
    return false;
}

bool hasSufficientBalance(int accountNo, double amount, const vector<Account>& accounts) {
    for (const auto& acc : accounts) {
        if (acc.accountNo == accountNo) return acc.balance >= amount;
    }
    return false;
}

bool isAccountActive(int accountNo, const vector<Account>& accounts) {
    for (const auto& acc : accounts) {
        if (acc.accountNo == accountNo) return acc.status == "active";
    }
    return false;
}

bool isWithinDailyLimit(int accountNo, double amount, const vector<Account>& accounts) {
    for (const auto& acc : accounts) {
        if (acc.accountNo == accountNo) {
            return (acc.dailyWithdrawn + amount) <= DAILY_WITHDRAWAL_LIMIT;
        }
    }
    return false;
}

bool isTransferValid(int senderAcc, int receiverAcc, double amount, const vector<Account>& accounts) {
    if (senderAcc == receiverAcc) return false;

    bool senderExists = false, receiverExists = false;
    bool senderActive = false, receiverActive = false;
    double senderBalance = 0;

    for (const auto& acc : accounts) {
        if (acc.accountNo == senderAcc) {
            senderExists = true;
            senderActive = (acc.status == "active");
            senderBalance = acc.balance;
        }
        if (acc.accountNo == receiverAcc) {
            receiverExists = true;
            receiverActive = (acc.status == "active");
        }
    }

    return senderExists && receiverExists && senderActive && receiverActive && (senderBalance >= amount);
}

bool isValidOTP(const string& input, const string& otp) {
    return input == otp;
}

bool isValidLoanAmount(double amount, double balance) {
    return amount > 0 && amount <= balance * 3;
}

bool isNumeric(const string& s) {
    if (s.empty()) return false;
    for (char c : s) {
        if (!isdigit(c)) return false;
    }
    return true;
}

bool isNumericDecimal(const string& s) {
    if (s.empty()) return false;
    int dotCount = 0;
    for (size_t i = 0; i < s.length(); i++) {
        char c = s[i];
        if (c == '.') {
            dotCount++;
            if (dotCount > 1) return false;
        } else if (!isdigit(c)) {
            return false;
        }
    }
    return true;
}

bool isMultipleOf500(double amount) {
    if (amount < 500.0) return false;
    long long rounded = (long long)(amount + 0.0001);
    if (abs(amount - (double)rounded) > 0.001) return false; // not an integer
    return (rounded % 500 == 0);
}

double getTotalCashInATM(const vector<CashNote>& inventory) {
    double total = 0;
    for (const auto& note : inventory) {
        total += (double)note.denomination * note.count;
    }
    return total;
}

bool hasPendingReactivationRequest(int accountNo, const vector<ReactivationRequest>& requests) {
    for (const auto& req : requests) {
        if (req.accountNo == accountNo && req.status == "pending") return true;
    }
    return false;
}

bool hasActiveLoan(int accountNo, const vector<Loan>& loans) {
    for (const auto& l : loans) {
        if (l.accountNo == accountNo && (l.status == "approved" || l.status == "active" || l.status == "pending")) {
            return true;
        }
    }
    return false;
}

bool isAlphabetic(const string& s) {
    if (s.empty()) return false;
    for (char c : s) {
        if (!isalpha(c) && c != ' ') return false;
    }
    return true;
}

int findAccountIndex(int accountNo, const vector<Account>& accounts) {
    for (size_t i = 0; i < accounts.size(); i++) {
        if (accounts[i].accountNo == accountNo) return (int)i;
    }
    return -1;
}

int findAccountByCNIC(const string& cnic, const vector<Account>& accounts) {
    string cleaned;
    for (char c : cnic) if (c != '-' && c != ' ') cleaned += c;
    for (size_t i = 0; i < accounts.size(); i++) {
        string accCnicClean;
        for (char c : accounts[i].cnic) if (c != '-' && c != ' ') accCnicClean += c;
        if (accCnicClean == cleaned) return (int)i;
    }
    return -1;
}
