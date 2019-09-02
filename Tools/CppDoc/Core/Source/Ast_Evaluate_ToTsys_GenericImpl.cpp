#include "Ast_Resolving_ExpandPotentialVta.h"

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
			ResolveGenericParameters(pa.WithArgs(taContext), genericFunction, args, isTypes, argSource, boundedAnys, 1);

			EvaluateSymbolContext esContext;
			process(genericFunction, declSymbol, esContext);

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
		ProcessGenericType(pa, result, isTypes, args, argSource, boundedAnys, [&](ITsys* genericFunction, Symbol* declSymbol, EvaluateSymbolContext& esContext)
		{
			switch (declSymbol->kind)
			{
			case symbol_component::SymbolKind::GenericTypeArgument:
				genericFunction->GetElement()->ReplaceGenericArgs(pa, esContext.evaluatedTypes);
				break;
			case symbol_component::SymbolKind::TypeAlias:
				{
					auto decl = declSymbol->GetImplDecl_NFb<TypeAliasDeclaration>();
					if (!decl->templateSpec) throw NotConvertableException();
					EvaluateTypeAliasSymbol(pa, decl.Obj(), &esContext);
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
		ProcessGenericType(pa, result, isTypes, args, argSource, boundedAnys, [&](ITsys* genericFunction, Symbol* declSymbol, EvaluateSymbolContext& esContext)
		{
			switch (declSymbol->kind)
			{
			case symbol_component::SymbolKind::FunctionBodySymbol:
				{
					auto decl = declSymbol->GetAnyForwardDecl<ForwardFunctionDeclaration>();
					if (!decl->templateSpec) throw NotConvertableException();
					symbol_type_resolving::EvaluateFuncSymbol(pa, decl.Obj(), &esContext);
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
					symbol_type_resolving::EvaluateValueAliasSymbol(pa, decl.Obj(), &esContext);
				}
				break;
			default:
				throw NotConvertableException();
			}
		});
	}
}