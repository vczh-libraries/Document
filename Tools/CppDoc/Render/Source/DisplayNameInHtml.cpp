#include "Render.h"

/***********************************************************************
GetTypeDisplayNameInHtml
***********************************************************************/

class GetDisplayNameInHtmlTypeVisitor : public Object, public ITypeVisitor
{
public:
	WString											result;
	bool											renderTypeArguments;

	GetDisplayNameInHtmlTypeVisitor(bool _renderTypeArguments)
		:renderTypeArguments(_renderTypeArguments)
	{
	}

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
			result += L"[<span class=\"cpp_keyword\">(expr)</span>]";
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

		result += AppendFunctionParametersInHtml(self);

		if (self->decoratorReturnType)
		{
			result += L"->";
			result += GetTypeDisplayNameInHtml(self->decoratorReturnType);
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
			result = GetUnscopedSymbolDisplayNameInHtml(self->resolving->resolvedSymbols[0], renderTypeArguments) + result;
		}
		else
		{
			result = HtmlTextSingleLineToString(self->name.name) + result;
		}
	}

	void Visit(ChildType* self)override
	{
		WString nameType;
		if (self->resolving)
		{
			nameType = GetUnscopedSymbolDisplayNameInHtml(self->resolving->resolvedSymbols[0], renderTypeArguments);
		}
		else
		{
			nameType = HtmlTextSingleLineToString(self->name.name);
		}
		result = GetTypeDisplayNameInHtml(self->classType) + L"::" + nameType + result;
	}

	void Visit(GenericType* self)override
	{
		result = GetTypeDisplayNameInHtml(self->type, false) + AppendGenericArguments(self->arguments) + result;
	}
};

WString GetTypeDisplayNameInHtml(Ptr<Type> type, bool renderTypeArguments)
{
	GetDisplayNameInHtmlTypeVisitor visitor(renderTypeArguments);
	type->Accept(&visitor);
	return visitor.result;
}

/***********************************************************************
AppendFunctionParametersInHtml
***********************************************************************/

WString AppendFunctionParametersInHtml(FunctionType* funcType)
{
	WString result = L"(";
	for (vint i = 0; i < funcType->parameters.Count(); i++)
	{
		if (i != 0) result += L", ";
		result += GetTypeDisplayNameInHtml(funcType->parameters[i].item->type);
		if (funcType->parameters[i].isVariadic)
		{
			result += L"...";
		}
	}
	if (funcType->ellipsis)
	{
		if (funcType->parameters.Count() > 0)
		{
			result += L", ";
		}
		result += L"...";
	}
	result += L")";

	return result;
}

/***********************************************************************
AppendTemplateArguments
***********************************************************************/

WString AppendTemplateArguments(List<TemplateSpec::Argument>& arguments)
{
	WString result = HtmlTextSingleLineToString(L"<");
	for (vint i = 0; i < arguments.Count(); i++)
	{
		if (i > 0) result += L", ";
		auto argument = arguments[i];
		if (argument.argumentType == CppTemplateArgumentType::Value)
		{
			result += L"<span class=\"cpp_keyword\">(expr)</span>";
		}
		else
		{
			result += L"<span class=\"cpp_type\">";
			result += HtmlTextSingleLineToString(argument.name.name);
			result += L"</span>";
		}
		if (argument.ellipsis)
		{
			result += L" ...";
		}
	}
	result += HtmlTextSingleLineToString(L">");

	return result;
}

/***********************************************************************
AppendGenericArguments
***********************************************************************/

WString AppendGenericArguments(VariadicList<GenericArgument>& arguments)
{
	WString result = HtmlTextSingleLineToString(L"<");
	for (vint i = 0; i < arguments.Count(); i++)
	{
		if (i > 0) result += L", ";
		auto argument = arguments[i];
		if (argument.item.expr)
		{
			result += L"<span class=\"cpp_keyword\">(expr)</span>";
		}
		else
		{
			result += GetTypeDisplayNameInHtml(argument.item.type, true);
		}
		if (argument.isVariadic)
		{
			result += L" ...";
		}
	}
	result += HtmlTextSingleLineToString(L">");

	return result;
}

/***********************************************************************
GetSymbolDisplayNameInHtml
***********************************************************************/

WString GetUnscopedSymbolDisplayNameInHtml(Symbol* symbol, bool renderTypeArguments)
{
	WString result;

	Ptr<TemplateSpec> templateSpec = symbol_type_resolving::GetTemplateSpecFromSymbol(symbol);
	Ptr<SpecializationSpec> specializationSpec;
	WString declName = symbol->name;
	switch (symbol->kind)
	{
	case symbol_component::SymbolKind::Class:
	case symbol_component::SymbolKind::Struct:
	case symbol_component::SymbolKind::Union:
		{
			auto decl = symbol->GetAnyForwardDecl<ForwardClassDeclaration>();
			specializationSpec = decl->specializationSpec;
			declName = decl->name.name;
		}
		break;
	case symbol_component::SymbolKind::FunctionSymbol:
		{
			auto decl = symbol->GetAnyForwardDecl<ForwardFunctionDeclaration>();
			specializationSpec = decl->specializationSpec;
			declName = decl->name.name;
		}
		break;
	}

	const wchar_t* divClass = GetSymbolDivClass(symbol);
	if (divClass)
	{
		result += L"<span class=\"";
		result += divClass;
		result += L"\">";
	}
	result += HtmlTextSingleLineToString(declName);
	if (divClass)
	{
		result += L"</span>";
	}

	if (renderTypeArguments)
	{
		if (specializationSpec)
		{
			result += AppendGenericArguments(specializationSpec->arguments);
		}
		else if (templateSpec)
		{
			result += AppendTemplateArguments(templateSpec->arguments);
		}
	}
	return result;
}

/***********************************************************************
GetSymbolDisplayNameInHtml
***********************************************************************/

WString GetSymbolDisplayNameInHtml(Symbol* symbol)
{
	switch (symbol->kind)
	{
	case symbol_component::SymbolKind::GenericTypeArgument:
		return L"<span class=\"cpp_type\">" + HtmlTextSingleLineToString(symbol->name) + L"</span>";
	case symbol_component::SymbolKind::GenericValueArgument:
		return HtmlTextSingleLineToString(symbol->name);
	}

	WString displayNameInHtml;
	if (symbol->kind == symbol_component::SymbolKind::Namespace)
	{
		auto current = symbol;
		while (current->kind != symbol_component::SymbolKind::Root)
		{
			if (current != symbol) displayNameInHtml = L"::" + displayNameInHtml;
			displayNameInHtml = HtmlTextSingleLineToString(current->name) + displayNameInHtml;
			current = current->GetParentScope();
		}
	}
	else if (symbol->kind == symbol_component::SymbolKind::Variable && (symbol->GetParentScope() && !symbol->GetParentScope()->GetImplDecl_NFb<ClassDeclaration>()))
	{
		displayNameInHtml = HtmlTextSingleLineToString(symbol->name);
	}
	else
	{
		auto current = symbol;
		while (current->kind != symbol_component::SymbolKind::Root && current->kind != symbol_component::SymbolKind::Namespace)
		{
			if (current != symbol) displayNameInHtml = L"::" + displayNameInHtml;
			displayNameInHtml = GetUnscopedSymbolDisplayNameInHtml(current, true) + displayNameInHtml;
			current = current->GetParentScope();
		}
	}

	if (auto funcDecl = symbol->GetAnyForwardDecl<ForwardFunctionDeclaration>())
	{
		if (auto funcType = GetTypeWithoutMemberAndCC(funcDecl->type).Cast<FunctionType>())
		{
			displayNameInHtml += AppendFunctionParametersInHtml(funcType.Obj());
		}
	}
	return displayNameInHtml;
}