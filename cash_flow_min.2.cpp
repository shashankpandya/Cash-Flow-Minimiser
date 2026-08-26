// Cash Flow Minimizer — Multilateral Netting Between Banks
// C++17 · Two solvers: greedy (fast) and exact subset-DP (minimum transfers)

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace {

using Amount = std::int64_t;

// ============================================================================
//  §1  Domain types
// ============================================================================

struct Bank {
    std::string name;
    std::unordered_set<std::string> supportedModes;

    Bank() = default;
    explicit Bank(std::string n) : name(std::move(n)) {}

    [[nodiscard]] bool supports(const std::string& m) const {
        return supportedModes.count(m) != 0;
    }
};

struct Debt {
    int debtor;
    int creditor;
    Amount amount;
};

struct Settlement {
    int payer;
    int payee;
    Amount amount;
    std::string mode;          // filled in after solving
};

// Net each debt into one balance per bank.
[[nodiscard]] std::vector<Amount> computeBalances(std::size_t n,
                                                   const std::vector<Debt>& debts) {
    std::vector<Amount> bal(n, 0);
    for (const auto& d : debts) {
        bal[d.debtor]  -= d.amount;
        bal[d.creditor] += d.amount;
    }
    return bal;
}

// ============================================================================
//  §2  Payment-mode selection (deterministic)
// ============================================================================

// Fixed preference keeps output reproducible; iterating the unordered_set
// directly would pick an arbitrary mode.
[[nodiscard]] const std::vector<std::string>& modePreference() {
    static const std::vector<std::string> kPref = {
        "UPI", "CreditCard", "DebitCard", "BankTransfer",
        "NEFT", "Cheque", "Cash"};
    return kPref;
}

[[nodiscard]] std::optional<std::string> chooseCommonMode(const Bank& payer,
                                                           const Bank& payee) {
    for (const auto& m : modePreference())
        if (payer.supports(m) && payee.supports(m)) return m;

    // Fallback: alphabetically first common mode (still deterministic).
    std::string best;
    for (const auto& m : payer.supportedModes)
        if (payee.supports(m) && (best.empty() || m < best)) best = m;

    if (best.empty()) return std::nullopt;
    return best;
}

void assignModes(std::vector<Settlement>& plan, const std::vector<Bank>& banks) {
    for (auto& s : plan)
        s.mode = chooseCommonMode(banks[s.payer], banks[s.payee])
                     .value_or("WorldBank (Intermediary)");
}

// ============================================================================
//  §3  Solvers
// ============================================================================

// Greedy settle on one zero-sum group.  Each round retires ≥ 1 bank,
// so a group of g banks needs at most g − 1 transfers.
void settleGroup(std::vector<int> group, const std::vector<Amount>& balance,
                 std::vector<Settlement>& out) {
    std::vector<Amount> bal;
    bal.reserve(group.size());
    for (int idx : group) bal.push_back(balance[idx]);

    for (;;) {
        int creditor = -1, debtor = -1;
        for (std::size_t i = 0; i < bal.size(); ++i) {
            if (bal[i] > 0 && (creditor < 0 || bal[i] > bal[creditor]))
                creditor = static_cast<int>(i);
            if (bal[i] < 0 && (debtor < 0 || bal[i] < bal[debtor]))
                debtor = static_cast<int>(i);
        }
        if (creditor < 0 || debtor < 0) return;

        // Exact-match: one transfer retires two banks at once.
        for (std::size_t i = 0; i < bal.size(); ++i) {
            if (bal[i] < 0 && -bal[i] == bal[creditor]) {
                debtor = static_cast<int>(i);
                break;
            }
        }

        const Amount paid = std::min(bal[creditor], -bal[debtor]);
        out.push_back({group[debtor], group[creditor], paid, {}});
        bal[creditor] -= paid;
        bal[debtor]   += paid;
    }
}

// --- Greedy solver (any size, O(n²), at most n−1 transfers) ---

[[nodiscard]] std::vector<Settlement> settleGreedy(const std::vector<Amount>& balance) {
    std::vector<int> nz;
    for (std::size_t i = 0; i < balance.size(); ++i)
        if (balance[i] != 0) nz.push_back(static_cast<int>(i));

    std::vector<Settlement> plan;
    settleGroup(std::move(nz), balance, plan);
    return plan;
}

// --- Exact solver (subset-DP, O(3^k), k ≤ 16) ---

constexpr int kMaxExactBanks = 16;

// Any valid plan splits banks into disjoint zero-sum islands.
// A zero-sum island of size g needs at least g − 1 transfers, and greedy
// achieves exactly g − 1.  Hence minimum = k − (max zero-sum islands).
[[nodiscard]] std::vector<Settlement> settleOptimal(const std::vector<Amount>& balance) {
    std::vector<int>    bankOf;          // local → global index
    std::vector<Amount> value;           // local balance
    for (std::size_t b = 0; b < balance.size(); ++b)
        if (balance[b] != 0) {
            bankOf.push_back(static_cast<int>(b));
            value.push_back(balance[b]);
        }

    const int k = static_cast<int>(value.size());
    if (k == 0) return {};
    const int full = (1 << k) - 1;

    // Subset sums via low-bit recurrence
    std::vector<Amount> sum(1 << k, 0);
    std::vector<int>    bitIdx(1 << k, 0);
    for (int i = 0; i < k; ++i) bitIdx[1 << i] = i;
    for (int mask = 1; mask <= full; ++mask) {
        const int low = mask & -mask;
        sum[mask] = sum[mask ^ low] + value[bitIdx[low]];
    }

    // dp[mask] = min transfers; split[mask] = chosen submask (0 = whole group)
    std::vector<int> dp(1 << k, 0), split(1 << k, 0);
    for (int mask = 1; mask <= full; ++mask) {
        if (sum[mask] != 0) continue;
        int pop = 0;
        for (int m = mask; m; m &= m - 1) ++pop;
        dp[mask] = pop - 1;              // greedy upper bound

        const int low  = mask & -mask;
        const int rest = mask ^ low;
        for (int sub = rest; sub > 0; sub = (sub - 1) & rest) {
            const int part = sub | low;
            if (sum[part] != 0) continue;
            const int cost = dp[part] + dp[mask ^ part];
            if (cost < dp[mask]) { dp[mask] = cost; split[mask] = part; }
        }
    }

    // Reconstruct islands (explicit stack, no recursion)
    std::vector<std::vector<int>> islands;
    {
        std::vector<int> stk{full};
        while (!stk.empty()) {
            const int m = stk.back(); stk.pop_back();
            if (m == 0) continue;
            if (split[m] != 0) {
                stk.push_back(split[m]);
                stk.push_back(m ^ split[m]);
                continue;
            }
            std::vector<int> island;
            for (int i = 0; i < k; ++i)
                if (m & (1 << i)) island.push_back(bankOf[i]);
            islands.push_back(std::move(island));
        }
    }

    std::vector<Settlement> plan;
    for (auto& island : islands)
        settleGroup(std::move(island), balance, plan);
    return plan;
}

// ============================================================================
//  §4  Input helpers
// ============================================================================

[[nodiscard]] std::string trim(const std::string& s) {
    const char* ws = " \t\r\n";
    auto b = s.find_first_not_of(ws);
    if (b == std::string::npos) return "";
    auto e = s.find_last_not_of(ws);
    return s.substr(b, e - b + 1);
}

[[nodiscard]] std::string readLine(const std::string& prompt) {
    std::string line;
    while (true) {
        std::cout << prompt;
        if (!std::getline(std::cin, line)) {
            std::cerr << "\nInput ended unexpectedly.\n";
            std::exit(EXIT_FAILURE);
        }
        line = trim(line);
        if (!line.empty()) return line;
        std::cout << "  Input cannot be empty.\n";
    }
}

[[nodiscard]] long long readNumber(const std::string& prompt, long long minVal) {
    while (true) {
        std::cout << prompt;
        long long v;
        if (std::cin >> v && v >= minVal) {
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            return v;
        }
        std::cerr << "  Enter a whole number >= " << minVal << ".\n";
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    }
}

void readBanks(std::vector<Bank>& banks, std::unordered_map<std::string, int>& nameMap) {
    const int n = static_cast<int>(readNumber("How many banks take part? ", 1));
    banks.reserve(n);

    for (int i = 0; i < n; ++i) {
        std::cout << "\nBank " << (i + 1) << " of " << n << "\n";
        std::string name;
        while (true) {
            name = readLine("  Name: ");
            if (nameMap.count(name))
                std::cout << "  \"" << name << "\" already exists.\n";
            else break;
        }
        banks.emplace_back(name);
        nameMap[name] = i;

        const int m = static_cast<int>(readNumber("  Number of payment modes: ", 0));
        for (int j = 0; j < m; ++j)
            banks.back().supportedModes.insert(
                readLine("    Mode " + std::to_string(j + 1) + ": "));
    }
}

[[nodiscard]] std::vector<Debt> readDebts(
        const std::unordered_map<std::string, int>& nameMap) {
    std::vector<Debt> debts;
    const int count = static_cast<int>(readNumber("\nHow many debts? ", 0));

    for (int i = 0; i < count; ++i) {
        std::cout << "\nDebt " << (i + 1) << " of " << count << "\n";
        const auto from = readLine("  From (debtor) : ");
        const auto to   = readLine("  To   (creditor): ");
        const Amount amt = readNumber("  Amount         : ", 1);

        auto fi = nameMap.find(from), ti = nameMap.find(to);
        if (fi == nameMap.end() || ti == nameMap.end()) {
            std::cout << "  Unknown bank — skipped.\n";
            continue;
        }
        if (fi->second == ti->second) {
            std::cout << "  Debtor = creditor — skipped.\n";
            continue;
        }
        debts.push_back({fi->second, ti->second, amt});
    }
    return debts;
}

// ============================================================================
//  §5  Reporting
// ============================================================================

void printBalances(const std::vector<Bank>& banks,
                   const std::vector<Amount>& balance) {
    std::cout << "\nNet balances:\n";
    for (std::size_t i = 0; i < banks.size(); ++i) {
        std::cout << "  " << banks[i].name << ": ";
        if (balance[i] > 0)      std::cout << "receives Rs " << balance[i];
        else if (balance[i] < 0) std::cout << "pays Rs "     << -balance[i];
        else                     std::cout << "settled";
        std::cout << "\n";
    }
}

void printPlan(const std::vector<Settlement>& plan,
               const std::vector<Bank>& banks, std::size_t debtCount) {
    std::cout << "\n================ SETTLEMENT PLAN ================\n";
    if (plan.empty()) {
        std::cout << "All balances are settled — no transfers needed.\n";
    } else {
        for (std::size_t i = 0; i < plan.size(); ++i) {
            const auto& s = plan[i];
            std::cout << "  " << (i + 1) << ". "
                      << banks[s.payer].name << " pays "
                      << banks[s.payee].name
                      << "  Rs " << s.amount
                      << "  [" << s.mode << "]\n";
        }
    }
    std::cout << "\nTransfers needed: " << plan.size()
              << "   (original debts: " << debtCount << ")\n";
    std::cout << "=================================================\n";
}

// ============================================================================
//  §6  Main
// ============================================================================

int main() {
    std::cout << "=========== Cash Flow Minimizer ===========\n";

    std::vector<Bank> banks;
    std::unordered_map<std::string, int> nameMap;
    readBanks(banks, nameMap);

    const auto debts    = readDebts(nameMap);
    const auto balance  = computeBalances(banks.size(), debts);

    // Zero-sum invariant: money neither appears nor vanishes.
    Amount total = 0;
    for (Amount b : balance) total += b;
    if (total != 0) {
        std::cerr << "\nError: balances do not sum to zero (Rs " << total
                  << ").\n";
        return EXIT_FAILURE;
    }

    if (debts.empty()) {
        std::cout << "\nNo debts entered — nothing to settle.\n";
        return 0;
    }

    printBalances(banks, balance);

    int nonzero = 0;
    for (Amount b : balance)
        if (b != 0) ++nonzero;

    std::vector<Settlement> plan;
    if (nonzero <= kMaxExactBanks) {
        std::cout << "\nSolving with exact subset-DP ("
                  << nonzero << " unsettled banks).\n";
        plan = settleOptimal(balance);
    } else {
        std::cout << "\n" << nonzero << " unsettled banks exceeds exact-solver "
                  << "limit (" << kMaxExactBanks << ") — using greedy.\n";
        plan = settleGreedy(balance);
    }

    assignModes(plan, banks);
    printPlan(plan, banks, debts.size());
}

} // namespace
