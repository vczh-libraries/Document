#include "Parser.h"
#include "Parser_Declaration.h"

using namespace partial_specification_ordering;

/////////////////////////////////////////////////////////////////////////////////////////////

Symbol* SearchForFunctionWithSameSignature(Symbol* context, Ptr<ForwardFunctionDeclaration> decl, const List<Ptr<Symbol>>& candidates, Ptr<CppTokenCursor>& cursor)
{
	for (vint i = 0; i < candidates.Count(); i++)
	{
		auto symbol = candidates[i].Obj();
		switch (symbol->kind)
		{
		case symbol_component::SymbolKind::FunctionSymbol:
			{
				auto declToCompare = symbol->GetAnyForwardDecl<ForwardFunctionDeclaration>();
				if (IsCompatibleFunctionDeclInSameScope(decl, declToCompare))
				{
					return symbol;
				}
			}
			break;
		case CSTYLE_TYPE_SYMBOL_KIND:
			// function can only override enum/class/struct/union
			break;
		default:
			throw StopParsingException(cursor);
		}
	}
	return nullptr;
}

Symbol* SearchForFunctionWithSameSignature(Symbol* context, Ptr<ForwardFunctionDeclaration> decl, Ptr<CppTokenCursor>& cursor)
{
	if (!decl->needResolveTypeFromStatement)
	{
		if (decl->specializationSpec)
		{
			// sync with Symbol::DecorateNameForSpecializationSpec
			// AssignPSPrimary is not called (because the return value is unknown)
			// so GetPSPrimaryDescendants_NF will never get the expected result

			auto prefix = decl->name.name + L"@<";
			auto& children = context->GetChildren_NFb();
			for (vint i = 0; i < children.Count(); i++)
			{
				auto key = children.Keys()[i];
				if (key.Length() > prefix.Length() && key.Left(prefix.Length()) == prefix)
				{
					auto& candidates = children.GetByIndex(i);
					if (auto result = SearchForFunctionWithSameSignature(context, decl, candidates, cursor))
					{
						return result;
					}
				}
			}
		}
		else
		{
			if (auto pSymbols = context->TryGetChildren_NFb(decl->name.name))
			{
				if (auto result = SearchForFunctionWithSameSignature(context, decl, *pSymbols, cursor))
				{
					return result;
				}
			}
		}
	}
	return context->CreateFunctionSymbol_NFb(decl);
}

/////////////////////////////////////////////////////////////////////////////////////////////

void BuildFunctionParameters(
	const ParsingArguments& pa,
	Ptr<FunctionType> funcType,
	Symbol* functionBodySymbol,
	Ptr<CppTokenCursor>& cursor
)
{
	if (funcType->parameters.Count() > 0)
	{
		for (vint i = 0; i < funcType->parameters.Count(); i++)
		{
			auto parameter = funcType->parameters[i];
			if (parameter.item->name)
			{
				if (functionBodySymbol->TryGetChildren_NFb(parameter.item->name.name))
				{
					goto SKIP_BUILDING_SYMBOLS;
				}
			}
		}
		BuildSymbols(pa, funcType->parameters, cursor);
	SKIP_BUILDING_SYMBOLS:;
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////

void DelayParseFunctionStatement(
	const ParsingArguments& pa,
	Ptr<FunctionDeclaration> decl,
	Ptr<CppTokenCursor>& cursor
)
{
	decl->delayParse = MakePtr<DelayParse>();
	decl->delayParse->pa = pa;
	cursor->Clone(decl->delayParse->reader, decl->delayParse->begin);

	vint counter = 0;
	while (true)
	{
		if (TestToken(cursor, CppTokens::LBRACE))
		{
			counter++;
		}
		else if (TestToken(cursor, CppTokens::RBRACE))
		{
			counter--;
			if (counter == 0)
			{
				if (cursor)
				{
					decl->delayParse->end = cursor->token;
				}
				else
				{
					memset(&decl->delayParse->end, 0, sizeof(RegexToken));
				}
				break;
			}
		}
		else
		{
			SkipToken(cursor);
			if (!cursor)
			{
				throw StopParsingException(cursor);
			}
		}
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////

void ParseDeclaration_Function(
	const ParsingArguments& pa,
	Ptr<Symbol> specSymbol,
	List<ClassSpec>& classSpecs,
	Ptr<TemplateSpec> functionSpec,
	Ptr<Declarator> declarator,
	Ptr<FunctionType> funcType,
	FUNCVAR_DECORATORS_FOR_FUNCTION(FUNCVAR_PARAMETER)
	CppMethodType methodType,
	Ptr<CppTokenCursor>& cursor,
	List<Ptr<Declaration>>& output
)
{
#define FILL_FUNCTION\
	decl->name = declarator->name;\
	decl->type = declarator->type;\
	decl->methodType = methodType;\
	FUNCVAR_DECORATORS_FOR_FUNCTION(FUNCVAR_FILL_DECLARATOR)\
	decl->needResolveTypeFromStatement = needResolveTypeFromStatement\

	bool hasStat = TestToken(cursor, CppTokens::COLON, false) || TestToken(cursor, CppTokens::LBRACE, false);
	bool needResolveTypeFromStatement = IsPendingType(funcType->returnType) && (!funcType->decoratorReturnType || IsPendingType(funcType->decoratorReturnType));
	if (needResolveTypeFromStatement && !hasStat)
	{
		throw StopParsingException(cursor);
	}

	// check for constraints of partial specialization
	if (declarator->specializationSpec)
	{
		if (!functionSpec) throw StopParsingException(cursor);
		if (functionSpec->arguments.Count() != 0) throw StopParsingException(cursor);
	}
	ValidateForRootTemplateSpec(functionSpec, cursor, declarator->specializationSpec, true);

	auto context = declarator->classMemberCache ? declarator->classMemberCache->containerClassTypes[0]->GetDecl() : pa.scopeSymbol;
	if (hasStat)
	{
		// if there is a statement, then it is a function declaration
		auto decl = MakePtr<FunctionDeclaration>();
		decl->templateSpec = functionSpec;
		decl->specializationSpec = declarator->specializationSpec;
		CopyFrom(decl->classSpecs, classSpecs);

		FILL_FUNCTION;
		output.Add(decl);

		// adjust classMemberCache
		if (declarator->classMemberCache)
		{
			TsysCV cv;
			cv.isGeneralConst = funcType->qualifierConstExpr || funcType->qualifierConst;
			cv.isVolatile = funcType->qualifierVolatile;

			auto classType = declarator->classMemberCache->containerClassTypes[0];
			declarator->classMemberCache->thisType = classType->CVOf(cv)->PtrOf();

			if (!declarator->classMemberCache->symbolDefinedInsideClass)
			{
				declarator->classMemberCache->parentScope = pa.scopeSymbol;
			}
		}

		// match declaration and implementation
		auto functionSymbol = SearchForFunctionWithSameSignature(context, decl, cursor);
		auto functionBodySymbol = functionSymbol->CreateFunctionImplSymbol_F(decl, specSymbol, declarator->classMemberCache);

		// find the primary symbol for partial specialization
		if (decl->specializationSpec)
		{
			AssignPSPrimary(pa, cursor, functionSymbol);
		}

		// remove cyclic referencing
		GetTypeWithoutMemberAndCC(decl->type).Cast<FunctionType>()->scopeSymbolToReuse = nullptr;

		auto newPa = pa.WithScope(functionBodySymbol);

		// variable symbols could have been created during parsing
		BuildFunctionParameters(newPa, funcType, functionBodySymbol, cursor);

		// delay parse the statement
		DelayParseFunctionStatement(newPa, decl, cursor);
	}
	else
	{
		// if there is ;, then it is a forward function declaration
		if (declarator->classMemberCache && !declarator->classMemberCache->symbolDefinedInsideClass)
		{
			throw StopParsingException(cursor);
		}

		auto decl = MakePtr<ForwardFunctionDeclaration>();
		decl->templateSpec = functionSpec;
		decl->specializationSpec = declarator->specializationSpec;
		FILL_FUNCTION;
		output.Add(decl);
		RequireToken(cursor, CppTokens::SEMICOLON);

		// match declaration and implementation
		auto functionSymbol = SearchForFunctionWithSameSignature(context, decl, cursor);
		functionSymbol->CreateFunctionForwardSymbol_F(decl, specSymbol);

		// find the primary symbol for partial specialization
		if (decl->specializationSpec)
		{
			AssignPSPrimary(pa, cursor, functionSymbol);
		}

		// remove cyclic referencing
		GetTypeWithoutMemberAndCC(decl->type).Cast<FunctionType>()->scopeSymbolToReuse = nullptr;
	}
#undef FILL_FUNCTION
}