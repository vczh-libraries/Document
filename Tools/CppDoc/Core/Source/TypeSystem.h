#ifndef VCZH_DOCUMENT_CPPDOC_TYPESYSTEM
#define VCZH_DOCUMENT_CPPDOC_TYPESYSTEM

#include <Vlpp.h>

using namespace vl;
using namespace vl::collections;

class Declaration;

/***********************************************************************
Interface
***********************************************************************/

enum class TsysPrimitiveType
{
	Signed,
	Unsigned,
	Float,
	Char,
	Bool,
	Void,
};

struct TsysPrimitive
{
	TsysPrimitiveType			type = TsysPrimitiveType::Void;
	vint						bytes = 0;

	TsysPrimitive() = default;
	TsysPrimitive(TsysPrimitiveType _type, vint _bytes) :type(_type), bytes(_bytes) {}
};

struct TsysCV
{
	bool						isConstExpr = false;
	bool						isConst = false;
	bool						isVolatile = false;

	TsysCV() = default;
	TsysCV(bool ce, bool c, bool v) :isConstExpr(ce), isConst(c), isVolatile(v) {}
};

enum class TsysType
{
	Primitive,		// Primitive
	LRef,			// Element
	RRef,			// Element
	Ptr,			// Element
	Array,			// Element, ParamCount
	Function,		// Element, ParamCount, Param
	Member,			// Element, ParamCount=1, Param(0)
	CV,				// CV
	Decl,			// Decl
	Generic,		// Element, ParamCount, Param
	GenericArg,		// Decl
	Expr,			// ?
};

class ITsys abstract : public Interface
{
public:
	virtual TsysType			GetType() = 0;
	virtual TsysPrimitive		GetPrimitive() = 0;
	virtual TsysCV				GetCV() = 0;
	virtual ITsys*				GetElement() = 0;
	virtual ITsys*				GetParam(vint index) = 0;
	virtual vint				GetParamCount() = 0;
	virtual Ptr<Declaration>	GetDecl() = 0;

	virtual ITsys*				LRefOf() = 0;
	virtual ITsys*				RRefOf() = 0;
	virtual ITsys*				PtrOf() = 0;
	virtual ITsys*				ArrayOf(vint dimensions) = 0;
	virtual ITsys*				FunctionOf(IEnumerable<ITsys*>& params) = 0;
	virtual ITsys*				MemberOf(ITsys* classType) = 0;
	virtual ITsys*				CVOf(TsysCV cv) = 0;
	virtual ITsys*				GenericOf(IEnumerable<ITsys*>& params) = 0;
};

/***********************************************************************
TypeAlloc
***********************************************************************/

class TypeAlloc : public Object
{
protected:
public:
	TypeAlloc();
	~TypeAlloc();

	ITsys*						PrimitiveOf(TsysPrimitive primitive);
	ITsys*						DeclOf(Ptr<Declaration> decl);
	ITsys*						GenericArgOf(Ptr<Declaration> decl);
};

#endif