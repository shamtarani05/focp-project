#ifndef FILEIO_H
#define FILEIO_H

#include <vector>
#include <string>
#include "types.h"

using namespace std;

bool ensureDirectoryExists(const string& dir);
bool ensureDataFilesExist();

bool loadAccounts(vector<Account>& accounts);
bool saveAccounts(const vector<Account>& accounts);
bool appendAccount(const Account& acc);
bool updateAccountInFile(const Account& acc);
bool deleteAccountFromFile(int accountNo);

bool loadTransactions(vector<Transaction>& transactions);
bool saveTransactions(const vector<Transaction>& transactions);
bool appendTransaction(const Transaction& t);

bool loadLoans(vector<Loan>& loans);
bool saveLoans(const vector<Loan>& loans);
bool appendLoan(const Loan& loan);
bool updateLoanInFile(const Loan& loan);

bool loadCashInventory(vector<CashNote>& inventory);
bool saveCashInventory(const vector<CashNote>& inventory);

bool appendAuditEntry(const AuditEntry& entry);
vector<AuditEntry> loadAuditEntries();

bool loadReactivationRequests(vector<ReactivationRequest>& requests);
bool saveReactivationRequests(const vector<ReactivationRequest>& requests);
bool appendReactivationRequest(const ReactivationRequest& req);


bool generateReceipt(const Transaction& t, const Account& acc);
bool backupData();

// Exporting Data & Reports
bool exportAccountsCSV(const string& filename = "data/accounts_export.csv");
bool exportTransactionsCSV(const string& filename = "data/transactions_export.csv");
bool exportAuditEntriesCSV(const string& filename = "data/audit_export.csv");
bool exportAccountStatementTXT(const Account& acc, const vector<Transaction>& txns, const string& filepath);

// Logging & Maintenance
bool writeSystemLog(const string& level, const string& message);
bool restoreBackup(const string& backupDir);
bool verifyDataIntegrity(string& reportOutput);

#endif
