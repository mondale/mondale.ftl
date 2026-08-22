# mondale.ftl

`mondale.ftl`

## Architectural Requirements

Follow any directory-specific README.md with higher precedence than this file.

`//base` may not depend on any libraries outside of `//base`.

`//core` may not depend on any libraries outside of `//base` and `//core`.

## Style Guidance

The following style guidance should be used for all code. Examples are
considered to be authoritative.

## Forbidden Patterns.

C++ exceptions may not be used.

### Naming.

Prefix globval variables' names with `global`.

```cpp
int global_variable_is_prefixed_by_global = 0;
```

Methods are named in `PascalCase`.

```cpp
void MethodDeclaration();
```

Classes are named in `PascalCase` and are marked `final` if not intende for
further inheritance.

```cpp
class ClassDeclaration final {
 public:
  ...
 private:
  ...
};

```

Use short names for method parameters, preferably initialisms.
```cpp
class Foo final {
 public:
  Foo(const BarBaz* bb) : bar_baz_(bb) {}
  ...
};
```

Use desciptive names in `snake_case` with a trailing `_` for member variables.
```cpp
class Foo final {
 public:
  ...

 private:
  Bar bar_;
};
```

### Common practices.

Use yoda comparisons to avoid accidental assignment in conditionals. I.e.,:
```cpp
  if (4 == x) {  // Preferred over x == 4
    ...
  }
```

Use `const` on all local variables that are not intended for mutation. I.e.,:
```cpp
  const int x = 4;
  ... // x not mutated
```

Most types should support `operator<<` as a freestanding method and either
`x.ToString()` or `ToString(x)` as human-readable debug aids.

```cpp
class X final {
 public:
  // Where possible without corruption risk.
  std::string_view ToString() const;

  // Where necessary.
  std::string ToString() const;
  ...
};

std::ostream& operator<<(std::ostream& out, const X& x) {
  out << x.ToString();
  return out;
}
```

When suppressing a `[[nodiscard]]` or an unused variable warning, use a
C++-style `static_cast<void>`:
```cpp
  int unused = 0;
  static_cast<void>(unused);  // no compiler warning
```

Use raw integral type names, not prefixed by `std::`. E.g.,:
```cpp
  int32_t no_std_prefix_necessary = 0;
```

### Exceptions to these rules.

Fils may be individually labeled as exempt from this guidance. This is commonly
used for AI-implemented sections of the codebase. An exemption takes the form of

`// Exempt from style expectations.`

### Code Formatting (.clang-format)

Formatting is strictly enforced across C++ source and header files using
`clang-format`. Key formatting parameters include:

* **Language Standard**: Standard C++ targeting modern specifications (`C++20` /
                         `C++26`).
* **Indentation & Spacing**:
  * **Indent Width**: 2 spaces (no hard tabs).
  * **Continuation Indent**: 4 spaces.
  * **Line Width**: 80 characters maximum.
* **Brace Placement**:
  * Attach braces (`Attach`) for functions, classes, control statements, and
    namespaces (K&R / Google style).
* **Pointers & References**:
  * Left-aligned pointer/reference qualifiers (e.g., `Foo* ptr`,
    `const Bar& ref`).
* **Namespaces**:
  * No indentation inside `namespace` blocks.
  * Namespace closing comments are required for clarity
    (e.g., `} // namespace base`).
* **Include Management**:
  * Sorted alphabetically and grouped logically (Standard Library headers 
    first, followed by system headers, then repository-relative headers).
  * Source files need not redundantly include headers that are provided by the
    same-name header file associated with the source file.

---

### File Organization & Naming

* **File Names**: Lowercase snake_case for all filenames 
                  (e.g., `async_safe.cc`, `raw_syscalls_test.cc`).
* **Headers**: Most `.cc` module should have a corresponding `.h` header 
               exposing public interfaces, except for standalone binary mains
               or test entry points.
* **Header Guards**: Protect headers using canonical `#ifndef` / `#define` 
                     guards using the path and filename in uppercase:
  ```cpp
  #ifndef BASE_ASYNC_SAFE_H_
  #define BASE_ASYNC_SAFE_H_

  // Header contents

  #endif  // BASE_ASYNC_SAFE_H_
  ```
