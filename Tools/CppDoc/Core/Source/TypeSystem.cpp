#include "TypeSystem.h"
#include "Symbol_TemplateSpec.h"

class TsysAlloc;

#define DEFINE_TSYS_TYPE(NAME) class ITsys_##NAME;
TSYS_TYPE_LIST(DEFINE_TSYS_TYPE)
#undef DEFINE_TSYS_TYPE

template<typename T, vint BlockSize>
class ITsys_Allocator;

/***********************************************************************
TsysBase
***********************************************************************/

template<typename TType, typename TData>
struct WithParams
{
	IEnumerable<ITsys*>*	params;
	TType*					itsys;
	TData					data;

	static vint Compare(const WithParams<TType, TData>& a, const WithParams<TType, TData>& b)
	{
		vint result = TData::Compare(a.data, b.data);
		if (result != 0) return result;
		return CompareEnumerable(*a.params, *b.params);
	}
};
#define OPERATOR_COMPARE(OP)\
	template<typename TType, typename TData>\
	bool operator OP(const WithParams<TType, TData>& a, const WithParams<TType, TData>& b)\
	{\
		return WithParams<TType, TData>::Compare(a, b) OP 0;\
	}\

OPERATOR_COMPARE(>)
OPERATOR_COMPARE(>=)
OPERATOR_COMPARE(<)
OPERATOR_COMPARE(<=)
OPERATOR_COMPARE(==)
OPERATOR_COMPARE(!=)
#undef OPERATOR_COMPARE

template<typename TType, typename TData>
using WithParamsList = SortedList<WithParams<TType, TData>>;

class TsysBase : public ITsys
{

#define DEFINE_TSYS_TYPE(NAME) friend class ITsys_##NAME;
	TSYS_TYPE_LIST(DEFINE_TSYS_TYPE)
#undef DEFINE_TSYS_TYPE
protected:
	TsysAlloc*														tsys;
	ITsys_LRef*														lrefOf = nullptr;
	ITsys_RRef*														rrefOf = nullptr;
	ITsys_Ptr*														ptrOf = nullptr;
	Dictionary<vint, ITsys_Array*>									arrayOf;
	Dictionary<ITsys*, ITsys_Member*>								memberOf;
	ITsys_CV*														cvOf[3] = { 0 };
	WithParamsList<ITsys_Function, TsysFunc>						functionOf;
	WithParamsList<ITsys_GenericFunction, TsysGenericFunction>		genericFunctionOf;

	virtual ITsys* GetEntityInternal(TsysCV& cv, TsysRefType& refType)
	{
		return this;
	}

public:
	TsysBase(TsysAlloc* _tsys) :tsys(_tsys) {}

	TsysPrimitive													GetPrimitive()						override { throw L"Not Implemented!"; }
	TsysCV															GetCV()								override { throw L"Not Implemented!"; }
	ITsys*															GetElement()						override { throw L"Not Implemented!"; }
	ITsys*															GetClass()							override { throw L"Not Implemented!"; }
	ITsys*															GetParam(vint index)				override { throw L"Not Implemented!"; }
	vint															GetParamCount()						override { throw L"Not Implemented!"; }
	TsysFunc														GetFunc()							override { throw L"Not Implemented!"; }
	const TsysInit&													GetInit()							override { throw L"Not Implemented!"; }
	const TsysGenericFunction&										GetGenericFunction()				override { throw L"Not Implemented!"; }
	TsysGenericArg													GetGenericArg()						override { throw L"Not Implemented!"; }
	Symbol*															GetDecl()							override { throw L"Not Implemented!"; }
	const TsysDeclInstant&											GetDeclInstant()					override { throw L"Not Implemented!"; }
	TsysPSRecord*													GetPSRecord()						override { throw L"Not Implemented!"; }
	void															MakePSRecordPrimaryThis()			override { throw L"Not Implemented!"; }

	ITsys* LRefOf()																						override;
	ITsys* RRefOf()																						override;
	ITsys* PtrOf()																						override;
	ITsys* ArrayOf(vint dimensions)																		override;
	ITsys* FunctionOf(IEnumerable<ITsys*>& params, TsysFunc func)										override;
	ITsys* MemberOf(ITsys* classType)																	override;
	ITsys* CVOf(TsysCV cv)																				override;
	ITsys* GenericFunctionOf(IEnumerable<ITsys*>& params, const TsysGenericFunction& genericFunction)	override;
	ITsys* GenericArgOf(TsysGenericArg genericArg)														override { throw L"Not Implemented!"; }

	ITsys* GetEntity(TsysCV& cv, TsysRefType& refType)override
	{
		cv = { false,false };
		refType = TsysRefType::None;
		return GetEntityInternal(cv, refType);
	}

	bool IsUnknownType()override
	{
		return false;
	}

	ITsys* ReplaceGenericArgs(const ParsingArguments& pa)override
	{
		return this;
	}
};

template<TsysType Type>
class TsysBase_ : public TsysBase
{
public:
	TsysBase_(TsysAlloc* _tsys) :TsysBase(_tsys) {}

	TsysType GetType()override { return Type; }
};

bool IsGenericArgInContext(const ParsingArguments& pa, ITsys* key)
{
	ITsys* replacedType = nullptr;
	return pa.TryGetReplacedGenericArg(key, replacedType);
}

/***********************************************************************
Concrete Tsys (Commons Members)
***********************************************************************/

#define ITSYS_CLASS(TYPE) ITsys_##TYPE : public TsysBase_<TsysType::TYPE>

#define ITSYS_HAS_GENERIC_TYPE(VALUE)	public: bool HasGenericArg(const ParsingArguments& pa)override { return VALUE; }
#define ITSYS_HAS_UNKNOWN_TYPE(VALUE)	public: bool HasUnknownType()override { return VALUE; }

#define ITSYS_MEMBERS_WITHELEMENT_SHARED															\
	protected:																						\
		TsysBase*			element;																\
	public:																							\
		ITsys* GetElement()override { return element; }												\

#define ITSYS_MEMBERS_WITHPARAMS_SHARED(TYPE, DATA, DATA_RET, NAME)									\
	protected:																						\
		List<ITsys*>		params;																	\
		DATA				data;																	\
	public:																							\
		List<ITsys*>& GetParams() { return params; }												\
		ITsys* GetParam(vint index)override { return params.Get(index); }							\
		vint GetParamCount()override { return params.Count(); }										\
		DATA_RET Get##NAME()override { return data; }												\

/***********************************************************************
Concrete Tsys (Members)
***********************************************************************/

#define ITSYS_MEMBERS_MINIMIZED(TYPE)																\
	public:																							\
		ITsys_##TYPE(TsysAlloc* _tsys) :TsysBase_(_tsys) {}											\

#define ITSYS_MEMBERS_DATA(TYPE, DATA, NAME)														\
	protected:																						\
		DATA				data;																	\
	public:																							\
		ITsys_##TYPE(TsysAlloc* _tsys, DATA const& _data) :TsysBase_(_tsys), data(_data) {}			\
		DATA Get##NAME()override { return data; }													\

#define ITSYS_MEMBERS_REF(TYPE)																		\
	ITSYS_MEMBERS_WITHELEMENT_SHARED																\
	public:																							\
		ITsys_##TYPE(TsysAlloc* _tsys, TsysBase* _element)											\
			:TsysBase_(_tsys), element(_element) {}													\

#define ITSYS_MEMBERS_DATA_WITHELEMENT(TYPE, DATA, NAME)											\
	ITSYS_MEMBERS_WITHELEMENT_SHARED																\
	protected:																						\
		DATA				data;																	\
	public:																							\
		ITsys_##TYPE(TsysAlloc* _tsys, TsysBase* _element, DATA const& _data)						\
			:TsysBase_(_tsys), element(_element), data(_data) {}									\
		DATA Get##NAME()override { return data; }													\

#define ITSYS_MEMBERS_WITHPARAMS_WITH_ELEMENT(TYPE, DATA, DATA_RET, NAME)							\
	ITSYS_MEMBERS_WITHPARAMS_SHARED(TYPE, DATA, DATA_RET, NAME)										\
	protected:																						\
		TsysBase*			element;																\
	public:																							\
		ITsys_##TYPE(TsysAlloc* _tsys, TsysBase* _element, DATA const& _data)						\
			:TsysBase_(_tsys), element(_element), data(_data) {}									\
		ITsys* GetElement()override { return element; }												\

#define ITSYS_MEMBERS_WITHPARAMS_WITHOUT_ELEMENT(TYPE, DATA, DATA_RET, NAME)						\
	ITSYS_MEMBERS_WITHPARAMS_SHARED(TYPE, DATA, DATA_RET, NAME)										\
	public:																							\
		ITsys_##TYPE(TsysAlloc* _tsys, TsysBase*, DATA const& _data)								\
			:TsysBase_(_tsys), data(_data) {}														\

/***********************************************************************
Concrete Tsys (ReplaceGenericArgs for type with element)
***********************************************************************/

#define ITSYS_REPLACE_GENERIC_ARGS_WITH_ELEMENT(NAME, DATA)											\
	public:																							\
		bool HasGenericArg(const ParsingArguments& pa)override										\
		{																							\
			return element->HasGenericArg(pa);														\
		}																							\
		bool HasUnknownType()override																\
		{																							\
			return element->HasUnknownType();														\
		}																							\
		ITsys* ReplaceGenericArgs(const ParsingArguments& pa)override								\
		{																							\
			auto elementResult = element->ReplaceGenericArgs(pa);									\
			if (elementResult == element)															\
			{																						\
				return this;																		\
			}																						\
			else																					\
			{																						\
				return elementResult->NAME(DATA);													\
			}																						\
		}																							\

/***********************************************************************
Concrete Tsys (GenericArgs)
***********************************************************************/

#define ITSYS_GENERIC_REPLACEGENERICARGS															\
	ITsys* ReplaceGenericArgs(const ParsingArguments& pa)override									\
	{																								\
		ITsys* replacedType = nullptr;																\
		if (pa.TryGetReplacedGenericArg(this, replacedType))										\
		{																							\
			return replacedType;																	\
		}																							\
		return this;																				\
	}																								\

#define ITSYS_GENERIC_ARG_CONFIGURATION																\
	bool IsUnknownType()override																	\
	{																								\
		return true;																				\
	}																								\
	bool HasGenericArg(const ParsingArguments& pa)override											\
	{																								\
		return IsGenericArgInContext(pa, this);														\
	}																								\
	bool HasUnknownType()override																	\
	{																								\
		return true;																				\
	}																								\
	ITSYS_GENERIC_REPLACEGENERICARGS																\

/***********************************************************************
Concrete Tsys (Singleton)
***********************************************************************/

class ITSYS_CLASS(Any)
{
	ITSYS_MEMBERS_MINIMIZED(Any)
	ITSYS_HAS_GENERIC_TYPE(false)
	ITSYS_HAS_UNKNOWN_TYPE(true)

public:
	bool IsUnknownType()override
	{
		return true;
	}
		
public:
	ITsys* LRefOf()																						override { return this; }
	ITsys* RRefOf()																						override { return this; }
	ITsys* PtrOf()																						override { return this; }
	ITsys* ArrayOf(vint dimensions)																		override { return this; }
	ITsys* MemberOf(ITsys* classType)																	override { return this; }
	ITsys* CVOf(TsysCV cv)																				override { return this; }
	ITsys* GenericArgOf(TsysGenericArg genericArg)														override { return this; }

};

class ITSYS_CLASS(Zero)
{
	ITSYS_MEMBERS_MINIMIZED(Zero)
	ITSYS_HAS_GENERIC_TYPE(false)
	ITSYS_HAS_UNKNOWN_TYPE(false)

public:
	ITsys* LRefOf()override
	{
		return this;
	}

	ITsys* RRefOf()override
	{
		return this;
	}

	ITsys* CVOf(TsysCV cv)override
	{
		return this;
	}
};

class ITSYS_CLASS(Nullptr)
{
	ITSYS_MEMBERS_MINIMIZED(Nullptr)
	ITSYS_HAS_GENERIC_TYPE(false)
	ITSYS_HAS_UNKNOWN_TYPE(false)

public:
	ITsys* LRefOf()override
	{
		return this;
	}

	ITsys* RRefOf()override
	{
		return this;
	}

	ITsys* CVOf(TsysCV cv)override
	{
		return this;
	}
};

class ITSYS_CLASS(Primitive)
{
	ITSYS_MEMBERS_DATA(Primitive, TsysPrimitive, Primitive)
	ITSYS_HAS_GENERIC_TYPE(false)
	ITSYS_HAS_UNKNOWN_TYPE(false)
};

/***********************************************************************
Concrete Tsys (User Defined)
***********************************************************************/

class ITSYS_CLASS(Decl)
{
	ITSYS_MEMBERS_DATA(Decl, Symbol*, Decl)
	ITSYS_HAS_GENERIC_TYPE((data->kind == symbol_component::SymbolKind::GenericTypeArgument || data->kind == symbol_component::SymbolKind::GenericValueArgument))
	ITSYS_HAS_UNKNOWN_TYPE(false)
	ITSYS_GENERIC_REPLACEGENERICARGS

protected:
	Dictionary<TsysGenericArg, ITsys*>				genericArgs;
	Ptr<TsysPSRecord>								psRecord;

public:
	ITsys*											GenericArgOf(TsysGenericArg genericArg)override;
	TsysPSRecord*									GetPSRecord()override;
};

class ITSYS_CLASS(GenericArg)
{
	ITSYS_MEMBERS_DATA_WITHELEMENT(GenericArg, TsysGenericArg, GenericArg)
	ITSYS_GENERIC_ARG_CONFIGURATION

public:
	Symbol* GetDecl()override
	{
		return element->GetDecl();
	}
};

/***********************************************************************
Concrete Tsys (Element Only)
***********************************************************************/

class ITSYS_CLASS(LRef)
{
	ITSYS_MEMBERS_REF(LRef)
	ITSYS_REPLACE_GENERIC_ARGS_WITH_ELEMENT(LRefOf, )

public:
	ITsys* LRefOf()override
	{
		return this;
	}

	ITsys* RRefOf()override
	{
		return this;
	}

	ITsys* CVOf(TsysCV cv)override
	{
		return this;
	}

protected:
	ITsys* GetEntityInternal(TsysCV& cv, TsysRefType& refType)override
	{
		refType = TsysRefType::LRef;
		return element->GetEntityInternal(cv, refType);
	}
};

class ITSYS_CLASS(RRef)
{
	ITSYS_MEMBERS_REF(RRef)
	ITSYS_REPLACE_GENERIC_ARGS_WITH_ELEMENT(RRefOf, )

public:
	ITsys* LRefOf()override
	{
		return element->LRefOf();
	}

	ITsys* RRefOf()override
	{
		return this;
	}

	ITsys* CVOf(TsysCV cv)override
	{
		return this;
	}

protected:
	ITsys* GetEntityInternal(TsysCV& cv, TsysRefType& refType)override
	{
		refType = TsysRefType::RRef;
		return element->GetEntityInternal(cv, refType);
	}
};

class ITSYS_CLASS(Ptr)
{
	ITSYS_MEMBERS_REF(Ptr)
	ITSYS_REPLACE_GENERIC_ARGS_WITH_ELEMENT(PtrOf, )
};

class ITSYS_CLASS(Array)
{
	ITSYS_MEMBERS_DATA_WITHELEMENT(Array, vint, ParamCount)
	ITSYS_REPLACE_GENERIC_ARGS_WITH_ELEMENT(ArrayOf, data)
};

class ITSYS_CLASS(CV)
{
	ITSYS_MEMBERS_DATA_WITHELEMENT(CV, TsysCV, CV)
	ITSYS_REPLACE_GENERIC_ARGS_WITH_ELEMENT(CVOf, data)

	ITsys* CVOf(TsysCV cv)override
	{
		cv.isGeneralConst |= data.isGeneralConst;
		cv.isVolatile |= data.isVolatile;
		return element->CVOf(cv);
	}
protected:
	ITsys* GetEntityInternal(TsysCV& cv, TsysRefType& refType)override
	{
		cv = data;
		return element->GetEntityInternal(cv, refType);
	}
};

class ITSYS_CLASS(Member)
{
	ITSYS_MEMBERS_DATA_WITHELEMENT(Member, ITsys*, Class)

	bool HasGenericArg(const ParsingArguments& pa)override
	{
		return element->HasGenericArg(pa) || data->HasGenericArg(pa);
	}

	bool HasUnknownType()override
	{
		return element->HasUnknownType() || data->HasUnknownType();
	}

	ITsys* ReplaceGenericArgs(const ParsingArguments& pa)override
	{
		if (!HasGenericArg(pa))
		{
			return this;
		}
		else
		{
			auto classResult = data->ReplaceGenericArgs(pa);
			auto memberResult = element->ReplaceGenericArgs(pa);
			return memberResult->MemberOf(classResult);
		}
	}
};

/***********************************************************************
Concrete Tsys (Params)
***********************************************************************/

template<typename TType, typename TData, vint BlockSize>
ITsys* ParamsOf(IEnumerable<ITsys*>& params, const TData& data, WithParamsList<TType, TData>& paramsOf, TsysBase* element, TsysAlloc* tsys, ITsys_Allocator<TType, BlockSize> TsysAlloc::* alloc)
{
	WithParams<TType, TData> key;
	key.params = &params;
	key.itsys = nullptr;
	key.data = data;
	vint index = paramsOf.IndexOf(key);
	if (index != -1) return paramsOf[index].itsys;

	auto itsys = (tsys->*alloc).Alloc(tsys, element, data);
	CopyFrom(itsys->GetParams(), params);
	key.params = &itsys->GetParams();
	key.itsys = itsys;
	paramsOf.Add(key);
	return itsys;
}

bool HasGenericArgWithParams(List<ITsys*>& params, ITsys* element, const ParsingArguments& pa)
{
	if (element)
	{
		if (element->HasGenericArg(pa))
		{
			return true;
		}
	}
	for (vint i = 0; i < params.Count(); i++)
	{
		if (params[i] && params[i]->HasGenericArg(pa))
		{
			return true;
		}
	}
	return false;
}

bool HasUnknownTypeWithParams(List<ITsys*>& params, ITsys* element)
{
	if (element)
	{
		if (element->HasUnknownType())
		{
			return true;
		}
	}
	for (vint i = 0; i < params.Count(); i++)
	{
		if (params[i]->HasUnknownType())
		{
			return true;
		}
	}
	return false;
}

template<typename TITsys>
ITsys* ReplaceGenericArgsWithParams(const ParsingArguments& pa, List<ITsys*>& params, ITsys* element, TITsys* self, ITsys* (TITsys::* callback)(ITsys* element, Array<ITsys*>& params))
{
	Array<vint> indices(params.Count());
	Array<ITsys*> replacedParams(params.Count());
	Array<ITsys*> selectedParams(params.Count());

	ITsys* elementResult = nullptr;
	if (element)
	{
		elementResult = element->ReplaceGenericArgs(pa);
	}

	Array<ITsys*> paramResults(params.Count());
	for (vint i = 0; i < params.Count(); i++)
	{
		paramResults[i] = params[i]->ReplaceGenericArgs(pa);
	}
	return (self->*callback)(elementResult, paramResults);
}

#define ITSYS_REPLACE_GENERIC_ARGS_WITHPARAMS(ELEMENT)																								\
public:																																				\
	bool HasGenericArg(const ParsingArguments& pa)override																							\
	{																																				\
		return HasGenericArgWithParams(params, ELEMENT, pa);																						\
	}																																				\
	bool HasUnknownType()override																													\
	{																																				\
		return HasUnknownTypeWithParams(params, ELEMENT);																							\
	}																																				\
	ITsys* ReplaceGenericArgs(const ParsingArguments& pa)override																					\
	{																																				\
		if (HasGenericArg(pa))																														\
		{																																			\
			return ReplaceGenericArgsWithParams(pa, params, ELEMENT, this, &RemoveReference<decltype(*this)>::Type::ReplaceGenericArgsCallback);	\
		}																																			\
		else																																		\
		{																																			\
			return this;																															\
		}																																			\
	}																																				\

class ITSYS_CLASS(Function)
{
	ITSYS_MEMBERS_WITHPARAMS_WITH_ELEMENT(Function, TsysFunc, TsysFunc, Func)
	ITSYS_REPLACE_GENERIC_ARGS_WITHPARAMS(element)

private:
	ITsys*					ReplaceGenericArgsCallback(ITsys* element, Array<ITsys*>& params);
};

class ITSYS_CLASS(Init)
{
	ITSYS_MEMBERS_WITHPARAMS_WITHOUT_ELEMENT(Init, TsysInit, const TsysInit&, Init)
	ITSYS_REPLACE_GENERIC_ARGS_WITHPARAMS(nullptr)

	ITsys* LRefOf()																						override { return this; }
	ITsys* RRefOf()																						override { return this; }
	ITsys* CVOf(TsysCV cv)																				override { return this; }

private:
	ITsys*					ReplaceGenericArgsCallback(ITsys* element, Array<ITsys*>& params);
};

class ITSYS_CLASS(GenericFunction)
{
	ITSYS_MEMBERS_WITHPARAMS_WITH_ELEMENT(GenericFunction, TsysGenericFunction, const TsysGenericFunction&, GenericFunction)
	ITSYS_GENERIC_ARG_CONFIGURATION
};

class ITSYS_CLASS(DeclInstant)
{
	ITSYS_MEMBERS_WITHPARAMS_WITH_ELEMENT(DeclInstant, TsysDeclInstant, const TsysDeclInstant&, DeclInstant)
	ITSYS_REPLACE_GENERIC_ARGS_WITHPARAMS(element)

	Symbol* GetDecl()override
	{
		return data.declSymbol;
	}

private:
	ITsys*					ReplaceGenericArgsCallback(ITsys* element, Array<ITsys*>& params);

protected:
	Ptr<TsysPSRecord>		psRecord;

public:
	TsysPSRecord*			GetPSRecord()override;
	void					MakePSRecordPrimaryThis()override;
};

#undef ITSYS_MEMBERS_DATA
#undef ITSYS_MEMBERS_REF
#undef ITSYS_MEMBERS_DECORATE
#undef ITSYS_MEMBERS_WITHPARAMS
#undef ITSYS_CLASS

/***********************************************************************
ITsys_Allocator
***********************************************************************/

template<typename T, vint BlockSize>
class ITsys_Allocator : public Object
{
protected:
	struct Node
	{
		char				items[BlockSize * sizeof(T)];
		vint				used = 0;
		Node*				next = nullptr;

		~Node()
		{
			auto itsys = (T*)items;
			for (vint i = 0; i < used; i++)
			{
				itsys[i].~T();
			}
		}
	};

	Node*					firstNode = nullptr;
	Node*					lastNode = nullptr;
public:

	~ITsys_Allocator()
	{
		auto current = firstNode;
		while (current)
		{
			auto next = current->next;
			delete current;
			current = next;
		}
	}

	template<typename ...TArgs>
	T* Alloc(TArgs&& ...args)
	{
		if (!firstNode)
		{
			firstNode = lastNode = new Node;
		}

		if (lastNode->used == BlockSize)
		{
			lastNode->next = new Node;
			lastNode = lastNode->next;
		}

		auto itsys = &((T*)lastNode->items)[lastNode->used++];
#ifdef VCZH_CHECK_MEMORY_LEAKS_NEW
#undef new
#endif
		return new(itsys)T(ForwardValue<TArgs&&>(args)...);
#ifdef VCZH_CHECK_MEMORY_LEAKS_NEW
#define new VCZH_CHECK_MEMORY_LEAKS_NEW
#endif
	}
};

/***********************************************************************
ITsysAlloc
***********************************************************************/

class TsysAlloc : public Object, public ITsysAlloc
{
protected:
	ITsys_Any												tsysAny;
	ITsys_Zero												tsysZero;
	ITsys_Nullptr											tsysNullptr;
	ITsys_Primitive*										primitives[(vint)TsysPrimitiveType::_COUNT * (vint)TsysBytes::_COUNT] = { 0 };
	Dictionary<Symbol*, ITsys_Decl*>						decls;
	WithParamsList<ITsys_DeclInstant, TsysDeclInstant>		declInstantOf;
	WithParamsList<ITsys_Init, TsysInit>					initOf;
	vint													anonymousCounter = 0;

public:
	ITsys_Allocator<ITsys_Primitive,			1024>		_primitive;
	ITsys_Allocator<ITsys_LRef,					1024>		_lref;
	ITsys_Allocator<ITsys_RRef,					1024>		_rref;
	ITsys_Allocator<ITsys_Ptr,					1024>		_ptr;
	ITsys_Allocator<ITsys_Array,				1024>		_array;
	ITsys_Allocator<ITsys_Function,				1024>		_function;
	ITsys_Allocator<ITsys_Member,				1024>		_member;
	ITsys_Allocator<ITsys_CV,					1024>		_cv;
	ITsys_Allocator<ITsys_Decl,					1024>		_decl;
	ITsys_Allocator<ITsys_DeclInstant,			1024>		_declInstant;
	ITsys_Allocator<ITsys_Init,					1024>		_init;
	ITsys_Allocator<ITsys_GenericFunction,		1024>		_genericFunction;
	ITsys_Allocator<ITsys_GenericArg,			1024>		_genericArg;

	TsysAlloc()
		:tsysAny(this)
		, tsysZero(this)
		, tsysNullptr(this)
	{
	}

	ITsys* Void()override
	{
		return PrimitiveOf({ TsysPrimitiveType::Void,TsysBytes::_1 });
	}

	ITsys* Any()override
	{
		return &tsysAny;
	}

	ITsys* Zero()override
	{
		return &tsysZero;
	}

	ITsys* Nullptr()override
	{
		return &tsysNullptr;
	}

	ITsys* Int()override
	{
		// TODO: Platform Specific
		return PrimitiveOf({ TsysPrimitiveType::SInt,TsysBytes::_4 });
	}

	ITsys* Size()override
	{
		// TODO: Platform Specific
		return PrimitiveOf({ TsysPrimitiveType::UInt,TsysBytes::_4 });
	}

	ITsys* IntPtr()override
	{
		// TODO: Platform Specific
		return PrimitiveOf({ TsysPrimitiveType::SInt,TsysBytes::_4 });
	}

	ITsys* PrimitiveOf(TsysPrimitive primitive)override
	{
		vint a = (vint)primitive.type;
		vint b = (vint)primitive.bytes;
		vint index = (vint)TsysBytes::_COUNT * a + b;
		if (index > sizeof(primitives) / sizeof(*primitives)) throw L"Not Implemented!";

		auto& itsys = primitives[index];
		if (!itsys)
		{
			itsys = _primitive.Alloc(this, primitive);
		}
		return itsys;
	}

	ITsys* DeclOf(Symbol* decl)override
	{
		vint index = decls.Keys().IndexOf(decl);
		if (index != -1) return decls.Values()[index];
		auto itsys = _decl.Alloc(this, decl);
		decls.Add(decl, itsys);
		return itsys;
	}

	ITsys* DeclInstantOf(Symbol* decl, IEnumerable<ITsys*>* params, ITsys* parentDeclType)override
	{
		auto spec = symbol_type_resolving::GetTemplateSpecFromSymbol(decl);
		if (spec ^ (params != nullptr))
		{
			throw L"Template class should have template argument provided.";
		}

		if (!params && !parentDeclType)
		{
			throw L"DeclOf should be called instead if params and parentDeclType are both nullptr.";
		}

		switch (decl->kind)
		{
		case CLASS_SYMBOL_KIND:
			break;
		default:
			throw L"Decl should be a class.";
		}

		if (parentDeclType)
		{
			switch (parentDeclType->GetType())
			{
			case TsysType::DeclInstant:
				break;
			default:
				throw L"parentDeclType should be a template class or a class in a template class.";
			}
		}

		{
			TsysDeclInstant data;
			data.declSymbol = decl;
			data.parentDeclType = parentDeclType;

			Array<ITsys*> noParams;
			auto itsys = ParamsOf(
				(params ? *params : noParams),
				data,
				declInstantOf,
				dynamic_cast<TsysBase*>(parentDeclType),
				this,
				&TsysAlloc::_declInstant
				);

			if (params)
			{
				auto& declInstant = const_cast<TsysDeclInstant&>(itsys->GetDeclInstant());
				if (!declInstant.taContext)
				{
					Ptr<TemplateArgumentContext> parentTaContext;
					if (parentDeclType)
					{
						parentTaContext = parentDeclType->GetDeclInstant().taContext;
					}

					auto taContext = MakePtr<TemplateArgumentContext>();
					taContext->parent = parentTaContext.Obj();
					taContext->symbolToApply = decl;
					
					FOREACH_INDEXER(ITsys*, param, index, *params)
					{
						if (index >= spec->arguments.Count())
						{
							throw L"The number of template argument should match the definition.";
						}
						taContext->arguments.Add(
							symbol_type_resolving::GetTemplateArgumentKey(spec->arguments[index], this),
							param
							);
					}
					if (taContext->arguments.Count() != spec->arguments.Count())
					{
						throw L"The number of template argument should match the definition.";
					}
					declInstant.taContext = taContext;
				}
			}
			return itsys;
		}
	}

	ITsys* InitOf(Array<ExprTsysItem>& params)override
	{
		List<ITsys*> tsys;
		TsysInit data;

		for (vint i = 0; i < params.Count(); i++)
		{
			tsys.Add(params[i].tsys);
			data.headers.Add(params[i]);
		}
		return ParamsOf(tsys, data, initOf, nullptr, this, &TsysAlloc::_init);
	}

	vint AllocateAnonymousCounter()
	{
		return anonymousCounter++;
	}
};

Ptr<ITsysAlloc> ITsysAlloc::Create()
{
	return new TsysAlloc;
}

/***********************************************************************
TsysBase (Impl)
***********************************************************************/

ITsys* TsysBase::LRefOf()
{
	if (!lrefOf) lrefOf = tsys->_lref.Alloc(tsys, this);
	return lrefOf;
}

ITsys* TsysBase::RRefOf()
{
	if (!rrefOf) rrefOf = tsys->_rref.Alloc(tsys, this);
	return rrefOf;
}

ITsys* TsysBase::PtrOf()
{
	if (!ptrOf) ptrOf = tsys->_ptr.Alloc(tsys, this);
	return ptrOf;
}

ITsys* TsysBase::ArrayOf(vint dimensions)
{
	vint index = arrayOf.Keys().IndexOf(dimensions);
	if (index != -1) return arrayOf.Values()[index];
	auto itsys = tsys->_array.Alloc(tsys, this, dimensions);
	arrayOf.Add(dimensions, itsys);
	return itsys;
}

ITsys* TsysBase::FunctionOf(IEnumerable<ITsys*>& params, TsysFunc func)
{
	return ParamsOf(params, func, functionOf, this, tsys, &TsysAlloc::_function);
}

ITsys* TsysBase::MemberOf(ITsys* classType)
{
	if (!classType) throw L"classType should not be nullptr";
	vint index = memberOf.Keys().IndexOf(classType);
	if (index != -1) return memberOf.Values()[index];
	auto itsys = tsys->_member.Alloc(tsys, this, classType);
	memberOf.Add(classType, itsys);
	return itsys;
}

ITsys* TsysBase::CVOf(TsysCV cv)
{
	vint index = ((cv.isGeneralConst ? 1 : 0) << 1) + (cv.isVolatile ? 1 : 0);

	if (index == 0)
	{
		return this;
	}
	else
	{
		index--;
	}

	if (index > sizeof(cvOf) / sizeof(*cvOf)) throw L"Not Implemented!";
	auto& itsys = cvOf[index];
	if (!itsys) itsys = tsys->_cv.Alloc(tsys, this, cv);
	return itsys;
}

ITsys* TsysBase::GenericFunctionOf(IEnumerable<ITsys*>& params, const TsysGenericFunction& genericFunction)
{
	return ParamsOf(params, genericFunction, genericFunctionOf, this, tsys, &TsysAlloc::_genericFunction);
}

/***********************************************************************
ITsys_Decl (Impl)
***********************************************************************/

ITsys* ITsys_Decl::GenericArgOf(TsysGenericArg genericArg)
{
	vint index = genericArgs.Keys().IndexOf(genericArg);
	if (index != -1) return genericArgs.Values()[index];

	auto itsys = tsys->_genericArg.Alloc(tsys, this, genericArg);
	genericArgs.Add(genericArg, itsys);
	return itsys;
}

TsysPSRecord* ITsys_Decl::GetPSRecord()
{
	if (!psRecord)
	{
		if (data->IsPSPrimary_NF())
		{
			psRecord = MakePtr<TsysPSRecord>();
		}
	}

	return psRecord.Obj();
}

/***********************************************************************
ITsys_Function (Impl)
***********************************************************************/

ITsys* ITsys_Function::ReplaceGenericArgsCallback(ITsys* element, Array<ITsys*>& params)
{
	return element->FunctionOf(params, data);
}

/***********************************************************************
ITsys_Init (Impl)
***********************************************************************/

ITsys* ITsys_Init::ReplaceGenericArgsCallback(ITsys* element, Array<ITsys*>& params)
{
	Array<ExprTsysItem> items(params.Count());
	for (vint i = 0; i < params.Count(); i++)
	{
		items[i] = { data.headers[i],params[i] };
	}
	return tsys->InitOf(items);
}

/***********************************************************************
ITsys_Init (DeclInstant)
***********************************************************************/

ITsys* ITsys_DeclInstant::ReplaceGenericArgsCallback(ITsys* element, Array<ITsys*>& params)
{
	return tsys->DeclInstantOf(data.declSymbol, (params.Count() == 0 ? nullptr : &params), element);
}

TsysPSRecord* ITsys_DeclInstant::GetPSRecord()
{
	if (!psRecord)
	{
		if (data.declSymbol->IsPSPrimary_NF())
		{
			psRecord = MakePtr<TsysPSRecord>();
		}
		else if (data.declSymbol->GetPSPrimary_NF())
		{
			psRecord = MakePtr<TsysPSRecord>();
			psRecord->version = TsysPSRecord::PSInstanceVersion;
		}
	}

	return psRecord.Obj();
}

void ITsys_DeclInstant::MakePSRecordPrimaryThis()
{
	if (!psRecord)
	{
		psRecord = MakePtr<TsysPSRecord>();
		psRecord->version = TsysPSRecord::PSPrimaryThisVersion;
	}
	else if (psRecord && psRecord->version != TsysPSRecord::PSPrimaryThisVersion)
	{
		throw L"GetPSRecord has been called before!";
	}
}