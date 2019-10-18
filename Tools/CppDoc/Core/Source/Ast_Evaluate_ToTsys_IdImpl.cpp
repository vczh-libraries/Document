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
		case symbol_component::SymbolKind::Class:
		case symbol_component::SymbolKind::Struct:
		case symbol_component::SymbolKind::Union:
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

	void CreateIdReferenceExpr(const ParsingArguments& pa, Ptr<Resolving> resolving, ExprTsysList& result, bool allowAny, bool allowVariadic, bool& isVta)
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

		if (pa.functionBodySymbol && pa.functionBodySymbol->GetClassMemberCache_NFb())
		{
			TsysCV thisCv;
			TsysRefType thisRef;
			auto thisType = pa.functionBodySymbol->GetClassMemberCache_NFb()->thisType->GetEntity(thisCv, thisRef);
			ExprTsysItem thisItem(nullptr, ExprTsysType::LValue, thisType->GetElement()->LRefOf());
			VisitResolvedMember(pa, &thisItem, resolving, result);
		}
		else
		{
			bool hasVariadic = false;
			bool hasNonVariadic = false;
			VisitResolvedMember(pa, resolving, result, hasVariadic, hasNonVariadic);

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

	template<typename TReceiver>
	void ResolveChildExprWithGenericArguments(const ParsingArguments& pa, ChildExpr* self, ITsys* classType, SortedList<Symbol*>& visited, TReceiver&& receiver)
	{
		if (classType->GetType() == TsysType::Decl)
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
						receiver(symbol);
					}
				}
			}
		}
		else if (classType->GetType() == TsysType::DeclInstant)
		{
			// TODO: [Cpp.md] Deal with DeclInstant here
			throw 0;
		}
	}

	//////////////////////////////////////////////////////////////////////////////////////
	// ProcessChildExpr
	//////////////////////////////////////////////////////////////////////////////////////

	void ProcessChildExpr(const ParsingArguments& pa, ExprTsysList& result, ChildExpr* self, ExprTsysItem argClass)
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
			CreateIdReferenceExpr(pa, resolving, result, false, false, childIsVta);
		}
	}

	//////////////////////////////////////////////////////////////////////////////////////
	// ResolveField
	//////////////////////////////////////////////////////////////////////////////////////

	void ResolveField(const ParsingArguments& pa, ExprTsysList& result, ResolveSymbolResult& totalRar, const ExprTsysItem& parentItem, ITsys* classType, const Ptr<IdExpr>& idExpr, const Ptr<ChildExpr>& childExpr)
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

	void ProcessFieldAccessExpr(const ParsingArguments& pa, ExprTsysList& result, FieldAccessExpr* self, ExprTsysItem argParent, ExprTsysItem argClass, const Ptr<IdExpr>& idExpr, const Ptr<ChildExpr>& childExpr, ResolveSymbolResult& totalRar, bool& operatorIndexed)
	{
		switch (self->type)
		{
		case CppFieldAccessType::Dot:
			{
				ResolveField(pa, result, totalRar, argParent, argClass.tsys, idExpr, childExpr);
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
						ResolveField(pa, result, totalRar, parentItem, argClass.tsys, idExpr, childExpr);
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
}