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
		auto resolving = ResolveInCurrentContext(pa, self->name, self->resolving, [&]()
		{
			return ResolveSymbolInContext(pa, self->name, false).values;
		});

		CreateIdReferenceExpr(pa, resolving, result, nullptr, false, true, isVta);
	}

	//////////////////////////////////////////////////////////////////////////////////////
	// ChildExpr
	//////////////////////////////////////////////////////////////////////////////////////

	void Visit(ChildExpr* self)override
	{
		if (!IsResolvedToType(self->classType))
		{
			auto resolving = ResolveInCurrentContext(pa, self->name, self->resolving, [&]()
			{
				return ResolveChildSymbol(pa, self->classType, self->name).values;
			});

			bool childIsVta = false;
			CreateIdReferenceExpr(pa, resolving, result, nullptr, true, false, childIsVta);
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
		CastCategoryExpr(self->name, idExpr, childExpr, genericExpr);

		ExprTsysList nonGenericResult;
		bool nonGenericIsVta = false;

		ResolveSymbolResult totalRar;
		List<ResolvedItem> ritems;
		if (idExpr)
		{
			nonGenericIsVta = ExpandPotentialVtaMultiResult(pa, nonGenericResult, [this, self, idExpr, &totalRar, &ritems](ExprTsysList& processResult, ExprTsysItem argParent)
			{
				ProcessFieldAccessExprForIdExpr(pa, processResult, self, argParent, idExpr, totalRar, (pa.IsGeneralEvaluation() && pa.recorder ? &ritems : nullptr));
			}, Input(parentTypes, parentVta));
		}
		else
		{
			TypeTsysList classTypes;
			bool classVta = false;
			TypeToTsysInternal(pa, childExpr->classType, classTypes, classVta);

			nonGenericIsVta = ExpandPotentialVtaMultiResult(pa, nonGenericResult, [this, self, idExpr, childExpr, &totalRar, &ritems](ExprTsysList& processResult, ExprTsysItem argParent, ExprTsysItem argClass)
			{
				ProcessFieldAccessExprForChildExpr(pa, processResult, self, argParent, argClass, childExpr, totalRar, (pa.IsGeneralEvaluation() && pa.recorder ? &ritems : nullptr));
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

		if (idExpr && !idExpr->resolving && pa.IsGeneralEvaluation())
		{
			idExpr->resolving = totalRar.values;
			if (pa.recorder && totalRar.values)
			{
				pa.recorder->Index(idExpr->name, totalRar.values->items);
			}
		}

		if (ritems.Count() > 0)
		{
			pa.recorder->IndexOverloadingResolution(self->opName, ritems);
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

		List<ResolvedItem> ritems;
		isVta = ExpandPotentialVtaMultiResult(pa, result, [this, self, &ritems](ExprTsysList& processResult, ExprTsysItem argArray, ExprTsysItem argIndex)
		{
			ProcessArrayAccessExpr(pa, processResult, self, argArray, argIndex, (pa.IsGeneralEvaluation() && pa.recorder ? &ritems : nullptr));
		}, Input(arrayTypes, arrayVta), Input(indexTypes, indexVta));

		if (ritems.Count() > 0)
		{
			pa.recorder->IndexOverloadingResolution(self->opName, ritems);
		}
	}

	//////////////////////////////////////////////////////////////////////////////////////
	// FuncAccessExpr
	//////////////////////////////////////////////////////////////////////////////////////

	void VisitFuncAccessExpr(FuncAccessExpr* self, const ExprTsysList& funcExprTypes, bool funcVta)
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

		if (funcVta)
		{
			if (argHasBoundedVta)
			{
				throw TypeCheckerException();
			}
			isVta = true;

			vint funcVtaCount = -1;
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

		List<ResolvedItem> tritems;
		ExpandPotentialVtaList(pa, result, argTypesList, isVtas, argHasBoundedVta, argUnboundedVtaCount,
			[this, self, funcVta, &funcExprTypes, &tritems](ExprTsysList& processResult, Array<ExprTsysItem>& args, vint unboundedVtaIndex, Array<vint>& argSource, SortedList<vint>& boundedAnys)
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

				List<ResolvedItem> ritems;
				FindQualifiedFunctors(pa, {}, TsysRefType::None, funcTypes, true);
				VisitOverloadedFunction(pa, funcTypes, args, boundedAnys, processResult, (pa.IsGeneralEvaluation() && pa.recorder ? &ritems : nullptr));
				ResolvedItem::AddItems(tritems, ritems);
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
				GetCategoryExprResolving(catIcgExpr, name, nameResolving);
			}

			List<ResolvedItem> nritems, oritems;
			for (vint i = 0; i < tritems.Count(); i++)
			{
				auto ritem = tritems[i];
				if (ritem.symbol)
				{
					if (auto funcDecl = ritem.symbol->GetAnyForwardDecl<ForwardFunctionDeclaration>())
					{
						if (funcDecl->name.type == CppNameType::Operator)
						{
							ResolvedItem::AddItem(oritems, ritem);
						}
						else
						{
							ResolvedItem::AddItem(nritems, ritem);
						}
					}
				}
			}

			if (name && nritems.Count() > 0)
			{
				pa.recorder->IndexOverloadingResolution(*name, nritems);
			}
			if (oritems.Count() > 0)
			{
				pa.recorder->IndexOverloadingResolution(self->opName, oritems);
			}
		}
	}

	void Visit(FuncAccessExpr* self)override
	{
		// avoid recursion for chained function calls
		List<FuncAccessExpr*> exprs;
		{
			auto current = self;
			while (current)
			{
				exprs.Add(current);
				current = current->expr.Cast<FuncAccessExpr>().Obj();
			}
		}

		ExprTsysList funcExprTypes;
		bool funcVta = false;
		ExprToTsysInternal(pa, exprs[exprs.Count() - 1]->expr, funcExprTypes, funcVta);

		for (vint i = exprs.Count() - 1; i >= 0; i--)
		{
			VisitFuncAccessExpr(exprs[i], funcExprTypes, funcVta);
			if (i > 0)
			{
				CopyFrom(funcExprTypes, result);
				funcVta = isVta;

				result.Clear();
				isVta = false;
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

		List<ResolvedItem> ritems;
		isVta = ExpandPotentialVtaMultiResult(pa, result, [this, self, &ritems](ExprTsysList& processResult, ExprTsysItem arg1)
		{
			ProcessPostfixUnaryExpr(pa, processResult, self, arg1, (pa.IsGeneralEvaluation() && pa.recorder ? &ritems : nullptr));
		}, Input(types, typesVta));

		if (ritems.Count() > 0)
		{
			pa.recorder->IndexOverloadingResolution(self->opName, ritems);
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
						for (vint i = 0; i < rsr.values->items.Count(); i++)
						{
							auto ritem = rsr.values->items[i];
							auto adjusted = AdjustThisItemForSymbol(newPa, arg1, ritem);
							VisitSymbolForScope(newPa, &adjusted, ritem, processResult);
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

		List<ResolvedItem> ritems;
		isVta = ExpandPotentialVtaMultiResult(pa, result, [this, self, &ritems](ExprTsysList& processResult, ExprTsysItem arg1)
		{
			ProcessPrefixUnaryExpr(pa, processResult, self, arg1, (pa.IsGeneralEvaluation() && pa.recorder ? &ritems : nullptr));
		}, Input(types, typesVta));

		if (ritems.Count() > 0)
		{
			pa.recorder->IndexOverloadingResolution(self->opName, ritems);
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

		List<ResolvedItem> ritems;
		isVta = ExpandPotentialVtaMultiResult(pa, result, [this, self, &ritems](ExprTsysList& processResult, ExprTsysItem argLeft, ExprTsysItem argRight)
		{
			ProcessBinaryExpr(pa, processResult, self, argLeft, argRight, (pa.IsGeneralEvaluation() && pa.recorder ? &ritems : nullptr));
		}, Input(leftTypes, leftVta), Input(rightTypes, rightVta));

		if (ritems.Count() > 0)
		{
			pa.recorder->IndexOverloadingResolution(self->opName, ritems);
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
	// LambdaExpr
	//////////////////////////////////////////////////////////////////////////////////////

	void Visit(LambdaExpr* self)override
	{
		if (pa.IsGeneralEvaluation())
		{
			if (!self->statementEvaluated)
			{
				self->statementEvaluated = true;
			}

			auto newPa = pa.AdjustForDecl(self->symbol);
			for (vint i = 0; i < self->varDecls.Count(); i++)
			{
				EvaluateVariableDeclaration(newPa, self->varDecls[0].Obj());
			}
			EvaluateStat(newPa, self->statement, false, nullptr);
		}
		AddType(result, pa.tsys->Any());
	}

	//////////////////////////////////////////////////////////////////////////////////////
	// BuiltinFuncAccessExpr
	//////////////////////////////////////////////////////////////////////////////////////

	void Visit(BuiltinFuncAccessExpr* self)override
	{
		isVta = false;
		vint vtaCount = -1;
		for (vint i = 0; i < self->arguments.Count(); i++)
		{
			auto argument = self->arguments[i];
			bool tsysVta = false;
			if (argument.item.type)
			{
				TypeTsysList tsys;
				TypeToTsysInternal(pa, argument.item.type, tsys, tsysVta);

				if (tsysVta && !argument.isVariadic)
				{
					isVta = true;
					if (vtaCount == -1)
					{
						for (vint j = 0; j < tsys.Count(); j++)
						{
							auto candidate = tsys[j];
							if (candidate->GetType() == TsysType::Init)
							{
								vtaCount = candidate->GetParamCount();
								break;
							}
						}
					}
				}
			}
			else
			{
				ExprTsysList tsys;
				ExprToTsysInternal(pa, argument.item.expr, tsys, tsysVta);

				if (tsysVta && !argument.isVariadic)
				{
					isVta = true;
					if (vtaCount == -1)
					{
						for (vint j = 0; j < tsys.Count(); j++)
						{
							auto candidate = tsys[j].tsys;
							if (candidate->GetType() == TsysType::Init)
							{
								vtaCount = candidate->GetParamCount();
								break;
							}
						}
					}
				}
			}

			if (!argument.isVariadic)
			{
				isVta |= tsysVta;
			}
		}

		TypeTsysList tsys;
		TypeToTsysNoVta(pa, self->returnType, tsys);

		if (isVta)
		{
			if (vtaCount == -1)
			{
				AddType(result, pa.tsys->Any());
			}
			else
			{
				Array<ExprTsysList> params(vtaCount);
				for (vint i = 0; i < vtaCount; i++)
				{
					AddType(params[i], tsys);
				}
				CreateUniversalInitializerType(pa, params, result);
			}
		}
		else
		{
			AddType(result, tsys);
		}
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