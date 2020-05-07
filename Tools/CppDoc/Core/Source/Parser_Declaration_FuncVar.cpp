#include "Parser.h"
#include "Parser_Declaration.h"

/////////////////////////////////////////////////////////////////////////////////////////////

Ptr<TemplateSpec> AssignContainerClassDeclsToSpecs(
	List<Ptr<TemplateSpec>>& specs,
	Ptr<Declarator> declarator,
	List<Ptr<TemplateSpec>>& containerClassSpecs,
	List<ClassDeclaration*>& containerClassDecls,
	Ptr<CppTokenCursor>& cursor
)
{
	if (declarator->classMemberCache)
	{
		for (vint i = 0; i < declarator->classMemberCache->containerClassTypes.Count(); i++)
		{
			auto classType = declarator->classMemberCache->containerClassTypes[i];
			if (classType->GetType() == TsysType::GenericFunction)
			{
				classType = classType->GetElement();
			}

			auto classDecl = classType->GetDecl()->GetAnyForwardDecl<ForwardClassDeclaration>();
			switch (classType->GetType())
			{
			case TsysType::Decl:
				if (classDecl->templateSpec && !classDecl->specializationSpec)
				{
					classType->MakePSRecordPrimaryThis();
				}
				break;
			case TsysType::DeclInstant:
				if (!classDecl->specializationSpec)
				{
					classType->MakePSRecordPrimaryThis();
				}
				break;
			default:
				throw L"Unexpected container class type!";
			}
		}
	}

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
			return nullptr;
		case 1:
			return specs[used];
		default:
			throw StopParsingException(cursor);
		}
	}
	else
	{
		return EnsureNoMultipleTemplateSpec(specs, cursor);
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
#define DEFINE_FUNCVAR_TEST(TOKEN, NAME) if (TestToken(cursor, CppTokens::TOKEN)) { decorator##NAME = true; while(SkipSpecifiers(cursor)); } else
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
		pda.scopeSymbolToReuse = specSymbol;

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

	// extract multiple levels of container classes
	if (specs.Count() > 0 && declarators.Count() > 1)
	{
		throw StopParsingException(cursor);
	}

	List<Ptr<TemplateSpec>> containerClassSpecs;
	List<ClassDeclaration*> containerClassDecls;
	List<ClassSpec> classSpecs;
	auto declSpec = AssignContainerClassDeclsToSpecs(specs, declarators[0], containerClassSpecs, containerClassDecls, cursor);
	for (vint i = 0; i < containerClassSpecs.Count(); i++)
	{
		classSpecs.Add({ containerClassSpecs[i],containerClassDecls[i] });
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
			declarators[0]->scopeSymbolToReuse,
			classSpecs,
			declSpec,
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
		for (vint i = 0; i < declarators.Count(); i++)
		{
			ParseDeclaration_Variable(
				pa,
				declarators[i]->scopeSymbolToReuse,
				classSpecs,
				declSpec,
				declarators[i],
				FUNCVAR_DECORATORS_FOR_VARIABLE(FUNCVAR_ARGUMENT)
				cursor,
				output
			);
		}
		RequireToken(cursor, CppTokens::SEMICOLON);
	}
}