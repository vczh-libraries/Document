#include "Ast_Resolving.h"
#include "Ast_Expr.h"

using namespace symbol_type_resolving;

/***********************************************************************
ExprToTsys
***********************************************************************/

class ExprToTsysVisitor : public Object, public virtual IExprVisitor
{
public:
	ExprTsysList&				result;
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
		if (gaContext) return;
		bool firstName = true;
		bool firstOp = true;

		for (vint i = 0; i < symbols.Count(); i++)
		{
			if (auto symbol = symbols[i].symbol)
			{
				bool isOperator = false;
				if (auto funcDecl = symbol->GetAnyForwardDecl<ForwardFunctionDeclaration>())
				{
					isOperator = funcDecl->name.type == CppNameType::Operator;
				}

				if (name)
				{
					if (!isOperator || name->type == CppNameType::Operator)
					{
						if (firstName)
						{
							*nameResolving = MakePtr<Resolving>();
						}
						(*nameResolving)->resolvedSymbols.Add(symbol);
						firstName = false;
						continue;
					}
				}

				if (op)
				{
					if (isOperator)
					{
						if (firstOp)
						{
							*opResolving = MakePtr<Resolving>();
						}
						(*opResolving)->resolvedSymbols.Add(symbol);
						firstOp = false;
					}
				}
			}
		}

		if (!firstName && name)
		{
			pa.recorder->IndexOverloadingResolution(*name, *nameResolving);
		}
		if (!firstOp && op)
		{
			pa.recorder->IndexOverloadingResolution(*op, *opResolving);
		}
	}

	/***********************************************************************
	Expressions
	***********************************************************************/

	void Visit(PlaceholderExpr* self)override
	{
		AddInternal(result, *self->types);
	}

	void Visit(LiteralExpr* self)override
	{
		switch ((CppTokens)self->tokens[0].token)
		{
		case CppTokens::INT:
		case CppTokens::HEX:
		case CppTokens::BIN:
			{
				auto& token = self->tokens[0];
				{
					auto reading = token.reading;
					auto end = token.reading + token.length;
					if (reading[0] == L'0')
					{
						switch (reading[1])
						{
						case L'x':
						case L'X':
						case L'b':
						case L'B':
							reading += 2;
						}
					}

					while (reading < end)
					{
						if (L'1' <= *reading && *reading <= L'9')
						{
							goto NOT_ZERO;
						}
						reading++;
					}

					AddTemp(result, pa.tsys->Zero());
					return;
				}
			NOT_ZERO:
#define COUNT_CHAR(NUM, UC, LC) ((_##NUM == UC || _##NUM == LC) ? 1 : 0)
#define COUNT_U(NUM) COUNT_CHAR(NUM, L'u', L'U')
#define COUNT_L(NUM) COUNT_CHAR(NUM, L'l', L'L')
				wchar_t _1 = token.length > 2 ? token.reading[token.length - 3] : 0;
				wchar_t _2 = token.length > 1 ? token.reading[token.length - 2] : 0;
				wchar_t _3 = token.reading[token.length - 1];
				vint us = COUNT_U(1) + COUNT_U(2) + COUNT_U(3);
				vint ls = COUNT_L(1) + COUNT_L(2) + COUNT_L(3);
				AddTemp(result, pa.tsys->PrimitiveOf({ (us > 0 ? TsysPrimitiveType::UInt : TsysPrimitiveType::SInt),{ls > 1 ? TsysBytes::_8 : TsysBytes::_4} }));
#undef COUNT_CHAR
#undef COUNT_U
#undef COUNT_L
			}
			return;
		case CppTokens::FLOAT:
			{
				auto& token = self->tokens[0];
				wchar_t _1 = token.reading[token.length - 1];
				if (_1 == L'f' || _1 == L'F')
				{
					AddTemp(result, pa.tsys->PrimitiveOf({ TsysPrimitiveType::Float, TsysBytes::_4 }));
				}
				else
				{
					AddTemp(result, pa.tsys->PrimitiveOf({ TsysPrimitiveType::Float, TsysBytes::_8 }));
				}
			}
			return;
		case CppTokens::STRING:
		case CppTokens::CHAR:
			{
				ITsys* tsysChar = nullptr;
				auto reading = self->tokens[0].reading;
				if (reading[0] == L'\"' || reading[0]==L'\'')
				{
					tsysChar = pa.tsys->PrimitiveOf({ TsysPrimitiveType::SChar,TsysBytes::_1 });
				}
				else if (reading[0] == L'L')
				{
					tsysChar = pa.tsys->PrimitiveOf({ TsysPrimitiveType::UWChar,TsysBytes::_2 });
				}
				else if (reading[0] == L'U')
				{
					tsysChar = pa.tsys->PrimitiveOf({ TsysPrimitiveType::UChar,TsysBytes::_4 });
				}
				else if (reading[0] == L'u')
				{
					if (reading[1] == L'8')
					{
						tsysChar = pa.tsys->PrimitiveOf({ TsysPrimitiveType::SChar,TsysBytes::_1 });
					}
					else
					{
						tsysChar = pa.tsys->PrimitiveOf({ TsysPrimitiveType::UChar,TsysBytes::_2 });
					}
				}

				if (!tsysChar)
				{
					throw IllegalExprException();
				}

				if ((CppTokens)self->tokens[0].token == CppTokens::CHAR)
				{
					AddTemp(result, tsysChar);
				}
				else
				{
					AddTemp(result, tsysChar->CVOf({ true,false })->ArrayOf(1)->LRefOf());
				}
			}
			return;
		case CppTokens::EXPR_TRUE:
		case CppTokens::EXPR_FALSE:
			AddTemp(result, pa.tsys->PrimitiveOf({ TsysPrimitiveType::Bool,TsysBytes::_1 }));
			return;
		}
		throw IllegalExprException();
	}

	void Visit(ThisExpr* self)override
	{
		if (auto funcSymbol = pa.funcSymbol)
		{
			if (auto methodCache = funcSymbol->methodCache)
			{
				if (auto thisType = methodCache->thisType)
				{
					AddTemp(result, thisType);
					return;
				}
			}
		}
		throw IllegalExprException();
	}

	void Visit(NullptrExpr* self)override
	{
		AddTemp(result, pa.tsys->Nullptr());
	}

	void Visit(ParenthesisExpr* self)override
	{
		ExprTsysList types;
		ExprToTsys(pa, self->expr, types, gaContext);
		for (vint i = 0; i < types.Count(); i++)
		{
			if (types[i].type == ExprTsysType::LValue)
			{
				AddTemp(result, types[i].tsys->LRefOf());
			}
			else
			{
				AddTemp(result, types[i].tsys);
			}
		}
	}

	void Visit(CastExpr* self)override
	{
		{
			ExprTsysList types;
			ExprToTsys(pa, self->expr, types, gaContext);
		}
		{
			TypeTsysList types;
			TypeToTsysNoVta(pa, self->type, types, gaContext);
			AddTemp(result, types);
		}
	}

	void Visit(TypeidExpr* self)override
	{
		if (self->type)
		{
			TypeTsysList types;
			TypeToTsysNoVta(pa, self->type, types, gaContext);
		}
		if (self->expr)
		{
			ExprTsysList types;
			ExprToTsys(pa, self->expr, types, gaContext);
		}

		auto global = pa.root.Obj();
		vint index = global->children.Keys().IndexOf(L"std");
		if (index == -1) return;
		auto& stds = global->children.GetByIndex(index);
		if (stds.Count() != 1) return;
		index = stds[0]->children.Keys().IndexOf(L"type_info");
		if (index == -1) return;
		auto& tis = stds[0]->children.GetByIndex(index);

		for (vint i = 0; i < tis.Count(); i++)
		{
			auto ti = tis[i];
			switch (ti->kind)
			{
			case symbol_component::SymbolKind::Class:
			case symbol_component::SymbolKind::Struct:
				AddInternal(result, { nullptr,ExprTsysType::LValue,pa.tsys->DeclOf(ti.Obj()) });
				return;
			}
		}
	}

	void Visit(SizeofExpr* self)override
	{
		if (self->type)
		{
			TypeTsysList types;
			bool typeIsVta = false;
			TypeToTsysInternal(pa, self->type.Obj(), types, gaContext, typeIsVta);
		}
		if (self->expr)
		{
			ExprTsysList types;
			ExprToTsys(pa, self->expr, types, gaContext);
		}

		AddTemp(result, pa.tsys->Size());
	}

	void Visit(ThrowExpr* self)override
	{
		if (self->expr)
		{
			ExprTsysList types;
			ExprToTsys(pa, self->expr, types, gaContext);
		}

		AddTemp(result, pa.tsys->Void());
	}

	void Visit(DeleteExpr* self)override
	{
		{
			ExprTsysList types;
			ExprToTsys(pa, self->expr, types, gaContext);
		}

		AddTemp(result, pa.tsys->Void());
	}

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

	Ptr<Resolving> ResolveChildExprWithGenericArguments(ChildExpr* self)
	{
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
					pa.recorder->Index(idExpr->name, totalRar.values);
				}
				if (totalRar.types)
				{
					pa.recorder->ExpectValueButType(idExpr->name, totalRar.types);
				}
			}
		}
	}

	void Visit(ArrayAccessExpr* self)override
	{
		List<Ptr<ExprTsysList>> argTypesList;
		{
			auto argTypes = MakePtr<ExprTsysList>();
			ExprToTsys(pa, self->index, *argTypes.Obj(), gaContext);
			argTypesList.Add(argTypes);
		}

		ExprTsysList arrayTypes, funcTypes;
		ExprToTsys(pa, self->expr, arrayTypes, gaContext);

		for (vint i = 0; i < arrayTypes.Count(); i++)
		{
			auto arrayType = arrayTypes[i];

			TsysCV cv;
			TsysRefType refType;
			auto entityType = arrayType.tsys->GetEntity(cv, refType);

			if (entityType->IsUnknownType())
			{
				AddTemp(result, pa.tsys->Any());
			}
			else if (entityType->GetType() == TsysType::Decl)
			{
				ExprTsysList opResult;
				VisitFunctors(pa, arrayType, L"operator []", opResult);
				AddNonVar(funcTypes, opResult);
			}
			else if (entityType->GetType() == TsysType::Array)
			{
				auto tsys = entityType->GetElement();
				if (refType == TsysRefType::LRef)
				{
					AddInternal(result, { nullptr,ExprTsysType::LValue,tsys->CVOf(cv)->LRefOf() });
				}
				else if (refType == TsysRefType::RRef)
				{
					if (arrayType.type == ExprTsysType::LValue)
					{
						AddInternal(result, { nullptr,ExprTsysType::LValue,tsys->CVOf(cv)->LRefOf() });
					}
					else
					{
						AddInternal(result, { nullptr,ExprTsysType::XValue,tsys->CVOf(cv)->RRefOf() });
					}
				}
				else
				{
					AddInternal(result, { nullptr,arrayType.type,tsys->CVOf(cv)->LRefOf() });
				}
			}
			else if (entityType->GetType() == TsysType::Ptr)
			{
				AddTemp(result, entityType->GetElement()->LRefOf());
			}
		}

		ExprTsysList selectedFunctions;
		VisitOverloadedFunction(pa, funcTypes, argTypesList, result, (pa.recorder ? &selectedFunctions : nullptr));
		if (pa.recorder && !gaContext)
		{
			ReIndex(nullptr, nullptr, &self->opName, &self->opResolving, selectedFunctions);
		}
	}

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

	void Visit(UniversalInitializerExpr* self)override
	{
		List<Ptr<ExprTsysList>> argTypesList;
		for (vint i = 0; i < self->arguments.Count(); i++)
		{
			auto argTypes = MakePtr<ExprTsysList>();
			ExprToTsys(pa, self->arguments[i], *argTypes.Obj(), gaContext);
			argTypesList.Add(argTypes);
		}

		CreateUniversalInitializerType(pa, argTypesList, result);
	}

	bool VisitOperator(ExprTsysItem* leftType, ExprTsysItem* rightType, const WString& name, CppName& resolvableName, Ptr<Resolving>& resolving)
	{
		TsysCV leftCV, rightCV;
		TsysRefType leftRef, rightRef;
		auto leftEntity = leftType->tsys->GetEntity(leftCV, leftRef);
		auto rightEntity = rightType ? rightType->tsys->GetEntity(rightCV, rightRef) : nullptr;
		
		if (leftEntity->IsUnknownType())
		{
			AddTemp(result, pa.tsys->Any());
			return true;
		}
		if (rightEntity && rightEntity->IsUnknownType())
		{
			AddTemp(result, pa.tsys->Any());
			return true;
		}

		CppName opName;
		opName.type = CppNameType::Operator;
		opName.name = L"operator " + name;

		if (leftEntity->GetType() == TsysType::Decl)
		{
			auto newPa = pa.WithContext(leftEntity->GetDecl());
			auto opMethods = ResolveSymbol(newPa, opName, SearchPolicy::ChildSymbol);

			if (opMethods.values)
			{
				ExprTsysList opTypes;
				for (vint j = 0; j < opMethods.values->resolvedSymbols.Count(); j++)
				{
					VisitSymbol(pa, leftType, opMethods.values->resolvedSymbols[j], false, opTypes);
				}
				FilterFieldsAndBestQualifiedFunctions(leftCV, leftRef, opTypes);

				List<Ptr<ExprTsysList>> argTypesList;
				if (rightType)
				{
					argTypesList.Add(MakePtr<ExprTsysList>());
					AddInternal(*argTypesList[0].Obj(), *rightType);
				}
				FindQualifiedFunctors(pa, {}, TsysRefType::None, opTypes, false);

				ExprTsysList selectedFunctions;
				VisitOverloadedFunction(pa, opTypes, argTypesList, result, (pa.recorder ? &selectedFunctions : nullptr));
				if (pa.recorder && !gaContext)
				{
					ReIndex(nullptr, nullptr, &resolvableName, &resolving, selectedFunctions);
				}
				return true;
			}
		}
		{
			ExprTsysList opTypes;

			auto opFuncs = ResolveSymbol(pa, opName, SearchPolicy::SymbolAccessableInScope);
			if (opFuncs.values)
			{
				for (vint j = 0; j < opFuncs.values->resolvedSymbols.Count(); j++)
				{
					VisitSymbol(pa, leftType, opFuncs.values->resolvedSymbols[j], false, opTypes);
				}
			}
			if (!opFuncs.values || IsAdlEnabled(pa, opFuncs.values))
			{
				SortedList<Symbol*> nss, classes;
				SearchAdlClassesAndNamespaces(pa, leftEntity, nss, classes);
				if (rightEntity)
				{
					SearchAdlClassesAndNamespaces(pa, rightEntity, nss, classes);
				}
				SerachAdlFunction(pa, nss, opName.name, opTypes);
			}

			if (opTypes.Count() > 0)
			{
				List<Ptr<ExprTsysList>> argTypesList;
				{
					argTypesList.Add(MakePtr<ExprTsysList>());
					AddInternal(*argTypesList[0].Obj(), *leftType);
				}
				if (rightType)
				{
					argTypesList.Add(MakePtr<ExprTsysList>());
					AddInternal(*argTypesList[1].Obj(), *rightType);
				}
				FindQualifiedFunctors(pa, {}, TsysRefType::None, opTypes, false);

				ExprTsysList selectedFunctions;
				VisitOverloadedFunction(pa, opTypes, argTypesList, result, (pa.recorder ? &selectedFunctions : nullptr));
				if (pa.recorder && !gaContext)
				{
					ReIndex(nullptr, nullptr, &resolvableName, &resolving, selectedFunctions);
				}
				if (result.Count() > 0)
				{
					return true;
				}
			}
		}
		return false;
	}

	void Visit(PostfixUnaryExpr* self)override
	{
		ExprTsysItem extraParam(nullptr, ExprTsysType::PRValue, pa.tsys->Int());

		ExprTsysList types;
		ExprToTsys(pa, self->operand, types, gaContext);
		for (vint i = 0; i < types.Count(); i++)
		{
			auto type = types[i].tsys;
			TsysCV cv;
			TsysRefType refType;
			auto entity = type->GetEntity(cv, refType);

			if (entity->IsUnknownType())
			{
				AddTemp(result, pa.tsys->Any());
			}
			else if (entity->GetType() == TsysType::Decl)
			{
				VisitOperator(&types[i], &extraParam, self->opName.name, self->opName, self->opResolving);
			}
			else if (entity->GetType()==TsysType::Primitive)
			{
				auto primitive = entity->GetPrimitive();
				switch (primitive.type)
				{
				case TsysPrimitiveType::Bool:
					AddTemp(result, type->LRefOf());
					break;
				default:
					AddTemp(result, type);
				}
			}
			else if (entity->GetType() == TsysType::Ptr)
			{
				AddTemp(result, entity);
			}
		}
	}

	void Visit(PrefixUnaryExpr* self)override
	{
		ExprTsysList types;
		if (self->op == CppPrefixUnaryOp::AddressOf)
		{
			if (auto childExpr = self->operand.Cast<ChildExpr>())
			{
				if (childExpr->resolving)
				{
					for (vint i = 0; i < childExpr->resolving->resolvedSymbols.Count(); i++)
					{
						VisitSymbol(pa, nullptr, childExpr->resolving->resolvedSymbols[i], true, types);
					}
				}
				goto SKIP_RESOLVING_OPERAND;
			}
		}
		ExprToTsys(pa, self->operand, types, gaContext);

	SKIP_RESOLVING_OPERAND:
		for (vint i = 0; i < types.Count(); i++)
		{
			auto type = types[i].tsys;
			TsysCV cv;
			TsysRefType refType;
			auto entity = type->GetEntity(cv, refType);

			if (entity->IsUnknownType())
			{
				AddTemp(result, pa.tsys->Any());
				continue;
			}
			else if (entity->GetType() == TsysType::Decl)
			{
				if (VisitOperator(&types[i], nullptr, self->opName.name, self->opName, self->opResolving))
				{
					break;
				}
			}

			switch (self->op)
			{
			case CppPrefixUnaryOp::Increase:
			case CppPrefixUnaryOp::Decrease:
				AddTemp(result, type->LRefOf());
				break;
			case CppPrefixUnaryOp::Revert:
			case CppPrefixUnaryOp::Positive:
			case CppPrefixUnaryOp::Negative:
				if (entity->GetType() == TsysType::Primitive)
				{
					auto primitive = entity->GetPrimitive();
					Promote(primitive);

					auto promotedEntity = pa.tsys->PrimitiveOf(primitive);
					if (promotedEntity == entity && primitive.type != TsysPrimitiveType::Float)
					{
						AddTemp(result, pa.tsys->PrimitiveOf(primitive)->CVOf(cv));
					}
					else
					{
						AddTemp(result, pa.tsys->PrimitiveOf(primitive));
					}
				}
				break;
			case CppPrefixUnaryOp::Not:
				AddTemp(result, pa.tsys->PrimitiveOf({ TsysPrimitiveType::Bool, TsysBytes::_1 }));
				break;
			case CppPrefixUnaryOp::AddressOf:
				if (entity->GetType() == TsysType::Ptr && types[i].type == ExprTsysType::PRValue)
				{
					if (entity->GetElement()->GetType() == TsysType::Member)
					{
						if (self->operand.Cast<ChildExpr>())
						{
							AddTemp(result, type);
						}
					}
					else if(entity->GetElement()->GetType() == TsysType::Function)
					{
						if (self->operand.Cast<ChildExpr>() || self->operand.Cast<IdExpr>())
						{
							AddTemp(result, type);
						}
					}
				}
				else if (type->GetType() == TsysType::LRef || type->GetType() == TsysType::RRef)
				{
					AddTemp(result, type->GetElement()->PtrOf());
				}
				else
				{
					AddTemp(result, type->PtrOf());
				}
				break;
			case CppPrefixUnaryOp::Dereference:
				if (entity->GetType() == TsysType::Ptr || entity->GetType() == TsysType::Array)
				{
					AddTemp(result, entity->GetElement()->LRefOf());
				}
				break;
			}
		}
	}

	void Visit(BinaryExpr* self)override
	{
		ExprTsysList leftTypes, rightTypes;
		ExprToTsys(pa, self->left, leftTypes, gaContext);
		ExprToTsys(pa, self->right, rightTypes, gaContext);

		for (vint i = 0; i < leftTypes.Count(); i++)
		{
			auto leftType = leftTypes[i].tsys;
			TsysCV leftCV;
			TsysRefType leftRefType;
			auto leftEntity = leftType->GetEntity(leftCV, leftRefType);

			for (vint j = 0; j < rightTypes.Count(); j++)
			{
				auto rightType = rightTypes[j].tsys;
				TsysCV rightCV;
				TsysRefType rightRefType;
				auto rightEntity = rightType->GetEntity(rightCV, rightRefType);

				if (self->op == CppBinaryOp::ValueFieldDeref || self->op == CppBinaryOp::PtrFieldDeref)
				{
					if (rightEntity->GetType() == TsysType::Ptr && rightEntity->GetElement()->GetType() == TsysType::Member)
					{
						auto fieldEntity = rightEntity->GetElement()->GetElement();
						if (fieldEntity->GetType() == TsysType::Function)
						{
							AddInternal(result, { nullptr,ExprTsysType::PRValue,fieldEntity->PtrOf() });
						}
						else
						{
							ExprTsysItem parentItem = leftTypes[i];
							if (self->op == CppBinaryOp::PtrFieldDeref && leftEntity->GetType() == TsysType::Ptr)
							{
								parentItem = { nullptr,ExprTsysType::LValue,leftEntity->GetElement()->LRefOf() };
							}
							CalculateValueFieldType(&parentItem, nullptr, fieldEntity, true, result);
						}
					}
					else if (rightEntity->IsUnknownType())
					{
						AddTemp(result, pa.tsys->Any());
					}
					continue;
				}

				if (VisitOperator(&leftTypes[i], &rightTypes[i], self->opName.name, self->opName, self->opResolving))
				{
					break;
				}

				if (self->op == CppBinaryOp::Comma)
				{
					AddInternal(result, rightTypes);
					continue;
				}

				auto leftPrim = leftEntity->GetType() == TsysType::Primitive;
				auto rightPrim = rightEntity->GetType() == TsysType::Primitive;
				auto leftPtrArr = leftEntity->GetType() == TsysType::Ptr || leftEntity->GetType() == TsysType::Array;
				auto rightPtrArr = rightEntity->GetType() == TsysType::Ptr || rightEntity->GetType() == TsysType::Array;

				if (leftPrim && rightPrim)
				{
					switch (self->op)
					{
					case CppBinaryOp::LT:
					case CppBinaryOp::GT:
					case CppBinaryOp::LE:
					case CppBinaryOp::GE:
					case CppBinaryOp::EQ:
					case CppBinaryOp::NE:
					case CppBinaryOp::And:
					case CppBinaryOp::Or:
						AddTemp(result, pa.tsys->PrimitiveOf({ TsysPrimitiveType::Bool,TsysBytes::_1 }));
						break;
					case CppBinaryOp::Assign:
					case CppBinaryOp::MulAssign:
					case CppBinaryOp::DivAssign:
					case CppBinaryOp::ModAssign:
					case CppBinaryOp::AddAssign:
					case CppBinaryOp::SubAddisn:
					case CppBinaryOp::ShlAssign:
					case CppBinaryOp::ShrAssign:
					case CppBinaryOp::AndAssign:
					case CppBinaryOp::OrAssign:
					case CppBinaryOp::XorAssign:
						AddTemp(result, leftType->LRefOf());
						break;
					case CppBinaryOp::Shl:
					case CppBinaryOp::Shr:
						{
							auto primitive = leftEntity->GetPrimitive();
							Promote(primitive);
							if (primitive.type == TsysPrimitiveType::UChar)
							{
								primitive.type = TsysPrimitiveType::UInt;
							}
							AddTemp(result, pa.tsys->PrimitiveOf(primitive));
						}
						break;
					default:
						{
							auto leftP = leftEntity->GetPrimitive();
							auto rightP = rightEntity->GetPrimitive();
							auto primitive = ArithmeticConversion(leftP, rightP);
							AddTemp(result, pa.tsys->PrimitiveOf(primitive));
						}
					}
				}
				else if (leftPrim && rightPtrArr)
				{
					AddTemp(result, rightEntity->GetElement()->PtrOf());
				}
				else if (leftPtrArr && rightPrim)
				{
					AddTemp(result, leftEntity->GetElement()->PtrOf());
				}
				else if (leftPtrArr && rightPtrArr)
				{
					AddTemp(result, pa.tsys->IntPtr());
				}
			}
		}
	}

	void Visit(IfExpr* self)override
	{
		ExprTsysList conditionTypes, leftTypes, rightTypes;
		ExprToTsys(pa, self->condition, conditionTypes, gaContext);
		ExprToTsys(pa, self->left, leftTypes, gaContext);
		ExprToTsys(pa, self->right, rightTypes, gaContext);

		if (leftTypes.Count() == 0 && rightTypes.Count()!=0)
		{
			AddInternal(result, rightTypes);
		}
		else if (leftTypes.Count() != 0 && rightTypes.Count() == 0)
		{
			AddInternal(result, leftTypes);
		}
		else
		{
			for (vint i = 0; i < leftTypes.Count(); i++)
			{
				auto left = leftTypes[i];
				auto leftType = left.type == ExprTsysType::LValue ? left.tsys->LRefOf() : left.tsys;
				TsysCV leftCV;
				TsysRefType leftRefType;
				auto leftEntity = leftType->GetEntity(leftCV, leftRefType);

				for (vint j = 0; j < rightTypes.Count(); j++)
				{
					auto right = rightTypes[j];
					auto rightType = right.type == ExprTsysType::LValue ? right.tsys->LRefOf() : right.tsys;
					TsysCV rightCV;
					TsysRefType rightRefType;
					auto rightEntity = rightType->GetEntity(rightCV, rightRefType);

					if (leftType == rightType)
					{
						AddTemp(result, leftType);
					}
					else if (leftEntity == rightEntity)
					{
						auto cv = leftCV;
						cv.isGeneralConst |= rightCV.isGeneralConst;
						cv.isVolatile |= rightCV.isVolatile;

						auto refType = leftRefType == rightRefType ? leftRefType : TsysRefType::None;

						switch (refType)
						{
						case TsysRefType::LRef:
							AddTemp(result, leftEntity->CVOf(cv)->LRefOf());
							break;
						case TsysRefType::RRef:
							AddTemp(result, leftEntity->CVOf(cv)->RRefOf());
							break;
						default:
							AddTemp(result, leftEntity->CVOf(cv));
							break;
						}
					}
					else
					{
						auto l2r = TestConvert(pa, rightType, left);
						auto r2l = TestConvert(pa, leftType, right);
						if (l2r < r2l)
						{
							AddTemp(result, rightType);
						}
						else if (l2r > r2l)
						{
							AddTemp(result, leftType);
						}
						else
						{
							auto leftPrim = leftEntity->GetType() == TsysType::Primitive;
							auto rightPrim = rightEntity->GetType() == TsysType::Primitive;
							auto leftPtrArr = leftEntity->GetType() == TsysType::Ptr || leftEntity->GetType() == TsysType::Array;
							auto rightPtrArr = rightEntity->GetType() == TsysType::Ptr || rightEntity->GetType() == TsysType::Array;
							auto leftNull = leftEntity->GetType() == TsysType::Zero || leftEntity->GetType() == TsysType::Nullptr;
							auto rightNull = rightEntity->GetType() == TsysType::Zero || rightEntity->GetType() == TsysType::Nullptr;

							if (l2r == TsysConv::StandardConversion && leftPrim && rightPrim)
							{
								auto leftP = leftEntity->GetPrimitive();
								auto rightP = rightEntity->GetPrimitive();
								auto primitive = ArithmeticConversion(leftP, rightP);
								AddTemp(result, pa.tsys->PrimitiveOf(primitive));
								continue;
							}

							if (leftPtrArr && rightNull)
							{
								AddTemp(result, leftEntity->GetElement()->PtrOf());
								continue;
							}

							if (leftNull && rightPtrArr)
							{
								AddTemp(result, rightEntity->GetElement()->PtrOf());
								continue;
							}

							AddInternal(result, left);
							AddInternal(result, right);
						}
					}
				}
			}
		}
	}

	void Visit(GenericExpr* self)override
	{
		ExprTsysList genericTypes;
		Array<Ptr<TypeTsysList>> argumentTypes;

		ExprToTsys(pa, self->expr, genericTypes, gaContext);
		symbol_type_resolving::ResolveGenericArguments(pa, self->arguments, argumentTypes, gaContext);

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
				if (!symbol_type_resolving::ResolveGenericParameters(pa, genericFunction, argumentTypes, &esContext.gaContext))
				{
					throw NotConvertableException();
				}

				switch (declSymbol->kind)
				{
				case symbol_component::SymbolKind::ValueAlias:
					{
						auto decl = declSymbol->definition.Cast<UsingDeclaration>();
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
void ExprToTsys(const ParsingArguments& pa, Ptr<Expr> e, ExprTsysList& tsys, GenericArgContext* gaContext)
{
	if (!e) throw IllegalExprException();
	ExprToTsysVisitor visitor(pa, tsys, gaContext);
	e->Accept(&visitor);
}