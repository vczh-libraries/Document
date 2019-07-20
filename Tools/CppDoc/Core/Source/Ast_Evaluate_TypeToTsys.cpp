#include "Ast_Expr.h"
#include "Ast_Resolving_ExpandPotentialVta.h"

using namespace symbol_type_resolving;
using namespace symbol_totsys_impl;

/***********************************************************************
TypeToTsys
  PrimitiveType				: literal		*
  ReferenceType				: unbounded		*
  ArrayType					: unbounded		*
  CallingConventionType		: unbounded
  FunctionType				: variant		+
  MemberType				: unbounded		*
  DeclType					: unbounded
  DecorateType				: unbounded		*
  RootType					: literal
  IdType					: identifier	*
  ChildType					: unbounded		*
  GenericType				: variant		+
***********************************************************************/

class TypeToTsysVisitor : public Object, public virtual ITypeVisitor
{
public:
	TypeTsysList&				result;
	bool						isVta = false;

	const ParsingArguments&		pa;
	TypeTsysList*				returnTypes;
	GenericArgContext*			gaContext = nullptr;
	bool						memberOf = false;
	TsysCallingConvention		cc = TsysCallingConvention::None;

	TypeToTsysVisitor(const ParsingArguments& _pa, TypeTsysList& _result, TypeTsysList* _returnTypes, GenericArgContext* _gaContext, bool _memberOf, TsysCallingConvention _cc)
		:pa(_pa)
		, result(_result)
		, returnTypes(_returnTypes)
		, gaContext(_gaContext)
		, cc(_cc)
		, memberOf(_memberOf)
	{
	}

	void AddResult(ITsys* tsys)
	{
		AddTsysToResult(result, tsys);
	}

	//////////////////////////////////////////////////////////////////////////////////////
	// PrimitiveType
	//////////////////////////////////////////////////////////////////////////////////////

	void Visit(PrimitiveType* self)override
	{
		AddResult(ProcessPrimitiveType(pa, self));
	}

	//////////////////////////////////////////////////////////////////////////////////////
	// ReferenceType
	//////////////////////////////////////////////////////////////////////////////////////

	void Visit(ReferenceType* self)override
	{
		TypeTsysList items1;
		bool isVta1 = false;
		TypeToTsysInternal(pa, self->type, items1, gaContext, isVta1);
		isVta = ExpandPotentialVta(pa, result, [=](ExprTsysItem arg1)
		{
			return ProcessReferenceType(pa, self, arg1);
		}, Input(items1, isVta1));
	}

	//////////////////////////////////////////////////////////////////////////////////////
	// ArrayType
	//////////////////////////////////////////////////////////////////////////////////////

	void Visit(ArrayType* self)override
	{
		TypeTsysList items1;
		bool isVta1 = false;
		TypeToTsysInternal(pa, self->type, items1, gaContext, isVta1);
		isVta = ExpandPotentialVta(pa, result, [=](ExprTsysItem arg1)
		{
			return ProcessArrayType(pa, self, arg1);
		}, Input(items1, isVta1));
	}

	//////////////////////////////////////////////////////////////////////////////////////
	// CallingConventionType
	//////////////////////////////////////////////////////////////////////////////////////

	void Visit(CallingConventionType* self)override
	{
		auto oldCc = cc;
		cc = self->callingConvention;

		self->type->Accept(this);
		cc = oldCc;
	}

	//////////////////////////////////////////////////////////////////////////////////////
	// FunctionType
	//////////////////////////////////////////////////////////////////////////////////////

	void Visit(FunctionType* self)override
	{
		vint count = self->parameters.Count() + 1;
		Array<TypeTsysList> tsyses(count);
		Array<bool> isVtas(count);
		isVtas[0] = false;

		if (returnTypes)
		{
			CopyFrom(tsyses[0], *returnTypes);
		}
		else if (self->decoratorReturnType)
		{
			TypeToTsysInternal(pa, self->decoratorReturnType, tsyses[0], gaContext, isVtas[0]);
		}
		else if (self->returnType)
		{
			TypeToTsysInternal(pa, self->returnType, tsyses[0], gaContext, isVtas[0]);
		}
		else
		{
			tsyses[0].Add(pa.tsys->Void());
		}

		for (vint i = 1; i < count; i++)
		{
			TypeToTsysInternal(pa, self->parameters[i - 1].item->type, tsyses[i], gaContext, isVtas[i]);
		}

		bool hasBoundedVta = false;
		bool hasUnboundedVta = isVtas[0];
		vint unboundedVtaCount = -1;
		CheckVta(self->parameters, tsyses, isVtas, 1, hasBoundedVta, hasUnboundedVta, unboundedVtaCount);
		isVta = hasUnboundedVta;

		TsysFunc func(cc, self->ellipsis);
		if (func.callingConvention == TsysCallingConvention::None)
		{
			func.callingConvention =
				memberOf && !func.ellipsis
				? TsysCallingConvention::ThisCall
				: TsysCallingConvention::CDecl
				;
		}

		ExpandPotentialVtaList(pa, result, tsyses, isVtas, hasBoundedVta, unboundedVtaCount,
			[=](ExprTsysList& processResult, Array<ExprTsysItem>& args, SortedList<vint>& boundedAnys)
			{
				if (boundedAnys.Count())
				{
					AddExprTsysItemToResult(processResult, GetExprTsysItem(pa.tsys->Any()));
				}
				else
				{
					Array<ITsys*> params(args.Count() - 1);
					for (vint i = 1; i < args.Count(); i++)
					{
						params[i - 1] = args[i].tsys;
					}

					auto funcTsys = args[0].tsys->FunctionOf(params, func);
					AddExprTsysItemToResult(processResult, GetExprTsysItem(funcTsys));
				}
			});
	}

	//////////////////////////////////////////////////////////////////////////////////////
	// MemberType
	//////////////////////////////////////////////////////////////////////////////////////

	void Visit(MemberType* self)override
	{
		TypeTsysList types, classTypes;
		bool typesVta = false;
		bool classTypesVta = false;
		TypeToTsysInternal(pa, self->type, types, gaContext, typesVta, true, cc);
		TypeToTsysInternal(pa, self->classType, classTypes, gaContext, classTypesVta);
		isVta = ExpandPotentialVta(pa, result, [=](ExprTsysItem argType, ExprTsysItem argClass)
		{
			return ProcessMemberType(pa, self, argType, argClass);
		}, Input(types, typesVta), Input(classTypes, classTypesVta));
	}

	//////////////////////////////////////////////////////////////////////////////////////
	// DeclType
	//////////////////////////////////////////////////////////////////////////////////////

	void Visit(DeclType* self)override
	{
		if (self->expr)
		{
			ExprTsysList types;
			ExprToTsys(pa, self->expr, types, gaContext);
			for (vint i = 0; i < types.Count(); i++)
			{
				auto exprTsys = types[i].tsys;
				if (exprTsys->GetType() == TsysType::Zero)
				{
					exprTsys = pa.tsys->Int();
				}
				AddResult(exprTsys);
			}
		}
	}

	//////////////////////////////////////////////////////////////////////////////////////
	// DecorateType
	//////////////////////////////////////////////////////////////////////////////////////

	void Visit(DecorateType* self)override
	{
		TypeTsysList items1;
		bool isVta1 = false;
		TypeToTsysInternal(pa, self->type, items1, gaContext, isVta1);
		isVta = ExpandPotentialVta(pa, result, [=](ExprTsysItem arg1)
		{
			return ProcessDecorateType(pa, self, arg1);
		}, Input(items1, isVta1));
	}

	//////////////////////////////////////////////////////////////////////////////////////
	// RootType
	//////////////////////////////////////////////////////////////////////////////////////

	void Visit(RootType* self)override
	{
		throw NotConvertableException();
	}

	//////////////////////////////////////////////////////////////////////////////////////
	// IdType
	//////////////////////////////////////////////////////////////////////////////////////

	void Visit(IdType* self)override
	{
		CreateIdReferenceType(pa, gaContext, self->resolving, false, true, result, isVta);
	}

	//////////////////////////////////////////////////////////////////////////////////////
	// ChildType
	//////////////////////////////////////////////////////////////////////////////////////

	static bool IsResolvingAllNamespaces(Ptr<Resolving> resolving)
	{
		if (resolving)
		{
			for (vint i = 0; i < resolving->resolvedSymbols.Count(); i++)
			{
				if (resolving->resolvedSymbols[i]->kind != symbol_component::SymbolKind::Namespace)
				{
					return false;
				}
			}
		}
		return true;
	}

	void Visit(ChildType* self)override
	{
		if (auto resolvableType = self->classType.Cast<ResolvableType>())
		{
			if (IsResolvingAllNamespaces(resolvableType->resolving))
			{
				CreateIdReferenceType(pa, gaContext, self->resolving, true, false, result, isVta);
				return;
			}
		}

		TypeTsysList classTypes;
		bool classIsVta = false;
		TypeToTsysInternal(pa, self->classType, classTypes, gaContext, classIsVta);

		isVta = ExpandPotentialVtaMultiResult(pa, result, [=](ExprTsysList& processResult, ExprTsysItem argClass)
		{
			ProcessChildType(pa, gaContext, self, argClass, processResult);
		}, Input(classTypes, classIsVta));
	}

	//////////////////////////////////////////////////////////////////////////////////////
	// GenericType
	//////////////////////////////////////////////////////////////////////////////////////

	void Visit(GenericType* self)override
	{
		vint count = self->arguments.Count();
		Array<TypeTsysList> tsyses(count + 1);
		Array<bool> isTypes(count);
		Array<bool> isVtas(count + 1);

		TypeToTsysInternal(pa, self->type, tsyses[0], gaContext, isVtas[0]);
		ResolveGenericArguments(pa, self->arguments, tsyses, isTypes, isVtas, 1, gaContext);

		bool hasBoundedVta = false;
		bool hasUnboundedVta = isVtas[0];
		vint unboundedVtaCount = -1;
		CheckVta(self->arguments, tsyses, isVtas, 1, hasBoundedVta, hasUnboundedVta, unboundedVtaCount);
		isVta = hasUnboundedVta;

		// TODO: Implement variadic template argument passing
		if (hasBoundedVta)
		{
			throw NotConvertableException();
		}

		ExpandPotentialVtaList(pa, result, tsyses, isVtas, hasBoundedVta, unboundedVtaCount,
			[&](ExprTsysList& processResult, Array<ExprTsysItem>& args, SortedList<vint>& boundedAnys)
			{
				auto genericFunction = args[0].tsys;
				if (genericFunction->GetType() == TsysType::GenericFunction)
				{
					auto declSymbol = genericFunction->GetGenericFunction().declSymbol;
					if (!declSymbol)
					{
						throw NotConvertableException();
					}

					EvaluateSymbolContext esContext;
					Array<TypeTsysList> argumentTypes(args.Count() - 1);
					for (vint i = 1; i < args.Count(); i++)
					{
						argumentTypes[i - 1].Add(args[i].tsys);
					}
					// TODO: Receive Array<TypeTsysItem> instead of Array<TypeTsysList> in ResolveGenericParameters
					if (!ResolveGenericParameters(pa, genericFunction, argumentTypes, isTypes, &esContext.gaContext))
					{
						throw NotConvertableException();
					}

					switch (declSymbol->kind)
					{
					case symbol_component::SymbolKind::GenericTypeArgument:
						genericFunction->GetElement()->ReplaceGenericArgs(esContext.gaContext, esContext.evaluatedTypes);
						break;
					case symbol_component::SymbolKind::TypeAlias:
						{
							auto decl = declSymbol->definition.Cast<TypeAliasDeclaration>();
							if (!decl->templateSpec) throw NotConvertableException();
							EvaluateSymbol(pa, decl.Obj(), &esContext);
						}
						break;
					default:
						throw NotConvertableException();
					}

					for (vint j = 0; j < esContext.evaluatedTypes.Count(); j++)
					{
						AddExprTsysItemToResult(processResult, GetExprTsysItem(esContext.evaluatedTypes[j]));
					}
				}
				else if (genericFunction->GetType() == TsysType::Any)
				{
					AddExprTsysItemToResult(processResult, GetExprTsysItem(pa.tsys->Any()));
				}
				else
				{
					throw NotConvertableException();
				}
			});
	}
};

// Convert type AST to type system object

void TypeToTsysInternal(const ParsingArguments& pa, Type* t, TypeTsysList& tsys, GenericArgContext* gaContext, bool& isVta, bool memberOf, TsysCallingConvention cc)
{
	if (!t) throw NotConvertableException();
	TypeToTsysVisitor visitor(pa, tsys, nullptr, gaContext, memberOf, cc);
	t->Accept(&visitor);
	isVta = visitor.isVta;
}
void TypeToTsysInternal(const ParsingArguments& pa, Ptr<Type> t, TypeTsysList& tsys, GenericArgContext* gaContext, bool& isVta, bool memberOf, TsysCallingConvention cc)
{
	TypeToTsysInternal(pa, t.Obj(), tsys, gaContext, isVta, memberOf, cc);
}

void TypeToTsysNoVta(const ParsingArguments& pa, Type* t, TypeTsysList& tsys, GenericArgContext* gaContext, bool memberOf, TsysCallingConvention cc)
{
	bool isVta = false;
	TypeToTsysInternal(pa, t, tsys, gaContext, isVta, memberOf, cc);
	if (isVta)
	{
		throw NotConvertableException();
	}
}

void TypeToTsysNoVta(const ParsingArguments& pa, Ptr<Type> t, TypeTsysList& tsys, GenericArgContext* gaContext, bool memberOf, TsysCallingConvention cc)
{
	TypeToTsysNoVta(pa, t.Obj(), tsys, gaContext, memberOf, cc);
}

void TypeToTsysAndReplaceFunctionReturnType(const ParsingArguments& pa, Ptr<Type> t, TypeTsysList& returnTypes, TypeTsysList& tsys, GenericArgContext* gaContext, bool memberOf)
{
	if (!t) throw NotConvertableException();
	TypeToTsysVisitor visitor(pa, tsys, &returnTypes, gaContext, memberOf, TsysCallingConvention::None);
	t->Accept(&visitor);
	if (visitor.isVta)
	{
		throw NotConvertableException();
	}
}