#include "Ast_Resolving_PSO.h"

namespace partial_specification_ordering
{
	/***********************************************************************
	MatchPSAncestorArguments (SpecializationSpec)
	***********************************************************************/

	void MatchPSAncestorArguments(
		const ParsingArguments& pa,
		const Dictionary<Symbol*, Ptr<MatchPSResult>>& matchingResult,
		const Dictionary<Symbol*, Ptr<MatchPSResult>>& matchingResultVta,
		Ptr<SpecializationSpec> ancestor,
		Ptr<SpecializationSpec> child,
		bool insideVariant,
		const SortedList<Symbol*>& freeTypeSymbols
	)
	{
		throw TypeCheckerException();
	}

	/***********************************************************************
	MatchPSAncestorArguments (FunctionType)
	***********************************************************************/

	void MatchPSAncestorArguments(
		const ParsingArguments& pa,
		const Dictionary<Symbol*, Ptr<MatchPSResult>>& matchingResult,
		const Dictionary<Symbol*, Ptr<MatchPSResult>>& matchingResultVta,
		Ptr<FunctionType> ancestor,
		Ptr<FunctionType> child,
		bool insideVariant,
		const SortedList<Symbol*>& freeTypeSymbols
	)
	{
		throw TypeCheckerException();
	}

	/***********************************************************************
	MatchPSAncestorArguments (GenericType)
	***********************************************************************/

	void MatchPSAncestorArguments(
		const ParsingArguments& pa,
		const Dictionary<Symbol*, Ptr<MatchPSResult>>& matchingResult,
		const Dictionary<Symbol*, Ptr<MatchPSResult>>& matchingResultVta,
		Ptr<GenericType> ancestor,
		Ptr<GenericType> child,
		bool insideVariant,
		const SortedList<Symbol*>& freeTypeSymbols
	)
	{
		throw TypeCheckerException();
	}
}