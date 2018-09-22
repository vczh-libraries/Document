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

enum class DecoratorRestriction
{
	Zero,
	Optional,
	One,
	Many,
};

extern Ptr<Declaration> ParseDecl(Ptr<Symbol> root, Ptr<Symbol> context, DecoratorRestriction dr, Ptr<CppTokenCursor>& cursor);
extern Ptr<Expr> ParseExpr(Ptr<Symbol> root, Ptr<Symbol> context, Ptr<CppTokenCursor>& cursor);
extern Ptr<Stat> ParseStat(Ptr<Symbol> root, Ptr<Symbol> context, Ptr<CppTokenCursor>& cursor);

#endif