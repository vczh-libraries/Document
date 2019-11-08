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
GenerateReferencedSymbols
***********************************************************************/

void GenerateReferencedSymbols(Ptr<FileLinesRecord> flr, StreamWriter& writer)
{
	Dictionary<WString, Symbol*> referencedSymbols;
	for (vint i = 0; i < flr->refSymbols.Count(); i++)
	{
		auto symbol = flr->refSymbols[i];
		referencedSymbols.Add(symbol->uniqueId, symbol);
	}

	writer.WriteLine(L"referencedSymbols = {");
	for (vint i = 0; i < referencedSymbols.Count(); i++)
	{
		auto symbol = referencedSymbols.Values()[i];
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