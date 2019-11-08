#include "Render.h"

extern void GenerateCppCodeInHtml(Ptr<FileLinesRecord> flr, StreamWriter& writer);
extern void GenerateReferencedSymbols(Ptr<FileLinesRecord> flr, StreamWriter& writer);
extern void GenerateSymbolToFiles(Ptr<GlobalLinesRecord> global, Ptr<FileLinesRecord> flr, StreamWriter& writer);

/***********************************************************************
GenerateFile
***********************************************************************/

void GenerateFile(Ptr<GlobalLinesRecord> global, Ptr<FileLinesRecord> flr, IndexResult& result, FilePath pathHtml)
{
	FileStream fileStream(pathHtml.GetFullPath(), FileStream::WriteOnly);
	Utf8Encoder encoder;
	EncoderStream encoderStream(fileStream, encoder);
	StreamWriter writer(encoderStream);

	writer.WriteLine(L"<!DOCTYPE html>");
	writer.WriteLine(L"<html>");
	writer.WriteLine(L"<head>");
	writer.WriteLine(L"    <title>" + flr->filePath.GetName() + L"</title>");
	writer.WriteLine(L"    <link rel=\"stylesheet\" href=\"../Cpp.css\" />");
	writer.WriteLine(L"    <link rel=\"shortcut icon\" href=\"../favicon.ico\" />");
	writer.WriteLine(L"    <script type=\"text/javascript\" src=\"../Cpp.js\" ></script>");
	writer.WriteLine(L"</head>");
	writer.WriteLine(L"<body>");
	writer.WriteLine(L"<a class=\"button\" href=\"./FileIndex.html\">File Index</a>");
	writer.WriteLine(L"<a class=\"button\" href=\"./SymbolIndex.html\">Symbol Index</a>");
	writer.WriteLine(L"<br>");
	writer.WriteLine(L"<br>");

	writer.WriteString(L"<div class=\"codebox\"><div class=\"cpp_default\">");
	GenerateCppCodeInHtml(flr, writer);
	writer.WriteLine(L"</div></div>");

	writer.WriteLine(L"<script type=\"text/javascript\">");
	GenerateReferencedSymbols(flr, writer);
	GenerateSymbolToFiles(global, flr, writer);
	writer.WriteLine(L"");
	writer.WriteLine(L"};");
	writer.WriteLine(L"turnOnSymbol();");

	writer.WriteLine(L"</script>");
	writer.WriteLine(L"</body>");
	writer.WriteLine(L"</html>");
}