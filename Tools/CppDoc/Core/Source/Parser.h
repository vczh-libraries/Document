#ifndef VCZH_DOCUMENT_CPPDOC_PARSER
#define VCZH_DOCUMENT_CPPDOC_PARSER

#include "Ast.h"

/***********************************************************************
Symbol
***********************************************************************/

class Symbol : public Object
{
	using SymbolGroup = Group<WString, Ptr<Symbol>>;
public:
	Symbol*					parent = nullptr;
	Ptr<Declaration>		decl;
	CppName					name;
	SymbolGroup				children;

	void					Add(Ptr<Symbol> child);
};

/***********************************************************************
Parsers
***********************************************************************/

class IndexRecorder : public Object
{
public:
};

enum class DecoratorRestriction
{
	Zero,
	Optional,
	One,
	Many,
};

struct ParsingArguments
{
	Ptr<Symbol>				root;
	Ptr<Symbol>				context;
	Ptr<IndexRecorder>		recorder;

	ParsingArguments();
	ParsingArguments(Ptr<Symbol> _root, Ptr<IndexRecorder> _recorder);
	ParsingArguments(ParsingArguments& pa, Ptr<Symbol> _context);
};

struct StopParsingException
{
	Ptr<CppTokenCursor>		position;

	StopParsingException() {}
	StopParsingException(Ptr<CppTokenCursor> _position) :position(_position) {}
};

extern Ptr<Declarator> ParseDeclarator(ParsingArguments& pa, DecoratorRestriction dr, Ptr<CppTokenCursor>& cursor);
extern Ptr<Declaration> ParseDeclaration(ParsingArguments& pa, Ptr<CppTokenCursor>& cursor);
extern Ptr<Expr> ParseExpr(ParsingArguments& pa, Ptr<CppTokenCursor>& cursor);
extern Ptr<Stat> ParseStat(ParsingArguments& pa, Ptr<CppTokenCursor>& cursor);

#endif