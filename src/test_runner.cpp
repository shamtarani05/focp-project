#include <iostream>
#include <cassert>
#include <vector>
#include "core/types.h"
#include "core/fileio.h"
#include "core/validation.h"
#include "banking.h"

using namespace std;

int main() {
    cout << "[TEST] Starting full banking system verification..." << endl;

    // 1. Ensure directory & files
    cout << "[TEST 1] Testing file & directory initialization..." << endl;
    assert(ensureDataFilesExist() == true);
    cout << " -> Data files initialized successfully." << endl;

    // 2. Account operations
    cout << "[TEST 2] Testing account operations..." << endl;
    vector<Account> accounts;
    loadAccounts(accounts);
    int initialCount = accounts.size();

    Account testAcc;
    testAcc.accountNo = 99999;
    testAcc.name = "Test User";
    testAcc.cnic = "12345-6789012-3";
    testAcc.accountType = "savings";
    testAcc.balance = 15000.0;
    testAcc.status = "active";
    testAcc.pinHash = encodePIN("1234");
    testAcc.pinAttempts = 0;
    testAcc.dailyWithdrawn = 0.0;
    testAcc.lastWithdrawalDate = getCurrentDateStr();
    testAcc.creationDate = getCurrentDateStr();

    assert(appendAccount(testAcc) == true);
    loadAccounts(accounts);
    assert((int)accounts.size() == initialCount + 1);
    cout << " -> Account appended and loaded successfully. Total accounts: " << accounts.size() << endl;

    // 3. Validation checks
    cout << "[TEST 3] Testing validation logic..." << endl;
    assert(isValidPIN("1234") == true);
    assert(isValidPIN("123") == false);
    assert(isValidCNIC("12345-6789012-3") == true);
    assert(isValidAccountType("savings") == true);
    assert(isValidAccountType("invalid_type") == false);
    assert(isMultipleOf100(500.0) == true);
    assert(isMultipleOf100(550.0) == false);
    cout << " -> Validation routines passed." << endl;

    // 4. Transaction & Receipt
    cout << "[TEST 4] Testing transaction & receipt generation..." << endl;
    Transaction txn;
    txn.transactionID = "TXN_TEST_001";
    txn.accountNo = 99999;
    txn.type = "deposit";
    txn.amount = 5000.0;
    txn.dateTime = getCurrentDateTimeStr();
    txn.resultingBalance = 20000.0;
    txn.details = "Test deposit";

    assert(appendTransaction(txn) == true);
    assert(generateReceipt(txn, testAcc) == true);
    cout << " -> Transaction saved and receipt generated." << endl;

    // 5. CSV Exports & Statements
    cout << "[TEST 5] Testing CSV exports and statement generation..." << endl;
    assert(exportAccountsCSV("data/accounts_export.csv") == true);
    assert(exportTransactionsCSV("data/transactions_export.csv") == true);
    assert(exportAuditEntriesCSV("data/audit_export.csv") == true);
    
    vector<Transaction> txns;
    loadTransactions(txns);
    assert(exportAccountStatementTXT(testAcc, txns, "receipts/test_statement.txt") == true);
    cout << " -> CSV exports and statement file generated." << endl;

    // 6. Data Integrity & System Log
    cout << "[TEST 6] Testing data integrity check & logging..." << endl;
    string report;
    assert(verifyDataIntegrity(report) == true);
    assert(writeSystemLog("INFO", "Verification test completed successfully") == true);
    cout << " -> Integrity report:\n" << report << endl;

    // Clean up test account
    deleteAccountFromFile(99999);
    cout << " -> Test account cleaned up." << endl;

    cout << "\n=============================================" << endl;
    cout << "  ALL COMPONENT TESTS PASSED SUCCESSFULLY!  " << endl;
    cout << "=============================================" << endl;
    return 0;
}
