#include "Parser.h"
#include "Parser_Declaration.h"

using namespace partial_specification_ordering;

void ParseDeclaration_ValueAlias(
	const ParsingArguments& pa,
	Ptr<Symbol> specSymbol,
	Ptr<TemplateSpec> varSpec,
	Ptr<Declarator> declarator,
	FUNCVAR_DECORATORS_FOR_VARIABLE(FUNCVAR_PARAMETER)
	Ptr<CppTokenCursor>& cursor,
	List<Ptr<Declaration>>& output
)
{
	if (declarator->classMemberCache && !declarator->classMemberCache->symbolDefinedInsideClass)
	{
		throw StopParsingException(cursor);
	}
	if (!declarator->initializer || declarator->initializer->initializerType != CppInitializerType::Equal)
	{
		throw StopParsingException(cursor);
	}

	ValidateForRootTemplateSpec(varSpec, cursor, declarator->specializationSpec, false);

	auto decl = MakePtr<ValueAliasDeclaration>();
	decl->templateSpec = varSpec;
	decl->specializationSpec = declarator->specializationSpec;
	decl->name = declarator->name;
	decl->type = declarator->type;
	decl->expr = declarator->initializer->arguments[0].item;
	decl->decoratorConstexpr = decoratorConstexpr;
	decl->needResolveTypeFromInitializer = IsPendingType(declarator->type);
	output.Add(decl);

	if (decl->needResolveTypeFromInitializer)
	{
		auto primitiveType = decl->type.Cast<PrimitiveType>();
		if (!primitiveType) throw StopParsingException(cursor);
		if (primitiveType->primitive != CppPrimitiveType::_auto) throw StopParsingException(cursor);
	}

	auto createdSymbol = pa.scopeSymbol->AddImplDeclToSymbol_NFb(decl, symbol_component::SymbolKind::ValueAlias, specSymbol);
	if (!createdSymbol)
	{
		throw StopParsingException(cursor);
	}

	if (decl->specializationSpec)
	{
		AssignPSPrimary(pa, cursor, createdSymbol);
	}
}