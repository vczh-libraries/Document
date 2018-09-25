#ifndef VCZH_DOCUMENT_CPPDOC_AST_DECL
#define VCZH_DOCUMENT_CPPDOC_AST_DECL

#include "Ast.h"

/***********************************************************************
Visitor
***********************************************************************/

#define CPPDOC_DECL_LIST(F)\
	F(ForwardVariableDeclaration)\
	F(ForwardFunctionDeclaration)\
	F(ForwardEnumDeclaration)\
	F(ForwardClassDeclaration)\
	F(VariableDeclaration)\
	F(FunctionDeclaration)\
	F(EnumItemDeclaration)\
	F(EnumDeclaration)\
	F(ClassDeclaration)\
	F(TypeAliasDeclaration)\
	F(UsingDeclaration)\
	F(NamespaceDeclaration)\

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

/***********************************************************************
Types
***********************************************************************/

class TemplateSpec : public Object
{
public:
};

class SpecializationSpec : public Object
{
public:
};

class SpecializableDeclaration : public Declaration
{
public:
	Ptr<TemplateSpec>								templateSpec;
	Ptr<SpecializationSpec>							speclizationSpec;
};

/***********************************************************************
Forward Declarations
***********************************************************************/

class ForwardVariableDeclaration : public Declaration
{
public:
	IDeclarationVisitor_ACCEPT;

	Ptr<Type>										type;
	bool											decoratorExtern = false;
	bool											decoratorStatic = false;
	bool											decoratorMutable = false;
	bool											decoratorThreadLocal = false;
	bool											decoratorRegister = false;
};

enum class MethodType
{
	Function,
	Constructor,
	Destructor,
};

class ForwardFunctionDeclaration : public SpecializableDeclaration
{
public:
	IDeclarationVisitor_ACCEPT;

	Ptr<Type>										type;
	MethodType										methodType = MethodType::Function;
	bool											decoratorExtern = false;
	bool											decoratorFriend = false;
	bool											decoratorStatic = false;
	bool											decoratorVirtual = false;
	bool											decoratorExplicit = false;
	bool											decoratorInline = false;
	bool											decoratorForceInline = false;
};

class ForwardEnumDeclaration : public Declaration
{
public:
	IDeclarationVisitor_ACCEPT;

	bool											enumClass = false;
	Ptr<Type>										baseType;
};

enum class ClassType
{
	Class,
	Struct,
	Union,
};

class ForwardClassDeclaration : public SpecializableDeclaration
{
public:
	IDeclarationVisitor_ACCEPT;

	ClassType										classType;
	bool											friendClass = false;
};

/***********************************************************************
Forwardable Declarations
***********************************************************************/

class VariableDeclaration : public ForwardVariableDeclaration
{
public:
	IDeclarationVisitor_ACCEPT;

	Ptr<Initializer>								initializer;
};

class FunctionDeclaration : public ForwardFunctionDeclaration
{
public:
	IDeclarationVisitor_ACCEPT;

	Ptr<Stat>										statement;
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

enum class ClassAccessor
{
	Private,
	Protected,
	Public,
};

class ClassDeclaration : public ForwardClassDeclaration
{
public:
	IDeclarationVisitor_ACCEPT;

	List<Tuple<ClassAccessor, Ptr<Type>>>			baseTypes;
	List<Tuple<ClassAccessor, Ptr<Declaration>>>	decls;
};

/***********************************************************************
Othere Declarations
***********************************************************************/

class TypeAliasDeclaration : public Declaration
{
public:
	IDeclarationVisitor_ACCEPT;

	Ptr<TemplateSpec>								templateSpec;
	Ptr<Type>										type;
};

// if using declaration has a name, then it is using a class member
// if using declaration doesn't have a name, then it is using a namespace
class UsingDeclaration : public Declaration
{
public:
	IDeclarationVisitor_ACCEPT;

	Ptr<Type>										type;
};

class NamespaceDeclaration : public Declaration
{
public:
	IDeclarationVisitor_ACCEPT;

	List<Ptr<Declaration>>							decls;
};

#endif