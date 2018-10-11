#ifndef VCZH_DOCUMENT_CPPDOC_AST
#define VCZH_DOCUMENT_CPPDOC_AST

#include <Vlpp.h>

using namespace vl;
using namespace vl::collections;
using namespace vl::regex;

struct ParsingArguments;
class ITsys;

/***********************************************************************
Symbol
***********************************************************************/

class Symbol;

enum class CppNameType
{
	Normal,
	Operator,
	Constructor,
	Destructor,
};

struct CppName
{
	CppNameType				type = CppNameType::Normal;
	vint					tokenCount = 0;
	WString					name;
	RegexToken				nameTokens[4];

	operator bool()const { return tokenCount != 0; }
};

class Resolving : public Object
{
protected:
	bool					fullyCalibrated = false;

public:
	List<Symbol*>			resolvedSymbols;

	void					Calibrate();
};

/***********************************************************************
AST
***********************************************************************/

class IDeclarationVisitor;
class Declaration : public Object
{
public:
	CppName					name;
	Symbol*					symbol = nullptr;

	virtual void			Accept(IDeclarationVisitor* visitor) = 0;
};

class ITypeVisitor;
class Type : public Object
{
public:
	virtual void			Accept(ITypeVisitor* visitor) = 0;
};

class IExprVisitor;
class Expr : public Object
{
public:
	virtual void			Accept(IExprVisitor* visitor) = 0;
};

class IStatVisitor;
class Stat : public Object
{
public:
	Symbol*					symbol = nullptr;

	virtual void			Accept(IStatVisitor* visitor) = 0;
};

class Program : public Object
{
public:
	List<Ptr<Declaration>>	decls;
};

/***********************************************************************
Declarator
***********************************************************************/

enum class InitializerType
{
	Equal,
	Constructor,
	Universal,
};

class Initializer : public Object
{
public:
	InitializerType			initializerType;
	List<Ptr<Expr>>			arguments;
};

class Declarator : public Object
{
public:
	Symbol*					containingClassSymbol = nullptr;
	Ptr<Type>				type;
	CppName					name;
	Symbol*					createdSymbol = nullptr;
	Ptr<Initializer>		initializer;
};

/***********************************************************************
Helpers
***********************************************************************/

struct NotConvertableException {};
struct IllegalExprException {};

struct ExprTsysItem
{
	Symbol*					symbol = nullptr;
	ITsys*					tsys = nullptr;

	ExprTsysItem() = default;
	ExprTsysItem(const ExprTsysItem&) = default;
	ExprTsysItem(ExprTsysItem&&) = default;

	ExprTsysItem(Symbol* _symbol, ITsys* _tsys)
		:symbol(_symbol), tsys(_tsys)
	{
	}

	ExprTsysItem& operator=(const ExprTsysItem&) = default;
	ExprTsysItem& operator=(ExprTsysItem&&) = default;

	static vint Compare(const ExprTsysItem& a, const ExprTsysItem& b)
	{
		if (a.symbol < b.symbol) return -1;
		if (a.symbol > b.symbol) return 1;
		if (a.tsys < b.tsys) return -1;
		if (a.tsys > b.tsys) return 1;
		return 0;
	}

	bool operator==	(const ExprTsysItem& item)const { return Compare(*this, item) ==	0; }
	bool operator!=	(const ExprTsysItem& item)const { return Compare(*this, item) !=	0; }
	bool operator<	(const ExprTsysItem& item)const { return Compare(*this, item) <		0; }
	bool operator<=	(const ExprTsysItem& item)const { return Compare(*this, item) <=	0; }
	bool operator>	(const ExprTsysItem& item)const { return Compare(*this, item) >		0; }
	bool operator>=	(const ExprTsysItem& item)const { return Compare(*this, item) >=	0; }
};

using TypeTsysList = List<ITsys*>;
using ExprTsysList = List<ExprTsysItem>;

extern bool					IsSameResolvedType(Ptr<Type> t1, Ptr<Type> t2);
extern void					TypeToTsys(ParsingArguments& pa, Ptr<Type> t, TypeTsysList& tsys);
extern void					ExprToTsys(ParsingArguments& pa, Ptr<Expr> e, ExprTsysList& tsys);

#endif