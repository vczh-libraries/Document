#ifndef VCZH_DOCUMENT_CPPDOC_PARSER
#define VCZH_DOCUMENT_CPPDOC_PARSER

#include "Lexer.h"

/***********************************************************************
Symbol
***********************************************************************/

struct CppName
{
	bool					operatorName = false;
	vint					tokenCount = 1;
	WString					name;
	RegexToken				nameTokens[4];
};

class Symbol : public Object
{
	using SymbolGroup = Group<WString, Ptr<Symbol>>;
public:
	Symbol*					parent = nullptr;
	CppName					name;
	SymbolGroup				children;

	void					Add(Ptr<Symbol> child);
};

/***********************************************************************
Basic Concept
***********************************************************************/

class Type : public Object
{
public:
};

class Declarator : public Object
{
public:
	Ptr<Type>				type;
	CppName					name;
};

/***********************************************************************
Types
***********************************************************************/

enum class CppPrimitiveType
{
	_auto,
	_void,
	_bool,
	_char, _wchar_t, _char16_t, _char32_t,
	_signed_short, _signed_int, _signed___int8, _signed___int16, _signed___int32, _signed___int64, _signed_long, _signed_long_long,
	_unsigned_short, _unsigned_int, _unsigned___int8, _unsigned___int16, _unsigned___int32, _unsigned___int64, _unsigned_long, _unsigned_long_long,
	_float, _double, _long_double,
};

class IdType : public Type
{
public:
	CppName					name;
	Symbol*					resolvedSymbol = nullptr;
};

class PrimitiveType : public Type
{
public:
	CppPrimitiveType		primitive;
};

enum class CppReferenceType
{
	Ptr,
	LRef,
	RRef,
};

class ReferenceType : public Type
{
public:
	CppReferenceType		reference;
	Ptr<Type>				type;
};

class ArrayType : public Type
{
public:
	Ptr<Type>				type;
};

enum class CppCallingConvention
{
	CDecl,
	ClrCall,
	StdCall,
	FastCall,
	ThisCall,
	VectorCall,
};

class FunctionType : public Type
{
public:
	CppCallingConvention	callingConvention;
	Ptr<Type>				returnType;
	List<Ptr<Declarator>>	parameters;
};

class MemberType : public Type
{
public:
	Ptr<Type>				classType;
	Ptr<Type>				type;
};

class DeclType : public Type
{
public:
};

enum class CppDecorateType
{
	ConstExpr,
	Const,
	Volatile,
};

class DecorateType : public Type
{
public:
	CppDecorateType			decorate;
	Ptr<Type>				type;
};

class ChildType : public Type
{
public:
	Ptr<Type>				parent;
	CppName					name;
	Symbol*					resolvedSymbol = nullptr;
};

class GenericType : public Type
{
public:
	Ptr<Type>				parent;
	List<Ptr<Type>>			arguments;
};

class VariadicTemplateArgumentType : public Type
{
public:
	Ptr<Type>				type;
};

/***********************************************************************
Declarations
***********************************************************************/

/***********************************************************************
Parsers
***********************************************************************/

#endif