#ifndef BANKING_H
#define BANKING_H

#include <string>
#include <vector>
#include <ctime>
#include "core/types.h"

using namespace std;

// PIN security
string encodePIN(const string& pin);
string decodePIN(const string& encoded);

// OTP
string generateOTP();

// Audit
void   logAudit(const string& action, const string& details);
string getTimestamp();
string getCurrentDateStr();
string getCurrentDateTimeStr();

// Cash inventory
void initCashInventory(vector<CashNote>& inventory);
bool dispenseCash(vector<CashNote>& inventory, double amount);

// Savings profit
double calculateProfit(double balance, double rate, int days);

// Daily reset
void resetDailyWithdrawals(vector<Account>& accounts);

// Transaction filtering
vector<Transaction> filterTransactions(const vector<Transaction>& transactions,
                                       int accountNo, const string& type,
                                       const string& startDate, const string& endDate,
                                       double minAmount, double maxAmount);

// ID generation
int    getNextAccountNo(const vector<Account>& accounts);
string generateTransactionID(const vector<Transaction>& transactions);
int    getNextLoanID(const vector<Loan>& loans);

#endif
