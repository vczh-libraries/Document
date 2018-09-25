#include "Parser.h"

/***********************************************************************
Symbol
***********************************************************************/

void Symbol::Add(Ptr<Symbol> child)
{
	child->parent = this;
	children.Add(child->name, child);
}

/***********************************************************************
ParsingArguments
***********************************************************************/

ParsingArguments::ParsingArguments()
{
}

ParsingArguments::ParsingArguments(Ptr<Symbol> _root, Ptr<IndexRecorder> _recorder)
	:root(_root)
	, context(_root)
	, recorder(_recorder)
{
}

ParsingArguments::ParsingArguments(ParsingArguments& pa, Ptr<Symbol> _context)
	:root(pa.root)
	, context(_context)
	, recorder(pa.recorder)
{
}

/***********************************************************************
ParsingArguments
***********************************************************************/

void ParseFile(ParsingArguments& pa, Ptr<CppTokenCursor>& cursor)
{
	while (cursor)
	{
		ParseDeclaration(pa, cursor);
	}
}