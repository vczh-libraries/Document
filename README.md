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
