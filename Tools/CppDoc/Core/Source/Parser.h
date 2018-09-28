#ifndef VCZH_DOCUMENT_CPPDOC_PARSER
#define VCZH_DOCUMENT_CPPDOC_PARSER

#include "Ast.h"

/***********************************************************************
Symbol
***********************************************************************/

class Symbol : public Object
{
	using SymbolGroup = Group<WString, Ptr<Symbol>>;
	using SymbolPtrList = List<Symbol*>;
public:
	Symbol*					parent = nullptr;
	WString					name;
	List<Ptr<Declaration>>	decls; // only namespaces share symbols
	SymbolGroup				children;

	bool					isForwardDeclaration = false;
	Symbol*					forwardDeclarationRoot = nullptr;
	SymbolPtrList			forwardDeclarations;

	Symbol*					specializationRoot = nullptr;
	SymbolPtrList			specializations;

	void					Add(Ptr<Symbol> child);

	template<typename T>
	Symbol* CreateSymbol(Ptr<T> _decl, Symbol* _specializationRoot = nullptr)
	{
		auto symbol = MakePtr<Symbol>();
		symbol->name = _decl->name.name;
		symbol->decls.Add(_decl);
		Add(symbol);

		_decl->symbol = symbol.Obj();

		if (_specializationRoot)
		{
			_specializationRoot->specializations.Add(symbol.Obj());
			symbol->specializationRoot = _specializationRoot;
		}
		return symbol.Obj();
	}

	bool SetForwardDeclarationRoot(Symbol* root)
	{
		if (forwardDeclarationRoot) return false;
		forwardDeclarationRoot = root;
		root->forwardDeclarations.Add(this);
		return true;
	}
};

/***********************************************************************
Parsers
***********************************************************************/

class IIndexRecorder : public virtual Interface
{
public:
	virtual void			Index(CppName& name, Ptr<Resolving> resolving) = 0;
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

struct ParsingArguments
{
	Ptr<Symbol>				root;
	Symbol*					context = nullptr;
	Ptr<IIndexRecorder>		recorder;

	ParsingArguments();
	ParsingArguments(Ptr<Symbol> _root, Ptr<IIndexRecorder> _recorder);
	ParsingArguments(const ParsingArguments& pa, Symbol* _context);
};

struct StopParsingException
{
	Ptr<CppTokenCursor>		position;

	StopParsingException() {}
	StopParsingException(Ptr<CppTokenCursor> _position) :position(_position) {}
};

class FunctionType;
class ClassDeclaration;
enum class CppCallingConvention;

extern bool					SkipSpecifiers(Ptr<CppTokenCursor>& cursor);
extern bool					ParseCppName(CppName& name, Ptr<CppTokenCursor>& cursor);
extern Ptr<Type>			GetTypeWithoutMemberAndCC(Ptr<Type> type);
extern Ptr<Type>			AdjustReturnTypeWithMemberAndCC(Ptr<FunctionType> functionType);
extern bool					ParseCallingConvention(CppCallingConvention& callingConvention, Ptr<CppTokenCursor>& cursor);

extern void					ParseDeclarator(const ParsingArguments& pa, Ptr<Type> typeResult, ClassDeclaration* specialMethodParent, DeclaratorRestriction dr, InitializerRestriction ir, Ptr<CppTokenCursor>& cursor, List<Ptr<Declarator>>& declarators);
extern void					ParseDeclarator(const ParsingArguments& pa, DeclaratorRestriction dr, InitializerRestriction ir, Ptr<CppTokenCursor>& cursor, List<Ptr<Declarator>>& declarators);
extern void					ParseDeclaration(const ParsingArguments& pa, Ptr<CppTokenCursor>& cursor, List<Ptr<Declaration>>& output);
extern Ptr<Expr>			ParseExpr(const ParsingArguments& pa, Ptr<CppTokenCursor>& cursor);
extern Ptr<Stat>			ParseStat(const ParsingArguments& pa, Ptr<CppTokenCursor>& cursor);
extern Ptr<Program>			ParseProgram(const ParsingArguments& pa, Ptr<CppTokenCursor>& cursor);

/***********************************************************************
Helpers
***********************************************************************/

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

__forceinline void RequireToken(Ptr<CppTokenCursor>& cursor, const wchar_t* content)
{
	if (!TestToken(cursor, content))
	{
		throw StopParsingException(cursor);
	}
}

__forceinline void RequireToken(Ptr<CppTokenCursor>& cursor, CppTokens token1)
{
	if (!TestToken(cursor, token1))
	{
		throw StopParsingException(cursor);
	}
}

__forceinline void RequireToken(Ptr<CppTokenCursor>& cursor, CppTokens token1, CppTokens token2)
{
	if (!TestToken(cursor, token1, token2))
	{
		throw StopParsingException(cursor);
	}
}

__forceinline void RequireToken(Ptr<CppTokenCursor>& cursor, CppTokens token1, CppTokens token2, CppTokens token3)
{
	if (!TestToken(cursor, token1, token2, token3))
	{
		throw StopParsingException(cursor);
	}
}

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