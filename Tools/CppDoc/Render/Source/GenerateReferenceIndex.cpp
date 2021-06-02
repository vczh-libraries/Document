#include "Render.h"
#include <Symbol_Resolve.h>
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

WString BuildHyperlinkName(Symbol* symbol, bool needKeyword)
{
	const wchar_t* keyword = nullptr;
	if (needKeyword)
	{
		switch (symbol->kind)
		{
		case symbol_component::SymbolKind::Enum:
			{
				auto decl = symbol->GetAnyForwardDecl<ForwardEnumDeclaration>();
				if (decl->enumClass)
				{
					keyword = L"enum class ";
				}
				else
				{
					keyword = L"enum ";
				}
			}
			break;
		case symbol_component::SymbolKind::Class:
			keyword = L"class ";
			break;
		case symbol_component::SymbolKind::Struct:
			keyword = L"struct ";
			break;
		case symbol_component::SymbolKind::Union:
			keyword = L"union ";
			break;
		case symbol_component::SymbolKind::TypeAlias:
			keyword = L"typedef ";
			break;
		case symbol_component::SymbolKind::FunctionSymbol:
			keyword = L"function ";
			break;
		case symbol_component::SymbolKind::Variable:
			keyword = L"variable ";
			break;
		case symbol_component::SymbolKind::ValueAlias:
			keyword = L"constexpr ";
			break;
		case symbol_component::SymbolKind::Namespace:
			keyword = L"namespace ";
			break;
		default:
			throw UnexpectedSymbolCategoryException();
		}
	}

	auto decl = symbol->GetAnyForwardDecl<Declaration>();
	Ptr<SpecializationSpec> sspec;

	WString result;
	if (keyword) result = keyword;
	result += decl->name.name;

	if (auto classDecl = decl.Cast<ForwardClassDeclaration>())
	{
		sspec = classDecl->specializationSpec;
	}
	else if (auto funcDecl = decl.Cast<ForwardFunctionDeclaration>())
	{
		sspec = funcDecl->specializationSpec;
	}
	else if (auto valueDecl = decl.Cast<ValueAliasDeclaration>())
	{
		sspec = valueDecl->specializationSpec;
	}

	if (sspec)
	{
		result += AppendGenericArgumentsInSignature(sspec->arguments);
	}
	return result;
}

Ptr<XmlElement> BuildHyperlink(
	Ptr<GlobalLinesRecord> global,
	IndexResult& result,
	Symbol* symbol
)
{
	auto xmlSymbol = MakePtr<XmlElement>();
	xmlSymbol->name.value = L"symbol";

	{
		auto attr = MakePtr<XmlAttribute>();
		attr->name.value = L"name";
		attr->value.value = BuildHyperlinkName(symbol, false);
		xmlSymbol->attributes.Add(attr);
	}
	
	Ptr<Declaration> decl;
	vint index = global->declComments.Keys().IndexOf(symbol);
	if (index != -1)
	{
		auto attr = MakePtr<XmlAttribute>();
		attr->name.value = L"docId";
		attr->value.value = GetSymbolId(symbol);
		xmlSymbol->attributes.Add(attr);
		decl = global->declComments.Values()[index]->decl;
	}
	else
	{
		if (symbol->GetCategory() == symbol_component::SymbolCategory::Normal)
		{
			if (!(decl = symbol->GetImplDecl_NFb()))
			{
				decl = symbol->GetForwardDecls_N()[0];
			}
		}
		else
		{
			if (symbol->GetImplSymbols_F().Count() > 0)
			{
				decl = symbol->GetImplSymbols_F()[0]->GetImplDecl_NFb();
			}
			else
			{
				decl = symbol->GetForwardSymbols_F()[0]->GetForwardDecl_Fb();
			}
		}
	}

	{
		auto attr = MakePtr<XmlAttribute>();
		attr->name.value = L"declFile";
		attr->value.value = global->fileLines[global->declToFiles[{decl, nullptr}]]->htmlFileName;
		xmlSymbol->attributes.Add(attr);
	}
	{
		auto attr = MakePtr<XmlAttribute>();
		attr->name.value = L"declId";
		attr->value.value = GetDeclId({ decl,nullptr });
		xmlSymbol->attributes.Add(attr);
	}

	return xmlSymbol;
}

Ptr<XmlElement> ResolveHyperLink(
	Ptr<GlobalLinesRecord> global,
	IndexResult& result,
	Symbol* symbol,
	Ptr<Declaration> decl,
	bool relativeSearch,
	const WString& hyperlinkType,
	const List<RegexString>& contents,
	const WString& hyperlinkText,
	const WString& xmlText
)
{
	if (!relativeSearch && hyperlinkType != L"T" && hyperlinkType != L"M" && hyperlinkType != L"F")
	{
		Console::WriteLine(L"");
		Console::WriteLine(L"UNRECONIZABLE HPERLINK: " + hyperlinkText);
		Console::WriteLine(xmlText);
	}

	ResolveSymbolResult rar;
	for (vint j = 0; j < contents.Count(); j++)
	{
		CppName cppName;
		cppName.type = CppNameType::Normal;
		cppName.name = contents[j].Value();

		vint templateArgumentCount = 0;
		{
			vint index = INVLOC.FindFirst(cppName.name, L"`", Locale::Normalization::None).key;
			if (index != -1)
			{
				auto filter = cppName.name.Right(cppName.name.Length() - index - 1);
				templateArgumentCount = filter == L"*" ? -1 : wtoi(filter);
				cppName.name = cppName.name.Left(index);
			}
		}

		if (j == 0)
		{
			if (relativeSearch)
			{
				rar = ResolveSymbolInContext(result.pa.AdjustForDecl(decl->symbol), cppName, false);
			}
			else
			{
				rar = ResolveSymbolInNamespaceContext(result.pa, result.pa.root.Obj(), cppName, false);
			}
		}
		else
		{
			auto ritem = rar.types->items[0];
			if (ritem.symbol->kind == symbol_component::SymbolKind::Namespace)
			{
				rar = ResolveSymbolInNamespaceContext(result.pa, ritem.symbol, cppName, false);
			}
			else
			{
				auto idType = MakePtr<IdType>();
				idType->resolving = rar.types;
				rar = ResolveChildSymbol(result.pa, idType, cppName);
			}
		}

		if (rar.types && templateArgumentCount != -1)
		{
			for (vint k = rar.types->items.Count() - 1; k >= 0; k--)
			{
				bool needToDelete = false;
				auto symbol = rar.types->items[k].symbol;
				switch (symbol->kind)
				{
				case CLASS_SYMBOL_KIND:
					if (auto spec = symbol->GetAnyForwardDecl<ForwardClassDeclaration>()->templateSpec)
					{
						if (templateArgumentCount != spec->arguments.Count())
						{
							needToDelete = true;
						}
					}
					else if (templateArgumentCount != 0)
					{
						needToDelete = true;
					}
					break;
				case symbol_component::SymbolKind::TypeAlias:
					if (auto spec = symbol->GetAnyForwardDecl<TypeAliasDeclaration>()->templateSpec)
					{
						if (templateArgumentCount != spec->arguments.Count())
						{
							needToDelete = true;
						}
					}
					else if (templateArgumentCount != 0)
					{
						needToDelete = true;
					}
					break;
				case symbol_component::SymbolKind::Enum:
				case symbol_component::SymbolKind::Namespace:
					if (templateArgumentCount != 0)
					{
						needToDelete = true;
					}
					break;
				default:
					needToDelete = true;
				}

				if (needToDelete)
				{
					rar.types->items.RemoveAt(k);
				}
			}

			if (rar.types->items.Count() == 0)
			{
				rar.types = nullptr;
			}
		}

		if (rar.values && templateArgumentCount != -1)
		{
			for (vint k = rar.values->items.Count() - 1; k >= 0; k--)
			{
				bool needToDelete = false;
				auto symbol = rar.values->items[k].symbol;
				switch (symbol->kind)
				{
				case symbol_component::SymbolKind::FunctionSymbol:
					if (auto spec = symbol->GetAnyForwardDecl<ForwardFunctionDeclaration>()->templateSpec)
					{
						if (templateArgumentCount != spec->arguments.Count())
						{
							needToDelete = true;
						}
					}
					else if (templateArgumentCount != 0)
					{
						needToDelete = true;
					}
					break;
				case symbol_component::SymbolKind::ValueAlias:
					{
						auto spec = symbol->GetAnyForwardDecl<ValueAliasDeclaration>()->templateSpec;
						if (templateArgumentCount != spec->arguments.Count())
						{
							needToDelete = true;
						}
					}
					break;
				case symbol_component::SymbolKind::EnumItem:
				case symbol_component::SymbolKind::Variable:
					if (templateArgumentCount != 0)
					{
						needToDelete = true;
					}
					break;
				default:
					needToDelete = true;
				}

				if (needToDelete)
				{
					rar.values->items.RemoveAt(k);
				}
			}

			if (rar.values->items.Count() == 0)
			{
				rar.values = nullptr;
			}
		}

		if (hyperlinkType == L"T" || j < contents.Count() - 1)
		{
			if (!rar.types) goto FOUND_ERROR;
			if (rar.types->items.Count() != 1) goto FOUND_ERROR;
		}
	}

	if (hyperlinkType == L"T")
	{
		if (!rar.types) goto FOUND_ERROR;
		if (rar.types->items.Count() != 1) goto FOUND_ERROR;
		return BuildHyperlink(global, result, rar.types->items[0].symbol);
	}
	else if (hyperlinkType == L"F")
	{
		if (!rar.values) goto FOUND_ERROR;
		if (rar.values->items.Count() != 1) goto FOUND_ERROR;
		return BuildHyperlink(global, result, rar.values->items[0].symbol);
	}
	else if (hyperlinkType == L"M")
	{
		if (!rar.values) goto FOUND_ERROR;
		if (rar.values->items.Count() == 1)
		{
			return BuildHyperlink(global, result, rar.values->items[0].symbol);
		}
		else
		{
			auto xmlSymbols = MakePtr<XmlElement>();
			xmlSymbols->name.value = L"symbols";
			for (vint j = 0; j < rar.values->items.Count(); j++)
			{
				xmlSymbols->subNodes.Add(BuildHyperlink(global, result, rar.values->items[j].symbol));
			}
			return xmlSymbols;
		}
	}
	else
	{
		if (!rar.values && !rar.types) goto FOUND_ERROR;
		if (rar.values && !rar.types && rar.values->items.Count() == 1)
		{
			return BuildHyperlink(global, result, rar.values->items[0].symbol);
		}
		if (!rar.values && rar.types && rar.types->items.Count() == 1)
		{
			return BuildHyperlink(global, result, rar.types->items[0].symbol);
		}

		auto xmlSymbols = MakePtr<XmlElement>();
		xmlSymbols->name.value = L"symbols";
		if (rar.values)
		{
			for (vint j = 0; j < rar.values->items.Count(); j++)
			{
				xmlSymbols->subNodes.Add(BuildHyperlink(global, result, rar.values->items[j].symbol));
			}
		}
		if (rar.types)
		{
			for (vint j = 0; j < rar.types->items.Count(); j++)
			{
				xmlSymbols->subNodes.Add(BuildHyperlink(global, result, rar.types->items[j].symbol));
			}
		}
		return xmlSymbols;
	}

FOUND_ERROR:
	Console::WriteLine(L"");
	Console::WriteLine(L"UNRESOLVABLE HPERLINK: " + hyperlinkText);
	Console::WriteLine(xmlText);
	return nullptr;
}

vint ProcessDocumentRecordHyperLinksInternal(
	Ptr<GlobalLinesRecord> global,
	IndexResult& result,
	Symbol* symbol,
	Ptr<Declaration> decl,
	Ptr<XmlElement> xmlContainer,
	bool isCData,
	vint indexInXmlContainer,
	const WString& xmlTextContent,
	const WString& xmlText
)
{
	static Regex regexHyperLink(L"/[(<type>/w):((<content>[a-zA-Z0-9_`*]+).)*(<content>[a-zA-Z0-9_`*]+)/]");

	RegexMatch::List matches;
	regexHyperLink.Cut(xmlTextContent, false, matches);
	if (matches.Count() == 1 && !matches[0]->Success()) return 1;

	bool foundError = false;
	List<Ptr<XmlNode>> subNodes;
	for (vint i = 0; i < matches.Count(); i++)
	{
		auto match = matches[i];
		if (match->Success())
		{
			auto type = match->Groups()[L"type"][0].Value();
			auto& contents = match->Groups()[L"content"];
			if (auto node = ResolveHyperLink(global, result, symbol, decl, false, type, contents, match->Result().Value(), xmlText))
			{
				subNodes.Add(node);
			}
			else
			{
				foundError = true;
			}
		}
		else if (isCData)
		{
			auto node = MakePtr<XmlCData>();
			node->content.value = match->Result().Value();
			subNodes.Add(node);
		}
		else
		{
			auto node = MakePtr<XmlText>();
			node->content.value = match->Result().Value();
			subNodes.Add(node);
		}
	}

	if (foundError) return 1;
	xmlContainer->subNodes.RemoveAt(indexInXmlContainer);
	for (vint i = 0; i < subNodes.Count(); i++)
	{
		xmlContainer->subNodes.Insert(i + indexInXmlContainer, subNodes[i]);
	}
	return subNodes.Count();
}

void ProcessDocumentRecordHyperLinksInternal(
	Ptr<GlobalLinesRecord> global,
	IndexResult& result,
	Symbol* symbol,
	Ptr<Declaration> decl,
	Ptr<XmlElement> xmlContainer,
	vint indexInXmlContainer,
	Ptr<XmlElement> xmlElement,
	const WString& xmlText
)
{
	static Regex regexHyperLink(L"^((<content>[a-zA-Z0-9_`*]+)::)*(<content>[a-zA-Z0-9_`*]+)$");

	if (xmlElement->name.value == L"see")
	{
		if (auto attr = XmlGetAttribute(xmlElement, L"cref"))
		{
			auto hyperlinkText = GenerateToStream([&](StreamWriter& writer)
			{
				XmlPrint(xmlElement, writer);
			});

			if (auto match = regexHyperLink.MatchHead(attr->value.value))
			{
				auto& contents = match->Groups()[L"content"];
				if (auto node = ResolveHyperLink(global, result, symbol, decl, true, L"", contents, hyperlinkText, xmlText))
				{
					xmlContainer->subNodes[indexInXmlContainer] = node;
				}
			}
			else
			{
				Console::WriteLine(L"");
				Console::WriteLine(L"UNRESOLVABLE HPERLINK: " + hyperlinkText);
				Console::WriteLine(xmlText);
			}
		}
		else
		{
			Console::WriteLine(L"");
			Console::WriteLine(L"MISSING cref IN <see/>:");
			Console::WriteLine(xmlText);
		}
	}
	else
	{
		for (vint i = 0; i < xmlElement->subNodes.Count(); i++)
		{
			auto subNode = xmlElement->subNodes[i];
			if (auto subElement = subNode.Cast<XmlElement>())
			{
				ProcessDocumentRecordHyperLinksInternal(global, result, symbol, decl, xmlElement, i, subElement, xmlText);
			}
			else if (auto subText = subNode.Cast<XmlText>())
			{
				vint converted = ProcessDocumentRecordHyperLinksInternal(global, result, symbol, decl, xmlElement, false, i, subText->content.value, xmlText);
				i += converted - 1;
			}
			else if (auto subCData = subNode.Cast<XmlCData>())
			{
				vint converted = ProcessDocumentRecordHyperLinksInternal(global, result, symbol, decl, xmlElement, true, i, subCData->content.value, xmlText);
				i += converted - 1;
			}
		}
	}
}

void ProcessDocumentRecordHyperLinks(
	Ptr<GlobalLinesRecord> global,
	IndexResult& result,
	Symbol* symbol,
	Ptr<Declaration> decl,
	Ptr<XmlDocument> xmlDocument,
	const WString& xmlText
)
{
	ProcessDocumentRecordHyperLinksInternal(global, result, symbol, decl, nullptr, -1, xmlDocument->rootElement, xmlText);
}

/***********************************************************************
ValidateAndFixDocumentRecord
***********************************************************************/

Ptr<XmlDocument> ValidateAndFixDocumentRecord(
	Ptr<GlobalLinesRecord> global,
	IndexResult& result,
	Ptr<ParsingTable> parsingTable,
	Ptr<SymbolGroup> symbolGroup,
	Ptr<DocumentRecord> documentRecord
)
{
	auto symbol = symbolGroup->symbol;

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
		ProcessDocumentRecordHyperLinks(global, result, symbol, documentRecord->decl, xmlDocument, xmlText);

		{
			auto att = MakePtr<XmlAttribute>();
			att->name.value = L"symbolId";
			att->value.value = symbol->uniqueId;
			xmlDocument->rootElement->attributes.Add(att);
		}
		{
			auto att = MakePtr<XmlAttribute>();
			att->name.value = L"accessor";
			xmlDocument->rootElement->attributes.Add(att);

			switch (symbol->GetParentScope()->kind)
			{
			case CLASS_SYMBOL_KIND:
				{
					auto classDecl = symbol->GetParentScope()->GetImplDecl_NFb<ClassDeclaration>();
					for (vint i = 0; i < classDecl->decls.Count(); i++)
					{
						auto child = classDecl->decls[i];
						if (auto childSymbol = child.f1->symbol)
						{
							if (childSymbol->kind == symbol_component::SymbolKind::FunctionBodySymbol)
							{
								childSymbol = childSymbol->GetFunctionSymbol_Fb();
							}
							if (symbol == childSymbol)
							{
								switch (child.f0)
								{
								case CppClassAccessor::Public:
									att->value.value = L"public";
									break;
								case CppClassAccessor::Protected:
									att->value.value = L"protected";
									break;
								case CppClassAccessor::Private:
									att->value.value = L"private";
									break;
								}
								goto FOUND_ACCESSOR;
							}
						}
					}
					throw L"Failed to find the accessor for this member.";
				}
			FOUND_ACCESSOR:
				break;
			}
		}
		{
			auto att = MakePtr<XmlAttribute>();
			att->name.value = L"category";
			xmlDocument->rootElement->attributes.Add(att);

			switch (symbol->kind)
			{
#define WRITE_CATEGORY(CATEGORY)										\
		case symbol_component::SymbolKind::CATEGORY:					\
			att->value.value = L#CATEGORY;								\
			break														\

				WRITE_CATEGORY(Enum);
				WRITE_CATEGORY(Class);
				WRITE_CATEGORY(Struct);
				WRITE_CATEGORY(Union);
				WRITE_CATEGORY(TypeAlias);
				WRITE_CATEGORY(Variable);
				WRITE_CATEGORY(ValueAlias);
				WRITE_CATEGORY(Namespace);
#undef WRITE_CATEGORY

			case symbol_component::SymbolKind::FunctionSymbol:
				att->value.value = L"Function";
				break;
			default:
				throw L"Unexpected symbol kind.";
			}
		}
		{
			auto att = MakePtr<XmlAttribute>();
			att->name.value = L"name";
			att->value.value = symbol->name;
			xmlDocument->rootElement->attributes.Add(att);
		}
		{
			auto decl = symbol->GetAnyForwardDecl<Declaration>();
			{
				auto attr = MakePtr<XmlAttribute>();
				attr->name.value = L"declFile";
				attr->value.value = global->fileLines[global->declToFiles[{decl, nullptr}]]->htmlFileName;
				xmlDocument->rootElement->attributes.Add(attr);
			}
			{
				auto attr = MakePtr<XmlAttribute>();
				attr->name.value = L"declId";
				attr->value.value = GetDeclId({ decl,nullptr });
				xmlDocument->rootElement->attributes.Add(attr);
			}
		}

		SortedList<Symbol*> seeAlsos, baseTypes;
		{
			auto cdata = MakePtr<XmlCData>();
			cdata->content.value = GetSymbolDisplayNameInSignature(symbol, seeAlsos, baseTypes);

			auto xmlSignature = MakePtr<XmlElement>();
			xmlSignature->name.value = L"signature";
			xmlSignature->subNodes.Add(cdata);

			xmlDocument->rootElement->subNodes.Add(xmlSignature);
		}
		if (seeAlsos.Count() > 0)
		{
			auto xmlSeeAlsos = MakePtr<XmlElement>();
			xmlSeeAlsos->name.value = L"seealsos";
			CopyFrom(
				xmlSeeAlsos->subNodes,
				From(seeAlsos)
					.OrderBy([](Symbol* a, Symbol* b) {return WString::Compare(a->uniqueId, b->uniqueId); })
					.Select([&](Symbol* symbol) {return BuildHyperlink(global, result, symbol); })
			);

			xmlDocument->rootElement->subNodes.Add(xmlSeeAlsos);
		}
		if (baseTypes.Count() > 0)
		{
			auto xmlBaseTypes = MakePtr<XmlElement>();
			xmlBaseTypes->name.value = L"basetypes";
			CopyFrom(
				xmlBaseTypes->subNodes,
				From(baseTypes)
					.OrderBy([](Symbol* a, Symbol* b) {return WString::Compare(a->uniqueId, b->uniqueId); })
					.Select([&](Symbol* symbol) {return BuildHyperlink(global, result, symbol); })
			);

			xmlDocument->rootElement->subNodes.Add(xmlBaseTypes);
		}

		return xmlDocument;
	}
	else
	{
		Console::WriteLine(L"");
		Console::WriteLine(L"FAILED TO PARSE:");
		Console::WriteLine(xmlText);
		return nullptr;
	}
}

/***********************************************************************
ExtractExamples
***********************************************************************/

void ExtractExamples(
	Ptr<XmlElement>& xmlElement,
	List<Ptr<XmlElement>>& examples
)
{
	if (xmlElement->name.value == L"example")
	{
		auto outputAttr = XmlGetAttribute(xmlElement, L"output");
		vint index = examples.Add(xmlElement);
		xmlElement = MakePtr<XmlElement>();
		xmlElement->name.value = L"example";
		{
			auto attr = MakePtr<XmlAttribute>();
			attr->name.value = L"index";
			attr->value.value = itow(index);
			xmlElement->attributes.Add(attr);
		}
		if (outputAttr)
		{
			auto attr = MakePtr<XmlAttribute>();
			attr->name.value = L"output";
			attr->value.value = outputAttr->value.value;
			xmlElement->attributes.Add(attr);
		}
	}
	else
	{
		for (vint i = 0; i < xmlElement->subNodes.Count(); i++)
		{
			if (auto subElement = xmlElement->subNodes[i].Cast<XmlElement>())
			{
				ExtractExamples(subElement, examples);
				xmlElement->subNodes[i] = subElement;
			}
		}
	}
}

void ExtractExamples(
	Ptr<XmlDocument> xmlDocument,
	List<Ptr<XmlElement>>& examples
)
{
	ExtractExamples(xmlDocument->rootElement, examples);
}

/***********************************************************************
RenderDocumentRecord
***********************************************************************/

void RenderDocumentRecord(
	Ptr<GlobalLinesRecord> global,
	IndexResult& result,
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
			List<Ptr<XmlElement>> examples;
			if (auto xmlDocument = ValidateAndFixDocumentRecord(global, result, parsingTable, symbolGroup, global->declComments.Values()[index]))
			{
				ExtractExamples(xmlDocument, examples);

				{
					FileStream fileStream((pathReference / fileGroupPrefix / (symbolGroup->uniqueId + L".xml")).GetFullPath(), FileStream::WriteOnly);
					Utf8Encoder encoder;
					EncoderStream encoderStream(fileStream, encoder);
					StreamWriter writer(encoderStream);
					XmlPrint(xmlDocument, writer);
				}

				for (vint i = 0; i < examples.Count(); i++)
				{
					FileStream fileStream((pathReference / fileGroupPrefix / (symbolGroup->uniqueId + L".ein." + itow(i) + L".xml")).GetFullPath(), FileStream::WriteOnly);
					Utf8Encoder encoder;
					EncoderStream encoderStream(fileStream, encoder);
					StreamWriter writer(encoderStream);
					XmlPrint(examples[i], writer);
				}
			}
		}
	}

	// enum item does not have a symbol group, just go through all children
	for (vint i = 0; i < symbolGroup->children.Count(); i++)
	{
		auto childGroup = symbolGroup->children[i];
		RenderDocumentRecord(global, result, parsingTable, childGroup, fileGroupPrefix, pathReference, progressReporter, writtenReferenceCount);
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

void FlagDocumentedSymbolGroups(
	Ptr<GlobalLinesRecord> global,
	Ptr<SymbolGroup> group,
	Dictionary<Symbol*, Ptr<SymbolGroup>>& nss
)
{
	switch (group->kind)
	{
	case SymbolGroupKind::Group:
		nss.Add(nullptr, group);
		break;
	case SymbolGroupKind::Symbol:
		group->assignedDocument = global->declComments.Keys().Contains(group->symbol);
		if (group->symbol->kind == symbol_component::SymbolKind::Namespace)
		{
			nss.Add(group->symbol, group);
		}
		break;
	}

	if (group->assignedDocument) group->hasDocument = true;

	for (vint i = 0; i < group->children.Count(); i++)
	{
		auto child = group->children[i];
		FlagDocumentedSymbolGroups(global, child, nss);
		if (child->hasDocument)
		{
			if (child->kind == SymbolGroupKind::Symbol && child->symbol->kind == symbol_component::SymbolKind::Namespace)
			{
				continue;
			}
			group->hasDocument = true;
		}
	}
}

void WriteSymbolGroupInReferenceIndex(
	WString indent,
	Ptr<SymbolGroup> group,
	StreamWriter& writer
)
{
	if (!group->hasDocument) return;

	bool hasChild = false;
	for (vint i = 0; i < group->children.Count(); i++)
	{
		auto child = group->children[i];
		if (child->kind == SymbolGroupKind::Symbol && child->symbol->kind == symbol_component::SymbolKind::Namespace)
		{
			continue;
		}
		if (child->hasDocument)
		{
			hasChild = true;
			break;
		}
	}

	auto xmlText = GenerateToStream([&group](StreamWriter& textWriter)
	{
		auto xmlElement = MakePtr<XmlElement>();
		xmlElement->name.value = L"document";
		{
			auto attr = MakePtr<XmlAttribute>();
			attr->name.value = L"name";
			xmlElement->attributes.Add(attr);

			switch (group->kind)
			{
			case SymbolGroupKind::Text:
				attr->value.value = group->name;
				break;
			case SymbolGroupKind::Symbol:
				attr->value.value = BuildHyperlinkName(group->symbol, true);
				break;
			case SymbolGroupKind::SymbolAndText:
				attr->value.value = BuildHyperlinkName(group->symbol, true) + L" " + group->name;
				break;
			}
		}
		if (group->assignedDocument)
		{
			auto attr = MakePtr<XmlAttribute>();
			attr->name.value = L"docId";
			attr->value.value = group->symbol->uniqueId;
			xmlElement->attributes.Add(attr);
		}
		if (group->assignedDocument)
		{
			auto attr = MakePtr<XmlAttribute>();
			attr->name.value = L"file";
			attr->value.value = group->uniqueId;
			xmlElement->attributes.Add(attr);
		}

		XmlPrint(xmlElement, textWriter);
	});

	if (hasChild)
	{
		writer.WriteString(indent);
		writer.WriteString(xmlText.Left(xmlText.Length() - 2));
		writer.WriteLine(L">");

		WString newIndent = indent + L"  ";
		for (vint i = 0; i < group->children.Count(); i++)
		{
			auto child = group->children[i];
			if (child->kind == SymbolGroupKind::Symbol && child->symbol->kind == symbol_component::SymbolKind::Namespace)
			{
				continue;
			}
			WriteSymbolGroupInReferenceIndex(newIndent, child, writer);
		}

		writer.WriteString(indent);
		writer.WriteLine(L"</document>");
	}
	else
	{
		writer.WriteString(indent);
		writer.WriteLine(xmlText);
	}
}

void GenerateReferenceIndex(
	Ptr<GlobalLinesRecord> global,
	IndexResult& result,
	Ptr<SymbolGroup> rootGroup,
	FilePath pathXml,
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
			RenderDocumentRecord(global, result, parsingTable, fileGroup, fileGroup->uniqueId, pathReference, progressReporter, writtenReferenceCount);
		}
	}

	{
		FileStream fileStream(pathXml.GetFullPath(), FileStream::WriteOnly);
		Utf8Encoder encoder;
		EncoderStream encoderStream(fileStream, encoder);
		StreamWriter writer(encoderStream);

		writer.WriteLine(L"<categories>");
		for (vint i = 0; i < rootGroup->children.Count(); i++)
		{
			auto fileGroup = rootGroup->children[i];
			if (predefinedGroups.Contains(fileGroup->name))
			{
				writer.WriteString(L"  <category name=\"");
				writer.WriteString(fileGroup->name);
				writer.WriteString(L"\" file=\"");
				writer.WriteString(fileGroup->uniqueId);
				writer.WriteLine(L"\"/>");
			}
		}
		writer.WriteLine(L"</categories>");
	}

	{
		for (vint i = 0; i < rootGroup->children.Count(); i++)
		{
			auto fileGroup = rootGroup->children[i];
			if (predefinedGroups.Contains(fileGroup->name))
			{
				Dictionary<Symbol*, Ptr<SymbolGroup>> nss;
				FlagDocumentedSymbolGroups(global, fileGroup, nss);

				Dictionary<WString, Symbol*> nsNames;
				for (vint i = 0; i < nss.Count(); i++)
				{
					if (auto symbol = nss.Keys()[i])
					{
						WString name;
						auto current = symbol;
						while (current)
						{
							if (current->kind == symbol_component::SymbolKind::Namespace)
							{
								name = L"::" + current->name + name;
								current = current->GetParentScope();
							}
							else
							{
								break;
							}
						}
						nsNames.Add(name, symbol);
					}
					else
					{
						nsNames.Add(L"::", nullptr);
					}
				}

				FileStream fileStream((pathReference / (fileGroup->uniqueId + L".xml")).GetFullPath(), FileStream::WriteOnly);
				Utf8Encoder encoder;
				EncoderStream encoderStream(fileStream, encoder);
				StreamWriter writer(encoderStream);

				writer.WriteLine(L"<reference>");
				for (vint i = 0; i < nsNames.Count(); i++)
				{
					auto group = nss[nsNames.Values()[i]];
					if (group->hasDocument)
					{
						writer.WriteString(L"  <namespace name=\"");
						writer.WriteString(nsNames.Keys()[i]);
						writer.WriteLine(L"\">");

						for (vint j = 0; j < group->children.Count(); j++)
						{
							auto child = group->children[j];
							if (child->kind == SymbolGroupKind::Symbol && child->symbol->kind == symbol_component::SymbolKind::Namespace)
							{
								continue;
							}
							WriteSymbolGroupInReferenceIndex(L"    ", child, writer);
						}

						writer.WriteLine(L"  </namespace>");
					}
				}
				writer.WriteLine(L"</reference>");
			}
		}
	}
}