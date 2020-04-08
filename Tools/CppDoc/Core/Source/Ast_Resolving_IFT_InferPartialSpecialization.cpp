#include "Ast_Resolving_AP.h"
#include "Ast_Evaluate_ExpandPotentialVta.h"

using namespace symbol_type_resolving;
using namespace assign_parameters;

namespace infer_function_type
{
	/***********************************************************************
	InferPartialSpecialization:	Perform type inferencing for partial specialization symbol
	***********************************************************************/

	bool IsPSEquivalentType(ITsys* a, ITsys* b)
	{
		if (a == b) return true;
		return false;
	}

	Ptr<TemplateArgumentContext> InferPartialSpecialization(
		const ParsingArguments& pa,
		Symbol* declSymbol,
		ITsys* parentDeclType,
		Ptr<TemplateSpec> templateSpec,
		Ptr<SpecializationSpec> specializationSpec,
		Array<ExprTsysItem>& argumentTypes,
		SortedList<vint>& boundedAnys
	)
	{
		try
		{
			TypeTsysList						parameterAssignment;
			SortedList<Symbol*>					freeTypeSymbols;
			ITsys*								lastAssignedVta = nullptr;

			// fill freeTypeSymbols with all template arguments
			FillFreeSymbols(pa, templateSpec, freeTypeSymbols);

			// adjusting context
			auto inferPa = pa.AdjustForDecl(declSymbol);
			inferPa.parentDeclType = ParsingArguments::AdjustDeclInstantForScope(declSymbol, parentDeclType, true);

			auto taContext = MakePtr<TemplateArgumentContext>();
			taContext->parent = inferPa.taContext;

			// assign parameters
			ResolveSpecializationSpecParameters(inferPa, parameterAssignment, *taContext.Obj(), freeTypeSymbols, specializationSpec.Obj(), argumentTypes, boundedAnys);

			// type inferencing
			{
				for (vint i = 0; i < templateSpec->arguments.Count(); i++)
				{
					auto argument = templateSpec->arguments[i];
					auto pattern = symbol_type_resolving::GetTemplateArgumentKey(argument, pa.tsys.Obj());
					taContext->arguments.Add(pattern, pa.tsys->Any());;
				}
				TemplateArgumentContext unusedVariadicContext;
				SortedList<ITsys*> unusedHardcodedPatterns;
				InferTemplateArgumentsForSpecializationSpec(inferPa, specializationSpec.Obj(), parameterAssignment, *taContext.Obj(), unusedVariadicContext, freeTypeSymbols, &lastAssignedVta, unusedHardcodedPatterns);
			}

			// ensure that inferred types are exactly expected types
			auto verifyPa = inferPa;
			verifyPa.taContext = taContext.Obj();
			for (vint i = 0; i < specializationSpec->arguments.Count(); i++)
			{
				auto argument = specializationSpec->arguments[i];
				if (argument.item.type)
				{
					TypeTsysList tsys;
					auto assignedTsys = parameterAssignment[i];

					bool isVta = false;
					TypeToTsysInternal(verifyPa, argument.item.type, tsys, isVta);
					if (!From(tsys).Any([=](ITsys* a) { return IsPSEquivalentType(a, assignedTsys); }))
					{
						return nullptr;
					}
				}
			}

			taContext->parent = nullptr;
			return taContext;
		}
		catch (const TypeCheckerException&)
		{
			// ignore this candidate if failed to match
		}
		return nullptr;
	}

	/***********************************************************************
	IsValuableTaContextWithMatchedPSChildren:	
	***********************************************************************/

	bool IsValuableTaContextWithMatchedPSChildren(TemplateArgumentContext* taContext)
	{
		for (vint i = 0; i < taContext->arguments.Count(); i++)
		{
			auto patternSymbol = TemplateArgumentPatternToSymbol(taContext->arguments.Keys()[i]);
			auto tsys = taContext->arguments.Values()[i];
			if (patternSymbol->ellipsis)
			{
				for (vint j = 0; j < tsys->GetParamCount(); j++)
				{
					auto tsysItem = tsys->GetParam(j);
					if (tsysItem->GetType() == TsysType::Any || tsysItem->GetType() == TsysType::GenericArg)
					{
						return true;
					}
				}
			}
			else
			{
				if (tsys->GetType() == TsysType::Any || tsys->GetType() == TsysType::GenericArg)
				{
					return true;
				}
			}
		}
		return false;
	}
}