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
bool IsPendingType(Ptr<Type> type)
{
	if (!type) return false;
	IsPendingTypeVisitor visitor;
	type->Accept(&visitor);
	return visitor.result;
}

/***********************************************************************
ResolvePendingType
***********************************************************************/

class ResolvePendingTypeVisitor : public Object, public virtual ITypeVisitor
{
public:
	ITsys*					result = nullptr;
	ITsys*					targetType = nullptr;
	ExprTsysItem			targetExpr;

	static ITsys* Execute(Ptr<Type> type, ITsys* target)
	{
		ResolvePendingTypeVisitor visitor;
		visitor.targetType = target;
		type->Accept(&visitor);
		return visitor.result;
	}

	static ITsys* Execute(Ptr<Type> type, ExprTsysItem target)
	{
		ResolvePendingTypeVisitor visitor;
		visitor.targetExpr = target;
		type->Accept(&visitor);
		return visitor.result;
	}

	void Visit(PrimitiveType* self)override
	{
		if (self->primitive == CppPrimitiveType::_auto)
		{
			throw 0;
		}
		else
		{
			throw 0;
		}
	}

	void Visit(ReferenceType* self)override
	{
		throw 0;
	}

	void Visit(ArrayType* self)override
	{
		throw NotResolvableException();
	}

	void Visit(CallingConventionType* self)override
	{
		throw 0;
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

		throw 0;
	}

	void Visit(MemberType* self)override
	{
		if (IsPendingType(self->classType))
		{
			throw NotResolvableException();
		}

		throw 0;
	}

	void Visit(DeclType* self)override
	{
		if (self->expr)
		{
			throw 0;
		}
		else
		{
			throw 0;
		}
	}

	void Visit(DecorateType* self)override
	{
		throw 0;
	}

	void Visit(RootType* self)override
	{
		throw NotResolvableException();
	}

	void Visit(IdType* self)override
	{
		throw NotResolvableException();
	}

	void Visit(ChildType* self)override
	{
		throw NotResolvableException();
	}

	void Visit(GenericType* self)override
	{
		throw NotResolvableException();
	}

	void Visit(VariadicTemplateArgumentType* self)override
	{
		throw NotResolvableException();
	}
};

// Resolve a pending type to a target type
ITsys* ResolvePendingType(Ptr<Type> type, ExprTsysItem target)
{
	return ResolvePendingTypeVisitor::Execute(type, target);
}