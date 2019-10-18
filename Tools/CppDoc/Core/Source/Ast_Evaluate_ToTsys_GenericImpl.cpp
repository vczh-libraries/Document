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
			ResolveGenericParameters(pa, taContext, genericFunction, args, isTypes, argSource, boundedAnys, 1);

			symbol_component::Evaluation* evaluation = nullptr;
			{
				symbol_component::SG_Cache cacheKey;
				cacheKey.parentDeclTypeAndParams = MakePtr<Array<ITsys*>>(genericFunction->GetParamCount() + 1);
				cacheKey.parentDeclTypeAndParams->Set(0, genericFunction->GetGenericFunction().parentDeclType);
				for (vint i = 0; i < genericFunction->GetParamCount(); i++)
				{
					cacheKey.parentDeclTypeAndParams->Set(i + 1, taContext.arguments[genericFunction->GetParam(i)]);
				}

				vint index = declSymbol->genericCaches.IndexOf(cacheKey);
				if (index == -1)
				{
					cacheKey.cachedEvaluation = MakePtr<symbol_component::Evaluation>();
					declSymbol->genericCaches.Add(cacheKey);
					evaluation = cacheKey.cachedEvaluation.Obj();

					EvaluateSymbolContext esContext;
					esContext.additionalArguments = &taContext;
					esContext.evaluation = evaluation;
					process(genericFunction, declSymbol, esContext);
				}
				else
				{
					evaluation = declSymbol->genericCaches[index].cachedEvaluation.Obj();
				}
			}

			{
				auto& tsys = evaluation->Get();
				for (vint j = 0; j < tsys.Count(); j++)
				{
					AddExprTsysItemToResult(result, GetExprTsysItem(tsys[j]));
				}
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
		ProcessGenericType(pa, result, isTypes, args, argSource, boundedAnys, [&pa](ITsys* genericFunction, Symbol* declSymbol, EvaluateSymbolContext& esContext)
		{
			auto parentDeclType = genericFunction->GetGenericFunction().parentDeclType;
			switch (declSymbol->kind)
			{
			case symbol_component::SymbolKind::Class:
			case symbol_component::SymbolKind::Struct:
			case symbol_component::SymbolKind::Union:
			case symbol_component::SymbolKind::GenericTypeArgument:
				esContext.evaluation->Allocate();
				esContext.evaluation->Get().Add(genericFunction->GetElement()->ReplaceGenericArgs(pa.AppendSingleLevelArgs(*esContext.additionalArguments)));
				break;
			case symbol_component::SymbolKind::TypeAlias:
				{
					auto decl = declSymbol->GetImplDecl_NFb<TypeAliasDeclaration>();
					if (!decl->templateSpec) throw NotConvertableException();
					EvaluateTypeAliasSymbol(pa, decl.Obj(), parentDeclType, &esContext);
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
		ProcessGenericType(pa, result, isTypes, args, argSource, boundedAnys, [&pa](ITsys* genericFunction, Symbol* declSymbol, EvaluateSymbolContext& esContext)
		{
			auto parentDeclType = genericFunction->GetGenericFunction().parentDeclType;
			switch (declSymbol->kind)
			{
			case symbol_component::SymbolKind::FunctionBodySymbol:
				{
					auto decl = declSymbol->GetAnyForwardDecl<ForwardFunctionDeclaration>();
					if (!decl->templateSpec) throw NotConvertableException();
					EvaluateFuncSymbol(pa, decl.Obj(), parentDeclType, &esContext);

					auto& tsys = esContext.evaluation->Get();
					for (vint i = 0; i < tsys.Count(); i++)
					{
						tsys[i] = tsys[i]->PtrOf();
					}
				}
				break;
			case symbol_component::SymbolKind::ValueAlias:
				{
					auto decl = declSymbol->GetImplDecl_NFb<ValueAliasDeclaration>();
					if (!decl->templateSpec) throw NotConvertableException();
					EvaluateValueAliasSymbol(pa, decl.Obj(), parentDeclType, &esContext);
				}
				break;
			default:
				throw NotConvertableException();
			}
		});
	}
}