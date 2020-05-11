#include "Parser.h"
#include "Parser_Declarator.h"
#include "Ast_Decl.h"
#include "Ast_Type.h"
#include "Ast_Expr.h"
#include "Symbol_Resolve.h"
#include "Symbol_TemplateSpec.h"
#include "EvaluateSymbol.h"

////////////////////////////////////////////////////////////////////////////////////////////////

bool IsInTemplateHeader(const ParsingArguments& pa)
{
	return pa.scopeSymbol->kind == symbol_component::SymbolKind::Root && pa.scopeSymbol->GetParentScope();
}

////////////////////////////////////////////////////////////////////////////////////////////////

struct QualifiedIdComponent
{
	Ptr<IdType>			idType;
	Ptr<ChildType>		childType;
	Ptr<GenericType>	genericType;
};

Symbol* BreakTypeIntoQualifiedIdComponent(const ParsingArguments& pa, Ptr<symbol_component::ClassMemberCache> cache, Ptr<Type> classType, List<QualifiedIdComponent>& qics, Ptr<CppTokenCursor>& cursor)
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
				return childType->resolving->items[0].symbol;
			}
		}
		else if (auto idType = currentClassType.Cast<IdType>())
		{
			if (Resolving::IsResolvedToType(idType->resolving))
			{
				qic.idType = idType;
				qics.Add(qic);

				return cache->parentScope;
			}
			else
			{
				return idType->resolving->items[0].symbol;
			}
			break;
		}
		else if (currentClassType.Cast<RootType>())
		{
			return pa.root.Obj();
		}
		else
		{
			throw StopParsingException(cursor);
		}
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////

bool MatchPrimaryTemplate(
	const ParsingArguments& pa,
	Ptr<TemplateSpec> thisSpec,
	Ptr<TemplateSpec> classSpec,
	Ptr<GenericType> genericType
)
{
	// the template header used in the member must be compatible with one in the class
	Dictionary<WString, WString> equivalentNames;
	if (!IsCompatibleTemplateSpec(thisSpec, classSpec, equivalentNames))
	{
		return false;
	}

	// template arguments for this class must be compatible with the template header
	if (genericType->arguments.Count() != thisSpec->arguments.Count())
	{
		return false;
	}
	for (vint i = 0; i < genericType->arguments.Count(); i++)
	{
		auto& gArg = genericType->arguments[i];
		auto& tArg = thisSpec->arguments[i];

		if (gArg.isVariadic != tArg.ellipsis)
		{
			return false;
		}

		switch (tArg.argumentType)
		{
		case CppTemplateArgumentType::HighLevelType:
		case CppTemplateArgumentType::Type:
			if (auto idType = gArg.item.type.Cast<IdType>())
			{
				if (idType->name.name != tArg.name.name)
				{
					return false;
				}
			}
			else
			{
				return false;
			}
			break;
		case CppTemplateArgumentType::Value:
			if (auto idExpr = gArg.item.expr.Cast<IdExpr>())
			{
				if (idExpr->name.name != tArg.name.name)
				{
					return false;
				}
			}
			else
			{
				return false;
			}
			break;
		}
	}

	return true;
}

bool MatchPSTemplate(
	const ParsingArguments& pa,
	Ptr<TemplateSpec> thisSpec,
	Ptr<TemplateSpec> classSpec,
	Ptr<GenericType> genericType,
	Ptr<SpecializationSpec> psSpec
)
{
	// the template header used in the member must be compatible with one in the class
	Dictionary<WString, WString> equivalentNames;
	if (!IsCompatibleTemplateSpec(thisSpec, classSpec, equivalentNames))
	{
		return false;
	}

	// template arguments for this class must be compatible with the specialization spec
	if (genericType->arguments.Count() != psSpec->arguments.Count())
	{
		return false;
	}
	for (vint i = 0; i < genericType->arguments.Count(); i++)
	{
		auto& gArg = genericType->arguments[i];
		auto& sArg = psSpec->arguments[i];

		if (gArg.isVariadic != sArg.isVariadic)
		{
			return false;
		}

		if (gArg.item.type && sArg.item.type)
		{
			if (!IsSameResolvedType(gArg.item.type, sArg.item.type, equivalentNames))
			{
				return false;
			}
		}
		else if (gArg.item.expr && sArg.item.expr)
		{
			if (!IsSameResolvedExpr(gArg.item.expr, sArg.item.expr, equivalentNames))
			{
				return false;
			}
		}
		else
		{
			return false;
		}
	}

	return true;
}

void PrepareTemplateArgumentContext(
	const ParsingArguments& pa,
	Ptr<TemplateSpec> thisSpec,
	Ptr<TemplateSpec> classSpec,
	TemplateArgumentContext& taContext
)
{
	for (vint i = 0; i < thisSpec->arguments.Count(); i++)
	{
		auto& tArg = thisSpec->arguments[i];
		auto& cArg = classSpec->arguments[i];
		auto cKey = symbol_type_resolving::GetTemplateArgumentKey(cArg.argumentSymbol, pa.tsys.Obj());

		switch (tArg.argumentType)
		{
		case CppTemplateArgumentType::HighLevelType:
		case CppTemplateArgumentType::Type:
			if (tArg.ellipsis)
			{
				taContext.arguments.Add(cKey, pa.tsys->Any());
			}
			else
			{
				auto tValue = symbol_type_resolving::GetTemplateArgumentKey(tArg.argumentSymbol, pa.tsys.Obj());
				taContext.arguments.Add(cKey, tValue);
			}
			break;
		case CppTemplateArgumentType::Value:
			if (tArg.ellipsis)
			{
				taContext.arguments.Add(cKey, pa.tsys->Any());
			}
			else
			{
				taContext.arguments.Add(cKey, nullptr);
			}
			break;
		}
	}
}

ITsys* StepIntoTemplateClass(
	const ParsingArguments& pa,
	Ptr<symbol_component::ClassMemberCache> cache,
	const QualifiedIdComponent& qic,
	ClassDeclaration* primaryClassDecl,
	List<Ptr<TemplateSpec>>* specs,
	vint& consumedSpecs,
	ITsys*& currentParentDeclType,
	Ptr<CppTokenCursor>& cursor
)
{
	// for template class
	// it must be used with template arguments
	// and a template header must be available
	if (!qic.genericType)
	{
		throw StopParsingException(cursor);
	}
	if (!specs)
	{
		throw StopParsingException(cursor);
	}
	if (consumedSpecs == (*specs).Count())
	{
		throw StopParsingException(cursor);
	}

	auto thisSpec = (*specs)[consumedSpecs++];
	ITsys* nextClass = nullptr;

	auto primaryClassSpec = primaryClassDecl->templateSpec;
	if (MatchPrimaryTemplate(pa, thisSpec, primaryClassSpec, qic.genericType))
	{
		// prepare TemplateArgumentContext and create the type
		TemplateArgumentContext taContext;
		taContext.symbolToApply = primaryClassDecl->symbol;
		PrepareTemplateArgumentContext(pa, thisSpec, primaryClassSpec, taContext);

		// evaluate the type
		auto& classTsys = symbol_type_resolving::EvaluateForwardClassSymbol(pa, primaryClassDecl, currentParentDeclType, &taContext);
		nextClass = classTsys[0];

		// even when the class has no partial specialization, it could be added later
		// so it needs to call nextClass->MakePSRecordPrimaryThis
		nextClass->MakePSRecordPrimaryThis();

		// the class is an instance of a template class
		// types of its child classes depent on this type
		currentParentDeclType = nextClass;
	}
	else
	{
		throw StopParsingException(cursor);
	}

	cache->containerClassTypes.Insert(0, nextClass);
	cache->containerClassSpecs.Insert(0, thisSpec);
	return nextClass;
}

////////////////////////////////////////////////////////////////////////////////////////////////

ITsys* StepIntoNonTemplateClass(
	const ParsingArguments& pa,
	Ptr<symbol_component::ClassMemberCache> cache,
	ClassDeclaration* classDecl,
	List<Ptr<TemplateSpec>>* specs,
	ITsys* currentParentDeclType
)
{
	// for non-template class
	// since it is resolved by name, it could not be an instance of a partial specialization
	// no need to call nextClass->MakePSRecordPrimaryThis
	auto& classTsys = symbol_type_resolving::EvaluateForwardClassSymbol(pa, classDecl, currentParentDeclType, nullptr);
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

	return nextClass;
}

////////////////////////////////////////////////////////////////////////////////////////////////

Ptr<symbol_component::ClassMemberCache> CreatePartialClassMemberCache(const ParsingArguments& pa, Ptr<Type> classType, List<Ptr<TemplateSpec>>* specs, Ptr<CppTokenCursor>& cursor)
{
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

	List<QualifiedIdComponent> qics;
	ITsys* currentClass = nullptr;
	ITsys* currentParentDeclType = nullptr;
	vint consumedSpecs = 0;

	auto currentNamespace = BreakTypeIntoQualifiedIdComponent(pa, cache, classType, qics, cursor);

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
				currentClass = StepIntoTemplateClass(pa, cache, qic, classDecl.Obj(), specs, consumedSpecs, currentParentDeclType, cursor);
			}
			else
			{
				currentClass = StepIntoNonTemplateClass(pa, cache, classDecl.Obj(), specs, currentParentDeclType);
			}
			currentNamespace = nullptr;
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
	return cache;
}