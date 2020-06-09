#include "Render.h"
#include <VlppParser.h>

using namespace vl::parsing::tabling;
using namespace vl::parsing::xml;

/***********************************************************************
PrintDocumentRecord
***********************************************************************/

void PrintDocumentRecord(
	Ptr<DocumentRecord> documentRecord,
	StreamWriter& writer
)
{
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
}

/***********************************************************************
FixEnumDocumentRecord
***********************************************************************/

void FixEnumDocumentRecord(
	Ptr<GlobalLinesRecord> global,
	Ptr<ParsingTable> parsingTable,
	Symbol* symbolEnum,
	Ptr<XmlDocument> xmlDocument
)
{
	auto& children = symbolEnum->GetChildren_NFb();
	for (vint i = 0; i < children.Count(); i++)
	{
		auto& values = children.GetByIndex(i);
		if (values.Count() == 1)
		{
			auto symbolEnumItem = values[0].childSymbol.Obj();
			if (symbolEnumItem->kind == symbol_component::SymbolKind::EnumItem)
			{
				vint index = global->declComments.Keys().IndexOf(symbolEnumItem);
				if (index != -1)
				{
					auto xmlEnumItemText = GenerateToStream([&](StreamWriter& xmlEnumItemWriter)
					{
						PrintDocumentRecord(global->declComments.Values()[index], xmlEnumItemWriter);
					});

					if (auto xmlEnumItem = XmlParseElement(xmlEnumItemText, parsingTable))
					{
						if (xmlEnumItem->name.value == L"summary")
						{
							xmlEnumItem->name.value = L"enumitem";
							xmlEnumItem->closingName.value = L"enumitem";

							auto attr = MakePtr<XmlAttribute>();
							attr->name.value = L"name";
							attr->value.value = symbolEnumItem->name;
							xmlEnumItem->attributes.Add(attr);
							xmlDocument->rootElement->subNodes.Add(xmlEnumItem);
						}
						else
						{
							Console::WriteLine(L"");
							Console::WriteLine(L"FAILED TO PARSE <summary/>:");
							Console::WriteLine(xmlEnumItemText);
						}
					}
					else
					{
						Console::WriteLine(L"");
						Console::WriteLine(L"FAILED TO PARSE <summary/>:");
						Console::WriteLine(xmlEnumItemText);
					}
				}
			}
		}
	}
}

/***********************************************************************
CheckDocumentRecordSubItem
***********************************************************************/

void CheckSubElement(
	Symbol* symbol,
	Ptr<XmlDocument> xmlDocument,
	const WString& xmlText,
	const WString& elementName,
	const WString& attributeValue
)
{
	if (XmlGetElements(xmlDocument->rootElement, elementName)
		.Where([&](Ptr<XmlElement> e)
		{
			auto attr = XmlGetAttribute(e, L"name");
			return attr ? attr->value.value == attributeValue : false;
		})
		.IsEmpty())
	{
		Console::WriteLine(L"");
		Console::WriteLine(L"MISSING <" + elementName + L" name=\"" + attributeValue + L"\"/> in " + symbol->uniqueId + L":");
		Console::WriteLine(xmlText);
	}
}

void CheckDocumentRecordSubItem(
	Symbol* symbol,
	Ptr<Declaration> decl,
	Ptr<XmlDocument> xmlDocument,
	const WString& xmlText
)
{
	if (auto spec = symbol_type_resolving::GetTemplateSpecFromDecl(decl))
	{
		for (vint i = 0; i < spec->arguments.Count(); i++)
		{
			auto& argument = spec->arguments[i];
			if (argument.name) CheckSubElement(symbol, xmlDocument, xmlText, L"typeparam", argument.name.name);
		}
	}

	if (auto funcDecl = decl.Cast<ForwardFunctionDeclaration>())
	{
		auto funcType = GetTypeWithoutMemberAndCC(funcDecl->type).Cast<FunctionType>();
		for (vint i = 0; i < funcType->parameters.Count(); i++)
		{
			auto varDecl = funcType->parameters[i].item;
			if (varDecl->name) CheckSubElement(symbol, xmlDocument, xmlText, L"param", varDecl->name.name);
		}

		switch (funcDecl->name.type)
		{
		case CppNameType::Normal:
		case CppNameType::Operator:
			{
				if (auto primitiveType = funcType->returnType.Cast<PrimitiveType>())
				{
					if (primitiveType->primitive == CppPrimitiveType::_void)
					{
						break;
					}
				}

				if (!XmlGetElement(xmlDocument->rootElement, L"returns"))
				{
					Console::WriteLine(L"");
					Console::WriteLine(L"MISSING <returns/> in " + symbol->uniqueId + L":");
					Console::WriteLine(xmlText);
				}
			}
			break;
		}
	}

	if (auto enumDecl = decl.Cast<EnumDeclaration>())
	{
		for (vint i = 0; i < enumDecl->items.Count(); i++)
		{
			auto enumItem = enumDecl->items[i];
			CheckSubElement(symbol, xmlDocument, xmlText, L"enumitem", enumItem->name.name);
		}
	}
}

/***********************************************************************
ProcessDocumentRecordHyperLinks
***********************************************************************/

void ProcessDocumentRecordHyperLinks(
	Ptr<GlobalLinesRecord> global,
	Symbol* symbol,
	Ptr<Declaration> decl,
	Ptr<XmlDocument> xmlDocument,
	const WString& xmlText
)
{
	// TODO: [cpp.md] Convert hyper-links to a normalized format: `<symbol docId="optional:SymbolId" declId="DeclId"/>`
}

/***********************************************************************
ValidateAndFixDocumentRecord
***********************************************************************/

void ValidateAndFixDocumentRecord(
	Ptr<GlobalLinesRecord> global,
	Ptr<ParsingTable> parsingTable,
	Symbol* symbol,
	Ptr<DocumentRecord> documentRecord,
	StreamWriter& writer
)
{
	auto xmlText = GenerateToStream([&](StreamWriter& xmlWriter)
	{
		xmlWriter.WriteLine(L"<Document>");
		PrintDocumentRecord(documentRecord, xmlWriter);
		xmlWriter.WriteLine(L"</Document>");
	});

	auto xmlDocument = XmlParseDocument(xmlText, parsingTable);
	if (xmlDocument)
	{
		if (symbol->kind == symbol_component::SymbolKind::Enum)
		{
			FixEnumDocumentRecord(global, parsingTable, symbol, xmlDocument);
		}

		CheckDocumentRecordSubItem(symbol, documentRecord->decl, xmlDocument, xmlText);
		ProcessDocumentRecordHyperLinks(global, symbol, documentRecord->decl, xmlDocument, xmlText);

		XmlPrint(xmlDocument, writer);
	}
	else
	{
		Console::WriteLine(L"");
		Console::WriteLine(L"FAILED TO PARSE:");
		Console::WriteLine(xmlText);
	}
}

/***********************************************************************
RenderDocumentRecord
***********************************************************************/

void RenderDocumentRecord(
	Ptr<GlobalLinesRecord> global,
	Ptr<ParsingTable> parsingTable,
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
			ValidateAndFixDocumentRecord(global, parsingTable, symbolGroup->symbol, global->declComments.Values()[index], referenceWriter);
		}
	}

	for (vint i = 0; i < symbolGroup->children.Count(); i++)
	{
		auto childGroup = symbolGroup->children[i];
		RenderDocumentRecord(global, parsingTable, childGroup, fileGroupPrefix, pathReference, progressReporter, writtenReferenceCount);
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
	auto parsingTable = XmlLoadTable();
	vint writtenReferenceCount = 0;
	for (vint i = 0; i < rootGroup->children.Count(); i++)
	{
		auto fileGroup = rootGroup->children[i];
		if (predefinedGroups.Contains(fileGroup->name))
		{
			Folder(pathReference / fileGroup->uniqueId).Create(true);
			RenderDocumentRecord(global, parsingTable, fileGroup, fileGroup->uniqueId, pathReference, progressReporter, writtenReferenceCount);
		}
	}
}