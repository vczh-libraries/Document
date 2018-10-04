#include "TypeSystem.h"

class TsysAlloc;

class ITsys_Primitive;
class ITsys_LRef;
class ITsys_RRef;
class ITsys_Ptr;
class ITsys_Array;
class ITsys_Member;
class ITsys_CV;
class ITsys_Decl;
class ITsys_GenericArg;
class ITsys_Expr;


template<TsysType Type>
class ITsys_WithParams;

using ITsys_Function = ITsys_WithParams<TsysType::Function>;
using ITsys_Generic = ITsys_WithParams<TsysType::Generic>;

/***********************************************************************
TsysBase
***********************************************************************/

template<typename T>
struct WithParams
{
	T* itsys;

	static vint Compare(WithParams<T> a, WithParams<T> b);
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
protected:
	TsysAlloc*										tsys;
	ITsys_LRef*										lrefOf = nullptr;
	ITsys_RRef*										rrefOf = nullptr;
	ITsys_Ptr*										ptrOf = nullptr;
	Dictionary<vint, ITsys_Array*>					arrayOf;
	Dictionary<ITsys*, ITsys_Member*>				memberOf;
	ITsys_CV*										cvOf[8] = { 0 };
	SortedList<WithParams<ITsys_Function>>			functionOf;
	SortedList<WithParams<ITsys_Generic>>			genericOf;

public:
	TsysBase(TsysAlloc* _tsys) :tsys(_tsys) {}

	TsysPrimitive		GetPrimitive()				{ throw "Not Implemented!"; }
	TsysCV				GetCV()						{ throw "Not Implemented!"; }
	ITsys*				GetElement()				{ throw "Not Implemented!"; }
	ITsys*				GetClass()					{ throw "Not Implemented!"; }
	ITsys*				GetParam(vint index)		{ throw "Not Implemented!"; }
	vint				GetParamCount()				{ throw "Not Implemented!"; }
	Ptr<Declaration>	GetDecl()					{ throw "Not Implemented!"; }

	ITsys* LRefOf()									override;
	ITsys* RRefOf()									override;
	ITsys* PtrOf()									override;
	ITsys* ArrayOf(vint dimensions)					override;
	ITsys* FunctionOf(IEnumerable<ITsys*>& params)	override;
	ITsys* MemberOf(ITsys* classType)				override;
	ITsys* CVOf(TsysCV cv)							override;
	ITsys* GenericOf(IEnumerable<ITsys*>& params)	override;
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

#define ITSYS_DATA(TYPE, DATA, NAME)															\
class ITsys_##TYPE : public TsysBase_<TsysType::TYPE>											\
{																								\
protected:																						\
	DATA				data;																	\
public:																							\
	ITsys_##TYPE(TsysAlloc* _tsys, DATA _data) :TsysBase_(_tsys), data(_data) {}				\
	DATA Get##NAME()override { return data; }													\
};																								\

#define ISYS_REF(TYPE)																			\
class ITsys_##TYPE : public TsysBase_<TsysType::TYPE>											\
{																								\
protected:																						\
	ITsys*				element;																\
public:																							\
	ITsys_##TYPE(TsysAlloc* _tsys, ITsys* _element) :TsysBase_(_tsys), element(_element) {}		\
	ITsys* GetElement()override { return element; }												\
};																								\

#define ITSYS_DECORATE(TYPE, DATA, NAME)														\
class ITsys_##TYPE : public TsysBase_<TsysType::TYPE>											\
{																								\
protected:																						\
	ITsys*				element;																\
	DATA				data;																	\
public:																							\
	ITsys_##TYPE(TsysAlloc* _tsys, ITsys* _element, DATA _data)									\
		:TsysBase_(_tsys), element(_element), data(_data) {}									\
	ITsys* GetElement()override { return element; }												\
	DATA Get##NAME()override { return data; }													\
};																								\

ITSYS_DATA(Primitive, TsysPrimitive, Primitive)
ITSYS_DATA(Decl, Ptr<Declaration>, Decl)
ITSYS_DATA(GenericArg, Ptr<Declaration>, Decl)

ISYS_REF(LRef)
ISYS_REF(RRef)
ISYS_REF(Ptr)

ITSYS_DECORATE(Array, vint, ParamCount)
ITSYS_DECORATE(CV, TsysCV, CV)
ITSYS_DECORATE(Member, ITsys*, Class)

#undef ITSYS_DATA
#undef ISYS_REF
#undef ITSYS_DECORATE

template<TsysType Type>
class ITsys_WithParams : public TsysBase_<Type>
{
protected:
	ITsys*				element;
	List<ITsys*>		params;

public:
	ITsys_WithParams(TsysAlloc* _tsys, ITsys* _element, List<ITsys*>* _params)
		:TsysBase_<Type>(_tsys)
		, element(_element)
		, params(_params)
	{
	}

	List<ITsys*>& GetParams()
	{
		return params;
	}

	ITsys* GetElement()override { return element; }
	ITsys* GetParam(vint index)override { return params.Get(index); }
	vint GetParamCount()override { return params.Count(); }
};

template<typename T>
vint WithParams<T>::Compare(WithParams<T> a, WithParams<T> b)
{
	return CompareEnumerable(a.itsys->GetParams(), b.itsys->GetParams());
}

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
		char				items[BlockSize * sizeof(T)] = { 0 };
		vint				used = 0;

		~Node()
		{
			auto itsys = (T*)items;
			for (vint i = 0; i < used; i++)
			{
				itsys[i].~T();
			}
		}
	};

	List<Ptr<Node>>		nodes;
public:

	template<typename ...TArgs>
	T* Alloc(TArgs ...args)
	{
		if (nodes.Count() == 0 || nodes[nodes.Count() - 1]->used == BlockSize)
		{
			nodes.Add(MakePtr<Node>());
		}

		auto lastNode = nodes[nodes.Count() - 1].Obj();
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
	ITsys_Primitive*								primitives[(vint)TsysPrimitiveType::_COUNT * (vint)TsysBytes::_COUNT] = { 0 };
	Dictionary<Ptr<Declaration>, ITsys_Decl*>		decls;
	Dictionary<Ptr<Declaration>, ITsys_GenericArg*>	genericArgs;

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

	ITsys* PrimitiveOf(TsysPrimitive primitive)override
	{
		vint a = (vint)primitive.type;
		vint b = (vint)primitive.bytes;
		vint index = (vint)TsysPrimitiveType::_COUNT*a + b;
		if (index > sizeof(primitives)) throw "Not Implemented!";

		auto& itsys = primitives[index];
		if (!itsys)
		{
			itsys = _primitive.Alloc(this, primitive);
		}
		return itsys;
	}

	ITsys* DeclOf(Ptr<Declaration> decl)override
	{
		vint index = decls.Keys().IndexOf(decl.Obj());
		if (index != -1) return decls.Values()[index];
		auto itsys = _decl.Alloc(this, decl);
		decls.Add(decl, itsys);
		return itsys;
	}

	ITsys* GenericArgOf(Ptr<Declaration> decl)override
	{
		vint index = genericArgs.Keys().IndexOf(decl.Obj());
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
	throw 0;
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
	auto& itsys = cvOf[index];
	if (!itsys) itsys = tsys->_cv.Alloc(tsys, this, cv);
	return itsys;
}

ITsys* TsysBase::GenericOf(IEnumerable<ITsys*>& params)
{
	throw 0;
}