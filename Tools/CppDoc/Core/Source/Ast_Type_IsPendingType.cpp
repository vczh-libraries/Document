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
			self->parameters[i]->type->Accept(this);
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

	void Visit(DecorateType* self)override
	{
		self->type->Accept(this);
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
			if (auto type = self->arguments[i].type)
			{
				type->Accept(this);
			}
		}
	}

	void Visit(VariadicTemplateArgumentType* self)override
	{
		self->type->Accept(this);
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
	ExactExceptDecorator,		// enable const->non const etc
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
		TypeToTsys(pa, self, types);
		for (vint i = 0; i < types.Count(); i++)
		{
			if (types[i] == target)
			{
				result = target;
				return;
			}
		}
		throw NotResolvableException();
	}

	void ShouldEqual(Type* self)
	{
		if (IsPendingType(self))
		{
			throw NotResolvableException();
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
				result = entity;
				break;
			case PendingMatching::ExactExceptDecorator:
				result = entity->CVOf(cv);
				break;
			case PendingMatching::Exact:
				switch (ref)
				{
				case TsysRefType::LRef:
					result = entity->CVOf(cv)->LRefOf();
					break;
				case TsysRefType::RRef:
					result = entity->CVOf(cv)->RRefOf();
					break;
				case TsysRefType::None:
					result = entity->CVOf(cv);
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
				switch (targetExpr.type)
				{
				case ExprTsysType::LValue:
					resolved = targetExpr.tsys->LRefOf();
					break;
				case ExprTsysType::XValue:
					resolved = targetExpr.tsys->RRefOf();
					break;
				case ExprTsysType::PRValue:
					resolved = targetExpr.tsys;
					break;
				}
			}

			TsysCV cv;
			TsysRefType ref;
			auto entity = resolved->GetEntity(cv, ref);
			auto subMatching = matching == PendingMatching::Free ? PendingMatching::ExactExceptDecorator : matching;

			switch (self->reference)
			{
			case CppReferenceType::Ptr:
				if (entity->GetType() != TsysType::Ptr)
				{
					throw NotResolvableException();
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
					throw NotResolvableException();
				case TsysRefType::None:
					if (matching != PendingMatching::Free)
					{
						throw NotResolvableException();
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

	void Visit(CallingConventionType* self)override
	{
		TsysCV cv;
		TsysRefType ref;
		auto entity = targetType ? targetType->GetEntity(cv, ref) : targetExpr.tsys->GetEntity(cv, ref);
		if (matching == PendingMatching::Exact)
		{
			if (cv.isGeneralConst || cv.isVolatile)
			{
				throw NotResolvableException();
			}
			if (ref != TsysRefType::None)
			{
				throw NotResolvableException();
			}
		}

		if (entity->GetType() != TsysType::Function)
		{
			throw NotResolvableException();
		}

		if (self->callingConvention == TsysCallingConvention::None)
		{
			if (entity->GetFunc().callingConvention != TsysCallingConvention::CDecl)
			{
				throw NotResolvableException();
			}
		}
		else
		{
			if (entity->GetFunc().callingConvention != self->callingConvention)
			{
				throw NotResolvableException();
			}
		}

		Execute(pa, self->type.Obj(), entity, PendingMatching::Exact);
		result = targetType ? targetType : targetExpr.tsys;
	}

	void Visit(FunctionType* self)override
	{
		for (vint i = 0; i < self->parameters.Count(); i++)
		{
			if (IsPendingType(self->parameters[i]->type))
			{
				throw NotResolvableException();
			}
		}

		TsysCV cv;
		TsysRefType ref;
		auto entity = targetType ? targetType->GetEntity(cv, ref) : targetExpr.tsys->GetEntity(cv, ref);
		if (matching == PendingMatching::Exact)
		{
			if (cv.isGeneralConst || cv.isVolatile)
			{
				throw NotResolvableException();
			}
			if (ref != TsysRefType::None)
			{
				throw NotResolvableException();
			}
		}

		if (entity->GetType() != TsysType::Function)
		{
			throw NotResolvableException();
		}
		if (self->parameters.Count() != entity->GetParamCount())
		{
			throw NotResolvableException();
		}
		for (vint i = 0; i < self->parameters.Count(); i++)
		{
			Execute(pa, self->parameters[i]->type.Obj(), entity->GetParam(i), PendingMatching::Exact);
		}

		if (self->decoratorReturnType)
		{
			if (IsPendingType(self->decoratorReturnType))
			{
				throw NotResolvableException();
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
			throw NotResolvableException();
		}

		TsysCV cv;
		TsysRefType ref;
		auto entity = targetType ? targetType->GetEntity(cv, ref) : targetExpr.tsys->GetEntity(cv, ref);
		if (matching == PendingMatching::Exact)
		{
			if (cv.isGeneralConst || cv.isVolatile)
			{
				throw NotResolvableException();
			}
			if (ref != TsysRefType::None)
			{
				throw NotResolvableException();
			}
		}

		if (entity->GetType() != TsysType::Member)
		{
			throw NotResolvableException();
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
					throw NotResolvableException();
				}

				if (cv.isGeneralConst != (self->isConst || self->isConstExpr))
				{
					throw NotResolvableException();
				}

				if (cv.isVolatile != self->isVolatile)
				{
					throw NotResolvableException();
				}
			}
			else
			{
				if (cv.isGeneralConst && !(self->isConst || self->isConstExpr))
				{
					throw NotResolvableException();
				}
				if (cv.isVolatile && !self->isVolatile)
				{
					throw NotResolvableException();
				}
			}
			Execute(pa, self->type.Obj(), entity, (matching == PendingMatching::Free ? PendingMatching::ExactExceptDecorator : matching));
			cv.isGeneralConst |= (self->isConst || self->isConstExpr);
			cv.isVolatile |= self->isVolatile;
			result = entity->CVOf(cv);
		}
		else
		{
			AssumeEqual(self);
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

	void Visit(VariadicTemplateArgumentType* self)override
	{
		ShouldEqual(self);
	}
};

// Resolve a pending type to a target type
ITsys* ResolvePendingType(const ParsingArguments& pa, Ptr<Type> type, ExprTsysItem target)
{
	return ResolvePendingTypeVisitor::Execute(pa, type.Obj(), target, PendingMatching::Free);
}