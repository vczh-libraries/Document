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
		for (vint i = 0; i < resolving->resolvedSymbols.Count(); i++)
		{
			auto symbol = resolving->resolvedSymbols[i];
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

	void SearchAdlClassesAndNamespaces(const ParsingArguments& pa, ITsys* type, SortedList<Symbol*>& nss)
	{
		if (!type) return;
		switch (type->GetType())
		{
		case TsysType::LRef:
		case TsysType::RRef:
		case TsysType::Ptr:
		case TsysType::Array:
		case TsysType::CV:
			SearchAdlClassesAndNamespaces(pa, type->GetElement(), nss);
			break;
		case TsysType::Function:
			SearchAdlClassesAndNamespaces(pa, type->GetElement(), nss);
			for (vint i = 0; i < type->GetParamCount(); i++)
			{
				SearchAdlClassesAndNamespaces(pa, type->GetParam(i), nss);
			}
			break;
		case TsysType::Member:
			SearchAdlClassesAndNamespaces(pa, type->GetElement(), nss);
			SearchAdlClassesAndNamespaces(pa, type->GetClass(), nss);
			break;
		case TsysType::DeclInstant:
			{
				for (vint i = 0; i < type->GetParamCount(); i++)
				{
					SearchAdlClassesAndNamespaces(pa, type->GetParam(i), nss);
				}
				const auto& di = type->GetDeclInstant();
				if (di.parentDeclType)
				{
					SearchAdlClassesAndNamespaces(pa, di.parentDeclType, nss);
				}
			}
			break;
		case TsysType::Decl:
			{
				AddAdlNamespace(pa, type->GetDecl(), nss);
				if (type->GetDecl()->GetImplDecl_NFb<ClassDeclaration>())
				{
					auto& ev = symbol_type_resolving::EvaluateClassType(pa, type);
					for (vint i = 0; i < ev.ExtraCount(); i++)
					{
						auto& tsys = ev.GetExtra(i);
						for (vint j = 0; j < tsys.Count(); j++)
						{
							SearchAdlClassesAndNamespaces(pa, tsys[j], nss);
						}
					}
				}
			}
			break;
		}
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
					auto child = pChildren->Get(j).Obj();
					if (child->kind == symbol_component::SymbolKind::FunctionSymbol)
					{
						VisitSymbol(pa, child, result);
					}
				}
			}
		}
	}
}