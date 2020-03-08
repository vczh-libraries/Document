# C++ and Document Compiler

## Goal

This is a command line tool for generating indexed code in HTML and documents from comments with Markdown for C++.

## "The compiler"

All **the compiler** here means the compiler created by this project.

## Requirements

- The compiler accepts a preprocessed cpp file from cl.exe.
  - The cpp file for preprocessed should compile using the same option which is also used to create the preprocessed file
- Comments containing documents should begin with `///`
- Comments should contain Markdown in XML tags
  - Details will be filled later

## Supported C++ Core Language Features

[C++20](https://en.wikipedia.org/wiki/C%2B%2B20): Review after publishing

- **Supported**
  - [C++11](https://en.wikipedia.org/wiki/C%2B%2B11)
    - Rvalue references and move constructors
    - Uniform initialization
    - Type inference
    - Range-based for loop
    - Alternative function syntax
    - Null pointer constant
    - Right angle bracket
    - Template aliases
    - Unrestricted unions
    - Variadic templates
    - New string literals
    - Explicitly defaulted and deleted special member functions
  - [C++14](https://en.wikipedia.org/wiki/C%2B%2B14)
    - Function return type deduction
    - Alternate type deduction on declaration
    - Variable templates
    - Binary literals
  - [C++17](https://en.wikipedia.org/wiki/C%2B%2B17)
    - Allow typename (as an alternative to class) in a template template parameter
    - Nested namespace definitions, e.g., `namespace X::Y { … }` instead of namespace `X { namespace Y { … } }`
    - Allowing attributes for namespaces and enumerators
    - UTF-8 (u8) character literals (UTF-8 string literals have existed since C++11; C++17 adds the corresponding character literals for consistency, though as they are restricted to a single byte they can only store ASCII)
    - Initializers in if and switch statements
- **Support before releasing**
  - [C++11](https://en.wikipedia.org/wiki/C%2B%2B11)
    - Initializer lists
    - Lambda functions and expressions
    - Object construction improvement
      - Constructor calls another constructor in initializing list
      - Using base class' constructors
    - Explicit overrides and final **(not care #2)**
      - `final` keyword
    - Extern template **(not care #1)**
      - `extern` keyword before `template` keyword
    - Type `long long int`
    - Control and query object alignment **(not care #1)**
    - Explicit conversion operators: **Treat all as implicit** **(not care #2)**
      - Need test to see if `explicit` is correctly parsed
  - [C++14](https://en.wikipedia.org/wiki/C%2B%2B14)
    - Generic lambdas
    - Lambda capture expressions
  - [C++17](https://en.wikipedia.org/wiki/C%2B%2B17)
    - New standard attributes `[[fallthrough]]`, `[[maybe_unused]]` and `[[nodiscard]]`
    - A compile-time static if with the form if constexpr(expression) **(not care #1)**
- **Parsed but it doesn't affect type inferencing so I don't care**
  - [C++11](https://en.wikipedia.org/wiki/C%2B%2B11)
    - Thread-local storage
    - Allow sizeof to work on members of classes without an explicit object:
    - Attributes
    - Static assertions
  - [C++14](https://en.wikipedia.org/wiki/C%2B%2B14)
    - The attribute `[[deprecated]]`
  - [C++17](https://en.wikipedia.org/wiki/C%2B%2B17)
    - copy-initialization and direct-initialization of objects of type `T` from prvalue expressions of type `T` (ignoring top-level cv-qualifiers) shall result in no copy or move constructors from the prvalue expression. See copy elision for more information. helper template function std::make_pair(5.0, false).
    - Inline variables, which allows the definition of variables in header files without violating the one definition rule. The rules are effectively the same as inline functions
    - Value of `__cplusplus` changed to 201703L: **I don't do preprocessing by myself**
    - `__has_include`, allowing the availability of a header to be checked by preprocessor directives: **I don't do preprocessing by myself**
    - Making the text message for static_assert optional
- **Parsed but it only reduces the accuracy of overloading so I don't care**
  - [C++11](https://en.wikipedia.org/wiki/C%2B%2B11)
    - constexpr – Generalized constant expressions: **Treat all constant values identical**
    - Modification to the definition of plain old data
    - Strongly typed enumerations: **Type is ignored**
    - Relaxed constexpr restrictions: **Treat all constant values identical**
  - [C++17](https://en.wikipedia.org/wiki/C%2B%2B17)
    - New rules for auto deduction from braced-init-list
    - Constant evaluation for all non-type template arguments: **Treat all constant values identical**
    - Exception specifications were made part of the function type: **Don't compare exception specifications**
- **No plan**
  - Explicit (full) specialization of a member of a template class
    - For `template<typename T> struct X { void f(); };`, `template<> void X<int>::f(){...}` is not supported.
  - Specializations defined not in the scope where the primary declaration is in
    - For `template<typname T> struct X { template<typename U> struct Y {}; };`, `template<typename T> template<typename U> struct X<T*>::Y {};` is not supported.
  - [C++11](https://en.wikipedia.org/wiki/C%2B%2B11)
    - User-defined literals
    - Multithreading memory model
    - Allow garbage collected implementations
  - [C++14](https://en.wikipedia.org/wiki/C%2B%2B14)
    - Aggregate member initialization
    - Digit separators
  - [C++17](https://en.wikipedia.org/wiki/C%2B%2B17)
    - Hexadecimal floating-point literals
    - Some extensions on over-aligned memory allocation
    - Class template argument deduction (CTAD), introducing constructor deduction guides, eg. allowing `std::pair(5.0, false)` instead of requiring explicit constructor arguments types `std::pair<double, bool>(5.0, false)` or an additional   helper template function std::make_pair(5.0, false).
    - Structured binding declarations, allowing `auto [a, b] = getTwoReturnValues();`for   more information.
    - Fold expressions, for variadic templates

## Limitations

**All limitations could be changed in the future**.

### Syntax

- The compiler doesn't parse all C++11/C++14/C++17/C++20 features, but it parses the latest GacUI and most old C++ programs.
- All features that cl.exe doesn't support, are also not supported here.
- The compiler doesn't handle implicitly declared ctors or dtor for `union`, all `union` are assumed constructible in any default ways.
- The compiler ignores anonymous namespaces and `extern "C"` declarations, and parses sub declarations without creating an anonymous scope.
- The compiler doesn't recognize [user-defined literals](https://en.cppreference.com/w/cpp/language/user_literal), maybe I will finish it later.

### Semantic

- Candidates of names with overloading may not be completely narrowed because of incomplete template-related type inferencing.
  - The compiler doesn't do `const` or `constexpr` constant folding, the compiler treats all values identical.
    - This usually means sometimes multiple types will be returned due to overloadings or specializations.
    - This also means `enable_if` cannot narrow down candidates, all possible candidates will be returned.
- All members are treated as public.
- Implicitly generated members doesn't care if any base class is virtual.
- C style type reference is allowed to refer to a unexisting type. In this case, a forward declaration with the symbol is created in the root namespace.
- Implicit generated type alias of current instantiation `A` is not really added to template class `A`.
  - The compiler only allow `A` (which is an `IdExpr`) to be the current instantiation when the code is in the scope of `A`.
  - The compiler will always let `A` becomes the current instantiation even when it is passed as a template argument.
- The compiler assumes MSVC 32bits.

### Analyzing Code

- The compiler parses all function bodies after the whole program is analyzed, so the behavior will be slightly different from the C++ standard.
  - One exception is that, if the type of the function is required, and the return type depends of the function body, then the body will be parsed at that moment.
- The compiler doesn't check bodies of template declarations when creating their instances.
  - For example, the compiler resolves the body of a template function right after it is parsed. All recognizable names will be identified at this moment. The body will not be processed again later when this function is instanciating in somewhere else.
  - The reason for this is, **GacUI** and its dependeices offers a lot of template declarations. They are supposed to be used by users. So there is no reason not to process template function bodies if they are not used inside the library. There is also no reason to offer more detailed indexing just for all places using these template functions inside the library.

## If you want to use it but you have trouble processing your own source code

- This project is not open sourced. I may change the license once the compiler is ready to release.
- I will announce when I think the compiler is ready to release. Now it is not completed so you are expected to have trouble.
- If you believe the compiler should be able to handle a certain valid case but it doesn't, please open an issue with a minimum reproduction, including your preprocessed file produced by cl.exe.
  - If the compiler cannot parse any Windows SDK file, I will consider it to be a bug and fix it.
  - Narrowing down candidates more precisely is not considered at this moment.
