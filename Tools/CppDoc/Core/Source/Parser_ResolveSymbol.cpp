#include "Ast_Resolving.h"

/***********************************************************************
IsPotentialTypeDeclVisitor
***********************************************************************/

void ResolveSymbolResult::Merge(Ptr<Resolving>& to, Ptr<Resolving> from)
{
	if (!from) return;

	if (!to)
	{
		to = MakePtr<Resolving>();
	}

	for (vint i = 0; i < from->resolvedSymbols.Count(); i++)
	{
		auto symbol = from->resolvedSymbols[i];
		if (!to->resolvedSymbols.Contains(symbol))
		{
			to->resolvedSymbols.Add(symbol);
		}
	}
}

void ResolveSymbolResult::Merge(const ResolveSymbolResult& rar)
{
	Merge(types, rar.types);
	Merge(values, rar.values);
}

/***********************************************************************
ResolveSymbolArguments
***********************************************************************/

struct ResolveSymbolArguments
{
	CppName&					name;
	ResolveSymbolResult&		result;
	bool&						found;
	SortedList<Symbol*>&		searchedScopes;

	ResolveSymbolArguments(CppName& _name, ResolveSymbolResult& _result, bool& _found, SortedList<Symbol*>& _searchedScopes)
		:name(_name)
		, result(_result)
		, found(_found)
		, searchedScopes(_searchedScopes)
	{
	}
};

#define PREPARE_RSA														\
	bool found = false;													\
	SortedList<Symbol*> searchedScopes;									\
	ResolveSymbolArguments rsa(name, input, found, searchedScopes)		\

void ResolveChildSymbolInternal(const ParsingArguments& pa, Ptr<Type> classType, SearchPolicy policy, ResolveSymbolArguments& rsa);

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
ResolveSymbolInternal
***********************************************************************/

void ResolveSymbolInternal(const ParsingArguments& pa, SearchPolicy policy, ResolveSymbolArguments& rsa)
{
	auto scope = pa.scopeSymbol;
	if (rsa.searchedScopes.Contains(scope))
	{
		return;
	}
	else
	{
		rsa.searchedScopes.Add(scope);
	}

	while (scope)
	{
		auto currentClassDecl = scope->GetImplDecl_NFb<ClassDeclaration>();
		bool switchFromSymbolAccessableInScope = false;
		if (currentClassDecl && policy == SearchPolicy::SymbolAccessableInScope)
		{
			policy = SearchPolicy::ChildSymbolFromMemberInside;
			switchFromSymbolAccessableInScope = true;
		}

		if (currentClassDecl && currentClassDecl->name.name == rsa.name.name && policy != SearchPolicy::ChildSymbolFromOutside)
		{
			// A::A could never be the type A
			// But searching A inside A will get the type A
			rsa.found = true;
			AddSymbolToResolve(rsa.result.types, currentClassDecl->symbol);
		}

		if (auto pSymbols = scope->TryGetChildren_NFb(rsa.name.name))
		{
			for (vint i = 0; i < pSymbols->Count(); i++)
			{
				auto symbol = pSymbols->Get(i).Obj();
				switch (symbol->kind)
				{
					// type symbols
				case symbol_component::SymbolKind::Enum:
				case symbol_component::SymbolKind::Class:
				case symbol_component::SymbolKind::Struct:
				case symbol_component::SymbolKind::Union:
				case symbol_component::SymbolKind::TypeAlias:
				case symbol_component::SymbolKind::Namespace:
					rsa.found = true;
					AddSymbolToResolve(rsa.result.types, symbol);
					break;

					// value symbols
				case symbol_component::SymbolKind::EnumItem:
				case symbol_component::SymbolKind::FunctionSymbol:
				case symbol_component::SymbolKind::Variable:
				case symbol_component::SymbolKind::ValueAlias:
					rsa.found = true;
					AddSymbolToResolve(rsa.result.values, symbol);
					break;

				case symbol_component::SymbolKind::GenericTypeArgument:
					if (policy == SearchPolicy::ChildSymbolFromMemberInside || policy == SearchPolicy::SymbolAccessableInScope)
					{
						rsa.found = true;
						AddSymbolToResolve(rsa.result.types, symbol);
					}
					break;

				case symbol_component::SymbolKind::GenericValueArgument:
					if (policy == SearchPolicy::ChildSymbolFromMemberInside || policy == SearchPolicy::SymbolAccessableInScope)
					{
						rsa.found = true;
						AddSymbolToResolve(rsa.result.values, symbol);
					}
					break;
				}
			}
		}

		if (scope->usingNss.Count() > 0)
		{
			for (vint i = 0; i < scope->usingNss.Count(); i++)
			{
				auto usingNs = scope->usingNss[i];
				auto newPa = pa.WithScope(usingNs);
				ResolveSymbolInternal(newPa, SearchPolicy::ChildSymbolFromOutside, rsa);
			}
		}

		if (rsa.found) break;

		if (auto cache = scope->GetClassMemberCache_NFb())
		{
			auto childPolicy
				= cache->symbolDefinedInsideClass
				? SearchPolicy::ChildSymbolFromMemberInside
				: SearchPolicy::ChildSymbolFromMemberOutside
				;
			for (vint i = 0; i < cache->classSymbols.Count(); i++)
			{
				auto newPa = pa.WithScope(cache->classSymbols[i]);
				ResolveSymbolInternal(newPa, childPolicy, rsa);
			}
		}

		if (rsa.found) break;

		switch (policy)
		{
		case SearchPolicy::SymbolAccessableInScope:
			if (auto cache = scope->GetClassMemberCache_NFb())
			{
				scope = cache->parentScope;
			}
			else
			{
				scope = scope->GetParentScope();
			}
			break;
		case SearchPolicy::ChildSymbolFromOutside:
		case SearchPolicy::ChildSymbolFromSubClass:
		case SearchPolicy::ChildSymbolFromMemberInside:
		case SearchPolicy::ChildSymbolFromMemberOutside:
			{
				// could be just a forward declaration
				if (currentClassDecl)
				{
					for (vint i = 0; i < currentClassDecl->baseTypes.Count(); i++)
					{
						auto baseType = currentClassDecl->baseTypes[i].f1;
						ResolveChildSymbolInternal(
							pa,
							baseType,
							(policy == SearchPolicy::ChildSymbolFromOutside ? SearchPolicy::ChildSymbolFromOutside : SearchPolicy::ChildSymbolFromSubClass),
							rsa
							);
					}
				}

				if (switchFromSymbolAccessableInScope)
				{
					// classMemberCache does not exist
					policy = SearchPolicy::SymbolAccessableInScope;
					scope = scope->GetParentScope();
				}
				else
				{
					scope = nullptr;
				}
			}
			break;
		}
	}

#undef FOUND
#undef RESOLVED_SYMBOLS_COUNT
}

/***********************************************************************
ResolveSymbol
***********************************************************************/

ResolveSymbolResult ResolveSymbol(const ParsingArguments& pa, CppName& name, SearchPolicy policy, ResolveSymbolResult input)
{
	PREPARE_RSA;
	ResolveSymbolInternal(pa, policy, rsa);
	return rsa.result;
}

/***********************************************************************
ResolveChildSymbolInternal
***********************************************************************/

class ResolveChildSymbolTypeVisitor : public Object, public virtual ITypeVisitor
{
public:
	const ParsingArguments&		pa;
	SearchPolicy				policy;
	ResolveSymbolArguments&		rsa;

	ResolveChildSymbolTypeVisitor(const ParsingArguments& _pa, SearchPolicy _policy, ResolveSymbolArguments& _rsa)
		:pa(_pa)
		, policy(_policy)
		, rsa(_rsa)
	{
	}

	void ResolveChildTypeOfResolving(Ptr<Resolving> parentResolving)
	{
		if (parentResolving)
		{
			auto& symbols = parentResolving->resolvedSymbols;
			for (vint i = 0; i < symbols.Count(); i++)
			{
				auto symbol = symbols[i];
				if (symbol->GetCategory() == symbol_component::SymbolCategory::Normal)
				{
					if (auto usingDecl = symbol->GetImplDecl_NFb<TypeAliasDeclaration>())
					{
						auto& types = symbol_type_resolving::EvaluateTypeAliasSymbol(pa, usingDecl.Obj());
						for (vint i = 0; i < types.Count(); i++)
						{
							auto tsys = types[i];
							if (tsys->GetType() == TsysType::Decl)
							{
								symbol = tsys->GetDecl();
								continue;
							}
						}
					}
				}
				ResolveSymbolInternal(pa.WithScope(symbol), policy, rsa);
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
		if (self->expr)
		{
			throw 0;
		}
	}

	void Visit(DecorateType* self)override
	{
		self->type->Accept(this);
	}

	void Visit(RootType* self)override
	{
		ResolveSymbolInternal(pa.WithScope(pa.root.Obj()), SearchPolicy::ChildSymbolFromOutside, rsa);
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
};

void ResolveChildSymbolInternal(const ParsingArguments& pa, Ptr<Type> classType, SearchPolicy policy, ResolveSymbolArguments& rsa)
{
	ResolveChildSymbolTypeVisitor visitor(pa, policy, rsa);
	classType->Accept(&visitor);
}

/***********************************************************************
ResolveChildSymbol
***********************************************************************/

ResolveSymbolResult ResolveChildSymbol(const ParsingArguments& pa, Ptr<Type> classType, CppName& name, ResolveSymbolResult input)
{
	PREPARE_RSA;
	ResolveChildSymbolInternal(pa, classType, SearchPolicy::ChildSymbolFromOutside, rsa);
	return rsa.result;
}