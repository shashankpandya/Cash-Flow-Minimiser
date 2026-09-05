# 💸 Cash Flow Minimizer

A C++ system that models a group of banks (or any set of parties with mutual debts) as a **weighted graph of transactions**, computes each party's **net balance**, and uses a **greedy algorithm** to minimize the number of cash settlements required to clear all debts — the same core idea used by real-world settlement systems like **Splitwise's "simplify debts" feature**, **RTGS/ACH netting engines**, and interbank clearing houses.

> Instead of settling every individual IOU one by one, the engine collapses a web of `N` debts down to as few as `N - 1` net payments.

---

## 📌 Table of Contents

- [Problem Statement](#-problem-statement)
- [How It Works](#-how-it-works)
- [Why Greedy Works Here (Correctness)](#-why-greedy-works-here-correctness)
- [Features](#-features)
- [Tech Stack](#-tech-stack)
- [Build & Run](#-build--run)
- [Input Format](#-input-format)
- [Example Walkthrough](#-example-walkthrough)
- [Complexity Analysis](#-complexity-analysis)
- [Project Structure](#-project-structure)
- [Known Limitations](#-known-limitations)
- [Roadmap: Interview-Grade Enhancements](#-roadmap-interview-grade-enhancements)
- [What This Project Demonstrates](#-what-this-project-demonstrates)

---

## 🧩 Problem Statement

Imagine three banks:

```
A owes B ₹100
B owes C ₹50
A owes C ₹30
```

Settling this naively takes **3 transactions**. But if you net out each party's balance first:

```
A: -130   (paid out 100 + 30)
B: +50    (received 100, paid out 50)
C: +80    (received 50 + 30)
```

You can now settle the *entire group* with just **2 transactions**:
```
A pays C ₹80
A pays B ₹50
```

This is the **Cash Flow Minimization problem** — a classic application of greedy algorithms over a graph of debts, and it's the exact mechanism real clearing houses use to reduce the number of wire transfers (and transaction fees) between banks at end-of-day settlement.

---

## ⚙️ How It Works

1. **Model each bank as a node** with a `netAmount`, initialized to `0`.
2. **Process every transaction** `(from, to, amount)`:
   - `netAmount[from] -= amount`
   - `netAmount[to]   += amount`

   After this pass, every bank has a single net position: positive (net creditor) or negative (net debtor).
3. **Greedily settle balances:**
   - Find the bank with the **maximum positive** net amount (`creditor`).
   - Find the bank with the **maximum negative** net amount (`debtor`).
   - Transfer `min(creditor's balance, |debtor's balance|)` from debtor to creditor.
   - Update both balances and repeat.
4. **Stop** when the maximum positive or maximum negative balance hits zero — at that point, every remaining bank has a net balance of zero (see [correctness](#-why-greedy-works-here-correctness)).
5. **Payment mode resolution:** for each settlement, the engine looks for a payment mode (e.g. `UPI`, `NEFT`, `IMPS`) supported by both parties. If none exists, it falls back to a labeled intermediary, `WorldBank`.

---

## ✅ Why Greedy Works Here (Correctness)

This isn't just "greedy because it's simple" — it's provably optimal for this formulation:

- The **sum of all net balances is always zero** (every transaction subtracts from one party and adds the same amount to another — money is conserved).
- Matching the **largest creditor with the largest debtor** at each step guarantees that at least one of the two balances is *fully zeroed* every iteration.
- Since each settlement step eliminates at least one non-zero node, the algorithm terminates in **at most `V - 1` transactions** for `V` banks — which is also the theoretical minimum for a fully-connected settlement.

This greedy strategy is optimal for this exact "settle every node to zero" formulation. (Worth knowing for a follow-up question: the *general* variant of this problem — minimizing transactions when you don't require every intermediate step to zero out a party — is a harder combinatorial problem; mentioning that distinction shows depth if an interviewer pushes on it.)

---

## 🚀 Features

- ✅ Multi-bank net balance computation from an arbitrary transaction list
- ✅ Greedy minimum-transaction settlement algorithm
- ✅ Custom generic `Pair<K, V>` template for transaction (amount, payment mode) tuples
- ✅ Payment-mode compatibility resolution with `WorldBank` intermediary fallback
- ✅ Simple, dependency-free CLI — builds and runs with just a C++ compiler

---

## 🛠️ Tech Stack

- **Language:** C++ (C++11 or newer)
- **STL:** `vector`, `unordered_map`, `unordered_set`
- **No external dependencies** — single-file, portable

---

## ⚡ Build & Run

Requires a C++ compiler (e.g. `g++`) with C++11+ support.

```bash
g++ -std=c++11 -O2 -o cash_flow_minimizer "CASH FLOW MINIMIZER.cpp"
./cash_flow_minimizer
```

The program is fully interactive and will prompt for all input.

---

## 📥 Input Format

1. **Number of banks**
2. For each bank:
   - Bank name (single word, no spaces)
   - Number of payment modes it supports
   - List of payment modes (e.g. `UPI NEFT IMPS`)
3. **Number of transactions**
4. For each transaction: `from to amount`

---

## 🧪 Example Walkthrough

```text
Enter number of banks: 3
Bank 1 name: A
Enter number of payment modes: 2
Enter payment modes: UPI NEFT
Bank 2 name: B
Enter number of payment modes: 2
Enter payment modes: UPI IMPS
Bank 3 name: C
Enter number of payment modes: 1
Enter payment modes: NEFT

Enter number of transactions: 3
Transaction 1 (from to amount): A B 100
Transaction 2 (from to amount): B C 50
Transaction 3 (from to amount): A C 30

Minimized Transactions:
A pays Rs 80 to C via NEFT
A pays Rs 50 to B via UPI
```

Net balances before settlement: `A = -130`, `B = +50`, `C = +80`
Settled in **2 transactions** instead of the original 3 — and note `A→C` correctly falls back to `NEFT` (their only shared mode), while `A→B` uses `UPI`.

---

## 📊 Complexity Analysis

| Step | Time Complexity | Notes |
|---|---|---|
| Building net balances | O(T) | T = number of transactions |
| Finding max creditor / debtor per round | O(V) each | V = number of banks |
| Finding common payment mode | O(M) | M = payment modes per bank |
| **Total settlement loop** | **O(V² · M)** | V rounds, each doing O(V) scans + O(M) mode lookup |
| Space | O(V²) | for the `resultGraph` settlement matrix |

**Optimization note:** replacing the linear `getMaxIndex` / `getMinIndex` scans with a **max-heap and min-heap** (or a single balanced structure) reduces the scanning cost from O(V) to O(log V) per round, bringing total time down to **O(V log V · M)** — this is one of the first optimizations to mention if asked "how would you make this faster?"

---

## 📁 Project Structure

```
cash-flow-minimizer/
├── CASH FLOW MINIMIZER.cpp   # main program (single file)
└── README.md
```

---

## ⚠️ Known Limitations

- Uses `int` for amounts — large totals can overflow; use `long long` for production-scale values.
- No input validation for negative amounts, duplicate bank names, or malformed input.
- `getMaxIndex` / `getMinIndex` are O(V) linear scans — fine for small V, but not optimal at scale.
- Bank names and payment modes are single tokens (`cin >>`), so no spaces allowed.
- Single currency only — all amounts assumed to be in the same unit.
- No persistence — state exists only for the lifetime of the process.

---

## 🔭 Roadmap: Interview-Grade Enhancements

The sections below are **not yet implemented** — they're the planned extensions that turn this from a DSA exercise into a system with real engineering depth. Recommended build order (~2–3 days):

### Day 1 — Correctness & Performance Hardening
- [ ] Replace `int` with `long long` for all monetary values
- [ ] Add input validation (duplicate names, negative amounts, unknown banks)
- [ ] Replace linear `getMaxIndex`/`getMinIndex` scans with a **max-heap + min-heap** — reduces per-round cost from O(V) to O(log V)
- [ ] Add unit tests (GoogleTest/Catch2): balance conservation (`sum before == sum after`), zero-cycle graphs, single-bank edge case

### Day 2 — Algorithmic Depth
- [ ] **Multi-currency support** — normalize all transactions to a base currency using exchange rates before netting; handle rounding with integer minor units (e.g. paise/cents) instead of floats
- [ ] **Second algorithm for comparison:** model the same problem as a **min-cost max-flow** graph (source → debtors → creditors → sink) and benchmark both approaches on identical datasets — a strong talking point for "trade-offs between greedy and flow-based formulations"
- [ ] **Debt cycle detection** on the *raw* transaction graph (before netting) using DFS — useful for fraud/audit flags (e.g. flag suspicious circular transfers) even though netting handles them mathematically

### Day 3 — Systems Layer
- [ ] Persist banks/transactions to **SQLite** so state survives restarts
- [ ] Wrap the engine in a minimal **REST API** (e.g. `POST /transactions`, `GET /settlements`) so it's demoable live, not just a CLI
- [ ] **Thread-safety:** guard balance updates with a mutex to support concurrent transaction ingestion
- [ ] Add structured logging of every settlement decision for auditability

---

## 🎯 What This Project Demonstrates

- **Greedy algorithm design** with a provable termination and optimality argument — not just "it works," but *why* it works
- **Graph modeling of a real financial process** (net settlement), directly analogous to production fintech infrastructure
- Understanding of **when greedy is optimal vs. when a related problem becomes harder** — a strong signal of algorithmic maturity in interviews
- (After the roadmap) **systems thinking**: persistence, concurrency, and API design layered on top of a correct algorithmic core

---

**Author:** *Sir Shashank Pandya*
**Tech:** C++ · Graph Algorithms · Greedy Optimization
**Contact:** pandyashashank1@gmail.com
