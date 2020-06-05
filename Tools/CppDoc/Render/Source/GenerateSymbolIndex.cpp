#include "Render.h"

/***********************************************************************
SymbolGroup
***********************************************************************/

enum class SymbolGroupKind
{
	Root,
	Group,
	Text,
	Symbol,
	SymbolAndText,
};

struct SymbolGroup
{
	SymbolGroupKind					kind = SymbolGroupKind::Symbol;
	WString							name;
	Symbol*							symbol = nullptr;
	bool							braces = false;
	List<Ptr<SymbolGroup>>			children;
};

/***********************************************************************
GenerateSymbolGroupForFileGroup
***********************************************************************/

bool IsDeclInFileGroup(Ptr<GlobalLinesRecord> global, Ptr<Declaration> decl, const WString& fileGroupPrefix)
{
	vint index = global->declToFiles.Keys().IndexOf({ decl,nullptr });
	if (index != -1)
	{
		if (INVLOC.StartsWith(global->declToFiles.Values()[index].GetFullPath(), fileGroupPrefix, Locale::Normalization::IgnoreCase))
		{
			return true;
		}
	}
	return false;
}

bool IsSymbolInFileGroup(Ptr<GlobalLinesRecord> global, Symbol* context, const WString& fileGroupPrefix, bool& generateChildSymbols)
{
	generateChildSymbols = false;
	switch (context->GetCategory())
	{
	case symbol_component::SymbolCategory::Normal:
		if (auto decl = context->GetImplDecl_NFb())
		{
			if (!decl->implicitlyGeneratedMember)
			{
				if (IsDeclInFileGroup(global, decl, fileGroupPrefix))
				{
					switch (context->kind)
					{
					case CLASS_SYMBOL_KIND:
						generateChildSymbols = true;
						break;
					}
					return true;
				}
			}
			return false;
		}
		for (vint i = 0; i < context->GetForwardDecls_N().Count(); i++)
		{
			auto decl = context->GetForwardDecls_N()[i];
			if (!decl->implicitlyGeneratedMember)
			{
				if (IsDeclInFileGroup(global, decl, fileGroupPrefix))
				{
					switch (context->kind)
					{
					case symbol_component::SymbolKind::Namespace:
						generateChildSymbols = true;
						break;
					}
					return true;
				}
			}
		}
		break;
	case symbol_component::SymbolCategory::Function:
		for (vint i = 0; i < context->GetImplSymbols_F().Count(); i++)
		{
			auto decl = context->GetImplSymbols_F()[i]->GetImplDecl_NFb();
			if (!decl->implicitlyGeneratedMember)
			{
				if (IsDeclInFileGroup(global, decl, fileGroupPrefix))
				{
					return true;
				}
			}
		}

		if (context->GetImplSymbols_F().Count() > 0)
		{
			return false;
		}

		for (vint i = 0; i < context->GetForwardSymbols_F().Count(); i++)
		{
			auto decl = context->GetForwardSymbols_F()[i]->GetForwardDecl_Fb();
			if (!decl->implicitlyGeneratedMember)
			{
				if (IsDeclInFileGroup(global, decl, fileGroupPrefix))
				{
					return true;
				}
			}
		}
		break;
	case symbol_component::SymbolCategory::FunctionBody:
		throw UnexpectedSymbolCategoryException();
	}
	return false;
}

Ptr<SymbolGroup> GenerateSymbolIndexForFileGroup(Ptr<GlobalLinesRecord> global, const WString& fileGroupPrefix, Symbol* context, SymbolGroup* parentGroup, Dictionary<Symbol*, Ptr<SymbolGroup>>& psContainers)
{
	if (psContainers.Keys().Contains(context))
	{
		return nullptr;
	}

	bool generateChildSymbols = false;
	if (context->kind == symbol_component::SymbolKind::Root)
	{
		generateChildSymbols = true;
	}
	else
	{
		if (!IsSymbolInFileGroup(global, context, fileGroupPrefix, generateChildSymbols))
		{
			return nullptr;
		}
	}

	auto symbolGroup = MakePtr<SymbolGroup>();
	symbolGroup->symbol = context;
	if (generateChildSymbols && context->kind != symbol_component::SymbolKind::Root)
	{
		symbolGroup->braces = true;
	}

	if (generateChildSymbols)
	{
		for (vint i = 0; i < context->GetChildren_NFb().Count(); i++)
		{
			auto& children = context->GetChildren_NFb().GetByIndex(i);
			for (vint j = 0; j < children.Count(); j++)
			{
				auto& childSymbol = children[j];
				if (!childSymbol.childExpr && !childSymbol.childType)
				{
					if (auto childGroup = GenerateSymbolIndexForFileGroup(global, fileGroupPrefix, childSymbol.childSymbol.Obj(), symbolGroup.Obj(), psContainers))
					{
						symbolGroup->children.Add(childGroup);
					}
				}
			}
		}
	}

	if (context->kind == symbol_component::SymbolKind::Root || context->kind == symbol_component::SymbolKind::Namespace)
	{
		if (symbolGroup->children.Count() == 0)
		{
			return nullptr;
		}
	}

	if (auto primary = context->GetPSPrimary_NF())
	{
		if (primary == context)
		{
			auto psGroup = MakePtr<SymbolGroup>();
			psGroup->kind = SymbolGroupKind::Text;
			psGroup->name = L"Partial Specializations";

			symbolGroup->children.Insert(0, psGroup);
			psContainers.Add(context, psGroup);
		}
		else
		{
			if (!psContainers.Keys().Contains(primary))
			{
				bool unused = false;
				if (IsSymbolInFileGroup(global, primary, fileGroupPrefix, unused))
				{
					if (auto primaryGroup = GenerateSymbolIndexForFileGroup(global, fileGroupPrefix, primary, parentGroup, psContainers))
					{
						parentGroup->children.Add(primaryGroup);
					}
					else
					{
						throw UnexpectedSymbolCategoryException();
					}
				}
				else
				{
					auto psGroup = MakePtr<SymbolGroup>();
					psGroup->kind = SymbolGroupKind::SymbolAndText;
					psGroup->name = L"(Partial Specializations)";
					psGroup->symbol = primary;

					parentGroup->children.Add(psGroup);
					psContainers.Add(primary, psGroup);
				}
			}

			auto container = psContainers[primary];
			container->children.Add(symbolGroup);
			return nullptr;
		}
	}

	return symbolGroup;
}

/***********************************************************************
RenderSymbolGroup
***********************************************************************/

void RenderSymbolGroup(Ptr<GlobalLinesRecord> global, StreamWriter& writer, Ptr<SymbolGroup> symbolGroup)
{
	writer.WriteString(L"<div>");
	if (symbolGroup->kind != SymbolGroupKind::Root)
	{
		writer.WriteString(L"<div class=\"symbol_header\">");
		if (symbolGroup->children.Count() > 0)
		{
			writer.WriteString(L"<a class=\"symbol_expanding\" onclick=\"toggleSymbolDropdown(this)\">+</a>");
		}
		switch (symbolGroup->kind)
		{
		case SymbolGroupKind::Group:
			writer.WriteString(L"<span class=\"fileGroupLabel\">");
			WriteHtmlTextSingleLine(symbolGroup->name, writer);
			writer.WriteString(L"</span>");
			break;
		case SymbolGroupKind::Text:
			writer.WriteString(L"<span>");
			WriteHtmlTextSingleLine(symbolGroup->name, writer);
			writer.WriteString(L"</span>");
			break;
		case SymbolGroupKind::Symbol:
		case SymbolGroupKind::SymbolAndText:
			{
				const wchar_t* keyword = nullptr;
				switch (symbolGroup->symbol->kind)
				{
				case symbol_component::SymbolKind::Enum:
					{
						auto decl = symbolGroup->symbol->GetAnyForwardDecl<ForwardEnumDeclaration>();
						if (decl->enumClass)
						{
							keyword = L"enum class";
						}
						else
						{
							keyword = L"enum";
						}
					}
					break;
				case symbol_component::SymbolKind::Class:
					keyword = L"class";
					break;
				case symbol_component::SymbolKind::Struct:
					keyword = L"struct";
					break;
				case symbol_component::SymbolKind::Union:
					keyword = L"union";
					break;
				case symbol_component::SymbolKind::TypeAlias:
					keyword = L"typedef";
					break;
				case symbol_component::SymbolKind::FunctionSymbol:
					keyword = L"function";
					break;
				case symbol_component::SymbolKind::Variable:
					keyword = L"variable";
					break;
				case symbol_component::SymbolKind::ValueAlias:
					keyword = L"constexpr";
					break;
				case symbol_component::SymbolKind::Namespace:
					keyword = L"namespace";
					break;
				default:
					throw UnexpectedSymbolCategoryException();
				}

				writer.WriteString(L"<div class=\"codebox\">");
				writer.WriteString(L"<div class=\"cpp_keyword\">");
				writer.WriteString(keyword);
				writer.WriteString(L"</div>");
				writer.WriteChar(L' ');
				writer.WriteString(GetUnscopedSymbolDisplayNameInHtml(symbolGroup->symbol, true));
				if (auto funcDecl = symbolGroup->symbol->GetAnyForwardDecl<ForwardFunctionDeclaration>())
				{
					if (auto funcType = GetTypeWithoutMemberAndCC(funcDecl->type).Cast<FunctionType>())
					{
						writer.WriteString(AppendFunctionParametersInHtml(funcType.Obj()));
					}
				}

				if (symbolGroup->symbol->kind != symbol_component::SymbolKind::Namespace)
				{
					auto writeTag = [&](const WString& declId, const WString& tag, DeclOrArg declOrArg)
					{
						vint index = global->declToFiles.Keys().IndexOf(declOrArg);
						if (index != -1)
						{
							auto filePath = global->declToFiles.Values()[index];
							auto htmlFileName = global->fileLines[filePath]->htmlFileName;
							writer.WriteString(L"<a class=\"symbolIndex\" href=\"./");
							WriteHtmlAttribute(htmlFileName, writer);
							writer.WriteString(L".html#");
							WriteHtmlAttribute(declId, writer);
							writer.WriteString(L"\">");
							WriteHtmlTextSingleLine(tag, writer);
							writer.WriteString(L"</a>");
						}
					};

					EnumerateDecls(symbolGroup->symbol, [&](DeclOrArg declOrArg, bool isImpl, vint index)
					{
						writeTag(GetDeclId(declOrArg), (isImpl ? L"impl" : L"decl"), declOrArg);
					});
				}

				if (symbolGroup->kind == SymbolGroupKind::SymbolAndText)
				{
					writer.WriteString(L" ");
					writer.WriteString(symbolGroup->name);
				}
				writer.WriteString(L"</div>");
			}
			break;
		}
		writer.WriteLine(L"</div>&nbsp;");
	}

	if (symbolGroup->children.Count() > 0)
	{
		if (symbolGroup->kind == SymbolGroupKind::Root)
		{
			writer.WriteString(L"<div class=\"symbol_dropdown_container expanded\">");
		}
		else
		{
			writer.WriteString(L"<div class=\"symbol_dropdown_container\">");
		}
		if (symbolGroup->braces)
		{
			writer.WriteString(L"{");
		}

		if (symbolGroup->kind != SymbolGroupKind::Root)
		{
			writer.WriteString(L"<div class=\"symbol_dropdown\">");
		}

		for (vint i = 0; i < symbolGroup->children.Count(); i++)
		{
			writer.WriteLine(L"");
			auto childGroup = symbolGroup->children[i];
			RenderSymbolGroup(global, writer, childGroup);
		}

		if (symbolGroup->kind != SymbolGroupKind::Root)
		{
			writer.WriteString(L"</div>");
		}

		if (symbolGroup->braces)
		{
			writer.WriteString(L"}");
		}
		writer.WriteString(L"</div>");
	}
	writer.WriteLine(L"</div>");
}

/***********************************************************************
GenerateSymbolIndex
***********************************************************************/

void GenerateSymbolIndex(Ptr<GlobalLinesRecord> global, IndexResult& result, FilePath pathHtml, FileGroupConfig& fileGroups)
{
	auto rootGroup = MakePtr<SymbolGroup>();
	{
		rootGroup->kind = SymbolGroupKind::Root;
		for (vint i = 0; i < fileGroups.Count(); i++)
		{
			auto prefix = fileGroups[i].f0;
			Dictionary<Symbol*, Ptr<SymbolGroup>> psContainers;
			if (auto fileGroup = GenerateSymbolIndexForFileGroup(global, prefix, result.pa.root.Obj(), nullptr, psContainers))
			{
				fileGroup->kind = SymbolGroupKind::Group;
				fileGroup->name = fileGroups[i].f1;
				rootGroup->children.Add(fileGroup);
			}
		}
	}

	FileStream fileStream(pathHtml.GetFullPath(), FileStream::WriteOnly);
	Utf8Encoder encoder;
	EncoderStream encoderStream(fileStream, encoder);
	StreamWriter writer(encoderStream);

	writer.WriteLine(L"<!DOCTYPE html>");
	writer.WriteLine(L"<html>");
	writer.WriteLine(L"<head>");
	writer.WriteLine(L"    <title>Symbol Index</title>");
	writer.WriteLine(L"    <link rel=\"stylesheet\" href=\"../Cpp.css\" />");
	writer.WriteLine(L"    <link rel=\"shortcut icon\" href=\"../favicon.ico\" />");
	writer.WriteLine(L"    <script type=\"text/javascript\" src=\"../Cpp.js\" ></script>");
	writer.WriteLine(L"</head>");
	writer.WriteLine(L"<body>");
	writer.WriteLine(L"<a class=\"button\" href=\"./FileIndex.html\">File Index</a>");
	writer.WriteLine(L"<a class=\"button\" href=\"./SymbolIndex.html\">Symbol Index</a>");
	writer.WriteLine(L"<br>");
	writer.WriteLine(L"<br>");
	writer.WriteString(L"<div class=\"cpp_default\"><div class=\"symbol_root\">");

	RenderSymbolGroup(global, writer, rootGroup);

	writer.WriteLine(L"</div></div>");
	writer.WriteLine(L"</body>");
	writer.WriteLine(L"</html>");
}