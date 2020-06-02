#include "Parser.h"
#include "Parser_Declarator.h"
#include "Ast_Stat.h"
#include "Ast_Decl.h"

template<typename T>
void ParseVariableOrExpression(const ParsingArguments& pa, Ptr<CppTokenCursor>& cursor, Ptr<T> stat)
{
	auto oldCursor = cursor;
	Ptr<Declarator> declarator;
	try
	{
		declarator = ParseNonMemberDeclarator(pa, pda_VarInit(), cursor);
		if (!declarator->initializer)
		{
			throw StopParsingException(cursor);
		}
	}
	catch (const StopParsingException&)
	{
		declarator = nullptr;
		cursor = oldCursor;
	}

	if (declarator)
	{
		stat->varExpr = BuildVariableAndSymbol(pa, declarator, cursor);
	}
	else
	{
		stat->expr = ParseExpr(pa, pea_Full(), cursor);
	}
}

Ptr<Stat> ParseStat(const ParsingArguments& pa, Ptr<CppTokenCursor>& cursor)
{
	if (TestToken(cursor, CppTokens::STAT_ASM))
	{
		// __asm { ... }
		// skip assembly code
		RequireToken(cursor, CppTokens::LBRACE);
		while (!TestToken(cursor, CppTokens::RBRACE)) SkipToken(cursor);
		return MakePtr<EmptyStat>();
	}
	else if (TestToken(cursor, CppTokens::SEMICOLON))
	{
		// ;
		return MakePtr<EmptyStat>();
	}
	else if (TestToken(cursor, CppTokens::LBRACE))
	{
		// { { STATEMENT ...} }
		auto stat = MakePtr<BlockStat>();
		auto newPa = pa.WithScope(pa.scopeSymbol->CreateStatSymbol_NFb(stat));
		while (!TestToken(cursor, CppTokens::RBRACE))
		{
			stat->stats.Add(ParseStat(newPa, cursor));
		}
		return stat;
	}
	else if (TestToken(cursor, CppTokens::STAT_DEFAULT))
	{
		// default: STATEMENT
		RequireToken(cursor, CppTokens::COLON);
		auto stat = MakePtr<DefaultStat>();
		stat->stat = ParseStat(pa, cursor);
		return stat;
	}
	else if (TestToken(cursor, CppTokens::STAT_CASE))
	{
		// case EXPRESSION: STATEMENT
		auto stat = MakePtr<CaseStat>();
		stat->expr = ParseExpr(pa, pea_Argument(), cursor);
		RequireToken(cursor, CppTokens::COLON);
		stat->stat = ParseStat(pa, cursor);
		return stat;
	}
	else if (TestToken(cursor, CppTokens::STAT_GOTO))
	{
		// goto IDENTIFIER;
		auto stat = MakePtr<GotoStat>();
		if (TestToken(cursor, CppTokens::ID, false) ||
			TestToken(cursor, CppTokens::INT, false) ||
			TestToken(cursor, CppTokens::HEX, false) ||
			TestToken(cursor, CppTokens::BIN, false))
		{
			stat->name.name = WString(cursor->token.reading, cursor->token.length);
			stat->name.tokenCount = 1;
			stat->name.nameTokens[0] = cursor->token;
			stat->name.type = CppNameType::Normal;
			SkipToken(cursor);
		}
		RequireToken(cursor, CppTokens::SEMICOLON);
		return stat;
	}
	else if (TestToken(cursor, CppTokens::STAT_BREAK))
	{
		// break;
		RequireToken(cursor, CppTokens::SEMICOLON);
		return MakePtr<BreakStat>();
	}
	else if (TestToken(cursor, CppTokens::STAT_CONTINUE))
	{
		// continue;
		RequireToken(cursor, CppTokens::SEMICOLON);
		return MakePtr<ContinueStat>();
	}
	else if (TestToken(cursor, CppTokens::STAT_WHILE))
	{
		// while (EXPRESSION) STATEMENT
		// while (VARIABLE-DECLARATION-WITH-INITIALIZER) STATEMENT
		auto stat = MakePtr<WhileStat>();
		auto newPa = pa.WithScope(pa.scopeSymbol->CreateStatSymbol_NFb(stat));
		RequireToken(cursor, CppTokens::LPARENTHESIS);
		ParseVariableOrExpression(newPa, cursor, stat);
		RequireToken(cursor, CppTokens::RPARENTHESIS);
		stat->stat = ParseStat(newPa, cursor);
		return stat;
	}
	else if (TestToken(cursor, CppTokens::STAT_DO))
	{
		// do STATEMENT while (EXPRESSION);
		auto stat = MakePtr<DoWhileStat>();
		stat->stat = ParseStat(pa, cursor);
		RequireToken(cursor, CppTokens::STAT_WHILE);
		RequireToken(cursor, CppTokens::LPARENTHESIS);
		stat->expr = ParseExpr(pa, pea_Full(), cursor);
		RequireToken(cursor, CppTokens::RPARENTHESIS);
		RequireToken(cursor, CppTokens::SEMICOLON);
		return stat;
	}
	else if (TestToken(cursor, CppTokens::STAT_FOR))
	{
		RequireToken(cursor, CppTokens::LPARENTHESIS);

		{
			// for (VARIABLE-DECLARATION : EXPRESSION) STATEMENT
			Ptr<Declarator> declarator;
			{
				auto oldCursor = cursor;
				try
				{
					declarator = ParseNonMemberDeclarator(pa, pda_VarNoInit(), cursor);
					RequireToken(cursor, CppTokens::COLON);
				}
				catch (const StopParsingException&)
				{
					declarator = nullptr;
					cursor = oldCursor;
					goto FOR_EACH_FAILED;
				}
			}

			auto stat = MakePtr<ForEachStat>();
			auto newPa = pa.WithScope(pa.scopeSymbol->CreateStatSymbol_NFb(stat));
			stat->varDecl = BuildVariableAndSymbol(newPa, declarator, cursor);
			stat->expr = ParseExpr(newPa, pea_Full(), cursor);

			if (stat->varDecl->needResolveTypeFromInitializer)
			{
				// infer the variable first, so that the type is clear when parsing any IdExpr that use this variable
				stat->stat = MakePtr<EmptyStat>();
				EvaluateStat(pa, stat, false, nullptr);
			}

			RequireToken(cursor, CppTokens::RPARENTHESIS);
			stat->stat = ParseStat(newPa, cursor);
			return stat;
		}
	FOR_EACH_FAILED:
		{
			// for ([VARIABLE-DECLARATION-S]; [EXPRESSION]; [EXPRESSION]) STATEMENT
			// for ([EXPRESSION]; [EXPRESSION]; [EXPRESSION]) STATEMENT
			auto stat = MakePtr<ForStat>();
			auto newPa = pa.WithScope(pa.scopeSymbol->CreateStatSymbol_NFb(stat));
			if (!TestToken(cursor, CppTokens::SEMICOLON))
			{
				List<Ptr<Declarator>> declarators;
				{
					auto oldCursor = cursor;
					try
					{
						ParseNonMemberDeclarator(newPa, pda_Decls(false, false), cursor, declarators);
					}
					catch (const StopParsingException&)
					{
						declarators.Clear();
						cursor = oldCursor;
					}
				}

				if (declarators.Count() > 0)
				{
					BuildVariablesAndSymbols(newPa, declarators, stat->varDecls, cursor);
				}
				else
				{
					stat->init = ParseExpr(newPa, pea_Full(), cursor);
				}
				RequireToken(cursor, CppTokens::SEMICOLON);
			}
			if (!TestToken(cursor, CppTokens::SEMICOLON))
			{
				stat->expr = ParseExpr(newPa, pea_Full(), cursor);
				RequireToken(cursor, CppTokens::SEMICOLON);
			}
			if (!TestToken(cursor, CppTokens::RPARENTHESIS))
			{
				stat->effect = ParseExpr(newPa, pea_Full(), cursor);
				RequireToken(cursor, CppTokens::RPARENTHESIS);
			}
			stat->stat = ParseStat(newPa, cursor);
			return stat;
		}
	}
	else if (TestToken(cursor, CppTokens::STAT_IF))
	{
		// if ([VARIABLE-DECLARATION-S;] EXPRESSION) STATEMENT [else STATEMENT]
		// if ([VARIABLE-DECLARATION-S;] VARIABLE-DECLARATION-WITH-INITIALIZER) STATEMENT [else STATEMENT]
		auto stat = MakePtr<IfElseStat>();
		auto newPa = pa.WithScope(pa.scopeSymbol->CreateStatSymbol_NFb(stat));
		RequireToken(cursor, CppTokens::LPARENTHESIS);
		{
			auto oldCursor = cursor;
			List<Ptr<Declarator>> declarators;
			try
			{
				ParseNonMemberDeclarator(newPa, pda_Decls(false, false), cursor, declarators);
				RequireToken(cursor, CppTokens::SEMICOLON);
				BuildVariablesAndSymbols(newPa, declarators, stat->varDecls, cursor);
			}
			catch (const StopParsingException&)
			{
				declarators.Clear();
				cursor = oldCursor;
			}
		}
		ParseVariableOrExpression(newPa, cursor, stat);
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
		// switch (EXPRESSION) STATEMENT
		// switch (VARIABLE-DECLARATION-WITH-INITIALIZER) STATEMENT
		auto stat = MakePtr<SwitchStat>();
		auto newPa = pa.WithScope(pa.scopeSymbol->CreateStatSymbol_NFb(stat));
		RequireToken(cursor, CppTokens::LPARENTHESIS);
		ParseVariableOrExpression(newPa, cursor, stat);
		RequireToken(cursor, CppTokens::RPARENTHESIS);
		stat->stat = ParseStat(newPa, cursor);
		return stat;
	}
	else if (TestToken(cursor, CppTokens::STAT_TRY))
	{
		// try STATEMENT catch(...) STATEMENT
		// try STATEMENT catch(VARIABLE-DECLARATION) STATEMENT
		auto stat = MakePtr<TryCatchStat>();
		stat->tryStat = ParseStat(pa, cursor);

		while (TestToken(cursor, CppTokens::STAT_CATCH))
		{
			TryCatchStat::CatchStat catchStat;
			RequireToken(cursor, CppTokens::LPARENTHESIS);

			catchStat.scopeStat = MakePtr<EmptyStat>();
			auto newPa = pa.WithScope(pa.scopeSymbol->CreateStatSymbol_NFb(catchStat.scopeStat));

			if (!TestToken(cursor, CppTokens::DOT, CppTokens::DOT, CppTokens::DOT))
			{
				auto declarator = ParseNonMemberDeclarator(newPa, pda_VarType(), cursor);
				catchStat.exception = BuildVariableAndSymbol(newPa, declarator, cursor);
			}
			RequireToken(cursor, CppTokens::RPARENTHESIS);
			catchStat.catchStat = ParseStat(newPa, cursor);
			stat->catchStats.Add(catchStat);
		}
		return stat;
	}
	else if (TestToken(cursor, CppTokens::STAT_RETURN))
	{
		// return [EXPRESSION];
		auto stat = MakePtr<ReturnStat>();
		if (!TestToken(cursor, CppTokens::SEMICOLON))
		{
			stat->expr = ParseExpr(pa, pea_Full(), cursor);
			RequireToken(cursor, CppTokens::SEMICOLON);
		}
		return stat;
	}
	else if (TestToken(cursor, CppTokens::STAT___TRY))
	{
		// __try STATEMENT __except(EXPRESSION) STATEMENT
		// __try STATEMENT __finally STATEMENT
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
			stat->expr = ParseExpr(pa, pea_Full(), cursor);
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
		// __leave;
		RequireToken(cursor, CppTokens::SEMICOLON);
		return MakePtr<__LeaveStat>();
	}
	else if (TestToken(cursor, CppTokens::STAT___IF_EXISTS))
	{
		// __if_exists (EXPRESSION) STATEMENT
		auto stat = MakePtr<__IfExistsStat>();
		RequireToken(cursor, CppTokens::LPARENTHESIS);
		stat->expr = ParseExpr(pa, pea_Full(), cursor);
		RequireToken(cursor, CppTokens::RPARENTHESIS);
		stat->stat = ParseStat(pa, cursor);
		return stat;
	}
	else if (TestToken(cursor, CppTokens::STAT___IF_NOT_EXISTS))
	{
		// __if_not_exists (EXPRESSION) STATEMENT
		auto stat = MakePtr<__IfNotExistsStat>();
		RequireToken(cursor, CppTokens::LPARENTHESIS);
		stat->expr = ParseExpr(pa, pea_Full(), cursor);
		RequireToken(cursor, CppTokens::RPARENTHESIS);
		stat->stat = ParseStat(pa, cursor);
		return stat;
	}
	else
	{
		{
			// IDENTIFIER: STATEMENT
			bool isLabel = false;
			{
				auto oldCursor = cursor;
				isLabel = (
					TestToken(cursor, CppTokens::ID) ||
					TestToken(cursor, CppTokens::INT) ||
					TestToken(cursor, CppTokens::HEX) ||
					TestToken(cursor, CppTokens::BIN)
					) &&
					!TestToken(cursor, CppTokens::COLON, CppTokens::COLON, false) &&
					TestToken(cursor, CppTokens::COLON);
				cursor = oldCursor;
			}

			if (isLabel)
			{
				auto stat = MakePtr<LabelStat>();
				stat->name.name = WString(cursor->token.reading, cursor->token.length);
				stat->name.tokenCount = 1;
				stat->name.nameTokens[0] = cursor->token;
				stat->name.type = CppNameType::Normal;
				SkipToken(cursor);
				RequireToken(cursor, CppTokens::COLON);
				stat->stat = ParseStat(pa, cursor);
				return stat;
			}
		}
		{
			// EXPRESSION;
			auto oldCursor = cursor;
			try
			{
				auto expr = ParseExpr(pa, pea_Full(), cursor);
				RequireToken(cursor, CppTokens::SEMICOLON);
				auto stat = MakePtr<ExprStat>();
				stat->expr = expr;

				// the expression could be too long and make the destructor stackoverflow
				while (oldCursor != cursor)
				{
					oldCursor = oldCursor->Next();
				}
				return stat;
			}
			catch (const StopParsingException&)
			{
				cursor = oldCursor;
			}
		}
		{
			// DECLARATION
			auto stat = MakePtr<DeclStat>();
			ParseDeclaration(pa, cursor, stat->decls);
			return stat;
		}
	}
}