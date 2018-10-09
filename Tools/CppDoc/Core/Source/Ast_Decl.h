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
	F(UsingNamespaceDeclaration)\
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
	using ForwardRootType = VariableDeclaration;

	IDeclarationVisitor_ACCEPT;

	Ptr<Type>										type;
	bool											decoratorExtern = false;
	bool											decoratorStatic = false;
	bool											decoratorMutable = false;
	bool											decoratorThreadLocal = false;
	bool											decoratorRegister = false;
};

enum class CppMethodType
{
	Function,
	Constructor,
	Destructor,
	TypeConversion,
};

class ForwardFunctionDeclaration : public SpecializableDeclaration
{
public:
	using ForwardRootType = FunctionDeclaration;

	IDeclarationVisitor_ACCEPT;

	Ptr<Type>										type;
	CppMethodType									methodType = CppMethodType::Function;
	bool											decoratorExtern = false;
	bool											decoratorFriend = false;
	bool											decoratorStatic = false;
	bool											decoratorVirtual = false;
	bool											decoratorExplicit = false;
	bool											decoratorInline = false;
	bool											decoratorForceInline = false;
	bool											decoratorAbstract = false;
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

class ForwardClassDeclaration : public SpecializableDeclaration
{
public:
	using ForwardRootType = ClassDeclaration;

	IDeclarationVisitor_ACCEPT;

	CppClassType									classType;
	bool											decoratorFriend = false;
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

class TypeAliasDeclaration : public Declaration
{
public:
	IDeclarationVisitor_ACCEPT;

	Ptr<TemplateSpec>								templateSpec;
	Ptr<Type>										type;
};

class UsingNamespaceDeclaration : public Declaration
{
public:
	IDeclarationVisitor_ACCEPT;

	Ptr<Type>										type;
};

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