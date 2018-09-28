#include "Parser.h"
#include "Ast_Type.h"
#include "Ast_Expr.h"
#include "Ast_Decl.h"

/***********************************************************************
FindForward
***********************************************************************/

template<typename TRoot, typename TForward>
void FindForwardDeclarationRoot(Symbol* scope, Symbol* forwardSymbol, Ptr<CppTokenCursor> cursor)
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
void FindForwardDeclarations(Symbol* scope, Symbol* contextSymbol, Ptr<CppTokenCursor> cursor)
{
	const auto& siblings = scope->children[contextSymbol->name];
	for (vint i = 0; i < siblings.Count(); i++)
	{
		auto& sibling = siblings[i];
		if (sibling->decls[0].Cast<TForward>() && sibling->isForwardDeclaration)
		{
			if (!sibling->SetForwardDeclarationRoot(contextSymbol))
			{
				throw StopParsingException(cursor);
			}
		}
	}
}

/***********************************************************************
ParseDeclaration
***********************************************************************/

void ParseDeclaration(const ParsingArguments& pa, Ptr<CppTokenCursor>& cursor, List<Ptr<Declaration>>& output)
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
					contextSymbol = contextSymbol->CreateDeclSymbol(decl);
				}
				else
				{
					auto& symbols = contextSymbol->children.GetByIndex(index);
					if (symbols.Count() == 1 && symbols[0]->decls[0].Cast<NamespaceDeclaration>())
					{
						contextSymbol = symbols[0].Obj();
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
			baseType = declarators[0]->type;
		}

		if (TestToken(cursor, CppTokens::LBRACE))
		{
			auto decl = MakePtr<EnumDeclaration>();
			decl->enumClass = enumClass;
			decl->name = cppName;
			decl->baseType = baseType;

			auto contextSymbol = pa.context->CreateDeclSymbol(decl);
			ParsingArguments newPa(pa, contextSymbol);

			while (!TestToken(cursor, CppTokens::RBRACE))
			{
				auto enumItem = MakePtr<EnumItemDeclaration>();
				if (!ParseCppName(enumItem->name, cursor)) throw StopParsingException(cursor);
				contextSymbol->CreateDeclSymbol(enumItem);
				decl->items.Add(enumItem);

				if (TestToken(cursor, CppTokens::EQ))
				{
					enumItem->value = ParseExpr(newPa, false, cursor);
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
			auto forwardSymbol = pa.context->CreateDeclSymbol(decl);
			forwardSymbol->isForwardDeclaration = true;
			output.Add(decl);
			FindForwardDeclarationRoot<EnumDeclaration, ForwardEnumDeclaration>(pa.context, forwardSymbol, cursor);
		}
	}
	else if (TestToken(cursor, CppTokens::DECL_CLASS, false) || TestToken(cursor, CppTokens::DECL_STRUCT, false) || TestToken(cursor, CppTokens::DECL_UNION, false))
	{
		auto classType = CppClassType::Union;
		auto defaultAccessor = CppClassAccessor::Public;

		switch ((CppTokens)cursor->token.token)
		{
		case CppTokens::DECL_CLASS:
			classType = CppClassType::Class;
			defaultAccessor = CppClassAccessor::Private;
			break;
		case CppTokens::DECL_STRUCT:
			classType = CppClassType::Struct;
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
			auto forwardSymbol = pa.context->CreateDeclSymbol(decl);
			forwardSymbol->isForwardDeclaration = true;
			output.Add(decl);
			FindForwardDeclarationRoot<ClassDeclaration, ForwardClassDeclaration>(pa.context, forwardSymbol, cursor);
		}
		else
		{
			auto decl = MakePtr<ClassDeclaration>();
			decl->classType = classType;
			decl->name = cppName;
			auto contextSymbol = pa.context->CreateDeclSymbol(decl);
			output.Add(decl);
			FindForwardDeclarations<ClassDeclaration, ForwardClassDeclaration>(pa.context, contextSymbol, cursor);

			ParsingArguments declPa(pa, contextSymbol);

			if (TestToken(cursor, CppTokens::COLON))
			{
				while (!TestToken(cursor, CppTokens::LBRACE, false))
				{
					auto accessor = defaultAccessor;
					if (TestToken(cursor, CppTokens::PUBLIC))
					{
						accessor = CppClassAccessor::Public;
					}
					else if (TestToken(cursor, CppTokens::PROTECTED))
					{
						accessor = CppClassAccessor::Protected;
					}
					else if (TestToken(cursor, CppTokens::PRIVATE))
					{
						accessor = CppClassAccessor::Private;
					}

					List<Ptr<Declarator>> declarators;
					ParseDeclarator(declPa, DeclaratorRestriction::Zero, InitializerRestriction::Zero, cursor, declarators);
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
			auto accessor = defaultAccessor;
			while (true)
			{
				if (TestToken(cursor, CppTokens::PUBLIC))
				{
					accessor = CppClassAccessor::Public;
					RequireToken(cursor, CppTokens::COLON);
				}
				else if (TestToken(cursor, CppTokens::PROTECTED))
				{
					accessor = CppClassAccessor::Protected;
					RequireToken(cursor, CppTokens::COLON);
				}
				else if (TestToken(cursor, CppTokens::PRIVATE))
				{
					accessor = CppClassAccessor::Private;
					RequireToken(cursor, CppTokens::COLON);
				}
				else if(TestToken(cursor,CppTokens::RBRACE))
				{
					break;
				}
				else
				{
					List<Ptr<Declaration>> declarations;
					ParseDeclaration(declPa, cursor, declarations);
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

		bool trySpecialMethod = false;
		Ptr<ClassDeclaration> specialMethodParent;
		if (pa.context && pa.context->decls.Count() > 0)
		{
			if (specialMethodParent = pa.context->decls[0].Cast<ClassDeclaration>())
			{
#define TRY_TOKEN(TOKEN) if (TestToken(cursor, CppTokens::TOKEN, false)) trySpecialMethod = true; else

				TRY_TOKEN(LPARENTHESIS)
				TRY_TOKEN(OPERATOR)
				TRY_TOKEN(REVERT)
				TRY_TOKEN(__CDECL)
				TRY_TOKEN(__CLRCALL)
				TRY_TOKEN(__STDCALL)
				TRY_TOKEN(__FASTCALL)
				TRY_TOKEN(__THISCALL)
				TRY_TOKEN(__VECTORCALL)
				if (TestToken(cursor, specialMethodParent->name.name.Buffer(), false))
				{
					trySpecialMethod = true;
				}
#undef TRY_TOKEN
			}
		}

		auto methodType = CppMethodType::Function;
		if (trySpecialMethod)
		{
			auto oldCursor = cursor;
			try
			{
				ParseDeclarator(pa, nullptr, specialMethodParent.Obj(), DeclaratorRestriction::One, InitializerRestriction::Optional, cursor, declarators);

				auto& cppName = declarators[0]->name;
				switch (cppName.type)
				{
				case CppNameType::Normal:
					methodType = CppMethodType::Constructor;
					break;
				case CppNameType::Operator:
					methodType = CppMethodType::TypeConversion;
					break;
				case CppNameType::Destructor:
					methodType = CppMethodType::Destructor;
					break;
				}
				goto SUCCEEDED_IN_SPECIAL_METHOD;
			}
			catch (const StopParsingException&)
			{
				cursor = oldCursor;
			}
		}
		ParseDeclarator(pa, DeclaratorRestriction::Many, InitializerRestriction::Optional, cursor, declarators);
	SUCCEEDED_IN_SPECIAL_METHOD:

		for (vint i = 0; i < declarators.Count(); i++)
		{
			auto declarator = declarators[i];

			if (GetTypeWithoutMemberAndCC(declarator->type).Cast<FunctionType>())
			{
				if (i != 0)
				{
					throw StopParsingException(cursor);
				}

				bool decoratorAbstract = false;
				if (declarator->initializer)
				{
					if (declarator->initializer->initializerType != InitializerType::Equal) throw StopParsingException(cursor);
					if (declarator->initializer->arguments.Count() != 1) throw StopParsingException(cursor);

					auto expr = declarator->initializer->arguments[0].Cast<LiteralExpr>();
					if (!expr) throw StopParsingException(cursor);
					if (expr->tokens.Count() != 1) throw StopParsingException(cursor);
					if (expr->tokens[0].length!=1) throw StopParsingException(cursor);
					if (*expr->tokens[0].reading != L'0') throw StopParsingException(cursor);
					decoratorAbstract = true;
				}

#define FILL_FUNCTION(NAME)\
				NAME->name = declarator->name;\
				NAME->type = declarator->type;\
				NAME->methodType = methodType;\
				NAME->decoratorExtern = decoratorExtern;\
				NAME->decoratorFriend = decoratorFriend;\
				NAME->decoratorStatic = decoratorStatic;\
				NAME->decoratorVirtual = decoratorVirtual;\
				NAME->decoratorExplicit = decoratorExplicit;\
				NAME->decoratorInline = decoratorInline;\
				NAME->decoratorForceInline = decoratorForceInline;\
				NAME->decoratorAbstract = decoratorAbstract\

				Ptr<Stat> stat;
				if (TestToken(cursor, CppTokens::LBRACE, false))
				{
					stat = ParseStat(pa, cursor);
				}

				if (stat)
				{
					auto decl = MakePtr<FunctionDeclaration>();
					FILL_FUNCTION(decl);
					decl->statement = stat;
					auto contextSymbol = pa.context->CreateDeclSymbol(decl);
					output.Add(decl);
					return;
				}
				else
				{
					auto decl = MakePtr<ForwardFunctionDeclaration>();
					FILL_FUNCTION(decl);
					auto forwardSymbol = pa.context->CreateDeclSymbol(decl);
					forwardSymbol->isForwardDeclaration = true;
					output.Add(decl);
					RequireToken(cursor, CppTokens::SEMICOLON);
					return;
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
					auto forwardSymbol = pa.context->CreateDeclSymbol(decl);
					forwardSymbol->isForwardDeclaration = true;
					output.Add(decl);
					FindForwardDeclarationRoot<VariableDeclaration, ForwardVariableDeclaration>(pa.context, forwardSymbol, cursor);
				}
				else
				{
					auto decl = MakePtr<VariableDeclaration>();
					FILL_VARIABLE(decl);
					decl->initializer = declarator->initializer;
					auto contextSymbol = pa.context->CreateDeclSymbol(decl);
					output.Add(decl);
					FindForwardDeclarations<VariableDeclaration, ForwardVariableDeclaration>(pa.context, contextSymbol, cursor);
				}
#undef FILL_VARIABLE
			}
		}
		RequireToken(cursor, CppTokens::SEMICOLON);
	}
}

/***********************************************************************
BuildVariablesAndSymbols
***********************************************************************/

void BuildSymbols(const ParsingArguments& pa, List<Ptr<VariableDeclaration>>& varDecls)
{
	for (vint i = 0; i < varDecls.Count(); i++)
	{
		auto varDecl = varDecls[i];
		if (varDecl->name)
		{
			pa.context->CreateDeclSymbol(varDecl);
		}
	}
}

/***********************************************************************
BuildVariablesAndSymbols
***********************************************************************/

void BuildVariablesAndSymbols(const ParsingArguments& pa, List<Ptr<Declarator>>& declarators, List<Ptr<VariableDeclaration>>& varDecls, bool createSymbols)
{
	for (vint i = 0; i < declarators.Count(); i++)
	{
		auto declarator = declarators[i];

		auto varDecl = MakePtr<VariableDeclaration>();
		varDecl->type = declarator->type;
		varDecl->name = declarator->name;
		varDecl->initializer = declarator->initializer;
		varDecls.Add(varDecl);
	}

	if (createSymbols)
	{
		BuildSymbols(pa, varDecls);
	}
}