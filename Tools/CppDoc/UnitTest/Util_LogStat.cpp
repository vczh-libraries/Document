#include <Ast_Stat.h>
#include "Util.h"

/***********************************************************************
LogStatVisitor
***********************************************************************/

class LogStatVisitor : public Object, public virtual IStatVisitor, private LogIndentation
{
public:
	LogStatVisitor(StreamWriter& _writer, vint _indentation)
		:LogIndentation(_writer, _indentation)
	{
	}

	void WriteSubStat(Ptr<Stat> stat)
	{
		if (stat.Cast<BlockStat>())
		{
			WriteIndentation();
			indentation++;
			stat->Accept(this);
			indentation--;
		}
		else
		{
			indentation++;
			WriteIndentation();
			stat->Accept(this);
			indentation--;
		}
	}

	void Visit(EmptyStat* self)override
	{
		writer.WriteLine(L";");
	}

	void Visit(BlockStat* self)override
	{
		bool hasIdentation = indentation > 0;
		if (hasIdentation) indentation--;
		writer.WriteLine(L"{");
		indentation++;
		for (vint i = 0; i < self->stats.Count(); i++)
		{
			WriteIndentation();
			self->stats[i]->Accept(this);
		}
		indentation--;
		WriteIndentation();
		writer.WriteLine(L"}");
		if (hasIdentation) indentation++;
	}

	void Visit(DeclStat* self)override
	{
		for (vint i = 0; i < self->decls.Count(); i++)
		{
			if (i != 0)
			{
				WriteIndentation();
			}
			Log(self->decls[i], writer, indentation, true);
		}
	}

	void Visit(ExprStat* self)override
	{
		Log(self->expr, writer, indentation);
		writer.WriteLine(L";");
	}

	void Visit(LabelStat* self)override
	{
		writer.WriteString(self->name.name);
		writer.WriteString(L": ");
		self->stat->Accept(this);
	}

	void Visit(DefaultStat* self)override
	{
		writer.WriteString(L"default: ");
		self->stat->Accept(this);
	}

	void Visit(CaseStat* self)override
	{
		writer.WriteString(L"case ");
		Log(self->expr, writer, indentation);
		writer.WriteString(L": ");
		self->stat->Accept(this);
	}

	void Visit(GotoStat* self)override
	{
		writer.WriteString(L"goto ");
		writer.WriteString(self->name.name);
		writer.WriteLine(L";");
	}

	void Visit(BreakStat* self)override
	{
		writer.WriteLine(L"break;");
	}

	void Visit(ContinueStat* self)override
	{
		writer.WriteLine(L"continue;");
	}

	void Visit(WhileStat* self)override
	{
		writer.WriteString(L"while (");
		if (self->varExpr)
		{
			Log(self->varExpr, writer, indentation, false);
		}
		else
		{
			Log(self->expr, writer, indentation);
		}
		writer.WriteLine(L")");
		WriteSubStat(self->stat);
	}

	void Visit(DoWhileStat* self)override
	{
		writer.WriteLine(L"do");
		WriteSubStat(self->stat);

		WriteIndentation();
		writer.WriteString(L"while (");
		Log(self->expr, writer, indentation);
		writer.WriteLine(L");");
	}

	void Visit(ForEachStat* self)override
	{
		writer.WriteString(L"foreach (");
		Log(self->varDecl, writer, 0, false);
		writer.WriteString(L" : ");
		Log(self->expr, writer, indentation);
		writer.WriteLine(L")");
		WriteSubStat(self->stat);
	}

	void Visit(ForStat* self)override
	{
		writer.WriteString(L"for (");
		if (self->init)
		{
			Log(self->init, writer, indentation);
		}
		else
		{
			for (vint i = 0; i < self->varDecls.Count(); i++)
			{
				if (i > 0)
				{
					writer.WriteString(L", ");
				}
				Log(self->varDecls[i], writer, 0, false);
			}
		}
		writer.WriteString(L"; ");

		if (self->expr) Log(self->expr, writer, indentation);
		writer.WriteString(L"; ");

		if (self->effect) Log(self->effect, writer, indentation);
		writer.WriteLine(L")");
		WriteSubStat(self->stat);
	}

	void Visit(IfElseStat* self)override
	{
		writer.WriteString(L"if (");
		for (vint i = 0; i < self->varDecls.Count(); i++)
		{
			Log(self->varDecls[i], writer, 0, false);
			if (i == self->varDecls.Count() - 1)
			{
				writer.WriteString(L"; ");
			}
			else
			{
				writer.WriteString(L", ");
			}
		}

		if (self->varExpr)
		{
			Log(self->varExpr, writer, indentation, false);
		}
		else
		{
			Log(self->expr, writer, indentation);
		}
		writer.WriteLine(L")");
		WriteSubStat(self->trueStat);

		if (self->falseStat)
		{
			WriteIndentation();
			if (self->falseStat.Cast<IfElseStat>())
			{
				writer.WriteString(L"else ");
				self->falseStat->Accept(this);
			}
			else
			{
				writer.WriteLine(L"else");
				WriteSubStat(self->falseStat);
			}
		}
	}

	void Visit(SwitchStat* self)override
	{
		writer.WriteString(L"switch (");
		if (self->varExpr)
		{
			Log(self->varExpr, writer, indentation, false);
		}
		else
		{
			Log(self->expr, writer, indentation);
		}
		writer.WriteLine(L")");
		WriteSubStat(self->stat);
	}

	void Visit(TryCatchStat* self)override
	{
		writer.WriteLine(L"try");
		WriteSubStat(self->tryStat);

		for (vint i = 0; i < self->catchStats.Count(); i++)
		{
			auto catchStat = self->catchStats[i];
			WriteIndentation();

			if (catchStat.exception)
			{
				writer.WriteString(L"catch (");
				Log(catchStat.exception, writer, 0, false);
				writer.WriteLine(L")");
			}
			else
			{
				writer.WriteLine(L"catch (...)");
			}

			WriteSubStat(catchStat.catchStat);
		}
	}

	void Visit(ReturnStat* self)override
	{
		if (self->expr)
		{
			writer.WriteString(L"return ");
			Log(self->expr, writer, indentation);
			writer.WriteLine(L";");
		}
		else
		{
			writer.WriteLine(L"return;");
		}
	}

	void Visit(__Try__ExceptStat* self)override
	{
		writer.WriteLine(L"__try");
		WriteSubStat(self->tryStat);

		WriteIndentation();
		writer.WriteString(L"__except (");
		Log(self->expr, writer, indentation);
		writer.WriteLine(L")");
		WriteSubStat(self->exceptStat);
	}

	void Visit(__Try__FinallyStat* self)override
	{
		writer.WriteLine(L"__try");
		WriteSubStat(self->tryStat);

		WriteIndentation();
		writer.WriteLine(L"__finally");
		WriteSubStat(self->finallyStat);
	}

	void Visit(__LeaveStat* self)override
	{
		writer.WriteLine(L"__leave;");
	}

	void Visit(__IfExistsStat* self)override
	{
		writer.WriteString(L"__if_exists (");
		Log(self->expr, writer, indentation);
		writer.WriteLine(L")");
		WriteSubStat(self->stat);
	}

	void Visit(__IfNotExistsStat* self)override
	{
		writer.WriteString(L"__if_not_exists (");
		Log(self->expr, writer, indentation);
		writer.WriteLine(L")");
		WriteSubStat(self->stat);
	}
};

/***********************************************************************
Log
***********************************************************************/

void Log(Ptr<Stat> stat, StreamWriter& writer, vint indentation)
{
	LogStatVisitor visitor(writer, indentation);
	stat->Accept(&visitor);
}