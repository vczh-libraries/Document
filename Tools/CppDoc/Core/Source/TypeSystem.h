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
	_COUNT,
};

enum class TsysBytes
{
	_1,
	_2,
	_4,
	_8,
	_10,
	_16,
	_COUNT,
};

struct TsysPrimitive
{
	TsysPrimitiveType			type = TsysPrimitiveType::Void;
	TsysBytes					bytes = TsysBytes::_1;

	TsysPrimitive() = default;
	TsysPrimitive(TsysPrimitiveType _type, TsysBytes _bytes) :type(_type), bytes(_bytes) {}
};

struct TsysCV
{
	bool						isConstExpr = false;
	bool						isConst = false;
	bool						isVolatile = false;

	TsysCV() = default;
	TsysCV(bool ce, bool c, bool v) :isConstExpr(ce), isConst(c), isVolatile(v) {}
};

const TsysCV		CV_CE	{ true,false,false };
const TsysCV		CV_C	{ false,true,false };
const TsysCV		CV_V	{ false,false,true };

enum class TsysType
{
	Primitive,		// Primitive
	LRef,			// Element
	RRef,			// Element
	Ptr,			// Element
	Array,			// Element, ParamCount
	Function,		// Element, ParamCount, Param
	Member,			// Element, Class
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
	virtual ITsys*				GetClass() = 0;
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
ITsysAlloc
***********************************************************************/

class ITsysAlloc abstract : public Interface
{
public:
	virtual ITsys*				PrimitiveOf(TsysPrimitive primitive) = 0;
	virtual ITsys*				DeclOf(Ptr<Declaration> decl) = 0;
	virtual ITsys*				GenericArgOf(Ptr<Declaration> decl) = 0;

	static Ptr<ITsysAlloc>		Create();
};

#endif