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

template<typename T>
struct WithParams
{
	IEnumerable<ITsys*>*	params;
	T*						itsys;

	static vint Compare(WithParams<T> a, WithParams<T> b)
	{
		return CompareEnumerable(*a.params, *b.params);
	}
};
#define OPERATOR_COMPARE(OP)\
	template<typename T>\
	bool operator OP(WithParams<T> a, WithParams<T> b)\
	{\
		return WithParams<T>::Compare(a, b) OP 0;\
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
	template<typename T>
	struct POD<WithParams<T>>
	{
		static const bool Result = true;
	};
}

class TsysBase : public ITsys
{
	template<typename T>
	using WithParamsList = SortedList<WithParams<T>>;

#define DEFINE_TSYS_TYPE(NAME) friend class ITsys_##NAME;
	TSYS_TYPE_LIST(DEFINE_TSYS_TYPE)
#undef DEFINE_TSYS_TYPE
protected:
	TsysAlloc*										tsys;
	ITsys_LRef*										lrefOf = nullptr;
	ITsys_RRef*										rrefOf = nullptr;
	ITsys_Ptr*										ptrOf = nullptr;
	Dictionary<vint, ITsys_Array*>					arrayOf;
	Dictionary<ITsys*, ITsys_Member*>				memberOf;
	ITsys_CV*										cvOf[7] = { 0 };
	WithParamsList<ITsys_Function>					functionOf;
	WithParamsList<ITsys_Generic>					genericOf;

	template<typename T, vint BlockSize>
	ITsys* ParamsOf(IEnumerable<ITsys*>& params, WithParamsList<T>& paramsOf, ITsys_Allocator<T, BlockSize> TsysAlloc::* alloc);

	virtual ITsys* GetEntityInternal(TsysCV& cv, TsysRefType& refType)
	{
		return this;
	}
public:
	TsysBase(TsysAlloc* _tsys) :tsys(_tsys) {}

	TsysPrimitive		GetPrimitive()				{ throw "Not Implemented!"; }
	TsysCV				GetCV()						{ throw "Not Implemented!"; }
	ITsys*				GetElement()				{ throw "Not Implemented!"; }
	ITsys*				GetClass()					{ throw "Not Implemented!"; }
	ITsys*				GetParam(vint index)		{ throw "Not Implemented!"; }
	vint				GetParamCount()				{ throw "Not Implemented!"; }
	Symbol*				GetDecl()					{ throw "Not Implemented!"; }

	ITsys* LRefOf()									override;
	ITsys* RRefOf()									override;
	ITsys* PtrOf()									override;
	ITsys* ArrayOf(vint dimensions)					override;
	ITsys* FunctionOf(IEnumerable<ITsys*>& params)	override;
	ITsys* MemberOf(ITsys* classType)				override;
	ITsys* CVOf(TsysCV cv)							override;
	ITsys* GenericOf(IEnumerable<ITsys*>& params)	override;

	ITsys* GetEntity(TsysCV& cv, TsysRefType& refType)override
	{
		cv = { false,false,false };
		refType = TsysRefType::None;
		return GetEntityInternal(cv, refType);
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
Concrete Tsys
***********************************************************************/

#define ITSYS_CLASS(TYPE) ITsys_##TYPE : public TsysBase_<TsysType::TYPE>

#define ITSYS_MEMBERS_MINIMIZED(TYPE)																\
	public:																							\
		ITsys_##TYPE(TsysAlloc* _tsys) :TsysBase_(_tsys) {}											\

#define ITSYS_MEMBERS_DATA(TYPE, DATA, NAME)														\
	protected:																						\
		DATA				data;																	\
	public:																							\
		ITsys_##TYPE(TsysAlloc* _tsys, DATA _data) :TsysBase_(_tsys), data(_data) {}				\
		DATA Get##NAME()override { return data; }													\

#define ITSYS_MEMBERS_REF(TYPE)																		\
	protected:																						\
		TsysBase*			element;																\
	public:																							\
		ITsys_##TYPE(TsysAlloc* _tsys, TsysBase* _element) :TsysBase_(_tsys), element(_element) {}	\
		ITsys* GetElement()override { return element; }												\

#define ITSYS_MEMBERS_DECORATE(TYPE, DATA, NAME)													\
	protected:																						\
		TsysBase*				element;															\
		DATA				data;																	\
	public:																							\
		ITsys_##TYPE(TsysAlloc* _tsys, TsysBase* _element, DATA _data)								\
			:TsysBase_(_tsys), element(_element), data(_data) {}									\
		ITsys* GetElement()override { return element; }												\
		DATA Get##NAME()override { return data; }													\

#define ITSYS_MEMBERS_WITHPARAMS(TYPE)																\
	protected:																						\
		TsysBase*				element;															\
		List<ITsys*>		params;																	\
	public:																							\
		ITsys_##TYPE(TsysAlloc* _tsys, TsysBase* _element)											\
			:TsysBase_(_tsys), element(_element) {}													\
		List<ITsys*>& GetParams() { return params; }												\
		ITsys* GetElement()override { return element; }												\
		ITsys* GetParam(vint index)override { return params.Get(index); }							\
		vint GetParamCount()override { return params.Count(); }										\

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

class ITSYS_CLASS(Decl)
{
	ITSYS_MEMBERS_DATA(Decl, Symbol*, Decl)
};

class ITSYS_CLASS(GenericArg)
{
	ITSYS_MEMBERS_DATA(GenericArg, Symbol*, Decl)
};

class ITSYS_CLASS(LRef)
{
	ITSYS_MEMBERS_REF(LRef)

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
};

class ITSYS_CLASS(Array)
{
	ITSYS_MEMBERS_DECORATE(Array, vint, ParamCount)
};

class ITSYS_CLASS(CV)
{
	ITSYS_MEMBERS_DECORATE(CV, TsysCV, CV)

	ITsys* CVOf(TsysCV cv)override
	{
		cv.isConstExpr |= data.isConstExpr;
		cv.isConst |= data.isConst;
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
	ITSYS_MEMBERS_DECORATE(Member, ITsys*, Class)
};

class ITSYS_CLASS(Function)
{
	ITSYS_MEMBERS_WITHPARAMS(Function)
};

class ITSYS_CLASS(Generic)
{
	ITSYS_MEMBERS_WITHPARAMS(Generic)
};


#undef ITSYS_MEMBERS_DATA
#undef ITSYS_MEMBERS_REF
#undef ITSYS_MEMBERS_DECORATE
#undef ITSYS_MEMBERS_WITHPARAMS
#undef ITSYS_CLASS

class ITsys_Expr : TsysBase_<TsysType::Expr>
{
public:
	ITsys_Expr(TsysAlloc* _tsys)
		:TsysBase_(_tsys)
	{
	}
};

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
	T* Alloc(TArgs ...args)
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
		return new(itsys)T(args...);
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
	ITsys_Zero										tsysZero;
	ITsys_Nullptr									tsysNullptr;
	ITsys_Primitive*								primitives[(vint)TsysPrimitiveType::_COUNT * (vint)TsysBytes::_COUNT] = { 0 };
	Dictionary<Symbol*, ITsys_Decl*>				decls;
	Dictionary<Symbol*, ITsys_GenericArg*>			genericArgs;

public:
	ITsys_Allocator<ITsys_Primitive,	1024>		_primitive;
	ITsys_Allocator<ITsys_LRef,			1024>		_lref;
	ITsys_Allocator<ITsys_RRef,			1024>		_rref;
	ITsys_Allocator<ITsys_Ptr,			1024>		_ptr;
	ITsys_Allocator<ITsys_Array,		1024>		_array;
	ITsys_Allocator<ITsys_Function,		1024>		_function;
	ITsys_Allocator<ITsys_Member,		1024>		_member;
	ITsys_Allocator<ITsys_CV,			1024>		_cv;
	ITsys_Allocator<ITsys_Decl,			1024>		_decl;
	ITsys_Allocator<ITsys_Generic,		1024>		_generic;
	ITsys_Allocator<ITsys_GenericArg,	1024>		_genericArg;
	ITsys_Allocator<ITsys_Expr,			1024>		_expr;

	TsysAlloc()
		:tsysZero(this)
		, tsysNullptr(this)
	{
	}

	ITsys* Zero()override
	{
		return &tsysZero;
	}

	ITsys* Nullptr()override
	{
		return &tsysNullptr;
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

	ITsys* GenericArgOf(Symbol* decl)override
	{
		vint index = genericArgs.Keys().IndexOf(decl);
		if (index != -1) return genericArgs.Values()[index];
		auto itsys = _genericArg.Alloc(this, decl);
		genericArgs.Add(decl, itsys);
		return itsys;
	}
};

Ptr<ITsysAlloc> ITsysAlloc::Create()
{
	return new TsysAlloc;
}

/***********************************************************************
TsysBase (Impl)
***********************************************************************/

template<typename T, vint BlockSize>
ITsys* TsysBase::ParamsOf(IEnumerable<ITsys*>& params, WithParamsList<T>& paramsOf, ITsys_Allocator<T, BlockSize> TsysAlloc::* alloc)
{
	WithParams<T> key;
	key.params = &params;
	key.itsys = nullptr;
	vint index = paramsOf.IndexOf(key);
	if (index != -1) return paramsOf[index].itsys;

	T* itsys = (tsys->*alloc).Alloc(tsys, this);
	CopyFrom(itsys->GetParams(), params);
	key.params = &itsys->GetParams();
	key.itsys = itsys;
	paramsOf.Add(key);
	return itsys;
}

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

ITsys* TsysBase::FunctionOf(IEnumerable<ITsys*>& params)
{
	return ParamsOf(params, functionOf, &TsysAlloc::_function);
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
	vint index = ((cv.isConstExpr ? 1 : 0) << 2) + ((cv.isConst ? 1 : 0) << 1) + (cv.isVolatile ? 1 : 0);

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

ITsys* TsysBase::GenericOf(IEnumerable<ITsys*>& params)
{
	return ParamsOf(params, genericOf, &TsysAlloc::_generic);
}