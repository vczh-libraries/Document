#include "Ast_Evaluate_ExpandPotentialVta.h"
#include "AP.h"
#include "EvaluateSymbol.h"

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
				throw TypeCheckerException();
			}

			TemplateArgumentContext taContext(genericFunction->GetGenericFunction().declSymbol, genericFunction->GetParamCount());
			bool allowPartialApply = declSymbol->kind == symbol_component::SymbolKind::FunctionBodySymbol;
			vint partialAppliedArguments = -1;

			if (allowPartialApply)
			{
				try
				{
					// function is allowed to fail because there could be multiple overloaded functions
					assign_parameters::ResolveGenericParameters(pa, taContext, genericFunction, args, isTypes, argSource, boundedAnys, 1, allowPartialApply, partialAppliedArguments);
				}
				catch (const TypeCheckerException&)
				{
					return;
				}
				process(genericFunction, declSymbol, &taContext, partialAppliedArguments);
			}
			else
			{
				assign_parameters::ResolveGenericParameters(pa, taContext, genericFunction, args, isTypes, argSource, boundedAnys, 1, allowPartialApply, partialAppliedArguments);
				process(genericFunction, declSymbol, &taContext, partialAppliedArguments);
			}
		}
		else if (genericFunction->GetType() == TsysType::Any)
		{
			AddExprTsysItemToResult(result, GetExprTsysItem(pa.tsys->Any()));
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
		ProcessGenericType(pa, result, isTypes, args, argSource, boundedAnys, [&pa, &result, self](ITsys* genericFunction, Symbol* declSymbol, TemplateArgumentContext* argumentsToApply, vint partialAppliedArguments)
		{
			if (partialAppliedArguments != -1) throw TypeCheckerException();
			auto parentDeclType = genericFunction->GetGenericFunction().parentDeclType;
			switch (declSymbol->kind)
			{
			case CLASS_SYMBOL_KIND:
				{
					auto decl = declSymbol->GetAnyForwardDecl<ForwardClassDeclaration>();
					if (!decl->templateSpec) throw TypeCheckerException();
					auto& tsys = EvaluateForwardClassSymbol(pa, decl.Obj(), parentDeclType, argumentsToApply);
					UseTypeTsysList(result, nullptr, tsys);

					if (pa.IsGeneralEvaluation() && pa.recorder && declSymbol->IsPSPrimary_NF())
					{
						if (auto psr = EvaluateClassPSRecord(pa, tsys[0]))
						{
							if (psr->version > 0 && (psr->evaluatedTypes.Count() != 1 || psr->evaluatedTypes[0] != tsys[0]))
							{
								CppName* name = nullptr;
								Ptr<Resolving>* resolving = nullptr;
								GetCategoryTypeResolving(self->type, name, resolving);

								List<ResolvedItem> ritems;
								for (vint i = 0; i < psr->evaluatedTypes.Count(); i++)
								{
									ResolvedItem::AddItem(ritems, { nullptr,psr->evaluatedTypes[i]->GetDecl() });
								}
								pa.recorder->IndexOverloadingResolution(*name, ritems);
							}
						}
					}
				}
				break;
			case symbol_component::SymbolKind::TypeAlias:
				{
					auto decl = declSymbol->GetImplDecl_NFb<TypeAliasDeclaration>();
					if (!decl->templateSpec) throw TypeCheckerException();
					auto& tsys = EvaluateTypeAliasSymbol(pa, decl.Obj(), parentDeclType, argumentsToApply);
					UseTypeTsysList(result, nullptr, tsys);
				}
				break;
			case symbol_component::SymbolKind::GenericTypeArgument:
				{
					if (genericFunction->GetElement()->GetType() != TsysType::Any)
					{
						throw L"Type of template template argument must be a generic function to any_t!";
					}
					UseTsys(result, nullptr, pa.tsys->Any());
				}
				break;
			default:
				throw TypeCheckerException();
			}
		});
	}

	//////////////////////////////////////////////////////////////////////////////////////
	// ProcessGenericExpr
	//////////////////////////////////////////////////////////////////////////////////////

	void ProcessGenericExpr(const ParsingArguments& pa, ExprTsysList& result, GenericExpr* self, Array<bool>& isTypes, Array<ExprTsysItem>& args, Array<vint>& argSource, SortedList<vint>& boundedAnys)
	{
		ProcessGenericType(pa, result, isTypes, args, argSource, boundedAnys, [&pa, &result, self](ITsys* genericFunction, Symbol* declSymbol, TemplateArgumentContext* argumentsToApply, vint partialAppliedArguments)
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
					if (!decl->templateSpec) throw TypeCheckerException();

					Array<ITsys*> decoratedTsys;
					{
						auto& tsys = EvaluateFuncSymbol(pa, decl.Obj(), parentDeclType, argumentsToApply);
						CopyFrom(decoratedTsys, tsys);
					}

					bool addGenericSource = false;
					if (partialAppliedArguments == -1)
					{
						if (
							decl->templateSpec->arguments[decl->templateSpec->arguments.Count() - 1].ellipsis ||
							decl->symbol->GetFunctionSymbol_Fb()->IsPSPrimary_NF()
							)
						{
							partialAppliedArguments = decl->templateSpec->arguments.Count();
							addGenericSource = true;
						}
					}

					TsysGenericFunction gfi;
					Array<ITsys*> genericParams;
					if (partialAppliedArguments != -1)
					{
						gfi = genericFunction->GetGenericFunction();
						gfi.filledArguments = partialAppliedArguments;

						genericParams.Resize(genericFunction->GetParamCount());
						for (vint i = 0; i < genericParams.Count(); i++)
						{
							auto pattern = genericFunction->GetParam(i);
							if (i < partialAppliedArguments)
							{
								genericParams[i] = argumentsToApply->GetValueByKey(pattern);
							}
							else
							{
								genericParams[i] = pattern;
							}
						}
					}

					if (addGenericSource)
					{
						for (vint i = 0; i < decoratedTsys.Count(); i++)
						{
							auto tsysFunc = decoratedTsys[i];
							TsysFunc func = tsysFunc->GetFunc();
							func.genericSource = decoratedTsys[i]->GenericFunctionOf(genericParams, gfi);

							Array<ITsys*> funcParams(tsysFunc->GetParamCount());
							for (vint i = 0; i < funcParams.Count(); i++)
							{
								funcParams[i] = tsysFunc->GetParam(i);
							}
							decoratedTsys[i] = tsysFunc->GetElement()->FunctionOf(funcParams, func);
						}
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

					if (partialAppliedArguments != -1 && !addGenericSource)
					{
						for (vint i = 0; i < decoratedTsys.Count(); i++)
						{
							decoratedTsys[i] = decoratedTsys[i]->GenericFunctionOf(genericParams, gfi);
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
					if (partialAppliedArguments != -1) throw TypeCheckerException();
					auto decl = declSymbol->GetImplDecl_NFb<ValueAliasDeclaration>();
					if (!decl->templateSpec) throw TypeCheckerException();

					List<ResolvedItem> ritems;
					auto& tsys = EvaluateValueAliasSymbol(pa, decl.Obj(), parentDeclType, argumentsToApply, (pa.IsGeneralEvaluation() && pa.recorder ? &ritems : nullptr));
					UseTypeTsysList(result, declSymbol, tsys);

					if (ritems.Count() > 0)
					{
						CppName* name = nullptr;
						Ptr<Resolving>* resolving = nullptr;
						GetCategoryExprResolving(self->expr, name, resolving);
						pa.recorder->IndexOverloadingResolution(*name, ritems);
					}
				}
				break;
			default:
				throw TypeCheckerException();
			}
		});
	}
}