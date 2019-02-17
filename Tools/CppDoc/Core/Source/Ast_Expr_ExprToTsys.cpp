#include "Ast_Expr_Resolving.h"
#include "Ast_Expr.h"

using namespace symbol_type_resolving;

/***********************************************************************
ExprToTsys
***********************************************************************/

class ExprToTsysVisitor : public Object, public virtual IExprVisitor
{
public:
	ExprTsysList&			result;
	ParsingArguments&		pa;

	ExprToTsysVisitor(ParsingArguments& _pa, ExprTsysList& _result)
		:pa(_pa)
		, result(_result)
	{
	}

	/***********************************************************************
	Expressions
	***********************************************************************/

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
				wchar_t _1 = token.length > 1 ? token.reading[token.length - 2] : 0;
				wchar_t _2 = token.reading[token.length - 1];
				bool u = _1 == L'u' || _1 == L'U' || _2 == L'u' || _2 == L'U';
				bool l = _1 == L'l' || _1 == L'L' || _2 == L'l' || _2 == L'L';
				AddTemp(result, pa.tsys->PrimitiveOf({ (u ? TsysPrimitiveType::UInt : TsysPrimitiveType::SInt),{l ? TsysBytes::_8 : TsysBytes::_4} }));
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
		ExprToTsys(pa, self->expr, types);
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
			ExprToTsys(pa, self->expr, types);
		}
		{
			TypeTsysList types;
			TypeToTsys(pa, self->type, types);
			AddTemp(result, types);
		}
	}

	void Visit(TypeidExpr* self)override
	{
		if (self->type)
		{
			TypeTsysList types;
			TypeToTsys(pa, self->type, types);
		}
		if (self->expr)
		{
			ExprTsysList types;
			ExprToTsys(pa, self->expr, types);
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
			if (ti->decls.Count() > 0)
			{
				if (ti->decls[0].Cast<ClassDeclaration>())
				{
					AddInternal(result, { nullptr,ExprTsysType::LValue,pa.tsys->DeclOf(ti.Obj()) });
					return;
				}
			}
		}
	}

	void Visit(SizeofExpr* self)override
	{
		if (self->type)
		{
			TypeTsysList types;
			TypeToTsys(pa, self->type, types);
		}
		if (self->expr)
		{
			ExprTsysList types;
			ExprToTsys(pa, self->expr, types);
		}

		AddTemp(result, pa.tsys->Size());
	}

	void Visit(ThrowExpr* self)override
	{
		if (self->expr)
		{
			ExprTsysList types;
			ExprToTsys(pa, self->expr, types);
		}

		AddTemp(result, pa.tsys->Void());
	}

	void Visit(DeleteExpr* self)override
	{
		{
			ExprTsysList types;
			ExprToTsys(pa, self->expr, types);
		}

		AddTemp(result, pa.tsys->Void());
	}

	void VisitResolvableExpr(ResolvableExpr* self)
	{
		if (self->resolving)
		{
			if (pa.funcSymbol && pa.funcSymbol->methodCache)
			{
				TsysCV thisCv;
				TsysRefType thisRef;
				auto thisType = pa.funcSymbol->methodCache->thisType->GetEntity(thisCv, thisRef);
				ExprTsysItem thisItem(nullptr, ExprTsysType::LValue, thisType->GetElement()->LRefOf());
				VisitResolvedMember(pa, &thisItem, self->resolving, result);
			}
			else
			{
				VisitResolvedMember(pa, nullptr, self->resolving, result);
			}
		}
	}

	void Visit(IdExpr* self)override
	{
		VisitResolvableExpr(self);
	}

	void Visit(ChildExpr* self)override
	{
		VisitResolvableExpr(self);
	}

	void Visit(FieldAccessExpr* self)override
	{
		ResolveSymbolResult totalRar;
		ExprTsysList parentItems;
		ExprToTsys(pa, self->expr, parentItems);

		auto childExpr = self->name.Cast<ChildExpr>();
		auto idExpr = self->name.Cast<IdExpr>();

		if (self->type == CppFieldAccessType::Dot)
		{
			for (vint i = 0; i < parentItems.Count(); i++)
			{
				if (idExpr)
				{
					VisitDirectField(pa, totalRar, parentItems[i], idExpr->name, result);
				}
				else if (childExpr->resolving)
				{
					VisitResolvedMember(pa, &parentItems[i], childExpr->resolving, result);
				}
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
					if (idExpr)
					{
						VisitDirectField(pa, totalRar, parentItem, idExpr->name, result);
					}
					else if (childExpr->resolving)
					{
						VisitResolvedMember(pa, &parentItem, childExpr->resolving, result);
					}
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

		if (idExpr)
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
			ExprToTsys(pa, self->index, *argTypes.Obj());
			argTypesList.Add(argTypes);
		}

		ExprTsysList arrayTypes, funcTypes;
		ExprToTsys(pa, self->expr, arrayTypes);

		for (vint i = 0; i < arrayTypes.Count(); i++)
		{
			auto arrayType = arrayTypes[i];

			TsysCV cv;
			TsysRefType refType;
			auto entityType = arrayType.tsys->GetEntity(cv, refType);

			if (entityType->GetType() == TsysType::Decl)
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

		VisitOverloadedFunction(pa, funcTypes, argTypesList, result);
	}

	void Visit(FuncAccessExpr* self)override
	{
		List<Ptr<ExprTsysList>> argTypesList;
		for (vint i = 0; i < self->arguments.Count(); i++)
		{
			auto argTypes = MakePtr<ExprTsysList>();
			ExprToTsys(pa, self->arguments[i], *argTypes.Obj());
			argTypesList.Add(argTypes);
		}

		{
			ExprTsysList funcTypes;
			ExprToTsys(pa, self->expr, funcTypes);

			FindQualifiedFunctors(pa, {}, TsysRefType::None, funcTypes, true);
			VisitOverloadedFunction(pa, funcTypes, argTypesList, result);
		}
	}

	void Visit(CtorAccessExpr* self)override
	{
		List<Ptr<ExprTsysList>> argTypesList;
		for (vint i = 0; i < self->initializer->arguments.Count(); i++)
		{
			auto argTypes = MakePtr<ExprTsysList>();
			ExprToTsys(pa, self->initializer->arguments[i], *argTypes.Obj());
			argTypesList.Add(argTypes);
		}

		{
			TypeTsysList types;
			TypeToTsys(pa, self->type, types);
			AddTemp(result, types);
		}
	}

	void Visit(NewExpr* self)override
	{
		for (vint i = 0; i < self->placementArguments.Count(); i++)
		{
			ExprTsysList types;
			ExprToTsys(pa, self->placementArguments[i], types);
		}

		if (self->initializer)
		{
			for (vint i = 0; i < self->initializer->arguments.Count(); i++)
			{
				ExprTsysList types;
				ExprToTsys(pa, self->initializer->arguments[i], types);
			}
		}

		TypeTsysList types;
		TypeToTsys(pa, self->type, types);
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

	void CreateUniversalInitializerType(List<Ptr<ExprTsysList>>& argTypesList, Array<vint>& indices, vint index)
	{
		if (index == argTypesList.Count())
		{
			Array<ExprTsysItem> params(index);
			for (vint i = 0; i < index; i++)
			{
				params[i] = argTypesList[i]->Get(indices[i]);
			}
			AddInternal(result, { nullptr,ExprTsysType::PRValue,pa.tsys->InitOf(params) });
		}
		else
		{
			for (vint i = 0; i < argTypesList[index]->Count(); i++)
			{
				indices[index] = i;
				CreateUniversalInitializerType(argTypesList, indices, index + 1);
			}
		}
	}

	void Visit(UniversalInitializerExpr* self)override
	{
		List<Ptr<ExprTsysList>> argTypesList;
		for (vint i = 0; i < self->arguments.Count(); i++)
		{
			auto argTypes = MakePtr<ExprTsysList>();
			ExprToTsys(pa, self->arguments[i], *argTypes.Obj());
			argTypesList.Add(argTypes);
		}

		Array<vint> indices(argTypesList.Count());
		CreateUniversalInitializerType(argTypesList, indices, 0);
	}

	bool VisitOperator(ExprTsysItem* leftType, ExprTsysItem* rightType, const WString& name)
	{
		TsysCV leftCV, rightCV;
		TsysRefType leftRef, rightRef;
		auto leftEntity = leftType->tsys->GetEntity(leftCV, leftRef);
		auto rightEntity = rightType ? rightType->tsys->GetEntity(rightCV, rightRef) : nullptr;

		CppName opName;
		opName.type = CppNameType::Operator;
		opName.name = L"operator " + name;

		if (leftEntity->GetType() == TsysType::Decl)
		{
			ParsingArguments newPa(pa, leftEntity->GetDecl());
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
				VisitOverloadedFunction(pa, opTypes, argTypesList, result);
				return true;
			}
		}
		if (rightEntity && rightEntity->GetType() == TsysType::Decl)
		{
			ParsingArguments newPa(pa, rightEntity->GetDecl());
			auto opMethods = ResolveSymbol(newPa, opName, SearchPolicy::ChildSymbol);

			if (opMethods.values)
			{
				ExprTsysList opTypes;
				for (vint j = 0; j < opMethods.values->resolvedSymbols.Count(); j++)
				{
					VisitSymbol(pa, leftType, opMethods.values->resolvedSymbols[j], false, opTypes);
				}
				FilterFieldsAndBestQualifiedFunctions(rightCV, rightRef, opTypes);

				List<Ptr<ExprTsysList>> argTypesList;
				{
					argTypesList.Add(MakePtr<ExprTsysList>());
					AddInternal(*argTypesList[0].Obj(), *leftType);
				}
				FindQualifiedFunctors(pa, {}, TsysRefType::None, opTypes, false);
				VisitOverloadedFunction(pa, opTypes, argTypesList, result);
				return true;
			}
		}
		{
			auto opFuncs = ResolveSymbol(pa, opName, SearchPolicy::SymbolAccessableInScope);
			if (opFuncs.values)
			{
				ExprTsysList opTypes;
				for (vint j = 0; j < opFuncs.values->resolvedSymbols.Count(); j++)
				{
					VisitSymbol(pa, leftType, opFuncs.values->resolvedSymbols[j], false, opTypes);
				}

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
				VisitOverloadedFunction(pa, opTypes, argTypesList, result);
				return true;
			}
		}
		{
			SortedList<Symbol*> nss, classes;
			SearchAdlClassesAndNamespaces(pa, leftEntity, nss, classes);
			if (rightEntity)
			{
				SearchAdlClassesAndNamespaces(pa, rightEntity, nss, classes);
			}

			ExprTsysList funcTypes;
			SerachAdlFunction(pa, nss, opName.name, funcTypes);

			if (funcTypes.Count())
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
				VisitOverloadedFunction(pa, funcTypes, argTypesList, result);
				return true;
			}
		}
		return false;
	}

	void Visit(PostfixUnaryExpr* self)override
	{
		ExprTsysItem extraParam(nullptr, ExprTsysType::PRValue, pa.tsys->Int());

		ExprTsysList types;
		ExprToTsys(pa, self->operand, types);
		for (vint i = 0; i < types.Count(); i++)
		{
			auto type = types[i].tsys;
			TsysCV cv;
			TsysRefType refType;
			auto entity = type->GetEntity(cv, refType);

			if (entity->GetType() == TsysType::Decl)
			{
				ResolveSymbolResult opMethods, opFuncs;
				{
					CppName opName = self->opName;
					opName.name = L"operator " + opName.name;
					ParsingArguments newPa(pa, entity->GetDecl());
					opMethods = ResolveSymbol(newPa, opName, SearchPolicy::ChildSymbol, opMethods);
				}
				{
					CppName opName = self->opName;
					opName.name = L"operator " + opName.name;
					ParsingArguments newPa(pa, entity->GetDecl()->parent);
					opFuncs = ResolveSymbol(newPa, opName, SearchPolicy::ChildSymbol, opFuncs);
				}
				{
					CppName opName = self->opName;
					opName.name = L"operator " + opName.name;
					opFuncs = ResolveSymbol(pa, opName, SearchPolicy::SymbolAccessableInScope, opFuncs);
				}

				if (opMethods.values)
				{
					ExprTsysList opTypes;
					for (vint j = 0; j < opMethods.values->resolvedSymbols.Count(); j++)
					{
						VisitSymbol(pa, &types[i], opMethods.values->resolvedSymbols[j], false, opTypes);
					}
					FilterFieldsAndBestQualifiedFunctions(cv, refType, opTypes);

					List<Ptr<ExprTsysList>> argTypesList;
					argTypesList.Add(MakePtr<ExprTsysList>());
					AddInternal(*argTypesList[0].Obj(), { nullptr,ExprTsysType::PRValue,pa.tsys->Int() });
					FindQualifiedFunctors(pa, {}, TsysRefType::None, opTypes, false);
					VisitOverloadedFunction(pa, opTypes, argTypesList, result);
				}

				if (opFuncs.values)
				{
					ExprTsysList opTypes;
					for (vint j = 0; j < opFuncs.values->resolvedSymbols.Count(); j++)
					{
						VisitSymbol(pa, nullptr, opFuncs.values->resolvedSymbols[j], true, opTypes);
					}

					List<Ptr<ExprTsysList>> argTypesList;
					argTypesList.Add(MakePtr<ExprTsysList>());
					argTypesList.Add(MakePtr<ExprTsysList>());
					AddInternal(*argTypesList[0].Obj(), types[i]);
					AddInternal(*argTypesList[1].Obj(), { nullptr,ExprTsysType::PRValue,pa.tsys->Int() });
					FindQualifiedFunctors(pa, {}, TsysRefType::None, opTypes, false);
					VisitOverloadedFunction(pa, opTypes, argTypesList, result);
				}
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
		ExprToTsys(pa, self->operand, types);

	SKIP_RESOLVING_OPERAND:
		for (vint i = 0; i < types.Count(); i++)
		{
			auto type = types[i].tsys;
			TsysCV cv;
			TsysRefType refType;
			auto entity = type->GetEntity(cv, refType);

			if (entity->GetType() == TsysType::Decl)
			{
				ResolveSymbolResult opMethods, opFuncs;
				{
					CppName opName = self->opName;
					opName.name = L"operator " + opName.name;
					ParsingArguments newPa(pa, entity->GetDecl());
					opMethods = ResolveSymbol(newPa, opName, SearchPolicy::ChildSymbol, opMethods);
				}
				{
					CppName opName = self->opName;
					opName.name = L"operator " + opName.name;
					ParsingArguments newPa(pa, entity->GetDecl()->parent);
					opFuncs = ResolveSymbol(newPa, opName, SearchPolicy::ChildSymbol, opFuncs);
				}
				{
					CppName opName = self->opName;
					opName.name = L"operator " + opName.name;
					opFuncs = ResolveSymbol(pa, opName, SearchPolicy::SymbolAccessableInScope, opFuncs);
				}

				if (opMethods.values)
				{
					ExprTsysList opTypes;
					for (vint j = 0; j < opMethods.values->resolvedSymbols.Count(); j++)
					{
						VisitSymbol(pa, &types[i], opMethods.values->resolvedSymbols[j], false, opTypes);
					}
					FilterFieldsAndBestQualifiedFunctions(cv, refType, opTypes);

					List<Ptr<ExprTsysList>> argTypesList;
					FindQualifiedFunctors(pa, {}, TsysRefType::None, opTypes, false);
					VisitOverloadedFunction(pa, opTypes, argTypesList, result);
				}

				if (opFuncs.values)
				{
					ExprTsysList opTypes;
					for (vint j = 0; j < opFuncs.values->resolvedSymbols.Count(); j++)
					{
						VisitSymbol(pa, nullptr, opFuncs.values->resolvedSymbols[j], true, opTypes);
					}

					List<Ptr<ExprTsysList>> argTypesList;
					argTypesList.Add(MakePtr<ExprTsysList>());
					AddInternal(*argTypesList[0].Obj(), types[i]);
					FindQualifiedFunctors(pa, {}, TsysRefType::None, opTypes, false);
					VisitOverloadedFunction(pa, opTypes, argTypesList, result);
				}

				if (opMethods.values || opFuncs.values)
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
		ExprToTsys(pa, self->left, leftTypes);
		ExprToTsys(pa, self->right, rightTypes);

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
					continue;
				}

				if (leftEntity->GetType() == TsysType::Decl || rightEntity->GetType() == TsysType::Decl)
				{
					ResolveSymbolResult opMethods, opFuncs;
					if (leftEntity->GetType() == TsysType::Decl)
					{
						{
							CppName opName = self->opName;
							opName.name = L"operator " + opName.name;
							ParsingArguments newPa(pa, leftEntity->GetDecl());
							opMethods = ResolveSymbol(newPa, opName, SearchPolicy::ChildSymbol, opMethods);
						}
						{
							CppName opName = self->opName;
							opName.name = L"operator " + opName.name;
							ParsingArguments newPa(pa, leftEntity->GetDecl()->parent);
							opFuncs = ResolveSymbol(newPa, opName, SearchPolicy::ChildSymbol, opFuncs);
						}
					}
					if (rightEntity->GetType() == TsysType::Decl)
					{
						{
							CppName opName = self->opName;
							opName.name = L"operator " + opName.name;
							ParsingArguments newPa(pa, rightEntity->GetDecl()->parent);
							opFuncs = ResolveSymbol(newPa, opName, SearchPolicy::ChildSymbol, opFuncs);
						}
					}
					{
						CppName opName = self->opName;
						opName.name = L"operator " + opName.name;
						opFuncs = ResolveSymbol(pa, opName, SearchPolicy::SymbolAccessableInScope, opFuncs);
					}

					if (opMethods.values)
					{
						ExprTsysList opTypes;
						for (vint k = 0; k < opMethods.values->resolvedSymbols.Count(); k++)
						{
							VisitSymbol(pa, &leftTypes[i], opMethods.values->resolvedSymbols[k], false, opTypes);
						}
						FilterFieldsAndBestQualifiedFunctions(leftCV, leftRefType, opTypes);

						List<Ptr<ExprTsysList>> argTypesList;
						argTypesList.Add(MakePtr<ExprTsysList>());
						AddInternal(*argTypesList[0].Obj(), rightTypes[j]);
						FindQualifiedFunctors(pa, {}, TsysRefType::None, opTypes, false);
						VisitOverloadedFunction(pa, opTypes, argTypesList, result);
					}

					if (opFuncs.values)
					{
						ExprTsysList opTypes;
						for (vint k = 0; k < opFuncs.values->resolvedSymbols.Count(); k++)
						{
							VisitSymbol(pa, nullptr, opFuncs.values->resolvedSymbols[k], true, opTypes);
						}

						List<Ptr<ExprTsysList>> argTypesList;
						argTypesList.Add(MakePtr<ExprTsysList>());
						argTypesList.Add(MakePtr<ExprTsysList>());
						AddInternal(*argTypesList[0].Obj(), leftTypes[i]);
						AddInternal(*argTypesList[1].Obj(), rightTypes[j]);
						FindQualifiedFunctors(pa, {}, TsysRefType::None, opTypes, false);
						VisitOverloadedFunction(pa, opTypes, argTypesList, result);
					}

					if (opMethods.values || opFuncs.values)
					{
						break;
					}
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
		ExprToTsys(pa, self->condition, conditionTypes);
		ExprToTsys(pa, self->left, leftTypes);
		ExprToTsys(pa, self->right, rightTypes);

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
};

// Resolve expressions to types
void ExprToTsys(ParsingArguments& pa, Ptr<Expr> e, ExprTsysList& tsys)
{
	if (!e) throw IllegalExprException();
	ExprToTsysVisitor visitor(pa, tsys);
	e->Accept(&visitor);
}