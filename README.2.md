# Cash Flow Minimizer

A **C++17 program** that **minimizes the number of fund transfers** needed to settle mutual debts between banks — also known as **multilateral netting**.

---

## Problem

If five banks owe each other money, naively settling each debt one-by-one requires one transfer per debt record. But many of those transfers cancel out.

This program **nets everything into per-bank balances**, then finds the smallest set of transfers that zeroes every balance.

---

## How It Works

### 1. Input

The program takes:

* Banks
* Payment modes supported by each bank
* All outstanding debts

### 2. Netting

Every debt is collapsed into a single **net balance** per bank.

| Balance       | Meaning                                         |
| ------------- | ----------------------------------------------- |
| `balance > 0` | Bank is a **creditor** and should receive money |
| `balance < 0` | Bank is a **debtor** and should pay money       |
| `balance = 0` | Bank is already settled                         |

### 3. Solve

The program computes a **minimum or near-minimum transfer plan**.

* Uses the **exact subset-DP solver** when the number of unsettled banks is small enough.
* Automatically falls back to the **greedy solver** for larger instances.

### 4. Assign Payment Modes

For every transfer, the program selects a payment method supported by **both banks**.

Mode selection is deterministic:

1. Use the fixed payment-mode preference order.
2. If necessary, fall back to lexicographic ordering.

---

# Algorithms

## Greedy Solver

The greedy solver is always available.

It repeatedly:

1. Finds the largest debtor.
2. Finds the largest creditor.
3. Transfers the maximum possible amount between them.
4. Gives priority to an **exact match**, where a debtor's outstanding amount exactly equals the creditor's requirement.

An exact match is preferred because a single transfer can completely settle **both banks**.

### Complexity

* **Time:** `O(n²)`
* **Space:** `O(n)`
* **Transfers:** At most `n - 1`

### Drawback

The greedy strategy is **not always globally optimal**.

---

## Exact Solver — Subset DP

For `k ≤ 16` unsettled banks, the program uses an exact **subset-DP** algorithm.

### Key Observation

Any valid settlement plan can be divided into disjoint **zero-sum islands**.

A zero-sum island is a group of banks whose balances add up to zero:

```text
sum(balance[i]) = 0
```

Money never needs to leave such an island.

For an island containing `g` banks:

* At least `g - 1` transfers are required.
* The greedy settlement routine can achieve exactly `g - 1` transfers within the island.

Therefore:

```text
minimum transfers
    = k - (maximum number of disjoint zero-sum islands)
```

The subset-DP searches for the partition that maximizes the number of such islands.

### Complexity

* **Time:** `O(3^k)`
* **Space:** `O(2^k)`
* **Constraint:** `k ≤ 16`

---

## Example Where Greedy Is Not Optimal

Consider the net balances:

```text
+10, +7, -7, -5, -5
```

| Solver     | Transfers | Plan                                                           |
| ---------- | --------: | -------------------------------------------------------------- |
| **Greedy** |         4 | C → A `7`, D → B `5`, E → A `3`, E → B `2`                     |
| **Exact**  |         3 | `{B,C}` → C pays B `7`; `{A,D,E}` → D pays A `5`, E pays A `5` |

The exact solver discovers two independent zero-sum islands:

```text
{B, C}     : +7 - 7 = 0
{A, D, E}  : +10 - 5 - 5 = 0
```

Therefore, only:

```text
(2 - 1) + (3 - 1) = 3 transfers
```

are required.

---

# Build & Run

Requires a **C++17-compatible compiler** such as `g++` or `clang++`.

```bash
g++ -std=c++17 -O2 -Wall -Wextra -o cashflow cashflow.cpp
./cashflow
```

---

# Example Session

```text
=========== Cash Flow Minimizer ===========

How many banks take part? 3

Bank 1 of 3
  Name: SBI
  Number of payment modes: 2
    Mode 1: UPI
    Mode 2: Cash

Bank 2 of 3
  Name: ICICI
  Number of payment modes: 2
    Mode 1: UPI
    Mode 2: BankTransfer

Bank 3 of 3
  Name: HDFC
  Number of payment modes: 2
    Mode 1: UPI
    Mode 2: Cash

How many debts? 3

Debt 1 of 3
  From (debtor) : SBI
  To   (creditor): ICICI
  Amount         : 5000

Debt 2 of 3
  From (debtor) : ICICI
  To   (creditor): HDFC
  Amount         : 3000

Debt 3 of 3
  From (debtor) : HDFC
  To   (creditor): SBI
  Amount         : 5000

Net balances:
  SBI: settled
  ICICI: receives Rs 2000
  HDFC: pays Rs 2000

Solving with exact subset-DP (2 unsettled banks).

================ SETTLEMENT PLAN ================
  1. HDFC pays ICICI  Rs 2000  [UPI]

Transfers needed: 1   (original debts: 3)
=================================================

3 debts → 1 transfer. All balances are now zero.
```

---

# Key Design Decisions

| Decision                         | Rationale                                                                             |
| -------------------------------- | ------------------------------------------------------------------------------------- |
| `int64_t` amounts                | Prevents integer overflow when handling large sums                                    |
| Structured `Settlement` type     | Replaces a matrix plus `pair<int, string>` with a simpler flat representation         |
| Solver does not mutate bank data | Balances are derived data; original input remains read-only                           |
| Deterministic mode selection     | Uses a fixed preference order such as `UPI > Card > ...`, with lexicographic fallback |
| Automatic exact-solver selection | Uses exact DP when possible without requiring extra UI decisions                      |
| Greedy fallback for large `k`    | Prevents exponential DP growth when there are more than 16 unsettled banks            |
| Self-debt rejection              | Prevents invalid transactions where a bank owes itself                                |
| Duplicate-name rejection         | Ensures every bank can be identified unambiguously                                    |

---

# Complexity

| Component              |       Time |    Space |
| ---------------------- | ---------: | -------: |
| Balance computation    |     `O(d)` |   `O(b)` |
| Greedy solver          |    `O(k²)` |   `O(k)` |
| Exact subset-DP solver |   `O(3^k)` | `O(2^k)` |
| Mode assignment        | `O(k × m)` |        — |

Where:

* `b` = number of banks
* `d` = number of debt records
* `k` = number of unsettled banks
* `m` = number of payment modes per bank

---

# Requirements

* **C++17 or later**
* Standard library only
* No external dependencies

The implementation relies on C++17 features such as:

* `std::optional`
* Structured bindings
* `int64_t`
* Other standard C++17 library facilities

---

# Summary

The program follows a simple pipeline:

```text
                    ┌──────────────┐
                    │    Input     │
                    │ Banks +      │
                    │ Debts +      │
                    │ Payment Modes│
                    └──────┬───────┘
                           │
                           ▼
                    ┌──────────────┐
                    │    Netting   │
                    │ Compute each │
                    │ bank balance │
                    └──────┬───────┘
                           │
                           ▼
                  ┌───────────────────┐
                  │ Choose Solver      │
                  │                   │
                  │ k ≤ 16 → Exact DP │
                  │ k > 16 → Greedy   │
                  └────────┬──────────┘
                           │
                           ▼
                    ┌──────────────┐
                    │  Settlement  │
                    │    Plan      │
                    └──────┬───────┘
                           │
                           ▼
                    ┌──────────────┐
                    │ Assign common│
                    │ payment mode │
                    └──────────────┘
```

The result is a settlement plan that reduces the number of actual fund transfers while preserving the total amount owed by every bank.
