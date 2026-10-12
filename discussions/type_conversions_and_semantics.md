# Type Conversions and Value Extraction in `cxx_sqlite3`

**Date:** October 2026  
**Context:** Design discussion on `generic_value` (`SQLITE3_OGEN`) conversion APIs, arithmetic operator ambiguity, and ISO C++ standards compliance.

---

## 1. Executive Summary

In a dynamically typed or variant-based wrapper such as `cxx_sqlite3::generic_value`, bridging the gap between dynamic SQLite storage (`std::variant<std::monostate, int64_t, double, std::string, blob_t>`) and strongly-typed C++ code requires deliberate design choices.

We analyzed three primary conversion and extraction mechanisms:

| Mechanism | Example Syntax | Primary Role | Safety & Standards Alignment |
| :--- | :--- | :--- | :--- |
| **1. Explicit Named Accessors** | `val.as_int()` | Low-level, non-templated runtime extraction | Zero template overhead; explicit intent; throws on mismatch. |
| **2. Generic Extractor** | `val.as<T>()` | Metaprogramming, ORM, deserialization | Primary driver for SFINAE, tuples, enums, and generic code. |
| **3. Conversion Operators** | `static_cast<int>(val)` or `(int)val` | Language-level ergonomics and type casting | Subject to C++ overload resolution; must be `explicit` by default. |

---

## 2. Detailed Analysis of Each Mechanism

### 2.1. Named Accessors: `val.as_int()`, `val.as_double()`, etc.

#### Exact Role
- Non-templated, direct member functions representing the fundamental types supported by the underlying SQLite representation.
- Mirrors standard variant accessors and database driver conventions (e.g., JDBC `getInt()`, Qt `toInt()`).

#### Technical Characteristics & Behavior
- **Implementation:** Direct inspection of the internal `std::variant`, with controlled cross-type coercions (e.g., widening integer to `double`, or formatting number to string where defined).
- **Compile-Time Overhead:** Minimal. No template instantiation required.
- **Diagnostics:** On unsupported conversion, throws `std::bad_variant_access` or a domain-specific `sqlite_exception`.
- **Best Practice Context:** Ideal for procedural, non-generic application code where the target type is statically known by the developer at the call site.

---

### 2.2. Generic Extractor: `val.as<T>()`

#### Exact Role
- Unified generic interface for type extraction.
- Acts as the core integration point for modern C++ template metaprogramming, ORM mapping, stream deserialization (`row >> obj`), and tuple unpacking.

#### Technical Characteristics & Behavior
- **Implementation via Customization Points:** Leverages `remove_cvref_t<T>` and delegates to specialized traits / extractors:
  ```cpp
  template <typename T>
  inline T generic_value::as() const {
      using CleanT = remove_cvref_t<T>;
      return static_cast<T>(value_extractor<CleanT>::extract(*this));
  }
  ```
- **Extensibility:** Enables automatic decay for cv-qualified types (`const int&`), deduction for `enum` / `enum class` via `std::underlying_type_t`, and user-defined extractor specializations for third-party domain types.
- **Fail-Fast Diagnostics:** Uses `static_assert` at compile time to reject unsupported target types (e.g., arbitrary non-serializable structs) before runtime invocation.
- **Industry Precedent:** Follows the design of `nlohmann::json::get<T>()`, `boost::lexical_cast<T>()`, and `std::any_cast<T>()`.

---

### 2.3. User-Defined Conversion Operators: `operator T()`

#### Exact Role
- Integrates `generic_value` into native C++ language expressions and initialization contexts.
- Allows an instance of `generic_value` to satisfy target type demands in assignments, function parameters, and casts.

#### The Core Problem: Implicit vs. Explicit Conversions

In C++, user-defined conversion operators without `explicit` participate in **implicit conversion sequences** across all language contexts:

```cpp
// If defined as:
operator int() const;
operator double() const;
operator int64_t() const;
```

#### The Ambiguity Dilemma in Binary Expressions:
When evaluating an expression such as:
```cpp
generic_value obj_int(3);
auto result = obj_int + 10; // COMPILE ERROR: Ambiguous overload
```
The ISO C++ compiler evaluates all candidate built-in operator overloads according to **[over.match.oper]**:
1. Candidate 1: `operator+(int, int)` (via `operator int()`)
2. Candidate 2: `operator+(double, int)` (via `operator double()`)
3. Candidate 3: `operator+(int64_t, int)` (via `operator int64_t()`)

All conversions from `generic_value` to a arithmetic operand are user-defined conversions of equal rank. Because no single conversion is preferred by the overload resolution rules, **the compiler must reject the code as ambiguous**.

#### Other Risks of Implicit Conversions:
- **Unintended Function Overloads:** Passing `generic_value` to overloaded functions (e.g., `void log(int)` vs `void log(double)`) triggers identical ambiguity errors.
- **Silent Lossy Conversions:** An accidental assignment `float f = obj;` silently invokes truncation through `operator double()` or `operator int64_t()` without warning.
- **Accidental Boolean Conversion:** Implicit conversion to numeric types allows values to participate unintentionally in boolean conditions (`if (val)`), masking logic errors.

---

## 3. C++ Standard & Core Guidelines Recommendations

### ISO C++ Core Guidelines

1. **C.164: "Avoid conversion operators"**
   > *Reason:* Conversion operators can be a source of subtle, hard-to-find bugs when invoked unexpectedly by the compiler during overload resolution.

2. **C.165: "Use `explicit` conversion operators to make conversions explicit"**
   > *Reason:* Marking conversion operators `explicit` prevents them from participating in implicit conversion sequences and implicit operator resolutions, while still allowing:
   > - Direct initialization: `int x(val);`
   > - Static casts: `static_cast<int>(val)`
   > - C-style casts: `(int)val`
   > - Functional casts: `int(val)`

### Contrast: Implicit vs. Explicit Behavior Matrix

| Expression | Implicit (`operator T()`) | Explicit (`explicit operator T()`) |
| :--- | :--- | :--- |
| `int x = val;` (Copy Init) | Allowed | **Rejected at compile time** |
| `int x(val);` (Direct Init) | Allowed | Allowed |
| `static_cast<int>(val)` | Allowed | Allowed |
| `(int)val + 10` | Allowed | Allowed (Unequivocal) |
| `val + 10` | **Compiler Error (Ambiguous)** | **Rejected at compile time** |
| `take_int(val)` | Allowed (or Ambiguous if overloaded) | **Rejected at compile time** |
| `take_int(val.as<int>())` | Allowed | Allowed |

---

## 4. Value Categories, *prvalues*, and Mandatory Standard Requirements

A critical dimension of this design centers on ISO C++ value categories—specifically how **prvalues** (*pure rvalues*) behave under C++17 rules, how they interact with conversion operators, and what the C++ standard mandates.

### 4.1. Definition and Role of *prvalues* in C++17 ([basic.lval])
Prior to C++17, an rvalue was often loosely thought of as a "temporary object." In C++17 and later, value categories were formalized:
- A **prvalue** ("pure rvalue") is an expression whose evaluation computes a value that is not an object or a bit-field, or whose evaluation initializes an object.
- Functions and conversion operators that return by value (e.g., `as_int()`, `as<T>()`, and `operator T()`) evaluate to **prvalues**.

### 4.2. Guaranteed Copy Elision ([dcl.init], [class.copy.elision])
Under C++17, guaranteed copy elision is a **mandatory language rule**, not an optional compiler optimization:
- When a prvalue initializes an object of the same type:
  ```cpp
  std::string s = val.as<std::string>(); // Mandatory zero-copy in C++17
  ```
  The prvalue initializes `s` directly in place. No temporary object is constructed, and no copy or move constructor is called—even if `T` is a non-movable, non-copyable type.

### 4.3. Temporary Materialization Conversion ([conv.rval])
When an expression requires a glvalue (such as binding to a const reference):
```cpp
const std::string& ref = val.as<std::string>();
```
The ISO C++ standard mandates a **temporary materialization conversion**:
- The prvalue is converted into an **xvalue** referring to a materialized temporary object.
- The lifetime of that materialized temporary is bound to the reference according to standard lifetime extension rules.

### 4.4. Mandatory Overload Resolution Rules for Built-in Operators ([over.match.oper])
Why did `obj + 3` fail under implicit conversion operators? The ISO C++ standard defines exact, mandatory mechanics:

1. **Candidate Set Generation:**  
   When evaluating binary operators on class types, the compiler generates candidate functions from:
   - Non-member `operator+` overloads.
   - Member `operator+` overloads.
   - **Built-in candidate operator functions** synthesized by the compiler:
     ```cpp
     operator+(int, int)
     operator+(int64_t, int)
     operator+(double, int)
     ```

2. **Standard Conversion Sequences:**  
   To call a built-in operator, the class operand (`generic_value`) must be converted to a prvalue of the parameter type.
   If `operator int()`, `operator int64_t()`, and `operator double()` are all implicit:
   - `generic_value -> int` (User-Defined Conversion Sequence)
   - `generic_value -> int64_t` (User-Defined Conversion Sequence)
   - `generic_value -> double` (User-Defined Conversion Sequence)

3. **Mandatory Failure ([over.match.best]):**  
   Under clause **[over.match.best] §1**, all user-defined conversions between unrelated fundamental types have **identical conversion rank**. Because none of the candidate built-in functions is strictly better than the others, the standard mandates:
   > *"If the best viable function is not unique, the program is ill-formed."*

### 4.5. Why `explicit` Solves the Ambiguity by Standard Mandate ([class.conv.fct])
Under **[class.conv.fct] §2**:
> *"An explicit conversion function will be considered for a user-defined conversion only where the target type is the type of the result of the conversion function and the conversion is explicit (such as in direct-initialization or an explicit cast)."*

- Because `val + 3` is an implicit context, **all explicit conversion functions are excluded from the candidate set by mandate**.
- When the user writes an explicit cast:
  ```cpp
  (int)val + 3
  // or
  static_cast<int>(val) + 3
  ```
  The explicit conversion operator is invoked directly, yielding an unambiguous **prvalue of type `int`**.
- The binary `+` operator then evaluates between two `int` prvalues under standard Usual Arithmetic Conversions ([expr.arith.conv]) with zero ambiguity.

---

## 5. The Architectural Solution Adopted in `cxx_sqlite3`

To achieve the optimal balance between **Strict Modern C++ Safety** and **User Flexibility**, the library implements a dual-mode configurable policy:

### 5.1. The `CXX_SQLITE3_EXPLICIT_CONV` Policy Macro

```cpp
#ifndef CXX_SQLITE3_EXPLICIT_CONV
    #if defined(NO_EXPLICIT)
        #define CXX_SQLITE3_EXPLICIT_CONV
    #else
        #define CXX_SQLITE3_EXPLICIT_CONV explicit
    #endif
#endif
```

All conversion operators in `include/object.hpp` are declared using this macro:

```cpp
CXX_SQLITE3_EXPLICIT_CONV operator int() const { return as_int(); }
CXX_SQLITE3_EXPLICIT_CONV operator int64_t() const { return as_int64(); }
CXX_SQLITE3_EXPLICIT_CONV operator double() const { return as_double(); }
CXX_SQLITE3_EXPLICIT_CONV operator std::string() const { return as_string(); }
CXX_SQLITE3_EXPLICIT_CONV operator blob_t() const { return as_blob(); }
```

### 5.2. Configuration Delivery Modes

To support both standalone/Makefile builds and single-header distribution (nlohmann style), configuration can be supplied at two levels:

1. **Build System Level (`config.mk`):**
   - The project `Makefile` includes `-include config.mk`.
   - Projects desiring implicit conversions can define:
     ```make
     DEFINES += -DNO_EXPLICIT
     ```
   - If `config.mk` is absent, the Makefile builds safely with defaults without error.

2. **Header-Only Level (`cxx_sqlite3_config.hpp`):**
   - For single-header deployments where users do not use Makefiles:
     ```cpp
     #if __has_include("cxx_sqlite3_config.hpp")
         #include "cxx_sqlite3_config.hpp"
     #endif
     ```
   - Users can drop a local `cxx_sqlite3_config.hpp` in their include path to control preprocessor definitions globally.

---

## 6. Decision Guidelines for Developers

1. **In Generic and Library Code:**
   - Always prefer `val.as<T>()`.
   - It is unambiguous, SFINAE-friendly, supports enums and CV-qualifiers, and produces clean compiler diagnostics.

2. **In Direct Procedural Queries:**
   - Use `val.as_int()`, `val.as_string()`, etc., for maximum clarity and zero template overhead.

3. **In Arithmetic Expressions:**
   - Always cast explicitly:
     ```cpp
     auto total = static_cast<int>(row["quantity"]) * unit_price;
     // or
     auto total = row["quantity"].as<int>() * unit_price;
     ```
   - This unambiguously documents the developer's intent and guarantees portability regardless of compiler options.
