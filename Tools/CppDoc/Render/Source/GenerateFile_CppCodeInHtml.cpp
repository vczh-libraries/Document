#include "Render.h"

/***********************************************************************
GenerateCppCodeInHtml
***********************************************************************/

void GenerateCppCodeInHtml(Ptr<FileLinesRecord> flr, StreamWriter& writer)
{
	List<WString> originalLines;
	File(flr->filePath).ReadAllLinesByBom(originalLines);

	vint originalIndex = 0;
	vint flrIndex = 0;
	while (originalIndex < originalLines.Count())
	{
		vint disableEnd = -1;
		vint nextProcessingLine = -1;
		WString embedHtmlInDisabled;

		if (flrIndex == flr->lines.Count())
		{
			disableEnd = originalLines.Count();
			nextProcessingLine = disableEnd;
		}
		else
		{
			vint nextAvailable = flr->lines.Keys()[flrIndex];
			if (originalIndex < nextAvailable)
			{
				disableEnd = nextAvailable;
				nextProcessingLine = disableEnd;
			}
			else if (originalIndex == nextAvailable)
			{
				auto& currentHtmlLines = flr->lines.Values()[flrIndex++];
				if (flrIndex == flr->lines.Count())
				{
					nextProcessingLine = originalLines.Count();
				}
				else
				{
					nextProcessingLine = flr->lines.Keys()[flrIndex];
				}

				bool rawCodeMatched = (nextProcessingLine - originalIndex) >= currentHtmlLines.lineCount;
				if (rawCodeMatched)
				{
					StringReader reader(WString(currentHtmlLines.rawBegin, (vint)(currentHtmlLines.rawEnd - currentHtmlLines.rawBegin)));
					for (vint i = 0; i < currentHtmlLines.lineCount; i++)
					{
						if (originalLines[originalIndex + i] != reader.ReadLine())
						{
							rawCodeMatched = false;
							break;
						}
					}
				}

				if (rawCodeMatched)
				{
					writer.WriteLine(currentHtmlLines.htmlCode);
					nextProcessingLine = originalIndex + currentHtmlLines.lineCount;
				}
				else
				{
					bool allSpaces = true;
					for (auto reading = currentHtmlLines.rawBegin; allSpaces && reading < currentHtmlLines.rawEnd; reading++)
					{
						switch (*reading++)
						{
						case L' ':
						case L'\t':
						case L'\r':
						case L'\n':
						case L'\v':
						case L'\f':
							break;
						default:
							allSpaces = false;
						}
					}

					if (!allSpaces)
					{
						embedHtmlInDisabled = currentHtmlLines.htmlCode;
					}
					disableEnd = nextProcessingLine;
				}
			}
			else
			{
				throw Exception(L"Too many lines are processed.");
			}
		}

		if (disableEnd != -1)
		{
			bool hasEmbeddedHtml = embedHtmlInDisabled.Length() != 0;
			if (hasEmbeddedHtml)
			{
				writer.WriteString(L"<div class=\"expandable\">");
			}
			writer.WriteString(L"<div class=\"disabled\">");
			for (vint i = originalIndex; i < disableEnd; i++)
			{
				if (i > originalIndex)
				{
					writer.WriteLine(L"");
				}
				WriteHtmlTextSingleLine(originalLines[i], writer);
			}
			writer.WriteLine(L"</div>");
			if (hasEmbeddedHtml)
			{
				writer.WriteString(L"<div class=\"expanded\">");
				writer.WriteString(embedHtmlInDisabled);
				writer.WriteLine(L"</div></div>");
			}
		}
		originalIndex = nextProcessingLine;
	}
}