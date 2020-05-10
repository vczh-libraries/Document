#include "Ast.h"
#include "Ast_Type.h"
#include "Ast_Decl.h"
#include "Ast_Expr.h"

/***********************************************************************
IsSameResolvedExpr
***********************************************************************/

class IsSameResolvedExprVisitor : public Object, public virtual IExprVisitor
{
public:
	bool								result = false;
	Ptr<Expr>							peerExpr;
	Dictionary<WString, WString>&		equivalentNames;

	IsSameResolvedExprVisitor(Ptr<Expr> _peerExpr, Dictionary<WString, WString>& _equivalentNames)
		:peerExpr(_peerExpr)
		, equivalentNames(_equivalentNames)
	{
	}

	void Visit(PlaceholderExpr* self)override
	{
	}

	void Visit(LiteralExpr* self)override
	{
		if (auto expr = peerExpr.Cast<LiteralExpr>())
		{
			if (self->tokens.Count() != expr->tokens.Count()) return;
			for (vint i = 0; i < self->tokens.Count(); i++)
			{
				auto& t1 = self->tokens[i];
				auto& t2 = expr->tokens[i];
				if (t1.length != t2.length) return;
				if (wcsncmp(t1.reading, t2.reading, t1.length) != 0) return;
			}
			result = true;
		}
	}

	void Visit(ThisExpr* self)override
	{
		if (auto expr = peerExpr.Cast<ThisExpr>())
		{
			result = true;
		}
	}

	void Visit(NullptrExpr* self)override
	{
		if (auto expr = peerExpr.Cast<NullptrExpr>())
		{
			result = true;
		}
	}

	void Visit(ParenthesisExpr* self)override
	{
		if (auto expr = peerExpr.Cast<ParenthesisExpr>())
		{
			result = IsSameResolvedExpr(self->expr, expr->expr, equivalentNames);
		}
	}

	void Visit(CastExpr* self)override
	{
		if (auto expr = peerExpr.Cast<CastExpr>())
		{
			result =
				self->castType == expr->castType &&
				IsSameResolvedType(self->type, expr->type, equivalentNames) &&
				IsSameResolvedExpr(self->expr, expr->expr, equivalentNames);
		}
	}

	void Visit(TypeidExpr* self)override
	{
		if (auto expr = peerExpr.Cast<TypeidExpr>())
		{
			result =
				IsSameResolvedType(self->type, expr->type, equivalentNames) &&
				IsSameResolvedExpr(self->expr, expr->expr, equivalentNames);
		}
	}

	void Visit(SizeofExpr* self)override
	{
		if (auto expr = peerExpr.Cast<SizeofExpr>())
		{
			result =
				self->ellipsis == expr->ellipsis &&
				IsSameResolvedType(self->type, expr->type, equivalentNames) &&
				IsSameResolvedExpr(self->expr, expr->expr, equivalentNames);
		}
	}

	void Visit(ThrowExpr* self)override
	{
		if (auto expr = peerExpr.Cast<ThrowExpr>())
		{
			result = IsSameResolvedExpr(self->expr, expr->expr, equivalentNames);
		}
	}

	void Visit(DeleteExpr* self)override
	{
		if (auto expr = peerExpr.Cast<DeleteExpr>())
		{
			result =
				self->arrayDelete == expr->arrayDelete &&
				IsSameResolvedExpr(self->expr, expr->expr, equivalentNames);
		}
	}

	void Visit(IdExpr* self)override
	{
		if (auto idExpr = peerExpr.Cast<IdExpr>())
		{
			vint index = equivalentNames.Keys().IndexOf(self->name.name);
			if (index != -1 && idExpr->name.name == equivalentNames.Values()[index])
			{
				result = true;
			}
			else
			{
				if (self->name.name != idExpr->name.name) return;
				result = Resolving::ContainsSameSymbol(self->resolving, idExpr->resolving);
			}
		}
		else if (auto childExpr = peerExpr.Cast<ChildExpr>())
		{
			if (self->name.name != childExpr->name.name) return;
			result = Resolving::ContainsSameSymbol(self->resolving, childExpr->resolving);
		}
	}

	void Visit(ChildExpr* self)override
	{
		if (auto idExpr = peerExpr.Cast<IdExpr>())
		{
			if (self->name.name != idExpr->name.name) return;
			result = Resolving::ContainsSameSymbol(self->resolving, idExpr->resolving);
		}
		else if (auto childExpr = peerExpr.Cast<ChildExpr>())
		{
			if (self->name.name != childExpr->name.name) return;
			result = IsSameResolvedType(self->classType, childExpr->classType, equivalentNames);
		}
	}

	void Visit(FieldAccessExpr* self)override
	{
		if (auto expr = peerExpr.Cast<FieldAccessExpr>())
		{
			result =
				self->type == expr->type &&
				IsSameResolvedExpr(self->expr, expr->expr, equivalentNames) &&
				IsSameResolvedExpr(self->name, expr->name, equivalentNames);
		}
	}

	void Visit(ArrayAccessExpr* self)override
	{
		if (auto expr = peerExpr.Cast<ArrayAccessExpr>())
		{
			result =
				IsSameResolvedExpr(self->expr, expr->expr, equivalentNames) &&
				IsSameResolvedExpr(self->index, expr->index, equivalentNames);
		}
	}

	bool TestArguments(VariadicList<Ptr<Expr>>& es1, VariadicList<Ptr<Expr>>& es2)
	{
		if (es1.Count() != es2.Count()) return false;
		for (vint i = 0; i < es1.Count(); i++)
		{
			auto& a1 = es1[i];
			auto& a2 = es2[i];
			if (a1.isVariadic != a2.isVariadic) return false;
			if (!IsSameResolvedExpr(a1.item, a2.item, equivalentNames)) return false;
		}
		return true;
	}

	void Visit(FuncAccessExpr* self)override
	{
		if (auto expr = peerExpr.Cast<FuncAccessExpr>())
		{
			if (!IsSameResolvedExpr(self->expr, expr->expr, equivalentNames)) return;
			if (!TestArguments(self->arguments, expr->arguments)) return;
			result = true;
		}
	}

	void Visit(CtorAccessExpr* self)override
	{
		if (auto expr = peerExpr.Cast<CtorAccessExpr>())
		{
			if (!IsSameResolvedType(self->type, expr->type, equivalentNames)) return;
			if (self->initializer->initializerType != expr->initializer->initializerType) return;
			if (!TestArguments(self->initializer->arguments, expr->initializer->arguments)) return;
			result = true;
		}
	}

	void Visit(NewExpr* self)override
	{
		if (auto expr = peerExpr.Cast<NewExpr>())
		{
			Visit((CtorAccessExpr*)self);
			if (!result) return;
			result = TestArguments(self->placementArguments, expr->placementArguments);
		}
	}

	void Visit(UniversalInitializerExpr* self)override
	{
		if (auto expr = peerExpr.Cast<UniversalInitializerExpr>())
		{
			result = TestArguments(self->arguments, expr->arguments);
		}
	}

	void Visit(PostfixUnaryExpr* self)override
	{
		if (auto expr = peerExpr.Cast<PostfixUnaryExpr>())
		{
			result =
				self->op == expr->op &&
				IsSameResolvedExpr(self->operand, expr->operand, equivalentNames);
		}
	}

	void Visit(PrefixUnaryExpr* self)override
	{
		if (auto expr = peerExpr.Cast<PrefixUnaryExpr>())
		{
			result =
				self->op == expr->op &&
				IsSameResolvedExpr(self->operand, expr->operand, equivalentNames);
		}
	}

	void Visit(BinaryExpr* self)override
	{
		if (auto expr = peerExpr.Cast<BinaryExpr>())
		{
			result =
				self->op == expr->op &&
				IsSameResolvedExpr(self->left, expr->left, equivalentNames) &&
				IsSameResolvedExpr(self->right, expr->right, equivalentNames);
		}
	}

	void Visit(IfExpr* self)override
	{
		if (auto expr = peerExpr.Cast<IfExpr>())
		{
			result =
				IsSameResolvedExpr(self->condition, expr->condition, equivalentNames) &&
				IsSameResolvedExpr(self->left, expr->left, equivalentNames) &&
				IsSameResolvedExpr(self->right, expr->right, equivalentNames);
		}
	}

	bool TestGenericArguments(VariadicList<GenericArgument>& es1, VariadicList<GenericArgument>& es2)
	{
		if (es1.Count() != es2.Count()) return false;
		for (vint i = 0; i < es1.Count(); i++)
		{
			auto& a1 = es1[i];
			auto& a2 = es2[i];
			if (a1.isVariadic != a2.isVariadic) return false;
			if (!IsSameResolvedExpr(a1.item.expr, a2.item.expr, equivalentNames)) return false;
			if (!IsSameResolvedType(a1.item.type, a2.item.type, equivalentNames)) return false;
		}
		return true;
	}

	void Visit(GenericExpr* self)override
	{
		if (auto expr = peerExpr.Cast<GenericExpr>())
		{
			if (!IsSameResolvedExpr(self->expr, expr->expr, equivalentNames)) return;
			if (!TestGenericArguments(self->arguments, expr->arguments)) return;
			result = true;
		}
	}

	void Visit(BuiltinFuncAccessExpr* self)override
	{
		if (auto expr = peerExpr.Cast<BuiltinFuncAccessExpr>())
		{
			result = IsSameResolvedType(self->returnType, expr->returnType, equivalentNames);
		}
	}
};

bool IsSameResolvedExpr(Ptr<Expr> e1, Ptr<Expr> e2, Dictionary<WString, WString>& equivalentNames)
{
	if (e1 && e2)
	{
		IsSameResolvedExprVisitor visitor(e2, equivalentNames);
		e1->Accept(&visitor);
		return visitor.result;
	}
	else
	{
		return (e1 == nullptr) == (e2 == nullptr);
	}
}

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
				result = IsSameResolvedExpr(self->expr, declType->expr, equivalentNames);
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
			result =	self->isConst == type->isConst
				&&		self->isVolatile == type->isVolatile
				&&		IsSameResolvedType(self->type, type->type, equivalentNames);
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
			}
			else
			{
				if (self->name.name != idType->name.name) return;
				result = Resolving::ContainsSameSymbol(self->resolving, idType->resolving);
			}
		}
		else if (auto childType = peerType.Cast<ChildType>())
		{
			if (self->name.name != childType->name.name) return;
			result = Resolving::ContainsSameSymbol(self->resolving, childType->resolving);
		}
	}

	void Visit(ChildType* self)override
	{
		if (auto idType = peerType.Cast<IdType>())
		{
			if (self->name.name != idType->name.name) return;
			result = Resolving::ContainsSameSymbol(self->resolving, idType->resolving);
		}
		else if (auto childType = peerType.Cast<ChildType>())
		{
			if (self->name.name != childType->name.name) return;
			result = IsSameResolvedType(self->classType, childType->classType, equivalentNames);
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
		return !t1 && !t2;
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

bool IsCompatibleSpecializationSpec(Ptr<SpecializationSpec> specNew, Ptr<SpecializationSpec> specOld, Dictionary<WString, WString>& equivalentNames)
{
	if (specNew && specOld)
	{
		if (specNew->arguments.Count() != specOld->arguments.Count()) return false;
		for (vint i = 0; i < specNew->arguments.Count(); i++)
		{
			const auto& argNew = specNew->arguments[i];
			const auto& argOld = specOld->arguments[i];
			if (argNew.isVariadic != argOld.isVariadic) return false;
			if (argNew.item.type && argOld.item.type)
			{
				if (!IsSameResolvedType(argNew.item.type, argOld.item.type, equivalentNames))
				{
					return false;
				}
			}
			else if (argNew.item.expr && argOld.item.expr)
			{
				if (!IsSameResolvedExpr(argNew.item.expr, argOld.item.expr, equivalentNames))
				{
					return false;
				}
			}
			else
			{
				return false;
			}
		}
		return true;
	}
	return !specNew && !specOld;
}

bool IsCompatibleFunctionDeclInSameScope(Ptr<symbol_component::ClassMemberCache> cacheNew, Ptr<ForwardFunctionDeclaration> declNew, Ptr<ForwardFunctionDeclaration> declOld)
{
	if (declNew->name.name != declOld->name.name) return false;
	if (declNew->needResolveTypeFromStatement || declOld->needResolveTypeFromStatement) return false;
	if (declNew->methodType != declOld->methodType) return false;

	Dictionary<WString, WString> equivalentNames;
	if (!IsCompatibleTemplateSpec(declNew->templateSpec, declOld->templateSpec, equivalentNames))
	{
		return false;
	}
	if (!IsCompatibleSpecializationSpec(declNew->specializationSpec, declOld->specializationSpec, equivalentNames))
	{
		return false;
	}
	if (auto funcDecl = declNew.Cast<FunctionDeclaration>())
	{
		if (cacheNew && cacheNew->containerClassSpecs.Count() > 0)
		{
			for (vint i = 0; i < cacheNew->containerClassTypes.Count(); i++)
			{
				auto tsys = cacheNew->containerClassTypes[i];
				auto spec = cacheNew->containerClassSpecs[i];
				if (spec)
				{
					auto classSpec = tsys->GetDecl()->GetImplDecl_NFb<ClassDeclaration>()->templateSpec;
					if (!IsCompatibleTemplateSpec(spec, classSpec, equivalentNames))
					{
						return false;
					}
				}
			}
		}
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