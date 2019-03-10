#include "Parser.h"
#include "Ast_Type.h"
#include "Ast_Expr.h"
#include "Ast_Decl.h"

/***********************************************************************
FindForward
***********************************************************************/

Symbol* SearchForFunctionWithSameSignature(Symbol* context, Ptr<ForwardFunctionDeclaration> decl, Ptr<CppTokenCursor>& cursor)
{
	bool inClass = context->definition.Cast<ClassDeclaration>();

	vint index = context->children.Keys().IndexOf(decl->name.name);
	if (index == -1) return nullptr;

	auto& symbols = context->children.GetByIndex(index);
	for (vint i = 0; i < symbols.Count(); i++)
	{
		auto symbol = symbols[i].Obj();
		if (symbol->kind == symbol_component::SymbolKind::Function)
		{
			auto declToCompare = symbol->GetAnyForwardDecl<ForwardFunctionDeclaration>();

			auto t1 = decl->type;
			auto t2 = declToCompare->type;
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
			if (IsSameResolvedType(t1, t2))
			{
				return symbol;
			}
		}
		else
		{
			throw StopParsingException(cursor);
		}
	}
	return nullptr;
}

/***********************************************************************
ParseTemplateSpec
***********************************************************************/

using TemplateSpecResult = Tuple<Ptr<Symbol>, Ptr<TemplateSpec>>;

TemplateSpecResult ParseTemplateSpec(const ParsingArguments& pa, Ptr<CppTokenCursor>& cursor)
{
	RequireToken(cursor, CppTokens::DECL_TEMPLATE);
	RequireToken(cursor, CppTokens::LT);

	auto symbol = MakePtr<Symbol>();
	symbol->parent = pa.context;
	auto newPa = pa.WithContext(symbol.Obj());

	auto spec = MakePtr<TemplateSpec>();
	while (!TestToken(cursor, CppTokens::GT))
	{
		TemplateSpec::Argument argument;
		if (TestToken(cursor, CppTokens::DECL_TEMPLATE, false))
		{
			argument.templateSpec = ParseTemplateSpec(newPa, cursor).f1;
		}

		if (TestToken(cursor, CppTokens::TYPENAME) || TestToken(cursor, CppTokens::DECL_CLASS))
		{
			argument.argumentType = CppTemplateArgumentType::Type;
			if (ParseCppName(argument.name, cursor))
			{
				if (symbol->children.Keys().Contains(argument.name.name))
				{
					throw StopParsingException(cursor);
				}
				auto argumentSymbol = MakePtr<Symbol>();
				argumentSymbol->kind = symbol_component::SymbolKind::GenericTypeArgument;
				argumentSymbol->name = argument.name.name;
				argumentSymbol->evaluation.Allocate();
				argumentSymbol->evaluation.Get().Add(pa.tsys->GenericArgOf(argumentSymbol.Obj()));
				symbol->children.Add(argumentSymbol->name, argumentSymbol);
			}

			if (TestToken(cursor, CppTokens::EQ))
			{
				argument.type = ParseType(newPa, cursor);
			}
		}
		else
		{
			if (argument.templateSpec)
			{
				throw StopParsingException(cursor);
			}

			auto declarator = ParseNonMemberDeclarator(newPa, pda_VarInit(), cursor);
			argument.argumentType = CppTemplateArgumentType::Value;
			argument.name = declarator->name;
			argument.type = declarator->type;
			if (declarator->initializer)
			{
				if (declarator->initializer->initializerType != CppInitializerType::Equal)
				{
					throw StopParsingException(cursor);
				}
				argument.expr = declarator->initializer->arguments[0];
			}
			if (argument.name)
			{
				if (symbol->children.Keys().Contains(argument.name.name))
				{
					throw StopParsingException(cursor);
				}
				auto argumentSymbol = MakePtr<Symbol>();
				argumentSymbol->kind = symbol_component::SymbolKind::GenericValueArgument;
				argumentSymbol->name = argument.name.name;
				argumentSymbol->evaluation.Allocate();
				TypeToTsys(newPa, argument.type, argumentSymbol->evaluation.Get());
				if (argumentSymbol->evaluation.Get().Count() == 0)
				{
					throw StopParsingException(cursor);
				}
				argumentSymbol->evaluation.Get().Add(pa.tsys->GenericArgOf(argumentSymbol.Obj()));
				symbol->children.Add(argumentSymbol->name, argumentSymbol);
			}
		}
		spec->arguments.Add(argument);

		if (!TestToken(cursor, CppTokens::COMMA))
		{
			RequireToken(cursor, CppTokens::GT);
			break;
		}
	}

	return { symbol,spec };
}

/***********************************************************************
ParseDeclaration_<NAMESPACE>
***********************************************************************/

void ParseDeclaration_Namespace(const ParsingArguments& pa, Ptr<CppTokenCursor>& cursor, List<Ptr<Declaration>>& output)
{
	RequireToken(cursor, CppTokens::DECL_NAMESPACE);
	if (TestToken(cursor, CppTokens::LBRACE))
	{
		// namespace { DECLARATION ...}
		// ignore it and add everything to its parent
		while (!TestToken(cursor, CppTokens::RBRACE))
		{
			ParseDeclaration(pa, cursor, output);
		}
	}
	else
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
				contextSymbol = contextSymbol->AddForwardDeclToSymbol(decl, symbol_component::SymbolKind::Namespace);
				if (!contextSymbol)
				{
					throw StopParsingException(cursor);
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

		auto newPa = pa.WithContext(contextSymbol);
		while (!TestToken(cursor, CppTokens::RBRACE))
		{
			ParseDeclaration(newPa, cursor, contextDecl->decls);
		}

		output.Add(topDecl);
	}
}

/***********************************************************************
ParseDeclaration_<ENUM>
***********************************************************************/

Ptr<EnumDeclaration> ParseDeclaration_Enum_NotConsumeSemicolon(const ParsingArguments& pa, bool forTypeDef, Ptr<CppTokenCursor>& cursor, List<Ptr<Declaration>>& output)
{
	RequireToken(cursor, CppTokens::DECL_ENUM);
	// enum [CLASS] NAME [: TYPE] ...
	bool enumClass = TestToken(cursor, CppTokens::DECL_CLASS);

	CppName cppName;
	bool isAnonymous = false;
	if (!ParseCppName(cppName, cursor))
	{
		if (enumClass)
		{
			throw StopParsingException(cursor);
		}
		else
		{
			isAnonymous = true;
			cppName.name = L"<anonymous>" + itow(pa.tsys->AllocateAnonymousCounter());
			cppName.type = CppNameType::Normal;
		}
	}

	Ptr<Type> baseType;
	if (TestToken(cursor, CppTokens::COLON))
	{
		baseType = ParseType(pa, cursor);
	}

	if (TestToken(cursor, CppTokens::SEMICOLON, false))
	{
		if (forTypeDef || isAnonymous)
		{
			throw StopParsingException(cursor);
		}

		// ... ;
		auto decl = MakePtr<ForwardEnumDeclaration>();
		decl->enumClass = enumClass;
		decl->name = cppName;
		decl->baseType = baseType;
		output.Add(decl);

		if (!pa.context->AddForwardDeclToSymbol(decl, symbol_component::SymbolKind::Enum))
		{
			throw StopParsingException(cursor);
		}
		return nullptr;
	}
	else
	{
		// ... { { IDENTIFIER [= EXPRESSION] ,... } };
		auto decl = MakePtr<EnumDeclaration>();
		decl->enumClass = enumClass;
		decl->name = cppName;
		decl->baseType = baseType;

		auto contextSymbol = pa.context->AddDeclToSymbol(decl, symbol_component::SymbolKind::Enum);
		if (!contextSymbol)
		{
			throw StopParsingException(cursor);
		}
		auto newPa = pa.WithContext(contextSymbol);

		RequireToken(cursor, CppTokens::LBRACE);
		while (!TestToken(cursor, CppTokens::RBRACE))
		{
			auto enumItem = MakePtr<EnumItemDeclaration>();
			if (!ParseCppName(enumItem->name, cursor)) throw StopParsingException(cursor);
			decl->items.Add(enumItem);
			if (!contextSymbol->AddDeclToSymbol(enumItem, symbol_component::SymbolKind::EnumItem))
			{
				throw StopParsingException(cursor);
			}

			if (!enumClass)
			{
				if (pa.context->children.Keys().Contains(enumItem->name.name))
				{
					throw StopParsingException(cursor);
				}
				pa.context->children.Add(enumItem->name.name, contextSymbol->children[enumItem->name.name][0]);
			}

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

		output.Add(decl);
		return decl;
	}
}

/***********************************************************************
ParseDeclaration_<CLASS / STRUCT / UNION>
***********************************************************************/

Ptr<ClassDeclaration> ParseDeclaration_Class_NotConsumeSemicolon(const ParsingArguments& pa, bool forTypeDef, Ptr<CppTokenCursor>& cursor, List<Ptr<Declaration>>& output)
{
	// [union | class | struct] NAME ...
	auto classType = CppClassType::Union;
	auto symbolKind = symbol_component::SymbolKind::Union;
	auto defaultAccessor = CppClassAccessor::Public;

	switch ((CppTokens)cursor->token.token)
	{
	case CppTokens::DECL_CLASS:
		classType = CppClassType::Class;
		symbolKind = symbol_component::SymbolKind::Class;
		defaultAccessor = CppClassAccessor::Private;
		break;
	case CppTokens::DECL_STRUCT:
		classType = CppClassType::Struct;
		symbolKind = symbol_component::SymbolKind::Struct;
		break;
	case CppTokens::DECL_UNION:
		classType = CppClassType::Union;
		symbolKind = symbol_component::SymbolKind::Union;
		break;
	default:
		throw StopParsingException(cursor);
	}
	cursor = cursor->Next();

	CppName cppName;
	bool isAnonymous = false;
	if (!ParseCppName(cppName, cursor))
	{
		isAnonymous = true;
		cppName.name = L"<anonymous>" + itow(pa.tsys->AllocateAnonymousCounter());
		cppName.type = CppNameType::Normal;
	}

	if (TestToken(cursor, CppTokens::SEMICOLON, false))
	{
		if (forTypeDef || isAnonymous)
		{
			throw StopParsingException(cursor);
		}

		// ... ;
		auto decl = MakePtr<ForwardClassDeclaration>();
		decl->classType = classType;
		decl->name = cppName;
		output.Add(decl);

		if (!pa.context->AddForwardDeclToSymbol(decl, symbolKind))
		{
			throw StopParsingException(cursor);
		}
		return nullptr;
	}
	else
	{
		// ... [: { [public|protected|private] TYPE , ...} ]
		auto decl = MakePtr<ClassDeclaration>();
		decl->classType = classType;
		decl->name = cppName;
		vint declIndex = output.Add(decl);

		auto contextSymbol = pa.context->AddDeclToSymbol(decl, symbolKind);
		if (!contextSymbol)
		{
			throw StopParsingException(cursor);
		}

		auto declPa = pa.WithContext(contextSymbol);

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
			else if (TestToken(cursor, CppTokens::RBRACE))
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

		if (!forTypeDef && isAnonymous && TestToken(cursor, CppTokens::SEMICOLON, false))
		{
			// class{ struct{ ... }; }
			// if an anonymous class is not defined after a typedef, and there is no variable declaration after the class
			// then this should be a nested anonymous class inside another class
			// in this case, this class should not have base types and non-public members
			// and all members should be either variables or nested anonymous classes
			// so a NestedAnonymousClassDeclaration is created, copy all members to it, and move all symbols to pa.context, which is the parent class declaration

			if (!pa.context->definition.Cast<ClassDeclaration>())
			{
				throw StopParsingException(cursor);
			}

			if (decl->baseTypes.Count() > 0)
			{
				throw StopParsingException(cursor);
			}

			auto nestedDecl = MakePtr<NestedAnonymousClassDeclaration>();
			nestedDecl->classType = classType;
			output[declIndex] = nestedDecl;

			for (vint i = 0; i < decl->decls.Count(); i++)
			{
				auto pair = decl->decls[i];
				if (pair.f0 != CppClassAccessor::Public)
				{
					throw StopParsingException(cursor);
				}
				if (!pair.f1.Cast<VariableDeclaration>() && !pair.f1.Cast<NestedAnonymousClassDeclaration>())
				{
					throw StopParsingException(cursor);
				}

				nestedDecl->decls.Add(pair.f1);
			}

			for (vint i = 0; i < contextSymbol->children.Count(); i++)
			{
				// variable doesn't override, so here just look at the first symbol
				auto child = contextSymbol->children.GetByIndex(i)[0];
				if (pa.context->children.Keys().Contains(child->name))
				{
					throw StopParsingException(cursor);
				}
				child->parent = pa.context;
				pa.context->children.Add(child->name, child);
			}
			pa.context->children.Remove(contextSymbol->name, contextSymbol);

			return nullptr;
		}

		if (classType != CppClassType::Union)
		{
			GenerateMembers(declPa, contextSymbol);
		}
		return decl;
	}
}

/***********************************************************************
ParseDeclaration_<USING>
***********************************************************************/

void ParseDeclaration_Using(const ParsingArguments& pa, Ptr<CppTokenCursor>& cursor, List<Ptr<Declaration>>& output)
{
	RequireToken(cursor, CppTokens::DECL_USING);
	if (TestToken(cursor, CppTokens::DECL_NAMESPACE))
	{
		// using namespace TYPE;
		auto decl = MakePtr<UsingNamespaceDeclaration>();
		decl->ns = ParseType(pa, cursor);
		output.Add(decl);
		RequireToken(cursor, CppTokens::SEMICOLON);

		if (auto resolvableType = decl->ns.Cast<ResolvableType>())
		{
			if (!resolvableType->resolving) throw StopParsingException(cursor);
			if (resolvableType->resolving->resolvedSymbols.Count() != 1) throw StopParsingException(cursor);
			auto symbol = resolvableType->resolving->resolvedSymbols[0];

			switch (symbol->kind)
			{
			case symbol_component::SymbolKind::Namespace:
				{
					if (pa.context && !(pa.context->usingNss.Contains(symbol)))
					{
						pa.context->usingNss.Add(symbol);
					}
				}
				break;
			default:
				throw StopParsingException(cursor);
			}
		}
		else
		{
			throw StopParsingException(cursor);
		}
	}
	else
	{
		auto oldCursor = cursor;
		{
			// using NAME = TYPE;
			CppName cppName;
			if (!ParseCppName(cppName, cursor) || !TestToken(cursor, CppTokens::EQ))
			{
				cursor = oldCursor;
				goto SKIP_TYPE_ALIAS;
			}

			auto decl = MakePtr<UsingDeclaration>();
			decl->name = cppName;
			decl->type = ParseType(pa, cursor);
			RequireToken(cursor, CppTokens::SEMICOLON);
			output.Add(decl);

			if (!pa.context->AddDeclToSymbol(decl, symbol_component::SymbolKind::TypeAlias))
			{
				throw StopParsingException(cursor);
			}
			return;
		}
	SKIP_TYPE_ALIAS:
		{
			// using TYPE[::NAME];
			auto decl = MakePtr<UsingSymbolDeclaration>();
			try
			{
				decl->expr = ParsePrimitiveExpr(pa, cursor);
			}
			catch (const StopParsingException&)
			{
				cursor = oldCursor;
				decl->type = ParseType(pa, cursor);
			}
			RequireToken(cursor, CppTokens::SEMICOLON);
			output.Add(decl);

			Ptr<Resolving> resolving;
			if (decl->expr)
			{
				if (auto resolvableExpr = decl->expr.Cast<ResolvableExpr>())
				{
					resolving = resolvableExpr->resolving;
				}
			}
			else if (decl->type)
			{
				if (auto resolvableType = decl->type.Cast<ResolvableType>())
				{
					resolving = resolvableType->resolving;
				}
			}

			if (!resolving) throw StopParsingException(cursor);
			for (vint i = 0; i < resolving->resolvedSymbols.Count(); i++)
			{
				auto rawSymbolPtr = resolving->resolvedSymbols[i];
				auto& siblings = rawSymbolPtr->parent->children[rawSymbolPtr->name];
				auto symbol = siblings[siblings.IndexOf(rawSymbolPtr)];

				switch (symbol->kind)
				{
				case symbol_component::SymbolKind::Enum:
				case symbol_component::SymbolKind::EnumItem:
				case symbol_component::SymbolKind::Class:
				case symbol_component::SymbolKind::Struct:
				case symbol_component::SymbolKind::Union:
				case symbol_component::SymbolKind::TypeAlias:
				case symbol_component::SymbolKind::Variable:
					{
						if (pa.context->children.Keys().Contains(symbol->name))
						{
							throw StopParsingException(cursor);
						}
						pa.context->children.Add(symbol->name, symbol);
					}
					break;
				case symbol_component::SymbolKind::Function:
					{
						vint index = pa.context->children.Keys().IndexOf(symbol->name);
						if (index != -1 && pa.context->children.GetByIndex(index)[0]->kind != symbol_component::SymbolKind::Function)
						{
							throw StopParsingException(cursor);
						}
						pa.context->children.Add(symbol->name, symbol);
					}
					break;
				default:
					throw StopParsingException(cursor);
				}
			}
		}
	}
}

/***********************************************************************
ParseDeclaration_<TYPEDEF>
***********************************************************************/

void ParseDeclaration_Typedef(const ParsingArguments& pa, Ptr<CppTokenCursor>& cursor, List<Ptr<Declaration>>& output)
{
	RequireToken(cursor, CppTokens::DECL_TYPEDEF);
	List<Ptr<Declarator>> declarators;
	Ptr<IdType> createdType;

	if (TestToken(cursor, CppTokens::DECL_CLASS, false) || TestToken(cursor, CppTokens::DECL_STRUCT, false) || TestToken(cursor, CppTokens::DECL_UNION, false))
	{
		// typedef class{} ...;
		auto classDecl = ParseDeclaration_Class_NotConsumeSemicolon(pa, true, cursor, output);

		createdType = MakePtr<IdType>();
		createdType->name = classDecl->name;
		createdType->resolving = MakePtr<Resolving>();
		createdType->resolving->resolvedSymbols.Add(classDecl->symbol);
		ParseNonMemberDeclarator(pa, pda_Typedefs(), createdType, cursor, declarators);
	}
	else if (TestToken(cursor, CppTokens::DECL_ENUM, false))
	{
		// typedef enum{} ...;
		auto enumDecl = ParseDeclaration_Enum_NotConsumeSemicolon(pa, true, cursor, output);

		createdType = MakePtr<IdType>();
		createdType->name = enumDecl->name;
		createdType->resolving = MakePtr<Resolving>();
		createdType->resolving->resolvedSymbols.Add(enumDecl->symbol);
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
		auto decl = MakePtr<UsingDeclaration>();
		decl->name = declarators[i]->name;
		decl->type = declarators[i]->type;
		output.Add(decl);

		if (!pa.context->AddDeclToSymbol(decl, symbol_component::SymbolKind::TypeAlias))
		{
			throw StopParsingException(cursor);
		}
	}
}

/***********************************************************************
ParseDeclaration_FuncVar
***********************************************************************/

#define FUNCVAR_DECORATORS(F)\
	F(DECL_EXTERN, Extern)\
	F(STATIC, Static)\
	F(MUTABLE, Mutable)\
	F(THREAD_LOCAL, ThreadLocal)\
	F(REGISTER, Register)\
	F(VIRTUAL, Virtual)\
	F(EXPLICIT, Explicit)\
	F(INLINE, Inline)\
	F(__INLINE, __Inline)\
	F(__FORCEINLINE, __ForceInline)\

#define FUNCVAR_DECORATORS_FOR_FUNCTION(F)\
	F(Extern)\
	F(Friend)\
	F(Static)\
	F(Virtual)\
	F(Explicit)\
	F(Inline)\
	F(__Inline)\
	F(__ForceInline)\
	F(Abstract)\
	F(Default)\
	F(Delete)\

#define FUNCVAR_DECORATORS_FOR_VARIABLE(F)\
	F(Extern)\
	F(Static)\
	F(Mutable)\
	F(ThreadLocal)\
	F(Register)\

#define FUNCVAR_PARAMETER(NAME) bool decorator##NAME,
#define FUNCVAR_ARGUMENT(NAME) decorator##NAME,
#define FUNCVAR_FILL_DECLARATOR(NAME) decl->decorator##NAME = decorator##NAME;

/////////////////////////////////////////////////////////////////////////////////////////////

void ParseDeclaration_Function(
	const ParsingArguments& pa,
	Ptr<Declarator> declarator,
	Ptr<FunctionType> funcType,
	FUNCVAR_DECORATORS_FOR_FUNCTION(FUNCVAR_PARAMETER)
	CppMethodType methodType,
	ClassDeclaration* containingClass,
	ClassDeclaration* containingClassForMember,
	Ptr<CppTokenCursor>& cursor,
	List<Ptr<Declaration>>& output
)
{
#define FILL_FUNCTION\
	decl->name = declarator->name;\
	decl->type = declarator->type;\
	decl->methodType = methodType;\
	FUNCVAR_DECORATORS_FOR_FUNCTION(FUNCVAR_FILL_DECLARATOR)\
	decl->needResolveTypeFromStatement = needResolveTypeFromStatement\

	bool hasStat = TestToken(cursor, CppTokens::COLON, false) || TestToken(cursor, CppTokens::LBRACE, false);
	bool needResolveTypeFromStatement = IsPendingType(funcType->returnType) && (!funcType->decoratorReturnType || IsPendingType(funcType->decoratorReturnType));
	if (needResolveTypeFromStatement && !hasStat)
	{
		throw StopParsingException(cursor);
	}

	auto context = containingClassForMember ? containingClassForMember->symbol : pa.context;
	if (hasStat)
	{
		// if there is a statement, then it is a function declaration
		auto decl = MakePtr<FunctionDeclaration>();
		FILL_FUNCTION;
		output.Add(decl);

		auto contextSymbol = context->CreateDeclSymbol(decl, SearchForFunctionWithSameSignature(context, decl, cursor), symbol_component::SymbolKind::Function);

		if (containingClass || containingClassForMember)
		{
			auto methodCache = MakePtr<symbol_component::MethodCache>();
			contextSymbol->methodCache = methodCache;

			methodCache->classSymbol = containingClass ? containingClass->symbol : containingClassForMember->symbol;
			methodCache->funcSymbol = contextSymbol;
			methodCache->classDecl = methodCache->classSymbol->definition.Cast<ClassDeclaration>();
			methodCache->funcDecl = decl;

			TsysCV cv;
			cv.isGeneralConst = funcType->qualifierConstExpr || funcType->qualifierConst;
			cv.isVolatile = funcType->qualifierVolatile;
			methodCache->thisType = pa.tsys->DeclOf(methodCache->classSymbol)->CVOf(cv)->PtrOf();
		}
		{
			auto newPa = pa.WithContext(contextSymbol);
			BuildSymbols(newPa, funcType->parameters, cursor);
		}
		// delay parse the statement
		{
			decl->delayParse = MakePtr<DelayParse>();
			decl->delayParse->pa = pa.WithContext(contextSymbol);
			cursor->Clone(decl->delayParse->reader, decl->delayParse->begin);

			vint counter = 0;
			while (true)
			{
				if (TestToken(cursor, CppTokens::LBRACE))
				{
					counter++;
				}
				else if (TestToken(cursor, CppTokens::RBRACE))
				{
					counter--;
					if (counter == 0)
					{
						if (cursor)
						{
							decl->delayParse->end = cursor->token;
						}
						else
						{
							memset(&decl->delayParse->end, 0, sizeof(RegexToken));
						}
						break;
					}
				}
				else
				{
					SkipToken(cursor);
					if (!cursor)
					{
						throw StopParsingException(cursor);
					}
				}
			}
		}
	}
	else
	{
		// if there is ;, then it is a forward function declaration
		if (containingClassForMember)
		{
			throw StopParsingException(cursor);
		}

		auto decl = MakePtr<ForwardFunctionDeclaration>();
		FILL_FUNCTION;
		output.Add(decl);
		RequireToken(cursor, CppTokens::SEMICOLON);
		context->CreateForwardDeclSymbol(decl, SearchForFunctionWithSameSignature(context, decl, cursor), symbol_component::SymbolKind::Function);
	}
#undef FILL_FUNCTION
}

/////////////////////////////////////////////////////////////////////////////////////////////

void ParseDeclaration_Variable(
	const ParsingArguments& pa,
	Ptr<Declarator> declarator,
	FUNCVAR_DECORATORS_FOR_VARIABLE(FUNCVAR_PARAMETER)
	ClassDeclaration* containingClassForMember,
	Ptr<CppTokenCursor>& cursor,
	List<Ptr<Declaration>>& output
)
{
	// for variables, names should not be constructor names, destructor names, type conversion operator names, or other operator names
	if (declarator->name.type != CppNameType::Normal)
	{
		throw StopParsingException(cursor);
	}

#define FILL_VARIABLE\
	decl->name = declarator->name;\
	decl->type = declarator->type;\
	FUNCVAR_DECORATORS_FOR_VARIABLE(FUNCVAR_FILL_DECLARATOR)\
	decl->needResolveTypeFromInitializer = needResolveTypeFromInitializer\

	bool needResolveTypeFromInitializer = IsPendingType(declarator->type);
	if (needResolveTypeFromInitializer && (!declarator->initializer || declarator->initializer->initializerType != CppInitializerType::Equal))
	{
		throw StopParsingException(cursor);
	}

	auto context = containingClassForMember ? containingClassForMember->symbol : pa.context;
	if (decoratorExtern || (decoratorStatic && !declarator->initializer))
	{
		// if there is extern, or static without an initializer, then it is a forward variable declaration
		if (containingClassForMember)
		{
			throw StopParsingException(cursor);
		}

		auto decl = MakePtr<ForwardVariableDeclaration>();
		FILL_VARIABLE;
		output.Add(decl);

		if (!context->AddForwardDeclToSymbol(decl, symbol_component::SymbolKind::Variable))
		{
			throw StopParsingException(cursor);
		}
	}
	else
	{
		// it is a variable declaration
		auto decl = MakePtr<VariableDeclaration>();
		FILL_VARIABLE;
		decl->initializer = declarator->initializer;
		output.Add(decl);

		if (!context->AddDeclToSymbol(decl, symbol_component::SymbolKind::Variable))
		{
			throw StopParsingException(cursor);
		}
	}
#undef FILL_VARIABLE
}

/////////////////////////////////////////////////////////////////////////////////////////////

void ParseDeclaration_FuncVar(const ParsingArguments& pa, bool decoratorFriend, Ptr<CppTokenCursor>& cursor, List<Ptr<Declaration>>& output)
{
	// parse declarators for functions and variables

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

	// prepare data structures for class members defined out of classes
	// non-null containingClass means this declaration is defined right inside a class
	// non-null containingClassForMember means this declaration is a class member defined out of the class
	List<Ptr<Declarator>> declarators;
	auto methodType = CppMethodType::Function;
	ClassDeclaration* containingClass = pa.context->definition.Cast<ClassDeclaration>().Obj();
	ClassDeclaration* containingClassForMember = nullptr;

	// get all declarators
	{
		auto pda = pda_Decls();
		pda.containingClass = containingClass;
		ParseMemberDeclarator(pa, pda, cursor, declarators);
	}

	Ptr<FunctionType> funcType;
	if (declarators.Count() > 0)
	{
		// a function declaration can only have one declarator
		auto declarator = declarators[0];
		funcType = GetTypeWithoutMemberAndCC(declarator->type).Cast<FunctionType>();
		if (funcType)
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
				containingClassForMember = declarator->containingClassSymbol->definition.Cast<ClassDeclaration>().Obj();
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

	if (funcType)
	{
		// for functions
		bool decoratorAbstract = false;
		bool decoratorDefault = false;
		bool decoratorDelete = false;

		if (TestToken(cursor, CppTokens::EQ))
		{
			if (TestToken(cursor, CppTokens::STAT_DEFAULT))
			{
				decoratorDefault = true;
			}
			else if (TestToken(cursor, CppTokens::DELETE))
			{
				decoratorDelete = true;
			}
			else
			{
				RequireToken(cursor, L"0");
				decoratorAbstract = true;
			}
		}

		ParseDeclaration_Function(
			pa,
			declarators[0],
			funcType,
			FUNCVAR_DECORATORS_FOR_FUNCTION(FUNCVAR_ARGUMENT)
			methodType,
			containingClass,
			containingClassForMember,
			cursor,
			output
		);
	}
	else
	{
		for (vint i = 0; i < declarators.Count(); i++)
		{
			ParseDeclaration_Variable(
				pa,
				declarators[i],
				FUNCVAR_DECORATORS_FOR_VARIABLE(FUNCVAR_ARGUMENT)
				containingClassForMember,
				cursor,
				output
			);
		}
		RequireToken(cursor, CppTokens::SEMICOLON);
	}
}

#undef FUNCVAR_DECORATORS
#undef FUNCVAR_DECORATORS_FOR_FUNCTION
#undef FUNCVAR_DECORATORS_FOR_VARIABLE
#undef FUNCVAR_FILL_DECLARATOR
#undef FUNCVAR_PARAMETER
#undef FUNCVAR_ARGUMENT

/***********************************************************************
ParseDeclaration
***********************************************************************/

void ParseVariablesFollowedByDecl_NotConsumeSemicolon(const ParsingArguments& pa, Ptr<Declaration> decl, Ptr<CppTokenCursor>& cursor, List<Ptr<Declaration>>& output)
{
	auto type = MakePtr<IdType>();
	type->name = decl->name;
	type->resolving = MakePtr<Resolving>();
	type->resolving->resolvedSymbols.Add(decl->symbol);

	List<Ptr<Declarator>> declarators;
	ParseNonMemberDeclarator(pa, pda_Typedefs(), type, cursor, declarators);

	for (vint i = 0; i < declarators.Count(); i++)
	{
		ParseDeclaration_Variable(
			pa,
			declarators[i],
			false, false, false, false, false,
			nullptr,
			cursor,
			output
		);
	}
}

void ParseDeclaration(const ParsingArguments& pa, Ptr<CppTokenCursor>& cursor, List<Ptr<Declaration>>& output)
{
	while (SkipSpecifiers(cursor));
	TemplateSpecResult spec;
	if (TestToken(cursor, CppTokens::DECL_TEMPLATE, false))
	{
		spec = ParseTemplateSpec(pa, cursor);
		// skip the whole thing for now

		int counter = 0;
		while (cursor)
		{
			if (TestToken(cursor, CppTokens::LBRACE))
			{
				counter++;
			}
			else if (TestToken(cursor, CppTokens::RBRACE))
			{
				counter--;
				if (counter == 0)
				{
					TestToken(cursor, CppTokens::SEMICOLON);
					return;
				}
			}
			else
			{
				SkipToken(cursor);
			}
		}
	}

	bool decoratorFriend = TestToken(cursor, CppTokens::DECL_FRIEND);
	bool cStyleTypeReference = false;

	// check if the following `struct Id` is trying to reference a type instead of to define a type
	if (TestToken(cursor, CppTokens::DECL_ENUM, false) || TestToken(cursor, CppTokens::DECL_CLASS, false) || TestToken(cursor, CppTokens::DECL_STRUCT, false) || TestToken(cursor, CppTokens::DECL_UNION, false))
	{
		auto oldCursor = cursor;
		SkipToken(cursor);
		CppName name;
		if (ParseCppName(name, cursor))
		{
			if (!TestToken(cursor, CppTokens::SEMICOLON) && !TestToken(cursor, CppTokens::LT) && !TestToken(cursor, CppTokens::COLON) && !TestToken(cursor, CppTokens::LBRACE))
			{
				cStyleTypeReference = true;
			}
		}
		cursor = oldCursor;
	}

	{
		auto oldCursor = cursor;
		if (TestToken(cursor, CppTokens::DECL_EXTERN) && TestToken(cursor, CppTokens::STRING))
		{
			// extern "C"
			// ignore it and add everything to its parent
			if (TestToken(cursor, CppTokens::LBRACE))
			{
				while (!TestToken(cursor, CppTokens::RBRACE))
				{
					ParseDeclaration(pa, cursor, output);
				}
			}
			else
			{
				ParseDeclaration(pa, cursor, output);
			}
			return;
		}
		else
		{
			cursor = oldCursor;
		}
	}

	if (TestToken(cursor, CppTokens::DECL_NAMESPACE, false))
	{
		ParseDeclaration_Namespace(pa, cursor, output);
	}
	else if (!cStyleTypeReference && TestToken(cursor, CppTokens::DECL_ENUM, false))
	{
		if (auto enumDecl = ParseDeclaration_Enum_NotConsumeSemicolon(pa, false, cursor, output))
		{
			if (!TestToken(cursor, CppTokens::SEMICOLON, false))
			{
				ParseVariablesFollowedByDecl_NotConsumeSemicolon(pa, enumDecl, cursor, output);
			}
		}
		RequireToken(cursor, CppTokens::SEMICOLON);
	}
	else if (!cStyleTypeReference && (TestToken(cursor, CppTokens::DECL_CLASS, false) || TestToken(cursor, CppTokens::DECL_STRUCT, false) || TestToken(cursor, CppTokens::DECL_UNION, false)))
	{
		if (auto classDecl = ParseDeclaration_Class_NotConsumeSemicolon(pa, false, cursor, output))
		{
			if (!TestToken(cursor, CppTokens::SEMICOLON, false))
			{
				ParseVariablesFollowedByDecl_NotConsumeSemicolon(pa, classDecl, cursor, output);
			}
		}
		RequireToken(cursor, CppTokens::SEMICOLON);
	}
	else if (TestToken(cursor, CppTokens::DECL_USING, false))
	{
		ParseDeclaration_Using(pa, cursor, output);
	}
	else if(TestToken(cursor,CppTokens::DECL_TYPEDEF, false))
	{
		ParseDeclaration_Typedef(pa, cursor, output);
	}
	else
	{
		ParseDeclaration_FuncVar(pa, decoratorFriend, cursor, output);
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
		varDecl->needResolveTypeFromInitializer = IsPendingType(varDecl->type);
		varDecls.Add(varDecl);
	}
}

/***********************************************************************
BuildSymbols
***********************************************************************/

void BuildSymbols(const ParsingArguments& pa, List<Ptr<VariableDeclaration>>& varDecls, Ptr<CppTokenCursor>& cursor)
{
	for (vint i = 0; i < varDecls.Count(); i++)
	{
		auto varDecl = varDecls[i];
		if (varDecl->name)
		{
			if (!pa.context->AddDeclToSymbol(varDecl, symbol_component::SymbolKind::Variable))
			{
				throw StopParsingException(cursor);
			}
		}
	}
}

/***********************************************************************
BuildVariablesAndSymbols
***********************************************************************/

void BuildVariablesAndSymbols(const ParsingArguments& pa, List<Ptr<Declarator>>& declarators, List<Ptr<VariableDeclaration>>& varDecls, Ptr<CppTokenCursor>& cursor)
{
	BuildVariables(declarators, varDecls);
	BuildSymbols(pa, varDecls, cursor);
}

/***********************************************************************
BuildVariableAndSymbol
***********************************************************************/

Ptr<VariableDeclaration> BuildVariableAndSymbol(const ParsingArguments& pa, Ptr<Declarator> declarator, Ptr<CppTokenCursor>& cursor)
{
	List<Ptr<Declarator>> declarators;
	declarators.Add(declarator);

	List<Ptr<VariableDeclaration>> varDecls;
	BuildVariablesAndSymbols(pa, declarators, varDecls, cursor);
	return varDecls[0];
}