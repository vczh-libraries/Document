#include "Render.h"

/***********************************************************************
GenerateFileIndex
***********************************************************************/

void GenerateFileIndexRecord(WString prefix, WString indentation, Ptr<FileLinesRecord> flr, StreamWriter& writer)
{
	writer.WriteString(indentation);
	writer.WriteString(L"<a class=\"fileIndex\" href=\"./");
	WriteHtmlAttribute(flr->htmlFileName, writer);
	writer.WriteString(L".html\">");
	WriteHtmlTextSingleLine(flr->filePath.GetFullPath().Right(flr->filePath.GetFullPath().Length() - prefix.Length()), writer);
	writer.WriteLine(L"</a><br>");
}

void GenerateFileIndexInFolder(WString prefix, List<Ptr<FileLinesRecord>>& flrs, StreamWriter& writer)
{
	for (vint i = 0; i < flrs.Count(); i++)
	{
		auto flr = flrs[i];
		GenerateFileIndexRecord(prefix, L"&nbsp;&nbsp;&nbsp;&nbsp;", flr, writer);
	}
}

void GenerateFileIndex(Ptr<GlobalLinesRecord> global, FilePath pathHtml, FileGroupConfig& fileGroups)
{
	FileStream fileStream(pathHtml.GetFullPath(), FileStream::WriteOnly);
	Utf8Encoder encoder;
	EncoderStream encoderStream(fileStream, encoder);
	StreamWriter writer(encoderStream);

	writer.WriteLine(L"<!DOCTYPE html>");
	writer.WriteLine(L"<html>");
	writer.WriteLine(L"<head>");
	writer.WriteLine(L"    <title>File Index</title>");
	writer.WriteLine(L"    <link rel=\"stylesheet\" href=\"../Cpp.css\" />");
	writer.WriteLine(L"    <link rel=\"shortcut icon\" href=\"../favicon.ico\" />");
	writer.WriteLine(L"</head>");
	writer.WriteLine(L"<body>");
	writer.WriteLine(L"<a class=\"button\" href=\"./FileIndex.html\">File Index</a>");
	writer.WriteLine(L"<a class=\"button\" href=\"./SymbolIndex.html\">Symbol Index</a>");
	writer.WriteLine(L"<br>");
	writer.WriteLine(L"<br>");

	List<Ptr<FileLinesRecord>> flrs;
	CopyFrom(
		flrs,
		From(global->fileLines.Values())
		.OrderBy([](Ptr<FileLinesRecord> flr1, Ptr<FileLinesRecord> flr2)
		{
			return WString::Compare(flr1->filePath.GetFullPath(), flr2->filePath.GetFullPath());
		})
	);
	for (vint i = 0; i < fileGroups.Count(); i++)
	{
		auto prefix = fileGroups[i].f0;
		List<Ptr<FileLinesRecord>> selectedFlrs;

		for (vint j = 0; j < flrs.Count(); j++)
		{
			auto flr = flrs[j];
			if (INVLOC.StartsWith(flr->filePath.GetFullPath(), prefix, Locale::Normalization::IgnoreCase))
			{
				selectedFlrs.Add(flr);
				flrs.RemoveAt(j--);
			}
		}

		writer.WriteString(L"<span class=\"fileGroupLabel\">");
		WriteHtmlTextSingleLine(fileGroups[i].f1, writer);
		writer.WriteLine(L"</span><br>");
		GenerateFileIndexInFolder(prefix, selectedFlrs, writer);
	}
	if (flrs.Count() > 0)
	{
		writer.WriteLine(L"<span class=\"fileGroupLabel\">MISC</span><br>");

		for (vint j = 0; j < flrs.Count(); j++)
		{
			auto flr = flrs[j];
			GenerateFileIndexRecord(L"", L"&nbsp;&nbsp;&nbsp;&nbsp;", flr, writer);
		}
	}
	writer.WriteLine(L"</body>");
	writer.WriteLine(L"</html>");
}