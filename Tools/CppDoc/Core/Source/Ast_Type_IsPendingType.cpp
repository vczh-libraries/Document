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

class ResolvePendingTypeVisitor : public Object, public virtual ITypeVisitor
{
public:
	ParsingArguments&		pa;
	ITsys*					result = nullptr;
	bool					exactMatch = false;
	ITsys*					targetType = nullptr;
	ExprTsysItem			targetExpr;

	static ITsys* Execute(ParsingArguments& pa, Type* type, ITsys* target, bool _exactMatch)
	{
		ResolvePendingTypeVisitor visitor(pa, target, _exactMatch);
		type->Accept(&visitor);
		return visitor.result;
	}

	static ITsys* Execute(ParsingArguments& pa, Type* type, ExprTsysItem target, bool _exactMatch)
	{
		ResolvePendingTypeVisitor visitor(pa, target, _exactMatch);
		type->Accept(&visitor);
		return visitor.result;
	}

	ResolvePendingTypeVisitor(ParsingArguments& _pa, ITsys* target, bool _exactMatch)
		:pa(_pa)
		, targetType(target)
		, exactMatch(_exactMatch)
	{
	}

	ResolvePendingTypeVisitor(ParsingArguments& _pa, ExprTsysItem target, bool _exactMatch)
		:pa(_pa)
		, targetExpr(target)
		, exactMatch(_exactMatch)
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
			result = targetType ? targetType : targetExpr.tsys;
			if (result->GetType() == TsysType::Zero)
			{
				result = pa.tsys->Int();
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
			{
				DeclType type;
				resolved = Execute(pa, &type, targetType ? targetType : targetExpr.tsys, true);
			}

			TsysCV cv;
			TsysRefType ref;
			auto entity = resolved->GetEntity(cv, ref);

			if (exactMatch)
			{
				if (cv.isGeneralConst || cv.isVolatile)
				{
					throw NotResolvableException();
				}
			}

			switch (self->reference)
			{
			case CppReferenceType::Ptr:
				if (entity->GetType() != TsysType::Ptr)
				{
					throw NotResolvableException();
				}
				else
				{
					result = Execute(pa, self->type.Obj(), entity, true);
				}
				break;
			case CppReferenceType::LRef:
				switch (ref)
				{
				case TsysRefType::LRef:
					result = Execute(pa, self->type.Obj(), entity->CVOf(cv), exactMatch)->LRefOf();
					break;
				case TsysRefType::RRef:
					throw NotResolvableException();
				case TsysRefType::None:
					if (exactMatch)
					{
						throw NotResolvableException();
					}
					cv.isGeneralConst = true;
					result = Execute(pa, self->type.Obj(), entity->CVOf(cv), exactMatch)->LRefOf();
					break;
				}
				break;
			case CppReferenceType::RRef:
				switch (ref)
				{
				case TsysRefType::LRef:
					result = Execute(pa, self->type.Obj(), entity->CVOf(cv)->LRefOf(), exactMatch);
					break;
				case TsysRefType::RRef:
					result = Execute(pa, self->type.Obj(), entity->CVOf(cv), exactMatch)->RRefOf();
					break;
				case TsysRefType::None:
					result = Execute(pa, self->type.Obj(), entity->CVOf(cv), exactMatch)->RRefOf();
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
		if (exactMatch)
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

		Execute(pa, self->type.Obj(), entity, true);
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
		if (exactMatch)
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
			Execute(pa, self->parameters[i]->type.Obj(), entity->GetParam(i), true);
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
			Execute(pa, self->returnType.Obj(), entity->GetElement(), true);
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
		if (exactMatch)
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
		Execute(pa, self->classType.Obj(), entity->GetClass(), true);

		if (IsPendingType(self->type))
		{
			Execute(pa, self->type.Obj(), entity->GetElement(), true);
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
			switch (targetExpr.type)
			{
			case ExprTsysType::LValue:
				result = targetExpr.tsys->LRefOf();
				break;
			case ExprTsysType::XValue:
				result = targetExpr.tsys->RRefOf();
				break;
			case ExprTsysType::PRValue:
				result = targetExpr.tsys;
				break;
			}
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
			if (exactMatch)
			{
				if (ref != TsysRefType::None)
				{
					throw NotResolvableException();
				}
			}

			if (exactMatch)
			{
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
			Execute(pa, self->type.Obj(), entity, true);
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
ITsys* ResolvePendingType(ParsingArguments& pa, Ptr<Type> type, ExprTsysItem target)
{
	return ResolvePendingTypeVisitor::Execute(pa, type.Obj(), target, false);
}