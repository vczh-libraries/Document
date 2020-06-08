#include "Render.h"

/***********************************************************************
GenerateReferenceIndex
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
			auto dr = global->declComments.Values()[index];

			FileStream fileStream((pathReference / fileGroupPrefix / (symbolGroup->uniqueId + L".xml")).GetFullPath(), FileStream::WriteOnly);
			Utf8Encoder encoder;
			EncoderStream encoderStream(fileStream, encoder);
			StreamWriter referenceWriter(encoderStream);

			referenceWriter.WriteLine(L"<Document>");
			for (vint i = 0; i < dr->comments.Count(); i++)
			{
				auto& token = dr->comments[i];
				if (token.token == (vint)CppTokens::DOCUMENT)
				{
					referenceWriter.WriteLine(token.reading + 3, token.length - 3);
				}
				else
				{
					referenceWriter.WriteLine(token.reading + 2, token.length - 2);
				}
			}
			referenceWriter.WriteLine(L"</Document>");
		}
	}

	if (symbolGroup->kind == SymbolGroupKind::Root)
	{
		for (vint i = 0; i < symbolGroup->children.Count(); i++)
		{
			auto childGroup = symbolGroup->children[i];
			RenderDocumentRecord(global, childGroup, childGroup->uniqueId, pathReference, progressReporter, writtenReferenceCount);
		}
	}
	else
	{
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
	IProgressReporter* progressReporter
)
{
	for (vint i = 0; i < rootGroup->children.Count(); i++)
	{
		Folder(pathReference / rootGroup->children[i]->uniqueId).Create(true);
	}

	vint writtenReferenceCount = 0;
	RenderDocumentRecord(global, rootGroup, L"", pathReference, progressReporter, writtenReferenceCount);
}