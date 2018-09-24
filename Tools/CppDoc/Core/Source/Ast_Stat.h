#ifndef VCZH_DOCUMENT_CPPDOC_AST_STAT
#define VCZH_DOCUMENT_CPPDOC_AST_STAT

#include "Ast.h"

/***********************************************************************
Visitor
***********************************************************************/

#define CPPDOC_STAT_LIST(F)\
	F(BlockStat)\

#define CPPDOC_FORWARD(NAME) class NAME;
CPPDOC_STAT_LIST(CPPDOC_FORWARD)
#undef CPPDOC_FORWARD

class IStatVisitor abstract : public virtual Interface
{
public:
#define CPPDOC_VISIT(NAME) virtual void Visit(NAME* self) = 0;
	CPPDOC_STAT_LIST(CPPDOC_VISIT)
#undef CPPDOC_VISIT
};

#define IStatVisitor_ACCEPT void Accept(IStatVisitor* visitor)override

/***********************************************************************
Statements
***********************************************************************/

class BlockStat : public Stat
{
public:
	IStatVisitor_ACCEPT;
};

#endif