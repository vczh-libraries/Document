#ifndef VCZH_DOCUMENT_CPPDOC_AST_TYPE
#define VCZH_DOCUMENT_CPPDOC_AST_TYPE

#include "Ast.h"

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
	List<Symbol*>			resolvedSymbols;
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

class DecorateType : public Type
{
public:
	Ptr<Type>				type;
	bool					isConstExpr = false;
	bool					isConst = false;
	bool					isVolatile = false;
};

// if parent is null, then it is from the root
class ChildType : public Type
{
public:
	Ptr<Type>				parent;
	CppName					name;
	List<Symbol*>			resolvedSymbols;
};

struct GenericParameter
{
	Ptr<Type>				type;
	Ptr<Expr>				expr;
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

#endif