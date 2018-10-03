#include "TypeSystem.h"

class TsysAlloc;

/***********************************************************************
TsysBase
***********************************************************************/

class TsysBase : public ITsys
{
protected:
	TsysAlloc*					tsys;
	ITsys*						lrefOf = nullptr;
	ITsys*						rrefOf = nullptr;
	ITsys*						ptrOf = nullptr;
	Dictionary<vint, ITsys*>	arrayOf;
	Dictionary<ITsys*, ITsys*>	memberOf;
	ITsys*						cvOf[8] = { 0 };

public:
	TsysBase(TsysAlloc* _tsys) :tsys(_tsys) {}

	TsysPrimitive		GetPrimitive()			{ throw "Not Implemented!"; }
	TsysCV				GetCV()					{ throw "Not Implemented!"; }
	ITsys*				GetElement()			{ throw "Not Implemented!"; }
	ITsys*				GetParam(vint index)	{ throw "Not Implemented!"; }
	vint				GetParamCount()			{ throw "Not Implemented!"; }
	Ptr<Declaration>	GetDecl()				{ throw "Not Implemented!"; }

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

#define ITSYS_DATA(TYPE, DATA, NAME)												\
class ITsys_##TYPE : public TsysBase_<TsysType::TYPE>								\
{																					\
protected:																			\
	DATA				data;														\
public:																				\
	ITsys_##TYPE(TsysAlloc* _tsys, DATA _data) :TsysBase_(_tsys), data(_data) {}	\
	DATA Get##NAME()override { return data; }										\
};																					\

#define ISYS_REF(TYPE)																			\
class ITsys_##TYPE : public TsysBase_<TsysType::TYPE>											\
{																								\
protected:																						\
	ITsys*				element;																\
public:																							\
	ITsys_##TYPE(TsysAlloc* _tsys, ITsys* _element) :TsysBase_(_tsys), element(_element) {}		\
	ITsys* GetElement()override { return element; }												\
};																								\

ITSYS_DATA(Primitive, TsysPrimitive, Primitive)
ITSYS_DATA(CV, TsysCV, CV)
ITSYS_DATA(Decl, Ptr<Declaration>, Decl)
ITSYS_DATA(GenericArg, Ptr<Declaration>, Decl)

ISYS_REF(LRef)
ISYS_REF(RRef)
ISYS_REF(Ptr)

#undef ITSYS_DATA
#undef ISYS_REF

class ITsys_Array : public TsysBase_<TsysType::Array>
{
protected:
	ITsys*				element;
	vint				dim;

public:
	ITsys_Array(TsysAlloc* _tsys, ITsys* _element, vint _dim)
		:TsysBase_(_tsys)
		, element(_element)
		, dim(_dim)
	{
	}

	ITsys* GetElement()override { return element; }
	vint GetParamCount()override { return dim; }
};

template<TsysType Type>
class ITsys_WithParams : TsysBase_<Type>
{
protected:
	ITsys*				element;
	List<ITsys*>		params;

public:
	ITsys_WithParams(TsysAlloc* _tsys, ITsys* _element)
		:TsysBase_(_tsys)
		, element(_element)
	{
	}

	List<ITsys*>& GetParams()
	{
		return params;
	}

	ITsys* GetElement()override { return element; }
	ITsys* GetParam(vint index)override { return params[index]; }
	vint GetParamCount()override { return params.Count(); }
};

using ITsys_Function = ITsys_WithParams<TsysType::Function>;
using ITsys_Member = ITsys_WithParams<TsysType::Member>;
using ITsys_Generic = ITsys_WithParams<TsysType::Generic>;

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
		T				items[BlockSize];
	};

	List<Ptr<Node>>		nodes;
	vint				lastNodeUsed = 0;
public:

	T* Alloc()
	{
		if (nodes.Count() == 0 || lastNodeUsed == BlockSize)
		{
			nodes.Add(MakePtr<Node>());
			lastNodeUsed = 0;
		}

		auto lastNode = nodes[nodes.Count() - 1].Obj();
		return &lastNode->items[lastNodeUsed++];
	}
};

/***********************************************************************
ITsysAlloc
***********************************************************************/

class TsysAlloc : public Object, public ITsysAlloc
{
protected:
	ITsys*									primitives[36];
	Dictionary<Ptr<Declaration>, ITsys*>	decls;
	Dictionary<Ptr<Declaration>, ITsys*>	genericArgs;

public:
	ITsys_Allocator<ITsys_Primitive, 1024>	_primitive;
	ITsys_Allocator<ITsys_LRef, 1024>		_lref;
	ITsys_Allocator<ITsys_RRef, 1024>		_rref;
	ITsys_Allocator<ITsys_Ptr, 1024>		_ptr;
	ITsys_Allocator<ITsys_Array, 1024>		_array;
	ITsys_Allocator<ITsys_Function, 1024>	_function;
	ITsys_Allocator<ITsys_Member, 1024>		_member;
	ITsys_Allocator<ITsys_CV, 1024>			_cv;
	ITsys_Allocator<ITsys_Decl, 1024>		_decl;
	ITsys_Allocator<ITsys_Generic, 1024>	_generic;
	ITsys_Allocator<ITsys_GenericArg, 1024> _genericArg;
	ITsys_Allocator<ITsys_Expr, 1024>		_expr;

	ITsys* PrimitiveOf(TsysPrimitive primitive)override
	{
		throw 0;
	}

	ITsys* DeclOf(Ptr<Declaration> decl)override
	{
		throw 0;
	}

	ITsys* GenericArgOf(Ptr<Declaration> decl)override
	{
		throw 0;
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
	throw 0;
}

ITsys* TsysBase::RRefOf()
{
	throw 0;
}

ITsys* TsysBase::PtrOf()
{
	throw 0;
}

ITsys* TsysBase::ArrayOf(vint dimensions)
{
	throw 0;
}

ITsys* TsysBase::FunctionOf(IEnumerable<ITsys*>& params)
{
	throw 0;
}

ITsys* TsysBase::MemberOf(ITsys* classType)
{
	throw 0;
}

ITsys* TsysBase::CVOf(TsysCV cv)
{
	throw 0;
}

ITsys* TsysBase::GenericOf(IEnumerable<ITsys*>& params)
{
	throw 0;
}