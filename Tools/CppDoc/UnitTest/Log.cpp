#include <Parser.h>
#include <Ast_Type.h>
#include <Ast_Decl.h>
#include <Ast_Expr.h>
#include <Ast_Stat.h>

extern void Log(Ptr<Declarator> declarator, StreamWriter& writer);
extern void Log(Ptr<Expr> expr, StreamWriter& writer);
extern void Log(Ptr<Type> type, StreamWriter& writer);

void Log(Ptr<Declarator> declarator, StreamWriter& writer)
{
	if (declarator->name)
	{
		writer.WriteString(declarator->name.name);
		writer.WriteString(L": ");
	}

	Log(declarator->type, writer);

	if (declarator->initializer)
	{
		switch (declarator->initializer->initializerType)
		{
		case InitializerType::Equal:
			writer.WriteString(L" = ");
			break;
		case InitializerType::Constructor:
			writer.WriteString(L" (");
			break;
		case InitializerType::Universal:
			writer.WriteString(L" {");
			break;
		}

		for (vint i = 0; i < declarator->initializer->arguments.Count(); i++)
		{
			if (i != 0)
			{
				writer.WriteString(L", ");
			}
			Log(declarator->initializer->arguments[i], writer);
		}

		switch (declarator->initializer->initializerType)
		{
		case InitializerType::Constructor:
			writer.WriteChar(L')');
			break;
		case InitializerType::Universal:
			writer.WriteChar(L'}');
			break;
		}
	}
}

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

	void Visit(CallingConventionType* self)override
	{
		Log(self->type, writer);

		switch (self->callingConvention)
		{
		case CppCallingConvention::CDecl:
			writer.WriteString(L" __cdecl");
			break;
		case CppCallingConvention::ClrCall:
			writer.WriteString(L" __clrcall");
			break;
		case CppCallingConvention::StdCall:
			writer.WriteString(L" __stdcall");
			break;
		case CppCallingConvention::FastCall:
			writer.WriteString(L" __fastcall");
			break;
		case CppCallingConvention::ThisCall:
			writer.WriteString(L" __thiscall");
			break;
		case CppCallingConvention::VectorCall:
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
			Log(self->parameters[i], writer);
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
LogStatVisitor
***********************************************************************/

class LogStatVisitor : public Object, public virtual IStatVisitor
{
public:
	StreamWriter&			writer;

	LogStatVisitor(StreamWriter& _writer)
		:writer(_writer)
	{
	}

	void Visit(BlockStat* self)override
	{
		throw 0;
	}
};

/***********************************************************************
LogDeclVisitor
***********************************************************************/

class LogDeclVisitor : public Object, public virtual IDeclarationVisitor
{
public:
	StreamWriter&			writer;

	LogDeclVisitor(StreamWriter& _writer)
		:writer(_writer)
	{
	}

	void Visit(ForwardVariableDeclaration* self)override
	{
		throw 0;
	}

	void Visit(ForwardFunctionDeclaration* self)override
	{
		throw 0;
	}

	void Visit(ForwardEnumDeclaration* self)override
	{
		throw 0;
	}

	void Visit(ForwardClassDeclaration* self)override
	{
		throw 0;
	}

	void Visit(VariableDeclaration* self)override
	{
		throw 0;
	}

	void Visit(FunctionDeclaration* self)override
	{
		throw 0;
	}

	void Visit(EnumItemDeclaration* self)override
	{
		throw 0;
	}

	void Visit(EnumDeclaration* self)override
	{
		throw 0;
	}

	void Visit(ClassDeclaration* self)override
	{
		throw 0;
	}

	void Visit(TypeAliasDeclaration* self)override
	{
		throw 0;
	}

	void Visit(UsingDeclaration* self)override
	{
		throw 0;
	}

	void Visit(NamespaceDeclaration* self)override
	{
		throw 0;
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

void Log(Ptr<Stat> stat, StreamWriter& writer)
{
	LogStatVisitor visitor(writer);
	stat->Accept(&visitor);
}

void Log(Ptr<Declaration> decl, StreamWriter& writer)
{
	LogDeclVisitor visitor(writer);
	decl->Accept(&visitor);
}