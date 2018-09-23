#include <Parser.h>
#include <Ast_Type.h>
#include <Ast_Decl.h>
#include <Ast_Expr.h>
#include <Ast_Stat.h>

extern void Log(Ptr<Expr> expr, StreamWriter& writer);
extern void Log(Ptr<Type> type, StreamWriter& writer);

/***********************************************************************
LogExprVisitor
***********************************************************************/

class LogExprVisitor : public Object, public virtual IExprVisitor
{
public:
	StreamWriter&			writer;

	LogExprVisitor(StreamWriter& _writer)
		:writer(_writer)
	{
	}

	void Visit(LiteralExpr* self)override
	{
		for (vint i = 0; i < self->tokens.Count(); i++)
		{
			if (i != 0)
			{
				writer.WriteChar(L' ');
			}
			writer.WriteString(self->tokens[i].reading, self->tokens[i].length);
		}
	}
};

/***********************************************************************
LogTypeVisitor
***********************************************************************/

class LogTypeVisitor : public Object, public virtual ITypeVisitor
{
public:
	StreamWriter&			writer;

	LogTypeVisitor(StreamWriter& _writer)
		:writer(_writer)
	{
	}

	void Visit(IdType* self)override
	{
		throw 0;
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

	void Visit(FunctionType* self)override
	{
		throw 0;
	}

	void Visit(MemberType* self)override
	{
		throw 0;
	}

	void Visit(DeclType* self)override
	{
		writer.WriteString(L"decltype(");
		Log(self->expr, writer);
		writer.WriteString(L")");
	}

	void Visit(DecorateType* self)override
	{
		Log(self->type, writer);
		if (self->isConstExpr)	writer.WriteString(L" constexpr");
		if (self->isConst)		writer.WriteString(L" const");
		if (self->isVolatile)	writer.WriteString(L" volatile");
	}

	void Visit(ChildType* self)override
	{
		throw 0;
	}

	void Visit(GenericType* self)override
	{
		Log(self->type, writer);
		writer.WriteString(L"<");
		for (vint i = 0; i < self->arguments.Count(); i++)
		{
			if (i != 0)
			{
				writer.WriteString(L", ");
			}

			auto arg = self->arguments[i];
			if (arg.expr) Log(arg.expr, writer);
			if (arg.type) Log(arg.type, writer);
		}
		writer.WriteString(L">");
	}

	void Visit(VariadicTemplateArgumentType* self)override
	{
		self->type->Accept(this);
		writer.WriteString(L"...");
	}
};

/***********************************************************************
Log
***********************************************************************/

void Log(Ptr<Expr> expr, StreamWriter& writer)
{
	LogExprVisitor visitor(writer);
	expr->Accept(&visitor);
}

void Log(Ptr<Type> type, StreamWriter& writer)
{
	LogTypeVisitor visitor(writer);
	type->Accept(&visitor);
}