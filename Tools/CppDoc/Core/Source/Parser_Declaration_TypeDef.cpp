#include "Parser.h"
#include "Parser_Declaration.h"
#include "Parser_Declarator.h"

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

		createdType = Ptr(new IdType);
		createdType->name = classDecl->name;
		Resolving::AddSymbol(pa, createdType->resolving, classDecl->symbol);
		ParseNonMemberDeclarator(pa, pda_Typedefs(), createdType, cursor, declarators);
	}
	else if (!cStyleTypeReference && TestToken(cursor, CppTokens::DECL_ENUM, false))
	{
		// typedef enum{} ...;
		auto enumDecl = ParseDeclaration_Enum_NotConsumeSemicolon(pa, true, cursor, output);

		createdType = Ptr(new IdType);
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
		auto decl = Ptr(new TypeAliasDeclaration);
		decl->name = declarators[i]->name;
		decl->type = declarators[i]->type;
		output.Add(decl);

		if (!pa.scopeSymbol->AddImplDeclToSymbol_NFb(decl, symbol_component::SymbolKind::TypeAlias))
		{
			bool ignorable = true;
			if (auto pSymbols = pa.scopeSymbol->TryGetChildren_NFb(decl->name.name))
			{
				for (vint j = 0; j < pSymbols->Count(); j++)
				{
					auto childSymbol = pSymbols->Get(j);
					if (!childSymbol.childExpr && !childSymbol.childType)
					{
						switch (childSymbol.childSymbol->kind)
						{
						case symbol_component::SymbolKind::TypeAlias:
							// found a duplicated typedef, ignore this one
							ignorable = true;
							break;
						case CLASS_SYMBOL_KIND:
							// found a class with the same name, ignore this one
							ignorable = true;
							break;
						}
					}
				}
			}
			if (!ignorable)
			{
				throw StopParsingException(cursor);
			}
		}
	}
}