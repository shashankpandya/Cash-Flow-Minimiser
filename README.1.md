# Cash Flow Minimizer (C++)

![C++](https://img.shields.io/badge/Language-C%2B%2B-blue.svg)
![Standard](https://img.shields.io/badge/Standard-C%2B%2B11%2B-green.svg)
![License](https://img.shields.io/badge/License-MIT-yellow.svg)

A robust and interactive C++ command-line program designed to **optimize financial transactions** among multiple entities (referred to as "banks"). The primary goal is to **minimize the number of direct cash flow transactions** required to settle all outstanding debts and credits within a system.

Given initial transactions and the payment capabilities of each bank, the program intelligently calculates the net financial position of each participant and generates a streamlined set of payments that resolves all balances with the fewest possible direct transfers.

## Table of Contents

- [Cash Flow Minimizer (C++)](#cash-flow-minimizer-c)
  - [Table of Contents](#table-of-contents)
  - [Project Overview](#project-overview)
  - [Features](#features)
  - [How It Works (Algorithm)](#how-it-works-algorithm)
  - [Getting Started](#getting-started)
    - [Prerequisites](#prerequisites)
    - [Build Instructions](#build-instructions)
    - [Run Instructions](#run-instructions)
  - [Usage Example (Interactive Session)](#usage-example-interactive-session)
  - [Code Structure](#code-structure)
  - [Limitations and Assumptions](#limitations-and-assumptions)
  - [Future Enhancements](#future-enhancements)
  - [License](#license)

## Project Overview

In a network of financial transactions, many individual payments can lead to complex and inefficient settlement processes. This program addresses this by:

1.  **Calculating Net Balances**: For each bank, it aggregates all incoming and outgoing transactions to determine its final net credit or debit.
2.  **Applying a Greedy Minimization Algorithm**: It iteratively identifies the largest creditor (who is owed the most) and the largest debtor (who owes the most) and facilities a direct payment between them, using a mutually supported payment method if available.
3.  **Generating Optimized Transactions**: The output is a clear list of who needs to pay whom, how much, and via what method, significantly reducing the overall number of transactions compared to the original, potentially many-to-many payments.

## Features

*   **Bank Management**: Define banks with unique names and a list of supported payment methods.
*   **Transaction Processing**: Input multiple initial transactions between banks to establish their net financial positions.
*   **Intelligent Payment Routing**: Automatically finds a common payment mode between transacting parties.
*   **Intermediary Fallback**: If two banks do not share a common payment method, a fallback "WorldBank (Intermediary)" mode is designated for the transaction, simulating a central clearing house or an external transfer mechanism.
*   **Greedy Optimization**: Employs a proven greedy algorithm to minimize the number of individual payment transfers.
*   **Robust Input Handling**: Features user-friendly interactive prompts with input validation for numerical entries and allows bank names and payment modes with spaces.
*   **Clear Output**: Presents minimized transactions in an easy-to-understand format.
*   **Balance Integrity Check**: Warns the user if the total net sum of all bank balances is not zero, indicating potential input errors.

## How It Works (Algorithm)

The program implements a classic **greedy algorithm** to achieve cash flow minimization.

1.  **Initialization**:
    *   Banks are created, each storing its name, a set of supported payment types, and an initial `netAmount` of 0.
    *   An `unordered_map` is used to quickly look up a bank's internal index by its name.

2.  **Processing Initial Transactions**:
    *   For each input transaction (`FromBank` pays `ToBank` an `Amount`):
        *   `FromBank`'s `netAmount` is decreased by `Amount`.
        *   `ToBank`'s `netAmount` is increased by `Amount`.
    *   After all transactions are processed, each bank holds its final `netAmount` (positive for creditors, negative for debtors).

3.  **Minimization Loop**:
    *   The core of the algorithm runs in a loop until all `netAmount`s become zero.
    *   In each iteration:
        *   `getCreditorIndex()`: Finds the bank with the **maximum positive `netAmount`** (the one owed the most).
        *   `getDebtorIndex()`: Finds the bank with the **maximum negative `netAmount`** (the one that owes the most).
        *   **Determine Transfer Amount**: The amount to be transferred is `min(creditor.netAmount, abs(debtor.netAmount))`. This ensures that either the creditor's positive balance is fully settled, or the debtor's negative balance is fully settled, or both.
        *   **Find Payment Mode**: `findCommonPaymentMode()` attempts to find a shared payment method between the creditor and debtor.
        *   **Fallback Mode**: If no common mode is found, the transaction defaults to `"WorldBank (Intermediary)"`.
        *   **Record Transaction**: The transfer (amount and mode) is recorded from the debtor to the creditor in a `resultGraph` matrix.
        *   **Update Balances**: The `netAmount` of both the creditor (decreased) and the debtor (increased) are updated by the transfer amount.

4.  **Output**: Once the loop terminates (all net amounts are zero), the `resultGraph` is iterated to print all the recorded minimized transactions.

## Getting Started

### Prerequisites

*   A C++ compiler that supports C++11 or newer (e.g., `g++`, Clang, MSVC).

### Build Instructions

Navigate to the directory containing `cash_flow_minimizer.cpp` (or `CASH FLOW MINIMIZER.cpp` as in your provided file name) in your terminal and compile it:

```bash
g++ -std=c++11 -O2 -o cash_flow_minimizer "cash_flow_minimizer.cpp"
# If your file is named "CASH FLOW MINIMIZER.cpp":
# g++ -std=c++11 -O2 -o cash_flow_minimizer "CASH FLOW MINIMIZER.cpp"
```
*   `-std=c++11`: Specifies the C++11 standard (you can use `c++14`, `c++17`, `c++20` as well).
*   `-O2`: Enables optimization flags for better performance.
*   `-o cash_flow_minimizer`: Names the output executable file `cash_flow_minimizer`.

### Run Instructions

After successful compilation, run the executable from your terminal:

```bash
./cash_flow_minimizer
```

The program is interactive and will guide you through the input process.

## Usage Example (Interactive Session)

Here's an example of how to interact with the program (user input is shown after prompts):

```text
--- Cash Flow Minimizer ---
Enter the number of banks: 3

--- Input Bank Details ---

Details for Bank 1:
  Enter Bank name (e.g., 'SBI', 'ICICI Bank'): Bank A
  Enter the number of payment modes supported by Bank A: 2
  Enter 2 payment mode(s) (e.g., 'Cash', 'CreditCard', 'UPI', 'BankTransfer'):
    Mode 1: UPI
    Mode 2: NEFT

Details for Bank 2:
  Enter Bank name (e.g., 'SBI', 'ICICI Bank'): Bank B
  Enter the number of payment modes supported by Bank B: 2
  Enter 2 payment mode(s) (e.g., 'Cash', 'CreditCard', 'UPI', 'BankTransfer'):
    Mode 1: UPI
    Mode 2: IMPS

Details for Bank 3:
  Enter Bank name (e.g., 'SBI', 'ICICI Bank'): Bank C
  Enter the number of payment modes supported by Bank C: 1
  Enter 1 payment mode(s) (e.g., 'Cash', 'CreditCard', 'UPI', 'BankTransfer'):
    Mode 1: NEFT

Enter the number of initial transactions to process: 3

--- Enter Initial Transactions ---
Format: FromBank ToBank Amount (e.g., 'SBI ICICI Bank 1000')

Transaction 1:
  From bank name: Bank A
  To bank name: Bank B
  Amount: 1000

Transaction 2:
  From bank name: Bank B
  To bank name: Bank C
  Amount: 500

Transaction 3:
  From bank name: Bank A
  To bank name: Bank C
  Amount: 300

--- Minimized Transactions ---
Bank B pays Rs 500 to Bank A via UPI.
Bank A pays Rs 800 to Bank C via NEFT.
------------------------------
```

## Code Structure

*   `cash_flow_minimizer.cpp` (or `CASH FLOW MINIMIZER.cpp`): Contains all the source code for the project, including the `Bank` class, utility functions, the `minimizeCashFlow` algorithm, and the `main` function.

## Limitations and Assumptions

*   **Greedy Approach**: The algorithm is greedy, which is proven to be optimal for minimizing the *number* of transactions in this specific problem.
*   **Integer Amounts**: All transaction amounts are handled as `int`. While `long long` is used for the total net balance check, individual transaction amounts might overflow if they exceed `INT_MAX`. For very large financial systems, `long long` or arbitrary-precision arithmetic might be necessary.
*   **Direct Payments Only**: The model assumes direct payments between two entities. More complex routing (e.g., A pays B, B pays C, so A directly pays C) is implicitly handled by the net balance approach but not explicitly optimized for intermediate hops.
*   **Intermediary Mechanism**: The "WorldBank (Intermediary)" is a conceptual placeholder. In a real-world scenario, this implies a bank transfer or another system that doesn't require direct compatibility between the two transacting parties.
*   **No Transaction Fees/Costs**: The model does not account for transaction fees, exchange rates, or any other costs associated with payments.

## Future Enhancements

*   **GUI Interface**: Develop a graphical user interface for easier interaction and visualization.
*   **Data Persistence**: Add functionality to save and load bank and transaction data from files (e.g., CSV, JSON).
*   **More Complex Payment Routing**: Implement advanced algorithms that might consider transaction costs or preferred payment routes.
*   **Multi-Currency Support**: Extend to handle transactions in different currencies with exchange rates.
*   **Transaction History**: Maintain a detailed log of all original and optimized transactions.
*   **Unit Tests**: Add a comprehensive suite of unit tests to ensure correctness and robustness.

## License

This project is open-source and available under the [MIT License](LICENSE).

---
