#ifndef VCZH_DOCUMENT_CPPDOC_AST_RESOLVING_IFT
#define VCZH_DOCUMENT_CPPDOC_AST_RESOLVING_IFT

#include "Ast_Resolving.h"
#include "Ast_Expr.h"

namespace symbol_type_resolving
{
	extern Symbol*		TemplateArgumentPatternToSymbol(ITsys* tsys);

	extern void			CollectFreeTypes(
							Ptr<Type> type,
							Ptr<Expr> expr,
							bool insideVariant,
							const SortedList<Symbol*>& freeTypeSymbols,
							SortedList<Type*>& involvedTypes,
							SortedList<Expr*>& involvedExprs);

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
							bool exactMatchForParameters);

	extern void			SetInferredResult(
							TemplateArgumentContext& taContext,
							ITsys* pattern,
							ITsys* type);

	extern void			InferTemplateArgumentsForGenericType(
							const ParsingArguments& pa,
							GenericType* genericType,
		TypeTsysList& parameterAssignment,
							TemplateArgumentContext& taContext,
							TemplateArgumentContext& variadicContext,
							const SortedList<Symbol*>& freeTypeSymbols);

	extern void			InferTemplateArgumentsForFunctionType(
							const ParsingArguments& pa,
							FunctionType* functionType,
							TypeTsysList& parameterAssignment,
							TemplateArgumentContext& taContext,
							TemplateArgumentContext& variadicContext,
							const SortedList<Symbol*>& freeTypeSymbols,
							bool exactMatchForParameters);
}

#endif