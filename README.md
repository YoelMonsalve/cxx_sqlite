# cxx_sqlite3

> **An elegant, high-level C++ wrapper for SQLite3 designed with modern C++ semantics and command-line natural ergonomics.**

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![C++ Standard](https://img.shields.io/badge/C%2B%2B-11%20%7C%2014%20%7C%2017%20%7C%2020-blue.svg)](#requirements)

---

## Author's Insight & Philosophy

Interacting with SQLite from native C is robust and high-performing, but the raw C API requires manual resource tracking (`sqlite3_finalize`, `sqlite3_close`), tedious manual step loops, and repetitive parameter index bindings. 

`cxx_sqlite3` was conceived with a clear vision: **writing and executing SQL sentences in C++ should feel as direct, natural, and expressive as interacting with the sqlite3 command-line prompt**, while preserving:
1. **Zero-leak RAII safety**: Database handles, prepared statements, and transactions manage their lifecycles automatically via strict Resource Acquisition Is Initialization (RAII).
2. **Natural Ergonomics**: Alternating variadic parameter binding (`":param", value`), range-based `for` iteration over query results, and type-safe column conversions without boilerplate.
3. **Dual Error-Handling Model**: Support both idiomatic modern C++ exceptions and non-throwing `ErrorCode` inspections based on the `std::error_code` architecture.

### *My story*

I started writing this library about two years ago, required as part of a larger project I was working on. The main purpose was having a practical mean to interact with a small local database that served as backup for the main application.

Lately, I've been leveraging the AI to accelerate the developement of the last parts of my code, and to take it to the next level! (while I keep overseeing and managing the overall process).

So, it is still an *in-progress* work, though it is actually 100% functional (as demonstrated by tests) and everyone can make use of it. *Working on the 2.0-future release !*

### Why the MIT License?
This library is distributed under the permissive **MIT License**. SQLite is arguably the most ubiquitous public domain database in software engineering—powering everything from embedded devices and mobile apps to foundational open-source platforms (such as OpenCode). To ensure `cxx_sqlite3` can be embedded, linked, adapted, and redistributed freely by the global developer community—in open source and commercial systems alike—without viral license constraints, the MIT license was chosen.

---

## Key Features

- **Strict RAII Architecture**:
  - `database`: Automatically closes SQLite database connections upon destruction.
  - `statement`: Movable, non-copyable RAII handle that ensures `sqlite3_finalize` is always called.
  - `transaction`: Scoped transaction guard. Automatically rolls back pending changes if an exception occurs or if the scope exits without an explicit `commit()`.
- **Natural Variadic Parameter Binding**:
  - Named parameters: `stmt.bind(":name", "Alice", ":age", 30);`
  - Positional 1-indexed parameters: `stmt.bind(1, "Alice", 2, 30);`
  - Direct execution helpers: `db.execute("INSERT INTO users VALUES (?, ?)", 1, "Bob");`
- **Range-Based `for` Query Iteration**:
  - Directly iterate queries with modern C++ range-for loops:
    ```cpp
    for (const auto &row : db.query("SELECT id, name FROM users WHERE age > ?", 18)) {
        std::cout << row["name"].as<std::string>() << "\n";
    }
    ```
- **High-Level Typing System (`generic_object`)**:
  - Seamless representation of SQLite's 5 storage classes (`NULL`, `INTEGER`, `FLOAT`, `TEXT`, `BLOB`).
  - Strong, type-safe extraction with `row["col"].as<T>()` and convenient implicit conversions.
  - Dedicated `SQLITE3_ONULL` sentinel and `std::vector<uint8_t>` BLOB support.
- **Dual Error Model**:
  - Exception hierarchy (`exception_open`, `exception_prepare`, `exception_step`, etc.) for clean, expressive control flow.
  - System-error compatible `ErrorCode` for codebases requiring error-code inspection.

---

## Quick Start & Examples

### 1. Opening a Database & Scoped Transactions

```cpp
#include <iostream>
#include "sqlite.hpp"

using namespace cxx_sqlite3;

int main() {
    // Open in-memory or on-disk database
    database db(":memory:");

    // Execute schema statements
    db.exec("CREATE TABLE users (id INTEGER PRIMARY KEY, name TEXT, balance REAL);");

    // RAII Transaction: auto-rolls back if an exception is thrown before commit()
    {
        transaction txn = db.begin_transaction();
        db.execute("INSERT INTO users (id, name, balance) VALUES (:id, :name, :bal);",
                   ":id", 1, ":name", "Alice", ":bal", 2500.50);
        
        txn.commit(); // Explicitly commit transaction
    }

    return 0;
}
```

### 2. Variadic Parameter Binding

```cpp
// Prepare movable statement
statement stmt = db.prepare_statement("INSERT INTO users (id, name, balance) VALUES (?, ?, ?);");

// Bind positional arguments in pairs of (index, value)
stmt.bind(1, 2, 2, "Bob", 3, 3100.0);
stmt.step();
```

### 3. Range-based For Loop & Type-Safe Extraction

```cpp
// Query returns an iterable statement
for (const auto &row : db.query("SELECT id, name, balance FROM users ORDER BY id ASC;")) {
    int id = row["id"].as<int>();
    std::string name = row["name"].as<std::string>();
    double balance = row["balance"].as<double>();

    std::cout << "User #" << id << ": " << name << " ($" << balance << ")\n";
}
```

### 4. Working with BLOBs and NULLs

```cpp
// Inserting binary data
blob_t avatar = { 0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A };
db.execute("INSERT INTO users (id, name, bio) VALUES (:id, :name, :bio);",
           ":id", 3, ":name", "Charlie", ":bio", avatar);

// Reading row values with std::optional for null safety
statement query = db.prepare_statement("SELECT name, bio FROM users WHERE id = :id;");
query.bind(":id", 3);
row r = query.fetch_row();

auto opt_bio = r["bio"].as<std::optional<blob_t>>();
if (opt_bio.has_value()) {
    std::cout << "Loaded avatar (" << opt_bio->size() << " bytes)\n";
} else {
    std::cout << "User has no avatar.\n";
}
```

### 5. C++17 Tuple Deconstruction (`row.as<Types...>()`)

```cpp
for (const auto &row : db.query("SELECT id, name, balance FROM users ORDER BY id ASC;")) {
    // Unpack an entire row into native types using C++17 structured bindings!
    auto [id, name, balance] = row.as<int, std::string, double>();
    std::cout << "User #" << id << ": " << name << " ($" << balance << ")\n";
}
```

### 6. Object-Relational Mapping & Strongly-Typed Queries (v1.2)

```cpp
struct Employee {
    int id;
    std::string name;
    double salary;
    std::optional<std::string> email;
};

// Map struct fields without intrusive boilerplate
CXX_SQLITE3_DEFINE_TYPE_NON_INTRUSIVE(Employee, id, name, salary, email)

// 1. Direct struct insertion via db.execute
Employee new_hire{101, "Alice", 9200.0, "alice@company.com"};
db.execute("INSERT INTO employees VALUES (:id, :name, :salary, :email);", new_hire);

// 2. Fetch all matching rows directly into std::vector<Employee>
std::vector<Employee> team = db.query_as<Employee>("SELECT * FROM employees WHERE salary > ?", 5000.0);

// 3. Query a single record as std::optional<Employee>
std::optional<Employee> emp = db.query_one<Employee>("SELECT * FROM employees WHERE id = ?", 101);
if (emp) {
    std::cout << "Found employee: " << emp->name << " ($" << emp->salary << ")\n";
}
```

### 7. Stream Operators `<<` & `>>` (Pipelining, `exec`, and `eol`)

```cpp
// 1. Immediate database pipelining with sequential parameters
db << "INSERT INTO employees VALUES (?, ?, ?, ?);"
   << 102 << "Bob" << 7500.0 << std::nullopt
   << cxx_sqlite3::exec; // or cxx_sqlite3::eol

// 2. Direct database pipelining with an ADL-mapped struct
Employee carl{103, "Carl", 8100.0, "carl@company.com"};
db << "INSERT INTO employees VALUES (:id, :name, :salary, :email);"
   << carl
   << cxx_sqlite3::eol; // eol is an alias for exec (End-Of-Line / End-Of-Query)

// 3. Sequential column extraction from a row
row r = db.query("SELECT id, name, salary FROM employees WHERE id = 102;").fetch_row();
int id;
std::string name;
double salary;
r >> id >> name >> salary;

// 4. Stream extraction loop over a query (while query has rows)
statement query = db.query("SELECT * FROM employees WHERE salary >= ? ORDER BY id ASC;", 7000.0);
Employee e;
while (query >> e) {
    std::cout << "Employee: " << e.name << " ($" << e.salary << ")\n";
}

// 5. Tuple stream extraction loop
statement q = db.query("SELECT id, name, salary FROM employees ORDER BY id ASC;");
std::tuple<int, std::string, double> record;
while (q >> record) {
    auto [emp_id, emp_name, emp_salary] = record;
    std::cout << "#" << emp_id << " " << emp_name << " (" << emp_salary << ")\n";
}
```

---

## Building and Testing

### Prerequisites
- C++17 compatible compiler (`g++` 7+, `clang++` 5+, or MSVC 2017+)
- Linux / POSIX: System SQLite (`libsqlite3-dev` / `sqlite3`)
- Windows: Standalone `wsqlite/` distribution included with this repository

### Install sqlite

#### Debian / Ubuntu / Mint
```bash
sudo apt update
sudo apt install libsqlite3-dev
```

#### Fedora / RHEL / CentOS / Rocky Linux
```bash
# Fedora / RHEL 8+
sudo dnf install sqlite-devel

# Older CentOS / RHEL 7
sudo yum install sqlite-devel
```

#### Arch Linux / Manjaro
*(Arch includes development headers in the main package)*
```bash
sudo pacman -S sqlite
```

#### Alpine Linux
```bash
sudo apk add sqlite-dev
```

#### openSUSE
```bash
sudo zypper install sqlite3-devel
```

### Build & Test Commands

```bash
# Compile library and run all tests
make

# Run the complete test suite (both bare and Catch2 tests)
make test

# Run specifically the Catch2-style test suite (tests/ctest/)
make test_catch2

# Run specifically the bare RAII test (tests/test_raii.cpp)
make test_raii

# Clean build artifacts
make clean
```

### Test Folder Structure
- [`tests/ctest/`](tests/ctest): Framework-based test suites (such as [`test_suite.cpp`](tests/ctest/test_suite.cpp), using the lightweight Catch2-style `TEST_CASE` / `REQUIRE` / `SECTION` runner).
- [`tests/`](tests): Pure standalone C++ tests without test frameworks (such as [`test_raii.cpp`](tests/test_raii.cpp)).
- Root (`test1.cpp`): Legacy/introductory example application.

---

## License

This project is licensed under the terms of the [MIT License](LICENSE).  
(C) Copyright Yoel Monsalve 2025-2026.
