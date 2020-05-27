#ifndef VCZH_DOCUMENT_CPPDOC_AST_DECL
#define VCZH_DOCUMENT_CPPDOC_AST_DECL

#include "Ast.h"
#include "Lexer.h"
#include "Symbol.h"

/***********************************************************************
Visitor
***********************************************************************/

#define CPPDOC_DECL_LIST(F)\
	F(ForwardVariableDeclaration)\
	F(ForwardFunctionDeclaration)\
	F(ForwardEnumDeclaration)\
	F(ForwardClassDeclaration)\
	F(FriendClassDeclaration)\
	F(VariableDeclaration)\
	F(FunctionDeclaration)\
	F(EnumItemDeclaration)\
	F(EnumDeclaration)\
	F(ClassDeclaration)\
	F(NestedAnonymousClassDeclaration)\
	F(UsingNamespaceDeclaration)\
	F(UsingSymbolDeclaration)\
	F(TypeAliasDeclaration)\
	F(ValueAliasDeclaration)\
	F(NamespaceDeclaration)\
	F(StaticAssertDeclaration)\

#define CPPDOC_FORWARD(NAME) class NAME;
CPPDOC_DECL_LIST(CPPDOC_FORWARD)
#undef CPPDOC_FORWARD

class IDeclarationVisitor abstract : public virtual Interface
{
public:
#define CPPDOC_VISIT(NAME) virtual void Visit(NAME* self) = 0;
	CPPDOC_DECL_LIST(CPPDOC_VISIT)
#undef CPPDOC_VISIT
};

#define IDeclarationVisitor_ACCEPT void Accept(IDeclarationVisitor* visitor)override

class IdExpr;

/***********************************************************************
Types
***********************************************************************/

enum class CppTemplateArgumentType
{
	HighLevelType,		// templateSpec(declaration), type(default value)
	Type,				// type(default value)
	Value,				// type(value type), expr(default value)
};

class TemplateSpec : public Object
{
public:
	struct Argument
	{
		bool										ellipsis = false;			// variadic template argument
		CppName										name;						// name of the argument
		Symbol*										argumentSymbol = nullptr;	// symbol of the argument
		CppTemplateArgumentType						argumentType;
		Ptr<Symbol>									templateSpecScope;			// for HighLevelType
		Ptr<TemplateSpec>							templateSpec;				// for HighLevelType
		Ptr<Type>									type;						// default value of type argument, or type of value argument
		Ptr<Expr>									expr;						// default value of value argument
	};

	List<Argument>									arguments;

	void AssignDeclSymbol(Symbol* declSymbol)
	{
		for (vint i = 0; i < arguments.Count(); i++)
		{
			arguments[i].argumentSymbol->declSymbolForGenericArg = declSymbol;
		}
	}
};

class SpecializationSpec : public Object
{
public:
	VariadicList<GenericArgument>					arguments;
};

class DelayParse : public Object
{
public:
	ParsingArguments								pa;
	Ptr<CppTokenReader>								reader;
	Ptr<CppTokenCursor>								begin;
	RegexToken										end;
};

/***********************************************************************
Forward Declarations
***********************************************************************/

class ForwardVariableDeclaration : public Declaration
{
public:
	using ForwardRootType = VariableDeclaration;

	IDeclarationVisitor_ACCEPT;

	Ptr<Type>										type;
	bool											decoratorConstexpr = false;
	bool											decoratorExtern = false;
	bool											decoratorStatic = false;
	bool											decoratorMutable = false;
	bool											decoratorThreadLocal = false;
	bool											decoratorRegister = false;
	bool											decoratorInline = false;
	bool											decorator__Inline = false;
	bool											decorator__ForceInline = false;
	bool											needResolveTypeFromInitializer = false;

	// for template forward declaration to keep symbols alive
	// do not access it after parsing
	// templateSpec->arguments[i].argumentSymbol will be children of it
	Ptr<Symbol>										keepTemplateArgumentAlive;
};

enum class CppMethodType
{
	Function,
	Constructor,
	Destructor,
	TypeConversion,
};

class ForwardFunctionDeclaration : public Declaration
{
public:
	using ForwardRootType = FunctionDeclaration;

	IDeclarationVisitor_ACCEPT;

	Ptr<TemplateSpec>								templateSpec;
	Ptr<SpecializationSpec>							specializationSpec;
	Ptr<Type>										type;
	CppMethodType									methodType = CppMethodType::Function;
	bool											decoratorConstexpr = false;
	bool											decoratorExtern = false;
	bool											decoratorFriend = false;
	bool											decoratorStatic = false;
	bool											decoratorVirtual = false;
	bool											decoratorExplicit = false;
	bool											decoratorInline = false;
	bool											decorator__Inline = false;
	bool											decorator__ForceInline = false;
	bool											decoratorAbstract = false;
	bool											decoratorDefault = false;
	bool											decoratorDelete = false;
	bool											needResolveTypeFromStatement = false;
};

class ForwardEnumDeclaration : public Declaration
{
public:
	using ForwardRootType = EnumDeclaration;

	IDeclarationVisitor_ACCEPT;

	bool											enumClass = false;
	Ptr<Type>										baseType;
};

enum class CppClassType
{
	Class,
	Struct,
	Union,
};

class ForwardClassDeclaration : public Declaration
{
public:
	using ForwardRootType = ClassDeclaration;

	IDeclarationVisitor_ACCEPT;

	Ptr<TemplateSpec>								templateSpec;
	Ptr<SpecializationSpec>							specializationSpec;
	CppClassType									classType;
	bool											decoratorFriend = false;

	// for template forward declaration to keep symbols alive
	// do not access it after parsing
	// templateSpec->arguments[i].argumentSymbol will be children of it
	Ptr<Symbol>										keepTemplateArgumentAlive;
};

class FriendClassDeclaration : public Declaration
{
public:
	IDeclarationVisitor_ACCEPT;

	Ptr<TemplateSpec>								templateSpec;
	Nullable<CppClassType>							classType;
	Ptr<Type>										usedClass;
};

/***********************************************************************
Forwardable Declarations
***********************************************************************/

class VariableDeclaration : public ForwardVariableDeclaration
{
public:
	IDeclarationVisitor_ACCEPT;

	List<Ptr<TemplateSpec>>							classSpecs;
	Ptr<Initializer>								initializer;
};

class FunctionDeclaration : public ForwardFunctionDeclaration
{
public:
	IDeclarationVisitor_ACCEPT;

	struct InitItem
	{
		Ptr<IdExpr>									field;
		bool										universalInitialization = false;
		VariadicList<Ptr<Expr>>						arguments;
	};

	List<Ptr<TemplateSpec>>							classSpecs;
	List<Ptr<InitItem>>								initList;
	Ptr<Stat>										statement;
	Ptr<DelayParse>									delayParse;
};

class EnumItemDeclaration : public Declaration
{
public:
	IDeclarationVisitor_ACCEPT;

	Ptr<Expr>										value;
};

class EnumDeclaration : public ForwardEnumDeclaration
{
public:
	IDeclarationVisitor_ACCEPT;

	List<Ptr<EnumItemDeclaration>>					items;
};

enum class CppClassAccessor
{
	Private,
	Protected,
	Public,
};

class ClassDeclaration : public ForwardClassDeclaration
{
public:
	IDeclarationVisitor_ACCEPT;

	List<Tuple<CppClassAccessor, Ptr<Type>>>		baseTypes;
	List<Tuple<CppClassAccessor, Ptr<Declaration>>>	decls;
};

/***********************************************************************
Othere Declarations
***********************************************************************/

class NestedAnonymousClassDeclaration : public Declaration
{
public:
	IDeclarationVisitor_ACCEPT;

	CppClassType									classType;
	List<Ptr<Declaration>>							decls;
};

class UsingNamespaceDeclaration : public Declaration
{
public:
	IDeclarationVisitor_ACCEPT;

	Ptr<Type>										ns;
};

class UsingSymbolDeclaration : public Declaration
{
public:
	IDeclarationVisitor_ACCEPT;

	Ptr<Type>										type;
	Ptr<Expr>										expr;
};

class TypeAliasDeclaration : public Declaration
{
public:
	IDeclarationVisitor_ACCEPT;

	Ptr<TemplateSpec>								templateSpec;
	Ptr<Type>										type;
};

class ValueAliasDeclaration : public Declaration
{
public:
	IDeclarationVisitor_ACCEPT;

	Ptr<TemplateSpec>								templateSpec;
	Ptr<SpecializationSpec>							specializationSpec;
	Ptr<Type>										type;
	Ptr<Expr>										expr;
	bool											decoratorConstexpr = false;
	bool											needResolveTypeFromInitializer = false;
};

class NamespaceDeclaration : public Declaration
{
public:
	IDeclarationVisitor_ACCEPT;

	List<Ptr<Declaration>>							decls;
};

class StaticAssertDeclaration : public Declaration
{
public:
	IDeclarationVisitor_ACCEPT;

	List<Ptr<Expr>>									exprs;
};

#endif