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

ParsingArguments::ParsingArguments(Ptr<Symbol> _root, Ptr<IIndexRecorder> _recorder)
	:root(_root)
	, context(_root.Obj())
	, recorder(_recorder)
{
}

ParsingArguments::ParsingArguments(const ParsingArguments& pa, Symbol* _context)
	:root(pa.root)
	, context(_context)
	, recorder(pa.recorder)
{
}

/***********************************************************************
ParsingArguments
***********************************************************************/

Ptr<Program> ParseProgram(const ParsingArguments& pa, Ptr<CppTokenCursor>& cursor)
{
	auto program = MakePtr<Program>();
	while (cursor)
	{
		ParseDeclaration(pa, cursor, program->decls);
	}
	return program;
}