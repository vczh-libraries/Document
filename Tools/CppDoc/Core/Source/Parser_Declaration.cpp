#include "Parser.h"
#include "Ast_Type.h"
#include "Ast_Expr.h"
#include "Ast_Decl.h"

/***********************************************************************
FindForward
***********************************************************************/

template<typename TRoot, typename TForward>
struct ForwardPolicy
{
	static bool IsSameCategory(Symbol* symbol, Symbol* sibling)
	{
		return true;
	}
};

template<>
struct ForwardPolicy<FunctionDeclaration, ForwardFunctionDeclaration>
{
	static bool IsSameCategory(Symbol* symbol, Symbol* sibling)
	{
		bool inClass = symbol->parent->decls.Count() > 0 && symbol->parent->decls[0].Cast<ClassDeclaration>();

		auto f1 = symbol->decls[0].Cast<ForwardFunctionDeclaration>();
		auto f2 = sibling->decls[0].Cast<ForwardFunctionDeclaration>();
		auto t1 = f1->type;
		auto t2 = f2->type;

		if (inClass)
		{
			if (auto mt = t1.Cast<MemberType>())
			{
				t1 = mt->type;
			}
			if (auto mt = t2.Cast<MemberType>())
			{
				t2 = mt->type;
			}
		}
		return IsSameResolvedType(t1, t2);
	}
};

template<typename TRoot, typename TForward>
void SearchForwards(Symbol* scope, Symbol* symbol, Ptr<CppTokenCursor> cursor, Symbol*& root, List<Symbol*>& forwards)
{
	const auto& siblings = scope->children[symbol->name];
	for (vint i = 0; i < siblings.Count(); i++)
	{
		auto& sibling = siblings[i];
		bool sameCategory = ForwardPolicy<TRoot, TForward>::IsSameCategory(symbol, sibling.Obj());
		if (sibling->decls[0].Cast<TRoot>())
		{
			if (sameCategory)
			{
				if (root)
				{
					throw StopParsingException(cursor);
				}
				else
				{
					root = sibling.Obj();
				}
			}
		}
		else if (sibling->decls[0].Cast<TForward>())
		{
			if (sameCategory)
			{
				forwards.Add(sibling.Obj());
			}
		}
		else
		{
			throw StopParsingException(cursor);
		}
	}
}

template<typename TRoot, typename TForward>
void ConnectForwards(Symbol* scope, Symbol* symbol, Ptr<CppTokenCursor> cursor)
{
	Symbol* root = nullptr;
	List<Symbol*> forwards;
	SearchForwards<TRoot, TForward>(scope, symbol, cursor, root, forwards);

	for (vint i = 0; i < forwards.Count(); i++)
	{
		if (!forwards[i]->SetForwardDeclarationRoot(root))
		{
			throw StopParsingException(cursor);
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
		// namespace NAME { :: NAME ...} { DECLARATION ...}
		auto contextSymbol = pa.context;
		Ptr<NamespaceDeclaration> topDecl;
		Ptr<NamespaceDeclaration> contextDecl;

		while (cursor)
		{
			// create AST
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
				// ensure all other overloadings are namespaces, and merge the scope with them
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
		// enum [CLASS] NAME [: TYPE] ...
		bool enumClass = TestToken(cursor, CppTokens::DECL_CLASS);

		CppName cppName;
		if (!ParseCppName(cppName, cursor)) throw StopParsingException(cursor);

		Ptr<Type> baseType;
		if (TestToken(cursor, CppTokens::COLON))
		{
			baseType = ParseType(pa, cursor);
		}

		if (TestToken(cursor, CppTokens::LBRACE))
		{
			// ... { { IDENTIFIER [= EXPRESSION] ,... } };
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
			ConnectForwards<EnumDeclaration, ForwardEnumDeclaration>(pa.context, contextSymbol, cursor);
		}
		else
		{
			// ... ;
			RequireToken(cursor, CppTokens::SEMICOLON);
			auto decl = MakePtr<ForwardEnumDeclaration>();
			decl->enumClass = enumClass;
			decl->name = cppName;
			decl->baseType = baseType;
			auto forwardSymbol = pa.context->CreateDeclSymbol(decl);
			forwardSymbol->isForwardDeclaration = true;
			output.Add(decl);
			ConnectForwards<EnumDeclaration, ForwardEnumDeclaration>(pa.context, forwardSymbol, cursor);
		}
	}
	else if (TestToken(cursor, CppTokens::DECL_CLASS, false) || TestToken(cursor, CppTokens::DECL_STRUCT, false) || TestToken(cursor, CppTokens::DECL_UNION, false))
	{
		// [union | class | struct] NAME ...
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
			// ... ;
			auto decl = MakePtr<ForwardClassDeclaration>();
			decl->classType = classType;
			decl->name = cppName;
			auto forwardSymbol = pa.context->CreateDeclSymbol(decl);
			forwardSymbol->isForwardDeclaration = true;
			output.Add(decl);
			ConnectForwards<ClassDeclaration, ForwardClassDeclaration>(pa.context, forwardSymbol, cursor);
		}
		else
		{
			// ... [: { [public|protected|private] TYPE , ...} ]
			auto decl = MakePtr<ClassDeclaration>();
			decl->classType = classType;
			decl->name = cppName;
			auto contextSymbol = pa.context->CreateDeclSymbol(decl);
			output.Add(decl);
			ConnectForwards<ClassDeclaration, ForwardClassDeclaration>(pa.context, contextSymbol, cursor);

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

					auto type = ParseType(declPa, cursor);
					decl->baseTypes.Add({ accessor,type });

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

			// ... { { (public: | protected: | private: | DECLARATION) } };
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
		// parse declarators for functions and variables
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

		// prepare data structures for class members defined out of classes
		// non-null containingClass means this declaration is defined right inside a class
		// non-null containingClassForMember means this declaration is a class member defined out of the class
		List<Ptr<Declarator>> declarators;
		auto methodType = CppMethodType::Function;
		ClassDeclaration* containingClass = nullptr;
		ClassDeclaration* containingClassForMember = nullptr;

		{
			if (pa.context->decls.Count() > 0)
			{
				containingClass = pa.context->decls[0].Cast<ClassDeclaration>().Obj();
			}
			// get all declarators
			ParseDeclarator(pa, containingClass, true, DeclaratorRestriction::Many, InitializerRestriction::Optional, cursor, declarators);

			if (declarators.Count() > 0)
			{
				// a function declaration can only have one declarator
				auto declarator = declarators[0];
				if (GetTypeWithoutMemberAndCC(declarator->type).Cast<FunctionType>())
				{
					if (declarators.Count() != 1)
					{
						throw StopParsingException(cursor);
					}
				}

				// see if it is a class member declarator defined out of the class
				if (!containingClass)
				{
					if (declarator->containingClassSymbol)
					{
						containingClassForMember = declarator->containingClassSymbol->decls[0].Cast<ClassDeclaration>().Obj();
					}
				}

				if (declarator->type.Cast<MemberType>())
				{
					if (containingClass || !containingClassForMember)
					{
						throw StopParsingException(cursor);
					}
				}

				// adjust name type
				if (containingClass || containingClassForMember)
				{
					auto& cppName = declarators[0]->name;
					switch (cppName.type)
					{
					case CppNameType::Operator:
						if (cppName.tokenCount == 1)
						{
							methodType = CppMethodType::TypeConversion;
						}
						break;
					case CppNameType::Constructor:
						methodType = CppMethodType::Constructor;
						break;
					case CppNameType::Destructor:
						methodType = CppMethodType::Destructor;
						break;
					}
				}
			}
		}

		for (vint i = 0; i < declarators.Count(); i++)
		{
			auto declarator = declarators[i];

			if (auto type = GetTypeWithoutMemberAndCC(declarator->type).Cast<FunctionType>())
			{
				// for functions
				bool decoratorAbstract = false;

				// the only legal initializer is = 0
				if (declarator->initializer)
				{
					if (declarator->initializer->initializerType != InitializerType::Equal) throw StopParsingException(cursor);
					if (declarator->initializer->arguments.Count() != 1) throw StopParsingException(cursor);

					auto expr = declarator->initializer->arguments[0].Cast<LiteralExpr>();
					if (!expr) throw StopParsingException(cursor);
					if (expr->tokens.Count() != 1) throw StopParsingException(cursor);
					if (expr->tokens[0].length != 1) throw StopParsingException(cursor);
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

				bool hasStat = TestToken(cursor, CppTokens::LBRACE, false);

				auto context = containingClassForMember ? containingClassForMember->symbol : pa.context;
				if (hasStat)
				{
					// if there is a statement, then it is a function declaration
					auto decl = MakePtr<FunctionDeclaration>();
					FILL_FUNCTION(decl);
					{
						ParsingArguments statPa(pa, context);
						decl->statement = ParseStat(statPa, cursor);
					}
					auto contextSymbol = context->CreateDeclSymbol(decl);
					{
						ParsingArguments newPa(pa, contextSymbol);
						BuildSymbols(newPa, type->parameters);
					}
					output.Add(decl);
					ConnectForwards<FunctionDeclaration, ForwardFunctionDeclaration>(context, contextSymbol, cursor);

					// no ; after a function declaration
					return;
				}
				else
				{
					// if there is ;, then it is a forward function declaration
					if (containingClassForMember)
					{
						throw StopParsingException(cursor);
					}

					auto decl = MakePtr<ForwardFunctionDeclaration>();
					FILL_FUNCTION(decl);
					auto forwardSymbol = context->CreateDeclSymbol(decl);
					forwardSymbol->isForwardDeclaration = true;
					output.Add(decl);
					ConnectForwards<FunctionDeclaration, ForwardFunctionDeclaration>(context, forwardSymbol, cursor);
					RequireToken(cursor, CppTokens::SEMICOLON);
					return;
				}
#undef FILL_FUNCTION
			}
			else
			{
				// for variables, names should not be constructor names, destructor names, type conversion operator names, or other operator names
				if (declarator->name.type != CppNameType::Normal)
				{
					throw StopParsingException(cursor);
				}

#define FILL_VARIABLE(NAME)\
				NAME->name = declarator->name;\
				NAME->type = declarator->type;\
				NAME->decoratorExtern = decoratorExtern;\
				NAME->decoratorStatic = decoratorStatic;\
				NAME->decoratorMutable = decoratorMutable;\
				NAME->decoratorThreadLocal = decoratorThreadLocal;\
				NAME->decoratorRegister = decoratorRegister\

				auto context = containingClassForMember ? containingClassForMember->symbol : pa.context;
				if (decoratorExtern || (decoratorStatic && !declarator->initializer))
				{
					// if there is extern, or static without an initializer, then it is a forward variable declaration
					if (containingClassForMember)
					{
						throw StopParsingException(cursor);
					}

					auto decl = MakePtr<ForwardVariableDeclaration>();
					FILL_VARIABLE(decl);
					auto forwardSymbol = context->CreateDeclSymbol(decl);
					forwardSymbol->isForwardDeclaration = true;
					output.Add(decl);
					ConnectForwards<VariableDeclaration, ForwardVariableDeclaration>(context, forwardSymbol, cursor);
				}
				else
				{
					// it is a variable declaration
					auto decl = MakePtr<VariableDeclaration>();
					FILL_VARIABLE(decl);
					decl->initializer = declarator->initializer;
					auto contextSymbol = context->CreateDeclSymbol(decl);
					output.Add(decl);

					ConnectForwards<VariableDeclaration, ForwardVariableDeclaration>(context, contextSymbol, cursor);
				}
#undef FILL_VARIABLE
			}
		}

		// ; is required after any forward function declaration, forward function declaration, or variable declaration
		RequireToken(cursor, CppTokens::SEMICOLON);
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
		varDecls.Add(varDecl);
	}
}

/***********************************************************************
BuildSymbols
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

void BuildVariablesAndSymbols(const ParsingArguments& pa, List<Ptr<Declarator>>& declarators, List<Ptr<VariableDeclaration>>& varDecls)
{
	BuildVariables(declarators, varDecls);
	BuildSymbols(pa, varDecls);
}