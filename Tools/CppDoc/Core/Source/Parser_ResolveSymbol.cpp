#include "Parser.h"
#include "Ast_Type.h"
#include "Ast_Decl.h"

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

void ResolveSymbolInternal(const ParsingArguments& pa, SearchPolicy policy, ResolveSymbolArguments& rsa)
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
					if (IsPotentialTypeDecl(symbol->decls[i].Obj()))
					{
						AddSymbolToResolve(rsa.result.types, symbol);
					}
					else
					{
						AddSymbolToResolve(rsa.result.values, symbol);
					}
					break;
				}
			}
		}
		if (rsa.found) break;

		if (scope->decls.Count() > 0)
		{
			if (auto decl = scope->decls[0].Cast<ClassDeclaration>())
			{
				if (decl->name.name == rsa.name.name && policy != SearchPolicy::ChildSymbol)
				{
					rsa.found = true;
					AddSymbolToResolve(rsa.result.types, decl->symbol);
				}
				else
				{
					for (vint i = 0; i < decl->baseTypes.Count(); i++)
					{
						auto childPolicy =
							policy == SearchPolicy::ChildSymbol
							? SearchPolicy::ChildSymbol
							: SearchPolicy::ChildSymbolRequestedFromSubClass;
						ResolveChildSymbolInternal(pa, decl->baseTypes[i].f1, childPolicy, rsa);
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
				auto newPa = pa.WithContextNoFunction(usingNs);
				ResolveSymbolInternal(newPa, SearchPolicy::ChildSymbol, rsa);
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
			parentResolving->Calibrate();
			auto& symbols = parentResolving->resolvedSymbols;
			for (vint i = 0; i < symbols.Count(); i++)
			{
				ResolveSymbolInternal(pa.WithContextNoFunction(symbols[i]), policy, rsa);
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
		ResolveSymbolInternal(pa.WithContextNoFunction(pa.root.Obj()), SearchPolicy::ChildSymbol, rsa);
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
	ResolveChildSymbolInternal(pa, classType, SearchPolicy::ChildSymbol, rsa);
	return rsa.result;
}