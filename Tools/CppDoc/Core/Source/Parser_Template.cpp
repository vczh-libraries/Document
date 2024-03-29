#include "Parser.h"
#include "Parser_Declarator.h"
#include "Ast_Type.h"
#include "Ast_Expr.h"
#include "Symbol_TemplateSpec.h"

/***********************************************************************
ParseTemplateSpec
***********************************************************************/

void ParseTemplateSpec(const ParsingArguments& pa, Ptr<CppTokenCursor>& cursor, Ptr<Symbol>& specSymbol, Ptr<TemplateSpec>& spec)
{
	RequireToken(cursor, CppTokens::DECL_TEMPLATE);
	RequireToken(cursor, CppTokens::LT);

	if (!specSymbol)
	{
		specSymbol = pa.root->CreateSymbol(pa.scopeSymbol);
	}
	spec = Ptr(new TemplateSpec);
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
			// parse
			argument.argumentType = CppTemplateArgumentType::Type;
			argument.ellipsis = TestToken(cursor, CppTokens::DOT, CppTokens::DOT, CppTokens::DOT);

			if (ParseCppName(argument.name, cursor))
			{
				if (specSymbol->TryGetChildren_NFb(argument.name.name))
				{
					throw StopParsingException(cursor);
				}
			}

			// create symbol
			auto argumentSymbol = argument.templateSpec ? argument.templateSpecScope : pa.root->CreateSymbol(specSymbol.Obj());
			argumentSymbol->kind = symbol_component::SymbolKind::GenericTypeArgument;
			argumentSymbol->ellipsis = argument.ellipsis;
			argumentSymbol->name = argument.name.name;
			argument.argumentSymbol = argumentSymbol.Obj();
			if (argument.templateSpec)
			{
				argument.templateSpec->AssignDeclSymbol(argumentSymbol.Obj());
			}

			// create key
			ITsys* argumentKey = nullptr;
			{
				TsysGenericArg arg;
				arg.argSymbol = argumentSymbol.Obj();
				arg.argIndex = spec->arguments.Count();
				arg.spec = spec.Obj();
				argumentKey = pa.tsys->GenericArgOf(arg);
			}

			// store type
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
				ev.Get().Add(argumentKey);
			}

			// store key
			ev.AllocateExtra(1);
			ev.GetExtra(0).Add(argumentKey);

			// add symbol
			if (argument.name)
			{
				specSymbol->AddChild_NFb(argumentSymbol->name, argumentSymbol);
			}
			else
			{
				specSymbol->AddChild_NFb(L"$GenericArg", argumentSymbol);
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

			// parse
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

			// create symbol
			auto argumentSymbol = pa.root->CreateSymbol(specSymbol.Obj());
			argumentSymbol->kind = symbol_component::SymbolKind::GenericValueArgument;
			argumentSymbol->ellipsis = argument.ellipsis;
			argumentSymbol->name = argument.name.name;
			argument.argumentSymbol = argumentSymbol.Obj();

			// create key
			ITsys* argumentKey = nullptr;
			{
				TsysGenericArg arg;
				arg.argSymbol = argumentSymbol.Obj();
				arg.argIndex = spec->arguments.Count();
				arg.spec = spec.Obj();
				argumentKey = pa.tsys->GenericArgOf(arg);
			}

			// store type
			auto& ev = argumentSymbol->GetEvaluationForUpdating_NFb();
			ev.Allocate();
			TypeToTsysNoVta(newPa, argument.type, ev.Get());
			if (ev.Get().Count() == 0)
			{
				ev.Get().Add(pa.tsys->Any());
			}

			// store key
			ev.AllocateExtra(1);
			ev.GetExtra(0).Add(argumentKey);
			
			// add symbol
			if (argument.name)
			{
				specSymbol->AddChild_NFb(argumentSymbol->name, argumentSymbol);
			}
			else
			{
				specSymbol->AddChild_NFb(L"$GenericArg", argumentSymbol);
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
		spec = Ptr(new SpecializationSpec);
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

		// if we see TYPE(), then it should be a function type
		if (ending == CppTokens::GT)
		{
			if (auto ctorExpr = argument.expr.Cast<CtorAccessExpr>())
			{
				if (ctorExpr->initializer->initializerType == CppInitializerType::Constructor && ctorExpr->initializer->arguments.Count() == 0)
				{
					auto funcType = Ptr(new FunctionType);
					funcType->returnType = ctorExpr->type;

					argument.type = funcType;
					argument.expr = nullptr;
				}
			}
		}

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