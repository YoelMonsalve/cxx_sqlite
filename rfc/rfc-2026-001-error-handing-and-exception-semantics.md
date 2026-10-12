# RFC / Design Document: Error Handling & Exception Semantics in `object.hpp`

**Document ID:** RFC-2026-001  
**Target:** `cxx_sqlite3::generic_value` (`SQLITE3_OGEN`)  
**Status:** Open for Discussion / Community RFC  
**Authors:** Yoel Monsalve, AI Architecture Pair  
**Date:** October 2026  

---

## 1. Problem Statement

A fundamental question in the design of `generic_value` (the dynamic variant wrapping SQLite’s storage types) is:

> **"What should happen when an explicit user conversion requirement cannot be undoubtedly and unambiguously satisfied?"**

Currently, within `include/object.hpp`, several methods exhibit divergent, mixed error-handling semantics:

1. **Silent Fallback to Default Values (Zero/Empty):**
   - [`as_int()`](file:///home/yoel/projects/cxx_sqlite/include/object.hpp#L200-L216): If the variant holds a `blob_t`, or if string parsing (`std::stoll`) throws, it silently returns `0`:
     ```cpp
     } else if constexpr (std::is_same_v<T, text_t>) {
         try { return std::stoll(arg); } catch (...) { return 0; }
     } else {
         /** @annotation  Should it throw (e.g., invalid_argument) ? */
         return 0;
     }
     ```
   - [`as_double()`](file:///home/yoel/projects/cxx_sqlite/include/object.hpp#L220-L235): Silently returns `0.0` on invalid string or incompatible types.
   - [`as_blob()`](file:///home/yoel/projects/cxx_sqlite/include/object.hpp#L280-L286): Silently returns a `static const blob_t empty_blob{}` if the variant is not a blob.

2. **Permissive / Lossy Coercion:**
   - [`as_string()`](file:///home/yoel/projects/cxx_sqlite/include/object.hpp#L250-L265): If the variant holds a `blob_t`, it blindly reinterprets arbitrary binary bytes as a `std::string`:
     ```cpp
     } else if constexpr (std::is_same_v<T, blob_t>) {
         return std::string(arg.begin(), arg.end());
     }
     ```
     *(See Yoel's `@todo` at lines 240–248: "What happens if the current stored value is not convertible to string? Should it throw?").*

3. **Active Throwing:**
   - [`as_string_view()`](file:///home/yoel/projects/cxx_sqlite/include/object.hpp#L267-L278): Throws `exception_object("Cannot cast value to string_view")` if the underlying type is not `std::string`.
     *(Yet line 304 marks `operator std::string_view() const noexcept`, causing an immediate `std::terminate` if invoked on non-strings!).*

This document formalizes the debate, evaluates three candidate philosophies, and proposes an architectural standard for exceptions in `include/exception.hpp` and `include/object.hpp`.

---

## 2. Evaluation of Conversion Philosophies

When a conversion cannot be performed unambiguously (e.g., converting `"abc"` to `int`, or converting an arbitrary binary PNG `blob` to `std::string`), three primary philosophies exist:

```
                            +-----------------------------------------+
                            | Conversion Request: val.as<T>()         |
                            +-----------------------------------------+
                                                 |
                                     Can it be satisfied?
                                                 |
                       +-------------------------+-------------------------+
                       | YES                                               | NO
                       v                                                   v
                Return typed value                        +----------------------------------+
                                                          | Which Philosophy Governs Failure?|
                                                          +----------------------------------+
                                                           /               |                \
                                                          /                |                 \
                                              [Philosophy A]         [Philosophy B]     [Philosophy C]
                                                Throw Error         Coerce / Reinterpret   Silent Default
                                              (Fail-Fast)          (Permissive)         (Concealed)
```

---

### Philosophy A: Throwing an Exception (Fail-Fast, Strict Safety)

*Behavior:* Throw a dedicated, strongly-typed domain exception (`exception_object_bad_cast`).

#### Pros:
- **Zero Ambiguity:** Eliminates the bug where `"0"` (valid string), `"invalid"` (malformed string), and `NULL` all silently become integer `0`.
- **Data Integrity:** In database applications, treating corrupt or incompatible data as `0` can result in destructive updates (e.g., writing `salary = 0` to a database).
- **Alignment with Standard Library:** Follows `std::get<T>(variant)` (throws `std::bad_variant_access`), `std::any_cast` (throws `std::bad_any_cast`), and `nlohmann::json::get<T>()` (throws `json::type_error`).

#### Cons:
- Incurs exception handling runtime overhead on failure paths.
- Callers in non-throwing or real-time contexts must wrap calls in `try/catch`.

---

### Philosophy B: Forcing a Compatible Coercion (Permissive / Scripting Style)

*Behavior:* Follow SQLite's native C API casting rules (e.g., reinterpreting byte streams, parsing prefixes until first non-digit).

#### Pros:
- Highly forgiving in quick-and-dirty scripting scripts.
- Matches SQLite C API raw behaviors (`sqlite3_column_int` returns `0` on non-numeric strings).

#### Cons:
- **Severe Semantic Pitfalls:** A binary `blob` (e.g., encrypted payload, JPEG header, Protobuf) cast to `std::string` can contain embedded `\0` null characters, breaking C-string APIs and terminal outputs.
- Violates Modern C++ type safety expectations where C++ developers expect strict type contracts.

---

### Philosophy C: Silently Dropping to Default Value (`0`, `""`, `{}`)

*Behavior:* Return zero, empty string, or empty container on error.

#### Pros:
- Guaranteed `noexcept` everywhere.
- Simplest implementation.

#### Cons:
- **Worst of all worlds:** Complete loss of observability.
- The caller has **no way to distinguish** between:
  1. A cell containing integer `0`.
  2. A cell containing `NULL`.
  3. A cell containing a malformed string `"not a number"`.
  4. A cell containing a binary blob.

---

## 3. Recommended Design: The "Dual-API" Pattern

In modern C++ (and standard libraries such as `std::variant`, `std::optional`, and `nlohmann::json`), the established best practice is **not to choose between throwing and non-throwing, but to provide both with clear semantics**:

| Intent | API | Failure Mode | `noexcept`? |
| :--- | :--- | :--- | :--- |
| **Strict / Explicit** | `val.as<T>()`, `val.as_int()`, casts | Throws `exception_object_bad_cast` | `no` |
| **Safe / Fallback** | `val.as_or<T>(default_val)` | Returns `default_val` on failure | `noexcept` |
| **Optional / Monadic**| `val.try_as<T>()` | Returns `std::optional<T>` (`std::nullopt` on failure) | `noexcept` |

---

## 4. Concrete Proposal for `include/exception.hpp`

Currently, `include/exception.hpp` defines:
```cpp
struct exception_object : public exception { ... };
struct exception_object_bad_cast : public exception_object { ... };
```

### Proposed Specialization

We propose enriching `exception_object_bad_cast` to carry structured diagnostics:

```cpp
namespace cxx_sqlite3
{
    /**
     * exception_object
     * Base exception class for all generic_value and object-level domain errors.
     */
    struct exception_object : public exception
    {
        explicit exception_object(const std::string &msg) 
            : exception((std::string{"cxx_sqlite3 error: object: "} + msg).c_str()) {}
    };

    /**
     * exception_object_bad_cast
     * Thrown when generic_value cannot be cast or extracted into the requested target type.
     */
    class exception_object_bad_cast : public exception_object
    {
    private:
        std::string from_type_;
        std::string to_type_;

    public:
        exception_object_bad_cast(const std::string &from_type, 
                                  const std::string &to_type, 
                                  const std::string &reason)
            : exception_object("cannot cast from '" + from_type + "' to '" + to_type + "': " + reason),
              from_type_(from_type),
              to_type_(to_type) {}

        const std::string &from_type() const noexcept { return from_type_; }
        const std::string &to_type() const noexcept { return to_type_; }
    };

    /**
     * exception_object_null_dereference
     * Thrown when an operation requires a non-null value, but generic_value is null.
     */
    struct exception_object_null_dereference : public exception_object
    {
        exception_object_null_dereference() 
            : exception_object("attempted to extract value from null object without fallback") {}
    };
}
```

---

## 5. Consumption Proposal for `include/object.hpp`

### 5.1. Resolving `as_int()` (Lines 200–216)
Instead of catching and returning `0`:
```cpp
int_t as_int() const {
    return std::visit([](const auto &arg) -> int_t {
        using T = remove_cvref_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, int_t>) {
            return arg;
        } else if constexpr (std::is_same_v<T, double_t>) {
            return static_cast<int_t>(arg);
        } else if constexpr (std::is_same_v<T, text_t>) {
            try {
                size_t idx = 0;
                int_t val = std::stoll(arg, &idx);
                if (idx != arg.size()) {
                    throw exception_object_bad_cast("text", "int", "trailing unparsed characters in '" + arg + "'");
                }
                return val;
            } catch (const std::exception &e) {
                throw exception_object_bad_cast("text", "int", "invalid numeric format");
            }
        } else if constexpr (std::is_same_v<T, null_t>) {
            throw exception_object_null_dereference();
        } else {
            // blob_t
            throw exception_object_bad_cast("blob", "int", "binary blob cannot be converted to integer");
        }
    }, storage_);
}
```

### 5.2. Resolving `as_string()` (Lines 250–266)
Resolving Yoel's `@todo` regarding binary blobs:
- **Rule:** A `blob_t` contains arbitrary binary octets. Interpreting it directly as a `std::string` can produce ill-formed UTF-8 and invisible null-termination truncations.
- **Strict Resolution:**
  ```cpp
  } else if constexpr (std::is_same_v<T, blob_t>) {
      throw exception_object_bad_cast("blob", "string", "raw binary blob cannot be implicitly treated as text; use as_blob()");
  }
  ```
  Users wanting a hex dump or string copy of a blob should explicitly use a dedicated utility or pass `as_blob()`.

### 5.3. Resolving `as_string_view()` and `operator std::string_view()` (Lines 267–278, 304)
- **Current Bug:** `operator std::string_view() const noexcept` calls `as_string_view()`, which throws `exception_object`. If invoked on a non-string, `std::terminate()` is invoked immediately!
- **Resolution:**
  1. Remove `noexcept` from `operator std::string_view()` so exceptions propagate cleanly if invoked on improper types.
  2. Or, if non-throwing behavior is required for views, provide a safe `std::optional<std::string_view> try_as_string_view() const noexcept`.

### 5.4. Non-Throwing Companions (`try_as` and `as_or`)
To support performance-critical loops and code that desires fallback values:
```cpp
template <typename T>
std::optional<T> try_as() const noexcept {
    try {
        return as<T>();
    } catch (...) {
        return std::nullopt;
    }
}

template <typename T>
T as_or(T fallback) const noexcept {
    try {
        return as<T>();
    } catch (...) {
        return fallback;
    }
}
```

---

## 6. Mandatory Specification: Explicit `noexcept` Contracts for Non-Throwing Methods

A vital architectural requirement following the introduction of formal exception semantics:

> **Mandate:** Once support for exceptions is established (whether Strict, Safe Fallback, Monadic, or all three), **all methods guaranteed not to throw must be explicitly declared with `noexcept`**.

This provides three crucial benefits in Modern C++:
1. **Performance & Move Optimization:** Enables standard library containers (e.g., `std::vector<generic_value>`) to perform move-reallocation rather than expensive copy-reallocation via `std::move_if_noexcept`.
2. **Deterministic Error Boundaries:** Clearly delineates for the user which methods are safe to invoke in destructors, error handlers, and performance-critical loops without requiring `try/catch`.
3. **Prevention of `std::terminate` Traps:** Enforces consistency between declarations and implementations (e.g., ensuring an operator marked `noexcept` does not call an underlying throwing accessor).

### 6.1. Inventory of Methods Required to be `noexcept`

| Category | Method | Specification |
| :--- | :--- | :--- |
| **State Inspection** | `type() const noexcept`<br>`index() const noexcept`<br>`storage() const noexcept` | Guaranteed non-throwing underlying `std::variant::index()`. |
| **Type Predicates** | `is_null() const noexcept`<br>`is_int() const noexcept`<br>`is_float() const noexcept`<br>`is_text() const noexcept`<br>`is_blob() const noexcept`<br>`is_numeric() const noexcept` | Pure value inspection; must never throw. |
| **Special Member Functions** | `~generic_value() noexcept`<br>`generic_value(generic_value&&) noexcept`<br>`operator=(generic_value&&) noexcept` | Move construction and destruction of `std::variant` over primitive types are `noexcept`. |
| **Relational & Logical Operators** | `operator==(const generic_value&) const noexcept`<br>`operator!=(const generic_value&) const noexcept`<br>`operator!() const noexcept` | Resolves Yoel's annotation at line 312 (`@attention Can this method throw? If not, protect it with noexcept`). |
| **Monadic & Fallback Accessors** | `try_as<T>() const noexcept`<br>`as_or<T>(T fallback) const noexcept`<br>`try_as_string_view() const noexcept` | Explicitly swallows or avoids exceptions; guaranteed never to throw. |

### 6.2. Correction of Erroneous `noexcept` Annotations
- **`operator std::string_view()` (Line 304):** Currently marked `noexcept`, yet calls `as_string_view()` which throws `exception_object`.
  - **Correction:** If `as_string_view()` throws on non-strings, `operator std::string_view()` **must NOT be marked `noexcept`**. Calling a throwing function from inside a `noexcept` function triggers an immediate `std::terminate()` if an exception is thrown.
  - Alternatively, if non-throwing string-view access is desired, provide `try_as_string_view() const noexcept` returning `std::optional<std::string_view>`.

---

## 7. Community Feedback & Open Questions

We invite community feedback on the following specific points:

1. **Null Handling in `as_int()` / `as_double()` / `as_string()`:**
   - *Question:* Should `null.as_int()` throw `exception_object_null_dereference`, or should `null` be treated as `0` / `""`?
   - *Consideration:* Standard SQL evaluates `NULL` distinctly from `0` and `""`. Throwing prevents silent `NULL` propagation bugs.

2. **Blob Reinterpretation:**
   - *Question:* Should `as_string()` throw when called on a `blob_t`, or is there any legitimate case where implicit reinterpretation as `std::string` is desirable?

3. **Opt-in vs. Opt-out via Macro:**
   - *Question:* Similar to `CXX_SQLITE3_EXPLICIT_CONV`, should there be a `CXX_SQLITE3_STRICT_CONVERSIONS` macro allowing legacy users to opt for silent `0` fallbacks if migrating legacy codebases?

---

*Comments, critiques, and RFC revisions are welcomed.*
