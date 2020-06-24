#include "Render.h"
#include <Symbol_TemplateSpec.h>

WString AppendFunctionParametersInSignature(FunctionType* funcType);
WString AppendGenericArgumentsInSignature(VariadicList<GenericArgument>& arguments);
WString GetUnscopedSymbolDisplayNameInSignature(Symbol* symbol);

/***********************************************************************
GetTypeDisplayNameInSignature
***********************************************************************/

class GetSignatureTypeVisitor : public Object, public ITypeVisitor
{
public:
	WString											signature;
	bool											needParenthesesForFuncArray = false;
	FunctionType*									topLevelFunctionType = nullptr;

	GetSignatureTypeVisitor()
	{
	}

	void Visit(PrimitiveType* self)override
	{
		switch (self->primitive)
		{
		case CppPrimitiveType::_auto:			signature = L"auto" + signature;		break;
		case CppPrimitiveType::_void:			signature = L"void" + signature;		break;
		case CppPrimitiveType::_bool:			signature = L"bool" + signature;		break;
		case CppPrimitiveType::_char:			signature = L"char" + signature;		break;
		case CppPrimitiveType::_wchar_t:		signature = L"wchar_t" + signature;		break;
		case CppPrimitiveType::_char16_t:		signature = L"char16_t" + signature;	break;
		case CppPrimitiveType::_char32_t:		signature = L"char32_t" + signature;	break;
		case CppPrimitiveType::_short:			signature = L"short" + signature;		break;
		case CppPrimitiveType::_int:			signature = L"int" + signature;			break;
		case CppPrimitiveType::___int8:			signature = L"__int8" + signature;		break;
		case CppPrimitiveType::___int16:		signature = L"__int16" + signature;		break;
		case CppPrimitiveType::___int32:		signature = L"__int32" + signature;		break;
		case CppPrimitiveType::___int64:		signature = L"__int64;" + signature;	break;
		case CppPrimitiveType::_long:			signature = L"long" + signature;		break;
		case CppPrimitiveType::_long_int:		signature = L"long int" + signature;	break;
		case CppPrimitiveType::_long_long:		signature = L"long long" + signature;	break;
		case CppPrimitiveType::_float:			signature = L"float" + signature;		break;
		case CppPrimitiveType::_double:			signature = L"double" + signature;		break;
		case CppPrimitiveType::_long_double:	signature = L"long double" + signature;	break;
		}

		switch (self->prefix)
		{
		case CppPrimitivePrefix::_signed:		signature = L"signed" + signature;		break;
		case CppPrimitivePrefix::_unsigned:		signature = L"unsigned" + signature;	break;
		}
	}

	void Visit(ReferenceType* self)override
	{
		switch (self->reference)
		{
		case CppReferenceType::Ptr:				signature = L" *" + signature;	break;
		case CppReferenceType::LRef:			signature = L" &" + signature;	break;
		case CppReferenceType::RRef:			signature = L" &&" + signature;	break;
		}
		needParenthesesForFuncArray = true;
		self->type->Accept(this);
	}

	void Visit(ArrayType* self)override
	{
		if (signature != L"" && needParenthesesForFuncArray)
		{
			signature = L"(" + signature + L")";
		}

		Type* elementType = nullptr;
		while (self)
		{
			if (self->expr)
			{
				signature += L"[expr]";
			}
			else
			{
				signature += L"[]";
			}

			elementType = self->type.Obj();
			self = dynamic_cast<ArrayType*>(elementType);
		}
		needParenthesesForFuncArray = true;
		self->type->Accept(this);
	}

	void Visit(DecorateType* self)override
	{
		if (self->isVolatile) signature = L" volatile" + signature;
		if (self->isConst) signature = L" const" + signature;
		needParenthesesForFuncArray = true;
		self->type->Accept(this);
	}

	void Visit(CallingConventionType* self)override
	{
		switch (self->callingConvention)
		{
		case TsysCallingConvention::CDecl:		signature = L" __cdecl" + signature;		break;
		case TsysCallingConvention::ClrCall:	signature = L" __clrcall" + signature;		break;
		case TsysCallingConvention::StdCall:	signature = L" __stdcall" + signature;		break;
		case TsysCallingConvention::FastCall:	signature = L" __fastcall" + signature;		break;
		case TsysCallingConvention::ThisCall:	signature = L" __thiscall" + signature;		break;
		case TsysCallingConvention::VectorCall:	signature = L" __vectorcall" + signature;	break;
		}
		self->type->Accept(this);
	}

	void Visit(FunctionType* self)override
	{
		if (signature != L"" && needParenthesesForFuncArray)
		{
			signature = L"(" + signature + L")";
		}

		signature += AppendFunctionParametersInSignature(self);

		if (self->decoratorReturnType)
		{
			signature += L"->";
			signature += GetTypeDisplayNameInSignature(self->decoratorReturnType);
		}

		if (self->qualifierConst)		signature += L" const";
		if (self->qualifierVolatile)	signature += L" volatile";
		if (self->qualifierLRef)		signature += L" &";
		if (self->qualifierRRef)		signature += L" &&";

		needParenthesesForFuncArray = true;
		self->returnType->Accept(this);
	}

	void Visit(MemberType* self)override
	{
		signature = L"::" + signature;
		self->classType->Accept(this);
		signature = L" " + signature;

		needParenthesesForFuncArray = true;
		self->type->Accept(this);
	}

	void Visit(DeclType* self)override
	{
		if (self->expr)
		{
			signature = L"decltype(expr)" + signature;
		}
		else
		{
			signature = L"decltype(auto)" + signature;
		}
	}

	void Visit(RootType* self)override
	{
		signature = L"::" + signature;
	}

	void Visit(IdType* self)override
	{
		if (self->resolving)
		{
			signature = GetUnscopedSymbolDisplayNameInSignature(self->resolving->items[0].symbol) + signature;
		}
		else
		{
			signature = self->name.name + signature;
		}
	}

	void Visit(ChildType* self)override
	{
		WString nameType;
		if (self->resolving)
		{
			nameType = GetUnscopedSymbolDisplayNameInSignature(self->resolving->items[0].symbol);
		}
		else
		{
			nameType = self->name.name;
		}
		signature = GetTypeDisplayNameInSignature(self->classType) + L"::" + nameType + signature;
	}

	void Visit(GenericType* self)override
	{
		signature = GetTypeDisplayNameInSignature(self->type) + AppendGenericArgumentsInSignature(self->arguments) + signature;
	}
};

WString GetTypeDisplayNameInSignature(Ptr<Type> type)
{
	GetSignatureTypeVisitor visitor;
	visitor.topLevelFunctionType = GetTypeWithoutMemberAndCC(type).Cast<FunctionType>().Obj();
	type->Accept(&visitor);
	return visitor.signature;
}

WString GetTypeDisplayNameInSignature(Ptr<Type> type, const WString& signature, bool needParenthesesForFuncArray)
{
	GetSignatureTypeVisitor visitor;
	visitor.signature = signature;
	visitor.needParenthesesForFuncArray = needParenthesesForFuncArray;
	visitor.topLevelFunctionType = GetTypeWithoutMemberAndCC(type).Cast<FunctionType>().Obj();
	type->Accept(&visitor);
	return visitor.signature;
}

/***********************************************************************
AppendFunctionParametersInSignature
***********************************************************************/

WString AppendFunctionParametersInSignature(FunctionType* funcType)
{
	WString result = L"(";
	for (vint i = 0; i < funcType->parameters.Count(); i++)
	{
		if (i != 0) result += L", ";
		if (funcType->parameters[i].isVariadic)
		{
			result += GetTypeDisplayNameInSignature(funcType->parameters[i].item->type, L"...", true);
		}
		else
		{
			result += GetTypeDisplayNameInSignature(funcType->parameters[i].item->type);
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
AppendGenericArgumentsInSignature
***********************************************************************/

WString AppendGenericArgumentsInSignature(VariadicList<GenericArgument>& arguments)
{
	WString signature = L"<";
	for (vint i = 0; i < arguments.Count(); i++)
	{
		if (i > 0) signature += L", ";
		auto argument = arguments[i];
		if (argument.item.expr)
		{
			signature += L"expr";
		}
		else
		{
			signature += GetTypeDisplayNameInSignature(argument.item.type);
		}
		if (argument.isVariadic)
		{
			signature += L" ...";
		}
	}
	signature += L">";

	return signature;
}

/***********************************************************************
GetSymbolDisplayNameInSignature
***********************************************************************/

WString GetUnscopedSymbolDisplayNameInSignature(Symbol* symbol)
{
	switch (symbol->kind)
	{
	case symbol_component::SymbolKind::Class:
	case symbol_component::SymbolKind::Struct:
	case symbol_component::SymbolKind::Union:
		return symbol->GetAnyForwardDecl<ForwardClassDeclaration>()->name.name;
	case symbol_component::SymbolKind::FunctionSymbol:
		return symbol->GetAnyForwardDecl<ForwardFunctionDeclaration>()->name.name;
	default:
		return symbol->name;
	}
}

/***********************************************************************
GetSymbolDisplayNameInSignature
***********************************************************************/

void WriteTemplateSpecInSignature(Ptr<TemplateSpec> spec, const WString& indent, StreamWriter& writer)
{
	writer.WriteString(indent);
	writer.WriteLine(L"template <");

	for (vint i = 0; i < spec->arguments.Count(); i++)
	{
		auto argument = spec->arguments[i];
		switch (argument.argumentType)
		{
		case CppTemplateArgumentType::HighLevelType:
			WriteTemplateSpecInSignature(argument.templateSpec, indent + L"    ", writer);
			writer.WriteString(indent);
			if (argument.name)
			{
				if (argument.ellipsis)
				{
					writer.WriteString(L"    class... ");
				}
				else
				{
					writer.WriteString(L"    class ");
				}
				writer.WriteString(argument.name.name);
			}
			else
			{
				writer.WriteString(L"    class");
				if (argument.ellipsis) writer.WriteString(L"...");
			}
			if (argument.type) writer.WriteString(L" /* optional */");
			break;
		case CppTemplateArgumentType::Type:
			writer.WriteString(indent);
			if (argument.name)
			{
				if (argument.ellipsis)
				{
					writer.WriteString(L"    typename... ");
				}
				else
				{
					writer.WriteString(L"    typename ");
				}
				writer.WriteString(argument.name.name);
			}
			else
			{
				writer.WriteString(L"    typename");
				if (argument.ellipsis) writer.WriteString(L"...");
			}
			if (argument.type) writer.WriteString(L" /* optional */");
			break;
		case CppTemplateArgumentType::Value:
			writer.WriteString(indent);
			writer.WriteString(L"    ");
			{
				WString name = argument.name.name;
				if (argument.ellipsis) name = L"... " + name;
				writer.WriteString(GetTypeDisplayNameInSignature(argument.type, name, argument.ellipsis));
			}
			if (argument.expr) writer.WriteString(L" /* optional */");
			break;
		}

		if (i < spec->arguments.Count() - 1) writer.WriteString(L", ");
		writer.WriteLine(L"");
	}

	writer.WriteString(indent);
	writer.WriteLine(L">");
}

WString GetSymbolDisplayNameInSignature(Symbol* symbol)
{
	switch (symbol->kind)
	{
	case symbol_component::SymbolKind::Enum:
		return GenerateToStream([=](StreamWriter& writer)
		{
			auto fdecl = symbol->GetAnyForwardDecl<ForwardEnumDeclaration>();
			auto decl = fdecl.Cast<EnumDeclaration>();

			writer.WriteString(L"enum ");
			if (fdecl->enumClass) writer.WriteString(L"class ");
			if (fdecl->name) writer.WriteString(fdecl->name.name);
			if (decl)
			{
				writer.WriteLine(L"");
				writer.WriteLine(L"{");
				for (vint i = 0; i < decl->items.Count(); i++)
				{
					writer.WriteString(L"    ");
					writer.WriteString(decl->items[i]->name.name);
					writer.WriteLine(L",");
				}
				writer.WriteLine(L"};");
			}
			else
			{
				writer.WriteLine(L";");
			}
		});
	case CLASS_SYMBOL_KIND:
		return GenerateToStream([=](StreamWriter& writer)
		{
			auto fdecl = symbol->GetAnyForwardDecl<ForwardClassDeclaration>();
			if (fdecl->templateSpec)
			{
				WriteTemplateSpecInSignature(fdecl->templateSpec, L"", writer);
			}
			switch (symbol->kind)
			{
			case symbol_component::SymbolKind::Class:
				writer.WriteString(L"class ");
				break;
			case symbol_component::SymbolKind::Struct:
				writer.WriteString(L"struct ");
				break;
			case symbol_component::SymbolKind::Union:
				writer.WriteString(L"union ");
				break;
			}
			if (fdecl->name) writer.WriteString(fdecl->name.name);
			if (fdecl->specializationSpec) writer.WriteString(AppendGenericArgumentsInSignature(fdecl->specializationSpec->arguments));
			writer.WriteLine(L";");
			// TODO: write base class
		});
	case symbol_component::SymbolKind::TypeAlias:
		return GenerateToStream([=](StreamWriter& writer)
		{
			auto decl = symbol->GetImplDecl_NFb<TypeAliasDeclaration>();
			if (decl->templateSpec)
			{
				WriteTemplateSpecInSignature(decl->templateSpec, L"", writer);
			}
			writer.WriteString(L"using ");
			writer.WriteString(decl->name.name);
			writer.WriteString(L" = ");
			writer.WriteString(GetTypeDisplayNameInSignature(decl->type));
			writer.WriteLine(L";");
		});
	case symbol_component::SymbolKind::ValueAlias:
		return GenerateToStream([=](StreamWriter& writer)
		{
			auto decl = symbol->GetImplDecl_NFb<ValueAliasDeclaration>();
			if (decl->templateSpec)
			{
				WriteTemplateSpecInSignature(decl->templateSpec, L"", writer);
			}
			if (decl->decoratorConstexpr) writer.WriteString(L"constexpr ");
			writer.WriteString(GetTypeDisplayNameInSignature(decl->type, decl->name.name, false));
			writer.WriteLine(L" = expr;");
		});
	case symbol_component::SymbolKind::Namespace:
		return GenerateToStream([=](StreamWriter& writer)
		{
			writer.WriteString(L"namespace ");
			writer.WriteString(symbol->name);
			writer.WriteLine(L";");
		});
	case symbol_component::SymbolKind::Variable:
		return L"";
	case symbol_component::SymbolKind::FunctionSymbol:
		return L"";
	default:
		throw L"Unexpected symbol kind.";
	}
}