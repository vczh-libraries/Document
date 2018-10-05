#include "Ast.h"
#include "Ast_Decl.h"
#include "Ast_Type.h"
#include "Ast_Expr.h"
#include "Ast_Stat.h"
#include "Parser.h"

#define CPPDOC_ACCEPT(NAME) void NAME::Accept(ITypeVisitor* visitor) { visitor->Visit(this); }
CPPDOC_TYPE_LIST(CPPDOC_ACCEPT)
#undef CPPDOC_ACCEPT

#define CPPDOC_ACCEPT(NAME) void NAME::Accept(IDeclarationVisitor* visitor) { visitor->Visit(this); }
CPPDOC_DECL_LIST(CPPDOC_ACCEPT)
#undef CPPDOC_ACCEPT

#define CPPDOC_ACCEPT(NAME) void NAME::Accept(IExprVisitor* visitor) { visitor->Visit(this); }
CPPDOC_EXPR_LIST(CPPDOC_ACCEPT)
#undef CPPDOC_ACCEPT

#define CPPDOC_ACCEPT(NAME) void NAME::Accept(IStatVisitor* visitor) { visitor->Visit(this); }
CPPDOC_STAT_LIST(CPPDOC_ACCEPT)
#undef CPPDOC_ACCEPT

/***********************************************************************
Resolving
***********************************************************************/

// Change all forward declaration symbols to their real definition
void Resolving::Calibrate()
{
	if (fullyCalibrated) return;
	vint forwards = 0;

	SortedList<Symbol*> used;
	for (vint i = 0; i < resolvedSymbols.Count(); i++)
	{
		auto& symbol = resolvedSymbols[i];
		if (symbol->isForwardDeclaration)
		{
			if (symbol->forwardDeclarationRoot)
			{
				if (used.Contains(symbol->forwardDeclarationRoot))
				{
					resolvedSymbols.RemoveAt(i);
					i--;
				}
				else
				{
					symbol = symbol->forwardDeclarationRoot;
				}
			}
			else
			{
				forwards++;
			}
		}
	}

	if (forwards == 0)
	{
		fullyCalibrated = true;
	}
}