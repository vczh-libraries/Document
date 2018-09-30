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

	void Visit(UsingNamespaceDeclaration* self)override
	{
		isPotentialType = false;
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
AddSymbolToResolve
***********************************************************************/

void AddSymbolToResolve(Ptr<Resolving>& resolving, Symbol* symbol)
{
	if (!resolving)
	{
		resolving = new Resolving;
	}

	if (!resolving->resolvedSymbols.Contains(symbol))
	{
		resolving->resolvedSymbols.Add(symbol);
	}
}

/***********************************************************************
ResolveTypeSymbol
***********************************************************************/

enum class SearchPolicy
{
	SymbolAccessableInScope,
	ChildSymbol,
	ChildSymbolRequestedFromSubClass,
};

Ptr<Resolving> ResolveTypeSymbol(const ParsingArguments& pa, CppName& name, Ptr<Resolving> resolving, SearchPolicy policy, SortedList<Symbol*>& searchedScopes);
Ptr<Resolving> ResolveTypeSymbol(const ParsingArguments& pa, CppName& name, Ptr<Resolving> resolving, SearchPolicy policy);
Ptr<Resolving> ResolveChildTypeSymbol(const ParsingArguments& pa, Ptr<Type> classType, CppName& name, Ptr<Resolving> resolving, SearchPolicy policy, SortedList<Symbol*>& searchedScopes);
Ptr<Resolving> ResolveChildTypeSymbol(const ParsingArguments& pa, Ptr<Type> classType, CppName& name, Ptr<Resolving> resolving);

Ptr<Resolving> ResolveTypeSymbol(const ParsingArguments& pa, CppName& name, Ptr<Resolving> resolving, SearchPolicy policy, SortedList<Symbol*>& searchedScopes)
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

#define RESOLVED_SYMBOLS_COUNT (resolving ? resolving->resolvedSymbols.Count() : 0)
#define FOUND (baseline < RESOLVED_SYMBOLS_COUNT)

	vint baseline = RESOLVED_SYMBOLS_COUNT;
	while (scope)
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
						AddSymbolToResolve(resolving, symbol);
						break;
					}
				}
			}
		}
		if (FOUND) break;

		if (scope->decls.Count() > 0)
		{
			if (auto decl = scope->decls[0].Cast<ClassDeclaration>())
			{
				if (decl->name.name == name.name && policy != SearchPolicy::ChildSymbol)
				{
					AddSymbolToResolve(resolving, decl->symbol);
				}
				else
				{
					for (vint i = 0; i < decl->baseTypes.Count(); i++)
					{
						auto childPolicy =
							policy == SearchPolicy::ChildSymbol
							? SearchPolicy::ChildSymbol
							: SearchPolicy::ChildSymbolRequestedFromSubClass;
						resolving = ResolveChildTypeSymbol(pa, decl->baseTypes[i].f1, name, resolving, childPolicy, searchedScopes);
					}
				}
			}
		}
		if (FOUND) break;

		if (policy != SearchPolicy::SymbolAccessableInScope) break;
		scope = scope->parent;
	}

#undef FOUND
#undef RESOLVED_SYMBOLS_COUNT

	return resolving;
}

Ptr<Resolving> ResolveTypeSymbol(const ParsingArguments& pa, CppName& name, Ptr<Resolving> resolving, SearchPolicy policy)
{
	SortedList<Symbol*> searchedScopes;
	return ResolveTypeSymbol(pa, name, resolving, policy, searchedScopes);
}

/***********************************************************************
ResolveChildTypeSymbol
***********************************************************************/

class ResolveChildSymbolTypeVisitor : public Object, public virtual ITypeVisitor
{
public:
	const ParsingArguments&		pa;
	CppName&					name;
	Ptr<Resolving>				resolving;
	SearchPolicy				policy;
	SortedList<Symbol*>&		searchedScopes;

	ResolveChildSymbolTypeVisitor(const ParsingArguments& _pa, CppName& _name, Ptr<Resolving> _resolving, SearchPolicy _policy, SortedList<Symbol*>& _searchedScopes)
		:pa(_pa)
		, name(_name)
		, resolving(_resolving)
		, policy(_policy)
		, searchedScopes(_searchedScopes)
	{
	}

	void ResolveChildTypeOfResolving(Ptr<Resolving> parentResolving)
	{
		if (parentResolving)
		{
			parentResolving->Calibrate();
			auto& symbols = parentResolving->resolvedSymbols;
			for (vint i = 0; i < symbols.Count(); i++)
			{
				resolving = ResolveTypeSymbol({ pa,symbols[i] }, name, resolving, policy, searchedScopes);
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
		resolving = ResolveTypeSymbol({ pa,pa.root.Obj() }, name, resolving, SearchPolicy::ChildSymbol, searchedScopes);
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

Ptr<Resolving> ResolveChildTypeSymbol(const ParsingArguments& pa, Ptr<Type> classType, CppName& name, Ptr<Resolving> resolving, SearchPolicy policy, SortedList<Symbol*>& searchedScopes)
{
	ResolveChildSymbolTypeVisitor visitor(pa, name, resolving, policy, searchedScopes);
	classType->Accept(&visitor);
	return visitor.resolving;
}

Ptr<Resolving> ResolveChildTypeSymbol(const ParsingArguments& pa, Ptr<Type> classType, CppName& name, Ptr<Resolving> resolving)
{
	SortedList<Symbol*> searchedScopes;
	return ResolveChildTypeSymbol(pa, classType, name, resolving, SearchPolicy::ChildSymbol, searchedScopes);
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
		auto resolving = ResolveChildTypeSymbol(pa, classType, cppName, nullptr);
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
		// signed INTEGRAL-TYPE
		return ParsePrimitiveType(cursor, CppPrimitivePrefix::_signed);
	}
	else if (TestToken(cursor, CppTokens::UNSIGNED))
	{
		// unsigned INTEGRAL-TYPE
		return ParsePrimitiveType(cursor, CppPrimitivePrefix::_unsigned);
	}
	else if (TestToken(cursor, CppTokens::COLON, CppTokens::COLON))
	{
		// :: NAME
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
			// PRIMITIVE-TYPE
			auto result = ParsePrimitiveType(cursor, CppPrimitivePrefix::_none);
			if (result) return result;
		}

		if (TestToken(cursor, CppTokens::DECLTYPE))
		{
			// decltype(EXPRESSION)
			RequireToken(cursor, CppTokens::LPARENTHESIS);
			auto type = MakePtr<DeclType>();
			type->expr = ParseExpr(pa, true, cursor);
			RequireToken(cursor, CppTokens::RPARENTHESIS);
			return type;
		}

		if (TestToken(cursor, CppTokens::CONSTEXPR))
		{
			// constexpr TYPE
			auto type = MakePtr<DecorateType>();
			type->isConstExpr = true;
			type->type = ParseShortType(pa, cursor);
			return type;
		}

		if (TestToken(cursor, CppTokens::CONST))
		{
			// const TYPE
			auto type = MakePtr<DecorateType>();
			type->isConst = true;
			type->type = ParseShortType(pa, cursor);
			return type;
		}

		if (TestToken(cursor, CppTokens::VOLATILE))
		{
			// volatile TYPE
			auto type = MakePtr<DecorateType>();
			type->isVolatile = true;
			type->type = ParseShortType(pa, cursor);
			return type;
		}

		if (pa.context)
		{
			// NAME
			CppName cppName;
			if (ParseCppName(cppName, cursor))
			{
				if (auto resolving = ResolveTypeSymbol(pa, cppName, nullptr, SearchPolicy::SymbolAccessableInScope))
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
			// TYPE constexpr
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
		else if (TestToken(cursor, CppTokens::LT))
		{
			// TYPE< { TYPE ...} >
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
			// TYPE ...
			auto type = MakePtr<VariadicTemplateArgumentType>();
			type->type = typeResult;
			typeResult = type;
		}
		else
		{
			// TYPE::NAME
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