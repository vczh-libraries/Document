#ifndef VCZH_DOCUMENT_CPPDOC_TYPESYSTEM
#define VCZH_DOCUMENT_CPPDOC_TYPESYSTEM

#include <Vlpp.h>

using namespace vl;
using namespace vl::collections;

class Symbol;
struct ParsingArguments;

/***********************************************************************
Interface
***********************************************************************/

enum class TsysPrimitiveType
{
	SInt,
	UInt,
	Float,
	SChar,
	UChar,
	SWChar,
	UWChar,
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
	bool						isGeneralConst = false;
	bool						isVolatile = false;

	TsysCV() = default;
	TsysCV(bool c, bool v) :isGeneralConst(c), isVolatile(v) {}
};

#define TSYS_TYPE_LIST(F)									\
	F(Zero)													\
	F(Nullptr)												\
	F(Primitive)		/* Primitive					*/	\
	F(LRef)				/* Element						*/	\
	F(RRef)				/* Element						*/	\
	F(Ptr)				/* Element						*/	\
	F(Array)			/* Element, ParamCount			*/	\
	F(Function)			/* Element, ParamCount, Param	*/	\
	F(Member)			/* Element, Class				*/	\
	F(CV)				/* CV							*/	\
	F(Decl)				/* Decl							*/	\
	F(Generic)			/* Element, ParamCount, Param	*/	\
	F(GenericArg)		/* Decl							*/	\
	F(Expr)				/* ?							*/	\

enum class TsysType
{
#define DEFINE_TSYS_TYPE(NAME) NAME,
	TSYS_TYPE_LIST(DEFINE_TSYS_TYPE)
#undef DEFINE_TSYS_TYPE
};

enum class TsysRefType
{
	None,
	LRef,
	RRef,
};

enum class TsysConv
{
	Exact,
	TrivalConversion,
	IntegralPromotion,
	StandardConversion,
	UserDefinedConversion,
	Illegal,
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
	virtual Symbol*				GetDecl() = 0;

	virtual ITsys*				LRefOf() = 0;
	virtual ITsys*				RRefOf() = 0;
	virtual ITsys*				PtrOf() = 0;
	virtual ITsys*				ArrayOf(vint dimensions) = 0;
	virtual ITsys*				FunctionOf(IEnumerable<ITsys*>& params) = 0;
	virtual ITsys*				MemberOf(ITsys* classType) = 0;
	virtual ITsys*				CVOf(TsysCV cv) = 0;
	virtual ITsys*				GenericOf(IEnumerable<ITsys*>& params) = 0;

	virtual ITsys*				GetEntity(TsysCV& cv, TsysRefType& refType) = 0;
};

/***********************************************************************
ITsysAlloc
***********************************************************************/

class ITsysAlloc abstract : public Interface
{
public:
	virtual ITsys*				Zero() = 0;
	virtual ITsys*				Nullptr() = 0;
	virtual ITsys*				PrimitiveOf(TsysPrimitive primitive) = 0;
	virtual ITsys*				DeclOf(Symbol* decl) = 0;
	virtual ITsys*				GenericArgOf(Symbol* decl) = 0;

	static Ptr<ITsysAlloc>		Create();
};

/***********************************************************************
Helpers
***********************************************************************/

extern TsysConv					TestConvert(ParsingArguments& pa, ITsys* toType, ITsys* fromType);

#endif