#include "Parser.h"
#include "Ast_Expr.h"
#include "Ast_Resolving.h"

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
		auto contextSymbol = pa.scopeSymbol;
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
				contextSymbol = contextSymbol->AddForwardDeclToSymbol_NFb(decl, symbol_component::SymbolKind::Namespace);
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

		auto newPa = pa.WithScope(contextSymbol);
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
	while (SkipSpecifiers(cursor));

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

		if (!pa.scopeSymbol->AddForwardDeclToSymbol_NFb(decl, symbol_component::SymbolKind::Enum))
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

		auto contextSymbol = pa.scopeSymbol->AddImplDeclToSymbol_NFb(decl, symbol_component::SymbolKind::Enum);
		if (!contextSymbol)
		{
			throw StopParsingException(cursor);
		}
		auto newPa = pa.WithScope(contextSymbol);

		RequireToken(cursor, CppTokens::LBRACE);
		while (!TestToken(cursor, CppTokens::RBRACE))
		{
			while (SkipSpecifiers(cursor));
			auto enumItem = MakePtr<EnumItemDeclaration>();
			if (!ParseCppName(enumItem->name, cursor)) throw StopParsingException(cursor);
			decl->items.Add(enumItem);
			if (!contextSymbol->AddImplDeclToSymbol_NFb(enumItem, symbol_component::SymbolKind::EnumItem))
			{
				throw StopParsingException(cursor);
			}

			if (!enumClass)
			{
				if (auto pEnumItems = pa.scopeSymbol->TryGetChildren_NFb(enumItem->name.name))
				{
					throw StopParsingException(cursor);
				}
				pa.scopeSymbol->AddChild_NFb(enumItem->name.name, contextSymbol->TryGetChildren_NFb(enumItem->name.name)->Get(0));
			}

			if (TestToken(cursor, CppTokens::EQ))
			{
				enumItem->value = ParseExpr(newPa, pea_Argument(), cursor);
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

Ptr<ClassDeclaration> ParseDeclaration_Class_NotConsumeSemicolon(const ParsingArguments& pa, Ptr<Symbol> specSymbol, Ptr<TemplateSpec> spec, bool forTypeDef, Ptr<CppTokenCursor>& cursor, List<Ptr<Declaration>>& output)
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
	SkipToken(cursor);
	while (SkipSpecifiers(cursor));

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
		decl->keepTemplateSpecArgumentSymbolsAliveOnlyForForwardDeclaration = specSymbol;
		decl->templateSpec = spec;
		decl->classType = classType;
		decl->name = cppName;
		output.Add(decl);

		if (!pa.scopeSymbol->AddForwardDeclToSymbol_NFb(decl, symbolKind))
		{
			throw StopParsingException(cursor);
		}
		return nullptr;
	}
	else
	{
		// ... [: { [public|protected|private] TYPE , ...} ]
		auto decl = MakePtr<ClassDeclaration>();
		decl->templateSpec = spec;
		decl->classType = classType;
		decl->name = cppName;
		vint declIndex = output.Add(decl);

		auto classContextSymbol = pa.scopeSymbol->AddImplDeclToSymbol_NFb(decl, symbolKind, specSymbol);
		if (!classContextSymbol)
		{
			throw StopParsingException(cursor);
		}

		auto declPa = pa.WithScope(classContextSymbol);

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
			// so a NestedAnonymousClassDeclaration is created, copy all members to it, and move all symbols to pa.scopeSymbol, which is the parent class declaration

			if (!pa.scopeSymbol->GetImplDecl_NFb<ClassDeclaration>())
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

			const auto& contextChildren = classContextSymbol->GetChildren_NFb();
			for (vint i = 0; i < contextChildren.Count(); i++)
			{
				// variable doesn't override, so here just look at the first symbol
				auto child = contextChildren.GetByIndex(i)[0];
				if (pa.scopeSymbol->TryGetChildren_NFb(child->name))
				{
					throw StopParsingException(cursor);
				}
				pa.scopeSymbol->AddChildAndSetParent_NFb(child->name, child);
			}
			pa.scopeSymbol->RemoveChildAndResetParent_NFb(classContextSymbol->name, classContextSymbol);

			return nullptr;
		}

		if (classType != CppClassType::Union)
		{
			GenerateMembers(declPa, classContextSymbol);
		}
		return decl;
	}
}

/***********************************************************************
ParseDeclaration_<USING>
***********************************************************************/

void ParseDeclaration_Using(const ParsingArguments& pa, Ptr<Symbol> specSymbol, Ptr<TemplateSpec> spec, Ptr<CppTokenCursor>& cursor, List<Ptr<Declaration>>& output)
{
	RequireToken(cursor, CppTokens::DECL_USING);
	if (TestToken(cursor, CppTokens::DECL_NAMESPACE))
	{
		if (spec)
		{
			throw StopParsingException(cursor);
		}

		// using namespace TYPE;
		auto decl = MakePtr<UsingNamespaceDeclaration>();
		decl->ns = ParseType(pa, cursor);
		output.Add(decl);
		RequireToken(cursor, CppTokens::SEMICOLON);

		if (auto catIdChildType = decl->ns.Cast<Category_Id_Child_Type>())
		{
			if (!catIdChildType->resolving) throw StopParsingException(cursor);
			if (catIdChildType->resolving->resolvedSymbols.Count() != 1) throw StopParsingException(cursor);
			auto symbol = catIdChildType->resolving->resolvedSymbols[0];

			switch (symbol->kind)
			{
			case symbol_component::SymbolKind::Namespace:
				{
					if (pa.scopeSymbol && !(pa.scopeSymbol->usingNss.Contains(symbol)))
					{
						pa.scopeSymbol->usingNss.Add(symbol);
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
		{
			auto oldCursor = cursor;
			// using NAME = TYPE;
			CppName cppName;
			if (!ParseCppName(cppName, cursor) || !TestToken(cursor, CppTokens::EQ))
			{
				cursor = oldCursor;
				goto SKIP_TYPE_ALIAS;
			}

			auto newPa = specSymbol ? pa.WithScope(specSymbol.Obj()) : pa;

			auto decl = MakePtr<TypeAliasDeclaration>();
			decl->templateSpec = spec;
			decl->name = cppName;
			decl->type = ParseType(newPa, cursor);
			RequireToken(cursor, CppTokens::SEMICOLON);

			output.Add(decl);
			if (!pa.scopeSymbol->AddImplDeclToSymbol_NFb(decl, symbol_component::SymbolKind::TypeAlias, specSymbol))
			{
				throw StopParsingException(cursor);
			}

			return;
		}
	SKIP_TYPE_ALIAS:
		{
			if (spec)
			{
				throw StopParsingException(cursor);
			}

			// using TYPE[::NAME];
			auto decl = MakePtr<UsingSymbolDeclaration>();
			ParseTypeOrExpr(pa, pea_Full(), cursor, decl->type, decl->expr);
			RequireToken(cursor, CppTokens::SEMICOLON);
			output.Add(decl);

			Ptr<Resolving> resolving;
			if (decl->expr)
			{
				if (auto catIdChild = decl->expr.Cast<Category_Id_Child_Expr>())
				{
					resolving = catIdChild->resolving;
				}
			}
			else if (decl->type)
			{
				if (auto catIdChildType = decl->type.Cast<Category_Id_Child_Type>())
				{
					resolving = catIdChildType->resolving;
				}
			}

			if (!resolving) throw StopParsingException(cursor);
			for (vint i = 0; i < resolving->resolvedSymbols.Count(); i++)
			{
				auto rawSymbolPtr = resolving->resolvedSymbols[i];
				auto pSiblings = rawSymbolPtr->GetParentScope()->TryGetChildren_NFb(rawSymbolPtr->name);
				auto symbol = pSiblings->Get(pSiblings->IndexOf(rawSymbolPtr));

				switch (symbol->kind)
				{
				case symbol_component::SymbolKind::Enum:
				case symbol_component::SymbolKind::Class:
				case symbol_component::SymbolKind::Struct:
				case symbol_component::SymbolKind::Union:
				case symbol_component::SymbolKind::TypeAlias:
				case symbol_component::SymbolKind::EnumItem:
				case symbol_component::SymbolKind::Variable:
				case symbol_component::SymbolKind::ValueAlias:
					{
						if (pa.scopeSymbol->TryGetChildren_NFb(symbol->name))
						{
							throw StopParsingException(cursor);
						}
						pa.scopeSymbol->AddChild_NFb(symbol->name, symbol);
					}
					break;
				case symbol_component::SymbolKind::FunctionSymbol:
					{
						if (auto pChildren = pa.scopeSymbol->TryGetChildren_NFb(symbol->name))
						{
							if (pChildren->Get(0)->kind != symbol_component::SymbolKind::FunctionSymbol)
							{
								throw StopParsingException(cursor);
							}
						}
						pa.scopeSymbol->AddChild_NFb(symbol->name, symbol);
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

bool IsCStyleTypeReference(Ptr<CppTokenCursor>& cursor)
{
	bool cStyleTypeReference = false;
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
	return cStyleTypeReference;
}

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

		createdType = MakePtr<IdType>();
		createdType->name = classDecl->name;
		createdType->resolving = MakePtr<Resolving>();
		createdType->resolving->resolvedSymbols.Add(classDecl->symbol);
		ParseNonMemberDeclarator(pa, pda_Typedefs(), createdType, cursor, declarators);
	}
	else if (!cStyleTypeReference && TestToken(cursor, CppTokens::DECL_ENUM, false))
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
		auto decl = MakePtr<TypeAliasDeclaration>();
		decl->name = declarators[i]->name;
		decl->type = declarators[i]->type;
		output.Add(decl);

		if (!pa.scopeSymbol->AddImplDeclToSymbol_NFb(decl, symbol_component::SymbolKind::TypeAlias))
		{
			throw StopParsingException(cursor);
		}
	}
}

/***********************************************************************
SearchForFunctionWithSameSignature
***********************************************************************/

Symbol* SearchForFunctionWithSameSignature(Symbol* context, Ptr<ForwardFunctionDeclaration> decl, Ptr<CppTokenCursor>& cursor)
{
	if (!decl->needResolveTypeFromStatement)
	{
		if (auto pSymbols = context->TryGetChildren_NFb(decl->name.name))
		{
			for (vint i = 0; i < pSymbols->Count(); i++)
			{
				auto symbol = pSymbols->Get(i).Obj();
				if (symbol->kind == symbol_component::SymbolKind::FunctionSymbol)
				{
					auto declToCompare = symbol->GetAnyForwardDecl<ForwardFunctionDeclaration>();
					if (IsCompatibleFunctionDeclInSameScope(decl, declToCompare))
					{
						return symbol;
					}
				}
				else
				{
					throw StopParsingException(cursor);
				}
			}
		}
	}
	return context->CreateFunctionSymbol_NFb(decl);
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
	F(Inline)\
	F(__Inline)\
	F(__ForceInline)\

#define FUNCVAR_PARAMETER(NAME) bool decorator##NAME,
#define FUNCVAR_ARGUMENT(NAME) decorator##NAME,
#define FUNCVAR_FILL_DECLARATOR(NAME) decl->decorator##NAME = decorator##NAME;

/////////////////////////////////////////////////////////////////////////////////////////////

void EnsureNoTemplateSpec(List<Ptr<TemplateSpec>>& specs, Ptr<CppTokenCursor>& cursor)
{
	if (specs.Count() > 0)
	{
		throw StopParsingException(cursor);
	}
}

Ptr<TemplateSpec> EnsureNoMultipleTemplateSpec(List<Ptr<TemplateSpec>>& specs, Ptr<CppTokenCursor>& cursor)
{
	if (specs.Count() > 1)
	{
		throw StopParsingException(cursor);
	}
	else if (specs.Count() == 1)
	{
		return specs[0];
	}
	else
	{
		return nullptr;
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////

void ParseDeclaration_Function(
	const ParsingArguments& pa,
	Ptr<Symbol> specSymbol,
	List<Ptr<TemplateSpec>>& specs,
	Ptr<Declarator> declarator,
	Ptr<FunctionType> funcType,
	FUNCVAR_DECORATORS_FOR_FUNCTION(FUNCVAR_PARAMETER)
	CppMethodType methodType,
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

	Ptr<TemplateSpec> functionSpec;
	List<Ptr<TemplateSpec>> containerClassSpecs;
	List<ClassDeclaration*> containerClassDecls;
	if (declarator->classMemberCache && !declarator->classMemberCache->symbolDefinedInsideClass)
	{
		vint used = 0;
		auto& thisTypes = declarator->classMemberCache->containerClassTypes;
		for (vint i = thisTypes.Count() - 1; i >= 0; i--)
		{
			auto thisType = thisTypes[i];
			auto thisDecl = thisType->GetDecl()->GetImplDecl_NFb<ClassDeclaration>();
			if (!thisDecl) throw StopParsingException(cursor);

			if (thisDecl->templateSpec)
			{
				if (used >= specs.Count()) throw StopParsingException(cursor);
				auto thisSpec = specs[used++];
				if (thisSpec->arguments.Count() != thisDecl->templateSpec->arguments.Count()) throw StopParsingException(cursor);
				for (vint j = 0; j < thisSpec->arguments.Count(); j++)
				{
					auto specArg = thisSpec->arguments[j];
					auto declArg = thisDecl->templateSpec->arguments[j];
					if (specArg.argumentType != declArg.argumentType) throw StopParsingException(cursor);
				}
				containerClassSpecs.Add(thisSpec);
				containerClassDecls.Add(thisDecl.Obj());
			}
		}

		switch (specs.Count() - used)
		{
		case 0:
			break;
		case 1:
			functionSpec = specs[used];
			break;
		default:
			throw StopParsingException(cursor);
		}
	}
	else
	{
		functionSpec = EnsureNoMultipleTemplateSpec(specs, cursor);
	}

	auto context = declarator->classMemberCache ? declarator->classMemberCache->containerClassTypes[0]->GetDecl() : pa.scopeSymbol;
	if (hasStat)
	{
		// if there is a statement, then it is a function declaration
		auto decl = MakePtr<FunctionDeclaration>();
		decl->templateSpec = functionSpec;
		decl->speclizationSpec = declarator->specializationSpec;
		for (vint i = 0; i < containerClassSpecs.Count(); i++)
		{
			decl->classSpecs.Add({ containerClassSpecs[i],containerClassDecls[i] });
		}

		FILL_FUNCTION;
		output.Add(decl);

		if (declarator->classMemberCache)
		{
			TsysCV cv;
			cv.isGeneralConst = funcType->qualifierConstExpr || funcType->qualifierConst;
			cv.isVolatile = funcType->qualifierVolatile;

			auto classType = declarator->classMemberCache->containerClassTypes[0];
			declarator->classMemberCache->thisType = classType->CVOf(cv)->PtrOf();

			if (!declarator->classMemberCache->symbolDefinedInsideClass)
			{
				declarator->classMemberCache->parentScope = pa.scopeSymbol;
			}
		}

		auto functionSymbol = SearchForFunctionWithSameSignature(context, decl, cursor);
		auto functionBodySymbol = functionSymbol->CreateFunctionImplSymbol_F(decl, specSymbol, declarator->classMemberCache);

		{
			auto newPa = pa.WithScope(functionBodySymbol);
			BuildSymbols(newPa, funcType->parameters, cursor);

			// delay parse the statement
			decl->delayParse = MakePtr<DelayParse>();
			decl->delayParse->pa = newPa;
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
		if (declarator->classMemberCache && !declarator->classMemberCache->symbolDefinedInsideClass)
		{
			throw StopParsingException(cursor);
		}

		auto decl = MakePtr<ForwardFunctionDeclaration>();
		decl->templateSpec = functionSpec;
		decl->speclizationSpec = declarator->specializationSpec;
		FILL_FUNCTION;
		output.Add(decl);
		RequireToken(cursor, CppTokens::SEMICOLON);

		auto functionSymbol = SearchForFunctionWithSameSignature(context, decl, cursor);
		functionSymbol->CreateFunctionForwardSymbol_F(decl, specSymbol);
	}
#undef FILL_FUNCTION
}

/////////////////////////////////////////////////////////////////////////////////////////////

void ParseDeclaration_Variable(
	const ParsingArguments& pa,
	Ptr<Symbol> specSymbol,
	Ptr<TemplateSpec> spec,
	Ptr<Declarator> declarator,
	FUNCVAR_DECORATORS_FOR_VARIABLE(FUNCVAR_PARAMETER)
	Ptr<CppTokenCursor>& cursor,
	List<Ptr<Declaration>>& output
)
{
	// for variables, names should not be constructor names, destructor names, type conversion operator names, or other operator names
	if (declarator->name.type != CppNameType::Normal)
	{
		throw StopParsingException(cursor);
	}

	if (spec)
	{
		if (!declarator->initializer || declarator->initializer->initializerType != CppInitializerType::Equal)
		{
			throw StopParsingException(cursor);
		}

		auto decl = MakePtr<ValueAliasDeclaration>();
		decl->templateSpec = spec;
		decl->name = declarator->name;
		decl->type = declarator->type;
		decl->expr = declarator->initializer->arguments[0].item;
		decl->needResolveTypeFromInitializer = IsPendingType(declarator->type);
		output.Add(decl);

		if (decl->needResolveTypeFromInitializer)
		{
			auto primitiveType = decl->type.Cast<PrimitiveType>();
			if (!primitiveType) throw StopParsingException(cursor);
			if (primitiveType->primitive != CppPrimitiveType::_auto) throw StopParsingException(cursor);
		}

		if (!pa.scopeSymbol->AddImplDeclToSymbol_NFb(decl, symbol_component::SymbolKind::ValueAlias, specSymbol))
		{
			throw StopParsingException(cursor);
		}
	}
	else
	{
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

		auto context = declarator->classMemberCache ? declarator->classMemberCache->containerClassTypes[0]->GetDecl() : pa.scopeSymbol;
		if (decoratorExtern || (decoratorStatic && !declarator->initializer))
		{
			// if there is extern, or static without an initializer, then it is a forward variable declaration
			if (declarator->classMemberCache && !declarator->classMemberCache->symbolDefinedInsideClass)
			{
				throw StopParsingException(cursor);
			}

			auto decl = MakePtr<ForwardVariableDeclaration>();
			FILL_VARIABLE;
			output.Add(decl);

			if (!context->AddForwardDeclToSymbol_NFb(decl, symbol_component::SymbolKind::Variable))
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

			if (!context->AddImplDeclToSymbol_NFb(decl, symbol_component::SymbolKind::Variable))
			{
				throw StopParsingException(cursor);
			}
		}
#undef FILL_VARIABLE
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////

void ParseDeclaration_FuncVar(const ParsingArguments& pa, Ptr<Symbol> specSymbol, List<Ptr<TemplateSpec>>& specs, bool decoratorFriend, Ptr<CppTokenCursor>& cursor, List<Ptr<Declaration>>& output)
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
	ClassDeclaration* containingClass = pa.scopeSymbol->GetImplDecl_NFb<ClassDeclaration>().Obj();

	// get all declarators
	{
		auto pda = pda_Decls(pa.scopeSymbol->GetImplDecl_NFb<ClassDeclaration>(), specs.Count() > 0);
		pda.containingClass = containingClass;

		auto newPa = specSymbol ? pa.WithScope(specSymbol.Obj()) : pa;
		ParseMemberDeclarator(newPa, pda, cursor, declarators);
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
		if (declarator->type.Cast<MemberType>())
		{
			if (!declarator->classMemberCache || declarator->classMemberCache->symbolDefinedInsideClass)
			{
				throw StopParsingException(cursor);
			}
		}

		// adjust name type
		if (declarator->classMemberCache)
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
			specSymbol,
			specs,
			declarators[0],
			funcType,
			FUNCVAR_DECORATORS_FOR_FUNCTION(FUNCVAR_ARGUMENT)
			methodType,
			cursor,
			output
		);
	}
	else
	{
		auto spec = EnsureNoMultipleTemplateSpec(specs, cursor);
		if (spec && declarators.Count() > 1)
		{
			throw StopParsingException(cursor);
		}
		for (vint i = 0; i < declarators.Count(); i++)
		{
			ParseDeclaration_Variable(
				pa,
				specSymbol,
				spec,
				declarators[i],
				FUNCVAR_DECORATORS_FOR_VARIABLE(FUNCVAR_ARGUMENT)
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
			nullptr,
			nullptr,
			declarators[i],
			false, false, false, false, false, false, false, false,
			cursor,
			output
		);
	}
}

void ParseDeclaration(const ParsingArguments& pa, Ptr<CppTokenCursor>& cursor, List<Ptr<Declaration>>& output)
{
	while (SkipSpecifiers(cursor));
	if (TestToken(cursor, CppTokens::SEMICOLON))
	{
		// ignore lonely semicolon
		return;
	}

	Ptr<Symbol> specSymbol;
	List<Ptr<TemplateSpec>> specs;
	while(TestToken(cursor, CppTokens::DECL_TEMPLATE, false))
	{
		Ptr<TemplateSpec> spec;
		ParseTemplateSpec(pa, cursor, specSymbol, spec);
		specs.Add(spec);
	}

	bool decoratorFriend = TestToken(cursor, CppTokens::DECL_FRIEND);
	if (decoratorFriend)
	{
		// someone will write "friend TYPE_NAME;", just throw away, since this name must have been declared.
		auto oldCursor = cursor;
		if (TestToken(cursor, CppTokens::ID) && TestToken(cursor, CppTokens::SEMICOLON))
		{
			return;
		}
		cursor = oldCursor;
	}
	bool cStyleTypeReference = IsCStyleTypeReference(cursor);

	// check if the following `struct Id` is trying to reference a type instead of to define a type

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

			// prevent from stack overflowing in CppTokenCursor's destructor
			while (oldCursor != cursor)
			{
				oldCursor = oldCursor->Next();
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
		EnsureNoTemplateSpec(specs, cursor);
		ParseDeclaration_Namespace(pa, cursor, output);
	}
	else if (!cStyleTypeReference && TestToken(cursor, CppTokens::DECL_ENUM, false))
	{
		EnsureNoTemplateSpec(specs, cursor);
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
		auto spec = EnsureNoMultipleTemplateSpec(specs, cursor);
		if (auto classDecl = ParseDeclaration_Class_NotConsumeSemicolon(pa, specSymbol, spec, false, cursor, output))
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
		auto spec = EnsureNoMultipleTemplateSpec(specs, cursor);
		ParseDeclaration_Using(pa, specSymbol, spec, cursor, output);
	}
	else if(TestToken(cursor,CppTokens::DECL_TYPEDEF, false))
	{
		EnsureNoTemplateSpec(specs, cursor);
		ParseDeclaration_Typedef(pa, cursor, output);
	}
	else
	{
		ParseDeclaration_FuncVar(pa, specSymbol, specs, decoratorFriend, cursor, output);
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

void BuildSymbol(const ParsingArguments& pa, Ptr<VariableDeclaration> varDecl, bool isVariadic, Ptr<CppTokenCursor>& cursor)
{
	if (varDecl->name)
	{
		auto symbol = pa.scopeSymbol->AddImplDeclToSymbol_NFb(varDecl, symbol_component::SymbolKind::Variable);
		if (!symbol)
		{
			throw StopParsingException(cursor);
		}
		symbol->ellipsis = isVariadic;
	}
}

void BuildSymbols(const ParsingArguments& pa, List<Ptr<VariableDeclaration>>& varDecls, Ptr<CppTokenCursor>& cursor)
{
	for (vint i = 0; i < varDecls.Count(); i++)
	{
		BuildSymbol(pa, varDecls[i], false, cursor);
	}
}

void BuildSymbols(const ParsingArguments& pa, VariadicList<Ptr<VariableDeclaration>>& varDecls, Ptr<CppTokenCursor>& cursor)
{
	for (vint i = 0; i < varDecls.Count(); i++)
	{
		BuildSymbol(pa, varDecls[i].item, varDecls[i].isVariadic, cursor);
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