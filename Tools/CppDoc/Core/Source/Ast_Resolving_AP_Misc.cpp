#include "Ast_Resolving_AP_CalculateGpa.h"

using namespace symbol_type_resolving;
using namespace infer_function_type;

namespace assign_parameters
{
	/***********************************************************************
	AssignParameterAssignment: Assign arguments to parameters according to the GPA mapping
	***********************************************************************/

	void AssignParameterAssignment(
		const ParsingArguments& invokerPa,
		vint parameterCount,
		TypeTsysList& parameterAssignment,
		GpaList& gpaMappings,
		Array<ExprTsysItem>& argumentTypes
	)
	{
		for (vint i = 0; i < parameterCount; i++)
		{
			auto gpa = gpaMappings[i];

			switch (gpa.kind)
			{
			case GenericParameterAssignmentKind::DefaultValue:
				{
					// if a default value is expected to fill this template argument
					parameterAssignment.Add(nullptr);
				}
				break;
			case GenericParameterAssignmentKind::OneArgument:
				{
					// if an offered argument is to fill this template argument
					parameterAssignment.Add(ApplyExprTsysType(argumentTypes[gpa.index].tsys, argumentTypes[gpa.index].type));
				}
				break;
			case GenericParameterAssignmentKind::EmptyVta:
				{
					// if an empty pack of offered arguments is to fill this variadic template argument
					Array<ExprTsysItem> items;
					auto init = invokerPa.tsys->InitOf(items);
					parameterAssignment.Add(init);
				}
				break;
			case GenericParameterAssignmentKind::MultipleVta:
				{
					// if a pack of offered arguments is to fill this variadic template argument
					Array<ExprTsysItem> items(gpa.count);

					for (vint j = 0; j < gpa.count; j++)
					{
						items[j] = argumentTypes[gpa.index + j];
					}
					auto init = invokerPa.tsys->InitOf(items);
					parameterAssignment.Add(init);
				}
				break;
			case GenericParameterAssignmentKind::Any:
				{
					// if any is to fill this (maybe variadic) template argument
					parameterAssignment.Add(invokerPa.tsys->Any());
				}
				break;
			default:
				// missing arguments are not allowed
				throw TypeCheckerException();
				break;
			}
		}
	}

	/***********************************************************************
	CalculateParameterPackSize: Calculate pack size from involedTypes and involvedExprs from a pattern
		all variadic template arguments used in a pattern are required to have the same pack size
		this function returns -1 if the pack size is unknown, and crash if pack size has confliction
	***********************************************************************/

	vint CalculateParameterPackSize(
		const ParsingArguments& invokerPa,
		const TemplateArgumentContext& knownArguments,
		const SortedList<Type*>& involvedTypes,
		const SortedList<Expr*>& involvedExprs
	)
	{
		vint packSize = -1;
		bool conflicted = false;
		CollectInvolvedVariadicArguments(invokerPa, involvedTypes, involvedExprs, [&invokerPa, &packSize, &conflicted, &knownArguments](Symbol*, ITsys* pattern)
		{
			vint index = knownArguments.arguments.Keys().IndexOf(pattern);
			ITsys* pack = nullptr;
			if (index != -1)
			{
				pack = knownArguments.arguments.Values()[index];
			}
			else
			{
				invokerPa.TryGetReplacedGenericArg(pattern, pack);
			}

			if (pack && pack->GetType() == TsysType::Init)
			{
				vint newPackSize = pack->GetParamCount();
				if (packSize == -1)
				{
					packSize = newPackSize;
				}
				else if (packSize != newPackSize)
				{
					conflicted = true;
				}
			}
		});

		if (conflicted)
		{
			// all assigned variadic template arguments that are used in a function parameter should have the same pack size
			throw TypeCheckerException();
		}
		else
		{
			return packSize;
		}
	}

	/***********************************************************************
	AdjustPaForCollecting: Adjust pa so that taContext of parentDeclType is used accordingly
		this adjustment is only valid for ResolveGenericTypeParameters and ResolveFunctionParameters
		because only tsys and TryGetReplacedGenericArg is used
	***********************************************************************/

	ParsingArguments AdjustPaForCollecting(const ParsingArguments& pa)
	{
		auto invokerPa = pa;
		if (!pa.taContext && pa.parentDeclType)
		{
			auto& di = pa.parentDeclType->GetDeclInstant();
			invokerPa.parentDeclType = di.parentDeclType;
			invokerPa.taContext = di.taContext.Obj();
		}
		return invokerPa;
	}
}