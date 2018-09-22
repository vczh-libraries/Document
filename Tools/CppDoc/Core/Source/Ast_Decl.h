#ifndef VCZH_DOCUMENT_CPPDOC_AST_DECL
#define VCZH_DOCUMENT_CPPDOC_AST_DECL

#include "Ast.h"

/***********************************************************************
Visitor
***********************************************************************/

#define CPPDOC_DECL_LIST(F)\
	F(ForwardClassDeclaration)\
	F(ForwardEnumDeclaration)\
	F(VariableDeclaration)\
	F(FunctionDeclaration)\
	F(TypeAliasDeclaration)\
	F(ClassDeclaration)\
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
Declarations
***********************************************************************/

class TemplateSpec : public Object
{
public:
};

class SpecializationSpec : public Object
{
public:
};

enum class ClassType
{
	Class,
	Struct,
	Union,
};

class SpecializableDeclaration : public Declaration
{
public:
	Ptr<TemplateSpec>		templateSpec;
	Ptr<SpecializationSpec>	speclizationSpec;
};

class ForwardClassDeclaration : public SpecializableDeclaration
{
public:
	IDeclarationVisitor_ACCEPT;

	ClassType				classType;
};

class ForwardEnumDeclaration : public Declaration
{
public:
	IDeclarationVisitor_ACCEPT;
};

class VariableDeclaration : public Declaration
{
public:
	IDeclarationVisitor_ACCEPT;

	Ptr<Type>				type;
	bool					externVariable = false;
	Ptr<Expr>				initializer;
};

enum class MethodType
{
	Function,
	Constructor,
	Destructor,
};

class FunctionDeclaration : public SpecializableDeclaration
{
public:
	IDeclarationVisitor_ACCEPT;

	Ptr<Type>				type;
	MethodType				methodType;
	bool					externFunction = false;
	Ptr<Stat>				statement;
};

class TypeAliasDeclaration : public Declaration
{
public:
	IDeclarationVisitor_ACCEPT;

	Ptr<TemplateSpec>		templateSpec;
	Ptr<Type>				type;
};

enum class ClassAccessor
{
	Private,
	Protected,
	Public,
};

class ClassDeclaration : public SpecializableDeclaration
{
public:
	IDeclarationVisitor_ACCEPT;

	ClassType										classType;
	List<Tuple<ClassAccessor ,Ptr<Type>>>			baseTypes;
	List<Tuple<ClassAccessor ,Ptr<Declaration>>>	decls;
};

// if using declaration has a name, then it is using a class member
// if using declaration doesn't have a name, then it is using a namespace
class UsingDeclaration : public Declaration
{
public:
	IDeclarationVisitor_ACCEPT;

	Ptr<Type>				type;
};

class NamespaceDeclaration : public Declaration
{
public:
	IDeclarationVisitor_ACCEPT;

	List<Ptr<Declaration>>	decls;
};

#endif