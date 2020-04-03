#ifndef VCZH_DOCUMENT_CPPDOC_AST_RESOLVING_AP
#define VCZH_DOCUMENT_CPPDOC_AST_RESOLVING_AP

#include "Ast_Resolving_IFT.h"

namespace assign_parameters
{
	struct GenericParameterAssignment;
	using GpaList = List<GenericParameterAssignment>;

	extern Symbol*					TemplateArgumentPatternToSymbol(ITsys* tsys);

	extern void						AssignParameterAssignment(
										const ParsingArguments& invokerPa,
										vint parameterCount,
										TypeTsysList& parameterAssignment,
										GpaList& gpaMappings,
										Array<ExprTsysItem>& argumentTypes
									);

	extern vint						CalculateParameterPackSize(
										const ParsingArguments& invokerPa,
										const TemplateArgumentContext& knownArguments,
										const SortedList<Type*>& involvedTypes,
										const SortedList<Expr*>& involvedExprs
									);

	extern ParsingArguments			AdjustPaForCollecting(
										const ParsingArguments& pa
									);

	extern void						ResolveGenericParameters(
										const ParsingArguments& invokerPa,
										TemplateArgumentContext& newTaContext,
										ITsys* genericFunction,
										Array<ExprTsysItem>& argumentTypes,
										Array<bool>& isTypes,
										Array<vint>& argSource,
										SortedList<vint>& boundedAnys,
										vint offset,
										bool allowPartialApply,
										vint& partialAppliedArguments
									);

	extern void						ResolveFunctionParameters(
										const ParsingArguments& invokerPa,
										TypeTsysList& parameterAssignment,
										const TemplateArgumentContext& knownArguments,
										const SortedList<Symbol*>& argumentSymbols,
										ITsys* lastAssignedVta,
										FunctionType* functionType,
										Array<ExprTsysItem>& argumentTypes,
										SortedList<vint>& boundedAnys
									);

	extern void						ResolveGenericTypeParameters(
										const ParsingArguments& invokerPa,
										TypeTsysList& parameterAssignment,
										const TemplateArgumentContext& knownArguments,
										const SortedList<Symbol*>& argumentSymbols,
										GenericType* genericType,
										Array<ExprTsysItem>& argumentTypes,
										SortedList<vint>& boundedAnys
									);

	extern void						ResolveSpecializationSpecParameters(
										const ParsingArguments& invokerPa,
										TypeTsysList& parameterAssignment,
										const TemplateArgumentContext& knownArguments,
										const SortedList<Symbol*>& argumentSymbols,
										SpecializationSpec* spec,
										Array<ExprTsysItem>& argumentTypes,
										SortedList<vint>& boundedAnys
									);

	template<typename TCallback>
	void FillFreeSymbols(const ParsingArguments& pa, const Ptr<TemplateSpec>& spec, SortedList<Symbol*>& freeSymbols, TCallback&& callback = ([](vint, TemplateSpec::Argument&, ITsys*, Symbol*) {}))
	{
		for (vint i = 0; i < spec->arguments.Count(); i++)
		{
			auto argument = spec->arguments[i];
			auto pattern = symbol_type_resolving::GetTemplateArgumentKey(argument, pa.tsys.Obj());
			auto patternSymbol = TemplateArgumentPatternToSymbol(pattern);
			freeSymbols.Add(patternSymbol);
			callback(i, argument, pattern, patternSymbol);
		}
	}

	inline void FillFreeSymbols(const ParsingArguments& pa, const Ptr<TemplateSpec>& spec, SortedList<Symbol*>& freeSymbols)
	{
		FillFreeSymbols(pa, spec, freeSymbols, [](vint, TemplateSpec::Argument&, ITsys*, Symbol*) {});
	}
}

#endif