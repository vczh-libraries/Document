#include "Ast.h"
#include "Ast_Expr.h"
#include "Ast_Decl.h"
#include "Ast_Type.h"
#include "Parser.h"

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
	Add: Add something to ExprTsysList
	***********************************************************************/

	static bool AddInternal(ExprTsysList& list, const ExprTsysItem& item)
	{
		if (list.Contains(item)) return false;
		list.Add(item);
		return true;
	}

	static void AddInternal(ExprTsysList& list, ExprTsysList& items)
	{
		for (vint i = 0; i < items.Count(); i++)
		{
			AddInternal(list, items[i]);
		}
	}

	static bool AddVar(ExprTsysList& list, const ExprTsysItem& item)
	{
		return AddInternal(list, { item.symbol,ExprTsysType::LValue,item.tsys });
	}

	static bool AddNonVar(ExprTsysList& list, const ExprTsysItem& item)
	{
		return AddInternal(list, { item.symbol,ExprTsysType::PRValue,item.tsys });
	}

	static void AddNonVar(ExprTsysList& list, ExprTsysList& items)
	{
		for (vint i = 0; i < items.Count(); i++)
		{
			AddNonVar(list, items[i]);
		}
	}

	static bool AddTemp(ExprTsysList& list, ITsys* tsys)
	{
		if (tsys->GetType() == TsysType::RRef)
		{
			return AddInternal(list, { nullptr,ExprTsysType::XValue,tsys });
		}
		else if (tsys->GetType() == TsysType::LRef)
		{
			return AddInternal(list, { nullptr,ExprTsysType::LValue,tsys });
		}
		else
		{
			return AddInternal(list, { nullptr,ExprTsysType::PRValue,tsys });
		}
	}

	static void AddTemp(ExprTsysList& list, TypeTsysList& items)
	{
		for (vint i = 0; i < items.Count(); i++)
		{
			AddTemp(list, items[i]);
		}
	}

	/***********************************************************************
	IsStaticSymbol: Test if a symbol is a static class member
	***********************************************************************/

	template<typename TForward>
	static bool IsStaticSymbol(Symbol* symbol, Ptr<TForward> decl)
	{
		if (auto rootDecl = decl.Cast<typename TForward::ForwardRootType>())
		{
			if (rootDecl->decoratorStatic)
			{
				return true;
			}
			else
			{
				for (vint i = 0; i < symbol->forwardDeclarations.Count(); i++)
				{
					auto forwardSymbol = symbol->forwardDeclarations[i];
					for (vint j = 0; j < forwardSymbol->decls.Count(); j++)
					{
						if (IsStaticSymbol<TForward>(forwardSymbol, forwardSymbol->decls[j].Cast<TForward>()))
						{
							return true;
						}
					}
				}
				return false;
			}
		}
		else
		{
			return decl->decoratorStatic;
		}
	}

	/***********************************************************************
	VisitSymbol: Fill a symbol to ExprTsysList
		thisItem: When afterScope==false
			it represents typeof(x) in x.name or typeof(&x) in x->name
			it could be null if it is initiated by IdExpr
	***********************************************************************/

	static void VisitSymbol(ParsingArguments& pa, const ExprTsysItem* thisItem, Symbol* symbol, bool afterScope, ExprTsysList& result)
	{
		ITsys* classScope = nullptr;
		if (symbol->parent && symbol->parent->decls.Count() > 0)
		{
			if (auto decl = symbol->parent->decls[0].Cast<ClassDeclaration>())
			{
				classScope = pa.tsys->DeclOf(symbol->parent);
			}
		}

		if (!symbol->forwardDeclarationRoot)
		{
			for (vint j = 0; j < symbol->decls.Count(); j++)
			{
				auto decl = symbol->decls[j];
				if (auto varDecl = decl.Cast<ForwardVariableDeclaration>())
				{
					bool isStaticSymbol = IsStaticSymbol<ForwardVariableDeclaration>(symbol, varDecl);

					TypeTsysList candidates;
					if (varDecl->needResolveTypeFromInitializer)
					{
						if (!symbol->resolvedTypes)
						{
							symbol->resolvedTypes = MakePtr<TypeTsysList>();
							if (auto rootVarDecl = varDecl.Cast<VariableDeclaration>())
							{
								auto declType = MakePtr<DeclType>();
								declType->expr = rootVarDecl->initializer->arguments[0];

								TypeTsysList types;
								ParsingArguments newPa(pa, symbol->parent);
								TypeToTsys(newPa, declType, types);

								for (vint k = 0; k < types.Count(); k++)
								{
									auto type = types[k];

									if (auto primitiveType = varDecl->type.Cast<PrimitiveType>())
									{
										TsysCV cv;
										TsysRefType refType;
										type = type->GetEntity(cv, refType);
									}

									if (!symbol->resolvedTypes->Contains(type))
									{
										symbol->resolvedTypes->Add(type);
									}
								}
							}
						}
						CopyFrom(candidates, *symbol->resolvedTypes.Obj());
					}
					else
					{
						TypeToTsys(pa, varDecl->type, candidates);
					}

					for (vint k = 0; k < candidates.Count(); k++)
					{
						auto tsys = candidates[k];
						if (tsys->GetType() == TsysType::Member && tsys->GetClass() == classScope)
						{
							tsys = tsys->GetElement();
						}

						if (isStaticSymbol)
						{
							AddInternal(result, { symbol,ExprTsysType::LValue,tsys });
						}
						else if (afterScope)
						{
							if (classScope)
							{
								AddInternal(result, { symbol,ExprTsysType::PRValue,tsys->MemberOf(classScope)->PtrOf() });
							}
							else
							{
								AddInternal(result, { symbol,ExprTsysType::LValue,tsys });
							}
						}
						else
						{
							if (thisItem)
							{
								TsysCV cv;
								TsysRefType refType;
								thisItem->tsys->GetEntity(cv, refType);
								if (refType == TsysRefType::LRef)
								{
									AddInternal(result, { symbol,ExprTsysType::LValue,tsys->CVOf(cv) });
								}
								else if (refType == TsysRefType::RRef)
								{
									if (thisItem->type == ExprTsysType::LValue)
									{
										AddInternal(result, { symbol,ExprTsysType::LValue,tsys->CVOf(cv) });
									}
									else
									{
										AddInternal(result, { symbol,ExprTsysType::XValue,tsys->CVOf(cv)->RRefOf() });
									}
								}
								else
								{
									AddInternal(result, { symbol,thisItem->type,tsys->CVOf(cv) });
								}
							}
							else
							{
								AddInternal(result, { symbol,ExprTsysType::LValue,tsys });
							}
						}
					}
				}
				else if (auto enumItemDecl = decl.Cast<EnumItemDeclaration>())
				{
					auto tsys = pa.tsys->DeclOf(enumItemDecl->symbol->parent);
					AddInternal(result, { symbol,ExprTsysType::PRValue,tsys });
				}
				else if (auto funcDecl = decl.Cast<ForwardFunctionDeclaration>())
				{
					bool isStaticSymbol = IsStaticSymbol<ForwardFunctionDeclaration>(symbol, funcDecl);
					bool isMember = classScope && !isStaticSymbol;

					TypeTsysList candidates;
					if (funcDecl->needResolveTypeFromStatement)
					{
						throw 0;
					}
					else
					{
						TypeToTsys(pa, funcDecl->type, candidates, TsysCallingConvention::None, isMember);
					}

					for (vint k = 0; k < candidates.Count(); k++)
					{
						auto tsys = candidates[k];
						if (tsys->GetType() == TsysType::Member && tsys->GetClass() == classScope)
						{
							tsys = tsys->GetElement();
						}

						if (isMember && afterScope)
						{
							tsys = tsys->MemberOf(classScope)->PtrOf();
						}
						else
						{
							tsys = tsys->PtrOf();
						}

						AddInternal(result, { symbol,ExprTsysType::PRValue,tsys });
					}
				}
				else
				{
					throw IllegalExprException();
				}
			}
		}
	}

	/***********************************************************************
	VisitNormalField: Fill all members of a name to ExprTsysList
	***********************************************************************/

	static void VisitNormalField(ParsingArguments& pa, CppName& name, ResolveSymbolResult* totalRar, const ExprTsysItem& parentItem, ExprTsysList& result)
	{
		TsysCV cv;
		TsysRefType refType;
		auto entity = parentItem.tsys->GetEntity(cv, refType);
		if (entity->GetType() == TsysType::Decl)
		{
			auto symbol = entity->GetDecl();
			ParsingArguments fieldPa(pa, symbol);
			auto rar = ResolveSymbol(fieldPa, name, SearchPolicy::ChildSymbol);
			if (totalRar) totalRar->Merge(rar);

			if (rar.values)
			{
				for (vint j = 0; j < rar.values->resolvedSymbols.Count(); j++)
				{
					auto symbol = rar.values->resolvedSymbols[j];
					VisitSymbol(pa, &parentItem, symbol, false, result);
				}
			}
		}
	}

	/***********************************************************************
	TestFunctionQualifier: Match this pointer's and functions' qualifiers
		Returns: Exact, TrivalConversion, Illegal
	***********************************************************************/

	static TsysConv TestFunctionQualifier(TsysCV thisCV, TsysRefType thisRef, const ExprTsysItem& funcType)
	{
		if (funcType.symbol && funcType.symbol->decls.Count() > 0)
		{
			if (auto decl = funcType.symbol->decls[0].Cast<ForwardFunctionDeclaration>())
			{
				if (auto declType = GetTypeWithoutMemberAndCC(decl->type).Cast<FunctionType>())
				{
					return ::TestFunctionQualifier(thisCV, thisRef, declType);
				}
			}
		}
		return TsysConv::Exact;
	}

	/***********************************************************************
	FilterFunctionByQualifier: Filter functions by their qualifiers
	***********************************************************************/

	static void FilterFunctionByQualifier(ExprTsysList& funcTypes, ArrayBase<TsysConv>& funcChoices)
	{
		auto target = TsysConv::Illegal;

		for (vint i = 0; i < funcChoices.Count(); i++)
		{
			auto candidate = funcChoices[i];
			if (target > candidate)
			{
				target = candidate;
			}
		}

		if (target == TsysConv::Illegal)
		{
			funcTypes.Clear();
			return;
		}

		for (vint i = funcTypes.Count() - 1; i >= 0; i--)
		{
			if (funcChoices[i] != target)
			{
				funcTypes.RemoveAt(i);
			}
		}
	}

	static void FilterFunctionByQualifier(TsysCV thisCV, TsysRefType thisRef, ExprTsysList& funcTypes)
	{
		Array<TsysConv> funcChoices(funcTypes.Count());

		for (vint i = 0; i < funcTypes.Count(); i++)
		{
			funcChoices[i] = TestFunctionQualifier(thisCV, thisRef, funcTypes[i]);
		}

		FilterFunctionByQualifier(funcTypes, funcChoices);
	}

	/***********************************************************************
	FindQualifiedFunctions: Remove everything that are not qualified functions
	***********************************************************************/

	static void FindQualifiedFunctions(ParsingArguments& pa, TsysCV thisCV, TsysRefType thisRef, ExprTsysList& funcTypes, bool lookForOp)
	{
		ExprTsysList expandedFuncTypes;
		List<TsysConv> funcChoices;

		for (vint i = 0; i < funcTypes.Count(); i++)
		{
			auto funcType = funcTypes[i];
			auto choice = TestFunctionQualifier(thisCV, thisRef, funcType);

			if (choice != TsysConv::Illegal)
			{
				TsysCV cv;
				TsysRefType refType;
				auto entityType = funcType.tsys->GetEntity(cv, refType);

				if (entityType->GetType() == TsysType::Decl && lookForOp)
				{
					CppName opName;
					opName.name = L"operator ()";
					ExprTsysList opResult;
					VisitNormalField(pa, opName, nullptr, funcType, opResult);
					FindQualifiedFunctions(pa, cv, refType, opResult, false);

					vint oldCount = expandedFuncTypes.Count();
					AddNonVar(expandedFuncTypes, opResult);
					vint newCount = expandedFuncTypes.Count();

					for (vint i = 0; i < (newCount - oldCount); i++)
					{
						funcChoices.Add(TsysConv::Exact);
					}
				}
				else if (entityType->GetType() == TsysType::Ptr)
				{
					entityType = entityType->GetElement();
					if (entityType->GetType() == TsysType::Function)
					{
						if (AddInternal(expandedFuncTypes, { funcType.symbol,funcType.type, entityType }))
						{
							funcChoices.Add(choice);
						}
					}
				}
			}
		}

		FilterFunctionByQualifier(expandedFuncTypes, funcChoices);
		CopyFrom(funcTypes, expandedFuncTypes);
	}

	/***********************************************************************
	VisitOverloadedFunction: Select good candidates from overloaded functions
	***********************************************************************/

	static void VisitOverloadedFunction(ParsingArguments& pa, ExprTsysList& funcTypes, List<Ptr<ExprTsysList>>& argTypesList, ExprTsysList& result)
	{
		Array<vint> funcDPs(funcTypes.Count());
		for (vint i = 0; i < funcTypes.Count(); i++)
		{
			auto symbol = funcTypes[i].symbol;
			if (symbol->decls.Count() == 1)
			{
				if (auto decl = symbol->decls[0].Cast<ForwardFunctionDeclaration>())
				{
					if (auto type = GetTypeWithoutMemberAndCC(decl->type).Cast<FunctionType>())
					{
						for (vint j = 0; j < type->parameters.Count(); j++)
						{
							if (type->parameters[j]->initializer)
							{
								funcDPs[i] = type->parameters.Count() - j;
								goto EXAMINE_NEXT_FUNCTION;
							}
						}
						funcDPs[i] = 0;
					EXAMINE_NEXT_FUNCTION:;
					}
				}
			}
		}

		ExprTsysList validFuncTypes;
		for (vint i = 0; i < funcTypes.Count(); i++)
		{
			auto funcType = funcTypes[i];

			vint funcParamCount = funcType.tsys->GetParamCount();
			vint missParamCount = funcParamCount - argTypesList.Count();
			if (missParamCount > 0)
			{
				if (missParamCount > funcDPs[i])
				{
					continue;
				}
			}
			else if (missParamCount < 0)
			{
				if (!funcType.tsys->GetFunc().ellipsis)
				{
					continue;
				}
			}

			validFuncTypes.Add(funcType);
		}

		ExprTsysList selectedFuncTypes;
		CopyFrom(selectedFuncTypes, validFuncTypes);

		for (vint i = 0; i < argTypesList.Count(); i++)
		{
			ExprTsysList candidateFuncTypes;
			CopyFrom(candidateFuncTypes, validFuncTypes);
			Array<TsysConv> funcChoices(candidateFuncTypes.Count());

			for (vint j = 0; j < candidateFuncTypes.Count(); j++)
			{
				auto funcType = candidateFuncTypes[j];

				vint funcParamCount = funcType.tsys->GetParamCount();
				if (funcParamCount <= j)
				{
					if (!funcType.tsys->GetFunc().ellipsis)
					{
						funcChoices[j] = TsysConv::Illegal;
						continue;
					}
				}

				auto paramType = funcType.tsys->GetParam(j);
				auto& argTypes = *argTypesList[j].Obj();

				auto bestChoice = TsysConv::Illegal;
				for (vint k = 0; k < argTypes.Count(); k++)
				{
					auto choice = TestConvert(pa, paramType, argTypes[k]);
					if ((vint)bestChoice > (vint)choice) bestChoice = choice;
				}
				funcChoices[j] = bestChoice;
			}

			FilterFunctionByQualifier(candidateFuncTypes, funcChoices);
			CopyFrom(selectedFuncTypes, From(selectedFuncTypes).Intersect(candidateFuncTypes));
		}

		for (vint i = 0; i < selectedFuncTypes.Count(); i++)
		{
			AddTemp(result, selectedFuncTypes[i].tsys->GetElement());
		}
	}

	/***********************************************************************
	VisitDirectField: Find variables or qualified functions
	***********************************************************************/

	static void VisitDirectField(ParsingArguments& pa, ResolveSymbolResult& totalRar, const ExprTsysItem& parentItem, CppName& name, ExprTsysList& result)
	{
		TsysCV cv;
		TsysRefType refType;
		auto entity = parentItem.tsys->GetEntity(cv, refType);

		ExprTsysList fieldResult;
		VisitNormalField(pa, name, &totalRar, parentItem, fieldResult);
		FilterFunctionByQualifier(cv, refType, fieldResult);
		AddInternal(result, fieldResult);
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
		throw 0;
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

		AddTemp(result, pa.tsys->PrimitiveOf({ TsysPrimitiveType::Void,TsysBytes::_1 }));
	}

	void Visit(NewExpr* self)override
	{
		for (vint i = 0; i < self->placementArguments.Count(); i++)
		{
			ExprTsysList types;
			ExprToTsys(pa, self->placementArguments[i], types);
		}
		for (vint i = 0; i < self->arguments.Count(); i++)
		{
			ExprTsysList types;
			ExprToTsys(pa, self->arguments[i], types);
		}

		TypeTsysList types;
		TypeToTsys(pa, self->type, types);
		for (vint i = 0; i < types.Count(); i++)
		{
			AddTemp(result, types[i]->PtrOf());
		}
	}

	void Visit(DeleteExpr* self)override
	{
		{
			ExprTsysList types;
			ExprToTsys(pa, self->expr, types);
		}

		AddTemp(result, pa.tsys->PrimitiveOf({ TsysPrimitiveType::Void,TsysBytes::_1 }));
	}

	void Visit(IdExpr* self)override
	{
		if (self->resolving)
		{
			for (vint i = 0; i < self->resolving->resolvedSymbols.Count(); i++)
			{
				VisitSymbol(pa, nullptr, self->resolving->resolvedSymbols[i], false, result);
			}
		}
	}

	void Visit(ChildExpr* self)override
	{
		if (self->resolving)
		{
			for (vint i = 0; i < self->resolving->resolvedSymbols.Count(); i++)
			{
				VisitSymbol(pa, nullptr, self->resolving->resolvedSymbols[i], true, result);
			}
		}
	}

	void Visit(FieldAccessExpr* self)override
	{
		ResolveSymbolResult totalRar;
		ExprTsysList parentItems;
		ExprToTsys(pa, self->expr, parentItems);

		if (self->type == CppFieldAccessType::Dot)
		{
			for (vint i = 0; i < parentItems.Count(); i++)
			{
				VisitDirectField(pa, totalRar, parentItems[i], self->name, result);
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
					auto parentItem = parentItems[i];
					VisitDirectField(pa, totalRar, { nullptr,ExprTsysType::LValue,entityType->GetElement() }, self->name, result);
				}
				else if (entityType->GetType() == TsysType::Decl)
				{
					if (!visitedDecls.Contains(entityType))
					{
						visitedDecls.Add(entityType);

						CppName opName;
						opName.name = L"operator ->";
						ExprTsysList opResult;
						VisitNormalField(pa, opName, nullptr, parentItems[i], opResult);
						FindQualifiedFunctions(pa, cv, refType, opResult, false);
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

		self->resolving = totalRar.values;
		if (pa.recorder)
		{
			if (totalRar.values)
			{
				pa.recorder->Index(self->name, totalRar.values);
			}
			if (totalRar.types)
			{
				pa.recorder->ExpectValueButType(self->name, totalRar.types);
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
				CppName opName;
				opName.name = L"operator []";
				ExprTsysList opResult;
				VisitNormalField(pa, opName, nullptr, arrayType, opResult);
				FindQualifiedFunctions(pa, cv, refType, opResult, false);
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

		if (self->type)
		{
			TypeTsysList types;
			TypeToTsys(pa, self->type, types);
			AddTemp(result, types);
		}
		else if (self->expr)
		{
			ExprTsysList funcTypes;
			ExprToTsys(pa, self->expr, funcTypes);

			FindQualifiedFunctions(pa, {}, TsysRefType::None, funcTypes, true);
			VisitOverloadedFunction(pa, funcTypes, argTypesList, result);
		}
	}

	void Visit(PostfixUnaryExpr* self)override
	{
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
				throw 0;
			}

			if (entity->GetType()==TsysType::Primitive)
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

	void Promote(TsysPrimitive& primitive)
	{
		switch (primitive.type)
		{
		case TsysPrimitiveType::Bool: primitive.type = TsysPrimitiveType::SInt; break;
		case TsysPrimitiveType::SChar: primitive.type = TsysPrimitiveType::SInt; break;
		case TsysPrimitiveType::UChar: if (primitive.bytes < TsysBytes::_4) primitive.type = TsysPrimitiveType::UInt; break;
		case TsysPrimitiveType::UWChar: primitive.type = TsysPrimitiveType::UInt; break;
		}

		switch (primitive.type)
		{
		case TsysPrimitiveType::SInt:
			if (primitive.bytes < TsysBytes::_4)
			{
				primitive.bytes = TsysBytes::_4;
			}
			break;
		case TsysPrimitiveType::UInt:
			if (primitive.bytes < TsysBytes::_4)
			{
				primitive.type = TsysPrimitiveType::SInt;
				primitive.bytes = TsysBytes::_4;
			}
			break;
		}
	}

	bool FullyContain(TsysPrimitive large, TsysPrimitive small)
	{
		if (large.type == TsysPrimitiveType::Float && small.type != TsysPrimitiveType::Float)
		{
			return true;
		}

		if (large.type != TsysPrimitiveType::Float && small.type == TsysPrimitiveType::Float)
		{
			return false;
		}

		if (large.bytes <= small.bytes)
		{
			return false;
		}

		if (large.type == TsysPrimitiveType::Float && small.type == TsysPrimitiveType::Float)
		{
			return true;
		}

		bool sl = large.type == TsysPrimitiveType::SInt || large.type == TsysPrimitiveType::SChar;
		bool ss = small.type == TsysPrimitiveType::SInt || small.type == TsysPrimitiveType::SChar;

		return sl == ss || sl;
	}

	void Visit(PrefixUnaryExpr* self)override
	{
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
				throw 0;
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

				if (leftEntity->GetType() == TsysType::Decl || rightEntity->GetType() == TsysType::Decl)
				{
					throw 0;
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
					case CppBinaryOp::Comma:
						AddInternal(result, rightTypes);
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

							if (FullyContain(leftP, rightP))
							{
								Promote(leftP);
								AddTemp(result, pa.tsys->PrimitiveOf(leftP));
								break;
							}

							if (FullyContain(rightP, leftP))
							{
								Promote(rightP);
								AddTemp(result, pa.tsys->PrimitiveOf(rightP));
								break;
							}

							Promote(leftP);
							Promote(rightP);

							TsysPrimitive primitive;
							primitive.bytes = leftP.bytes > rightP.bytes ? leftP.bytes : rightP.bytes;

							if (leftP.type == TsysPrimitiveType::Float || rightP.type == TsysPrimitiveType::Float)
							{
								primitive.type = TsysPrimitiveType::Float;
							}
							else if (leftP.bytes > rightP.bytes)
							{
								primitive.type = leftP.type;
							}
							else if (leftP.bytes < rightP.bytes)
							{
								primitive.type = rightP.type;
							}
							else
							{
								bool sl = leftP.type == TsysPrimitiveType::SInt || leftP.type == TsysPrimitiveType::SChar;
								bool sr = rightP.type == TsysPrimitiveType::SInt || rightP.type == TsysPrimitiveType::SChar;
								if (sl && !sr)
								{
									primitive.type = rightP.type;
								}
								else if (!sl && sr)
								{
									primitive.type = leftP.type;
								}
								else
								{
									primitive.type = leftP.type;
								}
							}

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
		{
			ExprTsysList types;
			ExprToTsys(pa, self->condition, types);
		}
		ExprToTsys(pa, self->left, result);
		ExprToTsys(pa, self->right, result);
	}
};

// Resolve expressions to types
void ExprToTsys(ParsingArguments& pa, Ptr<Expr> e, ExprTsysList& tsys)
{
	if (!e) throw IllegalExprException();
	ExprToTsysVisitor visitor(pa, tsys);
	e->Accept(&visitor);
}