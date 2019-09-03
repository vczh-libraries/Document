#ifndef VCZH_DOCUMENT_CPPDOC_TYPESYSTEM
#define VCZH_DOCUMENT_CPPDOC_TYPESYSTEM

#include <Vlpp.h>

using namespace vl;
using namespace vl::collections;

class Symbol;
struct ParsingArguments;
class FunctionType;
class ITsys;

/***********************************************************************
ExprTsysType
***********************************************************************/

enum class ExprTsysType
{
	LValue,
	XValue,
	PRValue,
};

struct ExprHeader
{
	Symbol*					symbol = nullptr;
	ExprTsysType			type = ExprTsysType::PRValue;

	ExprHeader() = default;
	ExprHeader(const ExprHeader&) = default;
	ExprHeader(ExprHeader&&) = default;

	ExprHeader(Symbol* _symbol, ExprTsysType _type)
		:symbol(_symbol), type(_type)
	{
	}

	ExprHeader& operator=(const ExprHeader&) = default;
	ExprHeader& operator=(ExprHeader&&) = default;

	static vint Compare(const ExprHeader& a, const ExprHeader& b)
	{
		if (a.symbol < b.symbol) return -1;
		if (a.symbol > b.symbol) return 1;
		if (a.type < b.type) return -1;
		if (a.type > b.type) return 1;
		return 0;
	}

	bool operator==	(const ExprHeader& item)const { return Compare(*this, item) == 0; }
	bool operator!=	(const ExprHeader& item)const { return Compare(*this, item) != 0; }
	bool operator<	(const ExprHeader& item)const { return Compare(*this, item) < 0; }
	bool operator<=	(const ExprHeader& item)const { return Compare(*this, item) <= 0; }
	bool operator>	(const ExprHeader& item)const { return Compare(*this, item) > 0; }
	bool operator>=	(const ExprHeader& item)const { return Compare(*this, item) >= 0; }
};

struct ExprTsysItem : ExprHeader
{
	ITsys*					tsys = nullptr;

	ExprTsysItem() = default;
	ExprTsysItem(const ExprTsysItem&) = default;
	ExprTsysItem(ExprTsysItem&&) = default;

	ExprTsysItem(ExprHeader _header, ITsys* _tsys)
		:ExprHeader(_header), tsys(_tsys)
	{
	}

	ExprTsysItem(Symbol* _symbol, ExprTsysType _type, ITsys* _tsys)
		:ExprHeader(_symbol, _type), tsys(_tsys)
	{
	}

	ExprTsysItem& operator=(const ExprTsysItem&) = default;
	ExprTsysItem& operator=(ExprTsysItem&&) = default;

	static vint Compare(const ExprTsysItem& a, const ExprTsysItem& b)
	{
		if (a.symbol < b.symbol) return -1;
		if (a.symbol > b.symbol) return 1;
		if (a.type < b.type) return -1;
		if (a.type > b.type) return 1;
		if (a.tsys < b.tsys) return -1;
		if (a.tsys > b.tsys) return 1;
		return 0;
	}

	bool operator==	(const ExprTsysItem& item)const { return Compare(*this, item) == 0; }
	bool operator!=	(const ExprTsysItem& item)const { return Compare(*this, item) != 0; }
	bool operator<	(const ExprTsysItem& item)const { return Compare(*this, item) < 0;  }
	bool operator<=	(const ExprTsysItem& item)const { return Compare(*this, item) <= 0; }
	bool operator>	(const ExprTsysItem& item)const { return Compare(*this, item) > 0;  }
	bool operator>=	(const ExprTsysItem& item)const { return Compare(*this, item) >= 0; }
};

using TypeTsysList = List<ITsys*>;
using ExprTsysList = List<ExprTsysItem>;

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

enum class TsysCallingConvention
{
	CDecl,
	ClrCall,
	StdCall,
	FastCall,
	ThisCall,
	VectorCall,
	None,
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

struct TsysFunc
{
	TsysCallingConvention		callingConvention = TsysCallingConvention::CDecl;
	bool						ellipsis = false;

	TsysFunc() = default;
	TsysFunc(TsysCallingConvention _callingConvention, bool _ellipsis) :callingConvention(_callingConvention), ellipsis(_ellipsis) {}

	static vint Compare(const TsysFunc& a, const TsysFunc& b)
	{
		if (a.callingConvention < b.callingConvention) return -1;
		if (a.callingConvention > b.callingConvention) return 1;
		if (a.ellipsis < b.ellipsis) return -1;
		if (a.ellipsis > b.ellipsis) return 1;
		return 0;
	}
};

struct TsysInit
{
	List<ExprHeader>			headers;

	TsysInit() = default;
	TsysInit(const TsysInit& init) { CopyFrom(headers, init.headers); }
	TsysInit& operator=(const TsysInit& init) { CopyFrom(headers, init.headers); return *this; }

	static vint Compare(const TsysInit& a, const TsysInit& b)
	{
		return CompareEnumerable(a.headers, b.headers);
	}
};

struct TsysGenericFunction
{
	bool						isLastParameterVta = false;
	Symbol*						declSymbol = nullptr;
	Array<bool>					acceptTypes;

	TsysGenericFunction() = default;

	TsysGenericFunction(const TsysGenericFunction& genericFunction)
		:isLastParameterVta(genericFunction.isLastParameterVta)
		, declSymbol(genericFunction.declSymbol)
	{
		CopyFrom(acceptTypes, genericFunction.acceptTypes);
	}

	TsysGenericFunction& operator=(const TsysGenericFunction& genericFunction)
	{
		declSymbol = genericFunction.declSymbol;
		CopyFrom(acceptTypes, genericFunction.acceptTypes);
		return *this;
	}

	static vint Compare(const TsysGenericFunction& a, const TsysGenericFunction& b)
	{
		if (a.isLastParameterVta < b.isLastParameterVta) return -1;
		if (a.isLastParameterVta > b.isLastParameterVta) return 1;
		if (a.declSymbol < b.declSymbol) return -1;
		if (a.declSymbol > b.declSymbol) return 1;
		return CompareEnumerable(a.acceptTypes, b.acceptTypes);
	}
};

struct TsysGenericArg
{
	vint						argIndex = -1;
	Symbol*						argSymbol = nullptr;

	static vint Compare(const TsysGenericArg& a, const TsysGenericArg& b)
	{
		if (a.argIndex < b.argIndex) return -1;
		if (a.argIndex > b.argIndex) return 1;
		if (a.argSymbol < b.argSymbol) return -1;
		if (a.argSymbol > b.argSymbol) return 1;
		return 0;
	}

	bool operator==	(const TsysGenericArg& arg)const { return Compare(*this, arg) == 0; }
	bool operator!=	(const TsysGenericArg& arg)const { return Compare(*this, arg) != 0; }
	bool operator<	(const TsysGenericArg& arg)const { return Compare(*this, arg) < 0;  }
	bool operator<=	(const TsysGenericArg& arg)const { return Compare(*this, arg) <= 0; }
	bool operator>	(const TsysGenericArg& arg)const { return Compare(*this, arg) > 0;  }
	bool operator>=	(const TsysGenericArg& arg)const { return Compare(*this, arg) >= 0; }
};

namespace vl
{
	template<>
	struct POD<TsysPrimitive>
	{
		static const bool Result = true;
	};

	template<>
	struct POD<TsysCV>
	{
		static const bool Result = true;
	};

	template<>
	struct POD<TsysFunc>
	{
		static const bool Result = true;
	};

	template<>
	struct POD<TsysGenericArg>
	{
		static const bool Result = true;
	};
}

#define TSYS_TYPE_LIST(F)															\
	F(Any)																			\
	F(Zero)																			\
	F(Nullptr)																		\
	F(Primitive)		/* Primitive											*/	\
	F(LRef)				/* Element												*/	\
	F(RRef)				/* Element												*/	\
	F(Ptr)				/* Element												*/	\
	F(Array)			/* Element, ParamCount									*/	\
	F(Function)			/* Element, ParamCount, Param, Func						*/	\
	F(Member)			/* Element, Class										*/	\
	F(CV)				/* CV													*/	\
	F(Decl)				/* Decl													*/	\
	F(Init)				/* ParamCount, Param, Init								*/	\
	F(GenericFunction)	/* Element(Decl), ParamCount, Param, GenericFunction	*/	\
	F(GenericArg)		/* Element(Decl), GenericArg							*/	\

/*
	Any:				a type that could be any type
	Zero:				type of 0
	Nullptr:			type of nullptr
	Primitive:			primitive type
	LRef:				Element&
	RRef:				Element&&
	Ptr:				Element*
	Array:				Element[]
	Function:			(Params)->Element, Func contrains other configuration
	Member:				Element Class::
	CV:					Element const volatile
	Decl:				Type symbol of a declaration
	Init:				{Params}
	GenericFunction:	<Params>->Element, GenericFunction contains all type arguments
							For example: <T&&, U*>->Tuple<T, U>, GenericFunction contains T and U.
							If a pattern accepts a value, the pattern is the declaration of the argument symbol
	GenericArg:			The GenericArg.argIndex-th type argument in Element
*/

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
	// Any means this could be any conversion including illegal.
	// Any is not superior than any conversion, instead it should be always included in overloading resolutions.
	Any,
	Exact,
	TrivialConversion,
	IntegralPromotion,
	StandardConversion,
	UserDefinedConversion,
	Ellipsis,
	Illegal,
};

class ITsys abstract : public Interface
{
public:
	virtual TsysType					GetType() = 0;
	virtual TsysPrimitive				GetPrimitive() = 0;
	virtual TsysCV						GetCV() = 0;
	virtual ITsys*						GetElement() = 0;
	virtual ITsys*						GetClass() = 0;
	virtual ITsys*						GetParam(vint index) = 0;
	virtual vint						GetParamCount() = 0;
	virtual TsysFunc					GetFunc() = 0;
	virtual const TsysInit&				GetInit() = 0;
	virtual const TsysGenericFunction&	GetGenericFunction() = 0;
	virtual TsysGenericArg				GetGenericArg() = 0;
	virtual Symbol*						GetDecl() = 0;

	virtual ITsys*						LRefOf() = 0;
	virtual ITsys*						RRefOf() = 0;
	virtual ITsys*						PtrOf() = 0;
	virtual ITsys*						ArrayOf(vint dimensions) = 0;
	virtual ITsys*						FunctionOf(IEnumerable<ITsys*>& params, TsysFunc func) = 0;
	virtual ITsys*						MemberOf(ITsys* classType) = 0;
	virtual ITsys*						CVOf(TsysCV cv) = 0;
	virtual ITsys*						GenericFunctionOf(IEnumerable<ITsys*>& params, const TsysGenericFunction& genericFunction) = 0;
	virtual ITsys*						GenericArgOf(TsysGenericArg genericArg) = 0;

	virtual ITsys*						GetEntity(TsysCV& cv, TsysRefType& refType) = 0;
	virtual bool						IsUnknownType() = 0;
	virtual bool						HasGenericArg(const ParsingArguments& pa) = 0;
	virtual bool						HasUnknownType() = 0;
	virtual void						ReplaceGenericArgs(const ParsingArguments& pa, List<ITsys*>& output) = 0;
};

/***********************************************************************
ITsysAlloc
***********************************************************************/

class ITsysAlloc abstract : public Interface
{
public:
	virtual ITsys*				Void() = 0;
	virtual ITsys*				Any() = 0;
	virtual ITsys*				Zero() = 0;
	virtual ITsys*				Nullptr() = 0;
	virtual ITsys*				Int() = 0;
	virtual ITsys*				Size() = 0;
	virtual ITsys*				IntPtr() = 0;

	virtual ITsys*				PrimitiveOf(TsysPrimitive primitive) = 0;
	virtual ITsys*				DeclOf(Symbol* decl) = 0;
	virtual ITsys*				InitOf(Array<ExprTsysItem>& params) = 0;

	virtual vint				AllocateAnonymousCounter() = 0;

	static Ptr<ITsysAlloc>		Create();
};

/***********************************************************************
Helpers
***********************************************************************/

extern ITsys*					ApplyExprTsysType(ITsys* tsys, ExprTsysType type);
extern TsysConv					TestFunctionQualifier(TsysCV thisCV, TsysRefType thisRef, Ptr<FunctionType> funcType);
extern TsysConv					TestConvert(const ParsingArguments& pa, ITsys* toType, ExprTsysItem fromItem);

#endif