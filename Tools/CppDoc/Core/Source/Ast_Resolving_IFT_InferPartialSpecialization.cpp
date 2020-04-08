#include "Ast_Resolving_AP.h"
#include "Ast_Evaluate_ExpandPotentialVta.h"

using namespace symbol_type_resolving;
using namespace assign_parameters;

namespace infer_function_type
{
	/***********************************************************************
	InferPartialSpecialization:	Perform type inferencing for partial specialization symbol
	***********************************************************************/

	// assuming template type argument equals to any_t, test if two types are the same
	bool IsPSEquivalentType(ITsys* a, ITsys* b)
	{
		if (a == b) return true;
		
		switch (a->GetType())
		{
		case TsysType::Any:
			// when b is any_t, it doesn't reach here
			return b->GetType() == TsysType::GenericArg;
		case TsysType::GenericArg:
			// if b is not any, then they equal only when two ITsys* pointers equal
			return b->GetType() == TsysType::Any;
		case TsysType::LRef:
		case TsysType::RRef:
		case TsysType::Ptr:
			return a->GetType() == b->GetType() && IsPSEquivalentType(a->GetElement(), b->GetElement());
		case TsysType::Array:
			return a->GetType() == b->GetType() && a->GetParamCount() == b->GetParamCount() && IsPSEquivalentType(a->GetElement(), b->GetElement());
		case TsysType::CV:
			return a->GetType() == b->GetType() && a->GetCV() == b->GetCV() && IsPSEquivalentType(a->GetElement(), b->GetElement());
		case TsysType::Member:
			return a->GetType() == b->GetType() && IsPSEquivalentType(a->GetClass(), b->GetClass()) && IsPSEquivalentType(a->GetElement(), b->GetElement());
		case TsysType::Init:
			if (b->GetType() != TsysType::Init) return false;
			if (a->GetParamCount() != b->GetParamCount()) return false;
			if (!IsPSEquivalentType(a->GetElement(), b->GetElement())) return false;
			for (vint i = 0; i < a->GetParamCount(); i++)
			{
				if (!IsPSEquivalentType(a->GetParam(i), b->GetParam(i))) return false;
			}
			return true;
		case TsysType::Function:
			if (b->GetType() != TsysType::Function) return false;
			if (a->GetFunc().ellipsis != b->GetFunc().ellipsis) return false;
			if (a->GetParamCount() != b->GetParamCount()) return false;
			if (!IsPSEquivalentType(a->GetElement(), b->GetElement())) return false;
			for (vint i = 0; i < a->GetParamCount(); i++)
			{
				if (!IsPSEquivalentType(a->GetParam(i), b->GetParam(i))) return false;
			}
			return true;
		case TsysType::DeclInstant:
			{
				if (b->GetType() != TsysType::DeclInstant) return false;
				if (a->GetDecl() != b->GetDecl()) return false;
				if (a->GetParamCount() != b->GetParamCount()) return false;
				if (!IsPSEquivalentType(a->GetElement(), b->GetElement())) return false;
				for (vint i = 0; i < a->GetParamCount(); i++)
				{
					if (!IsPSEquivalentType(a->GetParam(i), b->GetParam(i))) return false;
				}
			}
			return true;
		case TsysType::Zero:
		case TsysType::Nullptr:
		case TsysType::Primitive:
		case TsysType::Decl:
		case TsysType::GenericFunction:
			// equal only when two ITsys* pointers equal
			return false;
		default:
			return false;
		}
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