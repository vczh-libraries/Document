#include "Parser.h"
#include "Parser_Declaration.h"
#include "Parser_Declarator.h"

void ParseDeclaration_Variable(
	const ParsingArguments& pa,
	Ptr<Symbol> specSymbol,
	List<Ptr<TemplateSpec>>& classSpecs,
	Ptr<Declarator> declarator,
	FUNCVAR_DECORATORS_FOR_VARIABLE(FUNCVAR_PARAMETER)
	Ptr<CppTokenCursor>& cursor,
	List<Ptr<Declaration>>& output
)
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

	bool isForwardDecl = false;
	if (!declarator->classMemberCache || declarator->classMemberCache->symbolDefinedInsideClass)
	{
		if (decoratorExtern)
		{
			isForwardDecl = true;
		}
		else if (decoratorStatic && !declarator->initializer)
		{
			isForwardDecl = true;
		}
	}

	auto context = declarator->classMemberCache ? declarator->classMemberCache->containerClassTypes[0]->GetDecl() : pa.scopeSymbol;
	if (isForwardDecl)
	{
		// if there is extern, or static without an initializer, then it is a forward variable declaration
		if (declarator->classMemberCache && !declarator->classMemberCache->symbolDefinedInsideClass)
		{
			throw StopParsingException(cursor);
		}

		auto decl = MakePtr<ForwardVariableDeclaration>();
		FILL_VARIABLE;
		output.Add(decl);

		auto createdSymbol = context->AddForwardDeclToSymbol_NFb(decl, symbol_component::SymbolKind::Variable);
		if (!createdSymbol)
		{
			throw StopParsingException(cursor);
		}
	}
	else
	{
		// it is a variable declaration
		auto decl = MakePtr<VariableDeclaration>();
		CopyFrom(decl->classSpecs, classSpecs);
		FILL_VARIABLE;
		decl->initializer = declarator->initializer;
		output.Add(decl);

		auto createdSymbol = context->AddImplDeclToSymbol_NFb(decl, symbol_component::SymbolKind::Variable, nullptr, declarator->classMemberCache);
		if (!createdSymbol)
		{
			throw StopParsingException(cursor);
		}

		for (vint i = 0; i < decl->classSpecs.Count(); i++)
		{
			decl->classSpecs[i]->AssignDeclSymbol(createdSymbol);
		}
	}
#undef FILL_VARIABLE
}