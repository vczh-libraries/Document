#include "Parser.h"
#include "Ast_Type.h"
#include "Ast_Decl.h"


template<typename TRoot, typename TForward>
void FindForwardDeclarationRoot(Ptr<Symbol> scope, Ptr<Symbol> forwardSymbol, Ptr<CppTokenCursor> cursor)
{
	const auto& siblings = scope->children[forwardSymbol->name];
	for (vint i = 0; i < siblings.Count(); i++)
	{
		auto& sibling = siblings[i];
		if (sibling->decls[0].Cast<TRoot>())
		{
			if (!forwardSymbol->SetForwardDeclarationRoot(sibling.Obj()))
			{
				throw StopParsingException(cursor);
			}
		}
	}
}

template<typename TRoot, typename TForward>
void FindForwardDeclarations(Ptr<Symbol> scope, Ptr<Symbol> contextSymbol, Ptr<CppTokenCursor> cursor)
{
	const auto& siblings = scope->children[contextSymbol->name];
	for (vint i = 0; i < siblings.Count(); i++)
	{
		auto& sibling = siblings[i];
		if (sibling->decls[0].Cast<TForward>() && sibling->isForwardDeclaration)
		{
			if (!sibling->SetForwardDeclarationRoot(contextSymbol.Obj()))
			{
				throw StopParsingException(cursor);
			}
		}
	}
}

void ParseDeclaration(ParsingArguments& pa, Ptr<CppTokenCursor>& cursor, List<Ptr<Declaration>>& output)
{
	bool decoratorFriend = TestToken(cursor, CppTokens::DECL_FRIEND);

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
				vint index = contextSymbol->children.Keys().IndexOf(decl->name.name);
				if (index == -1)
				{
					contextSymbol = contextSymbol->CreateSymbol(decl);
				}
				else
				{
					auto& symbols = contextSymbol->children.GetByIndex(index);
					if (symbols.Count() == 1 && symbols[0]->decls[0].Cast<NamespaceDeclaration>())
					{
						contextSymbol = symbols[0];
						contextSymbol->decls.Add(decl);
					}
					else
					{
						throw StopParsingException(cursor);
					}
				}
			}
			else
			{
				throw StopParsingException(cursor);
			}

			if (TestToken(cursor, CppTokens::LBRACE))
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
					break;
				}
			}

			RequireToken(cursor, CppTokens::SEMICOLON);
			output.Add(decl);
			FindForwardDeclarations<EnumDeclaration, ForwardEnumDeclaration>(pa.context, contextSymbol, cursor);
		}
		else
		{
			RequireToken(cursor, CppTokens::SEMICOLON);
			auto decl = MakePtr<ForwardEnumDeclaration>();
			decl->enumClass = enumClass;
			decl->name = cppName;
			decl->baseType = baseType;
			auto forwardSymbol = pa.context->CreateSymbol(decl);
			forwardSymbol->isForwardDeclaration = true;
			output.Add(decl);
			FindForwardDeclarationRoot<EnumDeclaration, ForwardEnumDeclaration>(pa.context, forwardSymbol, cursor);
		}
	}
	else if (TestToken(cursor, CppTokens::DECL_CLASS, false) || TestToken(cursor, CppTokens::DECL_STRUCT, false) || TestToken(cursor, CppTokens::DECL_UNION, false))
	{
		ClassType classType = ClassType::Union;
		ClassAccessor defaultAccessor = ClassAccessor::Public;

		switch ((CppTokens)cursor->token.token)
		{
		case CppTokens::DECL_CLASS:
			classType = ClassType::Class;
			defaultAccessor = ClassAccessor::Private;
			break;
		case CppTokens::DECL_STRUCT:
			classType = ClassType::Struct;
			break;
		}
		cursor = cursor->Next();

		CppName cppName;
		if (!ParseCppName(cppName, cursor))
		{
			throw StopParsingException(cursor);
		}

		if (TestToken(cursor, CppTokens::SEMICOLON))
		{
			auto decl = MakePtr<ForwardClassDeclaration>();
			decl->classType = classType;
			decl->name = cppName;
			auto forwardSymbol = pa.context->CreateSymbol(decl);
			forwardSymbol->isForwardDeclaration = true;
			output.Add(decl);
			FindForwardDeclarationRoot<ClassDeclaration, ForwardClassDeclaration>(pa.context, forwardSymbol, cursor);
		}
		else
		{
			auto decl = MakePtr<ClassDeclaration>();
			decl->classType = classType;
			decl->name = cppName;
			auto forwardSymbol = pa.context->CreateSymbol(decl);
			output.Add(decl);
			FindForwardDeclarations<ClassDeclaration, ForwardClassDeclaration>(pa.context, forwardSymbol, cursor);

			if (TestToken(cursor, CppTokens::COLON))
			{
				while (true)
				{
					ClassAccessor accessor = defaultAccessor;
					if (TestToken(cursor, CppTokens::PUBLIC))
					{
						accessor = ClassAccessor::Public;
					}
					else if (TestToken(cursor, CppTokens::PROTECTED))
					{
						accessor = ClassAccessor::Protected;
					}
					else if (TestToken(cursor, CppTokens::PRIVATE))
					{
						accessor = ClassAccessor::Private;
					}

					List<Ptr<Declarator>> declarators;
					ParseDeclarator(pa, DeclaratorRestriction::Zero, InitializerRestriction::Zero, cursor, declarators);
					if (declarators.Count() != -1) throw StopParsingException(cursor);
					decl->baseTypes.Add({ accessor,declarators[0]->type });

					if (TestToken(cursor, CppTokens::LBRACE, false))
					{
						break;
					}
					else
					{
						RequireToken(cursor, CppTokens::COMMA);
					}
				}
			}

			RequireToken(cursor, CppTokens::LBRACE);
			ClassAccessor accessor = defaultAccessor;
			while (true)
			{
				if (TestToken(cursor, CppTokens::PUBLIC))
				{
					accessor = ClassAccessor::Public;
					RequireToken(cursor, CppTokens::COLON);
				}
				else if (TestToken(cursor, CppTokens::PROTECTED))
				{
					accessor = ClassAccessor::Protected;
					RequireToken(cursor, CppTokens::COLON);
				}
				else if (TestToken(cursor, CppTokens::PRIVATE))
				{
					accessor = ClassAccessor::Private;
					RequireToken(cursor, CppTokens::COLON);
				}
				else if(TestToken(cursor,CppTokens::RBRACE))
				{
					break;
				}
				else
				{
					List<Ptr<Declaration>> declarations;
					ParseDeclaration(pa, cursor, declarations);
					for (vint i = 0; i < declarations.Count(); i++)
					{
						decl->decls.Add({ accessor,declarations[i] });
					}
				}
			}

			RequireToken(cursor, CppTokens::SEMICOLON);
		}
	}
	else
	{
#define FUNCVAR_DECORATORS(F)\
		F(DECL_EXTERN, Extern)\
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
#define FILL_FUNCTION(NAME)\
				NAME->name = declarator->name;\
				NAME->type = declarator->type;\
				NAME->decoratorExtern = decoratorExtern;\
				NAME->decoratorFriend = decoratorFriend;\
				NAME->decoratorStatic = decoratorStatic;\
				NAME->decoratorVirtual = decoratorVirtual;\
				NAME->decoratorExplicit = decoratorExplicit;\
				NAME->decoratorInline = decoratorInline;\
				NAME->decoratorForceInline = decoratorForceInline\

				{
					auto decl = MakePtr<ForwardFunctionDeclaration>();
					FILL_FUNCTION(decl);
					auto forwardSymbol = pa.context->CreateSymbol(decl);
					forwardSymbol->isForwardDeclaration = true;
					output.Add(decl);
				}
#undef FILL_FUNCTION
			}
			else
			{
#define FILL_VARIABLE(NAME)\
				NAME->name = declarator->name;\
				NAME->type = declarator->type;\
				NAME->decoratorExtern = decoratorExtern;\
				NAME->decoratorStatic = decoratorStatic;\
				NAME->decoratorMutable = decoratorMutable;\
				NAME->decoratorThreadLocal = decoratorThreadLocal;\
				NAME->decoratorRegister = decoratorRegister\

				if (decoratorExtern)
				{
					auto decl = MakePtr<ForwardVariableDeclaration>();
					FILL_VARIABLE(decl);
					auto forwardSymbol = pa.context->CreateSymbol(decl);
					forwardSymbol->isForwardDeclaration = true;
					output.Add(decl);
					FindForwardDeclarationRoot<VariableDeclaration, ForwardVariableDeclaration>(pa.context, forwardSymbol, cursor);
				}
				else
				{
					auto decl = MakePtr<VariableDeclaration>();
					FILL_VARIABLE(decl);
					decl->initializer = declarator->initializer;
					auto contextSymbol = pa.context->CreateSymbol(decl);
					output.Add(decl);
					FindForwardDeclarations<VariableDeclaration, ForwardVariableDeclaration>(pa.context, contextSymbol, cursor);
				}
#undef FILL_VARIABLE
			}
		}
	}
}