#include "Parser.h"
#include "Ast_Expr.h"
#include "Ast_Resolving.h"

/***********************************************************************
ParseTemplateSpec
***********************************************************************/

void ParseTemplateSpec(const ParsingArguments& pa, Ptr<CppTokenCursor>& cursor, Ptr<Symbol>& specSymbol, Ptr<TemplateSpec>& spec)
{
	RequireToken(cursor, CppTokens::DECL_TEMPLATE);
	RequireToken(cursor, CppTokens::LT);

	if (!specSymbol)
	{
		specSymbol = MakePtr<Symbol>(pa.scopeSymbol);
	}
	spec = MakePtr<TemplateSpec>();
	auto newPa = pa.WithScope(specSymbol.Obj());

	while (!TestToken(cursor, CppTokens::GT))
	{
		TemplateSpec::Argument argument;
		if (TestToken(cursor, CppTokens::DECL_TEMPLATE, false))
		{
			ParseTemplateSpec(newPa, cursor, argument.templateSpecScope, argument.templateSpec);
			ValidateForRootTemplateSpec(argument.templateSpec, cursor,false, false);
		}

		if (TestToken(cursor, CppTokens::TYPENAME) || TestToken(cursor, CppTokens::DECL_CLASS))
		{
			argument.argumentType = CppTemplateArgumentType::Type;
			argument.ellipsis = TestToken(cursor, CppTokens::DOT, CppTokens::DOT, CppTokens::DOT);
			if (ParseCppName(argument.name, cursor))
			{
				if (specSymbol->TryGetChildren_NFb(argument.name.name))
				{
					throw StopParsingException(cursor);
				}
			}

			auto argumentSymbol = argument.templateSpec ? argument.templateSpecScope : MakePtr<Symbol>(specSymbol.Obj());
			argument.argumentSymbol = argumentSymbol.Obj();

			argumentSymbol->kind = symbol_component::SymbolKind::GenericTypeArgument;
			argumentSymbol->ellipsis = argument.ellipsis;
			argumentSymbol->name = argument.name.name;
			auto& ev = argumentSymbol->GetEvaluationForUpdating_NFb();
			ev.Allocate();

			if (argument.templateSpec)
			{
				TsysGenericFunction genericFunction;
				TypeTsysList params;
				symbol_type_resolving::CreateGenericFunctionHeader(pa,argumentSymbol.Obj(), nullptr, argument.templateSpec, params, genericFunction);
				ev.Get().Add(pa.tsys->Any()->GenericFunctionOf(params, genericFunction));
			}
			else
			{
				TsysGenericArg arg;
				arg.argIndex = spec->arguments.Count();
				arg.argSymbol = argumentSymbol.Obj();
				ev.Get().Add(pa.tsys->DeclOf(specSymbol.Obj())->GenericArgOf(arg));
			}

			if (argument.name)
			{
				specSymbol->AddChild_NFb(argumentSymbol->name, argumentSymbol);
			}
			else
			{
				specSymbol->AddChild_NFb(L"$", argumentSymbol);
			}

			if (TestToken(cursor, CppTokens::EQ))
			{
				if (argument.ellipsis)
				{
					throw StopParsingException(cursor);
				}
				argument.type = ParseType(newPa, cursor);
			}
		}
		else
		{
			if (argument.templateSpec)
			{
				throw StopParsingException(cursor);
			}

			auto declarator = ParseNonMemberDeclarator(newPa, pda_TemplateParam(), cursor);
			argument.argumentType = CppTemplateArgumentType::Value;
			argument.name = declarator->name;
			argument.type = declarator->type;
			argument.ellipsis = declarator->ellipsis;
			if (declarator->initializer)
			{
				if (declarator->initializer->initializerType != CppInitializerType::Equal)
				{
					throw StopParsingException(cursor);
				}
				if (argument.ellipsis)
				{
					throw StopParsingException(cursor);
				}
				argument.expr = declarator->initializer->arguments[0].item;
			}
			if (argument.name)
			{
				if (specSymbol->TryGetChildren_NFb(argument.name.name))
				{
					throw StopParsingException(cursor);
				}
			}

			auto argumentSymbol = MakePtr<Symbol>(specSymbol.Obj());
			argument.argumentSymbol = argumentSymbol.Obj();
			argumentSymbol->kind = symbol_component::SymbolKind::GenericValueArgument;
			argumentSymbol->ellipsis = argument.ellipsis;
			argumentSymbol->name = argument.name.name;
			auto& ev = argumentSymbol->GetEvaluationForUpdating_NFb();
			ev.Allocate();
			TypeToTsysNoVta(newPa, argument.type, ev.Get());
			if (ev.Get().Count() == 0)
			{
				ev.Get().Add(pa.tsys->Any());
			}

			if (argument.name)
			{
				specSymbol->AddChild_NFb(argumentSymbol->name, argumentSymbol);
			}
			else
			{
				specSymbol->AddChild_NFb(L"$", argumentSymbol);
			}
		}
		spec->arguments.Add(argument);

		if (!TestToken(cursor, CppTokens::COMMA))
		{
			RequireToken(cursor, CppTokens::GT);
			break;
		}
	}
}

/***********************************************************************
ValidateForRootTemplateSpec
***********************************************************************/

void ValidateForRootTemplateSpec(Ptr<TemplateSpec>& spec, Ptr<CppTokenCursor>& cursor, bool forPS, bool forFunction)
{
	if (!spec) return;
	if (forPS)
	{
		if (forFunction)
		{
			if (spec->arguments.Count() > 0)
			{
				// function does not support partial specialization
				throw StopParsingException(cursor);
			}
		}
		else
		{
			for (vint i = 0; i < spec->arguments.Count() - 1; i++)
			{
				auto argument = spec->arguments[i];
				switch (argument.argumentType)
				{
				case CppTemplateArgumentType::Type:
				case CppTemplateArgumentType::HighLevelType:
					if (argument.type)
					{
						// no default value for partial specialization
						throw StopParsingException(cursor);
					}
					break;
				case CppTemplateArgumentType::Value:
					if (argument.expr)
					{
						// no default value for partial specialization
						throw StopParsingException(cursor);
					}
					break;
				}
			}
		}
	}
	else
	{
		if (!forFunction)
		{
			for (vint i = 0; i < spec->arguments.Count() - 1; i++)
			{
				if (spec->arguments[i].ellipsis)
				{
					// there is no specialization, so only the last parameter can be variadic
					throw StopParsingException(cursor);
				}
			}
		}
	}
}

/***********************************************************************
ParseSpecializationSpec
***********************************************************************/

void ParseSpecializationSpec(const ParsingArguments& pa, Ptr<CppTokenCursor>& cursor, Ptr<SpecializationSpec>& spec)
{
	if (TestToken(cursor, CppTokens::LT))
	{
		// <{ TYPE/EXPR ...} >
		spec = MakePtr<SpecializationSpec>();
		ParseGenericArgumentsSkippedLT(pa, cursor, spec->arguments, CppTokens::GT, false);
	}
}

/***********************************************************************
ParseGenericArgumentsSkippedLT
***********************************************************************/

void ParseGenericArgumentsSkippedLT(const ParsingArguments& pa, Ptr<CppTokenCursor>& cursor, VariadicList<GenericArgument>& arguments, CppTokens ending, bool allowVariadicOnAllArguments)
{
	// { TYPE/EXPR ...} >
	while (!TestToken(cursor, ending))
	{
		GenericArgument argument;
		ParseTypeOrExpr(pa, pea_GenericArgument(), cursor, argument.type, argument.expr);
		bool isVariadic = TestToken(cursor, CppTokens::DOT, CppTokens::DOT, CppTokens::DOT);
		arguments.Add({ argument,isVariadic });

		if (!TestToken(cursor, CppTokens::COMMA))
		{
			RequireToken(cursor, ending);
			break;
		}
		else if (isVariadic && !allowVariadicOnAllArguments)
		{
			throw StopParsingException(cursor);
		}
	}
}