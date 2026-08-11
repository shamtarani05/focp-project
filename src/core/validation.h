#ifndef VALIDATION_H
#define VALIDATION_H

#include <string>
#include <vector>
#include "types.h"

using namespace std;

bool isValidPIN(const string& pin);
bool isValidAmount(double amount);
bool isPositiveAmount(double amount);
bool isUniqueAccountNo(int accountNo, const vector<Account>& accounts);
bool isValidCNIC(const string& cnic);
bool isValidName(const string& name);
bool isValidAccountType(const string& type);
bool hasSufficientBalance(int accountNo, double amount, const vector<Account>& accounts);
bool isAccountActive(int accountNo, const vector<Account>& accounts);
bool isWithinDailyLimit(int accountNo, double amount, const vector<Account>& accounts);
bool isTransferValid(int senderAcc, int receiverAcc, double amount, const vector<Account>& accounts);
bool isValidOTP(const string& input, const string& otp);
bool isValidLoanAmount(double amount, double balance);

bool isNumeric(const string& s);
bool isNumericDecimal(const string& s);
bool isMultipleOf500(double amount);
double getTotalCashInATM(const vector<CashNote>& inventory);
bool hasPendingReactivationRequest(int accountNo, const vector<ReactivationRequest>& requests);
bool hasActiveLoan(int accountNo, const vector<Loan>& loans);

int findAccountIndex(int accountNo, const vector<Account>& accounts);
int findAccountByCNIC(const string& cnic, const vector<Account>& accounts);

#endif
