#include "Parser.h"
#include "Parser_Declaration.h"

/***********************************************************************
IsCStyleTypeReference
***********************************************************************/

bool IsCStyleTypeReference(Ptr<CppTokenCursor>& cursor)
{
	bool cStyleTypeReference = false;
	if (TestToken(cursor, CppTokens::DECL_ENUM, false) || TestToken(cursor, CppTokens::DECL_CLASS, false) || TestToken(cursor, CppTokens::DECL_STRUCT, false) || TestToken(cursor, CppTokens::DECL_UNION, false))
	{
		auto oldCursor = cursor;
		SkipToken(cursor);
		CppName name;
		if (ParseCppName(name, cursor))
		{
			if (!TestToken(cursor, CppTokens::SEMICOLON) && !TestToken(cursor, CppTokens::LT) && !TestToken(cursor, CppTokens::DECL_FINAL) && !TestToken(cursor, CppTokens::COLON) && !TestToken(cursor, CppTokens::LBRACE))
			{
				cStyleTypeReference = true;
			}
		}
		cursor = oldCursor;
	}
	return cStyleTypeReference;
}

/***********************************************************************
EnsureNoTemplateSpec
***********************************************************************/

void EnsureNoTemplateSpec(List<Ptr<TemplateSpec>>& specs, Ptr<CppTokenCursor>& cursor)
{
	if (specs.Count() > 0)
	{
		throw StopParsingException(cursor);
	}
}

/***********************************************************************
EnsureNoMultipleTemplateSpec
***********************************************************************/

Ptr<TemplateSpec> EnsureNoMultipleTemplateSpec(List<Ptr<TemplateSpec>>& specs, Ptr<CppTokenCursor>& cursor)
{
	if (specs.Count() > 1)
	{
		throw StopParsingException(cursor);
	}
	else if (specs.Count() == 1)
	{
		return specs[0];
	}
	else
	{
		return nullptr;
	}
}

/***********************************************************************
ParseVariablesFollowedByDecl_NotConsumeSemicolon
***********************************************************************/

void ParseVariablesFollowedByDecl_NotConsumeSemicolon(const ParsingArguments& pa, Ptr<Declaration> decl, Ptr<CppTokenCursor>& cursor, List<Ptr<Declaration>>& output)
{
	auto type = MakePtr<IdType>();
	type->name = decl->name;
	Resolving::AddSymbol(pa, type->resolving, decl->symbol);

	List<Ptr<TemplateSpec>> specs;
	List<Ptr<Declarator>> declarators;
	ParseNonMemberDeclarator(pa, pda_Typedefs(), type, cursor, declarators);

	for (vint i = 0; i < declarators.Count(); i++)
	{
		ParseDeclaration_Variable(
			pa,
			nullptr,
			specs,
			declarators[i],
			false, false, false, false, false, false, false, false, false,
			cursor,
			output
		);
	}
}

/***********************************************************************
ParseDeclaration_StaticAssert
***********************************************************************/

void ParseDeclaration_StaticAssert(const ParsingArguments& pa, Ptr<CppTokenCursor>& cursor, List<Ptr<Declaration>>& output)
{
	RequireToken(cursor, CppTokens::STATIC_ASSERT);
	RequireToken(cursor, CppTokens::LPARENTHESIS);

	auto decl = MakePtr<StaticAssertDeclaration>();
	output.Add(decl);

	if (!TestToken(cursor, CppTokens::RPARENTHESIS))
	{
		while (true)
		{
			decl->exprs.Add(ParseExpr(pa, pea_Argument(), cursor));
			if (!TestToken(cursor, CppTokens::COMMA))
			{
				RequireToken(cursor, CppTokens::RPARENTHESIS);
				break;
			}
		}
	}

	RequireToken(cursor, CppTokens::SEMICOLON);
}

/***********************************************************************
ParseDeclaration
***********************************************************************/

void ParseDeclaration(const ParsingArguments& pa, Ptr<CppTokenCursor>& cursor, List<Ptr<Declaration>>& output)
{
	while (SkipSpecifiers(cursor));
	if (TestToken(cursor, CppTokens::SEMICOLON))
	{
		// ignore lonely semicolon
		return;
	}
	if (TestToken(cursor, CppTokens::STATIC_ASSERT, false))
	{
		ParseDeclaration_StaticAssert(pa, cursor, output);
		return;
	}

	Ptr<Symbol> specSymbol;
	List<Ptr<TemplateSpec>> specs;
	while(TestToken(cursor, CppTokens::DECL_TEMPLATE, false))
	{
		Ptr<TemplateSpec> spec;
		ParseTemplateSpec(pa, cursor, specSymbol, spec);
		specs.Add(spec);
		while (SkipSpecifiers(cursor));
	}

	bool decoratorFriend = TestToken(cursor, CppTokens::DECL_FRIEND);
	if (decoratorFriend)
	{
		while (SkipSpecifiers(cursor));
		if (specs.Count() == 0)
		{
			// someone will write "friend TYPE_NAME;", just throw away, since this name must have been declared.
			auto oldCursor = cursor;
			if (TestToken(cursor, CppTokens::ID) && TestToken(cursor, CppTokens::SEMICOLON))
			{
				return;
			}
			cursor = oldCursor;
		}

		if (TestToken(cursor, CppTokens::DECL_CLASS, false) || TestToken(cursor, CppTokens::DECL_STRUCT, false) || TestToken(cursor, CppTokens::DECL_UNION, false))
		{
			auto decl = MakePtr<FriendClassDeclaration>();
			decl->templateSpec = EnsureNoMultipleTemplateSpec(specs, cursor);

			switch ((CppTokens)cursor->token.token)
			{
			case CppTokens::DECL_CLASS:
				decl->classType = CppClassType::Class;
				break;
			case CppTokens::DECL_STRUCT:
				decl->classType = CppClassType::Struct;
				break;
			case CppTokens::DECL_UNION:
				decl->classType = CppClassType::Union;
				break;
			default:
				throw StopParsingException(cursor);
			}
			SkipToken(cursor);

			// friend class could not have partial specialization, so any symbols in template<...> will not be used
			decl->usedClass = ParseShortType(pa, ShortTypeTypenameKind::Implicit, cursor);
			RequireToken(cursor, CppTokens::SEMICOLON);
			output.Add(decl);
			return;
		}
	}
	bool cStyleTypeReference = IsCStyleTypeReference(cursor);

	// check if the following `struct Id` is trying to reference a type instead of to define a type

	{
		auto oldCursor = cursor;
		if (TestToken(cursor, CppTokens::DECL_EXTERN) && TestToken(cursor, CppTokens::STRING))
		{
			// extern "C"
			// ignore it and add everything to its parent
			if (TestToken(cursor, CppTokens::LBRACE))
			{
				while (!TestToken(cursor, CppTokens::RBRACE))
				{
					ParseDeclaration(pa, cursor, output);
				}
			}
			else
			{
				ParseDeclaration(pa, cursor, output);
			}

			// prevent from stack overflowing in CppTokenCursor's destructor
			while (oldCursor != cursor)
			{
				oldCursor = oldCursor->Next();
			}
			return;
		}
		else
		{
			cursor = oldCursor;
		}
	}

	if (TestToken(cursor, CppTokens::DECL_NAMESPACE, false))
	{
		EnsureNoTemplateSpec(specs, cursor);
		ParseDeclaration_Namespace(pa, cursor, output);
	}
	else if (!cStyleTypeReference && TestToken(cursor, CppTokens::DECL_ENUM, false))
	{
		EnsureNoTemplateSpec(specs, cursor);
		if (auto enumDecl = ParseDeclaration_Enum_NotConsumeSemicolon(pa, false, cursor, output))
		{
			if (!TestToken(cursor, CppTokens::SEMICOLON, false))
			{
				ParseVariablesFollowedByDecl_NotConsumeSemicolon(pa, enumDecl, cursor, output);
			}
		}
		RequireToken(cursor, CppTokens::SEMICOLON);
	}
	else if (!cStyleTypeReference && (TestToken(cursor, CppTokens::DECL_CLASS, false) || TestToken(cursor, CppTokens::DECL_STRUCT, false) || TestToken(cursor, CppTokens::DECL_UNION, false)))
	{
		auto spec = EnsureNoMultipleTemplateSpec(specs, cursor);
		if (auto classDecl = ParseDeclaration_Class_NotConsumeSemicolon(pa, specSymbol, spec, false, cursor, output))
		{
			if (!TestToken(cursor, CppTokens::SEMICOLON, false))
			{
				ParseVariablesFollowedByDecl_NotConsumeSemicolon(pa, classDecl, cursor, output);
			}
		}
		RequireToken(cursor, CppTokens::SEMICOLON);
	}
	else if (TestToken(cursor, CppTokens::DECL_USING, false))
	{
		auto spec = EnsureNoMultipleTemplateSpec(specs, cursor);
		ParseDeclaration_Using(pa, specSymbol, spec, cursor, output);
	}
	else if(TestToken(cursor,CppTokens::DECL_TYPEDEF, false))
	{
		EnsureNoTemplateSpec(specs, cursor);
		ParseDeclaration_Typedef(pa, cursor, output);
	}
	else
	{
		ParseDeclaration_FuncVar(pa, specSymbol, specs, decoratorFriend, cursor, output);
	}
}

/***********************************************************************
BuildVariables
***********************************************************************/

void BuildVariables(List<Ptr<Declarator>>& declarators, List<Ptr<VariableDeclaration>>& varDecls)
{
	for (vint i = 0; i < declarators.Count(); i++)
	{
		auto declarator = declarators[i];

		auto varDecl = MakePtr<VariableDeclaration>();
		varDecl->type = declarator->type;
		varDecl->name = declarator->name;
		varDecl->initializer = declarator->initializer;
		varDecl->needResolveTypeFromInitializer = IsPendingType(varDecl->type);
		varDecls.Add(varDecl);
	}
}

/***********************************************************************
BuildSymbols
***********************************************************************/

void BuildSymbol(const ParsingArguments& pa, Ptr<VariableDeclaration> varDecl, bool isVariadic, Ptr<CppTokenCursor>& cursor)
{
	if (varDecl->name)
	{
		auto symbol = pa.scopeSymbol->AddImplDeclToSymbol_NFb(varDecl, symbol_component::SymbolKind::Variable, nullptr);
		if (!symbol)
		{
			throw StopParsingException(cursor);
		}
		symbol->ellipsis = isVariadic;
	}
}

void BuildSymbols(const ParsingArguments& pa, List<Ptr<VariableDeclaration>>& varDecls, Ptr<CppTokenCursor>& cursor)
{
	for (vint i = 0; i < varDecls.Count(); i++)
	{
		BuildSymbol(pa, varDecls[i], false, cursor);
	}
}

void BuildSymbols(const ParsingArguments& pa, VariadicList<Ptr<VariableDeclaration>>& varDecls, Ptr<CppTokenCursor>& cursor)
{
	for (vint i = 0; i < varDecls.Count(); i++)
	{
		BuildSymbol(pa, varDecls[i].item, varDecls[i].isVariadic, cursor);
	}
}

/***********************************************************************
BuildVariablesAndSymbols
***********************************************************************/

void BuildVariablesAndSymbols(const ParsingArguments& pa, List<Ptr<Declarator>>& declarators, List<Ptr<VariableDeclaration>>& varDecls, Ptr<CppTokenCursor>& cursor)
{
	BuildVariables(declarators, varDecls);
	BuildSymbols(pa, varDecls, cursor);
}

/***********************************************************************
BuildVariableAndSymbol
***********************************************************************/

Ptr<VariableDeclaration> BuildVariableAndSymbol(const ParsingArguments& pa, Ptr<Declarator> declarator, Ptr<CppTokenCursor>& cursor)
{
	List<Ptr<Declarator>> declarators;
	declarators.Add(declarator);

	List<Ptr<VariableDeclaration>> varDecls;
	BuildVariablesAndSymbols(pa, declarators, varDecls, cursor);
	return varDecls[0];
}