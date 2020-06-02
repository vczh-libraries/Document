#include <Ast_Expr.h>
#include <Ast_Stat.h>
#include "Util.h"

/***********************************************************************
LogExprVisitor
***********************************************************************/

class LogExprVisitor : public Object, public virtual IExprVisitor, private LogIndentation
{
public:
	LogExprVisitor(StreamWriter& _writer, vint _indentation)
		:LogIndentation(_writer, _indentation)
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

		Log(self->type, writer, indentation);
		writer.WriteString(L">(");
		Log(self->expr, writer, indentation);
		writer.WriteString(L")");
	}

	void Visit(TypeidExpr* self)override
	{
		writer.WriteString(L"typeid(");
		if (self->type) Log(self->type, writer, indentation);
		if (self->expr) Log(self->expr, writer, indentation);
		writer.WriteString(L")");
	}

	void Visit(SizeofExpr* self)override
	{
		writer.WriteString(L"sizeof");
		if (self->ellipsis) writer.WriteString(L"...");
		writer.WriteString(L"(");
		if (self->type) Log(self->type, writer, indentation);
		if (self->expr) Log(self->expr, writer, indentation);
		writer.WriteString(L")");
	}

	void Visit(ThrowExpr* self)override
	{
		writer.WriteString(L"throw(");
		if (self->expr) Log(self->expr, writer, indentation);
		writer.WriteString(L")");
	}

	void Visit(DeleteExpr* self)override
	{
		if (self->globalOperator)
		{
			writer.WriteString(L"::");
		}
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
		Log(self->classType, writer, indentation);
		writer.WriteString(L" :: ");
		writer.WriteString(self->name.name);
	}

	void Visit(FieldAccessExpr* self)override
	{
		Log(self->expr, writer, indentation);
		switch (self->type)
		{
		case CppFieldAccessType::Dot: writer.WriteString(L"."); break;
		case CppFieldAccessType::Arrow: writer.WriteString(L"->"); break;
		}
		Log(Ptr<Expr>(self->name), writer, indentation);
	}

	void Visit(ArrayAccessExpr* self)override
	{
		Log(self->expr, writer, indentation);
		writer.WriteChar(L'[');
		Log(self->index, writer, indentation);
		writer.WriteChar(L']');
	}

	void Visit(FuncAccessExpr* self)override
	{
		Log(self->expr, writer, indentation);
		writer.WriteChar(L'(');
		for (vint i = 0; i < self->arguments.Count(); i++)
		{
			if (i > 0) writer.WriteString(L", ");
			Log(self->arguments[i].item, writer, indentation);
			if (self->arguments[i].isVariadic)
			{
				writer.WriteString(L"...");
			}
		}
		writer.WriteChar(L')');
	}

	void Visit(CtorAccessExpr* self)override
	{
		Log(self->type, writer, indentation);
		if (self->initializer)
		{
			writer.WriteChar(self->initializer->initializerType == CppInitializerType::Constructor ? L'(' : L'{');
			for (vint i = 0; i < self->initializer->arguments.Count(); i++)
			{
				if (i > 0) writer.WriteString(L", ");
				Log(self->initializer->arguments[i].item, writer, indentation);
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
		if (self->globalOperator)
		{
			writer.WriteString(L"::");
		}
		writer.WriteString(L"new ");
		if (self->placementArguments.Count() > 0)
		{
			writer.WriteString(L"(");
			for (vint i = 0; i < self->placementArguments.Count(); i++)
			{
				if (i > 0) writer.WriteString(L", ");
				Log(self->placementArguments[i].item, writer, indentation);
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
			Log(self->arguments[i].item, writer, indentation);
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
		Log(self->operand, writer, indentation);
		writer.WriteString(L" ");
		writer.WriteString(self->opName.name);
		writer.WriteString(L")");
	}

	void Visit(PrefixUnaryExpr* self)override
	{
		writer.WriteString(L"(");
		writer.WriteString(self->opName.name);
		writer.WriteString(L" ");
		Log(self->operand, writer, indentation);
		writer.WriteString(L")");
	}

	void Visit(BinaryExpr* self)override
	{
		writer.WriteString(L"(");
		Log(self->left, writer, indentation);
		writer.WriteString(L" ");
		writer.WriteString(self->opName.name);
		writer.WriteString(L" ");
		Log(self->right, writer, indentation);
		writer.WriteString(L")");
	}

	void Visit(IfExpr* self)override
	{
		writer.WriteString(L"(");
		Log(self->condition, writer, indentation);
		writer.WriteString(L" ? ");
		Log(self->left, writer, indentation);
		writer.WriteString(L" : ");
		Log(self->right, writer, indentation);
		writer.WriteString(L")");
	}

	void Visit(GenericExpr* self)override
	{
		Log(Ptr<Expr>(self->expr), writer, indentation);
		Log(self->arguments, L"<", L">", writer, indentation);
	}

	void Visit(LambdaExpr* self)override
	{
		writer.WriteString(L"[");
		switch (self->captureDefault)
		{
		case LambdaExpr::CaptureDefaultKind::Copy:
			writer.WriteString(L"=");
			break;
		case LambdaExpr::CaptureDefaultKind::Ref:
			writer.WriteString(L"&");
			break;
		}

		for (vint i = 0; i < self->captures.Count(); i++)
		{
			auto capture = self->captures[i];
			if (self->captureDefault != LambdaExpr::CaptureDefaultKind::None || i > 0)
			{
				writer.WriteString(L", ");
			}
			switch (capture.kind)
			{
			case LambdaExpr::CaptureKind::CopyThis:
				writer.WriteString(L"*this");
				break;
			case LambdaExpr::CaptureKind::RefThis:
				writer.WriteString(L"this");
				break;
			case LambdaExpr::CaptureKind::Copy:
				writer.WriteString(capture.name.name);
				break;
			case LambdaExpr::CaptureKind::Ref:
				writer.WriteString(L"&");
				writer.WriteString(capture.name.name);
				break;
			}
		}

		for (vint i = 0; i < self->varDecls.Count(); i++)
		{
			auto varDecl = self->varDecls[i];
			if (self->captureDefault != LambdaExpr::CaptureDefaultKind::None || self->captures.Count() > 0 || i > 0)
			{
				writer.WriteString(L", ");
			}
			writer.WriteString(varDecl->name.name);
			writer.WriteString(L" = ");
			Log(varDecl->initializer->arguments[0].item, writer, indentation);
		}
		writer.WriteString(L"] ");

		Log(Ptr<Type>(self->type), writer, indentation);

		auto blockStat = self->statement.Cast<BlockStat>();
		writer.WriteString(L" {");
		if (blockStat->stats.Count() > 0)
		{
			writer.WriteLine(L"");
			indentation++;
			for (vint i = 0; i < blockStat->stats.Count(); i++)
			{
				WriteIndentation();
				Log(blockStat->stats[i], writer, indentation);
			}
			indentation--;
			WriteIndentation();
		}
		writer.WriteString(L"}");
	}

	void Visit(BuiltinFuncAccessExpr* self)override
	{
		Log(Ptr<Expr>(self->name), writer, indentation);
		Log(self->arguments, L"(", L")", writer, indentation);
	}
};

/***********************************************************************
Log
***********************************************************************/

void Log(Ptr<Expr> expr, StreamWriter& writer, vint indentation)
{
	LogExprVisitor visitor(writer, indentation);
	expr->Accept(&visitor);
}