#include "Parser.h"
#include "Parser_Declaration.h"

using namespace partial_specification_ordering;

void ParseDeclaration_Variable(
	const ParsingArguments& pa,
	Ptr<Symbol> specSymbol,
	Ptr<TemplateSpec> spec,
	Ptr<Declarator> declarator,
	FUNCVAR_DECORATORS_FOR_VARIABLE(FUNCVAR_PARAMETER)
	Ptr<CppTokenCursor>& cursor,
	List<Ptr<Declaration>>& output
)
{
	// for variables, names should not be constructor names, destructor names, type conversion operator names, or other operator names
	if (declarator->name.type != CppNameType::Normal)
	{
		throw StopParsingException(cursor);
	}

	if (spec)
	{
		if (!declarator->initializer || declarator->initializer->initializerType != CppInitializerType::Equal)
		{
			throw StopParsingException(cursor);
		}

		ValidateForRootTemplateSpec(spec, cursor, declarator->specializationSpec, false);

		auto decl = MakePtr<ValueAliasDeclaration>();
		decl->templateSpec = spec;
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
	else
	{
#define FILL_VARIABLE\
		decl->name = declarator->name;\
		decl->type = declarator->type;\
		FUNCVAR_DECORATORS_FOR_VARIABLE(FUNCVAR_FILL_DECLARATOR)\
		decl->needResolveTypeFromInitializer = needResolveTypeFromInitializer\

		if (declarator->specializationSpec)
		{
			throw StopParsingException(cursor);
		}

		bool needResolveTypeFromInitializer = IsPendingType(declarator->type);
		if (needResolveTypeFromInitializer && (!declarator->initializer || declarator->initializer->initializerType != CppInitializerType::Equal))
		{
			throw StopParsingException(cursor);
		}

		auto context = declarator->classMemberCache ? declarator->classMemberCache->containerClassTypes[0]->GetDecl() : pa.scopeSymbol;
		if (decoratorExtern || (decoratorStatic && !declarator->initializer))
		{
			// if there is extern, or static without an initializer, then it is a forward variable declaration
			if (declarator->classMemberCache && !declarator->classMemberCache->symbolDefinedInsideClass)
			{
				throw StopParsingException(cursor);
			}

			auto decl = MakePtr<ForwardVariableDeclaration>();
			FILL_VARIABLE;
			output.Add(decl);

			if (!context->AddForwardDeclToSymbol_NFb(decl, symbol_component::SymbolKind::Variable))
			{
				throw StopParsingException(cursor);
			}
		}
		else
		{
			// it is a variable declaration
			auto decl = MakePtr<VariableDeclaration>();
			FILL_VARIABLE;
			decl->initializer = declarator->initializer;
			output.Add(decl);

			if (!context->AddImplDeclToSymbol_NFb(decl, symbol_component::SymbolKind::Variable))
			{
				throw StopParsingException(cursor);
			}
		}
#undef FILL_VARIABLE
	}
}