#include "Parser.h"
#include "Ast_Type.h"
#include "Ast_Decl.h"

/***********************************************************************
IsPotentialTypeDeclVisitor
***********************************************************************/

class IsPotentialTypeDeclVisitor : public Object, public virtual IDeclarationVisitor
{
public:
	bool					isPotentialType = true;

	void Visit(ForwardVariableDeclaration* self)override
	{
		isPotentialType = false;
	}

	void Visit(ForwardFunctionDeclaration* self)override
	{
		isPotentialType = false;
	}

	void Visit(ForwardEnumDeclaration* self)override
	{
	}

	void Visit(ForwardClassDeclaration* self)override
	{
	}

	void Visit(VariableDeclaration* self)override
	{
		isPotentialType = false;
	}

	void Visit(FunctionDeclaration* self)override
	{
		isPotentialType = false;
	}

	void Visit(EnumItemDeclaration* self)override
	{
		isPotentialType = false;
	}

	void Visit(EnumDeclaration* self)override
	{
	}

	void Visit(ClassDeclaration* self)override
	{
	}

	void Visit(TypeAliasDeclaration* self)override
	{
	}

	void Visit(UsingDeclaration* self)override
	{
		isPotentialType = false;
	}

	void Visit(NamespaceDeclaration* self)override
	{
	}
};

/***********************************************************************
ResolveTypeSymbol
***********************************************************************/

Ptr<Resolving> ResolveTypeSymbol(const ParsingArguments& pa, CppName& name, Ptr<Resolving> resolving, bool searchInParentScope, SortedList<Symbol*>& searchedScopes);
Ptr<Resolving> ResolveTypeSymbol(const ParsingArguments& pa, CppName& name, Ptr<Resolving> resolving, bool searchInParentScope);
Ptr<Resolving> ResolveChildTypeSymbol(const ParsingArguments& pa, Ptr<Type> classType, CppName& name, SortedList<Symbol*>& searchedScopes);
Ptr<Resolving> ResolveChildTypeSymbol(const ParsingArguments& pa, Ptr<Type> classType, CppName& name);

Ptr<Resolving> ResolveTypeSymbol(const ParsingArguments& pa, CppName& name, Ptr<Resolving> resolving, bool searchInParentScope, SortedList<Symbol*>& searchedScopes)
{
	auto scope = pa.context;
	if (searchedScopes.Contains(scope))
	{
		return resolving;
	}
	else
	{
		searchedScopes.Add(scope);
	}

	bool found = false;
	while (!found && scope)
	{
		vint index = scope->children.Keys().IndexOf(name.name);
		if (index != -1)
		{
			const auto& symbols = scope->children.GetByIndex(index);
			for (vint i = 0; i < symbols.Count(); i++)
			{
				auto symbol = symbols[i].Obj();
				if (symbol->forwardDeclarationRoot)
				{
					symbol = symbol->forwardDeclarationRoot;
				}

				for (vint i = 0; i < symbol->decls.Count(); i++)
				{
					IsPotentialTypeDeclVisitor visitor;
					symbol->decls[i]->Accept(&visitor);
					if (visitor.isPotentialType)
					{
						found = true;
						if (!resolving)
						{
							resolving = new Resolving;
						}

						if (!resolving->resolvedSymbols.Contains(symbol))
						{
							resolving->resolvedSymbols.Add(symbol);
						}
						break;
					}
				}
			}
		}

		if (!found && scope->decls.Count() > 0)
		{
			if (auto decl = scope->decls[0].Cast<ClassDeclaration>())
			{
				for (vint i = 0; i < decl->baseTypes.Count(); i++)
				{
					resolving = ResolveChildTypeSymbol(pa, decl->baseTypes[i].f1, name, searchedScopes);
				}
			}
		}

		if (!searchInParentScope) break;
		scope = scope->parent;
	}

	return resolving;
}

Ptr<Resolving> ResolveTypeSymbol(const ParsingArguments& pa, CppName& name, Ptr<Resolving> resolving, bool searchInParentScope)
{
	SortedList<Symbol*> searchedScopes;
	return ResolveTypeSymbol(pa, name, resolving, searchInParentScope, searchedScopes);
}

/***********************************************************************
ResolveChildTypeSymbol
***********************************************************************/

class ResolveChildSymbolTypeVisitor : public Object, public virtual ITypeVisitor
{
public:
	const ParsingArguments&	pa;
	CppName&				name;
	SortedList<Symbol*>&	searchedScopes;
	Ptr<Resolving>			resolving;

	ResolveChildSymbolTypeVisitor(const ParsingArguments& _pa, CppName& _name, SortedList<Symbol*>& _searchedScopes)
		:pa(_pa)
		, name(_name)
		, searchedScopes(_searchedScopes)
	{
	}

	void ResolveChildTypeOfResolving(Ptr<Resolving> parentResolving)
	{
		if (parentResolving)
		{
			auto& symbols = parentResolving->resolvedSymbols;
			for (vint i = 0; i < symbols.Count(); i++)
			{
				resolving = ResolveTypeSymbol({ pa,symbols[i] }, name, resolving, false, searchedScopes);
			}
		}
	}

	void Visit(PrimitiveType* self)override
	{
	}

	void Visit(ReferenceType* self)override
	{
	}

	void Visit(ArrayType* self)override
	{
	}

	void Visit(CallingConventionType* self)override
	{
		self->type->Accept(this);
	}

	void Visit(FunctionType* self)override
	{
	}

	void Visit(MemberType* self)override
	{
	}

	void Visit(DeclType* self)override
	{
	}

	void Visit(DecorateType* self)override
	{
		self->type->Accept(this);
	}

	void Visit(RootType* self)override
	{
		resolving = ResolveTypeSymbol({ pa,pa.root.Obj() }, name, resolving, false, searchedScopes);
	}

	void Visit(IdType* self)override
	{
		ResolveChildTypeOfResolving(self->resolving);
	}

	void Visit(ChildType* self)override
	{
		ResolveChildTypeOfResolving(self->resolving);
	}

	void Visit(GenericType* self)override
	{
		self->type->Accept(this);
	}

	void Visit(VariadicTemplateArgumentType* self)override
	{
	}
};

Ptr<Resolving> ResolveChildTypeSymbol(const ParsingArguments& pa, Ptr<Type> classType, CppName& name, SortedList<Symbol*>& searchedScopes)
{
	ResolveChildSymbolTypeVisitor visitor(pa, name, searchedScopes);
	classType->Accept(&visitor);
	return visitor.resolving;
}

Ptr<Resolving> ResolveChildTypeSymbol(const ParsingArguments& pa, Ptr<Type> classType, CppName& name)
{
	SortedList<Symbol*> searchedScopes;
	return ResolveChildTypeSymbol(pa, classType, name, searchedScopes);
}

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
		TEST_LONG_KEYWORD(TYPE_LONG, long);
		TEST_LONG_KEYWORD(TYPE_DOUBLE, double);
		return MakePtr<PrimitiveType>(prefix, CppPrimitiveType::_long);
	}

	if (prefix != CppPrimitivePrefix::_none)
	{
		throw StopParsingException(cursor);
	}
	else
	{
		return nullptr;
	}
#undef TEST_SINGLE_KEYWORD
#undef TEST_LONG_KEYWORD
}

/***********************************************************************
ParseChildType
***********************************************************************/

Ptr<ChildType> ParseChildType(const ParsingArguments& pa, Ptr<Type> classType, bool typenameType, Ptr<CppTokenCursor>& cursor)
{
	CppName cppName;
	if (ParseCppName(cppName, cursor))
	{
		auto resolving = ResolveChildTypeSymbol(pa, classType, cppName);
		if (resolving || typenameType)
		{
			auto type = MakePtr<ChildType>();
			type->classType = classType;
			type->typenameType = typenameType;
			type->name = cppName;
			type->resolving = resolving;
			if (pa.recorder && type->resolving)
			{
				pa.recorder->Index(type->name, type->resolving);
			}
			return type;
		}
	}
	return nullptr;
}

/***********************************************************************
ParseShortType
***********************************************************************/

Ptr<Type> ParseShortType(const ParsingArguments& pa, Ptr<CppTokenCursor>& cursor)
{
	if (TestToken(cursor, CppTokens::SIGNED))
	{
		return ParsePrimitiveType(cursor, CppPrimitivePrefix::_signed);
	}
	else if (TestToken(cursor, CppTokens::UNSIGNED))
	{
		return ParsePrimitiveType(cursor, CppPrimitivePrefix::_unsigned);
	}
	else if (TestToken(cursor, CppTokens::COLON, CppTokens::COLON))
	{
		if (auto type = ParseChildType(pa, MakePtr<RootType>(), false, cursor))
		{
			return type;
		}
		else
		{
			throw StopParsingException(cursor);
		}
	}
	else
	{
		{
			auto result = ParsePrimitiveType(cursor, CppPrimitivePrefix::_none);
			if (result) return result;
		}

		if (TestToken(cursor, CppTokens::DECLTYPE))
		{
			RequireToken(cursor, CppTokens::LPARENTHESIS);
			auto type = MakePtr<DeclType>();
			type->expr = ParseExpr(pa, true, cursor);
			RequireToken(cursor, CppTokens::RPARENTHESIS);
			return type;
		}

		if (TestToken(cursor, CppTokens::CONSTEXPR))
		{
			auto type = MakePtr<DecorateType>();
			type->isConstExpr = true;
			type->type = ParseShortType(pa, cursor);
			return type;
		}

		if (TestToken(cursor, CppTokens::CONST))
		{
			auto type = MakePtr<DecorateType>();
			type->isConst = true;
			type->type = ParseShortType(pa, cursor);
			return type;
		}

		if (TestToken(cursor, CppTokens::VOLATILE))
		{
			auto type = MakePtr<DecorateType>();
			type->isVolatile = true;
			type->type = ParseShortType(pa, cursor);
			return type;
		}

		if (pa.context)
		{
			CppName cppName;
			if (ParseCppName(cppName, cursor))
			{
				if (auto resolving = ResolveTypeSymbol(pa, cppName, nullptr, true))
				{
					auto type = MakePtr<IdType>();
					type->name = cppName;
					type->resolving = resolving;
					if (pa.recorder)
					{
						pa.recorder->Index(type->name, type->resolving);
					}
					return type;
				}
				else
				{
					throw StopParsingException(cursor);
				}
			}
		}
	}

	throw StopParsingException(cursor);
}

/***********************************************************************
ParseLongType
***********************************************************************/

Ptr<Type> ParseLongType(const ParsingArguments& pa, Ptr<CppTokenCursor>& cursor)
{
	bool typenameType = TestToken(cursor, CppTokens::TYPENAME);
	Ptr<Type> typeResult = ParseShortType(pa, cursor);

	while (true)
	{
		if (TestToken(cursor, CppTokens::CONSTEXPR))
		{
			auto type = typeResult.Cast<DecorateType>();
			if (!type)
			{
				type = MakePtr<DecorateType>();
				type->type = typeResult;
			}
			type->isConstExpr = true;
			typeResult = type;
		}
		else if (TestToken(cursor, CppTokens::CONST))
		{
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
			auto type = typeResult.Cast<DecorateType>();
			if (!type)
			{
				type = MakePtr<DecorateType>();
				type->type = typeResult;
			}
			type->isVolatile = true;
			typeResult = type;
		}
		else if (TestToken(cursor, CppTokens::LT))
		{
			auto type = MakePtr<GenericType>();
			type->type = typeResult;
			while (!TestToken(cursor, CppTokens::GT))
			{
				{
					GenericArgument argument;
					argument.type = ParseType(pa, cursor);
					type->arguments.Add(argument);
				}

				if (TestToken(cursor, CppTokens::GT))
				{
					break;
				}
				else
				{
					RequireToken(cursor, CppTokens::COMMA);
				}
			}
			typeResult = type;
		}
		else if (TestToken(cursor, CppTokens::DOT, CppTokens::DOT, CppTokens::DOT))
		{
			auto type = MakePtr<VariadicTemplateArgumentType>();
			type->type = typeResult;
			typeResult = type;
		}
		else
		{
			auto oldCursor = cursor;
			if (TestToken(cursor, CppTokens::COLON, CppTokens::COLON))
			{
				if (auto type = ParseChildType(pa, typeResult, typenameType, cursor))
				{
					typeResult = type;
					continue;
				}
			}

			cursor = oldCursor;
			break;
		}
	}

	return typeResult;
}