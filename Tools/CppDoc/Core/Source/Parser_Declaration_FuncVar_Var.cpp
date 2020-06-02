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
		if (!declarator->initializer)
		{
			throw StopParsingException(cursor);
		}
		else
		{
			switch (declarator->initializer->initializerType)
			{
			case CppInitializerType::Constructor:
			case CppInitializerType::Universal:
				if (declarator->initializer->arguments.Count() != 1)
				{
					throw StopParsingException(cursor);
				}
				else
				{
					declarator->initializer->initializerType = CppInitializerType::Equal;
				}
				break;
			}
		}
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
		decl->templateScope = specSymbol.Obj();
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

		auto createdSymbol = context->AddImplDeclToSymbol_NFb(decl, symbol_component::SymbolKind::Variable, specSymbol, declarator->classMemberCache);
		if (!createdSymbol)
		{
			// there could be a declaration that has been declared in the class
			// ensure that there is no initializer, so that declarator->classMemberCache is not necessary
			if (decl->initializer)
			{
				throw StopParsingException(cursor);
			}

			decl->templateScope = specSymbol.Obj();
			createdSymbol = context->AddForwardDeclToSymbol_NFb(decl, symbol_component::SymbolKind::Variable);

			if (!createdSymbol)
			{
				throw StopParsingException(cursor);
			}
		}

		for (vint i = 0; i < decl->classSpecs.Count(); i++)
		{
			decl->classSpecs[i]->AssignDeclSymbol(createdSymbol);
		}
	}
#undef FILL_VARIABLE
}