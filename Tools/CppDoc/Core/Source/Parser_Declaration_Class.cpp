#include "Parser.h"
#include "Parser_Declaration.h"
#include "Parser_Declarator.h"

using namespace partial_specification_ordering;

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

	Ptr<SpecializationSpec> specializationSpec;
	if (spec)
	{
		auto specPa = pa.WithScope(specSymbol.Obj());
		ParseSpecializationSpec(specPa, cursor, specializationSpec);
	}
	ValidateForRootTemplateSpec(spec, cursor, specializationSpec, false);

	TestToken(cursor, CppTokens::DECL_FINAL);
	TestToken(cursor, CppTokens::DECL_ABSTRACT);

	if (TestToken(cursor, CppTokens::SEMICOLON, false))
	{
		if (forTypeDef || isAnonymous)
		{
			throw StopParsingException(cursor);
		}

		// ... ;
		auto decl = MakePtr<ForwardClassDeclaration>();
		decl->templateScope = specSymbol.Obj();
		decl->templateSpec = spec;
		decl->specializationSpec = specializationSpec;
		decl->classType = classType;
		decl->name = cppName;
		output.Add(decl);

		auto createdSymbol = pa.scopeSymbol->AddForwardDeclToSymbol_NFb(decl, symbolKind);
		if (!createdSymbol)
		{
			throw StopParsingException(cursor);
		}

		if (decl->templateSpec)
		{
			decl->templateSpec->AssignDeclSymbol(createdSymbol);
		}

		if (decl->specializationSpec)
		{
			AssignPSPrimary(pa, cursor, createdSymbol);
		}
		return nullptr;
	}
	else
	{
		// ... [: { [public|protected|private] TYPE , ...} ]
		auto decl = MakePtr<ClassDeclaration>();
		decl->templateSpec = spec;
		decl->specializationSpec = specializationSpec;
		decl->classType = classType;
		decl->name = cppName;
		vint declIndex = output.Add(decl);

		auto classContextSymbol = pa.scopeSymbol->AddImplDeclToSymbol_NFb(decl, symbolKind, specSymbol);
		if (!classContextSymbol)
		{
			throw StopParsingException(cursor);
		}

		if (classContextSymbol->GetEvaluationForUpdating_NFb().progress == symbol_component::EvaluationProgress::Evaluated)
		{
			// this will happen when EvaluateForwardClassDeclaration is called on a template class that only has forward declarations
			// the type evaluated from the template spec of the first seen ForwardClassDeclaration is stored
			// but from now on Symbol::GetAnyForwardDecl returns the creating ClassDeclaration
			// there will be some issue on agreeing which template arguments are used in the GenericFunction of this class
			// so just delete the record

			auto& ev = classContextSymbol->GetEvaluationForUpdating_NFb();
			ev.Get().Clear();
			ev.progress = symbol_component::EvaluationProgress::NotEvaluated;
			// there will be no base class record for ForwardClassDeclaration
		}

		if (decl->templateSpec)
		{
			decl->templateSpec->AssignDeclSymbol(classContextSymbol);
		}

		if (decl->specializationSpec)
		{
			AssignPSPrimary(pa, cursor, classContextSymbol);
		}

		auto declPa = pa.WithScope(classContextSymbol);

		if (TestToken(cursor, CppTokens::COLON))
		{
			auto& ev = decl->symbol->GetEvaluationForUpdating_NFb().skipEvaluatingBaseTypes = true;
			while (!TestToken(cursor, CppTokens::LBRACE, false))
			{
				auto accessor = defaultAccessor;

				// ignore virtual
				TestToken(cursor, CppTokens::VIRTUAL);

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

				// ignore virtual
				TestToken(cursor, CppTokens::VIRTUAL);

				auto type = ParseShortType(declPa, ShortTypeTypenameKind::Implicit, cursor);
				bool isVariadic = TestToken(cursor, CppTokens::DOT, CppTokens::DOT, CppTokens::DOT);
				decl->baseTypes.Add({ { accessor,type },isVariadic });

				if (TestToken(cursor, CppTokens::LBRACE, false))
				{
					break;
				}
				else
				{
					RequireToken(cursor, CppTokens::COMMA);
				}
			}
			decl->symbol->GetEvaluationForUpdating_NFb().skipEvaluatingBaseTypes = false;
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

				if (auto classDecl = pair.f1.Cast<ClassDeclaration>())
				{
					// anonymous class is fine
					if (classDecl->name.name.Length() <= 11 || classDecl->name.name.Left(11) != L"<anonymous>")
					{
						throw StopParsingException(cursor);
					}
				}
				else if (auto enumDecl = pair.f1.Cast<EnumDeclaration>())
				{
					// anonymous enum is fine
					if (classDecl->name.name.Length() <= 11 || classDecl->name.name.Left(11) != L"<anonymous>")
					{
						throw StopParsingException(cursor);
					}
				}
				else if (pair.f1.Cast<VariableDeclaration>())
				{
					// variable is fine
				}
				else if (pair.f1.Cast<NestedAnonymousClassDeclaration>())
				{
					// nested anonymous class is fine
				}
				else
				{
					throw StopParsingException(cursor);
				}

				nestedDecl->decls.Add(pair.f1);
			}

			const auto& contextChildren = classContextSymbol->GetChildren_NFb();
			for (vint i = 0; i < contextChildren.Count(); i++)
			{
				// variable doesn't override, so here just look at the first symbol
				auto& child = contextChildren.GetByIndex(i)[0];
				if (pa.scopeSymbol->TryGetChildren_NFb(child.childSymbol->name))
				{
					throw StopParsingException(cursor);
				}
				pa.scopeSymbol->AddChildAndSetParent_NFb(child.childSymbol->name, child.childSymbol);
			}
			pa.scopeSymbol->RemoveChildAndResetParent_NFb(classContextSymbol->name, classContextSymbol);

			return nullptr;
		}

		if (classType != CppClassType::Union)
		{
			// TODO: [Cpp.md] generate members for specialized declarations
			if (!decl->specializationSpec)
			{
				GenerateMembers(declPa, classContextSymbol);
			}
		}
		return decl;
	}
}