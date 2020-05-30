#include "Ast_Evaluate_ExpandPotentialVta.h"
#include "Parser_Declarator.h"
#include "EvaluateSymbol.h"
#include "Symbol_TemplateSpec.h"

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
				auto argumentKey = GetTemplateArgumentKey(symbol);
				ITsys* replacedType = nullptr;
				if(pa.TryGetReplacedGenericArg(argumentKey, replacedType))
				{
					if (symbol->ellipsis)
					{
						if (!allowVariadic)
						{
							throw TypeCheckerException();
						}
						hasVariadic = true;

						switch (replacedType->GetType())
						{
						case TsysType::Init:
						case TsysType::Any:
							AddTsysToResult(result, replacedType);
							break;
						case TsysType::GenericArg:
							if (argumentKey == replacedType)
							{
								AddTsysToResult(result, pa.tsys->Any());
								break;
							}
						default:
							throw TypeCheckerException();
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
						throw TypeCheckerException();
					}
					AddTsysToResult(result, pa.tsys->Any());
					hasVariadic = true;
				}
				else
				{
					AddTsysToResult(result, EvaluateGenericArgumentType(symbol));
					hasNonVariadic = true;
				}
			}
			return;
		}
		throw TypeCheckerException();
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
			throw TypeCheckerException();
		}
		return hasVariadic;
	}

	//////////////////////////////////////////////////////////////////////////////////////
	// CreateIdReferenceType
	//////////////////////////////////////////////////////////////////////////////////////

	void CreateIdReferenceType(const ParsingArguments& pa, const Ptr<Resolving>& resolving, bool allowAny, bool allowVariadic, TypeTsysList& result, bool& isVta)
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
				throw TypeCheckerException();
			}
		}
		else if (resolving->items.Count() == 0)
		{
			throw TypeCheckerException();
		}

		isVta = SymbolListToTsys(pa, result, allowVariadic, [&](auto receiver)
		{
			for (vint i = 0; i < resolving->items.Count(); i++)
			{
				auto ritem = resolving->items[i];
				auto pdt = ritem.tsys;
				if (pdt)
				{
					if (pdt->GetType() == TsysType::DeclInstant)
					{
						auto& di = pdt->GetDeclInstant();
						if (!di.taContext)
						{
							pdt = di.parentDeclType;
						}
					}
					else
					{
						pdt = nullptr;
					}
				}
				receiver(pdt, ritem.symbol);
			}
		});
	}

	//////////////////////////////////////////////////////////////////////////////////////
	// ResolveChildTypeWithGenericArguments
	//////////////////////////////////////////////////////////////////////////////////////

	template<typename TReceiver>
	void ResolveChildTypeWithGenericArguments(const ParsingArguments& pa, ChildType* self, ITsys* classType, SortedList<ResolvedItem>& visited, TReceiver&& receiver)
	{
		auto rsr = ResolveChildSymbol(pa, classType, self->name);
		if (rsr.types)
		{
			auto newPa = pa.WithScope(classType->GetDecl());
			for (vint i = 0; i < rsr.types->items.Count(); i++)
			{
				auto ritem = rsr.types->items[i];
				if (!visited.Contains(ritem))
				{
					visited.Add(ritem);

					auto adjusted = AdjustThisItemForSymbol(newPa, { nullptr,ExprTsysType::PRValue,classType }, ritem);
					receiver((adjusted.tsys->GetType() == TsysType::DeclInstant ? adjusted.tsys : nullptr), ritem.symbol);
				}
			}
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
			SortedList<ResolvedItem> visited;
			SymbolListToTsys(pa, childTypes, false, [&](auto receiver)
			{
				ResolveChildTypeWithGenericArguments(pa, self, argClass.tsys, visited, receiver);
			});
			AddType(result, childTypes);
		}
	}

	//////////////////////////////////////////////////////////////////////////////////////
	// CreateIdReferenceExpr
	//////////////////////////////////////////////////////////////////////////////////////

	void CreateIdReferenceExpr(const ParsingArguments& pa, const Ptr<Resolving>& resolving, ExprTsysList& result, const ExprTsysItem* childExprClassItem, bool allowAny, bool allowVariadic, bool& isVta)
	{
		if (!resolving)
		{
			if (allowAny)
			{
				AddType(result, pa.tsys->Any());
			}
			return;
		}
		else if (resolving->items.Count() == 0)
		{
			throw TypeCheckerException();
		}

		bool hasVariadic = false;
		bool hasNonVariadic = false;

		if (pa.functionBodySymbol && pa.functionBodySymbol->GetClassMemberCache_NFb())
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
					thisType = ReplaceGenericArgsInClass(pa, thisType);
				}
			}
			ExprTsysItem thisItem(nullptr, ExprTsysType::LValue, thisType->GetElement()->LRefOf());
			VisitResolvedMember(pa, &thisItem, resolving, result, hasVariadic, hasNonVariadic);
		}
		else
		{
			VisitResolvedMember(pa, childExprClassItem, resolving, result, hasVariadic, hasNonVariadic);
		}

		if (hasVariadic && hasNonVariadic)
		{
			throw TypeCheckerException();
		}
		if (hasVariadic && !allowVariadic)
		{
			throw TypeCheckerException();
		}
		isVta = hasVariadic;
	}

	//////////////////////////////////////////////////////////////////////////////////////
	// ResolveChildExprWithGenericArguments
	//////////////////////////////////////////////////////////////////////////////////////

	Ptr<Resolving> ResolveChildExprWithGenericArguments(const ParsingArguments& pa, ChildExpr* self, ITsys* classType, SortedList<ResolvedItem>& visited)
	{
		Ptr<Resolving> resolving;
		auto rsr = ResolveChildSymbol(pa, classType, self->name);
		if (rsr.values)
		{
			for (vint i = 0; i < rsr.values->items.Count(); i++)
			{
				auto ritem = rsr.values->items[i];
				if (!visited.Contains(ritem))
				{
					visited.Add(ritem);
					if (!resolving)
					{
						resolving = MakePtr<Resolving>();
					}
					resolving->items.Add(ritem);
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
		SortedList<ResolvedItem> visited;
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
	void ProcessFieldAccessExpr(const ParsingArguments& pa, ExprTsysList& result, FieldAccessExpr* self, ExprTsysItem argParent, List<ResolvedItem>* ritems, TProcessor&& process)
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
							if (item.tsys->GetType() == TsysType::GenericFunction)
							{
								throw TypeCheckerException();
							}
							item.tsys = item.tsys->GetElement()->GetElement();
							AddNonVar(indirectionItems, item);
						}

						if (ritems && opResult.Count() > 0)
						{
							for (vint j = 0; j < opResult.Count(); j++)
							{
								ResolvedItem::AddItem(*ritems, { nullptr,opResult[j].symbol });
							}
						}
					}
				}
			}
			break;
		}
	}

	void ProcessFieldAccessExprForIdExpr(const ParsingArguments& pa, ExprTsysList& result, FieldAccessExpr* self, ExprTsysItem argParent, const Ptr<IdExpr>& idExpr, ResolveSymbolResult& totalRar, List<ResolvedItem>* ritems)
	{
		ProcessFieldAccessExpr(pa, result, self, argParent, ritems, [&](ExprTsysItem parentItem)
		{
			if (parentItem.tsys->IsUnknownType())
			{
				AddType(result, pa.tsys->Any());
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

	void ProcessFieldAccessExprForChildExpr(const ParsingArguments& pa, ExprTsysList& result, FieldAccessExpr* self, ExprTsysItem argParent, ExprTsysItem argClass, const Ptr<ChildExpr>& childExpr, ResolveSymbolResult& totalRar, List<ResolvedItem>* ritems)
	{
		ProcessFieldAccessExpr(pa, result, self, argParent, ritems, [&](ExprTsysItem parentItem)
		{
			if (argClass.tsys->IsUnknownType())
			{
				AddType(result, pa.tsys->Any());
			}
			else
			{
				SortedList<ResolvedItem> visited;
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