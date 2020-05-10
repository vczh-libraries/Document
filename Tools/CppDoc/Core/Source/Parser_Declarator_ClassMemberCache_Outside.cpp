#include "Parser.h"
#include "Parser_Declarator.h"
#include "Ast_Decl.h"
#include "Ast_Type.h"
#include "Symbol_Resolve.h"
#include "EvaluateSymbol.h"

bool IsInTemplateHeader(const ParsingArguments& pa)
{
	return pa.scopeSymbol->kind == symbol_component::SymbolKind::Root && pa.scopeSymbol->GetParentScope();
}

ITsys* EnsureClassType(const ParsingArguments& pa, Ptr<Type> classType, Ptr<CppTokenCursor>& cursor)
{
	TypeTsysList classTsys;
	TypeToTsysNoVta(pa, classType, classTsys);
	if (classTsys.Count() != 1) throw StopParsingException(cursor);

	auto tsys = classTsys[0];
	if (tsys->GetType() != TsysType::Decl && tsys->GetType() != TsysType::DeclInstant) throw StopParsingException(cursor);

	auto classDecl = tsys->GetDecl()->GetImplDecl_NFb<ClassDeclaration>().Obj();
	if (!classDecl) throw StopParsingException(cursor);

	return tsys;
}

void AssignContainerClassDeclsToSpecs(
	Ptr<symbol_component::ClassMemberCache> classMemberCache,
	List<Ptr<TemplateSpec>>& specs,
	Ptr<CppTokenCursor>& cursor
)
{
	vint used = 0;
	auto& thisTypes = classMemberCache->containerClassTypes;
	for (vint i = thisTypes.Count() - 1; i >= 0; i--)
	{
		auto thisType = thisTypes[i];
		auto thisDecl = thisType->GetDecl()->GetImplDecl_NFb<ClassDeclaration>();
		if (!thisDecl) throw StopParsingException(cursor);

		if (thisDecl->templateSpec)
		{
			if (used >= specs.Count()) throw StopParsingException(cursor);
			auto thisSpec = specs[used++];
			if (thisSpec->arguments.Count() != thisDecl->templateSpec->arguments.Count()) throw StopParsingException(cursor);
			for (vint j = 0; j < thisSpec->arguments.Count(); j++)
			{
				auto specArg = thisSpec->arguments[j];
				auto declArg = thisDecl->templateSpec->arguments[j];
				if (specArg.argumentType != declArg.argumentType) throw StopParsingException(cursor);
			}
			classMemberCache->containerClassSpecs.Insert(0, thisSpec);
		}
		else
		{
			classMemberCache->containerClassSpecs.Insert(0, {});
		}
	}

	switch (specs.Count() - used)
	{
	case 0:
		break;
	case 1:
		classMemberCache->declSpec = specs[used];
		break;
	default:
		throw StopParsingException(cursor);
	}
}

extern void FillSymbolToClassMemberCache(const ParsingArguments& pa, Symbol* classSymbol, symbol_component::ClassMemberCache* cache);
extern void FixClassMemberCacheTypes(Ptr<symbol_component::ClassMemberCache> classMemberCache);

Ptr<symbol_component::ClassMemberCache> CreatePartialClassMemberCache2(const ParsingArguments& pa, Ptr<Type> classType, List<Ptr<TemplateSpec>>* specs, Ptr<CppTokenCursor>& cursor)
{
	auto cache = MakePtr<symbol_component::ClassMemberCache>();
	cache->symbolDefinedInsideClass = false;

	auto current = EnsureClassType(pa, classType, cursor);
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
	
	List<Ptr<TemplateSpec>> noSpecs;
	FixClassMemberCacheTypes(cache);
	AssignContainerClassDeclsToSpecs(cache, (specs ? *specs : noSpecs), cursor);
	return cache;
}

struct QualifiedIdComponent
{
	Ptr<IdType>			idType;
	Ptr<ChildType>		childType;
	Ptr<GenericType>	genericType;
};

Ptr<symbol_component::ClassMemberCache> CreatePartialClassMemberCache(const ParsingArguments& pa, Ptr<Type> classType, List<Ptr<TemplateSpec>>* specs, Ptr<CppTokenCursor>& cursor)
{
	auto cacheToCompare = CreatePartialClassMemberCache2(pa, classType, specs, cursor);

	auto cache = MakePtr<symbol_component::ClassMemberCache>();
	cache->symbolDefinedInsideClass = false;

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

	Symbol* currentNamespace = nullptr;
	List<QualifiedIdComponent> qics;
	{
		// break class type to qualified identifier components

		auto currentClassType = classType;
		while (true)
		{
			QualifiedIdComponent qic;

			if (auto genericType = currentClassType.Cast<GenericType>())
			{
				qic.genericType = genericType;
				currentClassType = genericType->type;
			}

			if (auto childType = currentClassType.Cast<ChildType>())
			{
				if (Resolving::IsResolvedToType(childType->resolving))
				{
					qic.childType = childType;
					qics.Add(qic);

					currentClassType = childType->classType;
				}
				else
				{
					currentNamespace = childType->resolving->items[0].symbol;
					break;
				}
			}
			else if (auto idType = currentClassType.Cast<IdType>())
			{
				if (Resolving::IsResolvedToType(idType->resolving))
				{
					qic.idType = idType;
					qics.Add(qic);

					currentNamespace = cache->parentScope;
				}
				else
				{
					currentNamespace = idType->resolving->items[0].symbol;
				}
				break;
			}
			else if (currentClassType.Cast<RootType>())
			{
				currentNamespace = pa.root.Obj();
				break;
			}
			else
			{
				throw StopParsingException(cursor);
			}
		}

		if (!currentNamespace || qics.Count() == 0)
		{
			throw StopParsingException(cursor);
		}
	}

	ITsys* currentClass = nullptr;
	ITsys* currentParentDeclType = nullptr;
	vint consumedSpecs = 0;
	{
		// resolving each level of type

		for (vint i = qics.Count() - 1; i >= 0; i--)
		{
			const auto& qic = qics[i];
			auto& name = qic.idType ? qic.idType->name : qic.childType->name;
			auto rsr =
				currentNamespace
				? ResolveSymbolInNamespaceContext(pa, currentNamespace, name, false)
				: ResolveChildSymbol(pa, currentClass, name)
				;
			if (!rsr.types || rsr.types->items.Count() != 1)
			{
				// a type must be resolved
				throw StopParsingException(cursor);
			}

			auto classSymbol = rsr.types->items[0].symbol;
			if (currentNamespace)
			{
				if (classSymbol->GetParentScope() != currentNamespace)
				{
					// the resolved type must be declared as a direct child symbol of this namespace
					throw StopParsingException(cursor);
				}
			}
			else
			{
				if (classSymbol->GetParentScope() != currentClass->GetDecl())
				{
					// the resolved type must be declared as a direct child symbol of this type
					throw StopParsingException(cursor);
				}
			}

			auto classDecl = classSymbol->GetImplDecl_NFb<ClassDeclaration>();
			if (!classDecl)
			{
				// the resolved type must be a class, and it cannot just have a forward declaration
				throw StopParsingException(cursor);
			}

			if (classDecl->templateSpec)
			{
				throw 0;
			}
			else
			{
				auto& classTsys = symbol_type_resolving::EvaluateForwardClassSymbol(pa, classDecl.Obj(), currentParentDeclType, nullptr);
				auto nextClass = classTsys[0];

				cache->containerClassTypes.Insert(0, nextClass);
				if (specs)
				{
					cache->containerClassSpecs.Insert(0, {});
				}
				else
				{
					cache->containerClassSpecs.Insert(0, {});
				}

				currentNamespace = nullptr;
				currentClass = nextClass;
			}
		}
	}

	if (specs)
	{
		switch ((*specs).Count() - consumedSpecs)
		{
		case 0:
			break;
		case 1:
			cache->declSpec = (*specs)[consumedSpecs];
			break;
		default:
			throw StopParsingException(cursor);
		}
	}

	{
		if (cache->containerClassTypes.Count() != cacheToCompare->containerClassTypes.Count())
		{
			throw 0;
		}
		for (vint i = 0; i < cache->containerClassTypes.Count(); i++)
		{
			if (cache->containerClassTypes[i] != cacheToCompare->containerClassTypes[i])
			{
				throw 0;
			}
		}
		if (cache->containerClassSpecs.Count() != cacheToCompare->containerClassSpecs.Count())
		{
			throw 0;
		}
		for (vint i = 0; i < cache->containerClassSpecs.Count(); i++)
		{
			if (cache->containerClassSpecs[i] != cacheToCompare->containerClassSpecs[i])
			{
				throw 0;
			}
		}
		if (cache->declSpec != cacheToCompare->declSpec)
		{
			throw 0;
		}
		if (cache->symbolDefinedInsideClass != cacheToCompare->symbolDefinedInsideClass)
		{
			throw 0;
		}
		if (cache->parentScope != cacheToCompare->parentScope)
		{
			throw 0;
		}
	}
	return cache;
}