#include "Ast_Resolving_ExpandPotentialVta.h"

using namespace symbol_type_resolving;

namespace symbol_totsys_impl
{
	//////////////////////////////////////////////////////////////////////////////////////
	// ProcessGenericType
	//////////////////////////////////////////////////////////////////////////////////////
	
	void ProcessGenericType(const ParsingArguments& pa, ExprTsysList& result, GenericType* self, Array<bool>& isTypes, Array<ExprTsysItem>& args, SortedList<vint>& boundedAnys)
	{
		if (boundedAnys.Count() > 0)
		{
			// TODO: Implement variadic template argument passing
			throw NotConvertableException();
		}

		auto genericFunction = args[0].tsys;
		if (genericFunction->GetType() == TsysType::GenericFunction)
		{
			auto declSymbol = genericFunction->GetGenericFunction().declSymbol;
			if (!declSymbol)
			{
				throw NotConvertableException();
			}

			EvaluateSymbolContext esContext;
			if (!ResolveGenericParameters(pa, genericFunction, args, isTypes, 1, &esContext.gaContext))
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
				AddExprTsysItemToResult(result, GetExprTsysItem(esContext.evaluatedTypes[j]));
			}
		}
		else if (genericFunction->GetType() == TsysType::Any)
		{
			AddExprTsysItemToResult(result, GetExprTsysItem(pa.tsys->Any()));
		}
		else
		{
			throw NotConvertableException();
		}
	}

	//////////////////////////////////////////////////////////////////////////////////////
	// ProcessGenericExpr
	//////////////////////////////////////////////////////////////////////////////////////

	void ProcessGenericExpr(const ParsingArguments& pa, ExprTsysList& result, GenericExpr* self, Array<bool>& isTypes, Array<ExprTsysItem>& args, SortedList<vint>& boundedAnys)
	{
		if (boundedAnys.Count() > 0)
		{
			// TODO: Implement variadic template argument passing
			throw NotConvertableException();
		}

		auto genericFunction = args[0].tsys;
		if (genericFunction->GetType() == TsysType::GenericFunction)
		{
			auto declSymbol = genericFunction->GetGenericFunction().declSymbol;
			if (!declSymbol)
			{
				throw NotConvertableException();
			}

			symbol_type_resolving::EvaluateSymbolContext esContext;
			if (!symbol_type_resolving::ResolveGenericParameters(pa, genericFunction, args, isTypes, 1, &esContext.gaContext))
			{
				throw NotConvertableException();
			}

			switch (declSymbol->kind)
			{
			case symbol_component::SymbolKind::ValueAlias:
				{
					auto decl = declSymbol->definition.Cast<ValueAliasDeclaration>();
					if (!decl->templateSpec) throw NotConvertableException();
					symbol_type_resolving::EvaluateSymbol(pa, decl.Obj(), &esContext);
				}
				break;
			default:
				throw NotConvertableException();
			}

			for (vint j = 0; j < esContext.evaluatedTypes.Count(); j++)
			{
				AddInternal(result, { nullptr,ExprTsysType::PRValue,esContext.evaluatedTypes[j] });
			}
		}
		else if (genericFunction->GetType() == TsysType::Any)
		{
			AddTemp(result, pa.tsys->Any());
		}
		else
		{
			throw NotConvertableException();
		}
	}
}