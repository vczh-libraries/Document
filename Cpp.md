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

## SPECIFIERS
Specifiers can be put before any declaration, it will be ignored by the tool
- attributes: e.g. `[[noreturn]]`
- `__declspec( ... )`

## QUALIFIERS
- {`constexpr` | `const` | `volatile` | `&` | `&&`}+

## CALL
- `__cdecl`
- `__clrcall`
- `__stdcall`
- `__fastcall`
- `__thiscall`
- `__vectorcall`

## EXCEPTION-SPEC
- `noexcept`
- `nothrow`
- `throw` `(` {TYPE `,` ...} `)`

## INITIALIZER
- `=` EXPR
- `{` {EXPR `,` ...} `}`
- `(` {EXPR `,` ...} `)`
  -  When this initializer is ambiguous a function declaration, the initializer wins.

## FUNCTION-TAIL
- `(` {TYPE [DECLARATOR] [INITIALIZER] `,` ...} `)` {QUALIFIERS | EXCEPTION-SPEC | `->` TYPE | `override` | `=` `0` | `constexpr` | `mutable`}

## DECLARATOR
Declarator starts as early as it can
- IDENTIFIER
- SPECIFIERS DECLARATOR
- CALL DECLARATOR
- QUALIFIERS DECLARATOR
- `alignas` `(` EXPR `)` DECLARATOR
- TYPE `::` [`*`] DECLARATOR
  - The qualifiers here decorate the identifier, not the this pointer.
- `(` DECLARATOR `)`
- (`*` [`__ptr32` | `__ptr64`] | `&` | `&&`) DECLARATOR
- DECLARATOR `[` [EXPR] `]`
- DECLARATOR FUNCTION-TAIL

## Simple Declarations
- **Friend**: `friend` DECL `;`
- **Extern**" `extern` [STRING] (DECL `;` | `{` {DECLARATION ...} `}`)
- **Type definition**: `typedef` TYPE {DECLARATOR `,` ...}+ `;`
- **Type definition**: [TEMPLATE-SPEC] `using` IDENTIFIER `=` TYPE `;`
- **Import**: `using` { [`typename`] [TYPE `::` IDENTIFIER] `,` ...} `;`
- **Variable**: {`register` | `static` | `thread_local` | `mutable`} TYPE {DECLARATOR [INITIALIZER] `,` ...}+ `;`
- **Namespace** `namespace` {IDENTIFIER `::` ...}+ `{` {DECLARATION} `}`
- **Ctor, Dtor**: [`~`] IDENTIFIER ({TYPE [DECLARATOR] [INITIALIZER] `,` ...}) [EXCEPTION-SPEC] STAT

## TEMPLATE-SPEC

## SPECIALIZATION-SPEC

## Function
- [`static` | `virtual`] TYPE-SINGLE-DECLARATOR (`;` | STAT)

## Struct / Class
- [TEMPLATE-SPEC] (`class` | `struct`) [[SPECIFIERS] IDENTIFIER [SPECIALIZATION-SPEC]] [`abstract`] [`:` {TYPE `,` ...}+] [`{` {IDENTIFIER [`=` EXPR] `,` ...} [`,`] `}` {DECLARATOR [INITIALIZER] `,` ...}] `;`

## Enum
- `enum` [`class` | `struct`] [[SPECIFIERS]IDENTIFIER] [`:` TYPE] [`{` {IDENTIFIER [`=` EXPR] `,` ...} [`,`] `}` {DECLARATOR [INITIALIZER] `,` ...}] `;`

## Union
- [TEMPLATE-SPEC] `union` [[SPECIFIERS]IDENTIFIER [SPECIALIZATION-SPEC]] [`{` {DECL} `}` {DECLARATOR [INITIALIZER] `,` ...}] `;`

# TYPE (Type)
- `auto`
- `decltype` `(` (EXPR) `)`
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
- `if` [`constexpr`] `(` [TYPE IDENTIFIER `=`] EXPR `)` STAT [`else` STAT]
- `switch` `(` {STAT} EXPR `)` `{` STAT `}`
- `try` STAT `catch` `(` TYPE-OPTIONAL-DECLARATOR `)` STAT
- `return` EXPR `;`
- `goto` IDENTIFIER `;`
- `__try` STAT `__except` `(` EXPR `)` STAT
- `__try` STAT `__finally` STAT
- `__leave` `;`
- (`__if_exists` | `__if_not_exists`) `(` EXPR `)` STAT

# EXPR (Expression)
- LITERAL
- `this`
- `nullptr`
- [TYPE `::`] [`~`] IDENTIFIER
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
- `[` {`&` | `=` | [IDENTIFIER `=`] EXPR | } `]` FUNCTION-TAIL STAT

## Operators:
[Built-in Operators, Precedence and Associativity](https://docs.microsoft.com/en-us/cpp/cpp/cpp-built-in-operators-precedence-and-associativity?view=vs-2017)
