#ifndef VCZH_DOCUMENT_CPPDOC_PARSER
#define VCZH_DOCUMENT_CPPDOC_PARSER

#include "Lexer.h"
#include "Ast.h"

class TemplateSpec;
class SpecializationSpec;
class ClassDeclaration;
class FunctionDeclaration;
class VariableDeclaration;
class FunctionType;

/***********************************************************************
Parser Functions
***********************************************************************/

struct StopParsingException
{
	Ptr<CppTokenCursor>								position;

	StopParsingException(Ptr<CppTokenCursor> _position) :position(_position) {}
};

enum class DeclaratorRestriction
{
	Zero,
	Optional,
	One,
	Many,
};

enum class InitializerRestriction
{
	Zero,
	Optional,
};

// Parser_ResolveSymbol.cpp
enum class SearchPolicy
{
	SymbolAccessableInScope,							// search a name in a bounding scope of the current checking place
	SymbolAccessableInScope_CStyleTypeReference,		// like above but it is explicitly required to be a type using "struct X"
	ChildSymbolFromOutside,								// search scope::name
	ChildSymbolFromSubClass,							// search the base class from the following two policy
	ChildSymbolFromMemberInside,						// search a name in a class containing the current member, when the member is declared inside the class
	ChildSymbolFromMemberOutside,						// search a name in a class containing the current member, when the member is declared outside the class
};

struct ResolveSymbolResult
{
	Ptr<Resolving>									values;
	Ptr<Resolving>									types;

	void											Merge(Ptr<Resolving>& to, Ptr<Resolving> from);
	void											Merge(const ResolveSymbolResult& rar);
};
extern ResolveSymbolResult							ResolveSymbol(const ParsingArguments& pa, CppName& name, SearchPolicy policy, ResolveSymbolResult input = {});
extern ResolveSymbolResult							ResolveChildSymbol(const ParsingArguments& pa, Ptr<Type> classType, CppName& name, ResolveSymbolResult input = {});

// Parser_Misc.cpp
extern bool											SkipSpecifiers(Ptr<CppTokenCursor>& cursor);
extern bool											ParseCppName(CppName& name, Ptr<CppTokenCursor>& cursor, bool forceSpecialMethod = false);
extern Ptr<Type>									GetTypeWithoutMemberAndCC(Ptr<Type> type);
extern Ptr<Type>									ReplaceTypeInMemberAndCC(Ptr<Type>& type, Ptr<Type> typeToReplace);
extern Ptr<Type>									AdjustReturnTypeWithMemberAndCC(Ptr<FunctionType> functionType);
extern bool											ParseCallingConvention(TsysCallingConvention& callingConvention, Ptr<CppTokenCursor>& cursor);

// Parser_Type.cpp
enum class ShortTypeTypenameKind
{
	No,
	Yes,
	Implicit,
};
extern Ptr<Type>									ParseShortType(const ParsingArguments& pa, ShortTypeTypenameKind typenameKind, Ptr<CppTokenCursor>& cursor);
extern Ptr<Type>									ParseLongType(const ParsingArguments& pa, Ptr<CppTokenCursor>& cursor);

#define PARSING_DECLARATOR_ARGUMENTS(PREFIX, DELIMITER)												\
	ClassDeclaration*								PREFIX##containingClass			DELIMITER		\
	Ptr<Symbol>										PREFIX##scopeSymbolToReuse		DELIMITER		\
	bool											PREFIX##forParameter			DELIMITER		\
	DeclaratorRestriction							PREFIX##dr						DELIMITER		\
	InitializerRestriction							PREFIX##ir						DELIMITER		\
	bool											PREFIX##allowBitField			DELIMITER		\
	bool											PREFIX##allowEllipsis			DELIMITER		\
	bool											PREFIX##allowComma				DELIMITER		\
	bool											PREFIX##allowGtInInitializer	DELIMITER		\
	bool											PREFIX##allowSpecializationSpec					\

#define PARSING_DECLARATOR_COPY(PREFIX)										\
	containingClass(PREFIX##containingClass)								\
	, scopeSymbolToReuse(PREFIX##scopeSymbolToReuse)						\
	, forParameter(PREFIX##forParameter)									\
	, dr(PREFIX##dr)														\
	, ir(PREFIX##ir)														\
	, allowBitField(PREFIX##allowBitField)									\
	, allowEllipsis(PREFIX##allowEllipsis)									\
	, allowComma(PREFIX##allowComma)										\
	, allowGtInInitializer(PREFIX##allowGtInInitializer)					\
	, allowSpecializationSpec(PREFIX##allowSpecializationSpec)				\

// Parser_Declarator.cpp
struct ParsingDeclaratorArguments
{
	PARSING_DECLARATOR_ARGUMENTS(, ;);

#define __ ,
	ParsingDeclaratorArguments(PARSING_DECLARATOR_ARGUMENTS(_, __))
		: PARSING_DECLARATOR_COPY(_)
	{
	}
#undef __
};

#define PDA_HEADER(NAME) inline ParsingDeclaratorArguments	pda_##NAME
//																					class		symbol		param			declarator-count					initializer-count					bitfield		ellipsis		comma		=a>b	spec
PDA_HEADER(Type)			()										{	return {	nullptr,	nullptr,	false,			DeclaratorRestriction::Zero,		InitializerRestriction::Zero,		false,			false,			false,		false,	false	}; } // Type
PDA_HEADER(VarType)			()										{	return {	nullptr,	nullptr,	false,			DeclaratorRestriction::Optional,	InitializerRestriction::Zero,		false,			false,			false,		false,	false	}; } // Type or Variable without Initializer
PDA_HEADER(VarInit)			()										{	return {	nullptr,	nullptr,	false,			DeclaratorRestriction::One,			InitializerRestriction::Optional,	false,			false,			false,		true,	false	}; } // Variable with Initializer
PDA_HEADER(VarNoInit)		()										{	return {	nullptr,	nullptr,	false,			DeclaratorRestriction::One,			InitializerRestriction::Zero,		false,			false,			false,		false,	false	}; } // Variable without Initializer
PDA_HEADER(Param)			(bool forParameter)						{	return {	nullptr,	nullptr,	forParameter,	DeclaratorRestriction::Optional,	InitializerRestriction::Optional,	false,			forParameter,	false,		true,	false	}; } // Parameter
PDA_HEADER(TemplateParam)	()										{	return {	nullptr,	nullptr,	false,			DeclaratorRestriction::Optional,	InitializerRestriction::Optional,	false,			true,			false,		false,	false	}; } // Parameter
PDA_HEADER(Decls)			(bool allowBitField, bool allowComma)	{	return {	nullptr,	nullptr,	false,			DeclaratorRestriction::Many,		InitializerRestriction::Optional,	allowBitField,	false,			allowComma,	true,	true	}; } // Declarations
PDA_HEADER(Typedefs)		()										{	return {	nullptr,	nullptr,	false,			DeclaratorRestriction::Many,		InitializerRestriction::Zero,		false,			false,			false,		false,	false	}; } // Declarations after typedef keyword
#undef PDA_HEADER

extern void											ParseMemberDeclarator(const ParsingArguments& pa, const ParsingDeclaratorArguments& pda, Ptr<CppTokenCursor>& cursor, List<Ptr<Declarator>>& declarators);
extern void											ParseNonMemberDeclarator(const ParsingArguments& pa, const ParsingDeclaratorArguments& pda, Ptr<Type> type, Ptr<CppTokenCursor>& cursor, List<Ptr<Declarator>>& declarators);
extern void											ParseNonMemberDeclarator(const ParsingArguments& pa, const ParsingDeclaratorArguments& pda, Ptr<CppTokenCursor>& cursor, List<Ptr<Declarator>>& declarators);
extern Ptr<Declarator>								ParseNonMemberDeclarator(const ParsingArguments& pa, const ParsingDeclaratorArguments& pda, Ptr<CppTokenCursor>& cursor);
extern Ptr<Type>									ParseType(const ParsingArguments& pa, Ptr<CppTokenCursor>& cursor);

// Parser_Template.cpp
extern void											ParseTemplateSpec(const ParsingArguments& pa, Ptr<CppTokenCursor>& cursor, Ptr<Symbol>& specSymbol, Ptr<TemplateSpec>& spec);
extern void											ValidateForRootTemplateSpec(Ptr<TemplateSpec>& spec, Ptr<CppTokenCursor>& cursor, bool forPS, bool forFunction);
extern void											ParseSpecializationSpec(const ParsingArguments& pa, Ptr<CppTokenCursor>& cursor, Ptr<SpecializationSpec>& spec);
extern void											ParseGenericArgumentsSkippedLT(const ParsingArguments& pa, Ptr<CppTokenCursor>& cursor, VariadicList<GenericArgument>& arguments, CppTokens ending, bool allowVariadicOnAllArguments = true);

// Parser_Declaration.cpp
extern void											ParseDeclaration(const ParsingArguments& pa, Ptr<CppTokenCursor>& cursor, List<Ptr<Declaration>>& output);
extern void											BuildVariables(List<Ptr<Declarator>>& declarators, List<Ptr<VariableDeclaration>>& varDecls);
extern void											BuildSymbols(const ParsingArguments& pa, List<Ptr<VariableDeclaration>>& varDecls, Ptr<CppTokenCursor>& cursor);
extern void											BuildSymbols(const ParsingArguments& pa, VariadicList<Ptr<VariableDeclaration>>& varDecls, Ptr<CppTokenCursor>& cursor);
extern void											BuildVariablesAndSymbols(const ParsingArguments& pa, List<Ptr<Declarator>>& declarators, List<Ptr<VariableDeclaration>>& varDecls, Ptr<CppTokenCursor>& cursor);
extern Ptr<VariableDeclaration>						BuildVariableAndSymbol(const ParsingArguments& pa, Ptr<Declarator> declarator, Ptr<CppTokenCursor>& cursor);

// Parser_Expr.cpp
struct ParsingExprArguments
{
	bool											allowComma;
	bool											allowGt;
};

inline ParsingExprArguments							pea_Full()
	{	return { true, true }; }
inline ParsingExprArguments							pea_Argument()
	{	return { false, true }; }
inline ParsingExprArguments							pea_GenericArgument()
	{	return { false, false }; }

extern Ptr<Expr>									ParseExpr(const ParsingArguments& pa, const ParsingExprArguments& pea, Ptr<CppTokenCursor>& cursor);

// Parser_Stat.cpp
extern Ptr<Stat>									ParseStat(const ParsingArguments& pa, Ptr<CppTokenCursor>& cursor);

// Parser.cpp
extern void											EnsureFunctionBodyParsed(FunctionDeclaration* funcDecl);
extern bool											ParseTypeOrExpr(const ParsingArguments& pa, const ParsingExprArguments& pea, Ptr<CppTokenCursor>& cursor, Ptr<Type>& type, Ptr<Expr>& expr);
extern Ptr<Program>									ParseProgram(ParsingArguments& pa, Ptr<CppTokenCursor>& cursor);

/***********************************************************************
Helpers
***********************************************************************/

// Test if the next token's content matches the expected value
__forceinline bool TestToken(Ptr<CppTokenCursor>& cursor, const wchar_t* content, bool autoSkip = true)
{
	vint length = (vint)wcslen(content);
	if (cursor && cursor->token.length == length && wcsncmp(cursor->token.reading, content, length) == 0)
	{
		if (autoSkip) cursor = cursor->Next();
		return true;
	}
	return false;
}

// Test if the next token's type matches the expected value
__forceinline bool TestToken(Ptr<CppTokenCursor>& cursor, CppTokens token1, bool autoSkip = true)
{
	if (cursor && (CppTokens)cursor->token.token == token1)
	{
		if (autoSkip) cursor = cursor->Next();
		return true;
	}
	return false;
}

#define TEST_AND_SKIP(TOKEN)\
	if (TestToken(current, TOKEN, false) && current->token.start == start)\
	{\
		start += current->token.length;\
		current = current->Next();\
	}\
	else\
	{\
		return false;\
	}\

// Test if next two tokens' types match expected value, and there should not be spaces between tokens
__forceinline bool TestToken(Ptr<CppTokenCursor>& cursor, CppTokens token1, CppTokens token2, bool autoSkip = true)
{
	if (auto current = cursor)
	{
		vint start = current->token.start;
		TEST_AND_SKIP(token1);
		TEST_AND_SKIP(token2);
		if (autoSkip) cursor = current;
		return true;
	}
	return false;
}

// Test if next three tokens' types match expected value, and there should not be spaces between tokens
__forceinline bool TestToken(Ptr<CppTokenCursor>& cursor, CppTokens token1, CppTokens token2, CppTokens token3, bool autoSkip = true)
{
	if (auto current = cursor)
	{
		vint start = current->token.start;
		TEST_AND_SKIP(token1);
		TEST_AND_SKIP(token2);
		TEST_AND_SKIP(token3);
		if (autoSkip) cursor = current;
		return true;
	}
	return false;
}

// Throw exception if failed to test
__forceinline void RequireToken(Ptr<CppTokenCursor>& cursor, const wchar_t* content)
{
	if (!TestToken(cursor, content))
	{
		throw StopParsingException(cursor);
	}
}

// Throw exception if failed to test
__forceinline void RequireToken(Ptr<CppTokenCursor>& cursor, CppTokens token1)
{
	if (!TestToken(cursor, token1))
	{
		throw StopParsingException(cursor);
	}
}

// Throw exception if failed to test
__forceinline void RequireToken(Ptr<CppTokenCursor>& cursor, CppTokens token1, CppTokens token2)
{
	if (!TestToken(cursor, token1, token2))
	{
		throw StopParsingException(cursor);
	}
}

// Throw exception if failed to test
__forceinline void RequireToken(Ptr<CppTokenCursor>& cursor, CppTokens token1, CppTokens token2, CppTokens token3)
{
	if (!TestToken(cursor, token1, token2, token3))
	{
		throw StopParsingException(cursor);
	}
}

// Skip one token
__forceinline void SkipToken(Ptr<CppTokenCursor>& cursor)
{
	if (cursor)
	{
		cursor = cursor->Next();
	}
	else
	{
		throw StopParsingException(cursor);
	}
}

#endif