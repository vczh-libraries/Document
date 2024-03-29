#include "IFT.h"
#include "AP.h"
#include "EvaluateSymbol.h"
#include "Ast_Evaluate_ExpandPotentialVta.h"

using namespace symbol_type_resolving;
using namespace assign_parameters;

namespace infer_function_type
{
	/***********************************************************************
	InferPartialSpecialization:	Perform type inferencing for partial specialization symbol
	***********************************************************************/

	bool IsPSEquivalentTypeCached(const ParsingArguments& pa, ITsys* a, ITsys* b, bool forFilteringPSInstance, Dictionary<Pair<ITsys*, ITsys*>, bool>& visited);

	bool IsPSEquivalentToAny(ITsys* t)
	{
		switch (t->GetType())
		{
		case TsysType::GenericArg:
			return true;
		case TsysType::LRef:
		case TsysType::RRef:
		case TsysType::Ptr:
		case TsysType::Array:
		case TsysType::Member:
		case TsysType::CV:
			return IsPSEquivalentToAny(t->GetElement());
		default:
			return false;
		}
	}

	bool IsPSEquivalentTypeEnsuredPrimary(const ParsingArguments& pa, ITsys* a, ITsys* b, bool forFilteringPSInstance, Dictionary<Pair<ITsys*, ITsys*>, bool>& visited)
	{
		if (a == b) return true;
		if (forFilteringPSInstance)
		{
			if (b->GetType() == TsysType::Any || b->GetType() == TsysType::GenericArg)
			{
				return true;
			}
		}
		if (a->GetType() == TsysType::Any) return IsPSEquivalentToAny(b);
		if (b->GetType() == TsysType::Any) return IsPSEquivalentToAny(a);
		
		switch (a->GetType())
		{
		case TsysType::LRef:
		case TsysType::RRef:
		case TsysType::Ptr:
			return a->GetType() == b->GetType() && IsPSEquivalentTypeCached(pa, a->GetElement(), b->GetElement(), forFilteringPSInstance, visited);
		case TsysType::Array:
			{
				if (b->GetType() != TsysType::Array) return false;
				vint adim = a->GetParamCount();
				vint bdim = b->GetParamCount();
				vint min = adim < bdim ? adim : bdim;
				auto ae = adim == min ? a->GetElement() : a->GetElement()->ArrayOf(adim - min);
				auto be = bdim == min ? b->GetElement() : b->GetElement()->ArrayOf(bdim - min);
				if (!IsPSEquivalentTypeCached(pa, ae, be, forFilteringPSInstance, visited)) return false;
			}
			return true;
		case TsysType::CV:
			return a->GetType() == b->GetType() && a->GetCV() == b->GetCV() && IsPSEquivalentTypeCached(pa, a->GetElement(), b->GetElement(), forFilteringPSInstance, visited);
		case TsysType::Member:
			return a->GetType() == b->GetType() && IsPSEquivalentTypeCached(pa, a->GetClass(), b->GetClass(), forFilteringPSInstance, visited) && IsPSEquivalentTypeCached(pa, a->GetElement(), b->GetElement(), forFilteringPSInstance, visited);
		case TsysType::Init:
			if (b->GetType() != TsysType::Init) return false;
			if (a->GetParamCount() != b->GetParamCount()) return false;
			for (vint i = 0; i < a->GetParamCount(); i++)
			{
				if (!IsPSEquivalentTypeCached(pa, a->GetParam(i), b->GetParam(i), forFilteringPSInstance, visited)) return false;
			}
			return true;
		case TsysType::Function:
			if (b->GetType() != TsysType::Function) return false;
			if (a->GetFunc().ellipsis != b->GetFunc().ellipsis) return false;
			if (a->GetParamCount() != b->GetParamCount()) return false;
			if (!IsPSEquivalentTypeCached(pa, a->GetElement(), b->GetElement(), forFilteringPSInstance, visited)) return false;
			for (vint i = 0; i < a->GetParamCount(); i++)
			{
				if (!IsPSEquivalentTypeCached(pa, a->GetParam(i), b->GetParam(i), forFilteringPSInstance, visited)) return false;
			}
			return true;
		case TsysType::DeclInstant:
			{
				if (b->GetType() != TsysType::DeclInstant) return false;
				if (a->GetDecl() != b->GetDecl()) return false;
				if (a->GetParamCount() != b->GetParamCount()) return false;
				if (!IsPSEquivalentTypeCached(pa, a->GetElement(), b->GetElement(), forFilteringPSInstance, visited)) return false;
				for (vint i = 0; i < a->GetParamCount(); i++)
				{
					if (!IsPSEquivalentTypeCached(pa, a->GetParam(i), b->GetParam(i), forFilteringPSInstance, visited)) return false;
				}
			}
			return true;
		case TsysType::GenericArg:
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

	bool IsPSEquivalentTypeCached(const ParsingArguments& pa, ITsys* a, ITsys* b, bool forFilteringPSInstance, Dictionary<Pair<ITsys*, ITsys*>, bool>& visited)
	{
		if (a == b) return true;
		if (a && b)
		{
			bool exit = false;
			EnumerateClassPrimaryInstances(pa, a, true, [&](ITsys* primaryA)
			{
				EnumerateClassPrimaryInstances(pa, b, true, [&](ITsys* primaryB)
				{
					Pair<ITsys*, ITsys*> key = { primaryA,primaryB };
					vint index = visited.Keys().IndexOf(key);
					bool result = false;
					if (index == -1)
					{
						result = IsPSEquivalentTypeEnsuredPrimary(pa, primaryA, primaryB, forFilteringPSInstance, visited);
						visited.Set(key, result);
					}
					else
					{
						result = visited.Values()[index];
					}
					if (result)
					{
						exit = true;
					}
					return exit;
				});
				return exit;
			});
			return exit;
		}
		else
		{
			return false;
		}
	}

	bool IsPSEquivalentType(const ParsingArguments& pa, ITsys* a, ITsys* b, bool forFilteringPSInstance)
	{
		Dictionary<Pair<ITsys*, ITsys*>, bool> visited;
		return IsPSEquivalentTypeCached(pa, a, b, forFilteringPSInstance, visited);
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

			auto taContext = Ptr(new TemplateArgumentContext(declSymbol, templateSpec->arguments.Count()));
			taContext->parent = inferPa.taContext;

			// assign parameters
			ResolveSpecializationSpecParameters(inferPa, parameterAssignment, *taContext.Obj(), freeTypeSymbols, specializationSpec.Obj(), argumentTypes, boundedAnys);

			// type inferencing
			{
				for (vint i = 0; i < templateSpec->arguments.Count(); i++)
				{
					auto argument = templateSpec->arguments[i];
					auto pattern = symbol_type_resolving::GetTemplateArgumentKey(argument);
					taContext->SetValueByKey(pattern, pa.tsys->Any());;
				}
				TemplateArgumentContext unusedVariadicContext(nullptr, 0);
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
					if (!From(tsys).Any([=](ITsys* a) { return IsPSEquivalentType(pa, a, assignedTsys, true); }))
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
		for (vint i = 0; i < taContext->GetArgumentCount(); i++)
		{
			if (taContext->IsArgumentAvailable(i))
			{
				auto key = taContext->GetKey(i);
				auto tsys = taContext->GetValue(i);
				if (tsys)
				{
					auto patternSymbol = TemplateArgumentPatternToSymbol(key);

					if (patternSymbol->ellipsis)
					{
						if (tsys->GetType() == TsysType::Any)
						{
							return true;
						}
						else
						{
							for (vint j = 0; j < tsys->GetParamCount(); j++)
							{
								if (auto tsysItem = tsys->GetParam(j))
								{
									if (tsysItem->GetType() == TsysType::Any || tsysItem->GetType() == TsysType::GenericArg)
									{
										return true;
									}
								}
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
			}
		}
		return false;
	}
}