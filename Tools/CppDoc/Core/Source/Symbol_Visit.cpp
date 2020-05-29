#include "Symbol_Visit.h"
#include "Symbol_TemplateSpec.h"
#include "EvaluateSymbol.h"
#include "Ast_Resolving.h"
#include "Ast_Expr.h"

namespace symbol_type_resolving
{
	/***********************************************************************
	AdjustThisItemForSymbol: Fix thisItem according to symbol
	***********************************************************************/

	ExprTsysItem AdjustThisItemForSymbol(const ParsingArguments& pa, ExprTsysItem thisItem, ResolvedItem item)
	{
		auto thisType = ApplyExprTsysType(thisItem.tsys, thisItem.type);
		auto replacedThisType = ReplaceThisType(thisType, item.tsys);
		return { nullptr,thisItem.type,replacedThisType };
	}

	/***********************************************************************
	VisitSymbol: Fill a symbol to ExprTsysList
		thisItem:
			it represents typeof(x) in x.name or typeof(*x) in x->name
			it could be null if it is initiated by IdExpr (visitMemberKind == InScope)
	***********************************************************************/

	enum class VisitMemberKind
	{
		MemberAfterType,
		MemberAfterValue,
		InScope,
	};

	void VisitSymbolInternalWithCorrectThisItem(const ParsingArguments& pa, const ExprTsysItem* thisItem, Symbol* symbol, VisitMemberKind visitMemberKind, ExprTsysList& result, bool allowVariadic, bool& hasVariadic, bool& hasNonVariadic)
	{
		Symbol* classScope = nullptr;
		if (auto parent = symbol->GetParentScope())
		{
			if (parent->GetImplDecl_NFb<ClassDeclaration>())
			{
				classScope = parent;
			}
		}

		ITsys* parentDeclType = nullptr;
		ITsys* thisEntity = nullptr;
		if (thisItem)
		{
			thisEntity = GetThisEntity(thisItem->tsys);
			if (thisEntity->GetType() == TsysType::DeclInstant)
			{
				auto& di = thisEntity->GetDeclInstant();
				parentDeclType = di.taContext ? thisEntity : di.parentDeclType;
			}
		}

		switch (symbol->kind)
		{
		case symbol_component::SymbolKind::Variable:
			{
				bool isVariadic = false;
				auto varDecl = symbol->GetAnyForwardDecl<ForwardVariableDeclaration>();
				auto& evTypes = EvaluateVarSymbol(pa, varDecl.Obj(), parentDeclType, isVariadic);
				bool isStaticSymbol = IsStaticSymbol<ForwardVariableDeclaration>(symbol);
				bool isMember = classScope && !isStaticSymbol;
				if (!thisItem && isMember)
				{
					return;
				}

				for (vint k = 0; k < evTypes.Count(); k++)
				{
					auto tsys = evTypes[k];

					// modify the type if it is captured by a containing lambda expression
					switch (symbol->GetParentScope()->kind)
					{
					case CLASS_SYMBOL_KIND:
					case symbol_component::SymbolKind::Root:
					case symbol_component::SymbolKind::Namespace:
						// types of global variables and class fields will not be changed by a lambda expression
						break;
					default:
						{
							auto current = pa.scopeSymbol;
							while (current)
							{
								switch (current->kind)
								{
								case CLASS_SYMBOL_KIND:
								case symbol_component::SymbolKind::Root:
								case symbol_component::SymbolKind::Namespace:
								case symbol_component::SymbolKind::FunctionBodySymbol:
									// captures cannot pass function scope
									goto FINISHED_LAMBDA_EXAM;
								case symbol_component::SymbolKind::Expression:
									if (auto lambdaExpr = current->GetExpr_N().Cast<LambdaExpr>())
									{
										for (vint i = 0; i < lambdaExpr->captures.Count(); i++)
										{
											auto capture = lambdaExpr->captures[i];
											if (capture.name.name == symbol->name)
											{
												switch (capture.kind)
												{
												case LambdaExpr::CaptureKind::Copy:
													if (!lambdaExpr->type->decoratorMutable)
													{
														tsys = tsys->CVOf({ true,false });
													}
													goto FINISHED_LAMBDA_EXAM;
												case LambdaExpr::CaptureKind::Ref:
													tsys = tsys->LRefOf();
													goto FINISHED_LAMBDA_EXAM;
												}
											}
										}

										switch (lambdaExpr->captureDefault)
										{
										case LambdaExpr::CaptureDefaultKind::Copy:
											if (!lambdaExpr->type->decoratorMutable)
											{
												tsys = tsys->CVOf({ true,false });
											}
											goto FINISHED_LAMBDA_EXAM;
										case LambdaExpr::CaptureDefaultKind::Ref:
											tsys = tsys->LRefOf();
											goto FINISHED_LAMBDA_EXAM;
										}
									}
									break;
								}

								current = current->GetParentScope();
							}
						}
					}
				FINISHED_LAMBDA_EXAM:

					if (isStaticSymbol)
					{
						AddInternal(result, { symbol,ExprTsysType::LValue,tsys });
					}
					else
					{
						switch (visitMemberKind)
						{
						case VisitMemberKind::MemberAfterType:
							{
								if (classScope)
								{
									AddInternal(result, { symbol,ExprTsysType::PRValue,tsys->MemberOf(thisEntity) });
								}
								else
								{
									AddInternal(result, { symbol,ExprTsysType::LValue,tsys });
								}
							}
							break;
						case VisitMemberKind::MemberAfterValue:
							{
								CalculateValueFieldType(thisItem, symbol, tsys, false, result);
							}
							break;
						case VisitMemberKind::InScope:
							{
								AddInternal(result, { symbol,ExprTsysType::LValue,tsys });
							}
							break;
						}
					}
				}
				if (!allowVariadic && isVariadic) throw TypeCheckerException();
				hasNonVariadic = !isVariadic;
				hasVariadic = isVariadic;
			}
			return;
		case symbol_component::SymbolKind::FunctionSymbol:
			{
				auto funcDecl = symbol->GetAnyForwardDecl<ForwardFunctionDeclaration>();
				auto& evTypes = EvaluateFuncSymbol(pa, funcDecl.Obj(), parentDeclType, nullptr);
				bool isStaticSymbol = IsStaticSymbol<ForwardFunctionDeclaration>(symbol);
				bool isMember = classScope && !isStaticSymbol;
				if (!thisItem && isMember)
				{
					return;
				}

				for (vint k = 0; k < evTypes.Count(); k++)
				{
					auto tsys = evTypes[k];

					if (tsys->GetType() == TsysType::GenericFunction)
					{
						auto elementTsys = tsys->GetElement();
						if (isMember && visitMemberKind == VisitMemberKind::MemberAfterType)
						{
							elementTsys = elementTsys->MemberOf(thisEntity)->PtrOf();
						}
						else
						{
							elementTsys = elementTsys->PtrOf();
						}

						const auto& gf = tsys->GetGenericFunction();
						TypeTsysList params;
						vint count = tsys->GetParamCount();
						for (vint l = 0; l < count; l++)
						{
							params.Add(tsys->GetParam(l));
						}
						tsys = elementTsys->GenericFunctionOf(params, gf);
					}
					else
					{
						if (isMember && visitMemberKind == VisitMemberKind::MemberAfterType)
						{
							tsys = tsys->MemberOf(thisEntity)->PtrOf();
						}
						else
						{
							tsys = tsys->PtrOf();
						}
					}

					AddInternal(result, { symbol,ExprTsysType::PRValue,tsys });
				}
				hasNonVariadic = true;
			}
			return;
		case symbol_component::SymbolKind::EnumItem:
			{
				auto tsys = pa.tsys->DeclOf(symbol->GetParentScope());
				AddInternal(result, { symbol,ExprTsysType::PRValue,tsys });
				hasNonVariadic = true;
			}
			return;
		case symbol_component::SymbolKind::ValueAlias:
			{
				auto usingDecl = symbol->GetImplDecl_NFb<ValueAliasDeclaration>();
				auto& evTypes = EvaluateValueAliasSymbol(pa, usingDecl.Obj(), parentDeclType, nullptr);
				AddTempValue(result, evTypes);
				hasNonVariadic = true;
			}
			return;
		case symbol_component::SymbolKind::GenericValueArgument:
			{
				Ptr<Type> typeOfArgument;
				{
					auto ga = GetTemplateArgumentKey(symbol)->GetGenericArg();
					typeOfArgument = ga.spec->arguments[ga.argIndex].type;
				}

				TypeTsysList tsysOfArgument;
				bool tsysOfArgumentInitialized = false;

				auto evaluateValueArgument = [&]()->TypeTsysList&
				{
					if (!tsysOfArgumentInitialized)
					{
						tsysOfArgumentInitialized = true;
						TypeToTsysNoVta(pa, typeOfArgument, tsysOfArgument);
					}
					return tsysOfArgument;
				};

				if (symbol->ellipsis)
				{
					if (!allowVariadic)
					{
						throw TypeCheckerException();
					}
					hasVariadic = true;

					auto argumentKey = GetTemplateArgumentKey(symbol);
					ITsys* replacedType = nullptr;
					if (pa.TryGetReplacedGenericArg(argumentKey, replacedType))
					{
						if (!replacedType)
						{
							throw TypeCheckerException();
						}

						switch (replacedType->GetType())
						{
						case TsysType::Init:
							{
								TypeTsysList tsys;
								tsys.SetLessMemoryMode(false);

								Array<ExprTsysList> initArgs(replacedType->GetParamCount());
								for (vint j = 0; j < initArgs.Count(); j++)
								{
									AddTempValue(initArgs[j], evaluateValueArgument());
								}
								CreateUniversalInitializerType(pa, initArgs, result);
							}
							break;
						case TsysType::Any:
							AddType(result, pa.tsys->Any());
							break;
						default:
							throw TypeCheckerException();
						}
					}
					else
					{
						AddType(result, pa.tsys->Any());
					}
				}
				else
				{
					AddTempValue(result, evaluateValueArgument());
					hasNonVariadic = true;
				}
			}
			return;
		}
		throw IllegalExprException();
	}

	void VisitSymbolInternal(const ParsingArguments& pa, const ExprTsysItem* thisItem, ResolvedItem item, VisitMemberKind visitMemberKind, ExprTsysList& result, bool allowVariadic, bool& hasVariadic, bool& hasNonVariadic)
	{
		switch (item.symbol->GetParentScope()->kind)
		{
		case CLASS_SYMBOL_KIND:
			break;
		default:
			thisItem = nullptr;
		}

		if (thisItem)
		{
			auto adjustedThisItem = AdjustThisItemForSymbol(pa, *thisItem, item);

			VisitSymbolInternalWithCorrectThisItem(
				pa,
				adjustedThisItem.tsys ? &adjustedThisItem : nullptr,
				item.symbol,
				visitMemberKind,
				result,
				allowVariadic,
				hasVariadic,
				hasNonVariadic
			);
		}
		else
		{
			VisitSymbolInternalWithCorrectThisItem(
				pa,
				nullptr,
				item.symbol,
				visitMemberKind,
				result,
				allowVariadic,
				hasVariadic,
				hasNonVariadic
			);
		}
	}

	void VisitSymbol(const ParsingArguments& pa, ResolvedItem item, ExprTsysList& result)
	{
		bool hasVariadic = false;
		bool hasNonVariadic = false;
		VisitSymbolInternal(pa, nullptr, item, VisitMemberKind::InScope, result, false, hasVariadic, hasNonVariadic);
	}

	void VisitSymbolForScope(const ParsingArguments& pa, const ExprTsysItem* thisItem, ResolvedItem item, ExprTsysList& result)
	{
		bool hasVariadic = false;
		bool hasNonVariadic = false;
		VisitSymbolInternal(pa, thisItem, item, VisitMemberKind::MemberAfterType, result, false, hasVariadic, hasNonVariadic);
	}

	void VisitSymbolForField(const ParsingArguments& pa, const ExprTsysItem* thisItem, ResolvedItem item, ExprTsysList& result)
	{
		bool hasVariadic = false;
		bool hasNonVariadic = false;
		VisitSymbolInternal(pa, thisItem, item, VisitMemberKind::MemberAfterValue, result, false, hasVariadic, hasNonVariadic);
	}

	/***********************************************************************
	FindMembersByName: Fill all members of a name to ExprTsysList
	***********************************************************************/

	Ptr<Resolving> FindMembersByName(const ParsingArguments& pa, CppName& name, ResolveSymbolResult* totalRar, const ExprTsysItem& parentItem)
	{
		TsysCV cv;
		TsysRefType refType;
		auto entity = parentItem.tsys->GetEntity(cv, refType);
		auto rar = ResolveChildSymbol(pa, entity, name);
		if (totalRar) totalRar->Merge(rar);
		return rar.values;
	}

	/***********************************************************************
	VisitResolvedMember: Fill all resolved member symbol to ExprTsysList
	***********************************************************************/

	void VisitResolvedMemberInternal(const ParsingArguments& pa, const ExprTsysItem* thisItem, Ptr<Resolving> resolving, ExprTsysList& result, bool allowVariadic, bool& hasVariadic, bool& hasNonVariadic)
	{
		ExprTsysList varTypes, funcTypes;
		for (vint i = 0; i < resolving->items.Count(); i++)
		{
			auto targetTypeList = &result;
			auto ritem = resolving->items[i];

			bool parentScopeIsClass = false;
			if (auto parent = ritem.symbol->GetParentScope())
			{
				switch (parent->kind)
				{
				case CLASS_SYMBOL_KIND:
					parentScopeIsClass = true;
					switch (ritem.symbol->kind)
					{
					case symbol_component::SymbolKind::Variable:
						if (!IsStaticSymbol<ForwardVariableDeclaration>(ritem.symbol))
						{
							targetTypeList = &varTypes;
						}
						break;
					case symbol_component::SymbolKind::FunctionSymbol:
						if (!IsStaticSymbol<ForwardFunctionDeclaration>(ritem.symbol))
						{
							targetTypeList = &funcTypes;
						}
						break;
					}
					break;
				}
			}

			auto thisItemForSymbol = parentScopeIsClass ? thisItem : nullptr;
			VisitSymbolInternal(pa, thisItemForSymbol, ritem, (thisItemForSymbol ? VisitMemberKind::MemberAfterValue : VisitMemberKind::InScope), *targetTypeList, allowVariadic, hasVariadic, hasNonVariadic);
		}

		if (varTypes.Count() > 0)
		{
			// varTypes has non-static member fields
			AddInternal(result, varTypes);
		}

		if (funcTypes.Count() > 0)
		{
			if (thisItem)
			{
				// funcTypes has non-static member functions
				TsysCV thisCv;
				TsysRefType thisRef;
				thisItem->tsys->GetEntity(thisCv, thisRef);
				FilterFieldsAndBestQualifiedFunctions(thisCv, thisRef, funcTypes);
				AddInternal(result, funcTypes);
			}
			else
			{
				AddInternal(result, funcTypes);
			}
		}
	}

	void VisitResolvedMember(const ParsingArguments& pa, const ExprTsysItem* thisItem, Ptr<Resolving> resolving, ExprTsysList& result, bool& hasVariadic, bool& hasNonVariadic)
	{
		VisitResolvedMemberInternal(pa, thisItem, resolving, result, true, hasVariadic, hasNonVariadic);
	}

	void VisitResolvedMember(const ParsingArguments& pa, const ExprTsysItem* thisItem, Ptr<Resolving> resolving, ExprTsysList& result)
	{
		bool hasVariadic = false;
		bool hasNonVariadic = false;
		VisitResolvedMemberInternal(pa, thisItem, resolving, result, false, hasVariadic, hasNonVariadic);
	}

	/***********************************************************************
	VisitFunctors: Find qualified functors (including functions and operator())
	***********************************************************************/

	void VisitFunctors(const ParsingArguments& pa, const ExprTsysItem& parentItem, const WString& name, ExprTsysList& result)
	{
		TsysCV cv;
		TsysRefType refType;
		parentItem.tsys->GetEntity(cv, refType);

		CppName opName;
		opName.name = name;
		if (auto resolving = FindMembersByName(pa, opName, nullptr, parentItem))
		{
			for (vint j = 0; j < resolving->items.Count(); j++)
			{
				auto ritem = resolving->items[j];
				VisitSymbolForField(pa, &parentItem, ritem, result);
			}
			FindQualifiedFunctors(pa, cv, refType, result, false);
		}
	}
}