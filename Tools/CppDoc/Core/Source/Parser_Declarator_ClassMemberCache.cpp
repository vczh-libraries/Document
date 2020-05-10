#include "Parser.h"
#include "EvaluateSymbol.h"

bool IsInTemplateHeader(const ParsingArguments& pa)
{
	return pa.scopeSymbol->kind == symbol_component::SymbolKind::Root && pa.scopeSymbol->GetParentScope();
}

void FillSymbolToClassMemberCache(const ParsingArguments& pa, Symbol* classSymbol, symbol_component::ClassMemberCache* cache)
{
	auto current = FindParentClassSymbol(classSymbol, true);
	while (current)
	{
		if (auto classDecl = current->GetAnyForwardDecl<ForwardClassDeclaration>())
		{
			auto declPa = pa.AdjustForDecl(current, nullptr, true);
			auto& ev = symbol_type_resolving::EvaluateForwardClassSymbol(declPa, classDecl.Obj(), nullptr, nullptr);
			auto classType = ev[0];
			if (classType->GetType() == TsysType::GenericFunction)
			{
				classType = classType->GetElement();
			}
			cache->containerClassTypes.Add(classType);
		}
		current = FindParentClassSymbol(current, false);
	}
}

Ptr<symbol_component::ClassMemberCache> CreatePartialClassMemberCache(const ParsingArguments& pa, Symbol* classSymbol)
{
	auto cache = MakePtr<symbol_component::ClassMemberCache>();

	cache->symbolDefinedInsideClass = true;
	FillSymbolToClassMemberCache(pa, classSymbol, cache.Obj());
	cache->parentScope = cache->containerClassTypes[cache->containerClassTypes.Count() - 1]->GetDecl()->GetParentScope();

	return cache;
}

Ptr<symbol_component::ClassMemberCache> CreatePartialClassMemberCache(const ParsingArguments& pa, ITsys* classType)
{
	auto cache = MakePtr<symbol_component::ClassMemberCache>();

	cache->symbolDefinedInsideClass = false;
	auto current = classType;
	while (current)
	{
		switch (current->GetType())
		{
		case TsysType::Decl:
			// if current is a trivial class, add all levels of class types
			FillSymbolToClassMemberCache(pa, current->GetDecl(), cache.Obj());
			current = nullptr;
			break;
		case TsysType::DeclInstant:
			// if current is a generic class or a non-generic class inside a generic class
			{
				// add the current class type
				cache->containerClassTypes.Add(current);

				if (auto parentClass = FindParentClassSymbol(current->GetDecl(), false))
				{
					// if there is a parent class
					const auto& di = current->GetDeclInstant();
					if (!di.parentDeclType)
					{
						// if there is no parent generic class, then its parent class is trivial
						current = pa.tsys->DeclOf(parentClass);
					}
					else if (di.parentDeclType->GetDecl() == parentClass)
					{
						// if the parent class is a generic class
						current = di.parentDeclType;
					}
					else
					{
						// if the parent class is a non-generic class
						current = pa.tsys->DeclInstantOf(parentClass, nullptr, di.parentDeclType);
					}
				}
				else
				{
					// if there is no parent class, exit
					current = nullptr;
				}
			}
			break;
		}
	}

	if (IsInTemplateHeader(pa))
	{
		// in this case, the cache will be put in pa.scopeSymbol
		// so we should point parentScope to its parent, to get rid of a dead loop
		cache->parentScope = pa.scopeSymbol->GetParentScope();
	}
	else
	{
		cache->parentScope = pa.scopeSymbol;
	}

	return cache;
}