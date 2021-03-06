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
DeclOrArg
***********************************************************************/

vint DeclOrArg::Compare(const DeclOrArg& a, const DeclOrArg& b)
{
	if (a.decl < b.decl) return -1;
	if (a.decl > b.decl) return 1;
	if (a.symbol < b.symbol) return -1;
	if (a.symbol > b.symbol) return 1;
	return 0;
}

/***********************************************************************
IndexRecorder
***********************************************************************/

IndexRecorder::IndexRecorder(IndexResult& _result, IProgressReporter* _progressReporter, vint _codeLength)
	:result(_result)
	, progressReporter(_progressReporter)
	, codeLength(_codeLength)
{
}

void IndexRecorder::BeginPhase(Phase phase)
{
	currentPhase = (vint)phase;
}

void IndexRecorder::BeginDelayParse(FunctionDeclaration* decl)
{
	if (currentPhase == 1)
	{
		if (stack == 0)
		{
			if (decl->name && progressReporter)
			{
				progressReporter->OnProgress(currentPhase, decl->name.nameTokens[0].start, codeLength);
			}
		}
		stack++;
	}
}

void IndexRecorder::EndDelayParse(FunctionDeclaration* decl)
{
	if (currentPhase == 1)
	{
		stack--;
	}
}

void IndexRecorder::BeginEvaluate(Declaration* decl)
{
	if (currentPhase == 2)
	{
		if (stack == 0)
		{
			if (decl->name && progressReporter)
			{
				progressReporter->OnProgress(currentPhase, decl->name.nameTokens[0].start, codeLength);
			}
		}
		stack++;
	}
}

void IndexRecorder::EndEvaluate(Declaration* decl)
{
	if (currentPhase == 2)
	{
		stack--;
	}
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
			}
		}

		if (progressReporter && currentPhase == 0)
		{
			progressReporter->OnProgress(currentPhase, name.nameTokens[0].start, codeLength);
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