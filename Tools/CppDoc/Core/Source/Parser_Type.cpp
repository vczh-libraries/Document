#include "Parser.h"
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

Ptr<IdType> ParseIdType(const ParsingArguments& pa, Ptr<CppTokenCursor>& cursor)
{
	auto idKind = cursor ? (CppTokens)cursor->token.token : CppTokens::ID;

	// check if this is a c style type reference, e.g. struct something
	auto type = MakePtr<IdType>();
	if (TestToken(cursor, CppTokens::DECL_ENUM, false) || TestToken(cursor, CppTokens::DECL_CLASS, false) || TestToken(cursor, CppTokens::DECL_STRUCT, false) || TestToken(cursor, CppTokens::DECL_UNION, false))
	{
		type->cStyleTypeReference = true;
		SkipToken(cursor);
	}

	if (ParseCppName(type->name, cursor))
	{
		if (auto resolving = ResolveSymbol(pa, type->name, SearchPolicy::SymbolAccessableInScope).types)
		{
			type->resolving = resolving;
			if (pa.recorder)
			{
				pa.recorder->Index(type->name, type->resolving->resolvedSymbols);
			}

			if (type->cStyleTypeReference)
			{
				for (vint i = 0; i < type->resolving->resolvedSymbols.Count(); i++)
				{
					auto symbol = type->resolving->resolvedSymbols[i];
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
					type->resolving = MakePtr<Resolving>();
					type->resolving->resolvedSymbols.Add(contextSymbol);
				}
				else
				{
					throw StopParsingException(cursor);
				}
			}
			return type;
		}
	}
	throw StopParsingException(cursor);
}

/***********************************************************************
TryParseChildType
***********************************************************************/

Ptr<ChildType> TryParseChildType(const ParsingArguments& pa, Ptr<Category_Id_Child_Generic_Root_Type> classType, bool typenameType, bool& templateKeyword, Ptr<CppTokenCursor>& cursor)
{
	if ((templateKeyword = TestToken(cursor, CppTokens::DECL_TEMPLATE)))
	{
		if (!typenameType)
		{
			return nullptr;
		}
	}

	CppName cppName;
	if (ParseCppName(cppName, cursor))
	{
		auto resolving = ResolveChildSymbol(pa, classType, cppName).types;
		if (resolving || typenameType)
		{
			auto type = MakePtr<ChildType>();
			type->classType = classType;
			type->typenameType = typenameType;
			type->name = cppName;
			type->resolving = resolving;
			if (pa.recorder && type->resolving)
			{
				pa.recorder->Index(type->name, type->resolving->resolvedSymbols);
			}
			return type;
		}
	}
	return nullptr;
}

Ptr< Category_Id_Child_Generic_Root_Type> TryParseGenericType(const ParsingArguments& pa, Ptr<Category_Id_Child_Type> classType, Ptr<CppTokenCursor>& cursor)
{
	if (TestToken(cursor, CppTokens::LT))
	{
		// TYPE< { TYPE/EXPR ...} >
		auto type = MakePtr<GenericType>();
		type->type = classType;
		ParseGenericArgumentsSkippedLT(pa, cursor, type->arguments);
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

Ptr<Type> ParseNameType(const ParsingArguments& pa, bool typenameType, Ptr<CppTokenCursor>& cursor)
{
	bool templateKeyword = false;
	Ptr<Category_Id_Child_Generic_Root_Type> typeResult;
	if (TestToken(cursor, CppTokens::COLON, CppTokens::COLON))
	{
		// :: NAME
		if (auto type = TryParseChildType(pa, MakePtr<RootType>(), false, templateKeyword, cursor))
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
			static const wchar_t	NAME__make_integer_seq[]	= L"__make_integer_seq";
			static const vint		SIZE__make_integer_seq		= sizeof(NAME__make_integer_seq) / sizeof(wchar_t) - 1;

			if (cursor->token.length == SIZE__make_integer_seq && wcsncmp(cursor->token.reading, NAME__make_integer_seq, SIZE__make_integer_seq) == 0)
			{
				throw StopParsingException(cursor);
			}
		}
		// NAME
		typeResult = TryParseGenericType(pa, ParseIdType(pa, cursor), cursor);
	SKIP_NORMAL_PARSING:;
	}

	while (true)
	{
		// TYPE::NAME
		auto oldCursor = cursor;
		if (TestToken(cursor, CppTokens::COLON, CppTokens::COLON))
		{
			if (templateKeyword)
			{
				throw StopParsingException(cursor);
			}
			if (auto type = TryParseChildType(pa, typeResult, typenameType, templateKeyword, cursor))
			{
				typeResult = TryParseGenericType(pa, type, cursor);
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

Ptr<Type> ParseShortType(const ParsingArguments& pa, bool typenameType, Ptr<CppTokenCursor>& cursor)
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
		auto type = ParseShortType(pa, typenameType, cursor);
		auto dt = type.Cast<DecorateType>();
		if (!dt)
		{
			dt = MakePtr<DecorateType>();
			dt->type = type;
		}
		dt->isConst = true;
		return dt;
	}
	else if (TestToken(cursor, CppTokens::VOLATILE))
	{
		// volatile TYPE
		auto type = ParseShortType(pa, typenameType, cursor);
		auto dt = type.Cast<DecorateType>();
		if (!dt)
		{
			dt = MakePtr<DecorateType>();
			dt->type = type;
		}
		dt->isVolatile = true;
		return dt;
	}
	else if (TestToken(cursor, CppTokens::TYPENAME))
	{
		return ParseShortType(pa, true, cursor);
	}
	else
	{
		{
			// PRIMITIVE-TYPE
			auto result = ParsePrimitiveType(cursor, CppPrimitivePrefix::_none);
			if (result) return result;
		}

		return ParseNameType(pa, typenameType, cursor);
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
		typeResult = ParseShortType(pa, typenameType, cursor);
	}

	while (true)
	{
		if (TestToken(cursor, CppTokens::CONST))
		{
			// TYPE const
			auto type = typeResult.Cast<DecorateType>();
			if (!type)
			{
				type = MakePtr<DecorateType>();
				type->type = typeResult;
			}
			type->isConst = true;
			typeResult = type;
		}
		else if (TestToken(cursor, CppTokens::VOLATILE))
		{
			// TYPE volatile
			auto type = typeResult.Cast<DecorateType>();
			if (!type)
			{
				type = MakePtr<DecorateType>();
				type->type = typeResult;
			}
			type->isVolatile = true;
			typeResult = type;
		}
		else
		{
			break;
		}
	}

	return typeResult;
}