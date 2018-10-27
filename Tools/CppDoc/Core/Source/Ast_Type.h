#ifndef VCZH_DOCUMENT_CPPDOC_AST_TYPE
#define VCZH_DOCUMENT_CPPDOC_AST_TYPE

#include "Ast.h"

/***********************************************************************
Visitor
***********************************************************************/

#define CPPDOC_TYPE_LIST(F)\
	F(PrimitiveType)\
	F(ReferenceType)\
	F(ArrayType)\
	F(CallingConventionType)\
	F(FunctionType)\
	F(MemberType)\
	F(DeclType)\
	F(DecorateType)\
	F(RootType)\
	F(IdType)\
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
Preparation
***********************************************************************/

class ResolvableType : public Type
{
public:
	Ptr<Resolving>			resolving;
};

class VariableDeclaration;

/***********************************************************************
Types
***********************************************************************/

enum class CppPrimitiveType
{
	_auto,
	_void,
	_bool,
	_char, _wchar_t, _char16_t, _char32_t,
	_short, _int, ___int8, ___int16, ___int32, ___int64, _long, _long_int, _long_long,
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

	CppPrimitivePrefix		prefix = CppPrimitivePrefix::_none;
	CppPrimitiveType		primitive = CppPrimitiveType::_auto;

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

	CppReferenceType		reference = CppReferenceType::Ptr;
	Ptr<Type>				type;
};

class ArrayType : public Type
{
public:
	ITypeVisitor_ACCEPT;

	Ptr<Type>				type;
	Ptr<Expr>				expr;
};

class CallingConventionType : public Type
{
public:
	ITypeVisitor_ACCEPT;

	TsysCallingConvention	callingConvention = TsysCallingConvention::None;
	Ptr<Type>				type;
};

class FunctionType : public Type
{
public:
	ITypeVisitor_ACCEPT;

	Ptr<Type>						returnType;
	List<Ptr<VariableDeclaration>>	parameters;

	bool							qualifierConstExpr = false;
	bool							qualifierConst = false;
	bool							qualifierVolatile = false;
	bool							qualifierLRef = false;
	bool							qualifierRRef = false;

	bool							decoratorOverride = false;
	bool							decoratorNoExcept = false;
	bool							decoratorThrow = false;
	List<Ptr<Type>>					exceptions;
	Ptr<Type>						decoratorReturnType;
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

class RootType : public Type
{
public:
	ITypeVisitor_ACCEPT;
};

class IdType : public ResolvableType
{
public:
	ITypeVisitor_ACCEPT;

	CppName					name;
};

class ChildType : public ResolvableType
{
public:
	ITypeVisitor_ACCEPT;

	Ptr<Type>				classType;
	bool					typenameType = false;
	CppName					name;
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