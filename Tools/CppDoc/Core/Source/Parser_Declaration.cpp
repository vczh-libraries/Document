#include "Parser.h"
#include "Ast_Type.h"
#include "Ast_Decl.h"

void ParseDeclaration(ParsingArguments& pa, Ptr<CppTokenCursor>& cursor, List<Ptr<Declaration>>& output)
{
	if (TestToken(cursor, CppTokens::DECL_NAMESPACE))
	{
		auto contextSymbol = pa.context;
		Ptr<NamespaceDeclaration> topDecl;
		Ptr<NamespaceDeclaration> contextDecl;

		while (cursor)
		{
			auto decl = MakePtr<NamespaceDeclaration>();
			if (!topDecl)
			{
				topDecl = decl;
			}
			if (contextDecl)
			{
				contextDecl->decls.Add(decl);
			}
			contextDecl = decl;

			if (ParseCppName(decl->name, cursor))
			{
				auto declSymbol = contextSymbol->CreateSymbol(decl);
				contextSymbol = declSymbol;
			}
			else
			{
				throw StopParsingException(cursor);
			}

			if (TestToken(cursor, CppTokens::RBRACE))
			{
				break;
			}
			else
			{
				RequireToken(cursor, CppTokens::COLON, CppTokens::COLON);
			}
		}

		ParsingArguments newPa(pa, contextSymbol);
		while (!TestToken(cursor, CppTokens::RBRACE))
		{
			ParseDeclaration(newPa, cursor, contextDecl->decls);
		}

		output.Add(topDecl);
	}
	else if (TestToken(cursor, CppTokens::DECL_ENUM))
	{
		bool enumClass = TestToken(cursor, CppTokens::DECL_CLASS);

		CppName cppName;
		if (!ParseCppName(cppName, cursor)) throw StopParsingException(cursor);

		Ptr<Type> baseType;
		if (TestToken(cursor, CppTokens::COLON))
		{
			List<Ptr<Declarator>> declarators;
			ParseDeclarator(pa, DeclaratorRestriction::Zero, InitializerRestriction::Zero, cursor, declarators);
			if (declarators.Count() != 1) throw StopParsingException(cursor);
			baseType = declarators[0]->type;
		}

		if (TestToken(cursor, CppTokens::LBRACE))
		{
			auto decl = MakePtr<EnumDeclaration>();
			decl->enumClass = enumClass;
			decl->name = cppName;
			decl->baseType = baseType;

			auto contextSymbol = pa.context->CreateSymbol(decl);
			ParsingArguments newPa(pa, contextSymbol);

			while (!TestToken(cursor, CppTokens::RBRACE))
			{
				auto enumItem = MakePtr<EnumItemDeclaration>();
				if (!ParseCppName(enumItem->name, cursor)) throw StopParsingException(cursor);
				contextSymbol->CreateSymbol(enumItem);
				decl->items.Add(enumItem);

				if (TestToken(cursor, CppTokens::EQ))
				{
					enumItem->value = ParseExpr(newPa, cursor);
				}

				if (!TestToken(cursor, CppTokens::COMMA))
				{
					RequireToken(cursor, CppTokens::RBRACE);
					return;
				}
			}

			output.Add(decl);
		}
		else
		{
			RequireToken(cursor, CppTokens::SEMICOLON);
			throw StopParsingException(cursor);
		}
	}
	else
	{
#define FUNCVAR_DECORATORS(F)\
		F(STATIC, Static)\
		F(MUTABLE, Mutable)\
		F(THREAD_LOCAL, ThreadLocal)\
		F(REGISTER, Register)\
		F(VIRTUAL, Virtual)\
		F(EXPLICIT, Explicit)\
		F(INLINE, Inline)\
		F(__FORCEINLINE, ForceInline)\

#define DEFINE_FUNCVAR_BOOL(TOKEN, NAME) bool decorator##NAME = false;
		FUNCVAR_DECORATORS(DEFINE_FUNCVAR_BOOL)
#undef DEFINE_FUNCVAR_BOOL

		while (cursor)
		{
#define DEFINE_FUNCVAR_TEST(TOKEN, NAME) if (TestToken(cursor, CppTokens::TOKEN)) decorator##NAME = true; else
			FUNCVAR_DECORATORS(DEFINE_FUNCVAR_TEST)
#undef DEFINE_FUNCVAR_TEST
			break;
		}

#undef FUNCVAR_DECORATORS

		List<Ptr<Declarator>> declarators;
		ParseDeclarator(pa, DeclaratorRestriction::Many, InitializerRestriction::Optional, cursor, declarators);
		RequireToken(cursor, CppTokens::SEMICOLON);

		for (vint i = 0; i < declarators.Count(); i++)
		{
			auto declarator = declarators[i];

			bool isFunction = false;
			if (declarator->type.Cast<FunctionType>())
			{
				isFunction = true;
			}
			else if (auto ccType = declarator->type.Cast<CallingConventionType>())
			{
				if (ccType->type.Cast<FunctionType>())
				{
					isFunction = true;
				}
			}

			if (isFunction)
			{
				auto decl = MakePtr<FunctionDeclaration>();
				decl->name = declarator->name;
				decl->type = declarator->type;
				decl->decoratorStatic = decoratorStatic;
				decl->decoratorVirtual = decoratorVirtual;
				decl->decoratorExplicit = decoratorExplicit;
				decl->decoratorInline = decoratorInline;
				decl->decoratorForceInline = decoratorForceInline;
				pa.context->CreateSymbol(decl);
				output.Add(decl);
			}
			else
			{
				auto decl = MakePtr<VariableDeclaration>();
				decl->name = declarator->name;
				decl->type = declarator->type;
				decl->decoratorStatic = decoratorStatic;
				decl->decoratorMutable = decoratorMutable;
				decl->decoratorThreadLocal = decoratorThreadLocal;
				decl->decoratorRegister = decoratorRegister;
				pa.context->CreateSymbol(decl);
				output.Add(decl);
			}
		}
	}
}