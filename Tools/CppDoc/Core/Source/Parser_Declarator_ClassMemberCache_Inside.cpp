#include "Parser.h"
#include "Parser_Declarator.h"
#include "Ast_Decl.h"
#include "EvaluateSymbol.h"

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

void FixClassMemberCacheTypes(Ptr<symbol_component::ClassMemberCache> classMemberCache)
{
	for (vint i = 0; i < classMemberCache->containerClassTypes.Count(); i++)
	{
		auto classType = classMemberCache->containerClassTypes[i];
		if (classType->GetType() == TsysType::GenericFunction)
		{
			classType = classType->GetElement();
		}

		auto classDecl = classType->GetDecl()->GetAnyForwardDecl<ForwardClassDeclaration>();
		switch (classType->GetType())
		{
		case TsysType::Decl:
			if (classDecl->templateSpec && !classDecl->specializationSpec)
			{
				classType->MakePSRecordPrimaryThis();
			}
			break;
		case TsysType::DeclInstant:
			if (!classDecl->specializationSpec)
			{
				classType->MakePSRecordPrimaryThis();
			}
			break;
		default:
			throw L"Unexpected container class type!";
		}
	}
}

Ptr<symbol_component::ClassMemberCache> CreatePartialClassMemberCache(const ParsingArguments& pa, Symbol* classSymbol, Ptr<CppTokenCursor>& cursor)
{
	auto cache = MakePtr<symbol_component::ClassMemberCache>();
	cache->symbolDefinedInsideClass = true;

	FillSymbolToClassMemberCache(pa, classSymbol, cache.Obj());
	cache->parentScope = cache->containerClassTypes[cache->containerClassTypes.Count() - 1]->GetDecl()->GetParentScope();

	FixClassMemberCacheTypes(cache);
	return cache;
}