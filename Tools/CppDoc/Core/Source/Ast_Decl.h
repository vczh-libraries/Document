#ifndef VCZH_DOCUMENT_CPPDOC_AST_DECL
#define VCZH_DOCUMENT_CPPDOC_AST_DECL

#include "Ast.h"

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

class SpecializableDeclaration : public Object
{
public:
	Ptr<TemplateSpec>		templateSpec;
	Ptr<SpecializationSpec>	speclizationSpec;
};

class ForwardClassDeclaration : public SpecializableDeclaration
{
public:
	ClassType				classType;
};

class ForwardEnumDeclaration : public Declaration
{
public:
};

class VariableDeclaration : public Declaration
{
public:
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
	Ptr<Type>				type;
	MethodType				methodType;
	bool					externFunction = false;
	Ptr<Stat>				statement;
};

class TypeAliasDeclaration : public Declaration
{
public:
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
	ClassType										classType;
	List<Tuple<ClassAccessor ,Ptr<Type>>>			baseTypes;
	List<Tuple<ClassAccessor ,Ptr<Declaration>>>	decls;
};

// if using declaration has a name, then it is using a class member
// if using declaration doesn't have a name, then it is using a namespace
class UsingDeclaration : public Declaration
{
public:
	Ptr<Type>				type;
};

class NamespaceDeclaration : public Declaration
{
public:
	List<Ptr<Declaration>>	decls;
};

#endif