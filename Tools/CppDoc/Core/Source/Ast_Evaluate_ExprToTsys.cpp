#include "Ast_Resolving_ExpandPotentialVta.h"

using namespace symbol_type_resolving;
using namespace symbol_totsys_impl;

/***********************************************************************
ExprToTsys
	PlaceholderExpr				: *literal
	LiteralExpr					: literal		*
	ThisExpr					: literal		*
	NullptrExpr					: literal		*
	ParenthesisExpr				: unbounded		*
	CastExpr					: unbounded		*
	TypeidExpr					: literal		*
	SizeofExpr					: literal		*
	ThrowExpr					: literal		*
	DeleteExpr					: literal		*
	IdExpr						: *identifier
	ChildExpr					: *unbounded
	FieldAccessExpr				: *unbounded
	ArrayAccessExpr				: *unbounded
	FuncAccessExpr				: *variant
	CtorAccessExpr				: *variant
	NewExpr						: *variant
	UniversalInitializerExpr	: *variant
	PostfixUnaryExpr			: unbounded		*
	PrefixUnaryExpr				: unbounded		*
	BinaryExpr					: unbounded		*
	IfExpr						: unbounded		*
	GenericExpr					: *variant
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

	void ReIndex(CppName* name, Ptr<Resolving>* nameResolving, CppName* op, Ptr<Resolving>* opResolving, ExprTsysList& symbols)
	{
		bool addedName = false;
		bool addedOp = false;
		AddSymbolsToResolvings(gaContext, name, nameResolving, op, opResolving, symbols, addedName, addedOp);
		if (addedName)
		{
			pa.recorder->IndexOverloadingResolution(*name, (*nameResolving)->resolvedSymbols);
		}
		if (addedOp)
		{
			pa.recorder->IndexOverloadingResolution(*op, (*opResolving)->resolvedSymbols);
		}
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
		isVta = ExpandPotentialVtaMultiResult(pa, result, [=](ExprTsysList& processResult, ExprTsysItem arg1)
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

		isVta = ExpandPotentialVtaMultiResult(pa, result, [=](ExprTsysList& processResult, ExprTsysItem argType, ExprTsysItem argExpr)
		{
			ProcessCastExpr(pa, processResult, self, argType, argExpr);
		}, Input(types, typesVta), Input(exprTypes, exprTypesVta));
	}

	//////////////////////////////////////////////////////////////////////////////////////
	// TypeidExpr
	//////////////////////////////////////////////////////////////////////////////////////

	void Visit(TypeidExpr* self)override
	{
		ProcessTypeidExpr(pa, result, gaContext, self);
	}

	//////////////////////////////////////////////////////////////////////////////////////
	// SizeofExpr
	//////////////////////////////////////////////////////////////////////////////////////

	void Visit(SizeofExpr* self)override
	{
		ProcessSizeofExpr(pa, result, gaContext, self);
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
		ProcessDeleteExpr(pa, result, gaContext, self);
	}

	//////////////////////////////////////////////////////////////////////////////////////
	// IdExpr
	//////////////////////////////////////////////////////////////////////////////////////

	void VisitResolvableExpr(Ptr<Resolving> resolving, bool allowAny)
	{
		if (!resolving)
		{
			if (allowAny)
			{
				AddTemp(result, pa.tsys->Any());
			}
			return;
		}
		else if (resolving->resolvedSymbols.Count() == 0)
		{
			throw NotConvertableException();
		}

		ExprTsysList resolvableResult;
		auto& outputTarget = gaContext ? resolvableResult : result;

		if (pa.funcSymbol && pa.funcSymbol->methodCache)
		{
			TsysCV thisCv;
			TsysRefType thisRef;
			auto thisType = pa.funcSymbol->methodCache->thisType->GetEntity(thisCv, thisRef);
			ExprTsysItem thisItem(nullptr, ExprTsysType::LValue, thisType->GetElement()->LRefOf());
			VisitResolvedMember(pa, &thisItem, resolving, outputTarget);
		}
		else
		{
			VisitResolvedMember(pa, nullptr, resolving, outputTarget);
		}

		if (gaContext)
		{
			for (vint i = 0; i < resolvableResult.Count(); i++)
			{
				auto item = resolvableResult[i];
				TypeTsysList types;
				item.tsys->ReplaceGenericArgs(*gaContext, types);
				for (vint j = 0; j < types.Count(); j++)
				{
					AddInternal(result, { item.symbol,item.type,types[j] });
				}
			}
		}
	}

	void Visit(IdExpr* self)override
	{
		VisitResolvableExpr(self->resolving, false);
	}

	//////////////////////////////////////////////////////////////////////////////////////
	// ChildExpr
	//////////////////////////////////////////////////////////////////////////////////////

	Ptr<Resolving> ResolveChildExprWithGenericArguments(ChildExpr* self)
	{
		// TODO: Read void Visit(PrefixUnaryExpr* self) before refactoring
		Ptr<Resolving> resolving;

		TypeTsysList types;
		TypeToTsysNoVta(pa, self->classType, types, gaContext);
		for (vint i = 0; i < types.Count(); i++)
		{
			auto type = types[i];
			if (type->GetType() == TsysType::Decl)
			{
				auto newPa = pa.WithContext(type->GetDecl());
				auto rsr = ResolveSymbol(newPa, self->name, SearchPolicy::ChildSymbol);
				if (rsr.values)
				{
					if (!resolving)
					{
						resolving = rsr.values;
					}
					else
					{
						for (vint i = 0; i < rsr.values->resolvedSymbols.Count(); i++)
						{
							if (!resolving->resolvedSymbols.Contains(rsr.values->resolvedSymbols[i]))
							{
								resolving->resolvedSymbols.Add(rsr.values->resolvedSymbols[i]);
							}
						}
					}
				}
			}
		}

		return resolving;
	}

	void Visit(ChildExpr* self)override
	{
		if (gaContext && !self->resolving)
		{
			if (auto resolving = ResolveChildExprWithGenericArguments(self))
			{
				VisitResolvableExpr(resolving, false);
			}
		}
		else
		{
			VisitResolvableExpr(self->resolving, true);
		}
	}

	//////////////////////////////////////////////////////////////////////////////////////
	// FieldAccessExpr
	//////////////////////////////////////////////////////////////////////////////////////

	void ResolveField(ResolveSymbolResult& totalRar, const ExprTsysItem& parentItem, const Ptr<IdExpr>& idExpr, const Ptr<ChildExpr>& childExpr)
	{
		if (idExpr)
		{
			if (parentItem.tsys->IsUnknownType())
			{
				AddTemp(result, pa.tsys->Any());
			}
			else
			{
				VisitDirectField(pa, totalRar, parentItem, idExpr->name, result);
			}
		}
		else
		{
			if (childExpr->resolving)
			{
				VisitResolvedMember(pa, &parentItem, childExpr->resolving, result);
			}
			else if (gaContext)
			{
				if (auto resolving = ResolveChildExprWithGenericArguments(childExpr.Obj()))
				{
					VisitResolvedMember(pa, &parentItem, resolving, result);
				}
			}
			else
			{
				AddTemp(result, pa.tsys->Any());
			}
		}
	}

	void Visit(FieldAccessExpr* self)override
	{
		ResolveSymbolResult totalRar;
		ExprTsysList parentItems;
		ExprToTsys(pa, self->expr, parentItems, gaContext);

		auto childExpr = self->name.Cast<ChildExpr>();
		auto idExpr = self->name.Cast<IdExpr>();

		if (self->type == CppFieldAccessType::Dot)
		{
			for (vint i = 0; i < parentItems.Count(); i++)
			{
				ResolveField(totalRar, parentItems[i], idExpr, childExpr);
			}
		}
		else
		{
			SortedList<ITsys*> visitedDecls;
			for (vint i = 0; i < parentItems.Count(); i++)
			{
				TsysCV cv;
				TsysRefType refType;
				auto entityType = parentItems[i].tsys->GetEntity(cv, refType);

				if (entityType->GetType() == TsysType::Ptr)
				{
					ExprTsysItem parentItem(nullptr, ExprTsysType::LValue, entityType->GetElement());
					ResolveField(totalRar, parentItem, idExpr, childExpr);
				}
				else if (entityType->GetType() == TsysType::Decl)
				{
					if (!visitedDecls.Contains(entityType))
					{
						visitedDecls.Add(entityType);

						ExprTsysList opResult;
						VisitFunctors(pa, parentItems[i], L"operator ->", opResult);
						for (vint j = 0; j < opResult.Count(); j++)
						{
							auto item = opResult[j];
							item.tsys = item.tsys->GetElement();
							AddNonVar(parentItems, item);
						}
					}
				}
			}
		}

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
		isVta = ExpandPotentialVtaMultiResult(pa, result, [&](ExprTsysList& processResult, ExprTsysItem argArray, ExprTsysItem argIndex)
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
		List<Ptr<ExprTsysList>> argTypesList;
		for (vint i = 0; i < self->arguments.Count(); i++)
		{
			auto argTypes = MakePtr<ExprTsysList>();
			ExprToTsys(pa, self->arguments[i], *argTypes.Obj(), gaContext);
			argTypesList.Add(argTypes);
		}

		ExprTsysList funcTypes;
		ExprToTsys(pa, self->expr, funcTypes, gaContext);

		if (auto idExpr = self->expr.Cast<IdExpr>())
		{
			if (!idExpr->resolving || IsAdlEnabled(pa, idExpr->resolving))
			{
				SortedList<Symbol*> nss, classes;
				for (vint i = 0; i < argTypesList.Count(); i++)
				{
					SearchAdlClassesAndNamespaces(pa, *argTypesList[i].Obj(), nss, classes);
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
				AddTemp(result, pa.tsys->Any());
			}
		}
		FindQualifiedFunctors(pa, {}, TsysRefType::None, funcTypes, true);

		ExprTsysList selectedFunctions;
		VisitOverloadedFunction(pa, funcTypes, argTypesList, result, (pa.recorder ? &selectedFunctions : nullptr));
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

			ReIndex(name, nameResolving, &self->opName, &self->opResolving, selectedFunctions);
		}
	}

	//////////////////////////////////////////////////////////////////////////////////////
	// CtorAccessExpr
	//////////////////////////////////////////////////////////////////////////////////////

	void Visit(CtorAccessExpr* self)override
	{
		List<Ptr<ExprTsysList>> argTypesList;
		for (vint i = 0; i < self->initializer->arguments.Count(); i++)
		{
			auto argTypes = MakePtr<ExprTsysList>();
			ExprToTsys(pa, self->initializer->arguments[i], *argTypes.Obj(), gaContext);
			argTypesList.Add(argTypes);
		}

		{
			TypeTsysList types;
			TypeToTsysNoVta(pa, self->type, types, gaContext);
			AddTemp(result, types);
		}
	}

	//////////////////////////////////////////////////////////////////////////////////////
	// NewExpr
	//////////////////////////////////////////////////////////////////////////////////////

	void Visit(NewExpr* self)override
	{
		for (vint i = 0; i < self->placementArguments.Count(); i++)
		{
			ExprTsysList types;
			ExprToTsys(pa, self->placementArguments[i], types, gaContext);
		}

		if (self->initializer)
		{
			for (vint i = 0; i < self->initializer->arguments.Count(); i++)
			{
				ExprTsysList types;
				ExprToTsys(pa, self->initializer->arguments[i], types, gaContext);
			}
		}

		TypeTsysList types;
		TypeToTsysNoVta(pa, self->type, types, gaContext);
		for (vint i = 0; i < types.Count(); i++)
		{
			auto type = types[i];
			if (type->GetType() == TsysType::Array)
			{
				if (type->GetParamCount() == 1)
				{
					AddTemp(result, types[i]->GetElement()->PtrOf());
				}
				else
				{
					AddTemp(result, types[i]->GetElement()->ArrayOf(type->GetParamCount() - 1)->PtrOf());
				}
			}
			else
			{
				AddTemp(result, types[i]->PtrOf());
			}
		}
	}

	//////////////////////////////////////////////////////////////////////////////////////
	// UniversalInitializerExpr
	//////////////////////////////////////////////////////////////////////////////////////

	void Visit(UniversalInitializerExpr* self)override
	{
		Array<ExprTsysList> argTypesList(self->arguments.Count());
		for (vint i = 0; i < self->arguments.Count(); i++)
		{
			ExprToTsys(pa, self->arguments[i], argTypesList[i], gaContext);
		}

		CreateUniversalInitializerType(pa, argTypesList, result);
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
		isVta = ExpandPotentialVtaMultiResult(pa, result, [&](ExprTsysList& processResult, ExprTsysItem arg1)
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
					ExpandPotentialVtaMultiResult(pa, types, [=](ExprTsysList& processResult, ExprTsysItem arg1)
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

		isVta = ExpandPotentialVtaMultiResult(pa, result, [&](ExprTsysList& processResult, ExprTsysItem arg1)
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

		isVta = ExpandPotentialVtaMultiResult(pa, result, [&](ExprTsysList& processResult, ExprTsysItem argLeft, ExprTsysItem argRight)
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

		isVta = ExpandPotentialVtaMultiResult(pa, result, [=](ExprTsysList& processResult, ExprTsysItem argCond, ExprTsysItem argLeft, ExprTsysItem argRight)
		{
			ProcessIfExpr(pa, processResult, gaContext, self, argCond, argLeft, argRight);
		}, Input(conditionTypes, conditionVta), Input(leftTypes, leftVta), Input(rightTypes, rightVta));
	}

	//////////////////////////////////////////////////////////////////////////////////////
	// GenericExpr
	//////////////////////////////////////////////////////////////////////////////////////

	void Visit(GenericExpr* self)override
	{
		ExprTsysList genericTypes;
		vint count = self->arguments.Count();
		Array<TypeTsysList> argumentTypes(count);
		Array<bool> isTypes(count);

		ExprToTsys(pa, self->expr, genericTypes, gaContext);
		symbol_type_resolving::ResolveGenericArguments(pa, self->arguments, argumentTypes, isTypes, gaContext);

		for (vint i = 0; i < genericTypes.Count(); i++)
		{
			auto genericFunction = genericTypes[i].tsys;
			if (genericFunction->GetType() == TsysType::GenericFunction)
			{
				auto declSymbol = genericFunction->GetGenericFunction().declSymbol;
				if (!declSymbol)
				{
					throw NotConvertableException();
				}

				symbol_type_resolving::EvaluateSymbolContext esContext;
				if (!symbol_type_resolving::ResolveGenericParameters(pa, genericFunction, argumentTypes, isTypes, &esContext.gaContext))
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
};

// Resolve expressions to types

void ExprToTsysInternal(const ParsingArguments& pa, Ptr<Expr> e, ExprTsysList& tsys, bool& isVta, GenericArgContext* gaContext)
{
	if (!e) throw IllegalExprException();
	ExprToTsysVisitor visitor(pa, tsys, gaContext);
	e->Accept(&visitor);
	isVta = visitor.isVta;
}

void ExprToTsys(const ParsingArguments& pa, Ptr<Expr> e, ExprTsysList& tsys, GenericArgContext* gaContext)
{
	bool isVta = false;
	ExprToTsysInternal(pa, e, tsys, isVta, gaContext);
	if (isVta)
	{
		throw NotConvertableException();
	}
}