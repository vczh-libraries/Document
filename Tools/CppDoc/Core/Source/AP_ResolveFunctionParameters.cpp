#include "AP_CalculateGpa.h"

using namespace infer_function_type;

namespace assign_parameters
{
	/***********************************************************************
	ResolveFunctionParameters: Fill parameterAssignment by matching arguments to function parameters
	***********************************************************************/

	void ResolveFunctionParameters(
		const ParsingArguments& invokerPa,				// context
		TypeTsysList& parameterAssignment,				// store function argument to offered argument map, nullptr indicates the default value is applied
		const TemplateArgumentContext& knownArguments,	// all assigned template arguments
		const SortedList<Symbol*>& argumentSymbols,		// symbols of all template arguments
		ITsys* lastAssignedVta,							// symbol of the last template argument, if it is assigned and is variadic
		FunctionType* functionType,						// argument information
		Array<ExprTsysItem>& argumentTypes,				// (index of unpacked)		offered argument (unpacked)
		SortedList<vint>& boundedAnys					// (value of unpacked)		for each offered argument that is any_t, and it means unknown variadic arguments, instead of an unknown type
	)
	{
		auto adjustedPa = AdjustPaForCollecting(invokerPa);

		vint functionParameterCount = functionType->parameters.Count();
		vint passedParameterCount = functionParameterCount + (functionType->ellipsis ? 1 : 0);
		Array<vint> knownPackSizes(passedParameterCount);
		for (vint i = 0; i < knownPackSizes.Count(); i++)
		{
			knownPackSizes[i] = -1;
		}

		// calculate all variadic function arguments that with known pack size
		for (vint i = 0; i < functionType->parameters.Count(); i++)
		{
			auto& parameter = functionType->parameters[i];
			if (parameter.isVariadic)
			{
				SortedList<Type*> involvedTypes;
				SortedList<Expr*> involvedExprs;
				CollectFreeTypes(adjustedPa, true, parameter.item->type, nullptr, false, argumentSymbols, involvedTypes, involvedExprs);
				knownPackSizes[i] = CalculateParameterPackSize(adjustedPa, knownArguments, involvedTypes, involvedExprs);

				if (lastAssignedVta && i == functionType->parameters.Count() - 1)
				{
					// if all involved template argument is the last variadic template argument, set to -1 to allow extra arguments
					bool allow = true;
					CollectInvolvedArguments(
						invokerPa,
						involvedTypes,
						involvedExprs,
						[&allow, lastAssignedVta](Symbol*, ITsys* pattern, bool isVariadic)
						{
							if (isVariadic && pattern != lastAssignedVta)
							{
								allow = false;
							}
						});
					if (allow)
					{
						knownPackSizes[i] = -1;
					}
				}
			}
			else
			{
				knownPackSizes[i] = 1;
			}
		}

		// calculate how to assign offered arguments to function arguments
		// gpaMappings will contains decisions for every template arguments
		GpaList gpaMappings;
		CalculateGpa(gpaMappings, argumentTypes.Count(), boundedAnys, 0, false, passedParameterCount, knownPackSizes,
			[functionType](vint index) { return index == functionType->parameters.Count() ? functionType->ellipsis : functionType->parameters[index].isVariadic; },
			[functionType](vint index) { return index == functionType->parameters.Count() ? false : (bool)functionType->parameters[index].item->initializer; }
		);
		AssignParameterAssignment(adjustedPa, functionParameterCount, parameterAssignment, gpaMappings, argumentTypes);
	}
}