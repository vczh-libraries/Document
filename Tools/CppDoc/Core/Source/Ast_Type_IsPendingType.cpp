#include "Ast.h"
#include "Ast_Type.h"
#include "Ast_Decl.h"
#include "Parser.h"

/***********************************************************************
IsPendingType
***********************************************************************/

class IsPendingTypeVisitor : public Object, public virtual ITypeVisitor
{
public:
	bool					result = false;

	void Visit(PrimitiveType* self)override
	{
		if (self->primitive == CppPrimitiveType::_auto)
		{
			result = true;
		}
	}

	void Visit(ReferenceType* self)override
	{
		self->type->Accept(this);
	}

	void Visit(ArrayType* self)override
	{
		self->type->Accept(this);
	}

	void Visit(DecorateType* self)override
	{
		self->type->Accept(this);
	}

	void Visit(CallingConventionType* self)override
	{
		self->type->Accept(this);
	}

	void Visit(FunctionType* self)override
	{
		if (self->decoratorReturnType)
		{
			self->decoratorReturnType->Accept(this);
		}
		else
		{
			self->returnType->Accept(this);
		}

		for (vint i = 0; i < self->parameters.Count(); i++)
		{
			self->parameters[i].item->type->Accept(this);
		}
	}

	void Visit(MemberType* self)override
	{
		self->classType->Accept(this);
		self->type->Accept(this);
	}

	void Visit(DeclType* self)override
	{
		if (!self->expr)
		{
			result = true;
		}
	}

	void Visit(RootType* self)override
	{
	}

	void Visit(IdType* self)override
	{
	}

	void Visit(ChildType* self)override
	{
		self->classType->Accept(this);
	}

	void Visit(GenericType* self)override
	{
		self->type->Accept(this);
		for (vint i = 0; i < self->arguments.Count(); i++)
		{
			if (auto type = self->arguments[i].item.type)
			{
				type->Accept(this);
			}
		}
	}
};

// Test if a type contains auto or decltype(auto)
bool IsPendingType(Type* type)
{
	if (!type) return false;
	IsPendingTypeVisitor visitor;
	type->Accept(&visitor);
	return visitor.result;
}

bool IsPendingType(Ptr<Type> type)
{
	return IsPendingType(type.Obj());
}

/***********************************************************************
ResolvePendingType
***********************************************************************/

enum class PendingMatching
{
	Free,						// totally free
	AutoRef,					// match a type for a reference, disallow array to pointer conversion
	AutoCv,						// match a type for const volatile
	AutoCvRef,					// match a type for a const volatile reference
	Exact,						// exactly match
};

class ResolvePendingTypeVisitor : public Object, public virtual ITypeVisitor
{
public:
	const ParsingArguments&		pa;
	ITsys*						result = nullptr;
	PendingMatching				matching = PendingMatching::Exact;
	ITsys*						targetType = nullptr;
	ExprTsysItem				targetExpr;

	static ITsys* Execute(const ParsingArguments& pa, Type* type, ITsys* target, PendingMatching _matching)
	{
		ResolvePendingTypeVisitor visitor(pa, target, _matching);
		type->Accept(&visitor);
		return visitor.result;
	}

	static ITsys* Execute(const ParsingArguments& pa, Type* type, ExprTsysItem target, PendingMatching _matching)
	{
		ResolvePendingTypeVisitor visitor(pa, target, _matching);
		type->Accept(&visitor);
		return visitor.result;
	}

	ResolvePendingTypeVisitor(const ParsingArguments& _pa, ITsys* target, PendingMatching _matching)
		:pa(_pa)
		, targetType(target)
		, matching(_matching)
	{
	}

	ResolvePendingTypeVisitor(const ParsingArguments& _pa, ExprTsysItem target, PendingMatching _matching)
		:pa(_pa)
		, targetExpr(target)
		, matching(_matching)
	{
	}

	void AssumeEqual(Type* self)
	{
		auto target = targetType ? targetType : targetExpr.tsys;

		TypeTsysList types;
		TypeToTsysNoVta(pa, self, types);
		for (vint i = 0; i < types.Count(); i++)
		{
			if (types[i] == target)
			{
				result = target;
				return;
			}
		}
		throw TypeCheckerException();
	}

	void ShouldEqual(Type* self)
	{
		if (IsPendingType(self))
		{
			throw TypeCheckerException();
		}
		AssumeEqual(self);
	}

	void Visit(PrimitiveType* self)override
	{
		if (self->primitive == CppPrimitiveType::_auto)
		{
			TsysCV cv;
			TsysRefType ref;
			auto entity = (targetType ? targetType : targetExpr.tsys)->GetEntity(cv, ref);

			if (entity->GetType() == TsysType::Zero)
			{
				entity = pa.tsys->Int();
			}

			switch (matching)
			{
			case PendingMatching::Free:
			case PendingMatching::AutoCv:
				if (entity->GetType() == TsysType::Array)
				{
					if (entity->GetParamCount() == 1)
					{
						result = entity->GetElement()->PtrOf();
					}
					else
					{
						result = entity->GetElement()->ArrayOf(entity->GetParamCount() - 1)->PtrOf();
					}
				}
				else
				{
					result = entity;
				}
				break;
			case PendingMatching::AutoRef:
			case PendingMatching::AutoCvRef:
				result = entity->CVOf(cv);
				break;
			case PendingMatching::Exact:
				result = CvRefOf(entity, cv, ref);
				break;
			}
		}
		else
		{
			AssumeEqual(self);
		}
	}

	void Visit(ReferenceType* self)override
	{
		if (IsPendingType(self->type))
		{
			ITsys* resolved = nullptr;
			if (targetType)
			{
				resolved = targetType;
			}
			else
			{
				resolved = ApplyExprTsysType(targetExpr.tsys, targetExpr.type);
			}

			TsysCV cv;
			TsysRefType ref;
			auto entity = resolved->GetEntity(cv, ref);
			auto subMatching = matching;
			switch (matching)
			{
			case PendingMatching::Free:
				subMatching = PendingMatching::AutoRef;
				break;
			case PendingMatching::AutoRef:
				subMatching = PendingMatching::AutoRef;
				break;
			case PendingMatching::AutoCv:
				subMatching = PendingMatching::AutoCvRef;
				break;
			case PendingMatching::AutoCvRef:
				subMatching = PendingMatching::AutoCvRef;
				break;
			case PendingMatching::Exact:
				subMatching = PendingMatching::Exact;
				break;
			}

			switch (self->reference)
			{
			case CppReferenceType::Ptr:
				if (entity->GetType() != TsysType::Ptr)
				{
					throw TypeCheckerException();
				}
				else
				{
					result = Execute(pa, self->type.Obj(), entity, subMatching);
				}
				break;
			case CppReferenceType::LRef:
				switch (ref)
				{
				case TsysRefType::LRef:
					result = Execute(pa, self->type.Obj(), entity->CVOf(cv), subMatching)->LRefOf();
					break;
				case TsysRefType::RRef:
					throw TypeCheckerException();
				case TsysRefType::None:
					if (matching != PendingMatching::Free)
					{
						throw TypeCheckerException();
					}
					cv.isGeneralConst = true;
					result = Execute(pa, self->type.Obj(), entity->CVOf(cv), subMatching)->LRefOf();
					break;
				}
				break;
			case CppReferenceType::RRef:
				switch (ref)
				{
				case TsysRefType::LRef:
					result = Execute(pa, self->type.Obj(), entity->CVOf(cv)->LRefOf(), subMatching);
					break;
				case TsysRefType::RRef:
					result = Execute(pa, self->type.Obj(), entity->CVOf(cv), subMatching)->RRefOf();
					break;
				case TsysRefType::None:
					result = Execute(pa, self->type.Obj(), entity->CVOf(cv), subMatching)->RRefOf();
					break;
				}
				break;
			}
		}
		else
		{
			AssumeEqual(self);
		}
	}

	void Visit(ArrayType* self)override
	{
		ShouldEqual(self);
	}

	void Visit(DecorateType* self)override
	{
		if (IsPendingType(self->type))
		{
			TsysCV cv;
			TsysRefType ref;
			auto entity = targetType ? targetType->GetEntity(cv, ref) : targetExpr.tsys->GetEntity(cv, ref);

			if (matching == PendingMatching::Exact)
			{
				if (ref != TsysRefType::None)
				{
					throw TypeCheckerException();
				}

				if (cv.isGeneralConst != self->isConst)
				{
					throw TypeCheckerException();
				}

				if (cv.isVolatile != self->isVolatile)
				{
					throw TypeCheckerException();
				}
			}
			else
			{
				if (cv.isGeneralConst && !self->isConst)
				{
					throw TypeCheckerException();
				}
				if (cv.isVolatile && !self->isVolatile)
				{
					throw TypeCheckerException();
				}
			}

			auto subMatching = matching;
			switch (matching)
			{
			case PendingMatching::Free:
				subMatching = PendingMatching::AutoCv;
				break;
			case PendingMatching::AutoRef:
				subMatching = PendingMatching::AutoCvRef;
				break;
			case PendingMatching::AutoCv:
				subMatching = PendingMatching::AutoCv;
				break;
			case PendingMatching::AutoCvRef:
				subMatching = PendingMatching::AutoCvRef;
				break;
			case PendingMatching::Exact:
				subMatching = PendingMatching::Exact;
				break;
			}
			result = Execute(pa, self->type.Obj(), entity, subMatching);
			cv.isGeneralConst |= self->isConst;
			cv.isVolatile |= self->isVolatile;
			result = result->CVOf(cv);
		}
		else
		{
			AssumeEqual(self);
		}
	}

	void Visit(CallingConventionType* self)override
	{
		TsysCV cv;
		TsysRefType ref;
		auto entity = targetType ? targetType->GetEntity(cv, ref) : targetExpr.tsys->GetEntity(cv, ref);
		if (matching == PendingMatching::Exact)
		{
			if (cv.isGeneralConst || cv.isVolatile)
			{
				throw TypeCheckerException();
			}
			if (ref != TsysRefType::None)
			{
				throw TypeCheckerException();
			}
		}

		if (entity->GetType() != TsysType::Function)
		{
			throw TypeCheckerException();
		}

		if (self->callingConvention == TsysCallingConvention::None)
		{
			if (entity->GetFunc().callingConvention != TsysCallingConvention::CDecl)
			{
				throw TypeCheckerException();
			}
		}
		else
		{
			if (entity->GetFunc().callingConvention != self->callingConvention)
			{
				throw TypeCheckerException();
			}
		}

		Execute(pa, self->type.Obj(), entity, PendingMatching::Exact);
		result = targetType ? targetType : targetExpr.tsys;
	}

	void Visit(FunctionType* self)override
	{
		for (vint i = 0; i < self->parameters.Count(); i++)
		{
			if (IsPendingType(self->parameters[i].item->type))
			{
				throw TypeCheckerException();
			}
		}

		TsysCV cv;
		TsysRefType ref;
		auto entity = targetType ? targetType->GetEntity(cv, ref) : targetExpr.tsys->GetEntity(cv, ref);
		if (matching == PendingMatching::Exact)
		{
			if (cv.isGeneralConst || cv.isVolatile)
			{
				throw TypeCheckerException();
			}
			if (ref != TsysRefType::None)
			{
				throw TypeCheckerException();
			}
		}

		if (entity->GetType() != TsysType::Function)
		{
			throw TypeCheckerException();
		}
		if (self->parameters.Count() != entity->GetParamCount())
		{
			throw TypeCheckerException();
		}
		for (vint i = 0; i < self->parameters.Count(); i++)
		{
			Execute(pa, self->parameters[i].item->type.Obj(), entity->GetParam(i), PendingMatching::Exact);
		}

		if (self->decoratorReturnType)
		{
			if (IsPendingType(self->decoratorReturnType))
			{
				throw TypeCheckerException();
			}
			else
			{
				AssumeEqual(self);
			}
		}
		else if (IsPendingType(self->returnType))
		{
			Execute(pa, self->returnType.Obj(), entity->GetElement(), PendingMatching::Exact);
		}
		else
		{
			AssumeEqual(self);
		}
		result = targetType ? targetType : targetExpr.tsys;
	}

	void Visit(MemberType* self)override
	{
		if (IsPendingType(self->classType))
		{
			throw TypeCheckerException();
		}

		TsysCV cv;
		TsysRefType ref;
		auto entity = targetType ? targetType->GetEntity(cv, ref) : targetExpr.tsys->GetEntity(cv, ref);
		if (matching == PendingMatching::Exact)
		{
			if (cv.isGeneralConst || cv.isVolatile)
			{
				throw TypeCheckerException();
			}
			if (ref != TsysRefType::None)
			{
				throw TypeCheckerException();
			}
		}

		if (entity->GetType() != TsysType::Member)
		{
			throw TypeCheckerException();
		}
		Execute(pa, self->classType.Obj(), entity->GetClass(), PendingMatching::Exact);

		if (IsPendingType(self->type))
		{
			Execute(pa, self->type.Obj(), entity->GetElement(), PendingMatching::Exact);
		}
		else
		{
			AssumeEqual(self);
		}
		result = targetType ? targetType : targetExpr.tsys;
	}

	void Visit(DeclType* self)override
	{
		if (self->expr)
		{
			AssumeEqual(self);
		}
		else if (targetType)
		{
			result = targetType;
		}
		else
		{
			result = targetExpr.tsys;
		}
		if (result->GetType() == TsysType::Zero)
		{
			result = pa.tsys->Int();
		}
	}

	void Visit(RootType* self)override
	{
		ShouldEqual(self);
	}

	void Visit(IdType* self)override
	{
		ShouldEqual(self);
	}

	void Visit(ChildType* self)override
	{
		ShouldEqual(self);
	}

	void Visit(GenericType* self)override
	{
		ShouldEqual(self);
	}
};

// Resolve a pending type to a target type
ITsys* ResolvePendingType(const ParsingArguments& pa, Ptr<Type> type, ExprTsysItem target)
{
	return ResolvePendingTypeVisitor::Execute(pa, type.Obj(), target, PendingMatching::Free);
}