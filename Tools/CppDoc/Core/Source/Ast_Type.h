#ifndef VCZH_DOCUMENT_CPPDOC_AST_TYPE
#define VCZH_DOCUMENT_CPPDOC_AST_TYPE

#include "Ast.h"

/***********************************************************************
Visitor
***********************************************************************/

#define CPPDOC_TYPE_LIST(F)\
	F(IdType)\
	F(PrimitiveType)\
	F(ReferenceType)\
	F(ArrayType)\
	F(CallingConventionType)\
	F(FunctionType)\
	F(MemberType)\
	F(DeclType)\
	F(DecorateType)\
	F(ChildType)\
	F(GenericType)\
	F(VariadicTemplateArgumentType)\

#define CPPDOC_FORWARD(NAME) class NAME;
CPPDOC_TYPE_LIST(CPPDOC_FORWARD)
#undef CPPDOC_FORWARD

class ITypeVisitor abstract : public virtual Interface
{
public:
#define CPPDOC_VISIT(NAME) virtual void Visit(NAME* self) = 0;
	CPPDOC_TYPE_LIST(CPPDOC_VISIT)
#undef CPPDOC_VISIT
};

#define ITypeVisitor_ACCEPT void Accept(ITypeVisitor* visitor)override

/***********************************************************************
Types
***********************************************************************/

class IdType : public Type
{
public:
	ITypeVisitor_ACCEPT;

	CppName					name;
	Ptr<Resolving>			resolving;
};

enum class CppPrimitiveType
{
	_auto,
	_void,
	_bool,
	_char, _wchar_t, _char16_t, _char32_t,
	_short, _int, ___int8, ___int16, ___int32, ___int64, _long, _long_long,
	_float, _double, _long_double,
};

enum class CppPrimitivePrefix
{
	_none,
	_signed,
	_unsigned,
};

class PrimitiveType : public Type
{
public:
	ITypeVisitor_ACCEPT;

	CppPrimitivePrefix		prefix;
	CppPrimitiveType		primitive;

	PrimitiveType() {}
	PrimitiveType(CppPrimitivePrefix _prefix, CppPrimitiveType _primitive) :prefix(_prefix), primitive(_primitive) {}
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
	ITypeVisitor_ACCEPT;

	CppReferenceType		reference;
	Ptr<Type>				type;
};

class ArrayType : public Type
{
public:
	ITypeVisitor_ACCEPT;

	Ptr<Type>				type;
	Ptr<Expr>				expr;
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

class CallingConventionType : public Type
{
public:
	ITypeVisitor_ACCEPT;

	CppCallingConvention	callingConvention;
	Ptr<Type>				type;
};

class FunctionType : public Type
{
public:
	ITypeVisitor_ACCEPT;

	Ptr<Type>				returnType;
	List<Ptr<Declarator>>	parameters;

	bool					qualifierConstExpr = false;
	bool					qualifierConst = false;
	bool					qualifierVolatile = false;
	bool					qualifierLRef = false;
	bool					qualifierRRef = false;

	bool					decoratorOverride = false;
	bool					decoratorNoExcept = false;
	bool					decoratorThrow = false;
	List<Ptr<Type>>			exceptions;
	Ptr<Type>				decoratorReturnType;
};

class MemberType : public Type
{
public:
	ITypeVisitor_ACCEPT;

	Ptr<Type>				classType;
	Ptr<Type>				type;
};

class DeclType : public Type
{
public:
	ITypeVisitor_ACCEPT;

	Ptr<Expr>				expr;
};

class DecorateType : public Type
{
public:
	ITypeVisitor_ACCEPT;

	Ptr<Type>				type;
	bool					isConstExpr = false;
	bool					isConst = false;
	bool					isVolatile = false;
};

// if parent is null, then it is from the root
class ChildType : public Type
{
public:
	ITypeVisitor_ACCEPT;

	Ptr<Type>				parent;
	bool					typenameType = false;
	CppName					name;
	Ptr<Resolving>			resolving;
};

struct GenericArgument
{
	Ptr<Type>				type;
	Ptr<Expr>				expr;
};

class GenericType : public Type
{
public:
	ITypeVisitor_ACCEPT;

	Ptr<Type>				type;
	List<GenericArgument>	arguments;
};

class VariadicTemplateArgumentType : public Type
{
public:
	ITypeVisitor_ACCEPT;

	Ptr<Type>				type;
};

#endif