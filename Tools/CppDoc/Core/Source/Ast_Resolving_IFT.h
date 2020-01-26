#include "Ast_Resolving.h"

namespace symbol_type_resolving
{
	extern void			CollectFreeTypes(
							Ptr<Type> type,
							bool insideVariant,
							const SortedList<Symbol*>& freeTypeSymbols,
							SortedList<Type*>& involvedTypes);

	extern void			InferTemplateArgument(
							const ParsingArguments& pa,
							Ptr<Type> argumentType,
							ITsys* offeredType,
							TemplateArgumentContext& taContext,
							TemplateArgumentContext& variadicContext,
							const SortedList<Symbol*>& freeTypeSymbols,
							const SortedList<Type*>& involvedTypes,
							bool exactMatchForParameters);

	extern void			SetInferredResult(
							TemplateArgumentContext& taContext,
							ITsys* pattern,
							ITsys* type);

	extern void			InferTemplateArgumentsForFunctionType(
							const ParsingArguments& pa,
							ClassDeclaration* genericType,
							List<ITsys*>& parameterAssignment,
							TemplateArgumentContext& taContext,
							const SortedList<Symbol*>& freeTypeSymbols);

	extern void			InferTemplateArgumentsForFunctionType(
							const ParsingArguments& pa,
							FunctionType* functionType,
							List<ITsys*>& parameterAssignment,
							TemplateArgumentContext& taContext,
							const SortedList<Symbol*>& freeTypeSymbols,
							bool exactMatchForParameters);
}