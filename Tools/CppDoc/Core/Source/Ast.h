#ifndef VCZH_DOCUMENT_CPPDOC_AST
#define VCZH_DOCUMENT_CPPDOC_AST

#include "Lexer.h"

/***********************************************************************
Symbol
***********************************************************************/

class Symbol;

struct CppName
{
	bool					operatorName = false;
	vint					tokenCount = 0;
	WString					name;
	RegexToken				nameTokens[4];

	operator bool()const { return tokenCount != 0; }
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
	Ptr<Type>				type;
	CppName					name;
	Symbol*					createdSymbol = nullptr;
	Ptr<Initializer>		initializer;
};

#endif