#ifndef VCZH_DOCUMENT_CPPDOC_PSO
#define VCZH_DOCUMENT_CPPDOC_PSO

#include "Lexer.h"
#include "Symbol.h"
#include "Ast_Type.h"

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

	extern Ptr<Type>					FixFunctionParameterType(Ptr<Type> t);

	struct MatchPSResult
	{
		List<Ptr<Type>>					source;

		static bool Compare(const Ptr<MatchPSResult>& a, const Ptr<MatchPSResult>& b, bool forParameter)
		{
			if (a->source.Count() != b->source.Count()) return false;
			{
				Dictionary<WString, WString> equivalentNames;
				for (vint i = 0; i < a->source.Count(); i++)
				{
					auto at = a->source[i];
					auto bt = b->source[i];
					if (forParameter)
					{
						at = FixFunctionParameterType(at);
						bt = FixFunctionParameterType(bt);
					}
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
											GenericArgument ancestor,
											GenericArgument child,
											bool insideVariant,
											const SortedList<Symbol*>& freeAncestorSymbols,
											const SortedList<Symbol*>& freeChildSymbols,
											SortedList<Type*>& involvedTypes,
											SortedList<Expr*>& involvedExprs,
											bool forParameter
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