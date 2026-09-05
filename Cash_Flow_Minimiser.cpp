// CASH FLOW MINIMIZER — v2
// Minimizes the number of cash transactions required to settle debts among
// multiple banks, using a heap-optimized greedy algorithm over net balances.
//
// New in v2 (vs. the original single-file version):
//   1. long long everywhere for monetary values (overflow safety)
//   2. Input validation (duplicate bank names, negative amounts, unknown banks)
//   3. Heap-based creditor/debtor selection: O(log V) per round instead of O(V)
//      -> total settlement complexity improves from O(V^2 * M) to O(V log V * M)
//   4. Cycle detection on the RAW transaction graph (before netting) via
//      3-color DFS -- flags circular debt chains for audit purposes.
//
// Build:  g++ -std=c++17 -O2 -o cash_flow_minimizer cash_flow_minimizer_v2.cpp
// Run:    ./cash_flow_minimizer

#include <iostream>
#include <vector>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <algorithm>
#include <queue>
#include <tuple>
#include <climits>
using namespace std;

// ------------------------------------------------------------------------
// Data model
// ------------------------------------------------------------------------

struct Bank {
    string name;
    long long netAmount = 0;
    unordered_set<string> paymentTypes;
};

// Generic Pair used to store (settlement amount, payment mode) per edge.
template <typename K, typename V>
class Pair {
    K key;
    V value;

public:
    Pair(K k = K(), V v = V()) : key(k), value(v) {}
    K getKey() const { return key; }
    V getValue() const { return value; }
};

// ------------------------------------------------------------------------
// Cycle detection (3-color DFS) on the raw transaction graph
// WHITE = 0 (unvisited), GRAY = 1 (in progress), BLACK = 2 (done)
// A back-edge to a GRAY node means a circular debt chain exists.
// This does NOT affect correctness of netting (netting handles cycles
// mathematically), but it's valuable for audit/fraud-flagging purposes.
// ------------------------------------------------------------------------

bool dfsHasCycle(int node, const vector<vector<int>>& adj, vector<int>& color) {
    color[node] = 1; // GRAY
    for (int next : adj[node]) {
        if (color[next] == 1) return true;                    // back edge -> cycle
        if (color[next] == 0 && dfsHasCycle(next, adj, color)) return true;
    }
    color[node] = 2; // BLACK
    return false;
}

bool detectCycle(int numBanks, const vector<vector<int>>& adj) {
    vector<int> color(numBanks, 0);
    for (int i = 0; i < numBanks; ++i) {
        if (color[i] == 0 && dfsHasCycle(i, adj, color)) return true;
    }
    return false;
}

// ------------------------------------------------------------------------
// Utility: find a payment mode both banks support
// ------------------------------------------------------------------------

string getCommonPaymentMode(const Bank& a, const Bank& b) {
    for (const auto& mode : a.paymentTypes) {
        if (b.paymentTypes.count(mode)) return mode;
    }
    return "";
}

// ------------------------------------------------------------------------
// Heap-optimized greedy settlement
//
// Instead of scanning all V banks every round to find the max creditor and
// max debtor (O(V) per round, O(V^2) total), we maintain two max-heaps:
//   - maxHeap:  (netAmount, index)   for creditors (netAmount > 0)
//   - minHeap:  (-netAmount, index)  for debtors   (netAmount < 0),
//               negated so the "most negative" balance sits at the top
//
// Because balances are mutated in place, heap entries can go stale (a bank's
// netAmount may change after it was pushed). We use LAZY DELETION: when we
// pop an entry, we check it against the bank's current netAmount and simply
// discard it if it no longer matches, instead of trying to update-in-place.
//
// Each bank can be pushed/popped a bounded number of times across the run
// (at most once per settlement round it participates in), giving an overall
// complexity of O(V log V * M), where M is the cost of the payment-mode
// lookup per settlement.
// ------------------------------------------------------------------------

void minimizeCashFlow(vector<Bank>& banks,
                       vector<vector<Pair<long long, string>>>& resultGraph) {
    int n = static_cast<int>(banks.size());

    priority_queue<pair<long long, int>> maxHeap; // creditors
    priority_queue<pair<long long, int>> minHeap; // debtors (negated)

    for (int i = 0; i < n; ++i) {
        if (banks[i].netAmount > 0) maxHeap.push({banks[i].netAmount, i});
        else if (banks[i].netAmount < 0) minHeap.push({-banks[i].netAmount, i});
    }

    while (!maxHeap.empty() && !minHeap.empty()) {
        auto [credVal, credIdx] = maxHeap.top();
        if (credVal != banks[credIdx].netAmount) { maxHeap.pop(); continue; } // stale
        auto [debtVal, debtIdx] = minHeap.top();
        if (debtVal != -banks[debtIdx].netAmount) { minHeap.pop(); continue; } // stale

        maxHeap.pop();
        minHeap.pop();

        long long amount = min(credVal, debtVal);
        string mode = getCommonPaymentMode(banks[credIdx], banks[debtIdx]);
        if (mode.empty()) mode = "WorldBank"; // intermediary fallback

        resultGraph[debtIdx][credIdx] = Pair<long long, string>(amount, mode);

        banks[credIdx].netAmount -= amount;
        banks[debtIdx].netAmount += amount;

        if (banks[credIdx].netAmount > 0) maxHeap.push({banks[credIdx].netAmount, credIdx});
        if (banks[debtIdx].netAmount < 0) minHeap.push({-banks[debtIdx].netAmount, debtIdx});
    }
}

// ------------------------------------------------------------------------
// Output
// ------------------------------------------------------------------------

void printTransactions(const vector<vector<Pair<long long, string>>>& resultGraph,
                        const vector<Bank>& banks) {
    cout << "\nMinimized Transactions:\n";
    bool any = false;
    for (size_t i = 0; i < resultGraph.size(); ++i) {
        for (size_t j = 0; j < resultGraph.size(); ++j) {
            if (resultGraph[i][j].getKey() > 0) {
                any = true;
                cout << banks[i].name << " pays Rs " << resultGraph[i][j].getKey()
                     << " to " << banks[j].name << " via " << resultGraph[i][j].getValue() << "\n";
            }
        }
    }
    if (!any) cout << "All balances are already settled. No transactions needed.\n";
}

// ------------------------------------------------------------------------
// Input helpers (with validation)
// ------------------------------------------------------------------------

long long readPositiveLL(const string& prompt) {
    long long val;
    while (true) {
        cout << prompt;
        cin >> val;
        if (cin.fail()) {
            cin.clear();
            cin.ignore(INT_MAX, '\n');
            cout << "Invalid number, try again.\n";
            continue;
        }
        if (val <= 0) {
            cout << "Value must be positive, try again.\n";
            continue;
        }
        return val;
    }
}

int main() {
    int numBanks = static_cast<int>(readPositiveLL("Enter number of banks: "));

    vector<Bank> banks(numBanks);
    unordered_map<string, int> nameToIndex;

    for (int i = 0; i < numBanks; ++i) {
        string name;
        while (true) {
            cout << "\nBank " << i + 1 << " name: ";
            cin >> name;
            if (nameToIndex.count(name)) {
                cout << "Bank name already used. Please enter a unique name.\n";
                continue;
            }
            break;
        }
        banks[i].name = name;
        nameToIndex[name] = i;

        int numModes = 0;
        cout << "Enter number of payment modes: ";
        cin >> numModes;
        if (numModes < 0) numModes = 0;

        if (numModes > 0) {
            cout << "Enter payment modes: ";
            for (int j = 0; j < numModes; ++j) {
                string mode;
                cin >> mode;
                banks[i].paymentTypes.insert(mode);
            }
        }
    }

    int numTransactions = static_cast<int>(readPositiveLL("\nEnter number of transactions: "));

    // adjacency list for raw-transaction cycle detection (audit purposes)
    vector<vector<int>> adj(numBanks);

    for (int i = 0; i < numTransactions; ++i) {
        string from, to;
        long long amount;
        cout << "Transaction " << i + 1 << " (from to amount): ";
        cin >> from >> to >> amount;

        if (!nameToIndex.count(from) || !nameToIndex.count(to)) {
            cout << "Invalid bank name(s) — skipping this transaction.\n";
            continue;
        }
        if (from == to) {
            cout << "A bank cannot transact with itself — skipping.\n";
            continue;
        }
        if (amount <= 0) {
            cout << "Amount must be positive — skipping.\n";
            continue;
        }

        int fromIdx = nameToIndex[from];
        int toIdx = nameToIndex[to];

        banks[fromIdx].netAmount -= amount;
        banks[toIdx].netAmount += amount;

        adj[fromIdx].push_back(toIdx);
    }

    if (detectCycle(numBanks, adj)) {
        cout << "\n[Audit Warning] Circular debt chain detected in the raw transaction graph.\n"
             << "Netting will still settle correctly, but you may want to review these "
             << "transactions for anomalies.\n";
    }

    vector<vector<Pair<long long, string>>> resultGraph(
        numBanks, vector<Pair<long long, string>>(numBanks));

    minimizeCashFlow(banks, resultGraph);
    printTransactions(resultGraph, banks);

    return 0;
}
