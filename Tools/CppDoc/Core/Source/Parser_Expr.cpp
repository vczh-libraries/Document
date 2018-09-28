#include "Parser.h"
#include "Ast_Expr.h"

Ptr<Expr> ParseExpr(const ParsingArguments& pa, bool allowComma, Ptr<CppTokenCursor>& cursor)
{
	if (cursor)
	{
		switch ((CppTokens)cursor->token.token)
		{
		case CppTokens::INT:
		case CppTokens::HEX:
		case CppTokens::BIN:
		case CppTokens::FLOAT:
		case CppTokens::CHAR:
			{
				auto literal = MakePtr<LiteralExpr>();
				literal->tokens.Add(cursor->token);
				SkipToken(cursor);
				return literal;
			}
		case CppTokens::STRING:
			{
				auto literal = MakePtr<LiteralExpr>();
				while (cursor && (CppTokens)cursor->token.token == CppTokens::STRING)
				{
					literal->tokens.Add(cursor->token);
					SkipToken(cursor);
				}
				return literal;
			}
		}
	}
	throw StopParsingException(cursor);
}