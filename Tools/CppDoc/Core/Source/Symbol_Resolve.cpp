#include "Symbol_Resolve.h"
#include "EvaluateSymbol.h"
#include "Ast_Type.h"
#include "Ast_Expr.h"
#include "Parser_Declarator.h"

/***********************************************************************
SearchPolicy
***********************************************************************/

enum class SearchPolicy
{
	_NoFeature = 0,
	_ClassNameAsType = 1,
	_AllowTemplateArgument = 2,
	_AllowClassMember = 4,
	_AccessParentScope = 8,
	_AccessClassBaseType = 16,

	// when in a scope S, search for <NAME> that is accessible in S
	InContext
		= _ClassNameAsType
		| _AllowTemplateArgument
		| _AllowClassMember
		| _AccessParentScope
		| _AccessClassBaseType
		,

	// when in a scope S, search for <NAME> that is accessible in S, skip all class contexts
	InNamespaceContext
		= _ClassNameAsType
		| _AllowTemplateArgument
		| _AccessParentScope
		| _AccessClassBaseType
		,

	// when in any scope, search for Something::<NAME>, if scope is a class, never search in any base class
	ScopedChild_NoBaseType
		= _AllowClassMember
		,

	// when in any scope, search for Something::<NAME>
	ScopedChild
		= _AllowClassMember
		| _AccessClassBaseType
		,

	// when in class C, search for <NAME>, which is a member of C
	ClassMember_FromInside
		= _ClassNameAsType
		| _AllowTemplateArgument
		| _AllowClassMember
		| _AccessClassBaseType
		,

	// when not in class C, search for <NAME>, which is a member of C
	ClassMember_FromOutside
		= _ClassNameAsType
		| _AllowClassMember
		| _AccessClassBaseType
		,
};

constexpr bool operator& (SearchPolicy a, SearchPolicy b)
{
	return ((vint)a & (vint)b) > 0;
}

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

	for (vint i = 0; i < from->items.Count(); i++)
	{
		auto ritem = from->items[i];
		if (!to->items.Contains(ritem))
		{
			to->items.Add(ritem);
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
	ResolveSymbolResult			result;
	bool						cStyleTypeReference = false;
	CppName&					name;
	SortedList<Symbol*>			searchedScopes;
	Dictionary<ITsys*, bool>	searchedTypes;

	ResolveSymbolArguments(CppName& _name)
		:name(_name)
	{
	}
};

/***********************************************************************
AddSymbolToResolve
***********************************************************************/

void AddSymbolToResolve(Ptr<Resolving>& resolving, ITsys* thisItem, Symbol* childSymbol)
{
	if (!resolving)
	{
		resolving = new Resolving;
	}

	ResolvedItem ritem(thisItem, childSymbol);
	if (!resolving->items.Contains(ritem))
	{
		resolving->items.Add(ritem);
	}
}

void AddResolvedItemsToResolve(Ptr<Resolving>& resolving, List<ResolvedItem>& ritems)
{
	if (!resolving)
	{
		resolving = new Resolving;
	}

	for (vint i = 0; i < ritems.Count(); i++)
	{
		auto& ritem = ritems[i];
		if (!resolving->items.Contains(ritem))
		{
			resolving->items.Add(ritem);
		}
	}
}

void AddChildSymbolToResolve(const ParsingArguments& pa, Ptr<Resolving>& resolving, ITsys* thisItem, const symbol_component::ChildSymbol& childSymbol)
{
	if (childSymbol.childType)
	{
		auto newPa = thisItem ? symbol_type_resolving::GetPaInsideClass(pa, thisItem) : pa;
		auto rsr = ResolveChildSymbol(newPa, childSymbol.childType->classType, childSymbol.childType->name);
		if (rsr.types)
		{
			AddResolvedItemsToResolve(resolving, rsr.types->items);
		}
	}
	else if (childSymbol.childExpr)
	{
		auto newPa = thisItem ? symbol_type_resolving::GetPaInsideClass(pa, thisItem) : pa;
		auto rsr = ResolveChildSymbol(newPa, childSymbol.childExpr->classType, childSymbol.childExpr->name);
		if (rsr.values)
		{
			AddResolvedItemsToResolve(resolving, rsr.values->items);
		}
	}
	else
	{
		AddSymbolToResolve(resolving, thisItem, childSymbol.childSymbol.Obj());
	}
}

/***********************************************************************
PickResolvedSymbols
***********************************************************************/

void PickResolvedSymbols(const ParsingArguments& pa, ITsys* thisItem, const List<symbol_component::ChildSymbol>* pSymbols, bool allowTemplateArgument, bool& found, ResolveSymbolArguments& rsa)
{
	bool hasCStyleType = false;
	bool hasOthers = false;
	for (vint i = 0; i < pSymbols->Count(); i++)
	{
		auto symbol = pSymbols->Get(i).childSymbol.Obj();
		switch (symbol->kind)
		{
		case symbol_component::SymbolKind::Enum:
		case symbol_component::SymbolKind::Class:
		case symbol_component::SymbolKind::Struct:
		case symbol_component::SymbolKind::Union:
			hasCStyleType = true;
			break;
		default:
			hasOthers = true;
		}
	}

	bool acceptCStyleType = false;
	bool acceptOthers = false;

	if (hasCStyleType && hasOthers)
	{
		acceptCStyleType = rsa.cStyleTypeReference;
		acceptOthers = !rsa.cStyleTypeReference;
	}
	else if (hasCStyleType)
	{
		acceptCStyleType = true;
	}
	else if (hasOthers)
	{
		acceptOthers = !rsa.cStyleTypeReference;
	}
	else
	{
		throw L"This could not happen, because pSymbols should be nullptr in this case.";
	}

	if (acceptCStyleType)
	{
		for (vint i = 0; i < pSymbols->Count(); i++)
		{
			auto& symbol = pSymbols->Get(i);
			switch (symbol.childSymbol->kind)
			{
				// type symbols
			case CSTYLE_TYPE_SYMBOL_KIND:
				found = true;
				AddChildSymbolToResolve(pa, rsa.result.types, thisItem, symbol);
				break;
			}
		}
	}

	if (acceptOthers)
	{
		for (vint i = 0; i < pSymbols->Count(); i++)
		{
			auto& symbol = pSymbols->Get(i);
			switch (symbol.childSymbol->kind)
			{
				// type symbols
			case symbol_component::SymbolKind::TypeAlias:
			case symbol_component::SymbolKind::Namespace:
				found = true;
				AddChildSymbolToResolve(pa, rsa.result.types, thisItem, symbol);
				break;

				// value symbols
			case symbol_component::SymbolKind::EnumItem:
			case symbol_component::SymbolKind::FunctionSymbol:
			case symbol_component::SymbolKind::Variable:
			case symbol_component::SymbolKind::ValueAlias:
				found = true;
				AddChildSymbolToResolve(pa, rsa.result.values, thisItem, symbol);
				break;

				// template type argument
			case symbol_component::SymbolKind::GenericTypeArgument:
				if (allowTemplateArgument)
				{
					found = true;
					AddSymbolToResolve(rsa.result.types, nullptr, symbol.childSymbol.Obj());
				}
				break;

				// template value argument
			case symbol_component::SymbolKind::GenericValueArgument:
				if (allowTemplateArgument)
				{
					found = true;
					AddSymbolToResolve(rsa.result.values, nullptr, symbol.childSymbol.Obj());
				}
				break;
			}
		}
	}
}

void PickTemplateArgumentSymbols(const List<symbol_component::ChildSymbol>* pSymbols, bool& found, ResolveSymbolArguments& rsa)
{
	if (!rsa.cStyleTypeReference)
	{
		for (vint i = 0; i < pSymbols->Count(); i++)
		{
			auto symbol = pSymbols->Get(i).childSymbol.Obj();
			switch (symbol->kind)
			{
				// template type argument
			case symbol_component::SymbolKind::GenericTypeArgument:
				found = true;
				AddSymbolToResolve(rsa.result.types, nullptr, symbol);
				break;

				// template value argument
			case symbol_component::SymbolKind::GenericValueArgument:
				found = true;
				AddSymbolToResolve(rsa.result.values, nullptr, symbol);
				break;
			}
		}
	}
}

/***********************************************************************
ResolveSymbolInTypeInternal
***********************************************************************/

bool ResolveSymbolInTypeInternal(const ParsingArguments& pa, ITsys* tsys, SearchPolicy policy, ResolveSymbolArguments& rsa);

bool ResolveSymbolInClassInternal(const ParsingArguments& pa, ITsys* tsys, ITsys* entity, SearchPolicy policy, ResolveSymbolArguments& rsa)
{
	bool found = false;
	auto scope = entity->GetDecl();
	if (auto classDecl = scope->GetImplDecl_NFb<ClassDeclaration>())
	{
		if (classDecl->name.name == rsa.name.name)
		{
			if (policy & SearchPolicy::_ClassNameAsType)
			{
				// A::A could never be the type A
				// But searching A inside A will get the type A
				found = true;

				switch (entity->GetType())
				{
				case TsysType::Decl:
					Resolving::AddSymbol(pa, rsa.result.types, classDecl->symbol);
					break;
				case TsysType::DeclInstant:
					{
						auto& di = entity->GetDeclInstant();
						if (di.parentDeclType)
						{
							auto parentSymbol = classDecl->symbol->GetParentScope();
							if (di.parentDeclType->GetDecl() == parentSymbol)
							{
								AddSymbolToResolve(rsa.result.types, di.parentDeclType, classDecl->symbol);
							}
							else
							{
								auto parentClass = pa.tsys->DeclInstantOf(classDecl->symbol->GetParentScope(), nullptr, di.parentDeclType);
								AddSymbolToResolve(rsa.result.types, parentClass, classDecl->symbol);
							}
						}
						else
						{
							Resolving::AddSymbol(pa, rsa.result.types, classDecl->symbol);
						}
					}
					break;
				}
			}
		}

		if (auto pSymbols = scope->TryGetChildren_NFb(rsa.name.name))
		{
			PickResolvedSymbols(pa, tsys, pSymbols, (policy & SearchPolicy::_AllowTemplateArgument), found, rsa);
		}

		if (found) return found;

		if (policy & SearchPolicy::_AccessClassBaseType)
		{
			ClassDeclaration* cd = nullptr;
			ITsys* pdt = nullptr;
			TemplateArgumentContext* ata = nullptr;
			symbol_type_resolving::ExtractClassType(tsys, cd, pdt, ata);
			symbol_type_resolving::EnumerateClassSymbolBaseTypes(pa, cd, pdt, ata, [&](ITsys* classType, ITsys* baseType)
			{
				auto basePolicy
					= policy == SearchPolicy::ScopedChild || policy == SearchPolicy::InContext
					? SearchPolicy::ScopedChild
					: SearchPolicy::ClassMember_FromOutside
					;
				found = ResolveSymbolInTypeInternal(pa, baseType, basePolicy, rsa) || found;
				return false;
			});
		}
	}
	return found;
}

bool ResolveSymbolInTypeInternal(const ParsingArguments& pa, ITsys* tsys, SearchPolicy policy, ResolveSymbolArguments& rsa)
{
	if (tsys->GetType() == TsysType::GenericFunction)
	{
		tsys = tsys->GetElement();
	}

	TsysCV cv;
	TsysRefType ref;
	auto entity = tsys->GetEntity(cv, ref);

	{
		vint index = rsa.searchedTypes.Keys().IndexOf(entity);
		if (index != -1)
		{
			return rsa.searchedTypes.Values()[index];
		}
	}

	rsa.searchedTypes.Add(entity, false);

	bool found = false;
	if (entity->GetType() == TsysType::Decl || entity->GetType() == TsysType::DeclInstant)
	{
		auto scope = entity->GetDecl();
		if (auto classDecl = scope->GetAnyForwardDecl<ForwardClassDeclaration>())
		{
			symbol_type_resolving::EnumerateClassPSInstances(pa, entity, [&](ITsys* psEntity)
			{
				if (psEntity->GetType() == TsysType::GenericFunction)
				{
					psEntity = psEntity->GetElement();
				}
				found = ResolveSymbolInClassInternal(pa, CvRefOf(psEntity, cv, ref), psEntity, policy, rsa) || found;
				return false;
			});
		}
		else if (auto enumDecl = scope->GetImplDecl_NFb<EnumDeclaration>())
		{
			if (auto pSymbols = scope->TryGetChildren_NFb(rsa.name.name))
			{
				PickResolvedSymbols(pa, tsys, pSymbols, (policy & SearchPolicy::_AllowTemplateArgument), found, rsa);
			}
		}
	}
	rsa.searchedTypes.Set(entity, found);
	return found;
}

/***********************************************************************
ResolveSymbolInStaticScopeInternal
***********************************************************************/

void ResolveSymbolInStaticScopeInternal(const ParsingArguments& pa, Symbol* scope, SearchPolicy policy, ResolveSymbolArguments& rsa)
{
	bool found = false;
	while (scope)
	{
		Ptr<symbol_component::ClassMemberCache> cache;

		if (auto classDecl = scope->GetAnyForwardDecl<ForwardClassDeclaration>())
		{
			if (policy & SearchPolicy::_AllowTemplateArgument)
			{
				// if we can find a template argument, then don't need to continue searching for class members
				if (auto pSymbols = scope->TryGetChildren_NFb(rsa.name.name))
				{
					PickTemplateArgumentSymbols(pSymbols, found, rsa);
				}

				if (found) break;

				// for a class forward declaration, symbols are not in the class's scope
				if (classDecl->keepTemplateArgumentAlive)
				{
					if (auto pSymbols = classDecl->keepTemplateArgumentAlive->TryGetChildren_NFb(rsa.name.name))
					{
						PickTemplateArgumentSymbols(pSymbols, found, rsa);
					}
				}

				if (found) break;
			}

			// check if pa.taContext has enough template arguments for this class
			// if not, then it could be evaluating the default template argument, skip class members
			bool readyForClassMember = true;
			if(classDecl->templateSpec && classDecl->templateSpec->arguments.Count() > 0)
			{
				auto ata = pa.taContext;
				while (ata)
				{
					if (ata->GetSymbolToApply() == classDecl->symbol)
					{
						if (ata->GetAvailableArgumentCount() < classDecl->templateSpec->arguments.Count())
						{
							readyForClassMember = false;
						}
						break;
					}
					ata = ata->parent;
				}
			}

			if (readyForClassMember && (policy & SearchPolicy::_AllowClassMember))
			{
				// if pa is in a context of an instanciated generic class
				// find the correct parentDeclType and argumentsToApply
				auto parentTemplateClassSymbol = FindParentTemplateClassSymbol(scope);
				auto pdt = pa.parentDeclType;
				auto ata = pa.taContext;

				if (parentTemplateClassSymbol)
				{
					while (pdt && pdt->GetDecl() != parentTemplateClassSymbol)
					{
						pdt = pdt->GetDeclInstant().parentDeclType;
					}
				}
				else
				{
					pdt = nullptr;
				}

				while (ata && ata->GetSymbolToApply() != scope)
				{
					ata = ata->parent;
				}

				auto& tsyses = symbol_type_resolving::EvaluateForwardClassSymbol(pa, classDecl.Obj(), pdt, ata);
				for (vint i = 0; i < tsyses.Count(); i++)
				{
					auto tsys = tsyses[i];
					found = ResolveSymbolInTypeInternal(pa, tsys, policy, rsa) || found;
				}
			}

			if (found) break;
		}
		else
		{
			if (auto pSymbols = scope->TryGetChildren_NFb(rsa.name.name))
			{
				PickResolvedSymbols(pa.AdjustForDecl(scope), nullptr, pSymbols, (policy & SearchPolicy::_AllowTemplateArgument), found, rsa);
			}

			if (scope->usingNss.Count() > 0)
			{
				for (vint i = 0; i < scope->usingNss.Count(); i++)
				{
					auto usingNs = scope->usingNss[i];
					ResolveSymbolInStaticScopeInternal(pa, usingNs, SearchPolicy::ScopedChild, rsa);
				}
			}

			if (found) break;

			cache = scope->GetClassMemberCache_NFb();

			if (cache && policy & SearchPolicy::_AllowClassMember)
			{
				auto childPolicy
					= cache->symbolDefinedInsideClass
					? SearchPolicy::ClassMember_FromInside
					: SearchPolicy::ClassMember_FromOutside
					;
				for (vint i = 0; i < cache->containerClassTypes.Count(); i++)
				{
					auto classType = ReplaceGenericArgsInClass(pa, cache->containerClassTypes[i]);
					found = ResolveSymbolInTypeInternal(pa, classType, childPolicy, rsa) || found;
					if (found) break;
				}
			}

			if (found) break;
		}

		if (policy & SearchPolicy::_AccessParentScope)
		{
			// classMemberCache does not exist in class scope
			if (cache)
			{
				scope = cache->parentScope;
			}
			else
			{
				scope = scope->GetParentScope();
			}
		}
		else
		{
			scope = nullptr;
		}
	}
}

/***********************************************************************
ResolveSymbolInContext
***********************************************************************/

ResolveSymbolResult ResolveSymbolInContext(const ParsingArguments& pa, CppName& name, bool cStyleTypeReference)
{
	ResolveSymbolArguments rsa(name);
	rsa.cStyleTypeReference = cStyleTypeReference;
	ResolveSymbolInStaticScopeInternal(pa, pa.scopeSymbol, SearchPolicy::InContext, rsa);
	return rsa.result;
}

/***********************************************************************
ResolveSymbolInNamespaceContext
***********************************************************************/

ResolveSymbolResult ResolveSymbolInNamespaceContext(const ParsingArguments& pa, Symbol* ns, CppName& name, bool cStyleTypeReference)
{
	ResolveSymbolArguments rsa(name);
	rsa.cStyleTypeReference = cStyleTypeReference;
	ResolveSymbolInStaticScopeInternal(pa, ns, SearchPolicy::InNamespaceContext, rsa);
	return rsa.result;
}

/***********************************************************************
ResolveChildSymbol on ITsys*
***********************************************************************/

ResolveSymbolResult ResolveChildSymbol(const ParsingArguments& pa, ITsys* tsysDecl, CppName& name, bool searchInBaseTypes)
{
	ResolveSymbolArguments rsa(name);
	if (searchInBaseTypes)
	{
		ResolveSymbolInTypeInternal(pa, tsysDecl, SearchPolicy::ScopedChild, rsa);
	}
	else
	{
		ResolveSymbolInTypeInternal(pa, tsysDecl, SearchPolicy::ScopedChild_NoBaseType, rsa);
	}
	return rsa.result;
}

/***********************************************************************
ResolveChildSymbol on Ptr<Type>
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

	void ResolveChildSymbolOfType(Type* type)
	{
		TypeTsysList tsyses;
		bool isVta = false;
		TypeToTsysInternal(pa, type, tsyses, isVta);
		for (vint i = 0; i < tsyses.Count(); i++)
		{
			auto tsys = tsyses[i];
			if (tsys->GetType() == TsysType::Init)
			{
				if (isVta)
				{
					for (vint j = 0; j < tsys->GetParamCount(); j++)
					{
						ResolveSymbolInTypeInternal(pa, tsys->GetParam(j), policy, rsa);
					}
				}
			}
			else
			{
				ResolveSymbolInTypeInternal(pa, tsys, policy, rsa);
			}
		}
	}

	void Visit(PrimitiveType* self)override
	{
	}

	void Visit(ReferenceType* self)override
	{
		ResolveChildSymbolOfType(self);
	}

	void Visit(ArrayType* self)override
	{
	}

	void Visit(DecorateType* self)override
	{
		ResolveChildSymbolOfType(self);
	}

	void Visit(CallingConventionType* self)override
	{
		ResolveChildSymbolOfType(self);
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
			ResolveChildSymbolOfType(self);
		}
	}

	void Visit(RootType* self)override
	{
		ResolveSymbolInStaticScopeInternal(pa, pa.root.Obj(), SearchPolicy::ScopedChild, rsa);
	}

	void VisitIdOrChildType(Type* self, Ptr<Resolving>& resolving)
	{
		if (Resolving::IsResolvedToType(resolving))
		{
			ResolveChildSymbolOfType(self);
		}
		else
		{
			for (vint i = 0; i < resolving->items.Count(); i++)
			{
				ResolveSymbolInStaticScopeInternal(pa, resolving->items[i].symbol, SearchPolicy::ScopedChild, rsa);
			}
		}
	}

	void Visit(IdType* self)override
	{
		VisitIdOrChildType(self, self->resolving);
	}

	void Visit(ChildType* self)override
	{
		VisitIdOrChildType(self, self->resolving);
	}

	void Visit(GenericType* self)override
	{
		ResolveChildSymbolOfType(self);
	}
};

ResolveSymbolResult ResolveChildSymbol(const ParsingArguments& pa, Ptr<Type> classType, CppName& name)
{
	ResolveSymbolArguments rsa(name);
	ResolveChildSymbolTypeVisitor visitor(pa, SearchPolicy::ScopedChild, rsa);
	classType->Accept(&visitor);
	return rsa.result;
}

/***********************************************************************
IsResolvedToType
***********************************************************************/

bool IsResolvedToType(Ptr<Type> type)
{
	if (type.Cast<RootType>())
	{
		return false;
	}

	if (auto catIdChildType = type.Cast<Category_Id_Child_Type>())
	{
		return Resolving::IsResolvedToType(catIdChildType->resolving);
	}

	return true;
}