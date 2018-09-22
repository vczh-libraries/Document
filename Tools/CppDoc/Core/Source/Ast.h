#ifndef VCZH_DOCUMENT_CPPDOC_AST
#define VCZH_DOCUMENT_CPPDOC_AST

#include "Lexer.h"

/***********************************************************************
Basic Concept
***********************************************************************/

class Symbol;

struct CppName
{
	bool					operatorName = false;
	vint					tokenCount = 1;
	WString					name;
	RegexToken				nameTokens[4];
};

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

class Declarator : public Object
{
public:
	Ptr<Type>				type;
	CppName					name;
	Symbol*					symbol = nullptr;
};

class Expr : public Object
{
public:
};

class Stat : public Object
{
public:
};

#endif