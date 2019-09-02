#include "Ast_Evaluate_ExpandPotentialVta.h"

using namespace symbol_type_resolving;

namespace symbol_totsys_impl
{
	//////////////////////////////////////////////////////////////////////////////////////
	// ProcessGenericNode
	//////////////////////////////////////////////////////////////////////////////////////
	
	template<typename TProcess>
	void ProcessGenericType(const ParsingArguments& pa, ExprTsysList& result, Array<bool>& isTypes, Array<ExprTsysItem>& args, Array<vint>& argSource, SortedList<vint>& boundedAnys, TProcess&& process)
	{
		auto genericFunction = args[0].tsys;
		if (
			genericFunction->GetType() == TsysType::Ptr &&
			genericFunction->GetElement()->GetType() == TsysType::GenericFunction &&
			genericFunction->GetElement()->GetElement()->GetType() == TsysType::Function
			)
		{
			genericFunction = genericFunction->GetElement();
		}

		if (genericFunction->GetType() == TsysType::GenericFunction)
		{
			auto declSymbol = genericFunction->GetGenericFunction().declSymbol;
			if (!declSymbol)
			{
				throw NotConvertableException();
			}

			TemplateArgumentContext taContext;
			taContext.symbolToApply = genericFunction->GetGenericFunction().declSymbol;
			// TODO: Call WithArgs later with mommon nearest ancestry
			auto newPa = pa;
			newPa.taContext = &taContext;
			ResolveGenericParameters(newPa, genericFunction, args, isTypes, argSource, boundedAnys, 1);

			EvaluateSymbolContext esContext;
			process(newPa, genericFunction, declSymbol, esContext);

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
	// ProcessGenericType
	//////////////////////////////////////////////////////////////////////////////////////
	
	void ProcessGenericType(const ParsingArguments& pa, ExprTsysList& result, GenericType* self, Array<bool>& isTypes, Array<ExprTsysItem>& args, Array<vint>& argSource, SortedList<vint>& boundedAnys)
	{
		ProcessGenericType(pa, result, isTypes, args, argSource, boundedAnys, [](const ParsingArguments& newPa, ITsys* genericFunction, Symbol* declSymbol, EvaluateSymbolContext& esContext)
		{
			switch (declSymbol->kind)
			{
			case symbol_component::SymbolKind::GenericTypeArgument:
				genericFunction->GetElement()->ReplaceGenericArgs(newPa, esContext.evaluatedTypes);
				break;
			case symbol_component::SymbolKind::TypeAlias:
				{
					auto decl = declSymbol->GetImplDecl_NFb<TypeAliasDeclaration>();
					if (!decl->templateSpec) throw NotConvertableException();
					EvaluateTypeAliasSymbol(newPa, decl.Obj(), &esContext);
				}
				break;
			default:
				throw NotConvertableException();
			}
		});
	}

	//////////////////////////////////////////////////////////////////////////////////////
	// ProcessGenericExpr
	//////////////////////////////////////////////////////////////////////////////////////

	void ProcessGenericExpr(const ParsingArguments& pa, ExprTsysList& result, GenericExpr* self, Array<bool>& isTypes, Array<ExprTsysItem>& args, Array<vint>& argSource, SortedList<vint>& boundedAnys)
	{
		ProcessGenericType(pa, result, isTypes, args, argSource, boundedAnys, [](const ParsingArguments& newPa, ITsys* genericFunction, Symbol* declSymbol, EvaluateSymbolContext& esContext)
		{
			switch (declSymbol->kind)
			{
			case symbol_component::SymbolKind::FunctionBodySymbol:
				{
					auto decl = declSymbol->GetAnyForwardDecl<ForwardFunctionDeclaration>();
					if (!decl->templateSpec) throw NotConvertableException();
					symbol_type_resolving::EvaluateFuncSymbol(newPa, decl.Obj(), &esContext);
					for (vint i = 0; i < esContext.evaluatedTypes.Count(); i++)
					{
						esContext.evaluatedTypes[i] = esContext.evaluatedTypes[i]->PtrOf();
					}
				}
				break;
			case symbol_component::SymbolKind::ValueAlias:
				{
					auto decl = declSymbol->GetImplDecl_NFb<ValueAliasDeclaration>();
					if (!decl->templateSpec) throw NotConvertableException();
					symbol_type_resolving::EvaluateValueAliasSymbol(newPa, decl.Obj(), &esContext);
				}
				break;
			default:
				throw NotConvertableException();
			}
		});
	}
}