#include "Ast.h"
#include "Ast_Type.h"
#include "Ast_Decl.h"

/***********************************************************************
IsSameResolvedType
***********************************************************************/

class IsSameResolvedTypeVisitor : public Object, public virtual ITypeVisitor
{
public:
	bool								result = false;
	Ptr<Type>							peerType;
	Dictionary<WString, WString>&		equivalentNames;

	IsSameResolvedTypeVisitor(Ptr<Type> _peerType, Dictionary<WString, WString>& _equivalentNames)
		:peerType(_peerType)
		, equivalentNames(_equivalentNames)
	{
	}

	void TestResolving(Ptr<Resolving> resolving)
	{
		if (auto type = peerType.Cast<ResolvableType>())
		{
			if (type->resolving)
			{
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
			result = self->reference == type->reference && IsSameResolvedType(self->type, type->type, equivalentNames);
		}
	}

	void Visit(ArrayType* self)override
	{
		if (auto type = peerType.Cast<ArrayType>())
		{
			result = IsSameResolvedType(self->type, type->type, equivalentNames);
		}
	}

	void Visit(CallingConventionType* self)override
	{
		if (auto type = peerType.Cast<CallingConventionType>())
		{
			result =
				self->callingConvention == type->callingConvention &&
				IsSameResolvedType(self->type, type->type, equivalentNames);
		}
	}

	static bool IsFunctionTypeConfigCompatible(FunctionType* funcNew, FunctionType* funcOld)
	{
		if (funcNew->qualifierConstExpr != funcOld->qualifierConstExpr) return false;
		if (funcNew->qualifierConst != funcOld->qualifierConst) return false;
		if (funcNew->qualifierVolatile != funcOld->qualifierVolatile) return false;
		if (funcNew->qualifierLRef != funcOld->qualifierLRef) return false;
		if (funcNew->qualifierRRef != funcOld->qualifierRRef) return false;
		if (funcNew->parameters.Count() != funcOld->parameters.Count()) return false;

		for (vint i = 0; i < funcNew->parameters.Count(); i++)
		{
			if (funcNew->parameters[i].isVariadic != funcOld->parameters[i].isVariadic) return false;
		}
		return true;
	}

	void Visit(FunctionType* self)override
	{
		if (auto type = peerType.Cast<FunctionType>())
		{
			if (IsFunctionTypeConfigCompatible(self, type.Obj()))
			{
				if (!IsSameResolvedType(self->returnType, type->returnType, equivalentNames)) return;
				if (!IsSameResolvedType(self->decoratorReturnType, type->decoratorReturnType, equivalentNames)) return;
				for (vint i = 0; i < self->parameters.Count(); i++)
				{
					auto selfParam = self->parameters[i].item->type;
					auto targetParam = type->parameters[i].item->type;
					if (!IsSameResolvedType(selfParam, targetParam, equivalentNames)) return;
				}
				result = true;
			}
		}
	}

	void Visit(MemberType* self)override
	{
		if (auto type = peerType.Cast<MemberType>())
		{
			result =
				IsSameResolvedType(self->classType, type->classType, equivalentNames) &&
				IsSameResolvedType(self->type, type->type, equivalentNames);
		}
	}

	void Visit(DeclType* self)override
	{
		if (auto declType = peerType.Cast<DeclType>())
		{
			if (self->expr && declType->expr)
			{
				throw 0;
			}
			else if (!self->expr && !declType->expr)
			{
				result = true;
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
				&& IsSameResolvedType(self->type, type->type, equivalentNames);
		}
	}

	void Visit(RootType* self)override
	{
		result = peerType.Cast<RootType>();
	}

	void Visit(IdType* self)override
	{
		if (auto idType = peerType.Cast<IdType>())
		{
			vint index = equivalentNames.Keys().IndexOf(self->name.name);
			if (index != -1 && idType->name.name == equivalentNames.Values()[index])
			{
				result = true;
				return;
			}
		}

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
			if (!IsSameResolvedType(self->type, type->type, equivalentNames)) return;
			if (self->arguments.Count() != type->arguments.Count()) return;

			for (vint i = 0; i < self->arguments.Count(); i++)
			{
				if (self->arguments[i].isVariadic != type->arguments[i].isVariadic) return;
				auto ga1 = self->arguments[i].item;
				auto ga2 = type->arguments[i].item;
				if ((ga1.type == nullptr) != (ga2.type == nullptr)) return;

				if (ga1.type && ga2.type)
				{
					if (!IsSameResolvedType(ga1.type, ga2.type, equivalentNames)) return;
				}
				else if (ga1.expr && ga2.expr)
				{
					// assume all constant expression in type arguments to be identical
					continue;
				}
				else
				{
					return;
				}
			}
			result = true;
		}
	}
};

// Test if two types are actually the same one
//   ArrayType length is ignored
//   FunctionType declarators are ignored
//   DeclType is not supported yet (in the future, only look at the expression's shape as C++ISO requires)
bool IsSameResolvedType(Ptr<Type> t1, Ptr<Type> t2, Dictionary<WString, WString>& equivalentNames)
{
	if (t1 && t2)
	{
		IsSameResolvedTypeVisitor visitor(t2, equivalentNames);
		t1->Accept(&visitor);
		return visitor.result;
	}
	else
	{
		return (t1 == nullptr) == (t2 == nullptr);
	}
}

/***********************************************************************
IsCompatibleFunctionDecl
***********************************************************************/

bool IsCompatibleTemplateSpec(Ptr<TemplateSpec> specNew, Ptr<TemplateSpec> specOld, Dictionary<WString, WString>& equivalentNames)
{
	if (specNew && specOld)
	{
		if (specNew->arguments.Count() != specOld->arguments.Count()) return false;
		for (vint i = 0; i < specNew->arguments.Count(); i++)
		{
			const auto& argNew = specNew->arguments[i];
			const auto& argOld = specOld->arguments[i];
			if (argNew.argumentType != argOld.argumentType) return false;
			if (argNew.ellipsis != argOld.ellipsis) return false;

			if (argNew.name && argOld.name)
			{
				equivalentNames.Add(argNew.name.name, argOld.name.name);
			}

			switch (argNew.argumentType)
			{
			case CppTemplateArgumentType::HighLevelType:
				if (!IsCompatibleTemplateSpec(argNew.templateSpec, argOld.templateSpec, equivalentNames))
				{
					return false;
				}
				break;
			case CppTemplateArgumentType::Type:
				break;
			case CppTemplateArgumentType::Value:
				if (!IsSameResolvedType(argNew.type, argOld.type, equivalentNames))
				{
					return false;
				}
				break;
			}
		}
		return true;
	}
	return !specNew && !specOld;
}

bool IsCompatibleFunctionDeclInSameScope(Ptr<ForwardFunctionDeclaration> declNew, Ptr<ForwardFunctionDeclaration> declOld)
{
	if (declNew->name.name != declOld->name.name) return false;
	if (declNew->needResolveTypeFromStatement || declOld->needResolveTypeFromStatement) return false;
	if (declNew->methodType != declOld->methodType) return false;

	Dictionary<WString, WString> equivalentNames;
	if (!IsCompatibleTemplateSpec(declNew->templateSpec, declOld->templateSpec, equivalentNames))
	{
		return false;
	}

	auto typeNew = declNew->type;
	auto typeOld = declOld->type;

	while (true)
	{
		if (auto memberType = typeNew.Cast<MemberType>())
		{
			typeNew = memberType->type;
		}
		else if (auto ccType = typeNew.Cast<CallingConventionType>())
		{
			typeNew = ccType->type;
		}
		else
		{
			break;
		}
	}

	while (true)
	{
		if (auto memberType = typeOld.Cast<MemberType>())
		{
			typeOld = memberType->type;
		}
		else if (auto ccType = typeOld.Cast<CallingConventionType>())
		{
			typeOld = ccType->type;
		}
		else
		{
			break;
		}
	}

	auto funcNew = typeNew.Cast<FunctionType>();
	auto funcOld = typeOld.Cast<FunctionType>();
	if (!funcNew || !funcOld)
	{
		return false;
	}

	if (!IsSameResolvedTypeVisitor::IsFunctionTypeConfigCompatible(funcNew.Obj(), funcOld.Obj()))
	{
		return false;
	}

	if (!IsSameResolvedType(funcNew->returnType, funcOld->returnType, equivalentNames))
	{
		return false;
	}
	for (vint i = 0; i < funcNew->parameters.Count(); i++)
	{
		auto newParam = funcNew->parameters[i].item;
		auto oldParam = funcOld->parameters[i].item;
		if (newParam->name && oldParam->name)
		{
			equivalentNames.Add(newParam->name.name, oldParam->name.name);
		}
		if (!IsSameResolvedType(newParam->type, oldParam->type, equivalentNames))
		{
			return false;
		}
	}
	if (!IsSameResolvedType(funcNew->decoratorReturnType, funcOld->decoratorReturnType, equivalentNames))
	{
		return false;
	}
	return true;
}