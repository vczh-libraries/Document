#ifndef VCZH_DOCUMENT_CPPDOC_AST_STAT
#define VCZH_DOCUMENT_CPPDOC_AST_STAT

#include "Ast.h"

/***********************************************************************
Visitor
***********************************************************************/

#define CPPDOC_STAT_LIST(F)\
	F(EmptyStat)\
	F(BlockStat)\
	F(DeclStat)\
	F(ExprStat)\
	F(LabelStat)\
	F(DefaultStat)\
	F(CaseStat)\
	F(GotoStat)\
	F(BreakStat)\
	F(ContinueStat)\
	F(WhileStat)\
	F(DoWhileStat)\
	F(ForEachStat)\
	F(ForStat)\
	F(IfElseStat)\
	F(SwitchStat)\
	F(TryCatchStat)\
	F(ReturnStat)\
	F(__Try__ExceptStat)\
	F(__Try__FinallyStat)\
	F(__LeaveStat)\
	F(__IfExistsStat)\
	F(__IfNotExistsStat)\

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

class EmptyStat : public Stat
{
public:
	IStatVisitor_ACCEPT;
};

class BlockStat : public Stat
{
public:
	IStatVisitor_ACCEPT;

	List<Ptr<Stat>>				stats;
};

class DeclStat : public Stat
{
public:
	IStatVisitor_ACCEPT;

	List<Ptr<Declaration>>		decls;
};

class ExprStat : public Stat
{
public:
	IStatVisitor_ACCEPT;

	Ptr<Expr>					expr;
};

class LabelStat : public Stat
{
public:
	IStatVisitor_ACCEPT;

	CppName						name;
	Ptr<Stat>					stat;
};

class DefaultStat : public Stat
{
public:
	IStatVisitor_ACCEPT;

	Ptr<Stat>					stat;
};

class CaseStat : public Stat
{
public:
	IStatVisitor_ACCEPT;

	Ptr<Expr>					expr;
	Ptr<Stat>					stat;
};

class GotoStat : public Stat
{
public:
	IStatVisitor_ACCEPT;

	CppName						name;
};

class BreakStat : public Stat
{
public:
	IStatVisitor_ACCEPT;
};

class ContinueStat : public Stat
{
public:
	IStatVisitor_ACCEPT;
};

class WhileStat : public Stat
{
public:
	IStatVisitor_ACCEPT;

	Ptr<Expr>					expr;
	Ptr<Stat>					stat;
};

class DoWhileStat : public Stat
{
public:
	IStatVisitor_ACCEPT;

	Ptr<Expr>					expr;
	Ptr<Stat>					stat;
};

class VariableDeclaration;

class ForEachStat : public Stat
{
public:
	IStatVisitor_ACCEPT;

	Ptr<VariableDeclaration>	varDecl;
	Ptr<Expr>					expr;
	Ptr<Stat>					stat;
};

class ForStat : public Stat
{
public:
	IStatVisitor_ACCEPT;

	Ptr<Stat>					init;
	Ptr<Expr>					expr;
	Ptr<Expr>					effect;
	Ptr<Stat>					stat;
};

class IfElseStat : public Stat
{
public:
	IStatVisitor_ACCEPT;

	Ptr<Declaration>			decl;
	Ptr<Expr>					expr;
	Ptr<Stat>					trueStat;
	Ptr<Stat>					falseStat;
};

class SwitchStat : public Stat
{
public:
	IStatVisitor_ACCEPT;

	Ptr<Expr>					expr;
	Ptr<Stat>					stat;
};

class TryCatchStat : public Stat
{
public:
	IStatVisitor_ACCEPT;

	Ptr<Stat>					tryStat;
	Ptr<Stat>					catchStat;
	Ptr<Declarator>				exception;
};

class ReturnStat : public Stat
{
public:
	IStatVisitor_ACCEPT;

	Ptr<Expr>					expr;
};

class __Try__ExceptStat : public Stat
{
public:
	IStatVisitor_ACCEPT;

	Ptr<Stat>					tryStat;
	Ptr<Stat>					exceptStat;
	Ptr<Expr>					expr;
};

class __Try__FinallyStat : public Stat
{
public:
	IStatVisitor_ACCEPT;

	Ptr<Stat>					tryStat;
	Ptr<Stat>					finallyStat;
};

class __LeaveStat : public Stat
{
public:
	IStatVisitor_ACCEPT;
};

class __IfExistsStat : public Stat
{
public:
	IStatVisitor_ACCEPT;

	Ptr<Expr>					expr;
	Ptr<Stat>					stat;
};

class __IfNotExistsStat : public Stat
{
public:
	IStatVisitor_ACCEPT;

	Ptr<Expr>					expr;
	Ptr<Stat>					stat;
};

#endif