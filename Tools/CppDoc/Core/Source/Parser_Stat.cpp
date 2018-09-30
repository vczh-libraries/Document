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
			ParsingArguments newPa(pa, pa.context->CreateStatSymbol(stat));
			stat->stats.Add(ParseStat(newPa, cursor));
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
				ParseDeclarator(pa, nullptr, DeclaratorRestriction::One, InitializerRestriction::Zero, cursor, declarators);
				RequireToken(cursor, CppTokens::COLON);
			}
			catch (const StopParsingException&)
			{
				cursor = oldCursor;
				goto FOR_EACH_FAILED;
			}

			auto stat = MakePtr<ForEachStat>();
			ParsingArguments newPa(pa, pa.context->CreateStatSymbol(stat));
			{
				List<Ptr<VariableDeclaration>> varDecls;
				BuildVariablesAndSymbols(newPa, declarators, varDecls, true);
				stat->varDecl = varDecls[0];
			}

			stat->expr = ParseExpr(newPa, true, cursor);
			RequireToken(cursor, CppTokens::RPARENTHESIS);
			stat->stat = ParseStat(newPa, cursor);
			return stat;
		}
	FOR_EACH_FAILED:
		{
			auto stat = MakePtr<ForStat>();
			ParsingArguments newPa(pa, pa.context->CreateStatSymbol(stat));
			if (!TestToken(cursor, CppTokens::SEMICOLON))
			{
				auto oldCursor = cursor;
				List<Ptr<Declarator>> declarators;
				try
				{
					ParseDeclarator(newPa, nullptr, DeclaratorRestriction::Many, InitializerRestriction::Optional, cursor, declarators);
				}
				catch (const StopParsingException&)
				{
					cursor = oldCursor;
				}

				if (declarators.Count() > 0)
				{
					BuildVariablesAndSymbols(newPa, declarators, stat->varDecls, true);
				}
				else
				{
					stat->init = ParseExpr(newPa, true, cursor);
				}
				RequireToken(cursor, CppTokens::SEMICOLON);
			}
			if (!TestToken(cursor, CppTokens::SEMICOLON))
			{
				stat->expr = ParseExpr(newPa, true, cursor);
				RequireToken(cursor, CppTokens::SEMICOLON);
			}
			if (!TestToken(cursor, CppTokens::RPARENTHESIS))
			{
				stat->effect = ParseExpr(newPa, true, cursor);
				RequireToken(cursor, CppTokens::RPARENTHESIS);
			}
			stat->stat = ParseStat(newPa, cursor);
			return stat;
		}
	}
	else if (TestToken(cursor, CppTokens::STAT_IF))
	{
		auto stat = MakePtr<IfElseStat>();
		ParsingArguments newPa(pa, pa.context->CreateStatSymbol(stat));
		RequireToken(cursor, CppTokens::LPARENTHESIS);
		{
			auto oldCursor = cursor;
			List<Ptr<Declarator>> declarators;
			try
			{
				ParseDeclarator(newPa, nullptr, DeclaratorRestriction::Many, InitializerRestriction::Optional, cursor, declarators);
				RequireToken(cursor, CppTokens::SEMICOLON);
				BuildVariablesAndSymbols(newPa, declarators, stat->varDecls, true);
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
				ParseDeclarator(newPa, nullptr, DeclaratorRestriction::One, InitializerRestriction::Optional, cursor, declarators);
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
				List<Ptr<VariableDeclaration>> varDecls;
				BuildVariablesAndSymbols(newPa, declarators, varDecls, true);
				stat->varExpr = varDecls[0];
			}
			else
			{
				stat->expr = ParseExpr(newPa, true, cursor);
			}
		}
		RequireToken(cursor, CppTokens::RPARENTHESIS);
		stat->trueStat = ParseStat(newPa, cursor);

		if (TestToken(cursor, CppTokens::STAT_ELSE))
		{
			stat->falseStat = ParseStat(newPa, cursor);
		}
		return stat;
	}
	else if (TestToken(cursor, CppTokens::STAT_SWITCH))
	{
		auto stat = MakePtr<SwitchStat>();
		ParsingArguments newPa(pa, pa.context->CreateStatSymbol(stat));
		RequireToken(cursor, CppTokens::LPARENTHESIS);
		{
			auto oldCursor = cursor;
			List<Ptr<Declarator>> declarators;
			try
			{
				ParseDeclarator(newPa, nullptr, DeclaratorRestriction::One, InitializerRestriction::Optional, cursor, declarators);
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
				List<Ptr<VariableDeclaration>> varDecls;
				BuildVariablesAndSymbols(newPa, declarators, varDecls, true);
				stat->varExpr = varDecls[0];
			}
			else
			{
				stat->expr = ParseExpr(newPa, true, cursor);
			}
		}
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
			ParseDeclarator(pa, nullptr, DeclaratorRestriction::Optional, InitializerRestriction::Zero, cursor, declarators);
			List<Ptr<VariableDeclaration>> varDecls;
			BuildVariablesAndSymbols(pa, declarators, varDecls, true);
			stat->exception = varDecls[0];
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