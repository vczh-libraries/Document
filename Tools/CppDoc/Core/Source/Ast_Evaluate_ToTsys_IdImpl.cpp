#include "Ast_Resolving_ExpandPotentialVta.h"

using namespace symbol_type_resolving;

namespace symbol_totsys_impl
{
	//////////////////////////////////////////////////////////////////////////////////////
	// TypeSymbolToTsys
	//////////////////////////////////////////////////////////////////////////////////////

	void TypeSymbolToTsys(const ParsingArguments& pa, TypeTsysList& result, GenericArgContext* gaContext, Symbol* symbol, bool allowVariadic, bool& hasVariadic, bool& hasNonVariadic)
	{
		switch (symbol->kind)
		{
		case symbol_component::SymbolKind::Enum:
		case symbol_component::SymbolKind::Class:
		case symbol_component::SymbolKind::Struct:
		case symbol_component::SymbolKind::Union:
			AddTsysToResult(result, pa.tsys->DeclOf(symbol));
			hasNonVariadic = true;
			return;
		case symbol_component::SymbolKind::TypeAlias:
			{
				auto usingDecl = symbol->definition.Cast<TypeAliasDeclaration>();
				EvaluateSymbol(pa, usingDecl.Obj());
				auto& types = symbol->evaluation.Get();
				for (vint j = 0; j < types.Count(); j++)
				{
					AddTsysToResult(result, types[j]);
				}
				hasNonVariadic = true;
			}
			return;
		case symbol_component::SymbolKind::GenericTypeArgument:
			{
				auto& types = symbol->evaluation.Get();
				for (vint j = 0; j < types.Count(); j++)
				{
					if (gaContext)
					{
						auto type = types[j];
						vint index = gaContext->arguments.Keys().IndexOf(type);
						if (index != -1)
						{
							auto& replacedTypes = gaContext->arguments.GetByIndex(index);
							for (vint k = 0; k < replacedTypes.Count(); k++)
							{
								if (symbol->ellipsis)
								{
									if (!allowVariadic)
									{
										throw NotConvertableException();
									}
									auto replacedType = replacedTypes[k];
									if (replacedType->GetType() == TsysType::Any || replacedType->GetType() == TsysType::Init)
									{
										AddTsysToResult(result, replacedTypes[k]);
										hasVariadic = true;
									}
									else
									{
										throw NotConvertableException();
									}
								}
								else
								{
									AddTsysToResult(result, replacedTypes[k]);
									hasNonVariadic = true;
								}
							}
							continue;
						}
					}

					if (symbol->ellipsis)
					{
						if (!allowVariadic)
						{
							throw NotConvertableException();
						}
						AddTsysToResult(result, pa.tsys->Any());
						hasVariadic = true;
					}
					else
					{
						AddTsysToResult(result, types[j]);
						hasNonVariadic = true;
					}
				}
			}
			return;
		}
		throw NotConvertableException();
	}

	//////////////////////////////////////////////////////////////////////////////////////
	// SymbolListToTsys
	//////////////////////////////////////////////////////////////////////////////////////

	template<typename TGenerator>
	static bool SymbolListToTsys(const ParsingArguments& pa, TypeTsysList& result, GenericArgContext* gaContext, bool allowVariadic, TGenerator&& symbolGenerator)
	{
		bool hasVariadic = false;
		bool hasNonVariadic = false;
		symbolGenerator([&](Symbol* symbol)
		{
			TypeSymbolToTsys(pa, result, gaContext, symbol, allowVariadic, hasVariadic, hasNonVariadic);
		});

		if (hasVariadic && hasNonVariadic)
		{
			throw NotConvertableException();
		}
		return hasVariadic;
	}

	//////////////////////////////////////////////////////////////////////////////////////
	// CreateIdReferenceType
	//////////////////////////////////////////////////////////////////////////////////////

	void CreateIdReferenceType(const ParsingArguments& pa, GenericArgContext* gaContext, Ptr<Resolving> resolving, bool allowAny, bool allowVariadic, TypeTsysList& result, bool& isVta)
	{
		if (!resolving)
		{
			if (allowAny)
			{
				AddTsysToResult(result, pa.tsys->Any());
				return;
			}
			else
			{
				throw NotConvertableException();
			}
		}
		else if (resolving->resolvedSymbols.Count() == 0)
		{
			throw NotConvertableException();
		}

		isVta = SymbolListToTsys(pa, result, gaContext, allowVariadic, [&](auto receiver)
		{
			for (vint i = 0; i < resolving->resolvedSymbols.Count(); i++)
			{
				receiver(resolving->resolvedSymbols[i]);
			}
		});
	}

	//////////////////////////////////////////////////////////////////////////////////////
	// ResolveChildTypeWithGenericArguments
	//////////////////////////////////////////////////////////////////////////////////////

	template<typename TReceiver>
	void ResolveChildTypeWithGenericArguments(const ParsingArguments& pa, ChildType* self, ITsys* classType, SortedList<Symbol*>& visited, TReceiver&& receiver)
	{
		if (classType->GetType() == TsysType::Decl)
		{
			auto newPa = pa.WithContext(classType->GetDecl());
			auto rsr = ResolveSymbol(newPa, self->name, SearchPolicy::ChildSymbol);
			if (rsr.types)
			{
				for (vint i = 0; i < rsr.types->resolvedSymbols.Count(); i++)
				{
					auto symbol = rsr.types->resolvedSymbols[i];
					if (!visited.Contains(symbol))
					{
						visited.Add(symbol);
						receiver(symbol);
					}
				}
			}
		}
	}

	//////////////////////////////////////////////////////////////////////////////////////
	// ProcessChildType
	//////////////////////////////////////////////////////////////////////////////////////
	
	void ProcessChildType(const ParsingArguments& pa, GenericArgContext* gaContext, ChildType* self, ExprTsysItem argClass, ExprTsysList& result)
	{
		if (argClass.tsys->IsUnknownType())
		{
			result.Add(GetExprTsysItem(pa.tsys->Any()));
		}
		else
		{
			TypeTsysList childTypes;
			SortedList<Symbol*> visited;
			SymbolListToTsys(pa, childTypes, gaContext, false, [&](auto receiver)
			{
				ResolveChildTypeWithGenericArguments(pa, self, argClass.tsys, visited, receiver);
			});
			AddTemp(result, childTypes);
		}
	}

	//////////////////////////////////////////////////////////////////////////////////////
	// CreateIdReferenceExpr
	//////////////////////////////////////////////////////////////////////////////////////

	void CreateIdReferenceExpr(const ParsingArguments& pa, GenericArgContext* gaContext, Ptr<Resolving> resolving, ExprTsysList& result, bool allowAny, bool allowVariadic, bool& isVta)
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

	//////////////////////////////////////////////////////////////////////////////////////
	// ResolveChildExprWithGenericArguments
	//////////////////////////////////////////////////////////////////////////////////////

	template<typename TReceiver>
	void ResolveChildExprWithGenericArguments(const ParsingArguments& pa, ChildExpr* self, ITsys* classType, SortedList<Symbol*>& visited, TReceiver&& receiver)
	{
		if (classType->GetType() == TsysType::Decl)
		{
			auto newPa = pa.WithContext(classType->GetDecl());
			auto rsr = ResolveSymbol(newPa, self->name, SearchPolicy::ChildSymbol);
			if (rsr.values)
			{
				for (vint i = 0; i < rsr.values->resolvedSymbols.Count(); i++)
				{
					auto symbol = rsr.values->resolvedSymbols[i];
					if (!visited.Contains(symbol))
					{
						visited.Add(symbol);
						receiver(symbol);
					}
				}
			}
		}
	}

	//////////////////////////////////////////////////////////////////////////////////////
	// ProcessChildExpr
	//////////////////////////////////////////////////////////////////////////////////////

	void ProcessChildExpr(const ParsingArguments& pa, ExprTsysList& result, GenericArgContext* gaContext, ChildExpr* self, ExprTsysItem argClass)
	{
		Ptr<Resolving> resolving;
		SortedList<Symbol*> visited;
		ResolveChildExprWithGenericArguments(pa, self, argClass.tsys, visited, [&](Symbol* symbol)
		{
			if (!resolving)
			{
				resolving = MakePtr<Resolving>();
			}
			resolving->resolvedSymbols.Add(symbol);
		});

		if (resolving)
		{
			bool childIsVta = false;
			CreateIdReferenceExpr(pa, gaContext, resolving, result, false, false, childIsVta);
		}
	}

	//////////////////////////////////////////////////////////////////////////////////////
	// ResolveField
	//////////////////////////////////////////////////////////////////////////////////////

	void ResolveField(const ParsingArguments& pa, GenericArgContext* gaContext, ExprTsysList& result, ResolveSymbolResult& totalRar, const ExprTsysItem& parentItem, ITsys* classType, const Ptr<IdExpr>& idExpr, const Ptr<ChildExpr>& childExpr)
	{
		if (idExpr)
		{
			if (parentItem.tsys->IsUnknownType())
			{
				AddTemp(result, pa.tsys->Any());
			}
			else
			{
				if (auto resolving = FindMembersByName(pa, idExpr->name, &totalRar, parentItem))
				{
					VisitResolvedMember(pa, &parentItem, resolving, result);
				}
			}
		}
		else
		{
			if (classType)
			{
				if (classType->IsUnknownType())
				{
					AddTemp(result, pa.tsys->Any());
				}
				else
				{
					Ptr<Resolving> resolving;
					SortedList<Symbol*> visited;
					ResolveChildExprWithGenericArguments(pa, childExpr.Obj(), classType, visited, [&](Symbol* symbol)
					{
						if (!resolving)
						{
							resolving = MakePtr<Resolving>();
						}
						resolving->resolvedSymbols.Add(symbol);
					});

					if (resolving)
					{
						VisitResolvedMember(pa, &parentItem, resolving, result);
					}
				}
			}
			else
			{
				VisitResolvedMember(pa, &parentItem, childExpr->resolving, result);
			}
		}
	}

	//////////////////////////////////////////////////////////////////////////////////////
	// ProcessFieldAccessExpr
	//////////////////////////////////////////////////////////////////////////////////////

	void ProcessFieldAccessExpr(const ParsingArguments& pa, ExprTsysList& result, GenericArgContext* gaContext, FieldAccessExpr* self, ExprTsysItem argParent, ExprTsysItem argClass, const Ptr<IdExpr>& idExpr, const Ptr<ChildExpr>& childExpr, ResolveSymbolResult& totalRar, bool& operatorIndexed)
	{
		switch (self->type)
		{
		case CppFieldAccessType::Dot:
			{
				ResolveField(pa, gaContext, result, totalRar, argParent, argClass.tsys, idExpr, childExpr);
			}
			break;
		case CppFieldAccessType::Arrow:
			{
				ExprTsysList indirectionItems;
				indirectionItems.Add(argParent);

				for (vint i = 0; i < indirectionItems.Count(); i++)
				{
					TsysCV cv;
					TsysRefType refType;
					auto entityType = indirectionItems[i].tsys->GetEntity(cv, refType);

					if (entityType->GetType() == TsysType::Ptr)
					{
						ExprTsysItem parentItem(nullptr, ExprTsysType::LValue, entityType->GetElement());
						ResolveField(pa, gaContext, result, totalRar, parentItem, argClass.tsys, idExpr, childExpr);
					}
					else if (entityType->GetType() == TsysType::Decl)
					{
						ExprTsysList opResult;
						VisitFunctors(pa, indirectionItems[i], L"operator ->", opResult);
						for (vint j = 0; j < opResult.Count(); j++)
						{
							auto item = opResult[j];
							item.tsys = item.tsys->GetElement();
							AddNonVar(indirectionItems, item);
						}

						if (pa.recorder && !gaContext)
						{
							AddSymbolsToOperatorResolving(gaContext, self->opName, self->opResolving, opResult, operatorIndexed);
						}
					}
				}
			}
			break;
		}
	}
}