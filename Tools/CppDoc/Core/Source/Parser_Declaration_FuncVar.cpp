#include "Parser.h"
#include "Parser_Declaration.h"
#include "Parser_Declarator.h"

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
		pda.classOfMemberInside = containingClass;
		pda.specsOfMemberOutside = &specs;
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

	List<Ptr<TemplateSpec>> classSpecs;
	Ptr<TemplateSpec> declSpec;

	if (declarators[0]->classMemberCache)
	{
		auto cache = declarators[0]->classMemberCache;
		if (cache->symbolDefinedInsideClass)
		{
			declSpec = EnsureNoMultipleTemplateSpec(specs, cursor);
		}
		else
		{
			for (vint i = cache->containerClassSpecs.Count() - 1; i >= 0; i--)
			{
				if (auto spec = cache->containerClassSpecs[i])
				{
					classSpecs.Add(spec);
				}
			}
			declSpec = cache->declSpec;
		}
	}
	else
	{
		declSpec = EnsureNoMultipleTemplateSpec(specs, cursor);
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
			// for variables, names should not be constructor names, destructor names, type conversion operator names, or other operator names
			if (declarators[i]->name.type != CppNameType::Normal)
			{
				throw StopParsingException(cursor);
			}

			if (declSpec)
			{
				ParseDeclaration_ValueAlias(
					pa,
					declarators[i]->scopeSymbolToReuse,
					declSpec,
					declarators[i],
					FUNCVAR_DECORATORS_FOR_VARIABLE(FUNCVAR_ARGUMENT)
					cursor,
					output
				);
			}
			else
			{
				ParseDeclaration_Variable(
					pa,
					declarators[i]->scopeSymbolToReuse,
					classSpecs,
					declarators[i],
					FUNCVAR_DECORATORS_FOR_VARIABLE(FUNCVAR_ARGUMENT)
					cursor,
					output
				);
			}
		}
		RequireToken(cursor, CppTokens::SEMICOLON);
	}
}