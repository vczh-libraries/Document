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

Ptr<SymbolGroup> GenerateSymbolIndexForFileGroup(Ptr<GlobalLinesRecord> global, const WString& fileGroupPrefix, Symbol* context)
{
	if (context->kind != symbol_component::SymbolKind::Root)
	{
		switch (context->GetCategory())
		{
		case symbol_component::SymbolCategory::Normal:
			if (auto decl = context->GetImplDecl_NFb())
			{
				if (!decl->implicitlyGeneratedMember)
				{
					if (IsDeclInFileGroup(global, decl, fileGroupPrefix)) goto GENERATE_SYMBOL_GROUP;
				}
			}
			for (vint i = 0; i < context->GetForwardDecls_N().Count(); i++)
			{
				auto decl = context->GetForwardDecls_N()[i];
				if (!decl->implicitlyGeneratedMember)
				{
					if (IsDeclInFileGroup(global, decl, fileGroupPrefix)) goto GENERATE_SYMBOL_GROUP;
				}
			}
			break;
		case symbol_component::SymbolCategory::Function:
			for (vint i = 0; i < context->GetImplSymbols_F().Count(); i++)
			{
				if (auto decl = context->GetImplSymbols_F()[i]->GetImplDecl_NFb())
				{
					if (!decl->implicitlyGeneratedMember)
					{
						if (IsDeclInFileGroup(global, decl, fileGroupPrefix)) goto GENERATE_SYMBOL_GROUP;
					}
				}
			}
			for (vint i = 0; i < context->GetForwardSymbols_F().Count(); i++)
			{
				if (auto decl = context->GetForwardSymbols_F()[i]->GetForwardDecl_Fb())
				{
					if (!decl->implicitlyGeneratedMember)
					{
						if (IsDeclInFileGroup(global, decl, fileGroupPrefix)) goto GENERATE_SYMBOL_GROUP;
					}
				}
			}
			break;
		case symbol_component::SymbolCategory::FunctionBody:
			throw UnexpectedSymbolCategoryException();
		}
		return nullptr;
	}
GENERATE_SYMBOL_GROUP:

	auto symbolGroup = MakePtr<SymbolGroup>();
	symbolGroup->symbol = context;
	switch (context->kind)
	{
	case CLASS_SYMBOL_KIND:
	case symbol_component::SymbolKind::Namespace:
		symbolGroup->braces = true;
		break;
	}

	if (context->kind == symbol_component::SymbolKind::Root || symbolGroup->braces)
	{
		for (vint i = 0; i < context->GetChildren_NFb().Count(); i++)
		{
			auto& children = context->GetChildren_NFb().GetByIndex(i);
			for (vint j = 0; j < children.Count(); j++)
			{
				auto& childSymbol = children[j];
				if (!childSymbol.childExpr && !childSymbol.childType)
				{
					if (auto childGroup = GenerateSymbolIndexForFileGroup(global, fileGroupPrefix, childSymbol.childSymbol.Obj()))
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
	return symbolGroup;
}

/***********************************************************************
RenderSymbolGroup
***********************************************************************/

void RenderSymbolGroup(Ptr<GlobalLinesRecord> global, StreamWriter& writer, Ptr<SymbolGroup> symbolGroup)
{
	switch (symbolGroup->kind)
	{
	case SymbolGroupKind::Group:
		writer.WriteString(L"<span class=\"fileGroupLabel\">");
		WriteHtmlTextSingleLine(symbolGroup->name, writer);
		writer.WriteLine(L"</span>");
		break;
	case SymbolGroupKind::Text:
		WriteHtmlTextSingleLine(symbolGroup->name, writer);
		break;
	case SymbolGroupKind::Symbol:
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
			writer.WriteLine(L"</div>");
		}
		break;
	}

	if (symbolGroup->children.Count() > 0)
	{
		if (symbolGroup->braces)
		{
			writer.WriteLine(L"{");
		}

		if (symbolGroup->kind != SymbolGroupKind::Root)
		{
			writer.WriteString(L"<div style=\"margin-left: 4em;\">");
		}

		for (vint i = 0; i < symbolGroup->children.Count(); i++)
		{
			auto childGroup = symbolGroup->children[i];
			RenderSymbolGroup(global, writer, childGroup);
		}

		if (symbolGroup->kind != SymbolGroupKind::Root)
		{
			writer.WriteString(L"</div>");
		}

		if (symbolGroup->braces)
		{
			writer.WriteLine(L"}");
		}
	}
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
			if (auto fileGroup = GenerateSymbolIndexForFileGroup(global, prefix, result.pa.root.Obj()))
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
	writer.WriteLine(L"</head>");
	writer.WriteLine(L"<body>");
	writer.WriteLine(L"<a class=\"button\" href=\"./FileIndex.html\">File Index</a>");
	writer.WriteLine(L"<a class=\"button\" href=\"./SymbolIndex.html\">Symbol Index</a>");
	writer.WriteLine(L"<br>");
	writer.WriteLine(L"<br>");
	writer.WriteString(L"<div class=\"cpp_default\">");

	RenderSymbolGroup(global, writer, rootGroup);

	writer.WriteLine(L"</div>");
	writer.WriteLine(L"</body>");
	writer.WriteLine(L"</html>");
}