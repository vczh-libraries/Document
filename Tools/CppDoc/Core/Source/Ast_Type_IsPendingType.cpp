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

	void Visit(PrimitiveType* self)override
	{
	}

	void Visit(ReferenceType* self)override
	{
	}

	void Visit(ArrayType* self)override
	{
	}

	void Visit(CallingConventionType* self)override
	{
	}

	void Visit(FunctionType* self)override
	{
	}

	void Visit(MemberType* self)override
	{
	}

	void Visit(DeclType* self)override
	{
	}

	void Visit(DecorateType* self)override
	{
	}

	void Visit(RootType* self)override
	{
	}

	void Visit(IdType* self)override
	{
	}

	void Visit(ChildType* self)override
	{
	}

	void Visit(GenericType* self)override
	{
	}

	void Visit(VariadicTemplateArgumentType* self)override
	{
	}
};

// Resolve a pending type to a target type
ITsys* ResolvePendingType(Ptr<Type> type, ExprTsysItem target)
{
	throw 0;
}