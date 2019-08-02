#include "Ast_Resolving_ExpandPotentialVta.h"

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
	IdExpr						: *identifier
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
***********************************************************************/

class ExprToTsysVisitor : public Object, public virtual IExprVisitor
{
public:
	ExprTsysList&				result;
	bool						isVta = false;

	const ParsingArguments&		pa;
	GenericArgContext*			gaContext = nullptr;

	ExprToTsysVisitor(const ParsingArguments& _pa, ExprTsysList& _result, GenericArgContext* _gaContext)
		:pa(_pa)
		, result(_result)
		, gaContext(_gaContext)
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
		ProcessLiteralExpr(pa, result, gaContext, self);
	}

	//////////////////////////////////////////////////////////////////////////////////////
	// ThisExpr
	//////////////////////////////////////////////////////////////////////////////////////

	void Visit(ThisExpr* self)override
	{
		ProcessThisExpr(pa, result, gaContext, self);
	}

	//////////////////////////////////////////////////////////////////////////////////////
	// NullptrExpr
	//////////////////////////////////////////////////////////////////////////////////////

	void Visit(NullptrExpr* self)override
	{
		ProcessNullptrExpr(pa, result, gaContext, self);
	}

	//////////////////////////////////////////////////////////////////////////////////////
	// ParenthesisExpr
	//////////////////////////////////////////////////////////////////////////////////////

	void Visit(ParenthesisExpr* self)override
	{
		ExprTsysList types;
		bool typesVta = false;
		ExprToTsysInternal(pa, self->expr, types, typesVta, gaContext);
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
		TypeToTsysInternal(pa, self->type, types, gaContext, typesVta);

		ExprTsysList exprTypes;
		bool exprTypesVta = false;
		ExprToTsysInternal(pa, self->expr, exprTypes, exprTypesVta, gaContext);

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
			TypeToTsysInternal(pa, self->type, tsyses, gaContext, typesVta);
			AddTemp(types, tsyses);
		}
		if (self->expr)
		{
			ExprToTsysInternal(pa, self->expr, types, typesVta, gaContext);
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
			TypeToTsysInternal(pa, self->type, tsyses, gaContext, typesVta);
			AddTemp(types, tsyses);
		}
		if (self->expr)
		{
			ExprToTsysInternal(pa, self->expr, types, typesVta, gaContext);
		}

		if (self->ellipsis)
		{
			AddTemp(result, pa.tsys->Size());
		}
		else
		{
			isVta = ExpandPotentialVtaMultiResult(pa, result, [this](ExprTsysList& processResult, ExprTsysItem arg)
			{
				AddTemp(processResult, pa.tsys->Size());
			}, Input(types, typesVta));
		}
	}

	//////////////////////////////////////////////////////////////////////////////////////
	// ThrowExpr
	//////////////////////////////////////////////////////////////////////////////////////

	void Visit(ThrowExpr* self)override
	{
		ProcessThrowExpr(pa, result, gaContext, self);
	}

	//////////////////////////////////////////////////////////////////////////////////////
	// DeleteExpr
	//////////////////////////////////////////////////////////////////////////////////////

	void Visit(DeleteExpr* self)override
	{
		ExprTsysList types;
		bool typesVta = false;
		ExprToTsysInternal(pa, self->expr, types, typesVta, gaContext);
		isVta = ExpandPotentialVtaMultiResult(pa, result, [this](ExprTsysList& processResult, ExprTsysItem arg)
		{
			AddTemp(processResult, pa.tsys->Void());
		}, Input(types, typesVta));
	}

	//////////////////////////////////////////////////////////////////////////////////////
	// IdExpr
	//////////////////////////////////////////////////////////////////////////////////////

	void Visit(IdExpr* self)override
	{
		CreateIdReferenceExpr(pa, gaContext, self->resolving, result, false, true, isVta);
	}

	//////////////////////////////////////////////////////////////////////////////////////
	// ChildExpr
	//////////////////////////////////////////////////////////////////////////////////////

	void Visit(ChildExpr* self)override
	{
		if (!IsResolvedToType(self->classType))
		{
			bool childIsVta = false;
			CreateIdReferenceExpr(pa, gaContext, self->resolving, result, true, false, childIsVta);
			return;
		}

		TypeTsysList classTypes;
		bool classVta = false;
		TypeToTsysInternal(pa, self->classType, classTypes, gaContext, classVta);

		isVta = ExpandPotentialVtaMultiResult(pa, result, [this, self](ExprTsysList& processResult, ExprTsysItem argClass)
		{
			if (argClass.tsys->IsUnknownType())
			{
				AddTemp(processResult, pa.tsys->Any());
			}
			else
			{
				ProcessChildExpr(pa, processResult, gaContext, self, argClass);
			}
		}, Input(classTypes, classVta));
	}

	//////////////////////////////////////////////////////////////////////////////////////
	// FieldAccessExpr
	//////////////////////////////////////////////////////////////////////////////////////

	void Visit(FieldAccessExpr* self)override
	{
		ExprTsysList parentTypes;
		TypeTsysList classTypes;
		bool parentVta = false;
		bool classVta = false;
		ExprToTsysInternal(pa, self->expr, parentTypes, parentVta, gaContext);

		auto childExpr = self->name.Cast<ChildExpr>();
		auto idExpr = self->name.Cast<IdExpr>();
		if (childExpr && IsResolvedToType(childExpr->classType))
		{
			TypeToTsysInternal(pa, childExpr->classType, classTypes, gaContext, classVta);
		}
		else
		{
			classTypes.Add(nullptr);
		}

		ResolveSymbolResult totalRar;
		bool operatorIndexed = false;
		isVta = ExpandPotentialVtaMultiResult(pa, result, [this, self, idExpr, childExpr, &totalRar, &operatorIndexed](ExprTsysList& processResult, ExprTsysItem argParent, ExprTsysItem argClass)
		{
			ProcessFieldAccessExpr(pa, processResult, gaContext, self, argParent, argClass, idExpr, childExpr, totalRar, operatorIndexed);
		}, Input(parentTypes, parentVta), Input(classTypes, classVta));

		if (idExpr && !gaContext)
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
		ExprToTsysInternal(pa, self->expr, arrayTypes, arrayVta, gaContext);
		ExprToTsysInternal(pa, self->index, indexTypes, indexVta, gaContext);

		bool indexed = false;
		isVta = ExpandPotentialVtaMultiResult(pa, result, [this, self, &indexed](ExprTsysList& processResult, ExprTsysItem argArray, ExprTsysItem argIndex)
		{
			ProcessArrayAccessExpr(pa, processResult, gaContext, self, argArray, argIndex, indexed);
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
			ExprToTsysInternal(pa, self->arguments[i].item, argTypesList[i], isVtas[i], gaContext);
		}

		bool argHasBoundedVta = false;
		vint argUnboundedVtaCount = -1;
		isVta = CheckVta(self->arguments, argTypesList, isVtas, 0, argHasBoundedVta, argUnboundedVtaCount);

		ExprTsysList funcExprTypes;
		bool funcVta = false;
		vint funcVtaCount = -1;
		ExprToTsysInternal(pa, self->expr, funcExprTypes, funcVta, gaContext);

		if (funcVta)
		{
			if (argHasBoundedVta)
			{
				throw NotConvertableException();
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
						throw NotConvertableException();
					}

					if (argUnboundedVtaCount == -1)
					{
						argUnboundedVtaCount = funcVtaCount;
					}
					if (argUnboundedVtaCount != funcVtaCount)
					{
						throw NotConvertableException();
					}
				}
			}
		}

		ExprTsysList totalSelectedFunctions;
		ExpandPotentialVtaList(pa, result, argTypesList, isVtas, argHasBoundedVta, argUnboundedVtaCount,
			[this, self, funcVta, &funcExprTypes, &totalSelectedFunctions](ExprTsysList& processResult, Array<ExprTsysItem>& args, vint unboundedVtaIndex, SortedList<vint>& unboundedAnys)
			{
				// TODO: Implement variadic template argument passing
				if (unboundedAnys.Count() > 0)
				{
					throw NotConvertableException();
				}

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
						SortedList<Symbol*> nss, classes;
						for (vint i = 0; i < args.Count(); i++)
						{
							SearchAdlClassesAndNamespaces(pa, args[i].tsys, nss, classes);
						}
						SerachAdlFunction(pa, nss, idExpr->name.name, funcTypes);
					}
				}

				for (vint i = 0; i < funcTypes.Count(); i++)
				{
					TsysCV cv;
					TsysRefType refType;
					auto entityType = funcTypes[i].tsys->GetEntity(cv, refType);
					if (entityType->IsUnknownType())
					{
						AddTemp(processResult, pa.tsys->Any());
					}
				}
				FindQualifiedFunctors(pa, {}, TsysRefType::None, funcTypes, true);

				ExprTsysList selectedFunctions;
				VisitOverloadedFunction(pa, funcTypes, args, processResult, (pa.recorder ? &selectedFunctions : nullptr));
				AddInternal(totalSelectedFunctions, selectedFunctions);
			});

		if (pa.recorder && !gaContext)
		{
			CppName* name = nullptr;
			Ptr<Resolving>* nameResolving = nullptr;
			if (auto idExpr = self->expr.Cast<IdExpr>())
			{
				name = &idExpr->name;
				nameResolving = &idExpr->resolving;
			}
			else if (auto childExpr = self->expr.Cast<ChildExpr>())
			{
				name = &childExpr->name;
				nameResolving = &childExpr->resolving;
			}
			else if (auto fieldExpr = self->expr.Cast<FieldAccessExpr>())
			{
				if (auto fieldExprName = fieldExpr->name.Cast<IdExpr>())
				{
					name = &fieldExprName->name;
					nameResolving = &fieldExprName->resolving;
				}
			}

			bool addedName = false;
			bool addedOp = false;
			AddSymbolsToResolvings(gaContext, name, nameResolving, &self->opName, &self->opResolving, totalSelectedFunctions, addedName, addedOp);
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
		VariadicInput<ExprTsysItem> variadicInput(self->initializer ? self->initializer->arguments.Count() + 1 : 1, pa, gaContext);
		variadicInput.ApplySingle(0, self->type);
		if (self->initializer)
		{
			variadicInput.ApplyVariadicList(1, self->initializer->arguments);
		}
		isVta = variadicInput.Expand((self->initializer ? &self->initializer->arguments : nullptr), result,
			[this, self](ExprTsysList& processResult, Array<ExprTsysItem>& args, vint unboundedVtaIndex, SortedList<vint>& boundedAnys)
			{
				ProcessCtorAccessExpr(pa, processResult, self, args, boundedAnys);
			});
	}

	//////////////////////////////////////////////////////////////////////////////////////
	// NewExpr
	//////////////////////////////////////////////////////////////////////////////////////

	void Visit(NewExpr* self)override
	{
		// TODO: Enable variadic template argument in placement arguments
		for (vint i = 0; i < self->placementArguments.Count(); i++)
		{
			if (self->placementArguments[i].isVariadic)
			{
				throw NotConvertableException();
			}
			ExprTsysList types;
			ExprToTsysNoVta(pa, self->placementArguments[i].item, types, gaContext);
		}

		VariadicInput<ExprTsysItem> variadicInput(self->initializer ? self->initializer->arguments.Count() + 1 : 1, pa, gaContext);
		variadicInput.ApplySingle(0, self->type);
		if (self->initializer)
		{
			variadicInput.ApplyVariadicList(1, self->initializer->arguments);
		}
		isVta = variadicInput.Expand((self->initializer ? &self->initializer->arguments : nullptr), result,
			[this, self](ExprTsysList& processResult, Array<ExprTsysItem>& args, vint unboundedVtaIndex, SortedList<vint>& boundedAnys)
			{
				ProcessNewExpr(pa, processResult, self, args, boundedAnys);
			});
	}

	//////////////////////////////////////////////////////////////////////////////////////
	// UniversalInitializerExpr
	//////////////////////////////////////////////////////////////////////////////////////

	void Visit(UniversalInitializerExpr* self)override
	{
		VariadicInput<ExprTsysItem> variadicInput(self->arguments.Count(), pa, gaContext);
		variadicInput.ApplyVariadicList(0, self->arguments);
		isVta = variadicInput.Expand(&self->arguments, result,
			[this, self](ExprTsysList& processResult, Array<ExprTsysItem>& args, vint unboundedVtaIndex, SortedList<vint>& boundedAnys)
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
		ExprToTsysInternal(pa, self->operand, types, typesVta, gaContext);

		bool indexed = false;
		isVta = ExpandPotentialVtaMultiResult(pa, result, [this, self, &indexed](ExprTsysList& processResult, ExprTsysItem arg1)
		{
			ProcessPostfixUnaryExpr(pa, processResult, gaContext, self, arg1, indexed);
		}, Input(types, typesVta));

		if (indexed)
		{
			pa.recorder->IndexOverloadingResolution(self->opName, self->opResolving->resolvedSymbols);
		}
	}

	//////////////////////////////////////////////////////////////////////////////////////
	// PrefixUnaryExpr
	//////////////////////////////////////////////////////////////////////////////////////

	void Visit(PrefixUnaryExpr* self)override
	{
		ExprTsysList types;
		bool typesVta = false;
		if (self->op == CppPrefixUnaryOp::AddressOf)
		{
			if (auto childExpr = self->operand.Cast<ChildExpr>())
			{
				if (IsResolvedToType(childExpr->classType))
				{
					TypeTsysList classTypes;
					TypeToTsysInternal(pa, childExpr->classType, classTypes, gaContext, typesVta);
					ExpandPotentialVtaMultiResult(pa, types, [this, self, childExpr](ExprTsysList& processResult, ExprTsysItem arg1)
					{
						if (arg1.tsys->IsUnknownType())
						{
							AddTemp(processResult, pa.tsys->Any());
						}
						else if (arg1.tsys->GetType() == TsysType::Decl)
						{
							auto newPa = pa.WithContext(arg1.tsys->GetDecl());
							auto rsr = ResolveSymbol(newPa, childExpr->name, SearchPolicy::ChildSymbol);
							if (rsr.values)
							{
								for (vint i = 0; i < rsr.values->resolvedSymbols.Count(); i++)
								{
									VisitSymbol(newPa, nullptr, rsr.values->resolvedSymbols[i], true, processResult);
								}
							}
						}
					}, Input(classTypes, typesVta));
					goto SKIP_RESOLVING_OPERAND;
				}
			}
		}

		ExprToTsysInternal(pa, self->operand, types, typesVta, gaContext);
	SKIP_RESOLVING_OPERAND:
		bool indexed = false;

		isVta = ExpandPotentialVtaMultiResult(pa, result, [this, self, &indexed](ExprTsysList& processResult, ExprTsysItem arg1)
		{
			ProcessPrefixUnaryExpr(pa, processResult, gaContext, self, arg1, indexed);
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
		ExprToTsysInternal(pa, self->left, leftTypes, leftVta, gaContext);
		ExprToTsysInternal(pa, self->right, rightTypes, rightVta, gaContext);
		bool indexed = false;

		isVta = ExpandPotentialVtaMultiResult(pa, result, [this, self, &indexed](ExprTsysList& processResult, ExprTsysItem argLeft, ExprTsysItem argRight)
		{
			ProcessBinaryExpr(pa, processResult, gaContext, self, argLeft, argRight, indexed);
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
		ExprToTsysInternal(pa, self->condition, conditionTypes, conditionVta, gaContext);
		ExprToTsysInternal(pa, self->left, leftTypes, leftVta, gaContext);
		ExprToTsysInternal(pa, self->right, rightTypes, rightVta, gaContext);

		isVta = ExpandPotentialVtaMultiResult(pa, result, [this, self](ExprTsysList& processResult, ExprTsysItem argCond, ExprTsysItem argLeft, ExprTsysItem argRight)
		{
			ProcessIfExpr(pa, processResult, gaContext, self, argCond, argLeft, argRight);
		}, Input(conditionTypes, conditionVta), Input(leftTypes, leftVta), Input(rightTypes, rightVta));
	}

	//////////////////////////////////////////////////////////////////////////////////////
	// GenericExpr
	//////////////////////////////////////////////////////////////////////////////////////

	void Visit(GenericExpr* self)override
	{
		vint count = self->arguments.Count() + 1;
		Array<bool> isTypes(count);
		isTypes[0] = false;

		VariadicInput<ExprTsysItem> variadicInput(count, pa, gaContext);
		variadicInput.ApplySingle<Expr>(0, self->expr);
		variadicInput.ApplyGenericArguments(1, isTypes, self->arguments);
		isVta = variadicInput.Expand(&self->arguments, result,
			[this, self, &isTypes](ExprTsysList& processResult, Array<ExprTsysItem>& args, vint unboundedVtaIndex, SortedList<vint>& boundedAnys)
			{
				ProcessGenericExpr(pa, processResult, self, isTypes, args, boundedAnys);
			});
	}
};

// Resolve expressions to types

void ExprToTsysInternal(const ParsingArguments& pa, Ptr<Expr> e, ExprTsysList& tsys, bool& isVta, GenericArgContext* gaContext)
{
	if (!e) throw IllegalExprException();
	ExprToTsysVisitor visitor(pa, tsys, gaContext);
	e->Accept(&visitor);
	isVta = visitor.isVta;
}

void ExprToTsysNoVta(const ParsingArguments& pa, Ptr<Expr> e, ExprTsysList& tsys, GenericArgContext* gaContext)
{
	bool isVta = false;
	ExprToTsysInternal(pa, e, tsys, isVta, gaContext);
	if (isVta)
	{
		throw NotConvertableException();
	}
}