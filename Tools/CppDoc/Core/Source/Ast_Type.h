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
	List<Symbol*>			resolvedSymbols;
};

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

class PrimitiveType : public Type
{
public:
	ITypeVisitor_ACCEPT;

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
	ITypeVisitor_ACCEPT;

	CppReferenceType		reference;
	Ptr<Type>				type;
};

class ArrayType : public Type
{
public:
	ITypeVisitor_ACCEPT;

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
	ITypeVisitor_ACCEPT;

	CppCallingConvention	callingConvention;
	Ptr<Type>				returnType;
	List<Ptr<Declarator>>	parameters;
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
	ITypeVisitor_ACCEPT;

	Ptr<Type>				parent;
	List<Ptr<Type>>			arguments;
};

class VariadicTemplateArgumentType : public Type
{
public:
	ITypeVisitor_ACCEPT;

	Ptr<Type>				type;
};

#endif