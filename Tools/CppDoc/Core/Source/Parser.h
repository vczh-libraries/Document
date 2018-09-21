#ifndef VCZH_DOCUMENT_CPPDOC_PARSER
#define VCZH_DOCUMENT_CPPDOC_PARSER

#include "Lexer.h"

/***********************************************************************
Symbol
***********************************************************************/

class Symbol
{
	using SymbolGroup = Group<WString, Ptr<Symbol>>;
public:
	Symbol*				parent = nullptr;
	WString				name;
	SymbolGroup			children;

	void				Add(Ptr<Symbol> child);
};

/***********************************************************************
Declarations
***********************************************************************/

/***********************************************************************
Parsers
***********************************************************************/

#endif