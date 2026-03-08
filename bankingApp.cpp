#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>
#include <iomanip>
#include <ctime>
using namespace std;
// ===========================================================
//  BITWISE PERMISSION FLAGS
//  All 4 flags packed into one unsigned int:
//    Bit 0  (value 1) = Can Withdraw
//    Bit 1  (value 2) = Can Deposit
//    Bit 2  (value 4) = Can Transfer
//    Bit 3  (value 8) = VIP Account
// ===========================================================
#define PERM_WITHDRAW  1    // 0001
#define PERM_DEPOSIT   2    // 0010
#define PERM_TRANSFER  4    // 0100
#define PERM_VIP       8    // 1000

// Get current month index (0=Jan … 11=Dec) for monthly tracking
int currentMonth() {
    time_t t = time(0);
    tm* now = localtime(&t);
    return now->tm_mon;   // 0–11
}

// ===========================================================
//  MONTHLY SUMMARY ARRAYS  (requirement: use arrays)
//  Index 0 = January … Index 11 = December
// ===========================================================
double monthlyDeposits[12]    = {0};
double monthlyWithdrawals[12] = {0};

// Record amount into the correct month slot
void recordMonthly(double amount, int monthIndex) {
    if (monthIndex < 0 || monthIndex > 11) return;
    if (amount > 0)
        monthlyDeposits[monthIndex]    += amount;    // array indexing
    else
        monthlyWithdrawals[monthIndex] += (-amount); // array indexing
}

void showMonthlySummary() {
    string months[12] = {
        "January  ", "February ", "March    ", "April    ",
        "May      ", "June     ", "July     ", "August   ",
        "September", "October  ", "November ", "December "
    };

    // monthlyTotals: net change per month  (array as required)
    double monthlyTotals[12];
    for (int i = 0; i < 12; i++)
        monthlyTotals[i] = monthlyDeposits[i] - monthlyWithdrawals[i];

    cout << "\n  ============================================\n";
    cout << "              MONTHLY SUMMARY               \n";
    cout << "  ============================================\n";
    cout << "  " << left
         << setw(12) << "Month"
         << setw(14) << "Deposits"
         << setw(14) << "Withdrawals"
         << "Net Change\n";
    cout << "  --------------------------------------------\n";
    for (int i = 0; i < 12; i++) {
        cout << "  " << left
             << setw(12) << months[i]
             << setw(14) << fixed << setprecision(2) << monthlyDeposits[i]
             << setw(14) << fixed << setprecision(2) << monthlyWithdrawals[i]
             << (monthlyTotals[i] >= 0 ? "+" : "")
             << fixed << setprecision(2) << monthlyTotals[i] << "\n";
    }
    cout << "  ============================================\n";
}


// ===========================================================
//  ABSTRACT BASE CLASS:  Account
// ===========================================================
class Account {
protected:
    int            accountId;
    string         name;
    double         balance;
    unsigned int   permissions;       // bitwise permission flags
    vector<double> transactions;      // +deposit  /  -withdrawal

public:
    Account(int id, const string& n, double bal, unsigned int perms)
        : accountId(id), name(n), balance(bal), permissions(perms) {}

    // ---- Pure virtual functions ----
    virtual void deposit(double amount)  = 0;
    virtual void withdraw(double amount) = 0;
    virtual void saveToFile()            = 0;
    virtual void loadFromFile()          = 0;

    // ---- Getters ----
    int            getId()          const { return accountId;   }
    string         getName()        const { return name;        }
    double         getBalance()     const { return balance;     }
    unsigned int   getPermissions() const { return permissions; }
    vector<double>& getTransactions()     { return transactions;}

    // ---- Permission checks using Bitwise AND ----
    bool canWithdraw() const { return (permissions & PERM_WITHDRAW) != 0; }
    bool canDeposit()  const { return (permissions & PERM_DEPOSIT)  != 0; }
    bool canTransfer() const { return (permissions & PERM_TRANSFER) != 0; }
    bool isVIP()       const { return (permissions & PERM_VIP)      != 0; }

    // ---- Print all transaction history ----
    void printTransactions() const {
        if (transactions.empty()) {
            cout << "     (no transactions yet)\n";
            return;
        }
        for (int i = 0; i < (int)transactions.size(); i++) {
            double t = transactions[i];
            if (t >= 0)
                cout << "     [" << i+1 << "] Deposit    : +Rs."
                     << fixed << setprecision(2) <<  t << "\n";
            else
                cout << "     [" << i+1 << "] Withdrawal : -Rs."
                     << fixed << setprecision(2) << -t << "\n";
        }
    }

    // Virtual destructor — needed for correct polymorphic delete
    virtual ~Account() {}
};


// ===========================================================
//  DERIVED CLASS 1:  SavingsAccount
// ===========================================================
class SavingsAccount : public Account {
private:
    double interestRate;

public:
    SavingsAccount(int id, const string& n, double bal,
                   unsigned int perms, double rate = 0.05)
        : Account(id, n, bal, perms), interestRate(rate) {}

    // ---- Deposit — checks PERM_DEPOSIT bit with Bitwise AND ----
    void deposit(double amount) override {
        if (!(permissions & PERM_DEPOSIT)) {
            cout << "  [DENIED] Deposit permission not granted.\n";
            return;
        }
        if (amount <= 0) {
            cout << "  [ERROR] Amount must be positive.\n";
            return;
        }

        balance += amount;                   // update balance
        transactions.push_back(+amount);     // record as positive in vector

        // Auto-record into this month's slot in the global array
        recordMonthly(+amount, currentMonth());

        cout << "  [OK] Deposited Rs." << fixed << setprecision(2) << amount << "\n";
        cout << "  New Balance : Rs." << fixed << setprecision(2) << balance << "\n";
    }

    // ---- Withdraw — checks PERM_WITHDRAW bit with Bitwise AND ----
    void withdraw(double amount) override {
        if (!(permissions & PERM_WITHDRAW)) {
            cout << "  [DENIED] Withdraw permission not granted.\n";
            return;
        }
        if (amount <= 0) {
            cout << "  [ERROR] Amount must be positive.\n";
            return;
        }
        if (amount > balance) {
            cout << "  [ERROR] Insufficient funds.\n";
            cout << "  Current Balance: Rs." << fixed << setprecision(2) << balance << "\n";
            return;
        }

        balance -= amount;                   // update balance
        transactions.push_back(-amount);     // record as negative in vector

        recordMonthly(-amount, currentMonth());

        cout << "  [OK] Withdrew  Rs." << fixed << setprecision(2) << amount << "\n";
        cout << "  New Balance : Rs." << fixed << setprecision(2) << balance << "\n";
    }

    // ---- Display full account details ----
    void display() const {
        cout << "\n  +------------------------------------+\n";
        cout << "  |        SAVINGS ACCOUNT             |\n";
        cout << "  +------------------------------------+\n";
        cout << "  ID            : " << accountId << "\n";
        cout << "  Name          : " << name      << "\n";
        cout << "  Balance       : Rs." << fixed << setprecision(2) << balance << "\n";
        cout << "  Interest Rate : " << fixed << setprecision(1)
             << interestRate * 100 << "% per year\n";
        cout << "  VIP Status    : " << (isVIP() ? "YES" : "No") << "\n";
        cout << "  Permissions   : ";
        if (canDeposit())  cout << "[Deposit] ";
        if (canWithdraw()) cout << "[Withdraw] ";
        if (canTransfer()) cout << "[Transfer] ";
        cout << "\n";
        cout << "  ------------------------------------\n";
        cout << "  Transaction History:\n";
        printTransactions();
        cout << "  +------------------------------------+\n";
    }

    // ---- File save (via stream) ----
    void saveToStream(ofstream& f) const {
        f << "ACCOUNT Savings\n";
        f << accountId << " " << name << " "
          << fixed << setprecision(2) << balance << " "
          << permissions << " " << interestRate << "\n";
        f << "TRANSACTIONS\n";
        for (int i = 0; i < (int)transactions.size(); i++)
            f << transactions[i] << "\n";
        f << "END\n";
    }

    void saveToFile()   override {}   // handled via saveToStream
    void loadFromFile() override {}   // handled in BankSystem loadAll()

    double getInterestRate() const { return interestRate; }
};


// ===========================================================
//  DERIVED CLASS 2:  CurrentAccount
// ===========================================================
class CurrentAccount : public Account {
private:
    double overdraftLimit;

public:
    CurrentAccount(int id, const string& n, double bal,
                   unsigned int perms, double overdraft = 5000.0)
        : Account(id, n, bal, perms), overdraftLimit(overdraft) {}

    // ---- Deposit ----
    void deposit(double amount) override {
        if (!(permissions & PERM_DEPOSIT)) {
            cout << "  [DENIED] Deposit permission not granted.\n";
            return;
        }
        if (amount <= 0) {
            cout << "  [ERROR] Amount must be positive.\n";
            return;
        }

        balance += amount;
        transactions.push_back(+amount);
        recordMonthly(+amount, currentMonth());

        cout << "  [OK] Deposited Rs." << fixed << setprecision(2) << amount << "\n";
        cout << "  New Balance : Rs." << fixed << setprecision(2) << balance << "\n";
    }

    // ---- Withdraw (supports overdraft) ----
    void withdraw(double amount) override {
        if (!(permissions & PERM_WITHDRAW)) {
            cout << "  [DENIED] Withdraw permission not granted.\n";
            return;
        }
        if (amount <= 0) {
            cout << "  [ERROR] Amount must be positive.\n";
            return;
        }
        if (amount > balance + overdraftLimit) {
            cout << "  [ERROR] Exceeds overdraft limit.\n";
            cout << "  Max available: Rs." << fixed << setprecision(2)
                 << (balance + overdraftLimit) << "\n";
            return;
        }

        balance -= amount;
        transactions.push_back(-amount);
        recordMonthly(-amount, currentMonth());

        cout << "  [OK] Withdrew  Rs." << fixed << setprecision(2) << amount << "\n";
        cout << "  New Balance : Rs." << fixed << setprecision(2) << balance << "\n";
        if (balance < 0)
            cout << "  [NOTE] Account is in overdraft!\n";
    }

    // ---- Display ----
    void display() const {
        cout << "\n  +------------------------------------+\n";
        cout << "  |        CURRENT ACCOUNT             |\n";
        cout << "  +------------------------------------+\n";
        cout << "  ID            : " << accountId << "\n";
        cout << "  Name          : " << name      << "\n";
        cout << "  Balance       : Rs." << fixed << setprecision(2) << balance << "\n";
        cout << "  Overdraft Lmt : Rs." << fixed << setprecision(2) << overdraftLimit << "\n";
        cout << "  VIP Status    : " << (isVIP() ? "YES" : "No") << "\n";
        cout << "  Permissions   : ";
        if (canDeposit())  cout << "[Deposit] ";
        if (canWithdraw()) cout << "[Withdraw] ";
        if (canTransfer()) cout << "[Transfer] ";
        cout << "\n";
        cout << "  ------------------------------------\n";
        cout << "  Transaction History:\n";
        printTransactions();
        cout << "  +------------------------------------+\n";
    }

    void saveToStream(ofstream& f) const {
        f << "ACCOUNT Current\n";
        f << accountId << " " << name << " "
          << fixed << setprecision(2) << balance << " "
          << permissions << " " << overdraftLimit << "\n";
        f << "TRANSACTIONS\n";
        for (int i = 0; i < (int)transactions.size(); i++)
            f << transactions[i] << "\n";
        f << "END\n";
    }

    void saveToFile()   override {}
    void loadFromFile() override {}

    double getOverdraftLimit() const { return overdraftLimit; }
};


// ===========================================================
//  BANK SYSTEM  —  vector<Account*> stores all accounts
// ===========================================================
vector<Account*> accounts;
int nextId = 1001;

// Find account by ID — returns pointer or nullptr
Account* findById(int id) {
    for (int i = 0; i < (int)accounts.size(); i++)
        if (accounts[i]->getId() == id)
            return accounts[i];
    return nullptr;
}

// Print a summary table of all accounts
void showAccountList() {
    if (accounts.empty()) {
        cout << "  No accounts found. Create one first.\n";
        return;
    }
    cout << "\n  " << left
         << setw(8)  << "ID"
         << setw(20) << "Name"
         << setw(12) << "Type"
         << "Balance\n";
    cout << "  " << string(52, '-') << "\n";
    for (int i = 0; i < (int)accounts.size(); i++) {
        Account* a = accounts[i];
        string type = dynamic_cast<CurrentAccount*>(a) ? "Current" : "Savings";
        cout << "  " << left
             << setw(8)  << a->getId()
             << setw(20) << a->getName()
             << setw(12) << type
             << "Rs." << fixed << setprecision(2) << a->getBalance() << "\n";
    }
    cout << "  " << string(52, '-') << "\n";
}

// Free all dynamically allocated memory
void freeAll() {
    for (int i = 0; i < (int)accounts.size(); i++)
        delete accounts[i];   // virtual destructor runs correctly
    accounts.clear();
}


// ===========================================================
//  MENU FUNCTIONS
// ===========================================================

void createAccount() {
    cout << "\n  Account Type:\n"
         << "  1. Savings Account\n"
         << "  2. Current Account\n"
         << "  Choice: ";
    int type; cin >> type;
    cin.ignore();   // clear newline so getline works

    cout << "  Owner Name   : ";
    string name;
    getline(cin, name);   // getline reads full name with spaces

    cout << "  Initial Balance: Rs.";
    double bal; cin >> bal;

    cout << "  VIP? (1=Yes  0=No): ";
    int vip; cin >> vip;

    // Build permissions with Bitwise OR
    // 1 | 2 | 4 = 7  (Withdraw + Deposit + Transfer)
    unsigned int perms = PERM_WITHDRAW | PERM_DEPOSIT | PERM_TRANSFER;
    if (vip == 1)
        perms = perms | PERM_VIP;   // add bit 3 → value becomes 15

    Account* acc = nullptr;
    if (type == 1) {
        cout << "  Interest Rate (e.g. 0.05 for 5%): ";
        double rate; cin >> rate;
        acc = new SavingsAccount(nextId++, name, bal, perms, rate);
    } else {
        cout << "  Overdraft Limit: Rs.";
        double overdraft; cin >> overdraft;
        acc = new CurrentAccount(nextId++, name, bal, perms, overdraft);
    }

    accounts.push_back(acc);   // store pointer in vector<Account*>

    cout << "\n  [OK] Account created!\n";
    cout << "  Account ID      : " << acc->getId() << "\n";
    cout << "  Permissions Code: " << perms
         << "  (" << (vip ? "VIP | " : "") << "Transfer | Deposit | Withdraw)\n";
}

// ---- Deposit — NO extra prompts, just ID and amount ----
void doDeposit() {
    showAccountList();
    if (accounts.empty()) return;

    cout << "  Enter Account ID: ";
    int id; cin >> id;

    Account* acc = findById(id);
    if (!acc) {
        cout << "  [ERROR] Account ID " << id << " not found.\n";
        return;
    }

    cout << "  Amount: Rs.";
    double amt; cin >> amt;

    // Polymorphic call — runs SavingsAccount::deposit or CurrentAccount::deposit
    // Balance is updated inside the object. Show Account will reflect the change.
    acc->deposit(amt);
}

// ---- Withdraw — NO extra prompts, just ID and amount ----
void doWithdraw() {
    showAccountList();
    if (accounts.empty()) return;

    cout << "  Enter Account ID: ";
    int id; cin >> id;

    Account* acc = findById(id);
    if (!acc) {
        cout << "  [ERROR] Account ID " << id << " not found.\n";
        return;
    }

    cout << "  Amount: Rs.";
    double amt; cin >> amt;

    // Polymorphic call — balance is updated inside the object
    acc->withdraw(amt);
}

// ---- Show Account — reads updated balance from the same object ----
void doShowAccount() {
    showAccountList();
    if (accounts.empty()) return;

    cout << "  Enter Account ID: ";
    int id; cin >> id;

    Account* acc = findById(id);
    if (!acc) {
        cout << "  [ERROR] Account ID " << id << " not found.\n";
        return;
    }

    // Polymorphic display — calls correct display() based on actual type
    SavingsAccount* sa = dynamic_cast<SavingsAccount*>(acc);
    CurrentAccount* ca = dynamic_cast<CurrentAccount*>(acc);
    if (sa) sa->display();
    else if (ca) ca->display();
}


// ===========================================================
//  FILE HANDLING — Save all accounts to accounts.txt
//  Format:
//    ACCOUNT Savings
//    1001 Ali 5000.00 7 0.05
//    TRANSACTIONS
//    1000.00
//    -500.00
//    END
// ===========================================================
void saveAll() {
    ofstream f("accounts.txt");
    if (!f) { cout << "  [ERROR] Cannot open file.\n"; return; }

    for (int i = 0; i < (int)accounts.size(); i++) {
        SavingsAccount* sa = dynamic_cast<SavingsAccount*>(accounts[i]);
        CurrentAccount* ca = dynamic_cast<CurrentAccount*>(accounts[i]);
        if (sa) sa->saveToStream(f);
        else if (ca) ca->saveToStream(f);
    }

    f.close();
    cout << "  [OK] Saved " << accounts.size() << " account(s) to accounts.txt\n";

    // Show what was written so students can see the format
    cout << "\n  --- File Contents ---\n";
    ifstream rf("accounts.txt");
    string line;
    while (getline(rf, line))
        cout << "  " << line << "\n";
    rf.close();
    cout << "  --------------------\n";
}


// ===========================================================
//  FILE HANDLING — Load all accounts from accounts.txt
// ===========================================================
void loadAll() {
    ifstream f("accounts.txt");
    if (!f) { cout << "  [ERROR] Cannot open accounts.txt — save first.\n"; return; }

    freeAll();   // delete existing accounts before loading

    string line;
    while (getline(f, line)) {
        if (line.find("ACCOUNT") == string::npos) continue;

        string accountType = line.substr(8);   // "Savings" or "Current"

        // Read: id, then name words, then balance (first number we hit)
        int    id;
        string name = "";
        double balance, extra;
        unsigned int perms;

        f >> id;

        // Read name token by token until we hit the balance (a number)
        string token;
        while (f >> token) {
            istringstream iss(token);
            double test;
            if (iss >> test && iss.eof()) {
                balance = test;   // this token was the balance
                break;
            }
            if (!name.empty()) name += " ";
            name += token;
        }

        f >> perms >> extra;
        f.ignore();

        getline(f, line);   // skip "TRANSACTIONS" header line

        // Read transaction values until "END"
        vector<double> txns;
        while (getline(f, line) && line != "END") {
            if (line.empty()) continue;
            istringstream ts(line);
            double t; ts >> t;
            txns.push_back(t);
        }

        // Reconstruct the correct derived class object
        Account* acc = nullptr;
        if (accountType == "Savings")
            acc = new SavingsAccount(id, name, balance, perms, extra);
        else
            acc = new CurrentAccount(id, name, balance, perms, extra);

        // Restore transaction history into the object's vector
        for (int i = 0; i < (int)txns.size(); i++)
            acc->getTransactions().push_back(txns[i]);

        accounts.push_back(acc);
        if (id >= nextId) nextId = id + 1;
    }

    f.close();
    cout << "  [OK] Loaded " << accounts.size() << " account(s).\n";
    showAccountList();
}


// ===========================================================
//  MAIN  Menu Loop
// ===========================================================
int main() {
    cout << "\n  ==========================================\n";
    cout << "      SECURE BANKING TRANSACTION SYSTEM    \n";
    cout << "  ==========================================\n";

    int choice;
    do {
        cout << "\n  ------------------------------------------\n"
             << "  1.  Create Account\n"
             << "  2.  Deposit\n"
             << "  3.  Withdraw\n"
             << "  4.  Show Account\n"
             << "  5.  List All Accounts\n"
             << "  6.  Save to File\n"
             << "  7.  Load from File\n"
             << "  8.  Monthly Summary\n"
             << "  0.  Exit\n"
             << "  ------------------------------------------\n"
             << "  Choice: ";

        if (!(cin >> choice)) {
            cin.clear();
            cin.ignore(1000, '\n');
            cout << "  Please enter a number.\n";
            continue;
        }

        switch (choice) {
            case 1: createAccount();      break;
            case 2: doDeposit();          break;
            case 3: doWithdraw();         break;
            case 4: doShowAccount();      break;
            case 5: showAccountList();    break;
            case 6: saveAll();            break;
            case 7: loadAll();            break;
            case 8: showMonthlySummary(); break;
            case 0: cout << "  Goodbye!\n"; break;
            default: cout << "  Invalid option. Enter 0–8.\n";
        }

    } while (choice != 0);

    freeAll();   // delete all Account* to prevent memory leaks
    return 0;
}