# Draft

- The tool consumes a single preprocessed C++ source file, the following syntax is collected for this purpose.
- The syntax has less restrictions than the real C++ language specification, because the tool assumes the input is correct.

## Refactoring Work Items

- [ ] Add exception messages so that the final application won't crash leaving nothing.

## TODO

- Built-in types and functions can be found [here](https://clang.llvm.org/docs/LanguageExtensions.html)
  - `__is_*` and `__has_*` return null, arguments could be expressions or types, create a new Expr subclass.
    - This should be handled since arguments are not expressions
  - When more built-in things are found, list below:
    - Types
      - `__make_integer_seq<A, B, Size>` -> `A<B>`
      - `__underlying_type` -> `int` // should be enum's integral type
    - Functions (by default: ADL searching found nothing and evaluate to no type)
      - `__builtin_addressof(EXPR)` -> `&EXPR` without calling overloaded operators
      - `__builtin_huge_valf(...)` -> float
      - `__builtin_nanf(...)` -> float
      - `__builtin_nansf(...)` -> float

- [post](https://en.cppreference.com/w/cpp/language/function_template)

### Document Generation

- [ ] Improve `WebsiteSource`
  - [ ] Convert `<seealso cref="CREF"/>` to `<seealsos><symbol/>...</seealsos>`.
  - [ ] Convert `<example>` to `<program>`
  - [ ] `FileIndex.xml`: file tree view
  - [ ] `SymbolIndex.xml`: symbol tree view, fragments will be generated in `WebsiteSource`
  - [ ] `Sources/Source.html`: HTML fragment for source files, hyper links are converted to numbers
  - [ ] `Sources/Source.js`: hyper link information for `Source.html`
  - [ ] generate a completed list of article pages to a text file for `npm run download`.
- [ ] Build script for document
  - [ ] New projects in Document
    - [ ] reprocess of GacUI files
    - [ ] build GacUI files
  - [ ] Assign unique id to `<example>`.
  - [ ] Powershell script to build and execute all `<example>` to get output
  - [ ] Combine source and output and convert `<example>` to `<program>`
  - [ ] Call `WebsiteSource` to generate website
  - [ ] Copy website to `vczh-libraries.github.io`
- [ ] Refine document comment
  - [ ] Vlpp
  - [ ] VlppOS
  - [ ] VlppRegex
  - [ ] VlppReflection
  - [ ] VlppParser
  - [ ] Workflow
  - [ ] GacUI
- [ ] Connect reflection symbols to C++ symbols
- [ ] Author document (Separate document tree and reference tree)
  - [ ] Vlpp
  - [ ] VlppOS
  - [ ] VlppRegex
  - [ ] VlppReflection
  - [ ] VlppParser
    - [ ] Syntax
  - [ ] Workflow
    - [ ] Syntax
  - [ ] GacUI
    - [ ] XML Syntax
    - [ ] XML virtual class list
    - [ ] XML class to C++ class mapping

### Test Cases

- [ ] `this` used in field initializer.

### Bugs

- [ ] When `ParseIdExpr` is called for `NAME::`, it needs to tell `ResolveSymbolInContext` to always ignore value symbols.
- [ ] Create symbol for `friend class X;`.
- [ ] Fix memory leaks.
- [ ] **STL** Demo
  - [ ] Remove special handling for `common_type_t<...>`
- [ ] **TypePrinter** Demo
  - [ ] Ensure that `X<T>` in `template<typename T> void X<T>::F(){}` points to the correct partial specializations.
- [ ] Move `__forceinline` before declarator name to declaration.
  - [ ] Along with `virtual` and everything that could happen before a name.

### Features

- [ ] Core
  - [ ] `GenericExpr` allows to be partially applied only when `GenericExpr` is directly in a `FuncAccessExpr::expr`.
  - [ ] Consider about default values when matching partial specializations (both in GenericType and SpecializationSpec).
  - [ ] `IFT_InferTemplateArgument.cpp` `Visit(GenericType* self)` able to process multiple primary instances.
  - [ ] `MatchPSArgument` on `decltype(EXPR)`, success when the type to match is not `decltype(EXPR)`, or two expression match.
  - [ ] `IsSameResolvedExprVisitor` on lambda expressions.
  - [ ] `std::initialization_list`.
  - [ ] Converting lambda expression to a struct instead of `any_t`.
  - [ ] `auto` parameters on lambda expression.
  - [ ] Pass variadic template argument to placement new expression.
  - [ ] Create multiple instances if `T<THIS>` is evaluated to multiple type.
  - [ ] `GenerateMembers` on `DeclInstance`.
    - [ ] Generate `ClassMemberCache` for generated special members.
    - [ ] Generate special members for declarations with `specializationSpec`.
- [ ] Render
  - [ ] Add link to `#include "File"` and `#include <File>`.
  - [ ] Attach document content to declarations.
  - [ ] Enable markdown inside xml elements in comment.
  - [ ] Resolve symbols in markdown in comment.
  - [ ] Enable `<example>` with predefined project types, extract all example, compile and index them.
  - [ ] Refactor the HTML generation part to becomes a library.
    - [ ] Extract `<div>` token rendering functions.

## Lexical Conventions

Consumable UTF-32 code points:

- Punctuators: `_ { } [ ] # ( ) < > % : ; . ? * + - / ^ & | ~ ! = , \ " '`
- Legal characters in identifiers: `00A8, 00AA, 00AD, 00AF, 00B2-00B5, 00B7-00BA, 00BC-00BE, 00C0-00D6, 00D8-00F6, 00F8-00FF, 0100-02FF, 0370-167F, 1681-180D, 180F-1DBF, 1E00-1FFF, 200B-200D, 202A-202E, 203F-2040, 2054, 2060-206F, 2070-20CF, 2100-218F, 2460-24FF, 2776-2793, 2C00-2DFF, 2E80-2FFF, 3004-3007, 3021-302F, 3031-303F, 3040-D7FF, F900-FD3D, FD40-FDCF, FDF0-FE1F, FE30-FE44, FE47-FFFD, 10000-1FFFD, 20000-2FFFD, 30000-3FFFD, 40000-4FFFD, 50000-5FFFD, 60000-6FFFD, 70000-7FFFD, 80000-8FFFD, 90000-9FFFD, A0000-AFFFD, B0000-BFFFD, C0000-CFFFD, D0000-DFFFD, E0000-EFFFD`
- Legal characters in identifiers except the first: `0300-036F, 1DC0-1DFF, 20D0-20FF, FE20-FE2F`
- Keywords: [Keywords](https://docs.microsoft.com/en-us/cpp/cpp/keywords-cpp?view=vs-2017)
- Integers: `(\d+('?\d+)*|0x[0-9a-fA-F]+)[uU]?[lL]?[lL]?`
- Floating: `(\d+\.|\.\d+|\d+.\d+)([eE][+-]?\d+)?[fFlL]?`
- Binaries: `0[bB]\d+`
- Strings: `(u|U|L|u8)?"([^\"]|\\.)*"`
- Raw Strings: `(u|U|L|u8)?R"<name>([anything that is not ')<name>"']*)<name>"`
- Characters: `(u|U|L|u8)?'([^\']|\\.)'`
- User-Defined Literals: Integer, character, float-point, boolean and string literals followed by an identifier without any delimiter.
- Comment:
  - `//[^/\r\n]?[^\r\n]*\r?\n`
  - `/\*([^*]|\*+[^*/])*\*/`
  - `///[^\r\n]*\r?\n`: It can be put before any declaration for documentation

## EBNF for this Document

- `[ X ]`: optional
- `{ X }`: repeating 0+ times
  - `{ X }+`: `X {X}`
  - `{ X Y ... }`: `[X {Y X}]`
  - `{ X Y ... }+`: `X {Y X}`
- `( X )`: priority
- `X | Y`: alternation

## Supported C++ Syntax

- Type parser function: parse a type, with zero, optional or more declarator.
  - zero declarator: still need a declarator, but it cannot have an identity name. e.g. type in generic type
  - optional declarator: the declarator has optional identity name. e.g. function parameter
  - else: each declarator should have an identity name. e.g. variable declaration
- Declarator parser function: parse a type, with (maybe) optional identity name.

### SPECIFIERS

Specifiers can be put before any declaration, it will be ignored by the tool

- attributes: e.g. `[[noreturn]]`
- `__declspec( ... )`

### QUALIFIERS

- {`constexpr` | `const` | `volatile` | `&` | `&&`}+

### CALL

- `__cdecl`
- `__clrcall`
- `__stdcall`
- `__fastcall`
- `__thiscall`
- `__vectorcall`

### EXCEPTION-SPEC

- `noexcept`
- `throw` `(` {TYPE `,` ...} `)`

### INITIALIZER

- `=` EXPR
- `{` {EXPR `,` ...} `}`
- `(` {EXPR `,` ...} `)`
  - When this initializer is ambiguous a function declaration, the initializer wins.

### FUNCTION-TAIL

- `(` {TYPE-OPTIONAL [INITIALIZER] `,` ...} `)` {QUALIFIERS | EXCEPTION-SPEC | `->` TYPE | `override`)}

### DECLARATOR

- `operator` OPERATOR
- IDENTIFIER [SPECIALIZATION-SPEC]: Only allowed for functions
- SPECIFIERS DECLARATOR
- CALL DECLARATOR
- `alignas` `(` EXPR `)` DECLARATOR
- TYPE `::` DECLARATOR
- `(` DECLARATOR `)`
- (`*` [`__ptr32` | `__ptr64`] | `&` | `&&`) DECLARATOR
- (`constexpr` | `const` | `volatile`) DECLARATOR
- DECLARATOR `[` [EXPR] `]`
- DECLARATOR FUNCTION-TAIL

### TEMPLATE-SPEC

- `template` `<` {TEMPLATE-SPEC-ITEM `,` ...} `>`
- **TEMPLATE-SPEC-ITEM**:
  - TYPE-OPTIONAL-INITIALIZER
  - (`template`|`class`) [`...`] [IDENTIFIER] [`=` TYPE]
  - TEMPLATE-SPEC `class` [IDENTIFIER] [`=` TYPE]

### SPECIALIZATION-SPEC

- `<` {TYPE | EXPR} `>`

### FUNCTION

- [TEMPLATE-SPEC] {`static` | `virtual` | `explicit` | `inline` | `__forceinline`} TYPE-SINGLE (`;` | STAT) [ = (`0` | `default` | `delete`)
  - TEMPLATE-SPEC
  - SPECIALIZATION-SPEC
- [TEMPLATE-SPEC] {`static` | `virtual` | `explicit` | `inline` | `__forceinline`} `operator` TYPE-ZERO (`;` | STAT) [ = (`0` | `default` | `delete`)
  - TEMPLATE-SPEC

### OBJECT

- [TEMPLATE-SPEC] (`class` | `struct`) [[SPECIFIERS] IDENTIFIER [SPECIALIZATION-SPEC]] [`abstract`] [`:` {TYPE `,` ...}+] [`{` {DECL} `}`
  - TEMPLATE-SPEC: Not allowed when the class is defined after `typedef`, or is anonymous followed by variable declaration.
  - SPECIALIZATION-SPEC
- `enum` [`class` | `struct`] [[SPECIFIERS]IDENTIFIER] [`:` TYPE] [`{` {IDENTIFIER [`=` EXPR] `,` ...} [`,`] `}`
- [TEMPLATE-SPEC] `union` [[SPECIFIERS]IDENTIFIER [SPECIALIZATION-SPEC]] [`{` {DECL} `}`
  - TEMPLATE-SPEC: Not allowed when the class is defined after `typedef`, or is anonymous followed by variable declaration.
  - SPECIALIZATION-SPEC

### DECL (Declaration)

- **Friend**: `friend` DECL `;`
- **Extern**" `extern` [STRING] (DECL `;` | `{` {DECLARATION ...} `}`)
- **Type definition**: (CLASS_STRUCT | ENUM | UNION) {DECLARATOR [INITIALIZER] `,` ...}+ `;`
- **Type alias**:
  - `typedef` (CLASS_STRUCT | ENUM | UNION) {DECLARATOR `,` ...}+ `;` (**no template**)
  - `typedef` TYPE-MULTIPLE-INITIALIZER `;` (**no template**)
    - TEMPLATE-SPEC and SPECIALIZATION-SPEC are disallowed here
- **Type definition**: [TEMPLATE-SPEC] `using` IDENTIFIER `=` TYPE `;` (**no specialization**)
  - TEMPLATE-SPEC
- **Import**: `using` { [`typename`] [TYPE `::` IDENTIFIER] `,` ...} `;`
- **Variable**: {`register` | `static` | `thread_local` | `mutable`} TYPE-MULTIPLE-INITIALIZER `;`
  - TEMPLATE-SPEC: for value aliases
- **Namespace** `namespace` {IDENTIFIER `::` ...}+ `{` {DECLARATION} `}`
- **Ctor, Dtor**: [`~`] IDENTIFIER ({TYPE [DECLARATOR] [INITIALIZER] `,` ...}) [EXCEPTION-SPEC] STAT
- FUNCTION

## TYPE (Type)

- `auto`
- `decltype` `(` (EXPR) `)`
- (`constexpr` | `const` | `volatile`) TYPE
- TYPE (`constexpr` | `const` | `volatile`)
- `void` | `bool`
- `char` | `wchar_t` | `char16_t` | `char32_t`
- [`signed` | `unsigned`] (`__int8` | `__int16` | `__int32` | `__int64` | `__m64` | `__m128` | `__m128d` | `__m128i`)
- [TYPE `::` [`typename`]] IDENTIFIER
- TYPE `<` {(TYPE | EXPR) `,` ...}+ `>`
- TYPE `...`

## STAT (Statement)

- IDENTIFIER `:` STAT
- `default` `:` STAT
- `case` EXPR `:` STAT
- `;`
- `{` {STAT ...} `}`
- {EXPR `,` ...}+ `;`
- DECL
- `break` `;`
- `continue` `;`
- `while` `(` EXPR `)` STAT
- `do` STAT `while` `(` EXPR `)` `;`
- `for` ([TYPE {[DECLARATOR] [INITIALIZER] `,` ...}] `;` [EXPR] `;` [EXPR]) STAT
- `for` (TYPE-SINGLE-DECLARATOR `:` EXPR) STAT
- `if` [`constexpr`] `(`[STAT `;`] [TYPE IDENTIFIER `=`] EXPR `)` STAT [`else` STAT]
- `switch` `(` {STAT} EXPR `)` `{` STAT `}`
- `try` STAT `catch` `(` TYPE-OPTIONAL-DECLARATOR `)` STAT
- `return` EXPR `;`
- `goto` IDENTIFIER `;`
- `__try` STAT `__except` `(` EXPR `)` STAT
- `__try` STAT `__finally` STAT
- `__leave` `;`
- (`__if_exists` | `__if_not_exists`) `(` EXPR `)` STAT

## EXPR (Expression)

- LITERAL
- `{` ... `}`
- `this`
- `nullptr`
- IDENTIFIER
- `::` IDENTIFIER
- TYPE `::` IDENTIFIER
- `(` EXPR-WITH-COMMA `)`
- OPERATOR EXPR
  - predefined operators
- EXPR OPERATOR
  - EXPR `(` {EXPR `,` ...} `)`
  - EXPR `[` EXPR `]`
  - EXPR `<` {TYPE `,` ...} `>`
  - predefined operators
- EXPR OPERATOR EXPR
  - EXPR (`.` | `->`) IDENTIFIER
  - EXPR (`.*` | `->*`) EXPR
  - predefined operators
- EXPR `?` EXPR `:` EXPR
- (`dynamic_cast` | `static_cast` | `const_cast` | `reinterpret_cast` | `safe_cast`) `<` TYPE `>` `(` EXPR `)`
- `typeid` `(` (TYPE | EXPR) `)`
- `sizeof` [`...`] EXPR
  - `...`
- `sizeof` [`...`] `(` TYPE `)`
  - `...`
- `(` TYPE `)` EXPR
- `new` [`(` {EXPR `,` ...}+ `)`] TYPE [`(` {EXPR `,` ... } `)` | [`{` {EXPR `,` ... } `}`]]
- `delete` [`[` `]`] EXPR
- `throw` EXPR
- EXPR `...`
- `::new`
- `::delete`
- `[` {`&` | `=` | [IDENTIFIER `=`] EXPR | } `]` FUNCTION-TAIL STAT

### Operators

[Built-in Operators, Precedence and Associativity](https://docs.microsoft.com/en-us/cpp/cpp/cpp-built-in-operators-precedence-and-associativity?view=vs-2017)

### Precedence Groups

1. : primitive
2. : `::`
3. : `.` `->` `[]` `()` `x++` `x--`
4. (<-): `sizeof` `new` `delete` `++x` `--x` `~` `!` `-x` `+x` `&x` `*x` `(T)E`
5. : `.*` `->*`
6. : `*` `/` `%`
7. : `+` `-`
8. : `<<` `>>`
9. : `<` `>` `<=` `>=`
10. : `==` `!=`
11. : `&`
12. : `^`
13. : `|`
14. : `&&`
15. : `||`
16. (<-): `a?b:c`
17. (<-): `=` `*=` `/=` `%=` `+=` `-=` `<<=` `>>=` `&=` `|=` `^=`
18. (<-): `throw`
19. : `,`
