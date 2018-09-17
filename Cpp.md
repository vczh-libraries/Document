# Goal
- This is a code index tool, which means it doesn't narrow candidates using type inference, it shows every possible candidates instead.
- The tool consumes a single preprocessed C++ source file, the following syntax is collected for this purpose.
- The syntax has less restrictions than the real C++ language specification, because the tool assumes the input is correct.

# Lexical Conventions
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

# EBNF
- `[ X ]`: optional
- `{ X }`: repeating 0+ times
  - `{ X }+`: `X {X}`
  - `{ X Y ... }`: `[X {Y X}]`
  - `{ X Y ... }+`: `X {Y X}`
- `( X )`: priority
- `X | Y`: alternation


# DECL (Declaration)
A type parser function has a parameter for declarator configurations.

## QUALIFIERS
- {`constexpr` | `const` | `volatile` | `&` | `&&`}+

## Specifiers
Specifiers can be put before any declaration, it will be ignored by the tool
- attributes: e.g. `[[noreturn]]`
- `__declspec( ... )`

## INITIALIZER
- `=` EXPR
- `{` {EXPR `,` ...} `}`
- `(` {EXPR `,` ...} `)`
  -  When this initializer is ambiguous a function declaration, the initializer wins.

## DECLARATOR
Declarator starts as early as it can
- [TYPE `::`] IDENTIFIER
- [CALL] TYPE `::` `*` [`alignas` `(` EXPR `)`] [CALL] [QUALIFIERS] [CALL] IDENTIFIER
  - The qualifiers here decorate the identifier, not the this pointer.
- `(` DECLARATOR `)`
- (`*` [`__ptr32` | `__ptr64`] | `&` | `&&`) [QUALIFIERS] DECLARATOR
- DECLARATOR `[` [EXPR] `]`
- DECLARATOR `(` {TYPE [DECLARATOR] [INITIALIZER] `,` ...} `)` [QUALIFIERS] [EXCEPTION-SPEC] [`->` `decltype` `(` (EXPR) `)`]

## Simple Declarations
- **Friend**: `friend` DECL `;`
- **Extern**" `extern` [STRING] (DECL `;` | `{` {DECLARATION ...} `}`)
- **Type definition**: `typedef` TYPE {DECLARATOR `,` ...}+ `;`
- **Type definition**: [TEMPLATE-SPEC] `using` IDENTIFIER `=` TYPE `;`
- **Import**: `using` { [`typename`] [TYPE `::` IDENTIFIER] `,` ...} `;`
- **Variable**: {`register` | `static` | `thread_local`} TYPE {DECLARATOR [INITIALIZER] `,` ...}+ `;`
- **Namespace** `namespace` {IDENTIFIER `::` ...}+ `{` {DECLARATION} `}`

## Function

## Struct / Class

# TYPE (Type)
- `auto`
- (`constexpr` | `const` | `volatile`) TYPE <declarators-here>
- TYPE (`constexpr` | `const` | `volatile`) <declarators-here>
- TYPE (`*` | `&` | `&&`) <declarators-here>
- `void` | `bool`
- `char` | `wchar_t` | `char16_t` | `char32_t`
- [`signed` | `unsigned`] (`__int8` | `__int16` | `__int32` | `__int64` | `__m64` | `__m128` | `__m128d` | `__m128i`)
- [TYPE `::`] IDENTIFIER
- TYPE `<` {(TYPE | EXPR) `,` ...}+ `>`
- TYPE `...`

# STAT (Statement)
- {EXPR `,` ...}+

# EXPR (Expression)
- LITERAL
- `this`
- [TYPE `::`] IDENTIFIER
- `(` EXPR `)`
- EXPR (`.` | `->` | `.*` | `->*`) IDENTIFIER
- EXPR `(` {EXPR `,` ...} `)`
- EXPR `[` EXPR `]`
- EXPR `...`
- Prefix / Postfix unary operator expressions
- Binary operator expressions
- EXPR `?` EXPR `:` EXPR
- (`dynamic_cast` | `static_cast` | `const_cast` | `reinterpret_cast` | `safe_cast`) `<` TYPE `>` `(` EXPR `)`
- (`typeid` | `sizeof`) `(` (EXPR | TYPE) `)`
- `(` TYPE `)` EXPR
- [`::`] `new` [`(` {EXPR `,` ...}+ `)`] TYPE [`(` {EXPR `,` ... } `)` | [`{` {EXPR `,` ... } `}`]]
- [`::`] `delete` [`[` `]`] EXPR
- `throw` EXPR

## Operators:
[Built-in Operators, Precedence and Associativity](https://docs.microsoft.com/en-us/cpp/cpp/cpp-built-in-operators-precedence-and-associativity?view=vs-2017)
