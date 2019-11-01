# Draft

- The tool consumes a single preprocessed C++ source file, the following syntax is collected for this purpose.
- The syntax has less restrictions than the real C++ language specification, because the tool assumes the input is correct.

## Refactoring Work Items

- [ ] Add exception messages so that the final application won't crash leaving nothing.
- [ ] `CreateIdReferenceExpr`: Remove dependency to `Ptr<Resolving>`
- [ ] `FindMembersByName`: Remove dependency to `ResolveSymbolResult`

## TODO

- [ ] Next Demo!
  - [x] Connect `template<typename T> class X;` with `template<typename T> class X{};`.
  - [x] Parse `template<typename T> template<typename U> template<typename V> void A<T>::B<U>::F(){}`.
    - store all `template<...>` for classes in `FunctionDeclaration`
    - matches `template<typename X>class A { template<typename Y>class B { void template<typename Z>F(); }; };`.
  - [ ] `GenericExpr` on `FieldAccessExpr` (which is illegal now, `\.Cast<(Id|Child)Expr>`)
    - [x] Extract function: given `types` and `typeVta` and evaluate expression `types<...>`.
      - `VariaditInput<>::ApplyTypes` add `isVta` parameter.
    - [x] Rename `ResolvableType` to `Category_Id_Child_Type`.
    - [x] Rename `ResolvableExpr` to `Category_Id_Child_Expr`.
      - inheriting from `Category_Id_Child_Generic_Expr`.
    - [x] `FuncAccessExpr` take care of `Category_Id_Child_Generic_FieldAccess_Expr`.
      - [x] `Ast_Evaluate_ExprToTsys.cpp` -> `void Visit(FuncAccessExpr* self)override`: adding hyper linkes. Cast to `Category_Id_Child_Generic_FieldAccess_Expr` first and then deal with all possible situations.
    - [ ] `FieldAccessExpr` take care of `Category_Id_Child_Generic_Expr`.
      - [x] Change `name` from `ResolvableExpr` to `Category_Id_Child_Generic_Expr`.
      - [ ] `Ast_Evaluate_ExprToTsys.cpp` -> `void Visit(FieldAccessExpr* self)override`: at the beginning.
        - [x] Use `MatchCategoryExpr`
        - [x] Separate `ProcessFieldAccessExpr` by `idExpr` and `childExpr`
        - [ ] Handle `GenericExpr`
      - [x] `Ast_Evaluate_ExprToTsys.cpp` -> `void Visit(FuncAccessExpr* self)override`: adding hyper linkes to a function call to `FieldAccessExpr`.
    - [x] `PrefixUnaryExpr` (AddressOf) takes care of `Category_Id_Child_Generic_Expr`.
      - [x] `Ast_Evaluate_ExprToTsys.cpp` -> `void Visit(PrefixUnaryExpr* self)override`: at the beginning to set `SearchPolicy::ChildSymbolFromOutside` (evaluating `ChildExpr` directly won't do this).
        - [x] Use `MatchCategoryExpr`
        - [x] Handle `GenericExpr to ChildExpr`
      - [x] `Ast_Evaluate_ToTsys_ExprImpl.cpp` -> `void ProcessPrefixUnaryExpr(...)`: `case CppPrefixUnaryOp::AddressOf`.
    - [x] Check types of `&Name<T>` and `&Class::Method<T>`.
    - [ ] Check types of `obj.Method` and `obj.Method<T>` for methods defined outside of classes.
    - [ ] Check types of `obj.X::Method` and `obj.X::Method<T>` for methods defined outside of classes.
    - [ ] Check `FuncAccessExpr`, `FieldAccessExpr`, `PrefixUnaryExpr` (AddressOf) for generating hyper links on generic expression.
  - [ ] `TestFunctionQualifier` should take care of `this` when it points to a generic type.
  - [ ] Inside `template<...> class X`, if `X` is used as a type without type arguments, it is filled with template arguments.
  - [ ] Function specializations recognized but not used
  - [ ] Class specializations recognized but not used
  - [ ] Parse `template<typename T> template<typename U> template<typename V> void A<T*>::B<const U&>::F(){}`
    - matches `template<typename X>class A<X*> { template<typename Y>class B<const Y&> { void template<typename Z>F(); }; };`
    - activate commented test cases in `TestParseGenericMember.cpp`
  - [ ] Parse `UnitTest_Cases`, generate HTML and check.
- [ ] More `template` on functions. [post](https://en.cppreference.com/w/cpp/language/function_template)
  - [ ] Call a function with some or all template arguments unspecified.
  - [ ] Overload functions with some or all template arguments unspecified.
  - [ ] `...` arguments
  - [ ] Full Specialization
    - [ ] Connect functions with forward declarations.
      - [ ] When there are constant arguments, the shape of the expression should match, considering `NameExpr` and `ChildExpr` identical.
    - [ ] When overloading, specialized functions are not considered. When a template function wins, then choose among the primiary and its specializations.
- [ ] More `template` on classes (partial specialization).
  - [ ] Test scenario: first see the forward declaration of a generic class and evaluate its type, and then see the implementation and evaluate its type again.
  - [ ] Connect methods with forward declarations inside multiple levels of template classes.
    - [ ] When there are constant arguments, the shape of the expression should match, considering `NameExpr` and `ChildExpr` identical.
  - [ ] With a `DeclInstant` is created, best possible choices are returned at the same time.
- [ ] SFINAE on choosing among generic functions or classes.
- [ ] `std::initialization_list`.
- [ ] Lambda expressions.
- [ ] `nullptr` failed to convert to `function_type * const &`.
- [ ] `decltype(EXPR)::ChildType`
- [ ] `::new`
- [ ] `::delete`
- [ ] Connect function with forward declarations containing `decltype(EXPR)` in expression type.
- [ ] Pass variadic template argument to placement new expression.
- [ ] `GenerateMembers` on `DeclInstance`.

## Finisher Work Items

- [ ] Parse `UnitTest_Cases`
- [ ] Update CodePack.exe to produce `#include` only header and cpp files, so that the compiler can index preprocessed files with `#line` directly, without having to parse CodePack.exe produced comments.
- [ ] Produce `Preprocessed.txt` from `#include` only files, not from compacted files.
- [ ] Generic HTML index. (multiple pages)
- [ ] Attach document content to declarations.
- [ ] Enable markdown inside xml elements in comment.
- [ ] Resolve symbols in markdown in comment.
- [ ] Enable `<example>` with predefined project types, extract all example, compile and index them.

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

- [x] `operator` OPERATOR
- [ ] IDENTIFIER [SPECIALIZATION-SPEC]: Only allowed for functions
- [x] SPECIFIERS DECLARATOR
- [x] CALL DECLARATOR
- [x] `alignas` `(` EXPR `)` DECLARATOR
- [x] TYPE `::` DECLARATOR
- [x] `(` DECLARATOR `)`
- [x] (`*` [`__ptr32` | `__ptr64`] | `&` | `&&`) DECLARATOR
- [x] (`constexpr` | `const` | `volatile`) DECLARATOR
- [x] DECLARATOR `[` [EXPR] `]`
- [x] DECLARATOR FUNCTION-TAIL

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
