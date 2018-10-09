#include "Parser.h"
#include "Ast_Type.h"
#include "Ast_Decl.h"

enum class SearchCategory
{
	Type,
	Value,
};

Ptr<Resolving> ResolveChildSymbolInternal(const ParsingArguments& pa, Ptr<Type> classType, CppName& name, Ptr<Resolving> resolving, SearchPolicy policy, SearchCategory category, SortedList<Symbol*>& searchedScopes);

/***********************************************************************
IsPotentialTypeDeclVisitor
***********************************************************************/

class IsPotentialTypeDeclVisitor : public Object, public virtual IDeclarationVisitor
{
public:
	bool					isPotentialType = true;

	void Visit(ForwardVariableDeclaration* self)override
	{
		isPotentialType = false;
	}

	void Visit(ForwardFunctionDeclaration* self)override
	{
		isPotentialType = false;
	}

	void Visit(ForwardEnumDeclaration* self)override
	{
	}

	void Visit(ForwardClassDeclaration* self)override
	{
	}

	void Visit(VariableDeclaration* self)override
	{
		isPotentialType = false;
	}

	void Visit(FunctionDeclaration* self)override
	{
		isPotentialType = false;
	}

	void Visit(EnumItemDeclaration* self)override
	{
		isPotentialType = false;
	}

	void Visit(EnumDeclaration* self)override
	{
	}

	void Visit(ClassDeclaration* self)override
	{
	}

	void Visit(TypeAliasDeclaration* self)override
	{
	}

	void Visit(UsingNamespaceDeclaration* self)override
	{
		isPotentialType = false;
	}

	void Visit(UsingDeclaration* self)override
	{
		isPotentialType = false;
	}

	void Visit(NamespaceDeclaration* self)override
	{
	}
};

/***********************************************************************
AddSymbolToResolve
***********************************************************************/

void AddSymbolToResolve(Ptr<Resolving>& resolving, Symbol* symbol)
{
	if (!resolving)
	{
		resolving = new Resolving;
	}

	if (!resolving->resolvedSymbols.Contains(symbol))
	{
		resolving->resolvedSymbols.Add(symbol);
	}
}

/***********************************************************************
ResolveChildSymbolInternal
***********************************************************************/

Ptr<Resolving> ResolveSymbolInternal(const ParsingArguments& pa, CppName& name, Ptr<Resolving> resolving, SearchPolicy policy, SearchCategory category, SortedList<Symbol*>& searchedScopes)
{
	auto scope = pa.context;
	if (searchedScopes.Contains(scope))
	{
		return resolving;
	}
	else
	{
		searchedScopes.Add(scope);
	}

#define RESOLVED_SYMBOLS_COUNT (resolving ? resolving->resolvedSymbols.Count() : 0)
#define FOUND (baseline < RESOLVED_SYMBOLS_COUNT)

	vint baseline = RESOLVED_SYMBOLS_COUNT;
	while (scope)
	{
		vint index = scope->children.Keys().IndexOf(name.name);
		if (index != -1)
		{
			const auto& symbols = scope->children.GetByIndex(index);
			for (vint i = 0; i < symbols.Count(); i++)
			{
				auto symbol = symbols[i].Obj();
				if (symbol->forwardDeclarationRoot)
				{
					symbol = symbol->forwardDeclarationRoot;
				}

				for (vint i = 0; i < symbol->decls.Count(); i++)
				{
					IsPotentialTypeDeclVisitor visitor;
					symbol->decls[i]->Accept(&visitor);
					if (visitor.isPotentialType)
					{
						AddSymbolToResolve(resolving, symbol);
						break;
					}
				}
			}
		}
		if (FOUND) break;

		if (scope->decls.Count() > 0)
		{
			if (auto decl = scope->decls[0].Cast<ClassDeclaration>())
			{
				if (decl->name.name == name.name && policy != SearchPolicy::ChildSymbol)
				{
					AddSymbolToResolve(resolving, decl->symbol);
				}
				else
				{
					for (vint i = 0; i < decl->baseTypes.Count(); i++)
					{
						auto childPolicy =
							policy == SearchPolicy::ChildSymbol
							? SearchPolicy::ChildSymbol
							: SearchPolicy::ChildSymbolRequestedFromSubClass;
						resolving = ResolveChildSymbolInternal(pa, decl->baseTypes[i].f1, name, resolving, childPolicy, category, searchedScopes);
					}
				}
			}
		}
		if (FOUND) break;

		if (scope->usingNss.Count() > 0)
		{
			for (vint i = 0; i < scope->usingNss.Count(); i++)
			{
				auto usingNs = scope->usingNss[i];
				ParsingArguments newPa(pa, usingNs);
				resolving = ResolveTypeSymbol(newPa, name, resolving, SearchPolicy::ChildSymbol);
			}
		}
		if (FOUND) break;

		if (policy != SearchPolicy::SymbolAccessableInScope) break;
		scope = scope->parent;
	}

#undef FOUND
#undef RESOLVED_SYMBOLS_COUNT

	return resolving;
}

/***********************************************************************
ResolveTypeSymbol / ResolveValueSymbol
***********************************************************************/

Ptr<Resolving> ResolveTypeSymbol(const ParsingArguments& pa, CppName& name, Ptr<Resolving> resolving, SearchPolicy policy)
{
	SortedList<Symbol*> searchedScopes;
	return ResolveSymbolInternal(pa, name, resolving, policy, SearchCategory::Type, searchedScopes);
}

Ptr<Resolving> ResolveValueSymbol(const ParsingArguments& pa, CppName& name, Ptr<Resolving> resolving, SearchPolicy policy)
{
	SortedList<Symbol*> searchedScopes;
	return ResolveSymbolInternal(pa, name, resolving, policy, SearchCategory::Value, searchedScopes);
}

/***********************************************************************
ResolveChildSymbolInternal
***********************************************************************/

class ResolveChildSymbolTypeVisitor : public Object, public virtual ITypeVisitor
{
public:
	const ParsingArguments&		pa;
	CppName&					name;
	Ptr<Resolving>				resolving;
	SearchPolicy				policy;
	SearchCategory				category;
	SortedList<Symbol*>&		searchedScopes;

	ResolveChildSymbolTypeVisitor(const ParsingArguments& _pa, CppName& _name, Ptr<Resolving> _resolving, SearchPolicy _policy, SearchCategory _category, SortedList<Symbol*>& _searchedScopes)
		:pa(_pa)
		, name(_name)
		, resolving(_resolving)
		, policy(_policy)
		, category(_category)
		, searchedScopes(_searchedScopes)
	{
	}

	void ResolveChildTypeOfResolving(Ptr<Resolving> parentResolving)
	{
		if (parentResolving)
		{
			parentResolving->Calibrate();
			auto& symbols = parentResolving->resolvedSymbols;
			for (vint i = 0; i < symbols.Count(); i++)
			{
				resolving = ResolveSymbolInternal({ pa,symbols[i] }, name, resolving, policy, category, searchedScopes);
			}
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
		self->type->Accept(this);
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
		self->type->Accept(this);
	}

	void Visit(RootType* self)override
	{
		resolving = ResolveSymbolInternal({ pa,pa.root.Obj() }, name, resolving, SearchPolicy::ChildSymbol, category, searchedScopes);
	}

	void Visit(IdType* self)override
	{
		ResolveChildTypeOfResolving(self->resolving);
	}

	void Visit(ChildType* self)override
	{
		ResolveChildTypeOfResolving(self->resolving);
	}

	void Visit(GenericType* self)override
	{
		self->type->Accept(this);
	}

	void Visit(VariadicTemplateArgumentType* self)override
	{
	}
};

Ptr<Resolving> ResolveChildSymbolInternal(const ParsingArguments& pa, Ptr<Type> classType, CppName& name, Ptr<Resolving> resolving, SearchPolicy policy, SearchCategory category, SortedList<Symbol*>& searchedScopes)
{
	ResolveChildSymbolTypeVisitor visitor(pa, name, resolving, policy, category, searchedScopes);
	classType->Accept(&visitor);
	return visitor.resolving;
}

/***********************************************************************
ResolveTypeSymbol / ResolveValueSymbol
***********************************************************************/

Ptr<Resolving> ResolveChildTypeSymbol(const ParsingArguments& pa, Ptr<Type> classType, CppName& name, Ptr<Resolving> resolving)
{
	SortedList<Symbol*> searchedScopes;
	return ResolveChildSymbolInternal(pa, classType, name, resolving, SearchPolicy::ChildSymbol, SearchCategory::Type, searchedScopes);
}

Ptr<Resolving> ResolveChildValueSymbol(const ParsingArguments& pa, Ptr<Type> classType, CppName& name, Ptr<Resolving> resolving)
{
	SortedList<Symbol*> searchedScopes;
	return ResolveChildSymbolInternal(pa, classType, name, resolving, SearchPolicy::ChildSymbol, SearchCategory::Value, searchedScopes);
}