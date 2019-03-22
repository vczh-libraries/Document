#include "TypeSystem.h"

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

	static vint Compare(WithParams<TType, TData> a, WithParams<TType, TData> b)
	{
		vint result = TData::Compare(a.data, b.data);
		if (result != 0) return result;
		return CompareEnumerable(*a.params, *b.params);
	}
};
#define OPERATOR_COMPARE(OP)\
	template<typename TType, typename TData>\
	bool operator OP(WithParams<TType, TData> a, WithParams<TType, TData> b)\
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

namespace vl
{
	template<typename TType, typename TData>
	struct POD<WithParams<TType, TData>>
	{
		static const bool Result = POD<TData>::Result;
	};
}

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

	TsysPrimitive													GetPrimitive()				{ throw "Not Implemented!"; }
	TsysCV															GetCV()						{ throw "Not Implemented!"; }
	ITsys*															GetElement()				{ throw "Not Implemented!"; }
	ITsys*															GetClass()					{ throw "Not Implemented!"; }
	ITsys*															GetParam(vint index)		{ throw "Not Implemented!"; }
	vint															GetParamCount()				{ throw "Not Implemented!"; }
	TsysFunc														GetFunc()					{ throw "Not Implemented!"; }
	const TsysInit&													GetInit()					{ throw "Not Implemented!"; }
	const TsysGenericFunction&										GetGenericFunction()		{ throw "Not Implemented!"; }
	TsysGenericArg													GetGenericArg()				{ throw "Not Implemented!"; }
	Symbol*															GetDecl()					{ throw "Not Implemented!"; }

	ITsys* LRefOf()																						override;
	ITsys* RRefOf()																						override;
	ITsys* PtrOf()																						override;
	ITsys* ArrayOf(vint dimensions)																		override;
	ITsys* FunctionOf(IEnumerable<ITsys*>& params, TsysFunc func)										override;
	ITsys* MemberOf(ITsys* classType)																	override;
	ITsys* CVOf(TsysCV cv)																				override;
	ITsys* GenericFunctionOf(IEnumerable<ITsys*>& params, const TsysGenericFunction& genericFunction)	override;
	ITsys* GenericArgOf(TsysGenericArg genericArg)														override { throw "Not Implemented!"; }

	ITsys* GetEntity(TsysCV& cv, TsysRefType& refType)override
	{
		cv = { false,false };
		refType = TsysRefType::None;
		return GetEntityInternal(cv, refType);
	}

	bool HasGenericArgs()override
	{
		return false;
	}

	void ReplaceGenericArgs(const GenericArgContext& context, List<ITsys*>& output)override
	{
		output.Add(this);
	}
};

template<TsysType Type>
class TsysBase_ : public TsysBase
{
public:
	TsysBase_(TsysAlloc* _tsys) :TsysBase(_tsys) {}

	TsysType GetType()override { return Type; }
};

/***********************************************************************
Concrete Tsys (Commons Members)
***********************************************************************/

#define ITSYS_CLASS(TYPE) ITsys_##TYPE : public TsysBase_<TsysType::TYPE>

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
Concrete Tsys (ReplaceGenericArgs)
***********************************************************************/

#define ITSYS_REPLACE_GENERIC_ARGS(NAME, DATA)\
	public:\
		bool HasGenericArgs()override\
		{\
			return element->HasGenericArgs();\
		}\
		void ReplaceGenericArgs(const GenericArgContext& context, List<ITsys*>& output)override\
		{\
			element->ReplaceGenericArgs(context, output);\
			if (output.Count() == 1 && output[0] == element)\
			{\
				output[0] = this;\
			}\
			else\
			{\
				for (vint i = 0; i < output.Count(); i++)\
				{\
					output[i] = output[i]->NAME(DATA);\
				}\
			}\
		}\

/***********************************************************************
Concrete Tsys (Singleton)
***********************************************************************/

class ITSYS_CLASS(Zero)
{
	ITSYS_MEMBERS_MINIMIZED(Zero)

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
};

/***********************************************************************
Concrete Tsys (User Defined)
***********************************************************************/

class ITSYS_CLASS(Decl)
{
	ITSYS_MEMBERS_DATA(Decl, Symbol*, Decl)

protected:
	Dictionary<TsysGenericArg, ITsys*>								genericArgs;

public:
	ITsys*											GenericArgOf(TsysGenericArg genericArg)override;
};

class ITSYS_CLASS(GenericArg)
{
	ITSYS_MEMBERS_DATA_WITHELEMENT(GenericArg, TsysGenericArg, GenericArg)

public:
	Symbol* GetDecl()override
	{
		return element->GetDecl();
	}

	void ReplaceGenericArgs(const GenericArgContext& context, List<ITsys*>& output)override
	{
		vint index = context.arguments.Keys().IndexOf(this);
		if (index == -1)
		{
			output.Add(this);
		}
		else
		{
			CopyFrom(output, context.arguments.GetByIndex(index));
		}
	}
};

/***********************************************************************
Concrete Tsys (Element Only)
***********************************************************************/

class ITSYS_CLASS(LRef)
{
	ITSYS_MEMBERS_REF(LRef)
	ITSYS_REPLACE_GENERIC_ARGS(LRefOf, )

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
	ITSYS_REPLACE_GENERIC_ARGS(RRefOf, )

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
	ITSYS_REPLACE_GENERIC_ARGS(PtrOf, )
};

class ITSYS_CLASS(Array)
{
	ITSYS_MEMBERS_DATA_WITHELEMENT(Array, vint, ParamCount)
	ITSYS_REPLACE_GENERIC_ARGS(ArrayOf, data)
};

class ITSYS_CLASS(CV)
{
	ITSYS_MEMBERS_DATA_WITHELEMENT(CV, TsysCV, CV)
	ITSYS_REPLACE_GENERIC_ARGS(CVOf, data)

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
	ITSYS_REPLACE_GENERIC_ARGS(MemberOf, data)
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

bool HasGenericArgsWithParams(List<ITsys*>& params, ITsys* element, bool& initializedHasGenericArgs, bool& hasGenericArgs)
{
	if (!initializedHasGenericArgs)
	{
		initializedHasGenericArgs = true;
		if (element && element->HasGenericArgs())
		{
			return (hasGenericArgs = true);
		}
		for (vint i = 0; i < params.Count(); i++)
		{
			if (params[i]->HasGenericArgs())
			{
				return (hasGenericArgs = true);
			}
		}
	}
	return hasGenericArgs;
}

template<typename TITsys>
void ReplaceGenericArgsWithParamsPartial(ITsys* element, Array<vint>& indices, Array<List<ITsys*>>& replacedParams, Array<ITsys*>& selectedParams, vint count, TITsys* self, ITsys* (TITsys::* callback)(ITsys* element, Array<ITsys*>& params))
{
	if (count == replacedParams.Count())
	{
		(self->*callback)(element, selectedParams);
	}
	else
	{
		for (vint i = 0; i < replacedParams[count].Count(); i++)
		{
			indices[count] = i;
			selectedParams[count] = replacedParams[count][i];
			ReplaceGenericArgsWithParamsPartial(element, indices, replacedParams, selectedParams, count + 1, self, callback);
		}
	}
}

template<typename TITsys>
void ReplaceGenericArgsWithParams(const GenericArgContext& context, List<ITsys*>& params, ITsys* element, List<ITsys*>& output, TITsys* self, ITsys* (TITsys::* callback)(ITsys* element, Array<ITsys*>& params))
{
	List<ITsys*> replacedElement;
	Array<vint> indices;
	Array<List<ITsys*>> replacedParams(params.Count());
	Array<ITsys*> selectedParams(params.Count());

	if (element)
	{
		element->ReplaceGenericArgs(context, replacedElement);
	}
	else
	{
		replacedElement.Add(nullptr);
	}

	for (vint i = 0; i < params.Count(); i++)
	{
		params[i]->ReplaceGenericArgs(context, replacedParams[i]);
	}

	for (vint i = 0; i < replacedElement.Count(); i++)
	{
		ReplaceGenericArgsWithParamsPartial(replacedElement[i], indices, replacedParams, selectedParams, 0, self, callback);
	}
}

#define ITSYS_REPLACE_GENERIC_ARGS_WITHPARAMS(ELEMENT) \
public:\
	bool					initializedHasGenericArgs = false;\
	bool					hasGenericArgs = false;\
	bool HasGenericArgs()override\
	{\
		return HasGenericArgsWithParams(params, ELEMENT, initializedHasGenericArgs, hasGenericArgs);\
	}\
	void ReplaceGenericArgs(const GenericArgContext& context, List<ITsys*>& output)override\
	{\
		if (context.arguments.Count() != 0 && HasGenericArgs())\
		{\
			ReplaceGenericArgsWithParams(context, params, ELEMENT, output, this, &RemoveReference<decltype(*this)>::Type::ReplaceGenericArgsCallback);\
		}\
		else\
		{\
			output.Add(this);\
		}\
	}\

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

private:
	ITsys*					ReplaceGenericArgsCallback(ITsys* element, Array<ITsys*>& params);
};

class ITSYS_CLASS(GenericFunction)
{
	ITSYS_MEMBERS_WITHPARAMS_WITH_ELEMENT(GenericFunction, TsysGenericFunction, const TsysGenericFunction&, GenericFunction)
	ITSYS_REPLACE_GENERIC_ARGS_WITHPARAMS(element)

private:
	ITsys*					ReplaceGenericArgsCallback(ITsys* element, Array<ITsys*>& params);
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
	ITsys_Zero												tsysZero;
	ITsys_Nullptr											tsysNullptr;
	ITsys_Primitive*										primitives[(vint)TsysPrimitiveType::_COUNT * (vint)TsysBytes::_COUNT] = { 0 };
	Dictionary<Symbol*, ITsys_Decl*>						decls;
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
	ITsys_Allocator<ITsys_Init,					1024>		_init;
	ITsys_Allocator<ITsys_GenericFunction,		1024>		_genericFunction;
	ITsys_Allocator<ITsys_GenericArg,			1024>		_genericArg;

	TsysAlloc()
		:tsysZero(this)
		, tsysNullptr(this)
	{
	}

	ITsys* Void()override
	{
		return PrimitiveOf({ TsysPrimitiveType::Void,TsysBytes::_1 });
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
		if (index > sizeof(primitives) / sizeof(*primitives)) throw "Not Implemented!";

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

	ITsys* InitOf(Array<ExprTsysItem>& params)override
	{
		List<ITsys*> tsys;
		TsysInit data;

		for (vint i = 0; i < params.Count(); i++)
		{
			tsys.Add(params[i].tsys);
			data.types.Add(params[i].type);
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

	if (index > sizeof(cvOf) / sizeof(*cvOf)) throw "Not Implemented!";
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
		items[i] = { nullptr,data.types[i],params[i] };
	}
	return tsys->InitOf(items);
}

/***********************************************************************
ITsys_GenericFunction (Impl)
***********************************************************************/

ITsys* ITsys_GenericFunction::ReplaceGenericArgsCallback(ITsys* element, Array<ITsys*>& params)
{
	return element->GenericFunctionOf(params, data);
}