#include "Ast_Resolving.h"
#include "Symbol_Visit.h"

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

	class SearchBaseTypeAdlClassesAndNamespacesVisitor : public Object, public virtual ITypeVisitor
	{
	public:
		const ParsingArguments&		pa;
		SortedList<Symbol*>&		nss;
		SortedList<Symbol*>&		classes;

		SearchBaseTypeAdlClassesAndNamespacesVisitor(const ParsingArguments& _pa, SortedList<Symbol*>& _nss, SortedList<Symbol*>& _classes)
			:pa(_pa)
			, nss(_nss)
			, classes(_classes)
		{
		}

		void Resolve(Ptr<Resolving> resolving)
		{
			for (vint i = 0; i < resolving->resolvedSymbols.Count(); i++)
			{
				SearchAdlClassesAndNamespaces(pa, resolving->resolvedSymbols[i], nss, classes);
			}
		}

		void Visit(PrimitiveType* self)override
		{
		}

		void Visit(ReferenceType* self)override
		{
		}

		void Visit(ArrayType* self)override
		{
		}

		void Visit(CallingConventionType* self)override
		{
		}

		void Visit(FunctionType* self)override
		{
		}

		void Visit(MemberType* self)override
		{
		}

		void Visit(DeclType* self)override
		{
		}

		void Visit(DecorateType* self)override
		{
		}

		void Visit(RootType* self)override
		{
		}

		void Visit(IdType* self)override
		{
			Resolve(self->resolving);
		}

		void Visit(ChildType* self)override
		{
			Resolve(self->resolving);
		}

		void Visit(GenericType* self)override
		{
			self->type->Accept(this);
			for (vint i = 0; i < self->arguments.Count(); i++)
			{
				auto argument = self->arguments[i];
				if (argument.item.type)
				{
					argument.item.type->Accept(this);
				}
			}
		}
	};

	void SearchAdlClassesAndNamespaces(const ParsingArguments& pa, Symbol* symbol, SortedList<Symbol*>& nss, SortedList<Symbol*>& classes)
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
			else if (auto classDecl = symbol->GetCategory() == symbol_component::SymbolCategory::Normal ? symbol->GetImplDecl_NFb<ClassDeclaration>() : nullptr)
			{
				if (!classes.Contains(symbol))
				{
					classes.Add(symbol);
				}

				if (symbol == firstSymbol)
				{
					auto classPa = pa.WithScope(symbol);

					SearchBaseTypeAdlClassesAndNamespacesVisitor visitor(classPa, nss, classes);
					for (vint i = 0; i < classDecl->baseTypes.Count(); i++)
					{
						classDecl->baseTypes[i].f1->Accept(&visitor);
					}
				}
			}
			else
			{
				break;
			}
			symbol = symbol->GetParentScope();
		}
	}

	void SearchAdlClassesAndNamespaces(const ParsingArguments& pa, ITsys* type, SortedList<Symbol*>& nss, SortedList<Symbol*>& classes)
	{
		if (!type) return;
		switch (type->GetType())
		{
		case TsysType::LRef:
		case TsysType::RRef:
		case TsysType::Ptr:
		case TsysType::Array:
		case TsysType::CV:
			SearchAdlClassesAndNamespaces(pa, type->GetElement(), nss, classes);
			break;
		case TsysType::Function:
			SearchAdlClassesAndNamespaces(pa, type->GetElement(), nss, classes);
			for (vint i = 0; i < type->GetParamCount(); i++)
			{
				SearchAdlClassesAndNamespaces(pa, type->GetParam(i), nss, classes);
			}
			break;
		case TsysType::Member:
			SearchAdlClassesAndNamespaces(pa, type->GetElement(), nss, classes);
			SearchAdlClassesAndNamespaces(pa, type->GetClass(), nss, classes);
			break;
		case TsysType::Decl:
			SearchAdlClassesAndNamespaces(pa, type->GetDecl(), nss, classes);
			break;
		case TsysType::DeclInstant:
			{
				SearchAdlClassesAndNamespaces(pa, type->GetDecl(), nss, classes);
				for (vint i = 0; i < type->GetParamCount(); i++)
				{
					SearchAdlClassesAndNamespaces(pa, type->GetParam(i), nss, classes);
				}
				const auto& di = type->GetDeclInstant();
				if (di.parentDeclType)
				{
					SearchAdlClassesAndNamespaces(pa, di.parentDeclType, nss, classes);
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