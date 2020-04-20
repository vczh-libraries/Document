#include "AP_CalculateGpa.h"

using namespace infer_function_type;

namespace assign_parameters
{
	void ResolveGenericArgumentParameters(
		const ParsingArguments& invokerPa,					// context
		bool forGenericType,								// true for GenericType, false for SpecializationSpec
		TypeTsysList& parameterAssignment,					// store generic argument to offered argument map, nullptr indicates the default value is applied
		const TemplateArgumentContext& knownArguments,		// all assigned template arguments
		const SortedList<Symbol*>& argumentSymbols,			// symbols of all template arguments
		VariadicList<GenericArgument>& genericArguments,	// argument information
		Array<ExprTsysItem>& argumentTypes,					// (index of unpacked)		offered argument (unpacked)
		SortedList<vint>& boundedAnys						// (value of unpacked)		for each offered argument that is any_t, and it means unknown variadic arguments, instead of an unknown type
	)
	{
		auto adjustedPa = AdjustPaForCollecting(invokerPa);

		vint genericParameterCount = genericArguments.Count();
		vint passedParameterCount = forGenericType ? genericParameterCount + 1 : genericParameterCount;
		Array<vint> knownPackSizes(passedParameterCount);
		for (vint i = 0; i < knownPackSizes.Count(); i++)
		{
			knownPackSizes[i] = -1;
		}

		// calculate all variadic function arguments that with known pack size
		for (vint i = 0; i < genericArguments.Count(); i++)
		{
			auto& argument = genericArguments[i];
			if (argument.isVariadic)
			{
				SortedList<Type*> involvedTypes;
				SortedList<Expr*> involvedExprs;
				CollectFreeTypes(adjustedPa, true, argument.item.type, argument.item.expr, false, argumentSymbols, involvedTypes, involvedExprs);
				knownPackSizes[i] = CalculateParameterPackSize(adjustedPa, knownArguments, involvedTypes, involvedExprs);
			}
			else
			{
				knownPackSizes[i] = 1;
			}
		}

		// calculate how to assign offered arguments to generic type arguments
		// gpaMappings will contains decisions for every template arguments
		// for GenericType:
		//   set allowPartialApply to true because, arguments of genericType could be incomplete, but argumentTypes are always complete because it comes from a ITsys*
		// for SpecializationSpec
		//   set allowPartialApply to false because, arguments of SpecializationSpec are always complete
		GpaList gpaMappings;
		CalculateGpa(gpaMappings, argumentTypes.Count(), boundedAnys, 0, forGenericType, passedParameterCount, knownPackSizes,
			[&genericArguments](vint index) { return index == genericArguments.Count() ? true : genericArguments[index].isVariadic; },
			[](vint index) { return false; }
		);
		AssignParameterAssignment(adjustedPa, genericParameterCount, parameterAssignment, gpaMappings, argumentTypes);
	}

	/***********************************************************************
	ResolveGenericTypeParameters: Fill parameterAssignment by matching type/value arguments to template arguments
	***********************************************************************/

	void ResolveGenericTypeParameters(
		const ParsingArguments& invokerPa,
		TypeTsysList& parameterAssignment,
		const TemplateArgumentContext& knownArguments,
		const SortedList<Symbol*>& argumentSymbols,
		GenericType* genericType,
		Array<ExprTsysItem>& argumentTypes,
		SortedList<vint>& boundedAnys
	)
	{
		ResolveGenericArgumentParameters(invokerPa, true, parameterAssignment, knownArguments, argumentSymbols, genericType->arguments, argumentTypes, boundedAnys);
	}

	/***********************************************************************
	ResolveSpecializationSpecParameters: Fill parameterAssignment by matching type/value arguments to template arguments
	***********************************************************************/

	void ResolveSpecializationSpecParameters(
		const ParsingArguments& invokerPa,
		TypeTsysList& parameterAssignment,
		const TemplateArgumentContext& knownArguments,
		const SortedList<Symbol*>& argumentSymbols,
		SpecializationSpec* spec,
		Array<ExprTsysItem>& argumentTypes,
		SortedList<vint>& boundedAnys
	)
	{
		ResolveGenericArgumentParameters(invokerPa, false, parameterAssignment, knownArguments, argumentSymbols, spec->arguments, argumentTypes, boundedAnys);
	}
}