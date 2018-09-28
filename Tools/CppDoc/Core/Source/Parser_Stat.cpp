#include "Parser.h"
#include "Ast_Stat.h"

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
		}
		else if (TestToken(cursor, CppTokens::STAT___EXCEPT))
		{
			auto stat = MakePtr<__Try__ExceptStat>();
			stat->tryStat = tryStat;
			RequireToken(cursor, CppTokens::LPARENTHESIS);
			stat->expr = ParseExpr(pa, true, cursor);
			RequireToken(cursor, CppTokens::RPARENTHESIS);
			stat->exceptStat = ParseStat(pa, cursor);
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
		throw StopParsingException(cursor);
	}
}