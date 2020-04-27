#include "Ast_Evaluate_ExpandPotentialVta.h"

using namespace symbol_type_resolving;
using namespace symbol_totsys_impl;

/***********************************************************************
ExprToTsys
	PlaceholderExpr				: literal		*
	LiteralExpr					: literal
	ThisExpr					: literal
	NullptrExpr					: literal
	ParenthesisExpr				: unbounded
	CastExpr					: unbounded
	TypeidExpr					: unbounded
	SizeofExpr					: unbounded		*
	ThrowExpr					: literal
	DeleteExpr					: unbounded		*
	IdExpr						: identifier
	ChildExpr					: unbounded
	FieldAccessExpr				: unbounded
	ArrayAccessExpr				: unbounded
	FuncAccessExpr				: variant		*
	CtorAccessExpr				: variant
	NewExpr						: variant
	UniversalInitializerExpr	: variant
	PostfixUnaryExpr			: unbounded
	PrefixUnaryExpr				: unbounded
	BinaryExpr					: unbounded
	IfExpr						: unbounded
	GenericExpr					: variant
	BuiltinFuncAccessExpr		: variant		*
***********************************************************************/

class ExprToTsysVisitor : public Object, public virtual IExprVisitor
{
public:
	ExprTsysList&				result;
	bool						isVta = false;

	const ParsingArguments&		pa;

	ExprToTsysVisitor(const ParsingArguments& _pa, ExprTsysList& _result)
		:pa(_pa)
		, result(_result)
	{
	}

	//////////////////////////////////////////////////////////////////////////////////////
	// PlaceholderExpr
	//////////////////////////////////////////////////////////////////////////////////////

	void Visit(PlaceholderExpr* self)override
	{
		AddInternal(result, *self->types);
	}

	//////////////////////////////////////////////////////////////////////////////////////
	// LiteralExpr
	//////////////////////////////////////////////////////////////////////////////////////

	void Visit(LiteralExpr* self)override
	{
		ProcessLiteralExpr(pa, result, self);
	}

	//////////////////////////////////////////////////////////////////////////////////////
	// ThisExpr
	//////////////////////////////////////////////////////////////////////////////////////

	void Visit(ThisExpr* self)override
	{
		ProcessThisExpr(pa, result, self);
	}

	//////////////////////////////////////////////////////////////////////////////////////
	// NullptrExpr
	//////////////////////////////////////////////////////////////////////////////////////

	void Visit(NullptrExpr* self)override
	{
		ProcessNullptrExpr(pa, result, self);
	}

	//////////////////////////////////////////////////////////////////////////////////////
	// ParenthesisExpr
	//////////////////////////////////////////////////////////////////////////////////////

	void Visit(ParenthesisExpr* self)override
	{
		ExprTsysList types;
		bool typesVta = false;
		ExprToTsysInternal(pa, self->expr, types, typesVta);
		isVta = ExpandPotentialVtaMultiResult(pa, result, [this, self](ExprTsysList& processResult, ExprTsysItem arg1)
		{
			ProcessParenthesisExpr(pa, processResult, self, arg1);
		}, Input(types, typesVta));
	}

	//////////////////////////////////////////////////////////////////////////////////////
	// CastExpr
	//////////////////////////////////////////////////////////////////////////////////////

	void Visit(CastExpr* self)override
	{
		TypeTsysList types;
		bool typesVta = false;
		TypeToTsysInternal(pa, self->type, types, typesVta);

		ExprTsysList exprTypes;
		bool exprTypesVta = false;
		ExprToTsysInternal(pa, self->expr, exprTypes, exprTypesVta);

		isVta = ExpandPotentialVtaMultiResult(pa, result, [this, self](ExprTsysList& processResult, ExprTsysItem argType, ExprTsysItem argExpr)
		{
			ProcessCastExpr(pa, processResult, self, argType, argExpr);
		}, Input(types, typesVta), Input(exprTypes, exprTypesVta));
	}

	//////////////////////////////////////////////////////////////////////////////////////
	// TypeidExpr
	//////////////////////////////////////////////////////////////////////////////////////

	void Visit(TypeidExpr* self)override
	{
		ExprTsysList types;
		bool typesVta = false;

		if (self->type)
		{
			TypeTsysList tsyses;
			TypeToTsysInternal(pa, self->type, tsyses, typesVta);
			AddType(types, tsyses);
		}
		if (self->expr)
		{
			ExprToTsysInternal(pa, self->expr, types, typesVta);
		}

		isVta = ExpandPotentialVtaMultiResult(pa, result, [this, self](ExprTsysList& processResult, ExprTsysItem arg)
		{
			ProcessTypeidExpr(pa, processResult, self);
		}, Input(types, typesVta));
	}

	//////////////////////////////////////////////////////////////////////////////////////
	// SizeofExpr
	//////////////////////////////////////////////////////////////////////////////////////

	void Visit(SizeofExpr* self)override
	{
		ExprTsysList types;
		bool typesVta = false;

		if (self->type)
		{
			TypeTsysList tsyses;
			TypeToTsysInternal(pa, self->type, tsyses, typesVta);
			AddType(types, tsyses);
		}
		if (self->expr)
		{
			ExprToTsysInternal(pa, self->expr, types, typesVta);
		}

		if (self->ellipsis)
		{
			AddType(result, pa.tsys->Size());
		}
		else
		{
			isVta = ExpandPotentialVtaMultiResult(pa, result, [this](ExprTsysList& processResult, ExprTsysItem arg)
			{
				AddType(processResult, pa.tsys->Size());
			}, Input(types, typesVta));
		}
	}

	//////////////////////////////////////////////////////////////////////////////////////
	// ThrowExpr
	//////////////////////////////////////////////////////////////////////////////////////

	void Visit(ThrowExpr* self)override
	{
		ProcessThrowExpr(pa, result, self);
	}

	//////////////////////////////////////////////////////////////////////////////////////
	// DeleteExpr
	//////////////////////////////////////////////////////////////////////////////////////

	void Visit(DeleteExpr* self)override
	{
		ExprTsysList types;
		bool typesVta = false;
		ExprToTsysInternal(pa, self->expr, types, typesVta);
		isVta = ExpandPotentialVtaMultiResult(pa, result, [this](ExprTsysList& processResult, ExprTsysItem arg)
		{
			AddType(processResult, pa.tsys->Void());
		}, Input(types, typesVta));
	}

	//////////////////////////////////////////////////////////////////////////////////////
	// IdExpr
	//////////////////////////////////////////////////////////////////////////////////////

	void Visit(IdExpr* self)override
	{
		CreateIdReferenceExpr(pa, self->resolving, result, nullptr, false, true, isVta);
	}

	//////////////////////////////////////////////////////////////////////////////////////
	// ChildExpr
	//////////////////////////////////////////////////////////////////////////////////////

	void Visit(ChildExpr* self)override
	{
		if (!IsResolvedToType(self->classType))
		{
			bool childIsVta = false;
			CreateIdReferenceExpr(pa, self->resolving, result, nullptr, true, false, childIsVta);
			return;
		}

		TypeTsysList classTypes;
		bool classVta = false;
		TypeToTsysInternal(pa, self->classType, classTypes, classVta);

		isVta = ExpandPotentialVtaMultiResult(pa, result, [this, self](ExprTsysList& processResult, ExprTsysItem argClass)
		{
			if (argClass.tsys->IsUnknownType())
			{
				AddType(processResult, pa.tsys->Any());
			}
			else
			{
				ProcessChildExpr(pa, processResult, self, argClass);
			}
		}, Input(classTypes, classVta));
	}

	//////////////////////////////////////////////////////////////////////////////////////
	// FieldAccessExpr
	//////////////////////////////////////////////////////////////////////////////////////

	void Visit(FieldAccessExpr* self)override
	{
		ExprTsysList parentTypes;
		bool parentVta = false;
		ExprToTsysInternal(pa, self->expr, parentTypes, parentVta);

		Ptr<IdExpr> idExpr;
		Ptr<ChildExpr> childExpr;
		Ptr<GenericExpr> genericExpr;
		MatchCategoryExpr(
			self->name,
			[&idExpr](const Ptr<IdExpr>& _idExpr)
			{
				idExpr = _idExpr;
			},
			[&childExpr](const Ptr<ChildExpr>& _childExpr)
			{
				childExpr = _childExpr;
			},
			[&idExpr, &childExpr, &genericExpr](const Ptr<GenericExpr>& _genericExpr)
			{
				genericExpr = _genericExpr;
				MatchCategoryExpr(
					genericExpr->expr,
					[&idExpr](const Ptr<IdExpr>& _idExpr)
					{
						idExpr = _idExpr;
					},
					[&childExpr](const Ptr<ChildExpr>& _childExpr)
					{
						childExpr = _childExpr;
					}
				);
			}
		);

		ExprTsysList nonGenericResult;
		bool nonGenericIsVta = false;

		ResolveSymbolResult totalRar;
		bool operatorIndexed = false;
		if (idExpr)
		{
			nonGenericIsVta = ExpandPotentialVtaMultiResult(pa, nonGenericResult, [this, self, idExpr, &totalRar, &operatorIndexed](ExprTsysList& processResult, ExprTsysItem argParent)
			{
				ProcessFieldAccessExprForIdExpr(pa, processResult, self, argParent, idExpr, totalRar, operatorIndexed);
			}, Input(parentTypes, parentVta));
		}
		else
		{
			TypeTsysList classTypes;
			bool classVta = false;
			TypeToTsysInternal(pa, childExpr->classType, classTypes, classVta);

			nonGenericIsVta = ExpandPotentialVtaMultiResult(pa, nonGenericResult, [this, self, idExpr, childExpr, &totalRar, &operatorIndexed](ExprTsysList& processResult, ExprTsysItem argParent, ExprTsysItem argClass)
			{
				ProcessFieldAccessExprForChildExpr(pa, processResult, self, argParent, argClass, childExpr, totalRar, operatorIndexed);
			}, Input(parentTypes, parentVta), Input(classTypes, classVta));
		}

		if (genericExpr)
		{
			GenericExprToTsys(pa, genericExpr.Obj(), result, isVta,
				[&nonGenericResult, nonGenericIsVta](VariadicInput<ExprTsysItem>& variadicInput)
				{
					variadicInput.ApplyTypes(0, nonGenericResult, nonGenericIsVta);
				});
		}
		else
		{
			CopyFrom(result, nonGenericResult, true);
			isVta = nonGenericIsVta;
		}

		if (idExpr && pa.IsGeneralEvaluation())
		{
			idExpr->resolving = totalRar.values;
			if (pa.recorder)
			{
				if (totalRar.values)
				{
					pa.recorder->Index(idExpr->name, totalRar.values->resolvedSymbols);
				}
				if (totalRar.types)
				{
					pa.recorder->ExpectValueButType(idExpr->name, totalRar.types->resolvedSymbols);
				}
			}
		}

		if (operatorIndexed)
		{
			pa.recorder->IndexOverloadingResolution(self->opName, self->opResolving->resolvedSymbols);
		}
	}

	//////////////////////////////////////////////////////////////////////////////////////
	// ArrayAccessExpr
	//////////////////////////////////////////////////////////////////////////////////////

	void Visit(ArrayAccessExpr* self)override
	{
		ExprTsysList arrayTypes, indexTypes;
		bool arrayVta = false;
		bool indexVta = false;
		ExprToTsysInternal(pa, self->expr, arrayTypes, arrayVta);
		ExprToTsysInternal(pa, self->index, indexTypes, indexVta);

		bool indexed = false;
		isVta = ExpandPotentialVtaMultiResult(pa, result, [this, self, &indexed](ExprTsysList& processResult, ExprTsysItem argArray, ExprTsysItem argIndex)
		{
			ProcessArrayAccessExpr(pa, processResult, self, argArray, argIndex, indexed);
		}, Input(arrayTypes, arrayVta), Input(indexTypes, indexVta));

		if (indexed)
		{
			pa.recorder->IndexOverloadingResolution(self->opName, self->opResolving->resolvedSymbols);
		}
	}

	//////////////////////////////////////////////////////////////////////////////////////
	// FuncAccessExpr
	//////////////////////////////////////////////////////////////////////////////////////

	void Visit(FuncAccessExpr* self)override
	{
		Array<ExprTsysList> argTypesList(self->arguments.Count());
		Array<bool> isVtas(self->arguments.Count());
		for (vint i = 0; i < self->arguments.Count(); i++)
		{
			ExprToTsysInternal(pa, self->arguments[i].item, argTypesList[i], isVtas[i]);
		}

		bool argHasBoundedVta = false;
		vint argUnboundedVtaCount = -1;
		isVta = CheckVta(self->arguments, argTypesList, isVtas, 0, argHasBoundedVta, argUnboundedVtaCount);

		ExprTsysList funcExprTypes;
		bool funcVta = false;
		vint funcVtaCount = -1;
		ExprToTsysInternal(pa, self->expr, funcExprTypes, funcVta);

		if (funcVta)
		{
			if (argHasBoundedVta)
			{
				throw TypeCheckerException();
			}
			isVta = true;

			for (vint i = 0; i < funcExprTypes.Count(); i++)
			{
				auto funcTsys = funcExprTypes[i].tsys;
				if (funcTsys->GetType() == TsysType::Init)
				{
					if (funcVtaCount == -1)
					{
						funcVtaCount = funcTsys->GetParamCount();
					}
					else if (funcVtaCount != funcTsys->GetParamCount())
					{
						throw TypeCheckerException();
					}

					if (argUnboundedVtaCount == -1)
					{
						argUnboundedVtaCount = funcVtaCount;
					}
					if (argUnboundedVtaCount != funcVtaCount)
					{
						throw TypeCheckerException();
					}
				}
			}
		}

		ExprTsysList totalSelectedFunctions;
		ExpandPotentialVtaList(pa, result, argTypesList, isVtas, argHasBoundedVta, argUnboundedVtaCount,
			[this, self, funcVta, &funcExprTypes, &totalSelectedFunctions](ExprTsysList& processResult, Array<ExprTsysItem>& args, vint unboundedVtaIndex, Array<vint>& argSource, SortedList<vint>& boundedAnys)
			{
				ExprTsysList funcTypes;
				if (funcVta && unboundedVtaIndex != -1)
				{
					for (vint i = 0; i < funcExprTypes.Count(); i++)
					{
						auto funcItem = funcExprTypes[i];
						if (funcItem.tsys->GetType() == TsysType::Init)
						{
							funcTypes.Add({ funcItem.tsys->GetInit().headers[unboundedVtaIndex],funcItem.tsys->GetParam(unboundedVtaIndex) });
						}
						else
						{
							funcTypes.Add(funcItem);
						}
					}
				}
				else
				{
					CopyFrom(funcTypes, funcExprTypes);
				}

				if (auto idExpr = self->expr.Cast<IdExpr>())
				{
					if (!idExpr->resolving || IsAdlEnabled(pa, idExpr->resolving))
					{
						SortedList<Symbol*> nss;
						for (vint i = 0; i < args.Count(); i++)
						{
							SearchAdlClassesAndNamespaces(pa, args[i].tsys, nss);
						}
						SearchAdlFunction(pa, nss, idExpr->name.name, funcTypes);
					}
				}

				ExprTsysList selectedFunctions;
				FindQualifiedFunctors(pa, {}, TsysRefType::None, funcTypes, true);
				VisitOverloadedFunction(pa, funcTypes, args, boundedAnys, processResult, (pa.recorder ? &selectedFunctions : nullptr));
				AddInternal(totalSelectedFunctions, selectedFunctions);
			});

		if (pa.recorder && pa.IsGeneralEvaluation())
		{
			CppName* name = nullptr;
			Ptr<Resolving>* nameResolving = nullptr;

			Ptr<Category_Id_Child_Generic_Expr> catIcgExpr;
			if (auto fieldExpr = self->expr.Cast<FieldAccessExpr>())
			{
				catIcgExpr = fieldExpr->name;
			}
			else
			{
				catIcgExpr = self->expr.Cast<Category_Id_Child_Generic_Expr>();
			}

			if(catIcgExpr)
			{
				auto processId = [&](const Ptr<IdExpr>& idExpr)
				{
					name = &idExpr->name;
					nameResolving = &idExpr->resolving;
				};

				auto processChild = [&](const Ptr<ChildExpr>& childExpr)
				{
					name = &childExpr->name;
					nameResolving = &childExpr->resolving;
				};

				MatchCategoryExpr(
					catIcgExpr,
					processId,
					processChild,
					[&](const Ptr<GenericExpr>& genericExpr)
					{
						MatchCategoryExpr(
							genericExpr->expr,
							processId,
							processChild
						);
					}
				);
			}

			bool addedName = false;
			bool addedOp = false;
			AddSymbolsToResolvings(pa, name, nameResolving, &self->opName, &self->opResolving, totalSelectedFunctions, addedName, addedOp);
			if (addedName)
			{
				pa.recorder->IndexOverloadingResolution(*name, (*nameResolving)->resolvedSymbols);
			}
			if (addedOp)
			{
				pa.recorder->IndexOverloadingResolution(self->opName, self->opResolving->resolvedSymbols);
			}
		}
	}

	//////////////////////////////////////////////////////////////////////////////////////
	// CtorAccessExpr
	//////////////////////////////////////////////////////////////////////////////////////

	void Visit(CtorAccessExpr* self)override
	{
		VariadicInput<ExprTsysItem> variadicInput(self->initializer ? self->initializer->arguments.Count() + 1 : 1, pa);
		variadicInput.ApplySingle(0, self->type);
		if (self->initializer)
		{
			variadicInput.ApplyVariadicList(1, self->initializer->arguments);
		}
		isVta = variadicInput.Expand((self->initializer ? &self->initializer->arguments : nullptr), result,
			[this, self](ExprTsysList& processResult, Array<ExprTsysItem>& args, vint unboundedVtaIndex, Array<vint>& argSource, SortedList<vint>& boundedAnys)
			{
				ProcessCtorAccessExpr(pa, processResult, self, args, boundedAnys);
			});
	}

	//////////////////////////////////////////////////////////////////////////////////////
	// NewExpr
	//////////////////////////////////////////////////////////////////////////////////////

	void Visit(NewExpr* self)override
	{
		// TODO: [Cpp.md] Enable variadic template argument in placement arguments
		for (vint i = 0; i < self->placementArguments.Count(); i++)
		{
			if (self->placementArguments[i].isVariadic)
			{
				throw TypeCheckerException();
			}
			ExprTsysList types;
			ExprToTsysNoVta(pa, self->placementArguments[i].item, types);
		}

		VariadicInput<ExprTsysItem> variadicInput(self->initializer ? self->initializer->arguments.Count() + 1 : 1, pa);
		variadicInput.ApplySingle(0, self->type);
		if (self->initializer)
		{
			variadicInput.ApplyVariadicList(1, self->initializer->arguments);
		}
		isVta = variadicInput.Expand((self->initializer ? &self->initializer->arguments : nullptr), result,
			[this, self](ExprTsysList& processResult, Array<ExprTsysItem>& args, vint unboundedVtaIndex, Array<vint>& argSource, SortedList<vint>& boundedAnys)
			{
				ProcessNewExpr(pa, processResult, self, args, boundedAnys);
			});
	}

	//////////////////////////////////////////////////////////////////////////////////////
	// UniversalInitializerExpr
	//////////////////////////////////////////////////////////////////////////////////////

	void Visit(UniversalInitializerExpr* self)override
	{
		VariadicInput<ExprTsysItem> variadicInput(self->arguments.Count(), pa);
		variadicInput.ApplyVariadicList(0, self->arguments);
		isVta = variadicInput.Expand(&self->arguments, result,
			[this, self](ExprTsysList& processResult, Array<ExprTsysItem>& args, vint unboundedVtaIndex, Array<vint>& argSource, SortedList<vint>& boundedAnys)
			{
				ProcessUniversalInitializerExpr(pa, processResult, self, args, boundedAnys);
			});
	}

	//////////////////////////////////////////////////////////////////////////////////////
	// PostfixUnaryExpr
	//////////////////////////////////////////////////////////////////////////////////////

	void Visit(PostfixUnaryExpr* self)override
	{
		ExprTsysList types;
		bool typesVta = false;
		ExprToTsysInternal(pa, self->operand, types, typesVta);

		bool indexed = false;
		isVta = ExpandPotentialVtaMultiResult(pa, result, [this, self, &indexed](ExprTsysList& processResult, ExprTsysItem arg1)
		{
			ProcessPostfixUnaryExpr(pa, processResult, self, arg1, indexed);
		}, Input(types, typesVta));

		if (indexed)
		{
			pa.recorder->IndexOverloadingResolution(self->opName, self->opResolving->resolvedSymbols);
		}
	}

	//////////////////////////////////////////////////////////////////////////////////////
	// PrefixUnaryExpr
	//////////////////////////////////////////////////////////////////////////////////////

	static bool AddressOfChildExpr(const ParsingArguments& pa, ChildExpr* childExpr, ExprTsysList& tsys, bool& isVta)
	{
		if (IsResolvedToType(childExpr->classType))
		{
			TypeTsysList classTypes;
			TypeToTsysInternal(pa, childExpr->classType, classTypes, isVta);
			ExpandPotentialVtaMultiResult(pa, tsys, [&pa, childExpr](ExprTsysList& processResult, ExprTsysItem arg1)
			{
				if (arg1.tsys->IsUnknownType())
				{
					AddType(processResult, pa.tsys->Any());
				}
				else
				{
					auto rsr = ResolveChildSymbol(pa, arg1.tsys, childExpr->name);
					if (rsr.values)
					{
						auto newPa = pa.WithScope(arg1.tsys->GetDecl());
						for (vint i = 0; i < rsr.values->resolvedSymbols.Count(); i++)
						{
							auto symbol = rsr.values->resolvedSymbols[i];
							auto adjusted = AdjustThisItemForSymbol(newPa, arg1, symbol).Value();
							VisitSymbolForScope(newPa, &adjusted, symbol, processResult);
						}
					}
				}
			}, Input(classTypes, isVta));
			return true;
		}
		return false;
	}

	void Visit(PrefixUnaryExpr* self)override
	{
		ExprTsysList types;
		bool typesVta = false;
		bool skipEvaluatingOperand = false;
		if (self->op == CppPrefixUnaryOp::AddressOf)
		{
			if (auto catIcgExpr = self->operand.Cast<Category_Id_Child_Generic_Expr>())
			{
				MatchCategoryExpr(
					catIcgExpr,
					[](const Ptr<IdExpr>& idExpr)
					{
					},
					[this, &types, &typesVta, &skipEvaluatingOperand](const Ptr<ChildExpr>& childExpr)
					{
						skipEvaluatingOperand = AddressOfChildExpr(pa, childExpr.Obj(), types, typesVta);
					},
					[this, &types, &typesVta, &skipEvaluatingOperand](const Ptr<GenericExpr>& genericExpr)
					{
						MatchCategoryExpr(
							genericExpr->expr,
							[&](const Ptr<IdExpr>& idExpr)
							{
							},
							[this, &genericExpr, &types, &typesVta, &skipEvaluatingOperand](const Ptr<ChildExpr>& childExpr)
							{
								ExprTsysList classTypes;
								bool classIsVta = false;
								if (AddressOfChildExpr(pa, childExpr.Obj(), classTypes, classIsVta))
								{
									GenericExprToTsys(pa, genericExpr.Obj(), types, typesVta,
										[&classTypes, classIsVta](VariadicInput<ExprTsysItem>& variadicInput)
										{
											variadicInput.ApplyTypes(0, classTypes, classIsVta);
										});
									skipEvaluatingOperand = true;
								}
							}
						);
					}
				);
			}
		}

		if (!skipEvaluatingOperand)
		{
			ExprToTsysInternal(pa, self->operand, types, typesVta);
		}
		bool indexed = false;

		isVta = ExpandPotentialVtaMultiResult(pa, result, [this, self, &indexed](ExprTsysList& processResult, ExprTsysItem arg1)
		{
			ProcessPrefixUnaryExpr(pa, processResult, self, arg1, indexed);
		}, Input(types, typesVta));

		if (indexed)
		{
			pa.recorder->IndexOverloadingResolution(self->opName, self->opResolving->resolvedSymbols);
		}
	}

	//////////////////////////////////////////////////////////////////////////////////////
	// BinaryExpr
	//////////////////////////////////////////////////////////////////////////////////////

	void Visit(BinaryExpr* self)override
	{
		ExprTsysList leftTypes, rightTypes;
		bool leftVta = false;
		bool rightVta = false;
		ExprToTsysInternal(pa, self->left, leftTypes, leftVta);
		ExprToTsysInternal(pa, self->right, rightTypes, rightVta);
		bool indexed = false;

		isVta = ExpandPotentialVtaMultiResult(pa, result, [this, self, &indexed](ExprTsysList& processResult, ExprTsysItem argLeft, ExprTsysItem argRight)
		{
			ProcessBinaryExpr(pa, processResult, self, argLeft, argRight, indexed);
		}, Input(leftTypes, leftVta), Input(rightTypes, rightVta));

		if (indexed)
		{
			pa.recorder->IndexOverloadingResolution(self->opName, self->opResolving->resolvedSymbols);
		}
	}

	//////////////////////////////////////////////////////////////////////////////////////
	// IfExpr
	//////////////////////////////////////////////////////////////////////////////////////

	void Visit(IfExpr* self)override
	{
		ExprTsysList conditionTypes, leftTypes, rightTypes;
		bool conditionVta = false;
		bool leftVta = false;
		bool rightVta = false;
		ExprToTsysInternal(pa, self->condition, conditionTypes, conditionVta);
		ExprToTsysInternal(pa, self->left, leftTypes, leftVta);
		ExprToTsysInternal(pa, self->right, rightTypes, rightVta);

		isVta = ExpandPotentialVtaMultiResult(pa, result, [this, self](ExprTsysList& processResult, ExprTsysItem argCond, ExprTsysItem argLeft, ExprTsysItem argRight)
		{
			ProcessIfExpr(pa, processResult, self, argCond, argLeft, argRight);
		}, Input(conditionTypes, conditionVta), Input(leftTypes, leftVta), Input(rightTypes, rightVta));
	}

	//////////////////////////////////////////////////////////////////////////////////////
	// GenericExpr
	//////////////////////////////////////////////////////////////////////////////////////

	template<typename TAddTypes>
	static void GenericExprToTsys(const ParsingArguments& pa, GenericExpr* argumentsPart, ExprTsysList& tsys, bool& isVta, TAddTypes&& addTypes)
	{
		vint count = argumentsPart->arguments.Count() + 1;
		Array<bool> isTypes(count);
		isTypes[0] = false;

		VariadicInput<ExprTsysItem> variadicInput(count, pa);
		addTypes(variadicInput);
		variadicInput.ApplyGenericArguments(1, isTypes, argumentsPart->arguments);
		isVta = variadicInput.Expand(&argumentsPart->arguments, tsys,
			[&pa, argumentsPart, &isTypes](ExprTsysList& processResult, Array<ExprTsysItem>& args, vint unboundedVtaIndex, Array<vint>& argSource, SortedList<vint>& boundedAnys)
			{
				ProcessGenericExpr(pa, processResult, argumentsPart, isTypes, args, argSource, boundedAnys);
			});
	}

	void Visit(GenericExpr* self)override
	{
		GenericExprToTsys(pa, self, result, isVta,
			[self](VariadicInput<ExprTsysItem>& variadicInput)
			{
				variadicInput.ApplySingle<Expr>(0, self->expr);
			});
	}

	//////////////////////////////////////////////////////////////////////////////////////
	// BuiltinFuncAccessExpr
	//////////////////////////////////////////////////////////////////////////////////////

	void Visit(BuiltinFuncAccessExpr* self)override
	{
		isVta = false;
		for (vint i = 0; i < self->arguments.Count(); i++)
		{
			auto argument = self->arguments[i];
			bool tsysVta = false;
			if (argument.item.type)
			{
				TypeTsysList tsys;
				TypeToTsysInternal(pa, argument.item.type, tsys, tsysVta);
			}
			else
			{
				ExprTsysList tsys;
				ExprToTsysInternal(pa, argument.item.expr, tsys, tsysVta);
			}

			if (!argument.isVariadic)
			{
				isVta |= tsysVta;
			}
		}

		TypeTsysList tsys;
		TypeToTsysNoVta(pa, self->returnType, tsys);
		AddType(result, tsys);
	}
};

// Resolve expressions to types

void GenericExprToTsys(const ParsingArguments& pa, ExprTsysList& nameTypes, bool nameIsVta, GenericExpr* argumentsPart, ExprTsysList& tsys, bool& isVta)
{
	ExprToTsysVisitor::GenericExprToTsys(pa, argumentsPart, tsys, isVta,
		[&nameTypes, nameIsVta](VariadicInput<ExprTsysItem>& variadicInput)
		{
			variadicInput.ApplyTypes(0, nameTypes, nameIsVta);
		});
}

void ExprToTsysInternal(const ParsingArguments& pa, Ptr<Expr> e, ExprTsysList& tsys, bool& isVta)
{
	if (!e) throw IllegalExprException();
	ExprToTsysVisitor visitor(pa, tsys);
	e->Accept(&visitor);
	isVta = visitor.isVta;
}

void ExprToTsysNoVta(const ParsingArguments& pa, Ptr<Expr> e, ExprTsysList& tsys)
{
	bool isVta = false;
	ExprToTsysInternal(pa, e, tsys, isVta);
	if (isVta)
	{
		throw TypeCheckerException();
	}
}