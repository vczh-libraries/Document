#include "Render.h"

/***********************************************************************
Source Code Page Generating
***********************************************************************/

WString GetDisplayNameInHtml(Symbol* symbol);

class GetDisplayNameInHtmlTypeVisitor : public Object, public ITypeVisitor
{
public:
	WString											result;

	void Visit(PrimitiveType* self)override
	{
		switch (self->primitive)
		{
		case CppPrimitiveType::_auto:			result = L"<span class=\"cpp_keyword\">auto</span>" + result;			break;
		case CppPrimitiveType::_void:			result = L"<span class=\"cpp_keyword\">void</span>" + result;			break;
		case CppPrimitiveType::_bool:			result = L"<span class=\"cpp_keyword\">bool</span>" + result;			break;
		case CppPrimitiveType::_char:			result = L"<span class=\"cpp_keyword\">char</span>" + result;			break;
		case CppPrimitiveType::_wchar_t:		result = L"<span class=\"cpp_keyword\">wchar_t</span>" + result;		break;
		case CppPrimitiveType::_char16_t:		result = L"<span class=\"cpp_keyword\">char16_t</span>" + result;		break;
		case CppPrimitiveType::_char32_t:		result = L"<span class=\"cpp_keyword\">char32_t</span>" + result;		break;
		case CppPrimitiveType::_short:			result = L"<span class=\"cpp_keyword\">short</span>" + result;			break;
		case CppPrimitiveType::_int:			result = L"<span class=\"cpp_keyword\">int</span>" + result;			break;
		case CppPrimitiveType::___int8:			result = L"<span class=\"cpp_keyword\">__int8</span>" + result;			break;
		case CppPrimitiveType::___int16:		result = L"<span class=\"cpp_keyword\">__int16</span>" + result;		break;
		case CppPrimitiveType::___int32:		result = L"<span class=\"cpp_keyword\">__int32</span>" + result;		break;
		case CppPrimitiveType::___int64:		result = L"<span class=\"cpp_keyword\">__int64;</span>" + result;		break;
		case CppPrimitiveType::_long:			result = L"<span class=\"cpp_keyword\">long</span>" + result;			break;
		case CppPrimitiveType::_long_int:		result = L"<span class=\"cpp_keyword\">long int</span>" + result;		break;
		case CppPrimitiveType::_long_long:		result = L"<span class=\"cpp_keyword\">long long</span>" + result;		break;
		case CppPrimitiveType::_float:			result = L"<span class=\"cpp_keyword\">float</span>" + result;			break;
		case CppPrimitiveType::_double:			result = L"<span class=\"cpp_keyword\">double</span>" + result;			break;
		case CppPrimitiveType::_long_double:	result = L"<span class=\"cpp_keyword\">long double</span>" + result;	break;
		}

		switch (self->prefix)
		{
		case CppPrimitivePrefix::_signed:		result = L"<span class=\"cpp_keyword\">signed</span>" + result;			break;
		case CppPrimitivePrefix::_unsigned:		result = L"<span class=\"cpp_keyword\">unsigned</span>" + result;		break;
		}
	}

	void Visit(ReferenceType* self)override
	{
		switch (self->reference)
		{
		case CppReferenceType::Ptr:				result = L" *" + result;	break;
		case CppReferenceType::LRef:			result = L" &" + result;	break;
		case CppReferenceType::RRef:			result = L" &&" + result;	break;
		}
		self->type->Accept(this);
	}

	void Visit(ArrayType* self)override
	{
		if (result != L"")
		{
			result = L"(" + result + L")";
		}

		if (self->expr)
		{
			result += L"[...]";
		}
		else
		{
			result += L"[]";
		}

		self->type->Accept(this);
	}

	void Visit(CallingConventionType* self)override
	{
		switch (self->callingConvention)
		{
		case TsysCallingConvention::CDecl:		result = L" <span class=\"cpp_keyword\">__cdecl</span>" + result;		break;
		case TsysCallingConvention::ClrCall:	result = L" <span class=\"cpp_keyword\">__clrcall</span>" + result;		break;
		case TsysCallingConvention::StdCall:	result = L" <span class=\"cpp_keyword\">__stdcall</span>" + result;		break;
		case TsysCallingConvention::FastCall:	result = L" <span class=\"cpp_keyword\">__fastcall</span>" + result;	break;
		case TsysCallingConvention::ThisCall:	result = L" <span class=\"cpp_keyword\">__thiscall</span>" + result;	break;
		case TsysCallingConvention::VectorCall:	result = L" <span class=\"cpp_keyword\">__vectorcall</span>" + result;	break;
		}
		self->type->Accept(this);
	}

	void Visit(FunctionType* self)override
	{
		if (result != L"")
		{
			result = L"(" + result + L")";
		}

		result += L"(";
		for (vint i = 0; i < self->parameters.Count(); i++)
		{
			if (i != 0) result += L", ";
			GetDisplayNameInHtmlTypeVisitor visitor;
			self->parameters[i].item->type->Accept(&visitor);
			if (self->parameters[i].isVariadic)
			{
				result += L"...";
			}
			result += visitor.result;
		}
		if (self->ellipsis)
		{
			result += L" ...";
		}
		result += L")";

		if (self->decoratorReturnType)
		{
			result += L"->";
			GetDisplayNameInHtmlTypeVisitor visitor;
			self->decoratorReturnType->Accept(&visitor);
			result += visitor.result;
		}

		self->returnType->Accept(this);
	}

	void Visit(MemberType* self)override
	{
		result = L"::" + result;
		self->classType->Accept(this);
		result = L" " + result;
		self->type->Accept(this);
	}

	void Visit(DeclType* self)override
	{
		if (self->expr)
		{
			result = L"<span class=\"cpp_keyword\">decltype</span>(...)" + result;
		}
		else
		{
			result = L"<span class=\"cpp_keyword\">decltype</span>(<span class=\"cpp_keyword\">auto</span>)" + result;
		}
	}

	void Visit(DecorateType* self)override
	{
		if (self->isVolatile) result = L"<span class=\"cpp_keyword\"> volatile</span>" + result;
		if (self->isConst) result = L"<span class=\"cpp_keyword\"> const</span>" + result;
		if (self->isConstExpr) result = L"<span class=\"cpp_keyword\"> constexpr</span>" + result;
		self->type->Accept(this);
	}

	void Visit(RootType* self)override
	{
		result = L"::" + result;
	}

	void Visit(IdType* self)override
	{
		if (self->resolving)
		{
			result = GetDisplayNameInHtml(self->resolving->resolvedSymbols[0]) + result;
		}
		else
		{
			result = self->name.name + result;
		}
	}

	void Visit(ChildType* self)override
	{
		if (self->resolving)
		{
			result = GetDisplayNameInHtml(self->resolving->resolvedSymbols[0]) + result;
		}
		else
		{
			result = L"::" + self->name.name + result;
			self->classType->Accept(this);
		}
	}

	void Visit(GenericType* self)override
	{
		WString currentResult = result;
		WString genericResult;

		{
			result = L"";
			self->type->Accept(this);
			genericResult = result + L"<";
		}
		for (vint i = 0; i < self->arguments.Count(); i++)
		{
			if (i > 0) genericResult += L", ";
			auto argument = self->arguments[i];
			if (argument.item.type)
			{
				result = L"";
				argument.item.type->Accept(this);
				genericResult += result;
			}
			else
			{
				genericResult += L"...";
			}
		}
		genericResult += L">";

		result = genericResult + L" " + currentResult;
	}
};

WString GetDisplayNameInHtml(Symbol* symbol)
{
	WString displayNameInHtml;
	if (symbol->kind == symbol_component::SymbolKind::Namespace)
	{
		auto current = symbol;
		while (current->kind != symbol_component::SymbolKind::Root)
		{
			if (current != symbol) displayNameInHtml = L"::" + displayNameInHtml;
			displayNameInHtml = current->name + displayNameInHtml;
			current = current->GetParentScope();
		}
	}
	else if (symbol->kind == symbol_component::SymbolKind::Variable && (symbol->GetParentScope() && !symbol->GetParentScope()->GetImplDecl_NFb<ClassDeclaration>()))
	{
		displayNameInHtml = symbol->name;
	}
	else
	{
		auto current = symbol;
		while (current->kind != symbol_component::SymbolKind::Root && current->kind != symbol_component::SymbolKind::Namespace)
		{
			if (current != symbol) displayNameInHtml = L"::" + displayNameInHtml;
			if (auto divClass = GetSymbolDivClass(current))
			{
				displayNameInHtml = L"<span class=\"" + WString(divClass, false) + L"\">" + current->name + L"</span>" + displayNameInHtml;
			}
			else
			{
				displayNameInHtml = current->name + displayNameInHtml;
			}
			current = current->GetParentScope();
		}
	}

	if (auto funcDecl = symbol->GetAnyForwardDecl<ForwardFunctionDeclaration>())
	{
		if (auto funcType = GetTypeWithoutMemberAndCC(funcDecl->type).Cast<FunctionType>())
		{
			displayNameInHtml += L"(";
			for (vint i = 0; i < funcType->parameters.Count(); i++)
			{
				if (i != 0) displayNameInHtml += L", ";
				GetDisplayNameInHtmlTypeVisitor visitor;
				funcType->parameters[i].item->type->Accept(&visitor);
				displayNameInHtml += visitor.result;
				if (funcType->parameters[i].isVariadic)
				{
					displayNameInHtml += L"...";
				}
			}
			if (funcType->ellipsis)
			{
				displayNameInHtml += L" ...";
			}
			displayNameInHtml += L")";
		}
	}
	return displayNameInHtml;
}

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
				writer.WriteString(L"<div class=\"expandable\"/>");
			}
			writer.WriteString(L"<div class=\"disabled\"/>");
			for (vint i = originalIndex; i < disableEnd; i++)
			{
				if (i > originalIndex)
				{
					writer.WriteLine(L"");
				}
				auto reading = originalLines[i].Buffer();
				while (auto c = *reading++)
				{
					switch (c)
					{
					case L'<':
						writer.WriteString(L"&lt;");
						break;
					case L'>':
						writer.WriteString(L"&gt;");
						break;
					case L'&':
						writer.WriteString(L"&amp;");
						break;
					case L'\'':
						writer.WriteString(L"&apos;");
						break;
					case L'\"':
						writer.WriteString(L"&quot;");
						break;
					default:
						writer.WriteChar(c);
					}
				}
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

/***********************************************************************
GenerateReferencedSymbols
***********************************************************************/

void GenerateReferencedSymbols(Ptr<FileLinesRecord> flr, StreamWriter& writer)
{
	writer.WriteLine(L"referencedSymbols = {");
	for (vint i = 0; i < flr->refSymbols.Count(); i++)
	{
		auto symbol = flr->refSymbols[i];
		writer.WriteString(L"    \'");
		writer.WriteString(symbol->uniqueId);
		writer.WriteLine(L"\': {");

		writer.WriteString(L"        \'displayNameInHtml\': \'");
		writer.WriteString(GetDisplayNameInHtml(symbol));
		writer.WriteLine(L"\',");

		List<WString> impls, decls;
		switch (symbol->GetCategory())
		{
		case symbol_component::SymbolCategory::Normal:
			{
				if (symbol->GetImplDecl_NFb())
				{
					impls.Add(L"NI$" + symbol->uniqueId);
				}
				for (vint j = 0; j < symbol->GetForwardDecls_N().Count(); j++)
				{
					decls.Add(L"NF[" + itow(j) + L"]$" + symbol->uniqueId);
				}
			}
			break;
		case symbol_component::SymbolCategory::Function:
			{
				auto& symbols = symbol->GetImplSymbols_F();
				for (vint j = 0; j < symbols.Count(); j++)
				{
					impls.Add(L"FB$" + symbols[j]->uniqueId);
				}
			}
			{
				auto& symbols = symbol->GetForwardSymbols_F();
				for (vint j = 0; j < symbols.Count(); j++)
				{
					decls.Add(L"FB$" + symbols[j]->uniqueId);
				}
			}
			break;
		case symbol_component::SymbolCategory::FunctionBody:
			throw UnexpectedSymbolCategoryException();
		}

		if (impls.Count() == 0)
		{
			writer.WriteLine(L"        \'impls\': [],");
		}
		else
		{
			writer.WriteLine(L"        \'impls\': [");
			for (vint j = 0; j < impls.Count(); j++)
			{
				writer.WriteString(L"            \'");
				writer.WriteString(impls[j]);
				if (j == impls.Count() - 1)
				{
					writer.WriteLine(L"\'");
				}
				else
				{
					writer.WriteLine(L"\',");
				}
			}
			writer.WriteLine(L"        ],");
		}

		if (decls.Count() == 0)
		{
			writer.WriteLine(L"        \'decls\': []");
		}
		else
		{
			writer.WriteLine(L"        \'decls\': [");
			for (vint j = 0; j < decls.Count(); j++)
			{
				writer.WriteString(L"            \'");
				writer.WriteString(decls[j]);
				if (j == decls.Count() - 1)
				{
					writer.WriteLine(L"\'");
				}
				else
				{
					writer.WriteLine(L"\',");
				}
			}
			writer.WriteLine(L"        ]");
		}

		if (i == flr->refSymbols.Count() - 1)
		{
			writer.WriteLine(L"    }");
		}
		else
		{
			writer.WriteLine(L"    },");
		}
	}
	writer.WriteLine(L"};");
}

/***********************************************************************
GenerateSymbolToFiles
***********************************************************************/

void GenerateSymbolToFiles(Ptr<GlobalLinesRecord> global, Ptr<FileLinesRecord> flr, StreamWriter& writer)
{
	writer.WriteLine(L"symbolToFiles = {");
	bool firstFileMapping = false;
	auto writeFileMapping = [&](const WString& prefix, Ptr<Declaration> decl)
	{
		vint index = global->declToFiles.Keys().IndexOf(decl.Obj());
		if (index == -1) return;

		if (!firstFileMapping)
		{
			firstFileMapping = true;
		}
		else
		{
			writer.WriteLine(L",");
		}

		writer.WriteString(L"    \'");
		writer.WriteString(prefix);
		writer.WriteString(decl->symbol->uniqueId);
		writer.WriteString(L"\': ");

		auto filePath = global->declToFiles.Values()[index];
		auto flrTarget = global->fileLines[filePath];
		if (flrTarget == flr)
		{
			writer.WriteString(L"null");
		}
		else
		{
			writer.WriteString(L"{ \'htmlFileName\': \'");
			writer.WriteString(flrTarget->htmlFileName);
			writer.WriteString(L"\', \'displayName\': \'");
			writer.WriteString(flrTarget->filePath.GetName());
			writer.WriteString(L"\' }");
		}
	};
	for (vint i = 0; i < flr->refSymbols.Count(); i++)
	{
		auto symbol = flr->refSymbols[i];
		switch (symbol->GetCategory())
		{
		case symbol_component::SymbolCategory::Normal:
			{
				if (auto decl = symbol->GetImplDecl_NFb())
				{
					writeFileMapping(L"NI$", decl);
				}
				for (vint j = 0; j < symbol->GetForwardDecls_N().Count(); j++)
				{
					auto decl = symbol->GetForwardDecls_N()[j];
					writeFileMapping(L"NF[" + itow(j) + L"]$", decl);
				}
			}
			break;
		case symbol_component::SymbolCategory::Function:
			{
				auto& symbols = symbol->GetImplSymbols_F();
				for (vint j = 0; j < symbols.Count(); j++)
				{
					writeFileMapping(L"FB$", symbols[j]->GetImplDecl_NFb());
				}
			}
			{
				auto& symbols = symbol->GetForwardSymbols_F();
				for (vint j = 0; j < symbols.Count(); j++)
				{
					writeFileMapping(L"FB$", symbols[j]->GetForwardDecl_Fb());
				}
			}
			break;
		case symbol_component::SymbolCategory::FunctionBody:
			throw UnexpectedSymbolCategoryException();
		}
	}
}

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