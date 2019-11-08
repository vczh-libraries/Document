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
			vint index = global->declToFiles.Keys().IndexOf(decls[i].Obj());
			if (index != -1)
			{
				if (INVLOC.StartsWith(wupper(global->declToFiles.Values()[index].GetFullPath()), fileGroupPrefix, Locale::Normalization::None))
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
	const wchar_t* divClass = nullptr;
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
			divClass = L"cpp_type";
		}
		break;
	case symbol_component::SymbolKind::Class:
		if (context->GetAnyForwardDecl<ForwardClassDeclaration>()->name.tokenCount > 0)
		{
			searchForChild = true;
			keyword = L"class";
			divClass = L"cpp_type";
		}
		break;
	case symbol_component::SymbolKind::Struct:
		if (context->GetAnyForwardDecl<ForwardClassDeclaration>()->name.tokenCount > 0)
		{
			searchForChild = true;
			keyword = L"struct";
			divClass = L"cpp_type";
		}
		break;
	case symbol_component::SymbolKind::Union:
		if (context->GetAnyForwardDecl<ForwardClassDeclaration>()->name.tokenCount > 0)
		{
			searchForChild = true;
			keyword = L"union";
			divClass = L"cpp_type";
		}
		break;
	case symbol_component::SymbolKind::TypeAlias:
		keyword = L"typedef";
		divClass = L"cpp_type";
		break;
	case symbol_component::SymbolKind::FunctionSymbol:
		if (!context->GetAnyForwardDecl<ForwardFunctionDeclaration>()->implicitlyGeneratedMember)
		{
			keyword = L"function";
			divClass = L"cpp_function";
		}
		break;
	case symbol_component::SymbolKind::Variable:
		keyword = L"variable";
		if (auto parent = context->GetParentScope())
		{
			if (parent->GetImplDecl_NFb<ClassDeclaration>())
			{
				divClass = L"cpp_field";
			}
		}
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
		if (divClass)
		{
			writer.WriteString(L"<div class=\"");
			writer.WriteString(divClass);
			writer.WriteString(L"\">");
		}
		{
			Ptr<TemplateSpec> templateSpec = symbol_type_resolving::GetTemplateSpecFromSymbol(context);
			Ptr<SpecializationSpec> specializationSpec;
			WString declName = context->name;
			switch (context->kind)
			{
			case symbol_component::SymbolKind::Class:
			case symbol_component::SymbolKind::Struct:
			case symbol_component::SymbolKind::Union:
				{
					auto decl = context->GetAnyForwardDecl<ForwardClassDeclaration>();
					specializationSpec = decl->specializationSpec;
					declName = decl->name.name;
				}
				break;
			case symbol_component::SymbolKind::FunctionSymbol:
				{
					auto decl = context->GetAnyForwardDecl<ForwardFunctionDeclaration>();
					specializationSpec = decl->specializationSpec;
					declName = decl->name.name;
				}
				break;
			}

			WriteHtmlTextSingleLine(declName, writer);
			if (specializationSpec)
			{
				WriteHtmlTextSingleLine(L"<", writer);
				for (vint i = 0; i < specializationSpec->arguments.Count(); i++)
				{
					if (i > 0) writer.WriteString(L", ");
					auto argument = specializationSpec->arguments[i];
					if (argument.item.expr)
					{
						writer.WriteString(L"<span class=\"cpp_keyword\">(expr)</span>");
					}
					else
					{
						writer.WriteString(GetTypeDisplayNameInHtml(argument.item.type));
					}
					if (argument.isVariadic)
					{
						writer.WriteString(L" ...");
					}
				}
				WriteHtmlTextSingleLine(L">", writer);
			}
			else if (templateSpec)
			{
				WriteHtmlTextSingleLine(L"<", writer);
				for (vint i = 0; i < templateSpec->arguments.Count(); i++)
				{
					if (i > 0) writer.WriteString(L", ");
					auto argument = templateSpec->arguments[i];
					if (argument.argumentType == CppTemplateArgumentType::Value)
					{
						writer.WriteString(L"<span class=\"cpp_keyword\">(expr)</span>");
					}
					else
					{
						writer.WriteString(L"<span class=\"cpp_type\">");
						WriteHtmlTextSingleLine(argument.name.name, writer);
						writer.WriteString(L"</span>");
					}
					if (argument.ellipsis)
					{
						writer.WriteString(L" ...");
					}
				}
				WriteHtmlTextSingleLine(L">", writer);
			}
		}
		if (divClass)
		{
			writer.WriteString(L"</div>");
		}

		auto writeTag = [&](const WString& declId, const WString& tag, Ptr<Declaration> decl)
		{
			vint index = global->declToFiles.Keys().IndexOf(decl.Obj());
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

		EnumerateDecls(context, [&](Ptr<Declaration> decl, bool isImpl, vint index)
		{
			writeTag(GetDeclId(decl), (isImpl ? L"impl" : L"decl"), decl);
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
				GenerateSymbolIndexForFileGroup(global, writer, fileGroupPrefix, indentation + 1, children[j].Obj(), !isRoot, printedChildNextLevel);
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