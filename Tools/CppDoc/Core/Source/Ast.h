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

class Declaration : public Object
{
public:
	CppName					name;
	Symbol*					symbol = nullptr;
};

class Type : public Object
{
public:
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