#include "Render.h"

/***********************************************************************
GenerateSymbolIndexForFileGroup
***********************************************************************/

void GenerateSymbolIndexForFileGroup(Ptr<GlobalLinesRecord> global, StreamWriter& writer, const WString& fileGroupPrefix, vint indentation, Symbol* context, bool printBraceBeforeFirstChild, bool& printedChild)
{
	if (context->kind != symbol_component::SymbolKind::Root)
	{
		bool definedInThisFileGroup = false;
		List<Ptr<Declaration>> decls;
		switch (context->GetCategory())
		{
		case symbol_component::SymbolCategory::Normal:
			if (auto decl = context->GetImplDecl_NFb())
			{
				decls.Add(decl);
			}
			for (vint i = 0; i < context->GetForwardDecls_N().Count(); i++)
			{
				decls.Add(context->GetForwardDecls_N()[i]);
			}
			break;
		case symbol_component::SymbolCategory::Function:
			for (vint i = 0; i < context->GetImplSymbols_F().Count(); i++)
			{
				if (auto decl = context->GetImplSymbols_F()[i]->GetImplDecl_NFb())
				{
					decls.Add(decl);
				}
			}
			for (vint i = 0; i < context->GetForwardSymbols_F().Count(); i++)
			{
				if (auto decl = context->GetForwardSymbols_F()[i]->GetForwardDecl_Fb())
				{
					decls.Add(decl);
				}
			}
			break;
		case symbol_component::SymbolCategory::FunctionBody:
			throw UnexpectedSymbolCategoryException();
		}

		for (vint i = 0; i < decls.Count(); i++)
		{
			vint index = global->declToFiles.Keys().IndexOf({ decls[i],nullptr });
			if (index != -1)
			{
				if (INVLOC.StartsWith(global->declToFiles.Values()[index].GetFullPath(), fileGroupPrefix, Locale::Normalization::IgnoreCase))
				{
					definedInThisFileGroup = true;
					break;
				}
			}
		}

		if (!definedInThisFileGroup)
		{
			return;
		}
	}

	bool isRoot = false;
	bool searchForChild = false;
	const wchar_t* keyword = nullptr;
	switch (context->kind)
	{
	case symbol_component::SymbolKind::Enum:
		if (context->GetAnyForwardDecl<ForwardEnumDeclaration>()->name.tokenCount > 0)
		{
			searchForChild = true;
			if (context->GetAnyForwardDecl<ForwardEnumDeclaration>()->enumClass)
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
		if (context->GetAnyForwardDecl<ForwardClassDeclaration>()->name.tokenCount > 0)
		{
			searchForChild = true;
			keyword = L"class";
		}
		break;
	case symbol_component::SymbolKind::Struct:
		if (context->GetAnyForwardDecl<ForwardClassDeclaration>()->name.tokenCount > 0)
		{
			searchForChild = true;
			keyword = L"struct";
		}
		break;
	case symbol_component::SymbolKind::Union:
		if (context->GetAnyForwardDecl<ForwardClassDeclaration>()->name.tokenCount > 0)
		{
			searchForChild = true;
			keyword = L"union";
		}
		break;
	case symbol_component::SymbolKind::TypeAlias:
		keyword = L"typedef";
		break;
	case symbol_component::SymbolKind::FunctionSymbol:
		if (!context->GetAnyForwardDecl<ForwardFunctionDeclaration>()->implicitlyGeneratedMember)
		{
			keyword = L"function";
		}
		break;
	case symbol_component::SymbolKind::Variable:
		keyword = L"variable";
		break;
	case symbol_component::SymbolKind::Namespace:
		searchForChild = true;
		keyword = L"namespace";
		break;
	case symbol_component::SymbolKind::Root:
		searchForChild = true;
		isRoot = true;
		break;
	}

	if (keyword)
	{
		if (printBraceBeforeFirstChild)
		{
			if (!printedChild)
			{
				printedChild = true;
				for (vint i = 0; i < indentation - 1; i++)
				{
					writer.WriteString(L"    ");
				}
				writer.WriteLine(L"{");
			}
		}

		for (vint i = 0; i < indentation; i++)
		{
			writer.WriteString(L"    ");
		}
		writer.WriteString(L"<div class=\"cpp_keyword\">");
		writer.WriteString(keyword);
		writer.WriteString(L"</div>");
		writer.WriteChar(L' ');
		writer.WriteString(GetUnscopedSymbolDisplayNameInHtml(context, true));
		if (auto funcDecl = context->GetAnyForwardDecl<ForwardFunctionDeclaration>())
		{
			if (auto funcType = GetTypeWithoutMemberAndCC(funcDecl->type).Cast<FunctionType>())
			{
				writer.WriteString(AppendFunctionParametersInHtml(funcType.Obj()));
			}
		}

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

		EnumerateDecls(context, [&](DeclOrArg declOrArg, bool isImpl, vint index)
		{
			writeTag(GetDeclId(declOrArg), (isImpl ? L"impl" : L"decl"), declOrArg);
		});
		writer.WriteLine(L"");
	}

	if (searchForChild)
	{
		bool printedChildNextLevel = false;
		for (vint i = 0; i < context->GetChildren_NFb().Count(); i++)
		{
			auto& children = context->GetChildren_NFb().GetByIndex(i);
			for (vint j = 0; j < children.Count(); j++)
			{
				auto& childSymbol = children[j];
				if (!childSymbol.childExpr && !childSymbol.childType)
				{
					GenerateSymbolIndexForFileGroup(global, writer, fileGroupPrefix, indentation + 1, childSymbol.childSymbol.Obj(), !isRoot, printedChildNextLevel);
				}
			}
		}
		if (printedChildNextLevel)
		{
			for (vint i = 0; i < indentation; i++)
			{
				writer.WriteString(L"    ");
			}
			writer.WriteLine(L"}");
		}
	}
}

/***********************************************************************
GenerateSymbolIndex
***********************************************************************/

void GenerateSymbolIndex(Ptr<GlobalLinesRecord> global, IndexResult& result, FilePath pathHtml, FileGroupConfig& fileGroups)
{
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
	writer.WriteString(L"<div class=\"codebox\"><div class=\"cpp_default\">");
	for (vint i = 0; i < fileGroups.Count(); i++)
	{
		auto prefix = fileGroups[i].f0;
		writer.WriteString(L"<span class=\"fileGroupLabel\">");
		WriteHtmlTextSingleLine(fileGroups[i].f1, writer);
		writer.WriteLine(L"</span>");

		bool printedChild = false;
		GenerateSymbolIndexForFileGroup(global, writer, prefix, 0, result.pa.root.Obj(), false, printedChild);
	}
	writer.WriteLine(L"</div></div>");
	writer.WriteLine(L"</body>");
	writer.WriteLine(L"</html>");
}