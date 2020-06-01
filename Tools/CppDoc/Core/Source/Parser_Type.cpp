#include "Parser.h"
#include "Parser_Declarator.h"
#include "Symbol_Visit.h"
#include "Ast_Type.h"
#include "Ast_Decl.h"

/***********************************************************************
ParsePrimitiveType
***********************************************************************/

Ptr<Type> ParsePrimitiveType(Ptr<CppTokenCursor>& cursor, CppPrimitivePrefix prefix)
{
#define TEST_SINGLE_KEYWORD(TOKEN, KEYWORD)\
	if (TestToken(cursor, CppTokens::TOKEN)) return MakePtr<PrimitiveType>(prefix, CppPrimitiveType::_##KEYWORD)

#define TEST_LONG_KEYWORD(TOKEN, KEYWORD)\
	if (TestToken(cursor, CppTokens::TOKEN)) return MakePtr<PrimitiveType>(prefix, CppPrimitiveType::_long_##KEYWORD)

	TEST_SINGLE_KEYWORD(TYPE_AUTO, auto);
	TEST_SINGLE_KEYWORD(TYPE_VOID, void);
	TEST_SINGLE_KEYWORD(TYPE_BOOL, bool);
	TEST_SINGLE_KEYWORD(TYPE_CHAR, char);
	TEST_SINGLE_KEYWORD(TYPE_WCHAR_T, wchar_t);
	TEST_SINGLE_KEYWORD(TYPE_CHAR16_T, char16_t);
	TEST_SINGLE_KEYWORD(TYPE_CHAR32_T, char32_t);
	TEST_SINGLE_KEYWORD(TYPE_SHORT, short);
	TEST_SINGLE_KEYWORD(TYPE_INT, int);
	TEST_SINGLE_KEYWORD(TYPE___INT8, __int8);
	TEST_SINGLE_KEYWORD(TYPE___INT16, __int16);
	TEST_SINGLE_KEYWORD(TYPE___INT32, __int32);
	TEST_SINGLE_KEYWORD(TYPE___INT64, __int64);
	TEST_SINGLE_KEYWORD(TYPE_FLOAT, float);
	TEST_SINGLE_KEYWORD(TYPE_DOUBLE, double);

	if (TestToken(cursor, CppTokens::TYPE_LONG))
	{
		TEST_LONG_KEYWORD(TYPE_INT, int);
		TEST_LONG_KEYWORD(TYPE_LONG, long);
		TEST_LONG_KEYWORD(TYPE_DOUBLE, double);
		return MakePtr<PrimitiveType>(prefix, CppPrimitiveType::_long);
	}
#undef TEST_SINGLE_KEYWORD
#undef TEST_LONG_KEYWORD

	if (prefix == CppPrimitivePrefix::_none)
	{
		return nullptr;
	}

	return MakePtr<PrimitiveType>(prefix, CppPrimitiveType::_int);
}

/***********************************************************************
ParseIdType
***********************************************************************/

Ptr<IdType> ParseIdType(const ParsingArguments& pa, ShortTypeTypenameKind typenameKind, Ptr<CppTokenCursor>& cursor)
{
	auto idKind = cursor ? (CppTokens)cursor->token.token : CppTokens::ID;

	// check if this is a c style type reference, e.g. struct something
	auto type = MakePtr<IdType>();
	if (TestToken(cursor, CppTokens::DECL_ENUM, false) || TestToken(cursor, CppTokens::DECL_CLASS, false) || TestToken(cursor, CppTokens::DECL_STRUCT, false) || TestToken(cursor, CppTokens::DECL_UNION, false))
	{
		type->cStyleTypeReference = true;
		SkipToken(cursor);
	}

	while (SkipSpecifiers(cursor));
	if (ParseCppName(type->name, cursor))
	{
		if (auto resolving = ResolveSymbolInContext(pa, type->name, type->cStyleTypeReference).types)
		{
			type->resolving = resolving;
			if (pa.recorder)
			{
				pa.recorder->Index(type->name, type->resolving->items);
			}

			if (type->cStyleTypeReference)
			{
				for (vint i = 0; i < type->resolving->items.Count(); i++)
				{
					auto symbol = type->resolving->items[i].symbol;
					switch (idKind)
					{
					case CppTokens::DECL_ENUM:
						if (symbol->kind != symbol_component::SymbolKind::Enum)
						{
							throw StopParsingException(cursor);
						}
						break;
					case CppTokens::DECL_CLASS:
						if (symbol->kind != symbol_component::SymbolKind::Class)
						{
							throw StopParsingException(cursor);
						}
						break;
					case CppTokens::DECL_STRUCT:
						if (symbol->kind != symbol_component::SymbolKind::Struct)
						{
							throw StopParsingException(cursor);
						}
						break;
					case CppTokens::DECL_UNION:
						if (symbol->kind != symbol_component::SymbolKind::Union)
						{
							throw StopParsingException(cursor);
						}
						break;
					}
				}
			}

			return type;
		}

		// if a c style type reference doesn't resolve, a forward declaration is created
		if (type->cStyleTypeReference)
		{
			if (!type->resolving)
			{
				Ptr<Declaration> forwardDecl;
				symbol_component::SymbolKind symbolKind;
				switch (idKind)
				{
				case CppTokens::DECL_ENUM:
					{
						symbolKind = symbol_component::SymbolKind::Enum;
						auto decl = MakePtr<ForwardEnumDeclaration>();
						forwardDecl = decl;
					}
					break;
				case CppTokens::DECL_CLASS:
					{
						symbolKind = symbol_component::SymbolKind::Class;
						auto decl = MakePtr<ForwardClassDeclaration>();
						decl->classType = CppClassType::Class;
						forwardDecl = decl;
					}
					break;
				case CppTokens::DECL_STRUCT:
					{
						symbolKind = symbol_component::SymbolKind::Struct;
						auto decl = MakePtr<ForwardClassDeclaration>();
						decl->classType = CppClassType::Struct;
						forwardDecl = decl;
					}
					break;
				case CppTokens::DECL_UNION:
					{
						symbolKind = symbol_component::SymbolKind::Union;
						auto decl = MakePtr<ForwardClassDeclaration>();
						decl->classType = CppClassType::Union;
						forwardDecl = decl;
					}
					break;
				}

				forwardDecl->name.name = type->name.name;
				if (pa.program)
				{
					pa.program->decls.Insert(pa.program->createdForwardDeclByCStyleTypeReference++, forwardDecl);
				}
				if (auto contextSymbol = pa.root->AddForwardDeclToSymbol_NFb(forwardDecl, symbolKind))
				{
					Resolving::AddSymbol(pa, type->resolving, contextSymbol);
				}
				else
				{
					throw StopParsingException(cursor);
				}
			}
			return type;
		}

		// if an expression is not required, just return the type without resolving
		if (typenameKind != ShortTypeTypenameKind::No)
		{
			return type;
		}
	}
	throw StopParsingException(cursor);
}

/***********************************************************************
TryParseChildType
***********************************************************************/

Ptr<ChildType> TryParseChildType(const ParsingArguments& pa, Ptr<Category_Id_Child_Generic_Root_Type> classType, ShortTypeTypenameKind typenameKind, bool& templateKeyword, Ptr<CppTokenCursor>& cursor)
{
	if ((templateKeyword = TestToken(cursor, CppTokens::DECL_TEMPLATE)))
	{
		if (typenameKind == ShortTypeTypenameKind::No)
		{
			return nullptr;
		}
	}

	CppName cppName;
	if (ParseCppName(cppName, cursor))
	{
		auto resolving = ResolveChildSymbol(pa, classType, cppName).types;
		if (resolving || typenameKind != ShortTypeTypenameKind::No)
		{
			auto type = MakePtr<ChildType>();
			type->classType = classType;
			switch (typenameKind)
			{
			case ShortTypeTypenameKind::No:
				type->typenameType = false;
				break;
			case ShortTypeTypenameKind::Yes:
				type->typenameType = true;
				break;
			case ShortTypeTypenameKind::Implicit:
				type->typenameType = !resolving;
				break;
			}
			type->name = cppName;
			type->resolving = resolving;
			if (pa.recorder && type->resolving)
			{
				pa.recorder->Index(type->name, type->resolving->items);
			}
			return type;
		}
	}
	return nullptr;
}

/***********************************************************************
TryParseGenericType
***********************************************************************/

Ptr< Category_Id_Child_Generic_Root_Type> TryParseGenericType(const ParsingArguments& pa, Ptr<Category_Id_Child_Type> classType, Ptr<CppTokenCursor>& cursor)
{
	if (TestToken(cursor, CppTokens::LT))
	{
		// TYPE< { TYPE/EXPR ...} >
		auto type = MakePtr<GenericType>();
		type->type = classType;
		ParseGenericArgumentsSkippedLT(pa, cursor, type->arguments, CppTokens::GT);
		return type;
	}
	else
	{
		return classType;
	}
}

/***********************************************************************
ParseNameType
***********************************************************************/

Ptr<Type> ParseNameType(const ParsingArguments& pa, ShortTypeTypenameKind typenameKind, Ptr<CppTokenCursor>& cursor)
{
	Ptr<Category_Id_Child_Generic_Root_Type> typeResult;
	if (TestToken(cursor, CppTokens::COLON, CppTokens::COLON))
	{
		// :: NAME
		// :: NAME <...>
		bool templateKeyword = false;
		if (auto type = TryParseChildType(pa, MakePtr<RootType>(), ShortTypeTypenameKind::No, templateKeyword, cursor))
		{
			typeResult = TryParseGenericType(pa, type, cursor);
		}
		else
		{
			throw StopParsingException(cursor);
		}
	}
	else
	{
		if (TestToken(cursor, CppTokens::ID, false))
		{
#define DEFINE_INTRINSIC_NAME(INTRINSIC_NAME)																		\
			static const wchar_t	NAME##INTRINSIC_NAME[]	= L#INTRINSIC_NAME;										\
			static const vint		SIZE##INTRINSIC_NAME	= sizeof(NAME##INTRINSIC_NAME) / sizeof(wchar_t) - 1	\

#define MATCH_INTRINSIC_NAME(INTRINSIC_NAME) cursor->token.length == SIZE##INTRINSIC_NAME && wcsncmp(cursor->token.reading, NAME##INTRINSIC_NAME, SIZE##INTRINSIC_NAME) == 0

			DEFINE_INTRINSIC_NAME(__make_integer_seq);
			DEFINE_INTRINSIC_NAME(__underlying_type);

			if (MATCH_INTRINSIC_NAME(__make_integer_seq))
			{
				SkipToken(cursor);
				RequireToken(cursor, CppTokens::LT);
				auto sequenceType = ParseIdType(pa, ShortTypeTypenameKind::Implicit, cursor);
				RequireToken(cursor, CppTokens::COMMA);
				auto elementType = ParseType(pa, cursor);
				RequireToken(cursor, CppTokens::COMMA);
				ParseExpr(pa, pea_GenericArgument(), cursor);
				RequireToken(cursor, CppTokens::GT);

				auto genericType = MakePtr<GenericType>();
				genericType->type = sequenceType;

				VariadicItem<GenericArgument> argument;
				argument.isVariadic = false;
				argument.item.type = elementType;
				genericType->arguments.Add(argument);

				typeResult = genericType;
				goto SKIP_NORMAL_PARSING;
			}
			else if (MATCH_INTRINSIC_NAME(__underlying_type))
			{
				SkipToken(cursor);
				RequireToken(cursor, CppTokens::LPARENTHESIS);
				ParseType(pa, cursor);
				RequireToken(cursor, CppTokens::RPARENTHESIS);

				auto intType = MakePtr<PrimitiveType>();
				intType->prefix = CppPrimitivePrefix::_none;
				intType->primitive = CppPrimitiveType::_int;
				return intType;
			}
#undef MATCH_INTRINSIC_NAME
#undef DEFINE_INTRINSIC_NAME
		}
		// NAME
		// NAME <...>
		typeResult = TryParseGenericType(pa, ParseIdType(pa, typenameKind, cursor), cursor);
	SKIP_NORMAL_PARSING:;
	}

	while (true)
	{
		// TYPE::NAME
		// TYPE::NAME <...>
		auto oldCursor = cursor;
		if (TestToken(cursor, CppTokens::COLON, CppTokens::COLON))
		{
			bool templateKeyword = false;
			if (auto type = TryParseChildType(pa, typeResult, typenameKind, templateKeyword, cursor))
			{
				auto genericType = TryParseGenericType(pa, type, cursor);
				if (templateKeyword && genericType == type)
				{
					// ":: template X" not followed by "<"
					throw StopParsingException(cursor);
				}
				typeResult = genericType;
				continue;
			}
		}
		cursor = oldCursor;
		return typeResult;
	}
}

/***********************************************************************
ParseShortType
***********************************************************************/

Ptr<Type> ParseShortType(const ParsingArguments& pa, ShortTypeTypenameKind typenameKind, Ptr<CppTokenCursor>& cursor)
{
	if (TestToken(cursor, CppTokens::SIGNED))
	{
		// signed INTEGRAL-TYPE
		return ParsePrimitiveType(cursor, CppPrimitivePrefix::_signed);
	}
	else if (TestToken(cursor, CppTokens::UNSIGNED))
	{
		// unsigned INTEGRAL-TYPE
		return ParsePrimitiveType(cursor, CppPrimitivePrefix::_unsigned);
	}
	else if (TestToken(cursor, CppTokens::DECLTYPE))
	{
		// decltype(EXPRESSION)
		RequireToken(cursor, CppTokens::LPARENTHESIS);
		auto type = MakePtr<DeclType>();
		if (!TestToken(cursor, CppTokens::TYPE_AUTO))
		{
			type->expr = ParseExpr(pa, pea_Full(), cursor);
		}
		RequireToken(cursor, CppTokens::RPARENTHESIS);
		return type;
	}
	else if (TestToken(cursor, CppTokens::CONST))
	{
		// const TYPE
		auto type = ParseShortType(pa, typenameKind, cursor);
		return AddCVType(type, true, false);
	}
	else if (TestToken(cursor, CppTokens::VOLATILE))
	{
		// volatile TYPE
		auto type = ParseShortType(pa, typenameKind, cursor);
		return AddCVType(type, false, true);
	}
	else if (TestToken(cursor, CppTokens::TYPENAME))
	{
		return ParseShortType(pa, ShortTypeTypenameKind::Yes, cursor);
	}
	else
	{
		{
			// PRIMITIVE-TYPE
			auto result = ParsePrimitiveType(cursor, CppPrimitivePrefix::_none);
			if (result) return result;
		}

		return ParseNameType(pa, typenameKind, cursor);
	}
}

/***********************************************************************
ParseLongType
***********************************************************************/

Ptr<Type> ParseLongType(const ParsingArguments& pa, Ptr<CppTokenCursor>& cursor)
{
	Ptr<Type> typeResult;
	{
		bool typenameType = TestToken(cursor, CppTokens::TYPENAME);
		typeResult = ParseShortType(pa, (typenameType ? ShortTypeTypenameKind::Yes : ShortTypeTypenameKind::No), cursor);
	}

	while (true)
	{
		if (TestToken(cursor, CppTokens::CONST))
		{
			// TYPE const
			typeResult = AddCVType(typeResult, true, false);
		}
		else if (TestToken(cursor, CppTokens::VOLATILE))
		{
			// TYPE volatile
			typeResult = AddCVType(typeResult, false, true);
		}
		else
		{
			break;
		}
	}

	return typeResult;
}