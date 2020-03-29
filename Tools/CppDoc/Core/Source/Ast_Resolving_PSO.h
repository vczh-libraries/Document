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
		vint							variadicItemIndex = -1;
		List<Ptr<Type>>					source;

		static bool Compare(const Ptr<MatchPSResult>& a, const Ptr<MatchPSResult>& b)
		{
			if (a->variadicItemIndex != b->variadicItemIndex) return false;
			if (a->source.Count() != b->source.Count()) return false;
			{
				Dictionary<WString, WString> equivalentNames;
				for (vint i = 0; i < a->source.Count(); i++)
				{
					auto at = a->source[i];
					auto bt = b->source[i];
					if (at && bt)
					{
						if (!IsSameResolvedType(at, bt, equivalentNames)) return false;
					}
					else if (at || bt)
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
											const SortedList<Symbol*>& freeAncestorSymbols,
											const SortedList<Symbol*>& freeChildSymbols,
											SortedList<Type*>& involvedTypes,
											SortedList<Expr*>& involvedExprs
										);

	extern void							MatchPSAncestorArguments(
											const ParsingArguments& pa,
											SpecializationSpec* ancestor,
											SpecializationSpec* child,
											const SortedList<Symbol*>& freeAncestorSymbols,
											const SortedList<Symbol*>& freeChildSymbols
										);

	extern void							MatchPSAncestorArguments(
											const ParsingArguments& pa,
											bool& skipped,
											Dictionary<Symbol*, Ptr<MatchPSResult>>& matchingResult,
											Dictionary<Symbol*, Ptr<MatchPSResult>>& matchingResultVta,
											FunctionType* ancestor,
											FunctionType* child,
											bool insideVariant,
											const SortedList<Symbol*>& freeAncestorSymbols,
											const SortedList<Symbol*>& freeChildSymbols
										);

	extern void							MatchPSAncestorArguments(
											const ParsingArguments& pa,
											bool& skipped,
											Dictionary<Symbol*, Ptr<MatchPSResult>>& matchingResult,
											Dictionary<Symbol*, Ptr<MatchPSResult>>& matchingResultVta,
											GenericType* ancestor,
											GenericType* child,
											bool insideVariant,
											const SortedList<Symbol*>& freeAncestorSymbols,
											const SortedList<Symbol*>& freeChildSymbols
										);
	}

#endif