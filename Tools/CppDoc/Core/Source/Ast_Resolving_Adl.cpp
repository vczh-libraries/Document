#include "Ast_Resolving.h"
#include "Symbol_Visit.h"
#include "EvaluateSymbol.h"

namespace symbol_type_resolving
{
	/***********************************************************************
	IsAdlEnabled: Check if argument-dependent lookup is considered according to the unqualified lookup result
	***********************************************************************/

	bool IsAdlEnabled(const ParsingArguments& pa, Ptr<Resolving> resolving)
	{
		for (vint i = 0; i < resolving->items.Count(); i++)
		{
			auto symbol = resolving->items[i].symbol;
			if (symbol->GetAnyForwardDecl<ForwardFunctionDeclaration>())
			{
				auto parent = symbol->GetParentScope();
				while (parent)
				{
					switch (parent->kind)
					{
					case symbol_component::SymbolKind::Root:
					case symbol_component::SymbolKind::Namespace:
						parent = parent->GetParentScope();
						break;
					default:
						return false;
					}
				}
			}
			else
			{
				return false;
			}
		}
		return true;
	}

	/***********************************************************************
	SearchAdlClassesAndNamespaces: Preparing for argument-dependent lookup
	***********************************************************************/

	void AddAdlNamespace(const ParsingArguments& pa, Symbol* symbol, SortedList<Symbol*>& nss)
	{
		auto firstSymbol = symbol;
		while (symbol)
		{
			if (symbol->kind == symbol_component::SymbolKind::Namespace)
			{
				if (!nss.Contains(symbol))
				{
					nss.Add(symbol);
				}
			}
			symbol = symbol->GetParentScope();
		}
	}

	void SearchAdlClassesAndNamespacesInternal(const ParsingArguments& pa, ITsys* type, SortedList<Symbol*>& nss, SortedList<Symbol*>& visitedDecls, SortedList<ITsys*>& visitedTypes)
	{
		if (!type) return;
		if (visitedTypes.Contains(type)) return;
		visitedTypes.Add(type);

		switch (type->GetType())
		{
		case TsysType::LRef:
		case TsysType::RRef:
		case TsysType::Ptr:
		case TsysType::Array:
		case TsysType::CV:
			SearchAdlClassesAndNamespacesInternal(pa, type->GetElement(), nss, visitedDecls, visitedTypes);
			break;
		case TsysType::Function:
			SearchAdlClassesAndNamespacesInternal(pa, type->GetElement(), nss, visitedDecls, visitedTypes);
			for (vint i = 0; i < type->GetParamCount(); i++)
			{
				SearchAdlClassesAndNamespacesInternal(pa, type->GetParam(i), nss, visitedDecls, visitedTypes);
			}
			break;
		case TsysType::Member:
			SearchAdlClassesAndNamespacesInternal(pa, type->GetElement(), nss, visitedDecls, visitedTypes);
			SearchAdlClassesAndNamespacesInternal(pa, type->GetClass(), nss, visitedDecls, visitedTypes);
			break;
		case TsysType::DeclInstant:
			{
				if (!visitedDecls.Contains(type->GetDecl()))
				{
					visitedDecls.Add(type->GetDecl());
					AddAdlNamespace(pa, type->GetDecl(), nss);
				}

				for (vint i = 0; i < type->GetParamCount(); i++)
				{
					SearchAdlClassesAndNamespacesInternal(pa, type->GetParam(i), nss, visitedDecls, visitedTypes);
				}
				const auto& di = type->GetDeclInstant();
				if (di.parentDeclType)
				{
					SearchAdlClassesAndNamespacesInternal(pa, di.parentDeclType, nss, visitedDecls, visitedTypes);
				}
			}
			break;
		case TsysType::Decl:
			if (!visitedDecls.Contains(type->GetDecl()))
			{
				visitedDecls.Add(type->GetDecl());
				AddAdlNamespace(pa, type->GetDecl(), nss);
				if (type->GetDecl()->GetImplDecl_NFb<ClassDeclaration>())
				{
					ClassDeclaration* cd = nullptr;
					ITsys* pdt = nullptr;
					TemplateArgumentContext* ata = nullptr;
					symbol_type_resolving::ExtractClassType(type, cd, pdt, ata);
					symbol_type_resolving::EnumerateClassSymbolBaseTypes(pa, cd, pdt, ata, [&](ITsys* classType, ITsys* baseType)
					{
						SearchAdlClassesAndNamespacesInternal(pa, baseType, nss, visitedDecls, visitedTypes);
						return false;
					});
				}
			}
			break;
		}
	}

	void SearchAdlClassesAndNamespaces(const ParsingArguments& pa, ITsys* type, SortedList<Symbol*>& nss)
	{
		SortedList<Symbol*> visitedDecls;
		SortedList<ITsys*> visitedTypes;
		SearchAdlClassesAndNamespacesInternal(pa, type, nss, visitedDecls, visitedTypes);
	}

	/***********************************************************************
	SearchAdlFunction: Find functions in namespaces
	***********************************************************************/

	void SearchAdlFunction(const ParsingArguments& pa, SortedList<Symbol*>& nss, const WString& name, ExprTsysList& result)
	{
		for (vint i = 0; i < nss.Count(); i++)
		{
			auto ns = nss[i];
			if (auto pChildren = ns->TryGetChildren_NFb(name))
			{
				for (vint j = 0; j < pChildren->Count(); j++)
				{
					auto child = pChildren->Get(j).childSymbol.Obj();
					if (child->kind == symbol_component::SymbolKind::FunctionSymbol)
					{
						VisitSymbol(pa, { nullptr,child }, result);
					}
				}
			}
		}
	}
}