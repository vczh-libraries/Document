#ifndef VCZH_DOCUMENT_CPPDOC_AST
#define VCZH_DOCUMENT_CPPDOC_AST

#include "Symbol.h"

using namespace vl::regex;

namespace symbol_component
{
	struct ClassMemberCache;
}

struct ParsingArguments;
struct EvaluateSymbolContext;
class ITsys;
class SpecializationSpec;
class ForwardFunctionDeclaration;
class VariableDeclaration;
class GenericExpr;

/***********************************************************************
Symbol
***********************************************************************/

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

struct ResolvedItem
{
	ITsys*					tsys = nullptr;
	Symbol*					symbol = nullptr;

	ResolvedItem() = default;

	ResolvedItem(ITsys* _tsys, Symbol* _symbol)
		:tsys(_tsys)
		, symbol(_symbol)
	{
	}

	static vint Compare(const ResolvedItem& a, const ResolvedItem& b)
	{
		if (a.tsys < b.tsys) return -1;
		if (a.tsys > b.tsys) return 1;
		if (a.symbol < b.symbol) return -1;
		if (a.symbol > b.symbol) return 1;
		return 0;
	}

	bool operator==	(const ResolvedItem& item)const { return Compare(*this, item) == 0; }
	bool operator!=	(const ResolvedItem& item)const { return Compare(*this, item) != 0; }
	bool operator<	(const ResolvedItem& item)const { return Compare(*this, item) < 0; }
	bool operator<=	(const ResolvedItem& item)const { return Compare(*this, item) <= 0; }
	bool operator>	(const ResolvedItem& item)const { return Compare(*this, item) > 0; }
	bool operator>=	(const ResolvedItem& item)const { return Compare(*this, item) >= 0; }
};

class Resolving : public Object
{
public:
	List<ResolvedItem>		items;

	static bool				ContainsSameSymbol(const Ptr<Resolving>& a, const Ptr<Resolving>& b);
	static bool				IsResolvedToType(const Ptr<Resolving>& resolving);
	static Symbol*			EnsureSingleSymbol(const Ptr<Resolving>& resolving);
	static Symbol*			EnsureSingleSymbol(const Ptr<Resolving>& resolving, symbol_component::SymbolKind kind);
	static void				AddSymbol(const ParsingArguments& pa, Ptr<Resolving>& resolving, Symbol* symbol);
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
Variadic
***********************************************************************/

template<typename T>
struct VariadicItem
{
	T						item;
	bool					isVariadic = false;

	VariadicItem() = default;
	VariadicItem(T _item, bool _isVariadic) :item(_item), isVariadic(_isVariadic) {}
};

template<typename T>
using VariadicList = List<VariadicItem<T>>;

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
	CppInitializerType							initializerType;
	VariadicList<Ptr<Expr>>						arguments;
};

class Declarator : public Object
{
public:
	Ptr<symbol_component::ClassMemberCache>		classMemberCache;

	// only for object that is created from ParseSingleDeclarator, for non-template or non-function symbols, it is an empty scope
	// do not access it after parsing
	// this scope is created to store template arguments and function parameters
	Ptr<Symbol>									scopeSymbolToReuse;

	Ptr<Type>									type;
	bool										ellipsis = false;
	CppName										name;
	Ptr<Initializer>							initializer;
	Ptr<SpecializationSpec>						specializationSpec;
};

/***********************************************************************
Helpers
***********************************************************************/

struct TypeToTsysConfig
{
	bool					idTypeNoPrimaryClass = true;
	bool					idTypeNoGenericFunction = true;
	bool					memberOf = false;
	TsysCallingConvention	cc = TsysCallingConvention::None;

	TypeToTsysConfig() = default;
	TypeToTsysConfig(bool _idTypeNoPrimaryClass, bool _idTypeNoGenericFunction, bool _memberOf, TsysCallingConvention _cc)
		:idTypeNoPrimaryClass(_idTypeNoPrimaryClass)
		, idTypeNoGenericFunction(_idTypeNoGenericFunction)
		, memberOf(_memberOf)
		, cc(_cc)
	{
	}

	static TypeToTsysConfig ToBeAppliedTemplateArguments()
	{
		TypeToTsysConfig config;
		config.idTypeNoPrimaryClass = false;
		config.idTypeNoGenericFunction = false;
		return config;
	}

	static TypeToTsysConfig MemberOf(bool _memberOf)
	{
		TypeToTsysConfig config;
		config.memberOf = _memberOf;
		return config;
	}
};

extern bool					IsSameResolvedExpr(Ptr<Expr> e1, Ptr<Expr> e2, Dictionary<WString, WString>& equivalentNames);
extern bool					IsSameResolvedType(Ptr<Type> t1, Ptr<Type> t2, Dictionary<WString, WString>& equivalentNames);
extern bool					IsCompatibleTemplateSpec(Ptr<TemplateSpec> specNew, Ptr<TemplateSpec> specOld, Dictionary<WString, WString>& equivalentNames);
extern bool					IsCompatibleSpecializationSpec(Ptr<SpecializationSpec> specNew, Ptr<SpecializationSpec> specOld, Dictionary<WString, WString>& equivalentNames);
extern bool					IsCompatibleFunctionDeclInSameScope(Ptr<symbol_component::ClassMemberCache> cacheNew, Ptr<ForwardFunctionDeclaration> declNew, Ptr<ForwardFunctionDeclaration> declOld);
extern bool					IsPendingType(Type* type);
extern bool					IsPendingType(Ptr<Type> type);
extern ITsys*				ResolvePendingType(const ParsingArguments& pa, Ptr<Type> type, ExprTsysItem target);

extern void					TypeToTsysInternal(const ParsingArguments& pa, Type* t, TypeTsysList& tsys, bool& isVta, TypeToTsysConfig config = {});
extern void					TypeToTsysInternal(const ParsingArguments& pa, Ptr<Type> t, TypeTsysList& tsys, bool& isVta, TypeToTsysConfig config = {});
extern void					TypeToTsysNoVta(const ParsingArguments& pa, Type* t, TypeTsysList& tsys, TypeToTsysConfig config = {});
extern void					TypeToTsysNoVta(const ParsingArguments& pa, Ptr<Type> t, TypeTsysList& tsys, TypeToTsysConfig config = {});
extern void					TypeToTsysAndReplaceFunctionReturnType(const ParsingArguments& pa, Ptr<Type> t, TypeTsysList& returnTypes, TypeTsysList& tsys, bool memberOf);
extern void					GenericExprToTsys(const ParsingArguments& pa, ExprTsysList& nameTypes, bool nameIsVta, GenericExpr* argumentsPart, ExprTsysList& tsys, bool& isVta);
extern void					ExprToTsysInternal(const ParsingArguments& pa, Ptr<Expr> e, ExprTsysList& tsys, bool& isVta);
extern void					ExprToTsysNoVta(const ParsingArguments& pa, Ptr<Expr> e, ExprTsysList& tsys);

extern void					EvaluateStat(const ParsingArguments& pa, Ptr<Stat> s, bool resolvingFunctionType, TemplateArgumentContext* argumentsToApply);
extern void					EvaluateVariableDeclaration(const ParsingArguments& pa, VariableDeclaration* decl);
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

extern bool					IsSpecialMemberFeatureEnabled(const ParsingArguments& pa, ITsys* classType, SpecialMemberKind kind);
extern void					GenerateMembers(const ParsingArguments& pa, Symbol* classSymbol);

#endif