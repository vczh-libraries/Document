#include "Parser.h"
#include "Parser_Declaration.h"

void ParseDeclaration_Typedef(const ParsingArguments& pa, Ptr<CppTokenCursor>& cursor, List<Ptr<Declaration>>& output)
{
	RequireToken(cursor, CppTokens::DECL_TYPEDEF);
	List<Ptr<Declarator>> declarators;
	Ptr<IdType> createdType;

	bool cStyleTypeReference = IsCStyleTypeReference(cursor);
	if (!cStyleTypeReference && (TestToken(cursor, CppTokens::DECL_CLASS, false) || TestToken(cursor, CppTokens::DECL_STRUCT, false) || TestToken(cursor, CppTokens::DECL_UNION, false)))
	{
		// typedef class{} ...;
		auto classDecl = ParseDeclaration_Class_NotConsumeSemicolon(pa, nullptr, nullptr, true, cursor, output);

		createdType = MakePtr<IdType>();
		createdType->name = classDecl->name;
		Resolving::AddSymbol(pa, createdType->resolving, classDecl->symbol);
		ParseNonMemberDeclarator(pa, pda_Typedefs(), createdType, cursor, declarators);
	}
	else if (!cStyleTypeReference && TestToken(cursor, CppTokens::DECL_ENUM, false))
	{
		// typedef enum{} ...;
		auto enumDecl = ParseDeclaration_Enum_NotConsumeSemicolon(pa, true, cursor, output);

		createdType = MakePtr<IdType>();
		createdType->name = enumDecl->name;
		Resolving::AddSymbol(pa, createdType->resolving, enumDecl->symbol);
		ParseNonMemberDeclarator(pa, pda_Typedefs(), createdType, cursor, declarators);
	}
	else
	{
		// typedef ...;
		ParseNonMemberDeclarator(pa, pda_Typedefs(), cursor, declarators);
	}

	RequireToken(cursor, CppTokens::SEMICOLON);
	for (vint i = 0; i < declarators.Count(); i++)
	{
		if (declarators[i]->type == createdType && declarators[i]->name.name == createdType->name.name)
		{
			continue;
		}
		auto decl = MakePtr<TypeAliasDeclaration>();
		decl->name = declarators[i]->name;
		decl->type = declarators[i]->type;
		output.Add(decl);

		if (!pa.scopeSymbol->AddImplDeclToSymbol_NFb(decl, symbol_component::SymbolKind::TypeAlias))
		{
			throw StopParsingException(cursor);
		}
	}
}