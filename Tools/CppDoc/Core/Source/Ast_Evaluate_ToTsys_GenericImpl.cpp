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
			bool allowPartialApply = declSymbol->kind == symbol_component::SymbolKind::FunctionBodySymbol;
			vint partialAppliedArguments = -1;
			ResolveGenericParameters(pa, taContext, genericFunction, args, isTypes, argSource, boundedAnys, 1, allowPartialApply, partialAppliedArguments);
			process(genericFunction, declSymbol, &taContext, partialAppliedArguments);
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

	void UseTsys(ExprTsysList& result, Symbol* symbol, ITsys* tsys)
	{
		AddExprTsysItemToResult(result, { symbol,ExprTsysType::PRValue,tsys });
	}

	void UseTypeTsysList(ExprTsysList& result, Symbol* symbol, TypeTsysList& tsys)
	{
		for (vint j = 0; j < tsys.Count(); j++)
		{
			UseTsys(result, symbol, tsys[j]);
		}
	}

	//////////////////////////////////////////////////////////////////////////////////////
	// ProcessGenericType
	//////////////////////////////////////////////////////////////////////////////////////
	
	void ProcessGenericType(const ParsingArguments& pa, ExprTsysList& result, GenericType* self, Array<bool>& isTypes, Array<ExprTsysItem>& args, Array<vint>& argSource, SortedList<vint>& boundedAnys)
	{
		ProcessGenericType(pa, result, isTypes, args, argSource, boundedAnys, [&pa, &result](ITsys* genericFunction, Symbol* declSymbol, TemplateArgumentContext* argumentsToApply, vint partialAppliedArguments)
		{
			if (partialAppliedArguments != -1) throw NotConvertableException();
			auto parentDeclType = genericFunction->GetGenericFunction().parentDeclType;
			switch (declSymbol->kind)
			{
			case CLASS_SYMBOL_KIND:
				{
					auto decl = declSymbol->GetAnyForwardDecl<ForwardClassDeclaration>();
					if (!decl->templateSpec) throw NotConvertableException();
					auto& tsys = EvaluateForwardClassSymbol(pa, decl.Obj(), parentDeclType, argumentsToApply);
					UseTypeTsysList(result, declSymbol, tsys);
				}
				break;
			case symbol_component::SymbolKind::TypeAlias:
				{
					auto decl = declSymbol->GetImplDecl_NFb<TypeAliasDeclaration>();
					if (!decl->templateSpec) throw NotConvertableException();
					auto& tsys = EvaluateTypeAliasSymbol(pa, decl.Obj(), parentDeclType, argumentsToApply);
					UseTypeTsysList(result, nullptr, tsys);
				}
				break;
			case symbol_component::SymbolKind::GenericTypeArgument:
				{
					auto tsys = genericFunction->GetElement()->ReplaceGenericArgs(pa.AppendSingleLevelArgs(*argumentsToApply));
					UseTsys(result, nullptr, tsys);
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
		ProcessGenericType(pa, result, isTypes, args, argSource, boundedAnys, [&pa, &result](ITsys* genericFunction, Symbol* declSymbol, TemplateArgumentContext* argumentsToApply, vint partialAppliedArguments)
		{
			auto parentDeclType = genericFunction->GetGenericFunction().parentDeclType;
			switch (declSymbol->kind)
			{
			case symbol_component::SymbolKind::FunctionBodySymbol:
				{
					ITsys* classType = nullptr;
					{
						auto elementType = genericFunction->GetElement();
						if (elementType->GetType() == TsysType::Ptr)
						{
							elementType = elementType->GetElement();
						}
						if (elementType->GetType() == TsysType::Member)
						{
							// the GenericFunction type of a Class::* FunctionType does not contain type arguments for Class
							classType = elementType->GetClass();
						}
					}

					auto decl = declSymbol->GetAnyForwardDecl<ForwardFunctionDeclaration>();
					if (!decl->templateSpec) throw NotConvertableException();

					Array<ITsys*> decoratedTsys;
					{
						auto& tsys = EvaluateFuncSymbol(pa, decl.Obj(), parentDeclType, argumentsToApply);
						CopyFrom(decoratedTsys, tsys);
					}

					for (vint i = 0; i < decoratedTsys.Count(); i++)
					{
						if (classType)
						{
							decoratedTsys[i] = decoratedTsys[i]->MemberOf(classType)->PtrOf();
						}
						else
						{
							decoratedTsys[i] = decoratedTsys[i]->PtrOf();
						}
					}

					if (partialAppliedArguments != -1)
					{
						TsysGenericFunction gfi = genericFunction->GetGenericFunction();
						gfi.filledArguments = partialAppliedArguments;

						Array<ITsys*> params(genericFunction->GetParamCount());
						for (vint i = 0; i < params.Count(); i++)
						{
							auto pattern = genericFunction->GetParam(i);
							if (i < partialAppliedArguments)
							{
								params[i] = argumentsToApply->arguments[pattern];
							}
							else
							{
								params[i] = pattern;
							}
						}

						for (vint i = 0; i < decoratedTsys.Count(); i++)
						{
							decoratedTsys[i] = decoratedTsys[i]->GenericFunctionOf(params, gfi);
						}
					}

					for (vint i = 0; i < decoratedTsys.Count(); i++)
					{
						UseTsys(result, declSymbol, decoratedTsys[i]);
					}
				}
				break;
			case symbol_component::SymbolKind::ValueAlias:
				{
					if (partialAppliedArguments != -1) throw NotConvertableException();
					auto decl = declSymbol->GetImplDecl_NFb<ValueAliasDeclaration>();
					if (!decl->templateSpec) throw NotConvertableException();
					auto& tsys = EvaluateValueAliasSymbol(pa, decl.Obj(), parentDeclType, argumentsToApply);
					UseTypeTsysList(result, nullptr, tsys);
				}
				break;
			default:
				throw NotConvertableException();
			}
		});
	}
}