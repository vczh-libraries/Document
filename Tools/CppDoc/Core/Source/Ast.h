#ifndef VCZH_DOCUMENT_CPPDOC_AST
#define VCZH_DOCUMENT_CPPDOC_AST

#include "TypeSystem.h"

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

	operator bool()const { return name.Length() != 0; }
};

class Resolving : public Object
{
public:
	List<Symbol*>			resolvedSymbols;
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
	bool					implicitlyGeneratedMember = false;

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
	vint					createdForwardDeclByCStyleTypeReference = 0;
	List<Ptr<Declaration>>	decls;
};

struct GenericArgument
{
	Ptr<Type>				type;
	Ptr<Expr>				expr;
};

/***********************************************************************
Declarator
***********************************************************************/

enum class CppInitializerType
{
	Equal,
	Constructor,
	Universal,
};

class Initializer : public Object
{
public:
	CppInitializerType		initializerType;
	List<Ptr<Expr>>			arguments;
};

class Declarator : public Object
{
public:
	Symbol*					containingClassSymbol = nullptr;
	Ptr<Type>				type;
	CppName					name;
	Ptr<Initializer>		initializer;
};

/***********************************************************************
Helpers
***********************************************************************/

struct NotConvertableException {};
struct IllegalExprException {};
struct NotResolvableException {};

using TypeTsysList = List<ITsys*>;
using ExprTsysList = List<ExprTsysItem>;

extern bool					IsSameResolvedType(Ptr<Type> t1, Ptr<Type> t2);
extern bool					IsPendingType(Type* type);
extern bool					IsPendingType(Ptr<Type> type);
extern ITsys*				ResolvePendingType(const ParsingArguments& pa, Ptr<Type> type, ExprTsysItem target);
extern void					TypeToTsys(const ParsingArguments& pa, Type* t, TypeTsysList& tsys, GenericArgContext* gaContext, bool memberOf = false, TsysCallingConvention cc = TsysCallingConvention::None);
extern void					TypeToTsys(const ParsingArguments& pa, Ptr<Type> t, TypeTsysList& tsys, GenericArgContext* gaContext, bool memberOf = false, TsysCallingConvention cc = TsysCallingConvention::None);
extern void					TypeToTsysAndReplaceFunctionReturnType(const ParsingArguments& pa, Ptr<Type> t, TypeTsysList& returnTypes, TypeTsysList& tsys, GenericArgContext* gaContext, bool memberOf);
extern void					ExprToTsys(const ParsingArguments& pa, Ptr<Expr> e, ExprTsysList& tsys);
extern void					EvaluateStat(const ParsingArguments& pa, Ptr<Stat> s);
extern void					EvaluateDeclaration(const ParsingArguments& pa, Ptr<Declaration> s);
extern void					EvaluateProgram(const ParsingArguments& pa, Ptr<Program> program);

enum class SpecialMemberKind
{
	DefaultCtor,
	CopyCtor,
	MoveCtor,
	CopyAssignOp,
	MoveAssignOp,
	Dtor,
};

extern Symbol*				GetSpecialMember(const ParsingArguments& pa, Symbol* classSymbol, SpecialMemberKind kind);
extern bool					IsSpecialMemberFeatureEnabled(const ParsingArguments& pa, Symbol* classSymbol, SpecialMemberKind kind);
extern bool					IsSpecialMemberEnabledForType(const ParsingArguments& pa, ITsys* type, SpecialMemberKind kind);
extern void					GenerateMembers(const ParsingArguments& pa, Symbol* classSymbol);

#endif