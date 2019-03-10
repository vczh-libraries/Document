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

- The compiler doesn't parse all C++17 features, but it parses more enough for most of C++ programs.
- All features that cl.exe doesn't support, are also not supported here.
- The compiler doesn't handle implicitly declared ctors or dtor for `union`, all `union` are assumed constructible in any default ways.
- The compiler ignores anonymous namespaces and `extern "C"` declarations, and parses sub declarations without creating an anonymous scope.

### Semantic

- The compiler doesn't do `const` or `constexpr` constant folding, the compiler treats all values identical.
  - This usually means the compiler assumes `int[1]` and `int[2]` are the same type, which will cause the compiler fail to compile the input file in some very limited cases.
  - This also applies to `T<1>` and `T<2>`.
- All members are treated as public.
- Implicitly generated members doesn't care if any base class is virtual.
- C style type reference is allowed to refer to a unexisting type. In this case, a forward declaration with the symbol is created in the root namespace.

### Analyzing Code

- The compiler parses all function bodies after the whole program is analyzed, so the behavior will be slightly different from the C++ standard.
  - One exception is that, if the type of the function is required, and the return type depends of the function body, then the body will be parsed at that moment.
- The compiler doesn't check bodies of template declarations when creating their instances.
  - For example, the compiler resolves the body of a template function right after it is parsed. All recognizable names will be identified at this moment. The body will not be processed again later when this function is instanciating in somewhere else.
  - The reason for this is, **GacUI** and its dependeices offers a lot of template declarations. They are supposed to be used by users. So there is no reason not to process template function bodies if they are not used inside the library. There is also no reason to offer more detailed indexing just for all places using these template functions inside the library.
- Candidates of names with overloading may not be completely narrowed because of incomplete template-related type inferencing.

### Error Handing

- The compiler will report errors only if they stopped further processing.
  - `StopParsingException`
    - Syntax error.
    - Symbols of different type defined in the same scope using the same name.
    - Symbols of the same type defined in the same scope using the same name, but they are not functions.
    - Anonymous `class`/`struct`/`union` which creates new type, and it is not used after `typedef`.
    - Unable to resolve to a single target namepsaces in a `using namespace` declaration.
    - Unable to resolve to non-namespace symbols in a `using` declaration. It also applies to `typedef` declaration.
    - Symbols are resolved in a `using` declaration, but the name is conflict with existing symbols in that scope. It also applies to `typedef` declaration.
    - A function returns `auto`, `decltype(auto)` or similar types, but it doesn't has a function body.
    - A member function defined out side of a class without a function body.
    - A variable is defined using `auto`, `decltype(auto)` or similar types, but it doesn't has an initializer expression.
    - An `extern` variable with an initializer expression.
    - A multiple-declarator declarations of functions.
    - A `ClassType::Name` member defined inside a class.
    - Unable to resolve the class scope of a `ClassType::Name` declaration.
    - A function with `->` to define a return type, but the type defore the function name is not `auto`.
    - Unable to resolve symbols during parsing a name expression.
    - Unable to resolve symbols during parsing a name type.
  - `IllegalExprException`
    - Unrecognizable char, string or other literal constant.
    - Unable to resolve type for `this` expression.
    - An expression is needed by the name resolved to types.
  - `NotConvertableException`
    - Unable to resolve `auto`, `decltype(auto)` or similar types.
    - Unable to resolve a type referenced by a name token.
  - `NotResolvableException`
    - Resolving the type of a symbol and it turns out that the type directly or indirectly (cyclically) depends on itself.
    - Unable to resolve the type of the variable defined by a range-based for loop.
    - Unable to resolve the type of a variable, whose type is `auto`, `decltype(auto)` or similar types and
      - it lacks of an initializer expression.
      - it has an initializer expression, but unable to resolve the type of that expression.
    - Unable to resolve the type of a function, which returns `auto`, `decltype(auto)` or similar types and
      - it lacks of a function body.
      - it has a function body, but it lacks of any return statement.
      - unable to resolve the type of the expression in the first return statement.
    - Unable to resolve the type of a `using` or `typedef` type alias declaration.
    - After getting a type, it doesn't fit into `auto`, `decltype(auto)` or similar types.
    - Find initialization list in a function, but this function is not a constructor defined in a class.
  - If you have `int Foo::Bar(int(&)[1])` and `int Foo::Bar(int(&)[2])`, although this code is valid, but due to the limitation mentioned above, the compiler stops here.
  - Generic type instances with constant arguments are affected for the same reason.
  - For most of wrong cases, like `struct Foo { static inline virtual int Bar() = 0; };`, the compiler doesn't care.

## If you want to use it but you have trouble processing your own source code

- This project is not open sourced. I may change the license once this project is ready to release.
- I will announce when I think this project is ready to release. Now it is not completed so you are expected to have trouble.
- If you believe the compiler should be able to handle a certain valid case but it doesn't, please open an issue with a minimum reproduction, including your preprocessed file produced by cl.exe.