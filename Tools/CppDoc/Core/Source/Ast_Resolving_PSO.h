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

	struct MatchPSResult
	{
		// [start, stop] are valid to be not -1 when source[last] contains variadic template arguments
		// [x,y]: skip an x-elements prefix, skip a y-elements postfix
		// [x,-1]: skip an x-elements prefix, and pick only one item
		vint							start = -1;
		vint							stop = -1;
		VariadicList<Ptr<Type>>			source;

		static bool Compare(const Ptr<MatchPSResult>& a, const Ptr<MatchPSResult>& b)
		{
			if (a->start != b->start) return false;
			if (a->stop != b->stop) return false;
			if (a->source.Count() != b->source.Count()) return false;
			{
				Dictionary<WString, WString> equivalentNames;
				for (vint i = 0; i < a->source.Count(); i++)
				{
					auto at = a->source[i];
					auto bt = b->source[i];
					if (at.isVariadic != bt.isVariadic) return false;
					if (at.item && bt.item)
					{
						if (!IsSameResolvedType(at.item, bt.item, equivalentNames)) return false;
					}
					else if (at.item || bt.item)
					{
						return false;
					}
				}
			}
			return true;
		}
	};

	extern								void MatchPSArgument(
											const ParsingArguments& pa,
											bool& skipped,
											Dictionary<Symbol*, Ptr<MatchPSResult>>& matchingResult,
											Dictionary<Symbol*, Ptr<MatchPSResult>>& matchingResultVta,
											Ptr<Type> ancestor,
											Ptr<Type> child,
											bool insideVariant,
											const SortedList<Symbol*>& freeTypeSymbols,
											SortedList<Type*>& involvedTypes,
											SortedList<Expr*>& involvedExprs
										);

	extern void							MatchPSAncestorArguments(
											const ParsingArguments& pa,
											SpecializationSpec* ancestor,
											SpecializationSpec* child,
											const SortedList<Symbol*>& freeTypeSymbols
										);

	extern void							MatchPSAncestorArguments(
											const ParsingArguments& pa,
											bool& skipped,
											Dictionary<Symbol*, Ptr<MatchPSResult>>& matchingResult,
											Dictionary<Symbol*, Ptr<MatchPSResult>>& matchingResultVta,
											FunctionType* ancestor,
											FunctionType* child,
											bool insideVariant,
											const SortedList<Symbol*>& freeTypeSymbols
										);

	extern void							MatchPSAncestorArguments(
											const ParsingArguments& pa,
											bool& skipped,
											Dictionary<Symbol*, Ptr<MatchPSResult>>& matchingResult,
											Dictionary<Symbol*, Ptr<MatchPSResult>>& matchingResultVta,
											GenericType* ancestor,
											GenericType* child,
											bool insideVariant,
											const SortedList<Symbol*>& freeTypeSymbols
										);
	}

#endif