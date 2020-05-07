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

Ptr<TemplateSpec> AssignContainerClassDeclsToSpecs(
	List<Ptr<TemplateSpec>>& specs,
	Ptr<Declarator> declarator,
	List<Ptr<TemplateSpec>>& containerClassSpecs,
	List<ClassDeclaration*>& containerClassDecls,
	Ptr<CppTokenCursor>& cursor
)
{
	if (declarator->classMemberCache)
	{
		for (vint i = 0; i < declarator->classMemberCache->containerClassTypes.Count(); i++)
		{
			auto classType = declarator->classMemberCache->containerClassTypes[i];
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

	if (declarator->classMemberCache && !declarator->classMemberCache->symbolDefinedInsideClass)
	{
		vint used = 0;
		auto& thisTypes = declarator->classMemberCache->containerClassTypes;
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
				containerClassSpecs.Add(thisSpec);
				containerClassDecls.Add(thisDecl.Obj());
			}
		}

		switch (specs.Count() - used)
		{
		case 0:
			return nullptr;
		case 1:
			return specs[used];
		default:
			throw StopParsingException(cursor);
		}
	}
	else
	{
		return EnsureNoMultipleTemplateSpec(specs, cursor);
	}
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
	List<Ptr<TemplateSpec>>& specs,
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

	// extract multiple levels of container classes
	List<Ptr<TemplateSpec>> containerClassSpecs;
	List<ClassDeclaration*> containerClassDecls;
	auto functionSpec = AssignContainerClassDeclsToSpecs(specs, declarator, containerClassSpecs, containerClassDecls, cursor);

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
		for (vint i = 0; i < containerClassSpecs.Count(); i++)
		{
			decl->classSpecs.Add({ containerClassSpecs[i],containerClassDecls[i] });
		}

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

/////////////////////////////////////////////////////////////////////////////////////////////

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

/////////////////////////////////////////////////////////////////////////////////////////////

void ParseDeclaration_FuncVar(const ParsingArguments& pa, Ptr<Symbol> specSymbol, List<Ptr<TemplateSpec>>& specs, bool decoratorFriend, Ptr<CppTokenCursor>& cursor, List<Ptr<Declaration>>& output)
{
	// parse declarators for functions and variables

#define DEFINE_FUNCVAR_BOOL(TOKEN, NAME) bool decorator##NAME = false;
	FUNCVAR_DECORATORS(DEFINE_FUNCVAR_BOOL)
#undef DEFINE_FUNCVAR_BOOL

	while (cursor)
	{
#define DEFINE_FUNCVAR_TEST(TOKEN, NAME) if (TestToken(cursor, CppTokens::TOKEN)) { decorator##NAME = true; while(SkipSpecifiers(cursor)); } else
		FUNCVAR_DECORATORS(DEFINE_FUNCVAR_TEST)
#undef DEFINE_FUNCVAR_TEST
		break;
	}

	// prepare data structures for class members defined out of classes
	// non-null containingClass means this declaration is defined right inside a class
	// non-null containingClassForMember means this declaration is a class member defined out of the class
	List<Ptr<Declarator>> declarators;
	auto methodType = CppMethodType::Function;
	ClassDeclaration* containingClass = pa.scopeSymbol->GetImplDecl_NFb<ClassDeclaration>().Obj();

	// get all declarators
	{
		auto pda = pda_Decls(pa.scopeSymbol->GetImplDecl_NFb<ClassDeclaration>(), specs.Count() > 0);
		pda.containingClass = containingClass;
		pda.scopeSymbolToReuse = specSymbol;

		auto newPa = specSymbol ? pa.WithScope(specSymbol.Obj()) : pa;
		ParseMemberDeclarator(newPa, pda, cursor, declarators);
	}

	Ptr<FunctionType> funcType;
	if (declarators.Count() > 0)
	{
		// a function declaration can only have one declarator
		auto declarator = declarators[0];
		funcType = GetTypeWithoutMemberAndCC(declarator->type).Cast<FunctionType>();
		if (funcType)
		{
			if (declarators.Count() != 1)
			{
				throw StopParsingException(cursor);
			}
		}

		// see if it is a class member declarator defined out of the class
		if (declarator->type.Cast<MemberType>())
		{
			if (!declarator->classMemberCache || declarator->classMemberCache->symbolDefinedInsideClass)
			{
				throw StopParsingException(cursor);
			}
		}

		// adjust name type
		if (declarator->classMemberCache)
		{
			auto& cppName = declarators[0]->name;
			switch (cppName.type)
			{
			case CppNameType::Operator:
				if (cppName.tokenCount == 1)
				{
					methodType = CppMethodType::TypeConversion;
				}
				break;
			case CppNameType::Constructor:
				methodType = CppMethodType::Constructor;
				break;
			case CppNameType::Destructor:
				methodType = CppMethodType::Destructor;
				break;
			}
		}
	}

	if (funcType)
	{
		// for functions
		bool decoratorAbstract = false;
		bool decoratorDefault = false;
		bool decoratorDelete = false;

		if (TestToken(cursor, CppTokens::EQ))
		{
			if (TestToken(cursor, CppTokens::STAT_DEFAULT))
			{
				decoratorDefault = true;
			}
			else if (TestToken(cursor, CppTokens::DELETE))
			{
				decoratorDelete = true;
			}
			else
			{
				RequireToken(cursor, L"0");
				decoratorAbstract = true;
			}
		}

		ParseDeclaration_Function(
			pa,
			declarators[0]->scopeSymbolToReuse,
			specs,
			declarators[0],
			funcType,
			FUNCVAR_DECORATORS_FOR_FUNCTION(FUNCVAR_ARGUMENT)
			methodType,
			cursor,
			output
		);
	}
	else
	{
		auto spec = EnsureNoMultipleTemplateSpec(specs, cursor);
		if (spec && declarators.Count() > 1)
		{
			throw StopParsingException(cursor);
		}
		for (vint i = 0; i < declarators.Count(); i++)
		{
			ParseDeclaration_Variable(
				pa,
				declarators[i]->scopeSymbolToReuse,
				spec,
				declarators[i],
				FUNCVAR_DECORATORS_FOR_VARIABLE(FUNCVAR_ARGUMENT)
				cursor,
				output
			);
		}
		RequireToken(cursor, CppTokens::SEMICOLON);
	}
}