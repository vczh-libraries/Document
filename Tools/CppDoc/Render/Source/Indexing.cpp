#include "Render.h"

/***********************************************************************
IndexToken
***********************************************************************/

vint IndexToken::Compare(const IndexToken& a, const IndexToken& b)
{
	vint result;
	if ((result = a.rowStart - b.rowStart) != 0) return result;
	if ((result = a.columnStart - b.columnStart) != 0) return result;
	if ((result = a.rowEnd - b.rowEnd) != 0) return result;
	if ((result = a.columnEnd - b.columnEnd) != 0) return result;
	return 0;
}

IndexToken IndexToken::GetToken(CppName& name)
{
	return {
		name.nameTokens[0].rowStart,
		name.nameTokens[0].columnStart,
		name.nameTokens[name.tokenCount - 1].rowEnd,
		name.nameTokens[name.tokenCount - 1].columnEnd,
	};
}

/***********************************************************************
IndexRecorder
***********************************************************************/

IndexRecorder::IndexRecorder(IndexResult& _result)
	:result(_result)
{
}

void IndexRecorder::IndexInternal(CppName& name, List<ResolvedItem>& resolvedSymbols, IndexReason reason)
{
	auto key = IndexToken::GetToken(name);
	if (name.tokenCount > 0)
	{
		for (vint i = 0; i < resolvedSymbols.Count(); i++)
		{
			auto symbol = resolvedSymbols[i].symbol;
			if (!result.index[(vint)reason].Contains(key, symbol))
			{
				result.index[(vint)reason].Add(key, symbol);
				result.reverseIndex[(vint)reason].Add(symbol, key);
			}
		}
	}
}

void IndexRecorder::Index(CppName& name, List<ResolvedItem>& resolvedSymbols)
{
	IndexInternal(name, resolvedSymbols, IndexReason::Resolved);
}

void IndexRecorder::IndexOverloadingResolution(CppName& name, List<ResolvedItem>& resolvedSymbols)
{
	IndexInternal(name, resolvedSymbols, IndexReason::OverloadedResolution);
}

void IndexRecorder::ExpectValueButType(CppName& name, List<ResolvedItem>& resolvedSymbols)
{
	IndexInternal(name, resolvedSymbols, IndexReason::NeedValueButType);
}