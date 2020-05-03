#include <Ast_Type.h>
#include "Util.h"

/***********************************************************************
LogTypeVisitor
***********************************************************************/

class LogTypeVisitor : public Object, public virtual ITypeVisitor
{
private:
	StreamWriter&			writer;

public:
	LogTypeVisitor(StreamWriter& _writer)
		:writer(_writer)
	{
	}

	void Visit(PrimitiveType* self)override
	{
		switch (self->prefix)
		{
		case CppPrimitivePrefix::_signed:
			writer.WriteString(L"signed ");
			break;
		case CppPrimitivePrefix::_unsigned:
			writer.WriteString(L"unsigned ");
			break;
		}

		switch (self->primitive)
		{
		case CppPrimitiveType::_auto:			writer.WriteString(L"auto");		break;
		case CppPrimitiveType::_void:			writer.WriteString(L"void");		break;
		case CppPrimitiveType::_bool:			writer.WriteString(L"bool");		break;
		case CppPrimitiveType::_char:			writer.WriteString(L"char");		break;
		case CppPrimitiveType::_wchar_t:		writer.WriteString(L"wchar_t");		break;
		case CppPrimitiveType::_char16_t:		writer.WriteString(L"char16_t");	break;
		case CppPrimitiveType::_char32_t:		writer.WriteString(L"char32_t");	break;
		case CppPrimitiveType::_short:			writer.WriteString(L"short");		break;
		case CppPrimitiveType::_int:			writer.WriteString(L"int");			break;
		case CppPrimitiveType::___int8:			writer.WriteString(L"__int8");		break;
		case CppPrimitiveType::___int16:		writer.WriteString(L"__int16");		break;
		case CppPrimitiveType::___int32:		writer.WriteString(L"__int32");		break;
		case CppPrimitiveType::___int64:		writer.WriteString(L"__int64");		break;
		case CppPrimitiveType::_long:			writer.WriteString(L"long");		break;
		case CppPrimitiveType::_long_int:		writer.WriteString(L"long int");	break;
		case CppPrimitiveType::_long_long:		writer.WriteString(L"long long");	break;
		case CppPrimitiveType::_float:			writer.WriteString(L"float");		break;
		case CppPrimitiveType::_double:			writer.WriteString(L"double");		break;
		case CppPrimitiveType::_long_double:	writer.WriteString(L"long double");	break;
		default:
			throw 0;
		}
	}

	void Visit(ReferenceType* self)override
	{
		Log(self->type, writer);
		switch (self->reference)
		{
		case CppReferenceType::Ptr:				writer.WriteString(L" *");	break;
		case CppReferenceType::LRef:			writer.WriteString(L" &");	break;
		case CppReferenceType::RRef:			writer.WriteString(L" &&");	break;
		default:
			throw 0;
		}
	}

	void Visit(ArrayType* self)override
	{
		Log(self->type, writer);
		writer.WriteString(L" [");
		if (self->expr)
		{
			Log(self->expr, writer);
		}
		writer.WriteString(L"]");
	}

	void Visit(CallingConventionType* self)override
	{
		Log(self->type, writer);

		switch (self->callingConvention)
		{
		case TsysCallingConvention::CDecl:
			writer.WriteString(L" __cdecl");
			break;
		case TsysCallingConvention::ClrCall:
			writer.WriteString(L" __clrcall");
			break;
		case TsysCallingConvention::StdCall:
			writer.WriteString(L" __stdcall");
			break;
		case TsysCallingConvention::FastCall:
			writer.WriteString(L" __fastcall");
			break;
		case TsysCallingConvention::ThisCall:
			writer.WriteString(L" __thiscall");
			break;
		case TsysCallingConvention::VectorCall:
			writer.WriteString(L" __vectorcall");
			break;
		}
	}

	void Visit(FunctionType* self)override
	{
		if (self->decoratorReturnType)
		{
			writer.WriteChar(L'(');
			Log(self->returnType, writer);
			writer.WriteString(L"->");
			Log(self->decoratorReturnType, writer);
			writer.WriteChar(L')');
		}
		else
		{
			Log(self->returnType, writer);
		}

		writer.WriteString(L" (");
		for (vint i = 0; i < self->parameters.Count(); i++)
		{
			if (i != 0)
			{
				writer.WriteString(L", ");
			}

			Log(self->parameters[i].item, writer, 0, false);
			if (self->parameters[i].isVariadic)
			{
				writer.WriteString(L"...");
			}
		}
		writer.WriteChar(L')');

		if (self->qualifierConstExpr) writer.WriteString(L" constexpr");
		if (self->qualifierConst) writer.WriteString(L" const");
		if (self->qualifierVolatile) writer.WriteString(L" volatile");
		if (self->qualifierLRef) writer.WriteString(L" &");
		if (self->qualifierRRef) writer.WriteString(L" &&");

		if (self->decoratorOverride) writer.WriteString(L" override");
		if (self->decoratorNoExcept) writer.WriteString(L" noexcept");
		if (self->decoratorThrow)
		{
			writer.WriteString(L" throw(");
			for (vint i = 0; i < self->exceptions.Count(); i++)
			{
				if (i != 0)
				{
					writer.WriteString(L", ");
				}
				Log(self->exceptions[i], writer);
			}
			writer.WriteChar(L')');
		}
	}

	void Visit(MemberType* self)override
	{
		Log(self->type, writer);
		writer.WriteString(L" (");
		Log(self->classType, writer);
		writer.WriteString(L" ::)");
	}

	void Visit(DeclType* self)override
	{
		writer.WriteString(L"decltype(");
		if (self->expr)
		{
			Log(self->expr, writer);
		}
		else
		{
			writer.WriteString(L"auto");
		}
		writer.WriteString(L")");
	}

	void Visit(DecorateType* self)override
	{
		Log(self->type, writer);
		if (self->isConst)		writer.WriteString(L" const");
		if (self->isVolatile)	writer.WriteString(L" volatile");
	}

	void Visit(RootType* self)override
	{
		writer.WriteString(L"__root");
	}

	void Visit(IdType* self)override
	{
		if (self->cStyleTypeReference)
		{
			writer.WriteString(L"enum_class_struct_union ");
		}
		writer.WriteString(self->name.name);
	}

	void Visit(ChildType* self)override
	{
		Log(Ptr<Type>(self->classType), writer);
		writer.WriteString(L" :: ");
		if (self->typenameType) writer.WriteString(L"typename ");
		writer.WriteString(self->name.name);
	}

	void Visit(GenericType* self)override
	{
		Log(Ptr<Type>(self->type), writer);
		Log(self->arguments, L"<", L">", writer);
	}
};

/***********************************************************************
Log
***********************************************************************/

void Log(Ptr<Type> type, StreamWriter& writer)
{
	if (type)
	{
		LogTypeVisitor visitor(writer);
		type->Accept(&visitor);
	}
	else
	{
		writer.WriteString(L"__null");
	}
}