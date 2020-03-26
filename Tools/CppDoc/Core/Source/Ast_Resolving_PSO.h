#ifndef VCZH_DOCUMENT_CPPDOC_AST_RESOLVING_PSO
#define VCZH_DOCUMENT_CPPDOC_AST_RESOLVING_PSO

#include "Ast_Resolving_IFT.h"

namespace partial_specification_ordering
{
	struct MatchPSFailureException {};

	extern void							AssignPSPrimary(
											const ParsingArguments& pa,
											Ptr<CppTokenCursor>& cursor,
											Symbol* symbol
										);

	extern bool							IsPSAncestor(
											const ParsingArguments& pa,
											Symbol* symbolA,
											Ptr<TemplateSpec> tA,
											Ptr<SpecializationSpec> psA,
											Symbol* symbolB,
											Ptr<TemplateSpec> tB,
											Ptr<SpecializationSpec> psB
										);

	enum class MatchPSKind
	{
		Single,			// this template argument is not variant
		Variant,		// this template argument is variant, the result consist of multiple type/value or type/value pack
		PartOfVariant,	// this template argument is variant, the result is part of another type/value pack
	};

	struct MatchPSResult
	{
		MatchPSKind						kind;
		vint							start = -1;		// for PartOfVariant, the amount of the skipped prefix from source[0]
		vint							stop = -1;		// for PartOfVariant, the amount of the skipped postfix from source[0]
		VariadicList<GenericArgument>	source;		// for Single and PartOfVariant, source should have exactly one element
	};

	extern								void MatchPSArgument(
											const ParsingArguments& pa,
											const Dictionary<Symbol*, Ptr<MatchPSResult>>& matchingResult,
											const Dictionary<Symbol*, Ptr<MatchPSResult>>& matchingResultVta,
											Ptr<Type> ancestor,
											Ptr<Type> child,
											bool insideVariant,
											const SortedList<Symbol*>& freeTypeSymbols,
											SortedList<Type*>& involvedTypes,
											SortedList<Expr*>& involvedExprs
										);

	extern void							MatchPSAncestorArguments(
											const ParsingArguments& pa,
											const Dictionary<Symbol*, Ptr<MatchPSResult>>& matchingResult,
											const Dictionary<Symbol*, Ptr<MatchPSResult>>& matchingResultVta,
											SpecializationSpec* ancestor,
											SpecializationSpec* child,
											const SortedList<Symbol*>& freeTypeSymbols
										);

	extern void							MatchPSAncestorArguments(
											const ParsingArguments& pa,
											const Dictionary<Symbol*, Ptr<MatchPSResult>>& matchingResult,
											const Dictionary<Symbol*, Ptr<MatchPSResult>>& matchingResultVta,
											FunctionType* ancestor,
											FunctionType* child,
											bool insideVariant,
											const SortedList<Symbol*>& freeTypeSymbols
										);

	extern void							MatchPSAncestorArguments(
											const ParsingArguments& pa,
											const Dictionary<Symbol*, Ptr<MatchPSResult>>& matchingResult,
											const Dictionary<Symbol*, Ptr<MatchPSResult>>& matchingResultVta,
											GenericType* ancestor,
											GenericType* child,
											bool insideVariant,
											const SortedList<Symbol*>& freeTypeSymbols
										);
	}

#endif