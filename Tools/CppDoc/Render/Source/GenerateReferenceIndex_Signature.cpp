#include "Render.h"
#include <Symbol_TemplateSpec.h>

WString AppendFunctionParametersInSignature(FunctionType* funcType);
WString AppendTemplateArgumentsInSignature(List<TemplateSpec::Argument>& arguments);
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
		result += GetTypeDisplayNameInSignature(funcType->parameters[i].item->type);
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
AppendTemplateArgumentsInSignature
***********************************************************************/

WString AppendTemplateArgumentsInSignature(List<TemplateSpec::Argument>& arguments)
{
	WString signature = L"<";
	for (vint i = 0; i < arguments.Count(); i++)
	{
		if (i > 0) signature += L", ";
		auto argument = arguments[i];
		if (argument.argumentType == CppTemplateArgumentType::Value)
		{
			signature += L"expr";
		}
		else
		{
			signature += argument.name.name;
		}
		if (argument.ellipsis)
		{
			signature += L" ...";
		}
	}
	signature += L">";

	return signature;
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

WString GetSymbolDisplayNameInSignature(Symbol* symbol)
{
	throw 0;
}