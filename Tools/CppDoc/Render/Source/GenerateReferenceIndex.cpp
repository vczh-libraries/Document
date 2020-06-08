#include "Render.h"
#include <VlppParser.h>

using namespace vl::parsing::xml;

/***********************************************************************
ValidateAndFixDocumentRecord
***********************************************************************/

void ValidateAndFixDocumentRecord(
	Ptr<GlobalLinesRecord> global,
	Symbol* symbol,
	Ptr<DocumentRecord> documentRecord,
	StreamWriter& writer
)
{
	writer.WriteLine(L"<Document>");
	for (vint i = 0; i < documentRecord->comments.Count(); i++)
	{
		auto& token = documentRecord->comments[i];
		if (token.token == (vint)CppTokens::DOCUMENT)
		{
			writer.WriteLine(token.reading + 3, token.length - 3);
		}
		else
		{
			writer.WriteLine(token.reading + 2, token.length - 2);
		}
	}
	writer.WriteLine(L"</Document>");
}

/***********************************************************************
RenderDocumentRecord
***********************************************************************/

void RenderDocumentRecord(
	Ptr<GlobalLinesRecord> global,
	Ptr<SymbolGroup> symbolGroup,
	const WString& fileGroupPrefix,
	const FilePath& pathReference,
	IProgressReporter* progressReporter,
	vint& writtenReferenceCount
)
{
	if (symbolGroup->kind == SymbolGroupKind::Symbol)
	{
		vint index = global->declComments.Keys().IndexOf(symbolGroup->symbol);
		if (index != -1)
		{
			FileStream fileStream((pathReference / fileGroupPrefix / (symbolGroup->uniqueId + L".xml")).GetFullPath(), FileStream::WriteOnly);
			Utf8Encoder encoder;
			EncoderStream encoderStream(fileStream, encoder);
			StreamWriter referenceWriter(encoderStream);
			ValidateAndFixDocumentRecord(global, symbolGroup->symbol, global->declComments.Values()[index], referenceWriter);
		}
	}

	for (vint i = 0; i < symbolGroup->children.Count(); i++)
	{
		auto childGroup = symbolGroup->children[i];
		RenderDocumentRecord(global, childGroup, fileGroupPrefix, pathReference, progressReporter, writtenReferenceCount);
	}

	writtenReferenceCount++;
	if (progressReporter)
	{
		progressReporter->OnProgress((vint)IProgressReporter::ExtraPhases::ReferenceIndex, writtenReferenceCount, global->declComments.Count());
	}
}

/***********************************************************************
GenerateReferenceIndex
***********************************************************************/

void GenerateReferenceIndex(
	Ptr<GlobalLinesRecord> global,
	IndexResult& result,
	Ptr<SymbolGroup> rootGroup,
	FilePath pathHtml,
	FilePath pathReference,
	FileGroupConfig& fileGroups,
	SortedList<WString>& predefinedGroups,
	IProgressReporter* progressReporter
)
{
	vint writtenReferenceCount = 0;
	for (vint i = 0; i < rootGroup->children.Count(); i++)
	{
		auto fileGroup = rootGroup->children[i];
		if (predefinedGroups.Contains(fileGroup->name))
		{
			Folder(pathReference / fileGroup->uniqueId).Create(true);
			RenderDocumentRecord(global, fileGroup, fileGroup->uniqueId, pathReference, progressReporter, writtenReferenceCount);
		}
	}
}