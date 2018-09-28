#include "Parser.h"
#include "Ast_Stat.h"
#include "Ast_Decl.h"

Ptr<Stat> ParseStat(const ParsingArguments& pa, Ptr<CppTokenCursor>& cursor)
{
	if (TestToken(cursor, CppTokens::SEMICOLON))
	{
		return MakePtr<EmptyStat>();
	}
	else if (TestToken(cursor, CppTokens::LBRACE))
	{
		auto stat = MakePtr<BlockStat>();
		while (!TestToken(cursor, CppTokens::RBRACE))
		{
			stat->stats.Add(ParseStat(pa, cursor));
		}
		return stat;
	}
	else if (TestToken(cursor, CppTokens::STAT_DEFAULT))
	{
		RequireToken(cursor, CppTokens::COLON);
		auto stat = MakePtr<DefaultStat>();
		stat->stat = ParseStat(pa, cursor);
		return stat;
	}
	else if (TestToken(cursor, CppTokens::STAT_CASE))
	{
		auto stat = MakePtr<CaseStat>();
		stat->expr = ParseExpr(pa, false, cursor);
		RequireToken(cursor, CppTokens::COLON);
		stat->stat = ParseStat(pa, cursor);
		return stat;
	}
	else if (TestToken(cursor, CppTokens::STAT_GOTO))
	{
		auto stat = MakePtr<GotoStat>();
		if (!ParseCppName(stat->name, cursor))
		{
			throw StopParsingException(cursor);
		}
		RequireToken(cursor, CppTokens::SEMICOLON);
		return stat;
	}
	else if (TestToken(cursor, CppTokens::STAT_BREAK))
	{
		RequireToken(cursor, CppTokens::SEMICOLON);
		return MakePtr<BreakStat>();
	}
	else if (TestToken(cursor, CppTokens::STAT_CONTINUE))
	{
		RequireToken(cursor, CppTokens::SEMICOLON);
		return MakePtr<ContinueStat>();
	}
	else if (TestToken(cursor, CppTokens::STAT_WHILE))
	{
		auto stat = MakePtr<WhileStat>();
		RequireToken(cursor, CppTokens::LPARENTHESIS);
		stat->expr = ParseExpr(pa, true, cursor);
		RequireToken(cursor, CppTokens::RPARENTHESIS);
		stat->stat = ParseStat(pa, cursor);
		return stat;
	}
	else if (TestToken(cursor, CppTokens::STAT_DO))
	{
		auto stat = MakePtr<DoWhileStat>();
		stat->stat = ParseStat(pa, cursor);
		RequireToken(cursor, CppTokens::STAT_WHILE);
		RequireToken(cursor, CppTokens::LPARENTHESIS);
		stat->expr = ParseExpr(pa, true, cursor);
		RequireToken(cursor, CppTokens::RPARENTHESIS);
		RequireToken(cursor, CppTokens::SEMICOLON);
		return stat;
	}
	else if (TestToken(cursor, CppTokens::STAT_FOR))
	{
		RequireToken(cursor, CppTokens::LPARENTHESIS);

		{
			auto oldCursor = cursor;
			List<Ptr<Declarator>> declarators;
			try
			{
				ParseDeclarator(pa, DeclaratorRestriction::One, InitializerRestriction::Zero, cursor, declarators);
				RequireToken(cursor, CppTokens::COLON);
			}
			catch (const StopParsingException&)
			{
				cursor = oldCursor;
				goto FOR_EACH_FAILED;
			}

			if (declarators.Count() != 1)
			{
				throw StopParsingException(cursor);
			}

			auto stat = MakePtr<ForEachStat>();
			{
				auto varDecl = MakePtr<VariableDeclaration>();
				varDecl->name = declarators[0]->name;
				varDecl->type = declarators[0]->type;
				stat->varDecl = varDecl;
			}

			stat->expr = ParseExpr(pa, true, cursor);
			RequireToken(cursor, CppTokens::RPARENTHESIS);
			stat->stat = ParseStat(pa, cursor);
			return stat;
		}
	FOR_EACH_FAILED:
		{
			auto stat = MakePtr<ForStat>();
			stat->init = ParseStat(pa, cursor);
			if (!stat->init.Cast<EmptyStat>() && !stat->init.Cast<ExprStat>() && !stat->init.Cast<DeclStat>())
			{
				throw StopParsingException(cursor);
			}
			if (!TestToken(cursor, CppTokens::SEMICOLON))
			{
				stat->expr = ParseExpr(pa, true, cursor);
				RequireToken(cursor, CppTokens::SEMICOLON);
			}
			if (!TestToken(cursor, CppTokens::RPARENTHESIS))
			{
				stat->effect = ParseExpr(pa, true, cursor);
				RequireToken(cursor, CppTokens::RPARENTHESIS);
			}
			stat->stat = ParseStat(pa, cursor);
			return stat;
		}
	}
	else if (TestToken(cursor, CppTokens::STAT_IF))
	{
		auto stat = MakePtr<IfElseStat>();
		RequireToken(cursor, CppTokens::LPARENTHESIS);
		{
			auto oldCursor = cursor;
			try
			{
				stat->init = ParseStat(pa, cursor);
			}
			catch (const StopParsingException&)
			{
				cursor = oldCursor;
			}
		}
		{
			auto oldCursor = cursor;
			List<Ptr<Declarator>> declarators;
			try
			{
				ParseDeclarator(pa, DeclaratorRestriction::One, InitializerRestriction::Optional, cursor, declarators);
				if (declarators.Count() != 1)
				{
					throw StopParsingException(cursor);
				}
				if (!declarators[0]->initializer)
				{
					throw StopParsingException(cursor);
				}
			}
			catch (const StopParsingException&)
			{
				cursor = oldCursor;
			}

			if (declarators.Count() == 1)
			{
				auto varDecl = MakePtr<VariableDeclaration>();
				varDecl->name = declarators[0]->name;
				varDecl->type = declarators[0]->type;
				varDecl->initializer = declarators[0]->initializer;
				stat->varDecl = varDecl;
			}
			else
			{
				stat->expr = ParseExpr(pa, true, cursor);
			}
		}
		RequireToken(cursor, CppTokens::RPARENTHESIS);
		stat->trueStat = ParseStat(pa, cursor);

		if (TestToken(cursor, CppTokens::STAT_ELSE))
		{
			stat->falseStat = ParseStat(pa, cursor);
		}
		return stat;
	}
	else if (TestToken(cursor, CppTokens::STAT_SWITCH))
	{
		auto stat = MakePtr<SwitchStat>();
		RequireToken(cursor, CppTokens::LPARENTHESIS);
		stat->expr = ParseExpr(pa, true, cursor);
		RequireToken(cursor, CppTokens::RPARENTHESIS);
		stat->stat = ParseStat(pa, cursor);
		return stat;
	}
	else if (TestToken(cursor, CppTokens::STAT_TRY))
	{
		auto stat = MakePtr<TryCatchStat>();
		stat->tryStat = ParseStat(pa, cursor);
		RequireToken(cursor, CppTokens::STAT_CATCH);
		RequireToken(cursor, CppTokens::LPARENTHESIS);
		if (!TestToken(cursor, CppTokens::DOT, CppTokens::DOT, CppTokens::DOT))
		{
			List<Ptr<Declarator>> declarators;
			ParseDeclarator(pa, DeclaratorRestriction::Optional, InitializerRestriction::Zero, cursor, declarators);
			if (declarators.Count() != 1)
			{
				throw StopParsingException(cursor);
			}
			stat->exception = declarators[0];
		}
		RequireToken(cursor, CppTokens::RPARENTHESIS);
		stat->catchStat = ParseStat(pa, cursor);
		return stat;
	}
	else if (TestToken(cursor, CppTokens::STAT_RETURN))
	{
		auto stat = MakePtr<ReturnStat>();
		if (!TestToken(cursor, CppTokens::SEMICOLON))
		{
			stat->expr = ParseExpr(pa, true, cursor);
			RequireToken(cursor, CppTokens::SEMICOLON);
		}
		return stat;
	}
	else if (TestToken(cursor, CppTokens::STAT___TRY))
	{
		auto tryStat = ParseStat(pa, cursor);
		if (TestToken(cursor, CppTokens::STAT___FINALLY))
		{
			auto stat = MakePtr<__Try__FinallyStat>();
			stat->tryStat = tryStat;
			stat->finallyStat = ParseStat(pa, cursor);
			return stat;
		}
		else if (TestToken(cursor, CppTokens::STAT___EXCEPT))
		{
			auto stat = MakePtr<__Try__ExceptStat>();
			stat->tryStat = tryStat;
			RequireToken(cursor, CppTokens::LPARENTHESIS);
			stat->expr = ParseExpr(pa, true, cursor);
			RequireToken(cursor, CppTokens::RPARENTHESIS);
			stat->exceptStat = ParseStat(pa, cursor);
			return stat;
		}
		else
		{
			throw StopParsingException(cursor);
		}
	}
	else if (TestToken(cursor, CppTokens::STAT___LEAVE))
	{
		RequireToken(cursor, CppTokens::SEMICOLON);
		return MakePtr<__LeaveStat>();
	}
	else if (TestToken(cursor, CppTokens::STAT___IF_EXISTS))
	{
		auto stat = MakePtr<__IfExistsStat>();
		RequireToken(cursor, CppTokens::LPARENTHESIS);
		stat->expr = ParseExpr(pa, true, cursor);
		RequireToken(cursor, CppTokens::RPARENTHESIS);
		stat->stat = ParseStat(pa, cursor);
		return stat;
	}
	else if (TestToken(cursor, CppTokens::STAT___IF_NOT_EXISTS))
	{
		auto stat = MakePtr<__IfNotExistsStat>();
		RequireToken(cursor, CppTokens::LPARENTHESIS);
		stat->expr = ParseExpr(pa, true, cursor);
		RequireToken(cursor, CppTokens::RPARENTHESIS);
		stat->stat = ParseStat(pa, cursor);
		return stat;
	}
	else
	{
		{
			auto oldCursor = cursor;
			bool isLabel = TestToken(cursor, CppTokens::ID) && TestToken(cursor, CppTokens::COLON);
			cursor = oldCursor;
			if (isLabel)
			{
				auto stat = MakePtr<LabelStat>();
				if (!ParseCppName(stat->name, cursor))
				{
					throw StopParsingException(cursor);
				}
				RequireToken(cursor, CppTokens::COLON);
				stat->stat = ParseStat(pa, cursor);
				return stat;
			}
		}
		{
			auto oldCursor = cursor;
			try
			{
				auto expr = ParseExpr(pa, true, cursor);
				RequireToken(cursor, CppTokens::SEMICOLON);
				auto stat = MakePtr<ExprStat>();
				stat->expr = expr;
				return stat;
			}
			catch (const StopParsingException&)
			{
				cursor = oldCursor;
			}
		}
		{
			auto stat = MakePtr<DeclStat>();
			ParseDeclaration(pa, cursor, stat->decls);
			return stat;
		}
	}
}