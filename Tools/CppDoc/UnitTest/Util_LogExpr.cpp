#include <Ast_Expr.h>
#include "Util.h"

/***********************************************************************
LogExprVisitor
***********************************************************************/

class LogExprVisitor : public Object, public virtual IExprVisitor
{
private:
	StreamWriter&			writer;

public:
	LogExprVisitor(StreamWriter& _writer)
		:writer(_writer)
	{
	}

	void Visit(PlaceholderExpr* self)override
	{
		writer.WriteString(L"<PLACE-HOLDER>");
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

	void Visit(ThisExpr* self)override
	{
		writer.WriteString(L"this");
	}

	void Visit(NullptrExpr* self)override
	{
		writer.WriteString(L"nullptr");
	}

	void Visit(ParenthesisExpr* self)override
	{
		writer.WriteChar(L'(');
		self->expr->Accept(this);
		writer.WriteChar(L')');
	}

	void Visit(CastExpr* self)override
	{
		switch (self->castType)
		{
		case CppCastType::CCast:
			writer.WriteString(L"c_cast<");
			break;
		case CppCastType::DynamicCast:
			writer.WriteString(L"dynamic_cast<");
			break;
		case CppCastType::StaticCast:
			writer.WriteString(L"static_cast<");
			break;
		case CppCastType::ConstCast:
			writer.WriteString(L"const_cast<");
			break;
		case CppCastType::ReinterpretCast:
			writer.WriteString(L"reinterpret_cast<");
			break;
		case CppCastType::SafeCast:
			writer.WriteString(L"safe_cast<");
			break;
		}

		Log(self->type, writer);
		writer.WriteString(L">(");
		Log(self->expr, writer);
		writer.WriteString(L")");
	}

	void Visit(TypeidExpr* self)override
	{
		writer.WriteString(L"typeid(");
		if (self->type) Log(self->type, writer);
		if (self->expr) Log(self->expr, writer);
		writer.WriteString(L")");
	}

	void Visit(SizeofExpr* self)override
	{
		writer.WriteString(L"sizeof");
		if (self->ellipsis) writer.WriteString(L"...");
		writer.WriteString(L"(");
		if (self->type) Log(self->type, writer);
		if (self->expr) Log(self->expr, writer);
		writer.WriteString(L")");
	}

	void Visit(ThrowExpr* self)override
	{
		writer.WriteString(L"throw(");
		if (self->expr) Log(self->expr, writer);
		writer.WriteString(L")");
	}

	void Visit(DeleteExpr* self)override
	{
		writer.WriteString(self->arrayDelete ? L"delete[] (" : L"delete (");
		self->expr->Accept(this);
		writer.WriteString(L")");
	}

	void Visit(IdExpr* self)override
	{
		writer.WriteString(self->name.name);
	}

	void Visit(ChildExpr* self)override
	{
		Log(self->classType, writer);
		writer.WriteString(L" :: ");
		writer.WriteString(self->name.name);
	}

	void Visit(FieldAccessExpr* self)override
	{
		Log(self->expr, writer);
		switch (self->type)
		{
		case CppFieldAccessType::Dot: writer.WriteString(L"."); break;
		case CppFieldAccessType::Arrow: writer.WriteString(L"->"); break;
		}
		Log(Ptr<Expr>(self->name), writer);
	}

	void Visit(ArrayAccessExpr* self)override
	{
		Log(self->expr, writer);
		writer.WriteChar(L'[');
		Log(self->index, writer);
		writer.WriteChar(L']');
	}

	void Visit(FuncAccessExpr* self)override
	{
		Log(self->expr, writer);
		writer.WriteChar(L'(');
		for (vint i = 0; i < self->arguments.Count(); i++)
		{
			if (i > 0) writer.WriteString(L", ");
			Log(self->arguments[i].item, writer);
			if (self->arguments[i].isVariadic)
			{
				writer.WriteString(L"...");
			}
		}
		writer.WriteChar(L')');
	}

	void Visit(CtorAccessExpr* self)override
	{
		Log(self->type, writer);
		if (self->initializer)
		{
			writer.WriteChar(self->initializer->initializerType == CppInitializerType::Constructor ? L'(' : L'{');
			for (vint i = 0; i < self->initializer->arguments.Count(); i++)
			{
				if (i > 0) writer.WriteString(L", ");
				Log(self->initializer->arguments[i].item, writer);
				if (self->initializer->arguments[i].isVariadic)
				{
					writer.WriteString(L"...");
				}
			}
			writer.WriteChar(self->initializer->initializerType == CppInitializerType::Constructor ? L')' : L'}');
		}
	}

	void Visit(NewExpr* self)override
	{
		writer.WriteString(L"new ");
		if (self->placementArguments.Count() > 0)
		{
			writer.WriteString(L"(");
			for (vint i = 0; i < self->placementArguments.Count(); i++)
			{
				if (i > 0) writer.WriteString(L", ");
				Log(self->placementArguments[i].item, writer);
				if (self->placementArguments[i].isVariadic)
				{
					writer.WriteString(L"...");
				}
			}
			writer.WriteString(L") ");
		}

		Visit((CtorAccessExpr*)self);
	}

	void Visit(UniversalInitializerExpr* self)override
	{
		writer.WriteChar(L'{');
		for (vint i = 0; i < self->arguments.Count(); i++)
		{
			if (i > 0) writer.WriteString(L", ");
			Log(self->arguments[i].item, writer);
			if (self->arguments[i].isVariadic)
			{
				writer.WriteString(L"...");
			}
		}
		writer.WriteChar(L'}');
	}

	void Visit(PostfixUnaryExpr* self)override
	{
		writer.WriteString(L"(");
		Log(self->operand, writer);
		writer.WriteString(L" ");
		writer.WriteString(self->opName.name);
		writer.WriteString(L")");
	}

	void Visit(PrefixUnaryExpr* self)override
	{
		writer.WriteString(L"(");
		writer.WriteString(self->opName.name);
		writer.WriteString(L" ");
		Log(self->operand, writer);
		writer.WriteString(L")");
	}

	void Visit(BinaryExpr* self)override
	{
		writer.WriteString(L"(");
		Log(self->left, writer);
		writer.WriteString(L" ");
		writer.WriteString(self->opName.name);
		writer.WriteString(L" ");
		Log(self->right, writer);
		writer.WriteString(L")");
	}

	void Visit(IfExpr* self)override
	{
		writer.WriteString(L"(");
		Log(self->condition, writer);
		writer.WriteString(L" ? ");
		Log(self->left, writer);
		writer.WriteString(L" : ");
		Log(self->right, writer);
		writer.WriteString(L")");
	}

	void Visit(GenericExpr* self)override
	{
		Log(Ptr<Expr>(self->expr), writer);
		Log(self->arguments, L"<", L">", writer);
	}

	void Visit(BuiltinFuncAccessExpr* self)override
	{
		Log(Ptr<Expr>(self->name), writer);
		Log(self->arguments, L"(", L")", writer);
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