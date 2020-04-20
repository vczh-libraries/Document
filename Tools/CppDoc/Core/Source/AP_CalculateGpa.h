#ifndef VCZH_DOCUMENT_CPPDOC_AST_RESOLVING_AP_CALCULATEGPA
#define VCZH_DOCUMENT_CPPDOC_AST_RESOLVING_AP_CALCULATEGPA

#include "AP.h"

namespace assign_parameters
{
	enum class GenericParameterAssignmentKind
	{
		DefaultValue,
		OneArgument,
		EmptyVta,
		MultipleVta,
		Any,
		Unfilled,
	};

	struct GenericParameterAssignment
	{
		GenericParameterAssignmentKind		kind;
		vint								index;
		vint								count;

		GenericParameterAssignment() = default;
		GenericParameterAssignment(GenericParameterAssignmentKind _kind, vint _index, vint _count)
			:kind(_kind)
			, index(_index)
			, count(_count)
		{
		}

		static GenericParameterAssignment DefaultValue()
		{
			return { GenericParameterAssignmentKind::DefaultValue,-1,-1 };
		}

		static GenericParameterAssignment OneArgument(vint index)
		{
			return { GenericParameterAssignmentKind::OneArgument,index,-1 };
		}

		static GenericParameterAssignment EmptyVta()
		{
			return { GenericParameterAssignmentKind::EmptyVta,-1,-1 };
		}

		static GenericParameterAssignment MultipleVta(vint index, vint count)
		{
			return { GenericParameterAssignmentKind::MultipleVta,index,count };
		}

		static GenericParameterAssignment Any()
		{
			return { GenericParameterAssignmentKind::Any,-1,-1 };
		}

		static GenericParameterAssignment Unfilled()
		{
			return { GenericParameterAssignmentKind::Unfilled,-1,-1 };
		}
	};

	/***********************************************************************
	CalculateGpa: Calculate how arguments are grouped and passed to template
	***********************************************************************/

	template<typename TIsVta, typename THasDefault>
	void CalculateGpa(
		GpaList& gpaMappings,							// list to store results
		vint inputArgumentCount,						// number of offered arguments
		SortedList<vint>& boundedAnys,					// boundedAnys[x] == i + offset means the i-th offered argument is any_t, and it means unknown variadic arguments, instead of an unknown type
		vint offset,									// offset to use with boundedAnys
		bool allowPartialApply,							// true means it is legal to not offer enough amount of arguments
		vint templateArgumentCount,						// number of template argument
		Array<vint>& knownPackSizes,					// pack size of all template arguments, -1 for unknown, empty array to ignore
		TIsVta&& argIsVat,								// test if a template argument is variadic
		THasDefault&& argHasDefault						// test if a template argument has a default value
	)
	{
		// both template arguments and offered arguments could have unknown pack size
		// for template argument, it satisfies knownPackSizes[argumentIndex] == -1
		// for offered argument, it satisfies boundedAnys[x] == argumentIndex

		// fill knownPackSizes if empty
		if (knownPackSizes.Count() == 0)
		{
			knownPackSizes.Resize(templateArgumentCount);
			for (vint i = 0; i < templateArgumentCount; i++)
			{
				knownPackSizes[i] = argIsVat(i) ? -1 : 1;
			}
		}

		// first offered argument with unknown pack size
		vint firstBoundedAny = boundedAnys.Count() > 0 ? boundedAnys[0] - offset : -1;

		// when given up, fill the rest of arguments with any_t
		bool givenUp = false;

		vint readingInput = 0;
		bool seenVat = false;

		for (vint i = 0; i < templateArgumentCount; i++)
		{
			if (givenUp)
			{
				gpaMappings.Add(GenericParameterAssignment::Any());
			}
			else
			{
				if (seenVat)
				{
					// if there are arguments after the first variadic template argument with unknown pack size
					if (allowPartialApply)
					{
						// since all offered arguments has been used, unfill them for both variadic or non-variadic template argument
						gpaMappings.Add(GenericParameterAssignment::Unfilled());
					}
					else
					{
						// not enough offered arguments
						throw TypeCheckerException();
					}
				}
				else if (argIsVat(i))
				{
					// for variadic template argument, there is no default values
					vint packSize = knownPackSizes[i];
					if (packSize == -1)
					{
						// if the pack size is unknown
						seenVat = true;
						if (firstBoundedAny != -1)
						{
							// if there is any offered argument with unknown pack size, give up
							givenUp = true;
							i--;
							readingInput = inputArgumentCount;
							continue;
						}
						else
						{
							// give all remaining offered arguments to it
							if (readingInput == inputArgumentCount)
							{
								gpaMappings.Add(GenericParameterAssignment::EmptyVta());
							}
							else
							{
								gpaMappings.Add(GenericParameterAssignment::MultipleVta(readingInput + offset, inputArgumentCount - readingInput));
								readingInput = inputArgumentCount;
							}
						}
					}
					else
					{
						// if the pack size is known
						if (firstBoundedAny != -1 && readingInput + packSize > firstBoundedAny)
						{
							// an offered argument with unknown pack size has been read, give up
							givenUp = true;
							i--;
							readingInput = inputArgumentCount;
							continue;
						}
						else if (readingInput + packSize <= inputArgumentCount)
						{
							// if there are still enough offered arguments to apply
							if (packSize == 0)
							{
								gpaMappings.Add(GenericParameterAssignment::EmptyVta());
							}
							else
							{
								gpaMappings.Add(GenericParameterAssignment::MultipleVta(readingInput + offset, packSize));
								readingInput += packSize;
							}
						}
						else if (allowPartialApply)
						{
							// if partial applying is allowed
							gpaMappings.Add(GenericParameterAssignment::Unfilled());
						}
						else
						{
							// not enough offered arguments
							throw TypeCheckerException();
						}
					}
				}
				else
				{
					// for non-variadic template argument
					if (firstBoundedAny != -1 && readingInput >= firstBoundedAny)
					{
						// an offered argument with unknown pack size has been read, give up
						givenUp = true;
						i--;
						readingInput = inputArgumentCount;
						continue;
					}
					else if (readingInput < inputArgumentCount)
					{
						// if there is still an offered argument to apply
						gpaMappings.Add(GenericParameterAssignment::OneArgument(readingInput + offset));
						readingInput++;
					}
					else if (argHasDefault(i))
					{
						// if there is a default value
						gpaMappings.Add(GenericParameterAssignment::DefaultValue());
					}
					else if (allowPartialApply)
					{
						// if partial applying is allowed
						gpaMappings.Add(GenericParameterAssignment::Unfilled());
					}
					else
					{
						// not enough offered arguments
						throw TypeCheckerException();
					}
				}
			}
		}

		if (!givenUp && readingInput < inputArgumentCount)
		{
			// check if all the rest are offered arguments with unknown pack size
			// since all these offered arguments has not been read, just check the count
			vint remainingCount = inputArgumentCount - readingInput;
			if (boundedAnys.Count() != remainingCount)
			{
				// too many offered arguments
				throw TypeCheckerException();
			}
		}
	}
}

#endif