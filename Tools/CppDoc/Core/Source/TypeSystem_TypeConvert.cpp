#include "Ast_Resolving.h"

using TCITestedSet = SortedList<Tuple<ITsys*, ITsys*>>;
TypeConv TestTypeConversionInternal(const ParsingArguments& pa, ITsys* toType, ITsys* fromType, TCITestedSet& tested);

namespace TestTypeConversion_Impl
{
	TypeConv TestTypeConversionImpl(const ParsingArguments& pa, ITsys* toType, ITsys* fromType, TCITestedSet& tested)
	{
	}
}
using namespace TestTypeConversion_Impl;

TypeConv TestTypeConversionInternal(const ParsingArguments& pa, ITsys* toType, ITsys* fromType, TCITestedSet& tested)
{
	Tuple<ITsys*, ITsys*> pair(toType, fromType);
	if (tested.Contains(pair))
	{
		// make sure caller doesn't call this function recursively for the same pair of type.
		struct TestConvertInternalStackOverflowException {};
		throw TestConvertInternalStackOverflowException();
	}

	vint index = tested.Add(pair);
	auto result = TestTypeConversionImpl(pa, toType, fromType, tested);
	tested.RemoveAt(index);
	return result;
}

TypeConv TestTypeConversion(const ParsingArguments& pa, ITsys* toType, ExprTsysItem fromItem)
{
	TCITestedSet tested;
	return TestTypeConversionInternal(pa, toType, ApplyExprTsysType(fromItem.tsys, fromItem.type), tested);
}