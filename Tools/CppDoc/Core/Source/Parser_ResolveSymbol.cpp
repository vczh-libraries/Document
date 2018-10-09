#include "Parser.h"
#include "Ast_Type.h"
#include "Ast_Decl.h"

enum class SearchCategory
{
	Type,
	Value,
};

struct ResolveSymbolArguments
{
	CppName&					name;
	Ptr<Resolving>&				resolving;
	bool&						found;
	SortedList<Symbol*>&		searchedScopes;

	ResolveSymbolArguments(CppName& _name, Ptr<Resolving>& _resolving, bool& _found, SortedList<Symbol*>& _searchedScopes)
		:name(_name)
		, resolving(_resolving)
		, found(_found)
		, searchedScopes(_searchedScopes)
	{
	}
};

#define PREPARE_RSA														\
	bool found = false;													\
	SortedList<Symbol*> searchedScopes;									\
	ResolveSymbolArguments rsa(name, resolving, found, searchedScopes)	\

void ResolveChildSymbolInternal(const ParsingArguments& pa, Ptr<Type> classType, SearchPolicy policy, SearchCategory category, ResolveSymbolArguments& rsa);

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

bool IsPotentialTypeDecl(Declaration* decl)
{
	IsPotentialTypeDeclVisitor visitor;
	decl->Accept(&visitor);
	return visitor.isPotentialType;
}

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

void ResolveSymbolInternal(const ParsingArguments& pa, SearchPolicy policy, SearchCategory category, ResolveSymbolArguments& rsa)
{
	auto scope = pa.context;
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
		vint index = scope->children.Keys().IndexOf(rsa.name.name);
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
					rsa.found = true;
					if (IsPotentialTypeDecl(symbol->decls[i].Obj()) == (category == SearchCategory::Type))
					{
						AddSymbolToResolve(rsa.resolving, symbol);
						break;
					}
				}
			}
		}
		if (rsa.found) break;

		if (scope->decls.Count() > 0)
		{
			if (auto decl = scope->decls[0].Cast<ClassDeclaration>())
			{
				if (decl->name.name == rsa.name.name && policy != SearchPolicy::ChildSymbol && category == SearchCategory::Type)
				{
					rsa.found = true;
					AddSymbolToResolve(rsa.resolving, decl->symbol);
				}
				else
				{
					for (vint i = 0; i < decl->baseTypes.Count(); i++)
					{
						auto childPolicy =
							policy == SearchPolicy::ChildSymbol
							? SearchPolicy::ChildSymbol
							: SearchPolicy::ChildSymbolRequestedFromSubClass;
						ResolveChildSymbolInternal(pa, decl->baseTypes[i].f1, childPolicy, category, rsa);
					}
				}
			}
		}
		if (rsa.found) break;

		if (scope->usingNss.Count() > 0)
		{
			for (vint i = 0; i < scope->usingNss.Count(); i++)
			{
				auto usingNs = scope->usingNss[i];
				ParsingArguments newPa(pa, usingNs);
				ResolveSymbolInternal(newPa, SearchPolicy::ChildSymbol, category, rsa);
			}
		}
		if (rsa.found) break;

		if (policy != SearchPolicy::SymbolAccessableInScope) break;
		scope = scope->parent;
	}

#undef FOUND
#undef RESOLVED_SYMBOLS_COUNT
}

/***********************************************************************
ResolveTypeSymbol / ResolveValueSymbol
***********************************************************************/

Ptr<Resolving> ResolveTypeSymbol(const ParsingArguments& pa, CppName& name, Ptr<Resolving> resolving, SearchPolicy policy)
{
	PREPARE_RSA;
	ResolveSymbolInternal(pa, policy, SearchCategory::Type, rsa);
	return resolving;
}

Ptr<Resolving> ResolveValueSymbol(const ParsingArguments& pa, CppName& name, Ptr<Resolving> resolving, SearchPolicy policy)
{
	PREPARE_RSA;
	ResolveSymbolInternal(pa, policy, SearchCategory::Value, rsa);
	return resolving;
}

/***********************************************************************
ResolveChildSymbolInternal
***********************************************************************/

class ResolveChildSymbolTypeVisitor : public Object, public virtual ITypeVisitor
{
public:
	const ParsingArguments&		pa;
	SearchPolicy				policy;
	SearchCategory				category;
	ResolveSymbolArguments&		rsa;

	ResolveChildSymbolTypeVisitor(const ParsingArguments& _pa, SearchPolicy _policy, SearchCategory _category, ResolveSymbolArguments& _rsa)
		:pa(_pa)
		, policy(_policy)
		, category(_category)
		, rsa(_rsa)
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
				ResolveSymbolInternal({ pa,symbols[i] }, policy, category, rsa);
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
		ResolveSymbolInternal({ pa,pa.root.Obj() }, SearchPolicy::ChildSymbol, category, rsa);
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

void ResolveChildSymbolInternal(const ParsingArguments& pa, Ptr<Type> classType, SearchPolicy policy, SearchCategory category, ResolveSymbolArguments& rsa)
{
	ResolveChildSymbolTypeVisitor visitor(pa, policy, category, rsa);
	classType->Accept(&visitor);
}

/***********************************************************************
ResolveTypeSymbol / ResolveValueSymbol
***********************************************************************/

Ptr<Resolving> ResolveChildTypeSymbol(const ParsingArguments& pa, Ptr<Type> classType, CppName& name, Ptr<Resolving> resolving)
{
	PREPARE_RSA;
	ResolveChildSymbolInternal(pa, classType, SearchPolicy::ChildSymbol, SearchCategory::Type, rsa);
	return resolving;
}

Ptr<Resolving> ResolveChildValueSymbol(const ParsingArguments& pa, Ptr<Type> classType, CppName& name, Ptr<Resolving> resolving)
{
	PREPARE_RSA;
	ResolveChildSymbolInternal(pa, classType, SearchPolicy::ChildSymbol, SearchCategory::Value, rsa);
	return resolving;
}