#include "Symbol_TemplateSpec.h"
#include "EvaluateSymbol.h"

namespace symbol_type_resolving
{

	/***********************************************************************
	GetTemplateSpecFromDecl: Get TempalteSpec from a declaration if it is a generic declaration
	***********************************************************************/

	Ptr<TemplateSpec> GetTemplateSpecFromDecl(Ptr<Declaration> decl)
	{
		if (auto funcDecl = decl.Cast<ForwardFunctionDeclaration>())
		{
			return funcDecl->templateSpec;
		}
		else if (auto classDecl = decl.Cast<ForwardClassDeclaration>())
		{
			return classDecl->templateSpec;
		}
		else if (auto typeAliasDecl = decl.Cast<TypeAliasDeclaration>())
		{
			return typeAliasDecl->templateSpec;
		}
		else if (auto valueAliasDecl = decl.Cast<ValueAliasDeclaration>())
		{
			return valueAliasDecl->templateSpec;
		}
		else
		{
			return nullptr;
		}
	}

	/***********************************************************************
	GetTemplateSpecFromSymbol: Get TempalteSpec from a symbol if it is a generic declaration
	***********************************************************************/

	Ptr<TemplateSpec> GetTemplateSpecFromSymbol(Symbol* symbol)
	{
		if (symbol)
		{
			if (auto funcDecl = symbol->GetAnyForwardDecl<ForwardFunctionDeclaration>())
			{
				return funcDecl->templateSpec;
			}
			else if (auto classDecl = symbol->GetAnyForwardDecl<ForwardClassDeclaration>())
			{
				return classDecl->templateSpec;
			}
			else if (auto typeAliasDecl = symbol->GetAnyForwardDecl<TypeAliasDeclaration>())
			{
				return typeAliasDecl->templateSpec;
			}
			else if (auto valueAliasDecl = symbol->GetAnyForwardDecl<ValueAliasDeclaration>())
			{
				return valueAliasDecl->templateSpec;
			}
		}
		return nullptr;
	}

	/***********************************************************************
	GetTemplateArgumentKey: Get an ITsys* for TsysGenericFunction keys
	***********************************************************************/

	ITsys* GetTemplateArgumentKey(const TemplateSpec::Argument& argument)
	{
		return GetTemplateArgumentKey(argument.argumentSymbol);
	}

	ITsys* GetTemplateArgumentKey(Symbol* argumentSymbol)
	{
		switch (argumentSymbol->kind)
		{
		case symbol_component::SymbolKind::GenericTypeArgument:
		case symbol_component::SymbolKind::GenericValueArgument:
			return argumentSymbol->GetEvaluationForUpdating_NFb().GetExtra(0)[0];
		default:
			throw TypeCheckerException();
		}
	}

	/***********************************************************************
	TemplateArgumentPatternToSymbol:	Get the symbol from a type representing a template argument
	***********************************************************************/

	Symbol* TemplateArgumentPatternToSymbol(ITsys* tsys)
	{
		switch (tsys->GetType())
		{
		case TsysType::GenericArg:
			return tsys->GetGenericArg().argSymbol;
		default:
			throw TypeCheckerException();
		}
	}

	/***********************************************************************
	CreateGenericFunctionHeader: Calculate enough information to create a generic function type
	***********************************************************************/

	void CreateGenericFunctionHeader(const ParsingArguments& pa, Symbol* declSymbol, ITsys* parentDeclType, Ptr<TemplateSpec> spec, TypeTsysList& params, TsysGenericFunction& genericFunction)
	{
		if (!declSymbol) throw TypeCheckerException();
		if (!spec) throw TypeCheckerException();

		genericFunction.declSymbol = declSymbol;
		genericFunction.parentDeclType = parentDeclType;
		genericFunction.spec = spec;
		for (vint i = 0; i < spec->arguments.Count(); i++)
		{
			const auto& argument = spec->arguments[i];
			params.Add(GetTemplateArgumentKey(argument));
		}
	}

	/***********************************************************************
	EnsureGenericTypeParameterAndArgumentMatched: Ensure values and types are passed in correct order
	***********************************************************************/

	void EnsureGenericFunctionParameterAndArgumentMatched(ITsys* parameter, ITsys* argument);

	void EnsureGenericTypeParameterAndArgumentMatched(ITsys* parameter, ITsys* argument)
	{
		switch (parameter->GetType())
		{
		case TsysType::GenericArg:
			if (argument->GetType() == TsysType::GenericFunction)
			{
				throw TypeCheckerException();
			}
			break;
		case TsysType::GenericFunction:
			if (argument->GetType() != TsysType::GenericFunction)
			{
				throw TypeCheckerException();
			}
			EnsureGenericFunctionParameterAndArgumentMatched(parameter, argument);
			break;
		default:
			// until class specialization begins to develop, this should always not happen
			throw TypeCheckerException();
		}
	}

	void EnsureGenericFunctionParameterAndArgumentMatched(ITsys* parameter, ITsys* argument)
	{
		if (parameter->GetParamCount() != argument->GetParamCount())
		{
			throw TypeCheckerException();
		}

		auto& pGF = parameter->GetGenericFunction();
		auto& aGF = argument->GetGenericFunction();
		for (vint i = 0; i < parameter->GetParamCount(); i++)
		{
			auto nestedParameter = parameter->GetParam(i);
			auto nestedArgument = argument->GetParam(i);

			auto pT = pGF.spec->arguments[i].argumentType;
			auto aT = aGF.spec->arguments[i].argumentType;
			if (pT != aT)
			{
				throw TypeCheckerException();
			}

			if (pT != CppTemplateArgumentType::Value)
			{
				EnsureGenericTypeParameterAndArgumentMatched(
					EvaluateGenericArgumentType(nestedParameter->GetGenericArg().argSymbol),
					EvaluateGenericArgumentType(nestedArgument->GetGenericArg().argSymbol)
				);
			}
		}
	}
}