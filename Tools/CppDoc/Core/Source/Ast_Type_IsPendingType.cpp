#include "Ast.h"
#include "Ast_Type.h"
#include "Ast_Decl.h"

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
	ITsys*					result = nullptr;
	bool					exactMatch = false;
	ITsys*					targetType = nullptr;
	ExprTsysItem			targetExpr;

	static ITsys* Execute(Type* type, ITsys* target, bool _exactMatch)
	{
		ResolvePendingTypeVisitor visitor;
		visitor.exactMatch = _exactMatch;
		visitor.targetType = target;
		type->Accept(&visitor);
		return visitor.result;
	}

	static ITsys* Execute(Type* type, ExprTsysItem target, bool _exactMatch)
	{
		ResolvePendingTypeVisitor visitor;
		visitor.exactMatch = _exactMatch;
		visitor.targetExpr = target;
		type->Accept(&visitor);
		return visitor.result;
	}

	void AssumeEqual(Type* self)
	{
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
			throw 0;
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
			throw 0;
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
		self->type->Accept(this);
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
			throw 0;
		}
		else
		{
			AssumeEqual(self);
		}
	}

	void Visit(MemberType* self)override
	{
		if (IsPendingType(self->classType))
		{
			throw NotResolvableException();
		}

		if (IsPendingType(self->type))
		{
			throw 0;
		}
		else
		{
			AssumeEqual(self);
		}
	}

	void Visit(DeclType* self)override
	{
		if (self->expr)
		{
			AssumeEqual(self);
		}
		else
		{
			throw 0;
		}
	}

	void Visit(DecorateType* self)override
	{
		if (IsPendingType(self->type))
		{
			throw 0;
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
ITsys* ResolvePendingType(Ptr<Type> type, ExprTsysItem target)
{
	return ResolvePendingTypeVisitor::Execute(type.Obj(), target, false);
}