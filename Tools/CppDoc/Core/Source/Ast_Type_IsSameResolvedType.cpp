#include "Ast.h"
#include "Ast_Type.h"
#include "Ast_Decl.h"

/***********************************************************************
IsSameResolvedType
***********************************************************************/

class IsSameResolvedTypeVisitor : public Object, public virtual ITypeVisitor
{
public:
	bool					result = false;
	Ptr<Type>				peerType;

	void TestResolving(Ptr<Resolving> resolving)
	{
		if (auto type = peerType.Cast<ResolvableType>())
		{
			if (type->resolving)
			{
				resolving->Calibrate();
				type->resolving->Calibrate();
				result = CompareEnumerable(resolving->resolvedSymbols, type->resolving->resolvedSymbols) == 0;
			}
		}
	}

	void Visit(PrimitiveType* self)override
	{
		if (auto type = peerType.Cast<PrimitiveType>())
		{
			result = self->prefix == type->prefix && self->primitive == type->primitive;
		}
	}

	void Visit(ReferenceType* self)override
	{
		if (auto type = peerType.Cast<ReferenceType>())
		{
			result = self->reference == type->reference && IsSameResolvedType(self->type, type->type);
		}
	}

	void Visit(ArrayType* self)override
	{
		if (auto type = peerType.Cast<ArrayType>())
		{
			result = IsSameResolvedType(self->type, type->type);
		}
	}

	void Visit(CallingConventionType* self)override
	{
		if (auto type = peerType.Cast<CallingConventionType>())
		{
			result = self->callingConvention == type->callingConvention && IsSameResolvedType(self->type, type->type);
		}
	}

	void Visit(FunctionType* self)override
	{
		if (auto type = peerType.Cast<FunctionType>())
		{
			if (self->qualifierConstExpr != type->qualifierConstExpr) return;
			if (self->qualifierConst != type->qualifierConst) return;
			if (self->qualifierVolatile != type->qualifierVolatile) return;
			if (self->qualifierLRef != type->qualifierLRef) return;
			if (self->qualifierRRef != type->qualifierRRef) return;
			if (!IsSameResolvedType(self->returnType, type->returnType)) return;
			if (!IsSameResolvedType(self->decoratorReturnType, type->decoratorReturnType)) return;
			if (self->parameters.Count() != type->parameters.Count()) return;

			for (vint i = 0; i < self->parameters.Count(); i++)
			{
				if (!IsSameResolvedType(self->parameters[i]->type, type->parameters[i]->type)) return;
			}
			result = true;
		}
	}

	void Visit(MemberType* self)override
	{
		if (auto type = peerType.Cast<MemberType>())
		{
			result = IsSameResolvedType(self->classType, type->classType) && IsSameResolvedType(self->type, type->type);
		}
	}

	void Visit(DeclType* self)override
	{
		if (self->expr)
		{
			throw 0;
		}
		else
		{
			if (auto declType = peerType.Cast<DeclType>())
			{
				result = !declType->expr;
			}
			else
			{
				result = false;
			}
		}
	}

	void Visit(DecorateType* self)override
	{
		if (auto type = peerType.Cast<DecorateType>())
		{
			result = self->isConstExpr == type->isConstExpr
				&& self->isConst == type->isConst
				&& self->isVolatile == type->isVolatile
				&& IsSameResolvedType(self->type, type->type);
		}
	}

	void Visit(RootType* self)override
	{
		result = peerType.Cast<RootType>();
	}

	void Visit(IdType* self)override
	{
		if (self->resolving)
		{
			TestResolving(self->resolving);
		}
	}

	void Visit(ChildType* self)override
	{
		if (self->resolving)
		{
			TestResolving(self->resolving);
		}
	}

	void Visit(GenericType* self)override
	{
		if (auto type = peerType.Cast<GenericType>())
		{
			if (!IsSameResolvedType(self->type, type->type)) return;
			if (self->arguments.Count() != type->arguments.Count()) return;

			for (vint i = 0; i < self->arguments.Count(); i++)
			{
				auto ga1 = self->arguments[i];
				auto ga2 = self->arguments[i];
				if ((ga1.type == nullptr) != (ga2.type == nullptr)) return;

				if (ga1.type && ga2.type)
				{
					if (!IsSameResolvedType(ga1.type, ga2.type)) return;
				}
				else
				{
					throw 0;
				}
			}
			result = true;
		}
	}

	void Visit(VariadicTemplateArgumentType* self)override
	{
		if (auto type = peerType.Cast<VariadicTemplateArgumentType>())
		{
			result = IsSameResolvedType(self->type, type->type);
		}
	}
};

// Test if two types are actually the same one
//   ArrayType length is ignored
//   FunctionType declarators are ignored
//   DeclType is not supported yet
//   Expression arguments in GenericType are not supported yet
bool IsSameResolvedType(Ptr<Type> t1, Ptr<Type> t2)
{
	if (t1 && t2)
	{
		IsSameResolvedTypeVisitor visitor;
		visitor.peerType = t2;
		t1->Accept(&visitor);
		return visitor.result;
	}
	else
	{
		return (t1 == nullptr) == (t2 == nullptr);
	}
}