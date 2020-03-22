#ifndef VCZH_DOCUMENT_CPPDOC_AST_RESOLVING_IFT
#define VCZH_DOCUMENT_CPPDOC_AST_RESOLVING_IFT

#include "Ast_Resolving.h"
#include "Ast_Expr.h"

namespace infer_function_type
{
	extern Symbol*		TemplateArgumentPatternToSymbol(ITsys* tsys);

	extern void			CollectFreeTypes(
							const ParsingArguments& pa,
							bool includeParentDeclArguments,
							Ptr<Type> type,
							Ptr<Expr> expr,
							bool insideVariant,
							const SortedList<Symbol*>& freeTypeSymbols,
							SortedList<Type*>& involvedTypes,
							SortedList<Expr*>& involvedExprs
						);

	extern void			InferTemplateArgument(
							const ParsingArguments& pa,
							Ptr<Type> typeToInfer,
							Ptr<Expr> exprToInfer,
							ITsys* offeredType,
							TemplateArgumentContext& taContext,
							TemplateArgumentContext& variadicContext,
							const SortedList<Symbol*>& freeTypeSymbols,
							const SortedList<Type*>& involvedTypes,
							const SortedList<Expr*>& involvedExprs,
							bool exactMatchForParameters,
							ITsys** lastAssignedVta,
							SortedList<ITsys*>& hardcodedPatterns
						);

	extern void			SetInferredResult(
							const ParsingArguments& pa,
							TemplateArgumentContext& taContext,
							ITsys* pattern,
							ITsys* type,
							ITsys** lastAssignedVta,
							SortedList<ITsys*>& hardcodedPatterns
						);

	extern void			InferTemplateArgumentsForGenericType(
							const ParsingArguments& pa,
							GenericType* genericType,
							TypeTsysList& parameterAssignment,
							TemplateArgumentContext& taContext,
							TemplateArgumentContext& variadicContext,
							const SortedList<Symbol*>& freeTypeSymbols,
							ITsys** lastAssignedVta,
							SortedList<ITsys*>& hardcodedPatterns
						);

	extern void			InferTemplateArgumentsForFunctionType(
							const ParsingArguments& pa,
							FunctionType* functionType,
							TypeTsysList& parameterAssignment,
							TemplateArgumentContext& taContext,
							TemplateArgumentContext& variadicContext,
							const SortedList<Symbol*>& freeTypeSymbols,
							bool exactMatchForParameters,
							ITsys** lastAssignedVta,
							SortedList<ITsys*>& hardcodedPatterns
						);


	extern void			InferFunctionType(
							const ParsingArguments& pa,
							ExprTsysList& inferredFunctionTypes,
							ExprTsysItem funcType,
							Array<ExprTsysItem>& argTypes,
							SortedList<vint>& boundedAnys
						);

	template<typename TCallback>
	void CollectInvolvedVariadicArguments(const ParsingArguments& pa, const SortedList<Type*>& involvedTypes, const SortedList<Expr*>& involvedExprs, TCallback&& callback)
	{
		for (vint j = 0; j < involvedTypes.Count(); j++)
		{
			if (auto idType = dynamic_cast<IdType*>(involvedTypes[j]))
			{
				auto patternSymbol = idType->resolving->resolvedSymbols[0];
				auto pattern = symbol_type_resolving::GetTemplateArgumentKey(patternSymbol, pa.tsys.Obj());
				if (patternSymbol->ellipsis)
				{
					callback(patternSymbol, pattern);
				}
			}
		}

		for (vint j = 0; j < involvedExprs.Count(); j++)
		{
			if (auto idExpr = dynamic_cast<IdExpr*>(involvedExprs[j]))
			{
				auto patternSymbol = idExpr->resolving->resolvedSymbols[0];
				auto pattern = symbol_type_resolving::GetTemplateArgumentKey(patternSymbol, pa.tsys.Obj());
				if (patternSymbol->ellipsis)
				{
					callback(patternSymbol, pattern);
				}
			}
		}
	}
}

#endif