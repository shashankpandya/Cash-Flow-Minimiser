#include <iostream>     // For input/output operations (cin, cout)
#include <vector>       // For dynamic arrays (std::vector)
#include <string>       // For string manipulation (std::string)
#include <unordered_map> // For efficient key-value lookups (std::unordered_map)
#include <unordered_set> // For efficient storage of unique payment types (std::unordered_set)
#include <algorithm>    // For std::min, std::max
#include <limits>       // For std::numeric_limits (e.g., min/max integer values)

// Using std namespace for convenience in this example.
// In larger projects, it's often better to qualify with std:: or use specific 'using' declarations.
using namespace std;

// --- Type Aliases for Clarity ---
// Represents a single minimized transaction: amount and the payment method used.
using TransactionDetail = pair<int, string>;

// --- Bank Class ---
// Encapsulates bank data and related operations.
class Bank
{
public:
    string name;                    // Name of the bank
    int netAmount;                  // Net balance of the bank (positive for creditor, negative for debtor)
    unordered_set<string> paymentTypes; // Set of payment methods supported by this bank

    // Constructor to initialize a bank with a name and zero net amount.
    // 'std::move' is used for efficient string transfer.
    Bank(string name_val = "") : name(std::move(name_val)), netAmount(0) {}

    // Adds a payment type to the bank's supported methods.
    void addPaymentType(const string& type) {
        paymentTypes.insert(type);
    }

    // Checks if the bank supports a specific payment type.
    // 'const' indicates this method does not modify the object.
    bool supportsPaymentType(const string& type) const {
        return paymentTypes.count(type) > 0;
    }

    // Overload the stream insertion operator for easy printing of Bank objects.
    friend ostream& operator<<(ostream& os, const Bank& bank) {
        os << bank.name << " (Net: " << bank.netAmount << ")";
        return os;
    }
};

// --- Utility Functions ---

// Finds the index of the bank with the largest positive net amount (creditor).
// Returns -1 if all banks have non-positive or zero net amounts.
int getCreditorIndex(const vector<Bank>& banks)
{
    int maxIdx = -1;
    int maxAmount = numeric_limits<int>::min(); // Initialize with the smallest possible integer value

    for (int i = 0; i < banks.size(); ++i)
    {
        if (banks[i].netAmount > maxAmount)
        {
            maxAmount = banks[i].netAmount;
            maxIdx = i;
        }
    }
    return maxIdx;
}

// Finds the index of the bank with the largest negative net amount (debtor).
// Returns -1 if all banks have non-negative or zero net amounts.
int getDebtorIndex(const vector<Bank>& banks)
{
    int minIdx = -1;
    int minAmount = numeric_limits<int>::max(); // Initialize with the largest possible integer value

    for (int i = 0; i < banks.size(); ++i)
    {
        if (banks[i].netAmount < minAmount)
        {
            minAmount = banks[i].netAmount;
            minIdx = i;
        }
    }
    return minIdx;
}

// Finds a common payment mode between two given banks.
// Iterates through the payment types of the first bank and checks if the second bank supports it.
// Returns the first common mode found, or an empty string if none exist.
string findCommonPaymentMode(const Bank& bank1, const Bank& bank2)
{
    for (const auto& mode : bank1.paymentTypes)
    {
        if (bank2.supportsPaymentType(mode))
        {
            return mode;
        }
    }
    return ""; // No common payment mode found
}

// --- Core Algorithm ---

// Minimizes the cash flow transactions between banks using a greedy approach.
// It repeatedly finds the largest creditor and largest debtor, makes a transaction
// to settle as much as possible, and updates their net amounts.
// 'banks' vector's net amounts are modified.
// 'resultGraph' is populated with the minimized transactions.
void minimizeCashFlow(vector<Bank>& banks, vector<vector<TransactionDetail>>& resultGraph)
{
    while (true)
    {
        int creditorIdx = getCreditorIndex(banks);
        int debtorIdx = getDebtorIndex(banks);

        // Termination condition: if both the largest creditor and largest debtor have a net amount of 0,
        // it means all balances are settled.
        if (banks[creditorIdx].netAmount == 0 && banks[debtorIdx].netAmount == 0)
            break;

        // If either the largest creditor or debtor has a zero balance, it implies that all
        // balances have been settled for that side, or there's an imbalance in the system
        // (which should ideally be caught by `totalNet` check in main).
        // This condition prevents infinite loops if there's a theoretical imbalance.
        if (banks[creditorIdx].netAmount == 0 || banks[debtorIdx].netAmount == 0) {
            break;
        }

        // The amount to be transacted is the minimum of the creditor's positive balance
        // and the absolute value of the debtor's negative balance.
        int amount = min(banks[creditorIdx].netAmount, -banks[debtorIdx].netAmount);

        // Determine the payment mode for this transaction.
        string mode = findCommonPaymentMode(banks[creditorIdx], banks[debtorIdx]);

        // If no common payment mode exists, an intermediary (like a central bank) is used.
        if (mode.empty()) {
            mode = "WorldBank (Intermediary)"; // More descriptive fallback
        }

        // Record the transaction: debtor (index `debtorIdx`) pays creditor (index `creditorIdx`).
        resultGraph[debtorIdx][creditorIdx] = {amount, mode};

        // Update the net amounts of the involved banks.
        // Creditor's balance decreases, Debtor's balance increases.
        banks[creditorIdx].netAmount -= amount;
        banks[debtorIdx].netAmount += amount;
    }
}

// --- Output Function ---

// Displays the minimized transactions in a user-friendly format.
void printTransactions(const vector<vector<TransactionDetail>>& resultGraph, const vector<Bank>& banks)
{
    cout << "\n--- Minimized Transactions ---\n";
    bool anyTransactions = false; // Flag to check if any transactions were found

    for (int i = 0; i < resultGraph.size(); ++i)
    {
        for (int j = 0; j < resultGraph.size(); ++j)
        {
            // A transaction is recorded if the amount is positive.
            if (resultGraph[i][j].first > 0)
            {
                cout << banks[i].name << " pays Rs " << resultGraph[i][j].first
                     << " to " << banks[j].name << " via " << resultGraph[i][j].second << ".\n";
                anyTransactions = true;
            }
        }
    }

    if (!anyTransactions) {
        cout << "No transactions needed. All balances are already settled.\n";
    }
    cout << "------------------------------\n";
}

// --- Input Helper Function ---

// Prompts the user for a non-negative integer and handles invalid input.
// Clears error flags and discards remaining input on failure.
int getValidIntInput(const string& prompt) {
    int value;
    while (true) {
        cout << prompt;
        cin >> value;
        // Check for failed extraction (e.g., non-numeric input) or negative value
        if (cin.fail() || value < 0) {
            cout << "Invalid input. Please enter a non-negative integer.\n";
            cin.clear(); // Clear error flags from cin
            // Discard remaining characters in the input buffer up to the newline
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
        } else {
            // Discard any remaining characters on the line (like a leftover newline from previous input)
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            return value;
        }
    }
}

// --- Main Program ---

int main()
{
    cout << "--- Cash Flow Minimizer ---\n";

    // Get the number of banks from the user.
    int numBanks = getValidIntInput("Enter the number of banks: ");
    if (numBanks <= 0) {
        cout << "Error: The number of banks must be positive. Exiting.\n";
        return 1; // Indicate an error
    }

    // Vector to store Bank objects.
    vector<Bank> banks(numBanks);
    // Map to quickly find a bank's index by its name.
    unordered_map<string, int> nameToIndex;

    // --- Input Bank Details ---
    cout << "\n--- Input Bank Details ---\n";
    for (int i = 0; i < numBanks; ++i)
    {
        cout << "\nDetails for Bank " << i + 1 << ":\n";
        string bankName;
        while (true) {
            cout << "  Enter Bank name (e.g., 'SBI', 'ICICI Bank'): ";
            getline(cin, bankName); // Use getline to allow bank names with spaces
            if (bankName.empty()) {
                cout << "  Bank name cannot be empty. Please try again.\n";
            } else if (nameToIndex.count(bankName)) {
                cout << "  Error: A bank with this name already exists. Please enter a unique name.\n";
            } else {
                banks[i].name = bankName;
                nameToIndex[bankName] = i;
                break;
            }
        }

        int numModes = getValidIntInput("  Enter the number of payment modes supported by " + banks[i].name + ": ");
        if (numModes > 0) {
            cout << "  Enter " << numModes << " payment mode(s) (e.g., 'Cash', 'CreditCard', 'UPI', 'BankTransfer'):\n";
            for (int j = 0; j < numModes; ++j)
            {
                string mode;
                cout << "    Mode " << j + 1 << ": ";
                getline(cin, mode);
                if (!mode.empty()) {
                    banks[i].addPaymentType(mode);
                } else {
                    cout << "    Payment mode cannot be empty. Skipping this mode.\n";
                    j--; // Decrement to re-prompt for this mode
                }
            }
        } else {
             cout << "  No payment modes entered for " << banks[i].name << ".\n";
        }
    }

    // --- Input Initial Transactions ---
    int numTransactions = getValidIntInput("\nEnter the number of initial transactions to process: ");

    cout << "\n--- Enter Initial Transactions ---\n";
    cout << "Format: FromBank ToBank Amount (e.g., 'SBI ICICI Bank 1000')\n";
    for (int i = 0; i < numTransactions; ++i)
    {
        string fromBankName, toBankName;
        int amount;

        cout << "\nTransaction " << i + 1 << ":\n";
        cout << "  From bank name: ";
        getline(cin, fromBankName);
        cout << "  To bank name: ";
        getline(cin, toBankName);
        amount = getValidIntInput("  Amount: ");

        // Validate bank names
        if (!nameToIndex.count(fromBankName))
        {
            cout << "  Error: Bank '" << fromBankName << "' not found. Skipping transaction.\n";
            continue; // Skip to the next transaction
        }
        if (!nameToIndex.count(toBankName))
        {
            cout << "  Error: Bank '" << toBankName << "' not found. Skipping transaction.\n";
            continue; // Skip to the next transaction
        }
        // Validate amount
        if (amount <= 0) {
             cout << "  Error: Transaction amount must be positive. Skipping transaction.\n";
             continue; // Skip to the next transaction
        }

        // Get indices for the involved banks
        int fromIdx = nameToIndex[fromBankName];
        int toIdx = nameToIndex[toBankName];

        // Update net amounts: 'fromBank' pays, so its balance decreases; 'toBank' receives, so its balance increases.
        banks[fromIdx].netAmount -= amount;
        banks[toIdx].netAmount += amount;
    }

    // --- Verification of Initial Balances ---
    // The sum of all net amounts across all banks should ideally be zero.
    long long totalNet = 0;
    for (const auto& bank : banks) {
        totalNet += bank.netAmount;
    }
    if (totalNet != 0) {
        cout << "\nWarning: The initial total net amount across all banks is not zero ("
             << totalNet << "). This might indicate an error in transaction input, "
             << "but the minimization will proceed based on the current balances.\n";
    }

    // --- Cash Flow Minimization ---
    // Initialize a 2D vector (matrix) to store the minimized transactions.
    // resultGraph[i][j] will store the amount and mode if bank 'i' pays bank 'j'.
    vector<vector<TransactionDetail>> resultGraph(numBanks, vector<TransactionDetail>(numBanks));

    // Execute the minimization algorithm.
    minimizeCashFlow(banks, resultGraph);

    // --- Display Results ---
    printTransactions(resultGraph, banks);

    return 0; // Indicate successful execution
}
