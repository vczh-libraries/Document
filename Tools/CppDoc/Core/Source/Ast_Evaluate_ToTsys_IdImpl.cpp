#include "Ast_Evaluate_ExpandPotentialVta.h"

using namespace symbol_type_resolving;

namespace symbol_totsys_impl
{
	//////////////////////////////////////////////////////////////////////////////////////
	// TypeSymbolToTsys
	//////////////////////////////////////////////////////////////////////////////////////

	void TypeSymbolToTsys(const ParsingArguments& pa, TypeTsysList& result, ITsys* parentDeclType, Symbol* symbol, bool allowVariadic, bool& hasVariadic, bool& hasNonVariadic)
	{
		switch (symbol->kind)
		{
		case symbol_component::SymbolKind::Enum:
			AddTsysToResult(result, pa.tsys->DeclOf(symbol));
			hasNonVariadic = true;
			return;
		case CLASS_SYMBOL_KIND:
			{
				auto classDecl = symbol->GetAnyForwardDecl<ForwardClassDeclaration>();
				auto& evTypes = EvaluateForwardClassSymbol(pa, classDecl.Obj(), parentDeclType, nullptr);
				for (vint j = 0; j < evTypes.Count(); j++)
				{
					AddTsysToResult(result, evTypes[j]);
				}
				hasNonVariadic = true;
			}
			return;
		case symbol_component::SymbolKind::TypeAlias:
			{
				auto usingDecl = symbol->GetImplDecl_NFb<TypeAliasDeclaration>();
				auto& evTypes = EvaluateTypeAliasSymbol(pa, usingDecl.Obj(), parentDeclType, nullptr);
				for (vint j = 0; j < evTypes.Count(); j++)
				{
					AddTsysToResult(result, evTypes[j]);
				}
				hasNonVariadic = true;
			}
			return;
		case symbol_component::SymbolKind::GenericTypeArgument:
			{
				auto argumentKey = EvaluateGenericArgumentSymbol(symbol);
				ITsys* replacedType = nullptr;
				if(pa.TryGetReplacedGenericArg(argumentKey, replacedType))
				{
					if (symbol->ellipsis)
					{
						if (!allowVariadic)
						{
							throw NotConvertableException();
						}
						hasVariadic = true;

						switch (replacedType->GetType())
						{
						case TsysType::Init:
						case TsysType::Any:
							AddTsysToResult(result, replacedType);
							break;
						default:
							throw NotConvertableException();
						}
					}
					else
					{
						AddTsysToResult(result, replacedType);
						hasNonVariadic = true;
					}
				}
				else if (symbol->ellipsis)
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
					AddTsysToResult(result, argumentKey);
					hasNonVariadic = true;
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
	static bool SymbolListToTsys(const ParsingArguments& pa, TypeTsysList& result, bool allowVariadic, TGenerator&& symbolGenerator)
	{
		bool hasVariadic = false;
		bool hasNonVariadic = false;
		symbolGenerator([&](ITsys* parentDeclType, Symbol* symbol)
		{
			TypeSymbolToTsys(pa, result, parentDeclType, symbol, allowVariadic, hasVariadic, hasNonVariadic);
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

	void CreateIdReferenceType(const ParsingArguments& pa, Ptr<Resolving> resolving, bool allowAny, bool allowVariadic, TypeTsysList& result, bool& isVta)
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

		isVta = SymbolListToTsys(pa, result, allowVariadic, [&](auto receiver)
		{
			for (vint i = 0; i < resolving->resolvedSymbols.Count(); i++)
			{
				receiver(nullptr, resolving->resolvedSymbols[i]);
			}
		});
	}

	//////////////////////////////////////////////////////////////////////////////////////
	// ResolveChildTypeWithGenericArguments
	//////////////////////////////////////////////////////////////////////////////////////

	template<typename TReceiver>
	void ResolveChildTypeWithGenericArguments(const ParsingArguments& pa, ChildType* self, ITsys* classType, SortedList<Symbol*>& visited, TReceiver&& receiver)
	{
		switch (classType->GetType())
		{
		case TsysType::Decl:
		case TsysType::DeclInstant:
			{
				auto newPa = pa.WithScope(classType->GetDecl());
				auto rsr = ResolveSymbol(newPa, self->name, SearchPolicy::ChildSymbolFromOutside);

				ITsys* parentDeclType = nullptr;
				if (classType->GetType() == TsysType::DeclInstant)
				{
					parentDeclType = classType;
				}

				if (rsr.types)
				{
					for (vint i = 0; i < rsr.types->resolvedSymbols.Count(); i++)
					{
						auto symbol = rsr.types->resolvedSymbols[i];
						if (!visited.Contains(symbol))
						{
							visited.Add(symbol);
							receiver(parentDeclType, symbol);
						}
					}
				}
			}
			break;
		}
	}

	//////////////////////////////////////////////////////////////////////////////////////
	// ProcessChildType
	//////////////////////////////////////////////////////////////////////////////////////
	
	void ProcessChildType(const ParsingArguments& pa, ChildType* self, ExprTsysItem argClass, ExprTsysList& result)
	{
		if (argClass.tsys->IsUnknownType())
		{
			result.Add(GetExprTsysItem(pa.tsys->Any()));
		}
		else
		{
			TypeTsysList childTypes;
			SortedList<Symbol*> visited;
			SymbolListToTsys(pa, childTypes, false, [&](auto receiver)
			{
				ResolveChildTypeWithGenericArguments(pa, self, argClass.tsys, visited, receiver);
			});
			AddTemp(result, childTypes);
		}
	}

	//////////////////////////////////////////////////////////////////////////////////////
	// CreateIdReferenceExpr
	//////////////////////////////////////////////////////////////////////////////////////

	void CreateIdReferenceExpr(const ParsingArguments& pa, Ptr<Resolving> resolving, ExprTsysList& result, const ExprTsysItem* childExprClassItem, bool allowAny, bool allowVariadic, bool& isVta)
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

		else if (pa.functionBodySymbol && pa.functionBodySymbol->GetClassMemberCache_NFb())
		{
			auto thisType = pa.functionBodySymbol->GetClassMemberCache_NFb()->thisType;
			if (childExprClassItem)
			{
				thisType = ReplaceThisType(thisType, childExprClassItem->tsys);
			}
			else
			{
				if (pa.taContext)
				{
					thisType = thisType->ReplaceGenericArgs(pa);
				}
			}
			ExprTsysItem thisItem(nullptr, ExprTsysType::LValue, thisType->GetElement()->LRefOf());
			VisitResolvedMember(pa, &thisItem, resolving, result);
		}
		else
		{
			bool hasVariadic = false;
			bool hasNonVariadic = false;
			VisitResolvedMember(pa, childExprClassItem, resolving, result, hasVariadic, hasNonVariadic);

			if (hasVariadic && hasNonVariadic)
			{
				throw NotConvertableException();
			}
			isVta = hasVariadic;
		}
	}

	//////////////////////////////////////////////////////////////////////////////////////
	// ResolveChildExprWithGenericArguments
	//////////////////////////////////////////////////////////////////////////////////////

	Ptr<Resolving> ResolveChildExprWithGenericArguments(const ParsingArguments& pa, ChildExpr* self, ITsys* classType, SortedList<Symbol*>& visited)
	{
		Ptr<Resolving> resolving;
		if (classType->GetType() == TsysType::Decl || classType->GetType() == TsysType::DeclInstant)
		{
			auto newPa = pa.WithScope(classType->GetDecl());
			auto rsr = ResolveSymbol(newPa, self->name, SearchPolicy::ChildSymbolFromOutside);
			if (rsr.values)
			{
				for (vint i = 0; i < rsr.values->resolvedSymbols.Count(); i++)
				{
					auto symbol = rsr.values->resolvedSymbols[i];
					if (!visited.Contains(symbol))
					{
						visited.Add(symbol);
						if (!resolving)
						{
							resolving = MakePtr<Resolving>();
						}
						resolving->resolvedSymbols.Add(symbol);
					}
				}
			}
		}
		return resolving;
	}

	//////////////////////////////////////////////////////////////////////////////////////
	// ProcessChildExpr
	//////////////////////////////////////////////////////////////////////////////////////

	void ProcessChildExpr(const ParsingArguments& pa, ExprTsysList& result, ChildExpr* self, ExprTsysItem argClass)
	{
		SortedList<Symbol*> visited;
		if (auto resolving = ResolveChildExprWithGenericArguments(pa, self, argClass.tsys, visited))
		{
			bool childIsVta = false;
			argClass.type = ExprTsysType::LValue;
			CreateIdReferenceExpr(pa, resolving, result, &argClass, false, false, childIsVta);
		}
	}

	//////////////////////////////////////////////////////////////////////////////////////
	// ProcessFieldAccessExpr
	//////////////////////////////////////////////////////////////////////////////////////

	template<typename TProcessor>
	void ProcessFieldAccessExpr(const ParsingArguments& pa, ExprTsysList& result, FieldAccessExpr* self, ExprTsysItem argParent, bool& operatorIndexed, TProcessor&& process)
	{
		switch (self->type)
		{
		case CppFieldAccessType::Dot:
			process(argParent);
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
						process(parentItem);
					}
					else if (entityType->GetType() == TsysType::Decl || entityType->GetType() == TsysType::DeclInstant)
					{
						ExprTsysList opResult;
						VisitFunctors(pa, indirectionItems[i], L"operator ->", opResult);
						for (vint j = 0; j < opResult.Count(); j++)
						{
							auto item = opResult[j];
							item.tsys = item.tsys->GetElement();
							AddNonVar(indirectionItems, item);
						}

						if (pa.recorder && pa.IsGeneralEvaluation())
						{
							AddSymbolsToOperatorResolving(pa, self->opName, self->opResolving, opResult, operatorIndexed);
						}
					}
				}
			}
			break;
		}
	}

	void ProcessFieldAccessExprForIdExpr(const ParsingArguments& pa, ExprTsysList& result, FieldAccessExpr* self, ExprTsysItem argParent, const Ptr<IdExpr>& idExpr, ResolveSymbolResult& totalRar, bool& operatorIndexed)
	{
		ProcessFieldAccessExpr(pa, result, self, argParent, operatorIndexed, [&](ExprTsysItem parentItem)
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
		});
	}

	void ProcessFieldAccessExprForChildExpr(const ParsingArguments& pa, ExprTsysList& result, FieldAccessExpr* self, ExprTsysItem argParent, ExprTsysItem argClass, const Ptr<ChildExpr>& childExpr, ResolveSymbolResult& totalRar, bool& operatorIndexed)
	{
		ProcessFieldAccessExpr(pa, result, self, argParent, operatorIndexed, [&](ExprTsysItem parentItem)
		{
			if (argClass.tsys->IsUnknownType())
			{
				AddTemp(result, pa.tsys->Any());
			}
			else
			{
				SortedList<Symbol*> visited;
				if (auto resolving = ResolveChildExprWithGenericArguments(pa, childExpr.Obj(), argClass.tsys, visited))
				{
					ExprTsysItem classItem = parentItem;
					classItem.tsys = ReplaceThisType(classItem.tsys, argClass.tsys);
					VisitResolvedMember(pa, &classItem, resolving, result);
				}
			}
		});
	}
}