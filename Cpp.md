# Goal
- This is a code index tool, which means it doesn't narrow candidates using type inference, it shows every possible candidates instead.
- The tool consumes a single preprocessed C++ source file, the following syntax is collected for this purpose.
- The syntax has less restrictions than the real C++ language specification, because the tool assumes the input is correct.

# Steps
- [x] Parse non-resolving types
- [x] `Namespace`s, `Enum`s, `Variable`s `Forward Variable`s and `Forward Function`s
  - [x] Decorators of variables and functions: `friend`, `extern`, `static`, `mutable`, `thread_local`, `register`, `virtual`, `explicit`, `inline`, `__forceinline`.
  - [ ] Assert ASTs
- [ ] Parse qualified types and member pointer types (`::`)
  - [ ] Assert ASTs
  - [ ] Assert resolvings
- [ ] `Class`es, `Struct`s, `Union`s
  - [ ] Assert ASTs
  - [ ] Assert resolvings in scopes
- [ ] `Forward Declaration`s
  - [ ] Assert ASTs
  - [ ] Assert resolvings in scopes
- [ ] Parse function body and Statements
  - [ ] Assert ASTs
- [ ] `Using` namespaces and symbols
  - [ ] Assert ASTs
  - [ ] Assert resolvings in scopes
- [ ] Parse full expressions
  - [ ] Assert ASTs
  - [ ] Assert resolvings in scopes
- [ ] `Template` and `Typedef`
  - [ ] Assert ASTs
- [ ] Parse ambiguious expressions and types with generic
  - [ ] Assert ASTs
  - [ ] Assert resolvings in scopes
- [ ] Preprocess `Preprocessed.txt` to get rid of `#line`s and save line informations to another structure
- [ ] Parse `Preprocessed.txt`
  - [ ] Skip any other structures like `#pragma`
- [ ] Attach document content to declarations
- [ ] Parse `Preprocessed.txt`
- [ ] Save index and document result to another file
- [ ] Write markdown parse to understand comments
- [ ] Resolve symbols in markdown in comment
- [ ] Markdown to book compiler

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


# Declarations
- Type parser function: parse a type, with zero, optional or more declarator.
  - zero declarator: still need a declarator, but it cannot have an identity name. e.g. type in generic type
  - optional declarator: the declarator has optional identity name. e.g. function parameter
  - else: each declarator should have an identity name. e.g. variable declaration
- Declarator parser function: parse a type, with (maybe) optional identity name.

## SPECIFIERS
Specifiers can be put before any declaration, it will be ignored by the tool
- attributes: e.g. `[[noreturn]]`
- `__declspec( ... )`

## QUALIFIERS
- {`constexpr` | `const` | `volatile` | `&` | `&&`}+

## CALL
- [x] `__cdecl`
- [x] `__clrcall`
- [x] `__stdcall`
- [x] `__fastcall`
- [x] `__thiscall`
- [x] `__vectorcall`

## EXCEPTION-SPEC
- `noexcept`
- `throw` `(` {TYPE `,` ...} `)`

## INITIALIZER
- [x] `=` EXPR
- [x] `{` {EXPR `,` ...} `}`
- [x] `(` {EXPR `,` ...} `)`
  -  When this initializer is ambiguous a function declaration, the initializer wins.

## FUNCTION-TAIL
- [x] `(` {TYPE-OPTIONAL [INITIALIZER] `,` ...} `)` {QUALIFIERS | EXCEPTION-SPEC | `->` TYPE | `override`}
  - `= 0` will be in the initializer

## DECLARATOR
- [x] `operator` OPERATOR
- [ ] IDENTIFIER [SPECIALIZATION-SPEC]
- [x] SPECIFIERS DECLARATOR
- [x] CALL DECLARATOR
- [x] `alignas` `(` EXPR `)` DECLARATOR
- [ ] TYPE `::` DECLARATOR
  - The qualifiers here decorate the identifier, not the this pointer.
- [x] `(` DECLARATOR `)`
- [x] (`*` [`__ptr32` | `__ptr64`] | `&` | `&&`) DECLARATOR
- [x] (`constexpr` | `const` | `volatile`) DECLARATOR
- [x] DECLARATOR `[` [EXPR] `]`
- [x] DECLARATOR FUNCTION-TAIL

## TEMPLATE-SPEC
- `template` `<` {TEMPLATE-SPEC-ITEM `,` ...} `>`
- **TEMPLATE-SPEC-ITEM**:
  - TYPE-OPTIONAL-INITIALIZER
  - (`template`|`class`) [`...`] [IDENTIFIER] [`=` TYPE]
  - TEMPLATE-SPEC `class` [IDENTIFIER] [`=` TYPE]

## SPECIALIZATION-SPEC
- `<` {TYPE | EXPR} `>`

## FUNCTION
- [ ] [TEMPLATE-SPEC] {`static` | `virtual` | `explicit` | `inline` | `__forceinline`} TYPE-SINGLE (`;` | STAT)
- [ ] [TEMPLATE-SPEC] {`static` | `virtual` | `explicit` | `inline` | `__forceinline`} `operator` TYPE-ZERO (`;` | STAT)

## CLASS_STRUCT
- [ ] [TEMPLATE-SPEC] (`class` | `struct`) [[SPECIFIERS] IDENTIFIER [SPECIALIZATION-SPEC]] [`abstract`] [`:` {TYPE `,` ...}+] [`{` {DECL} `}`

## ENUM
- [ ] `enum` [`class` | `struct`] [[SPECIFIERS]IDENTIFIER] [`:` TYPE] [`{` {IDENTIFIER [`=` EXPR] `,` ...} [`,`] `}`

## UNION
- [ ] [TEMPLATE-SPEC] `union` [[SPECIFIERS]IDENTIFIER [SPECIALIZATION-SPEC]] [`{` {DECL} `}`

## DECL
- [ ] **Friend**: `friend` DECL `;`
- [ ] **Extern**" `extern` [STRING] (DECL `;` | `{` {DECLARATION ...} `}`)
- [ ] **Type definition**: (CLASS_STRUCT | ENUM | UNION) {DECLARATOR [INITIALIZER] `,` ...}+ `;`
  [ ] - TEMPLATE-SPEC and SPECIALIZATION-SPEC are disallowed here
- [ ] **Type alias**:
  - [ ] `typedef` (CLASS_STRUCT | ENUM | UNION) {DECLARATOR `,` ...}+ `;`
  - [ ] `typedef` TYPE-MULTIPLE-INITIALIZER `;`
  - [ ] [TEMPLATE-SPEC] `using` NAME = TYPE-ZERO `;`
- [ ] **Type definition**: [TEMPLATE-SPEC] `using` IDENTIFIER `=` TYPE `;`
- [ ] **Import**: `using` { [`typename`] [TYPE `::` IDENTIFIER] `,` ...} `;`
- [ ] **Variable**: {`register` | `static` | `thread_local` | `mutable`} TYPE-MULTIPLE-INITIALIZER `;`
- [ ] **Namespace** `namespace` {IDENTIFIER `::` ...}+ `{` {DECLARATION} `}`
- [ ] **Ctor, Dtor**: [`~`] IDENTIFIER ({TYPE [DECLARATOR] [INITIALIZER] `,` ...}) [EXCEPTION-SPEC] STAT
- [ ] FUNCTION

# TYPE (Type)
- [x] `auto`
- [x] `decltype` `(` (EXPR) `)`
- [x] (`constexpr` | `const` | `volatile`) TYPE
- [x] TYPE (`constexpr` | `const` | `volatile`)
- [x] `void` | `bool`
- [x] `char` | `wchar_t` | `char16_t` | `char32_t`
- [x] [`signed` | `unsigned`] (`__int8` | `__int16` | `__int32` | `__int64` | `__m64` | `__m128` | `__m128d` | `__m128i`)
- [ ] [TYPE `::` [`typename`]] IDENTIFIER
- [x] TYPE `<` {(TYPE | EXPR) `,` ...}+ `>`
- [x] TYPE `...`

# STAT (Statement)
- [ ] IDENTIFIER `:` STAT
- [ ] `default` `:` STAT
- [ ] `case` EXPR `:` STAT
- [ ] `;`
- [ ] `{` {STAT ...} `}`
- [ ] {EXPR `,` ...}+ `;`
- [ ] DECL
- [ ] `break` `;`
- [ ] `continue` `;`
- [ ] `while` `(` EXPR `)` STAT
- [ ] `do` STAT `while` `(` EXPR `)` `;`
- [ ] `for` ([TYPE {[DECLARATOR] [INITIALIZER] `,` ...}] `;` [EXPR] `;` [EXPR]) STAT
- [ ] `for` (TYPE-SINGLE-DECLARATOR `:` EXPR) STAT
- [ ] `if` [`constexpr`] `(` [TYPE IDENTIFIER `=`] EXPR `)` STAT [`else` STAT]
- [ ] `switch` `(` {STAT} EXPR `)` `{` STAT `}`
- [ ] `try` STAT `catch` `(` TYPE-OPTIONAL-DECLARATOR `)` STAT
- [ ] `return` EXPR `;`
- [ ] `goto` IDENTIFIER `;`
- [ ] `__try` STAT `__except` `(` EXPR `)` STAT
- [ ] `__try` STAT `__finally` STAT
- [ ] `__leave` `;`
- [ ] (`__if_exists` | `__if_not_exists`) `(` EXPR `)` STAT

# EXPR (Expression)
- [ ] LITERAL
- [ ] `this`
- [ ] `nullptr`
- [ ] [TYPE `::`] [`~`] IDENTIFIER
- [ ] `(` EXPR `)`
- [ ] EXPR (`.` | `->` | `.*` | `->*`) IDENTIFIER
- [ ] EXPR `(` {EXPR `,` ...} `)`
- [ ] EXPR `[` EXPR `]`
- [ ] EXPR `...`
- [ ] Prefix / Postfix unary operator expressions
- [ ] Binary operator expressions
- [ ] EXPR `?` EXPR `:` EXPR
- [ ] (`dynamic_cast` | `static_cast` | `const_cast` | `reinterpret_cast` | `safe_cast`) `<` TYPE `>` `(` EXPR `)`
- [ ] (`typeid` | `sizeof`) `(` (EXPR | TYPE) `)`
- [ ] `(` TYPE `)` EXPR
- [ ] [`::`] `new` [`(` {EXPR `,` ...}+ `)`] TYPE [`(` {EXPR `,` ... } `)` | [`{` {EXPR `,` ... } `}`]]
- [ ] [`::`] `delete` [`[` `]`] EXPR
- [ ] `throw` EXPR
- [ ] `[` {`&` | `=` | [IDENTIFIER `=`] EXPR | } `]` FUNCTION-TAIL STAT

## Operators:
[Built-in Operators, Precedence and Associativity](https://docs.microsoft.com/en-us/cpp/cpp/cpp-built-in-operators-precedence-and-associativity?view=vs-2017)
