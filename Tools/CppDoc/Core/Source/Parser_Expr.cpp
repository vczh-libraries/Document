#include "Parser.h"
#include "Ast_Expr.h"
#include "Ast_Type.h"

/***********************************************************************
ParseIdExpr
***********************************************************************/

Ptr<IdExpr> ParseIdExpr(const ParsingArguments& pa, Ptr<CppTokenCursor>& cursor)
{
	CppName cppName;
	if (ParseCppName(cppName, cursor))
	{
		auto rsr = ResolveSymbol(pa, cppName, SearchPolicy::SymbolAccessableInScope);
		if (!rsr.types)
		{
			auto type = MakePtr<IdExpr>();
			type->name = cppName;
			type->resolving = rsr.values;
			if (pa.recorder)
			{
				pa.recorder->Index(type->name, type->resolving);
			}
			return type;
		}
	}
	throw StopParsingException(cursor);
}

/***********************************************************************
TryParseChildExpr
***********************************************************************/

Ptr<ChildExpr> TryParseChildExpr(const ParsingArguments& pa, Ptr<Type> classType, Ptr<CppTokenCursor>& cursor)
{
	CppName cppName;
	if (ParseCppName(cppName, cursor))
	{
		auto rsr = ResolveChildSymbol(pa, classType, cppName);
		if (!rsr.types)
		{
			auto type = MakePtr<ChildExpr>();
			type->classType = classType;
			type->name = cppName;
			type->resolving = rsr.values;
			if (pa.recorder && type->resolving)
			{
				pa.recorder->Index(type->name, type->resolving);
			}
			return type;
		}
		else
		{
			throw StopParsingException(cursor);
		}
	}
	return nullptr;
}

/***********************************************************************
ParsePrimitiveExpr
***********************************************************************/

Ptr<Expr> ParsePrimitiveExpr(const ParsingArguments& pa, Ptr<CppTokenCursor>& cursor)
{
	if (cursor)
	{
		switch ((CppTokens)cursor->token.token)
		{
		case CppTokens::EXPR_TRUE:
		case CppTokens::EXPR_FALSE:
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
		case CppTokens::EXPR_THIS:
			{
				SkipToken(cursor);
				return MakePtr<ThisExpr>();
			}
		case CppTokens::EXPR_NULLPTR:
			{
				SkipToken(cursor);
				return MakePtr<NullptrExpr>();
			}
		case CppTokens::LPARENTHESIS:
			{
				SkipToken(cursor);
				auto expr = MakePtr<ParenthesisExpr>();
				expr->expr = ParseExpr(pa, true, cursor);
				RequireToken(cursor, CppTokens::RPARENTHESIS);
				return expr;
			}
		case CppTokens::EXPR_DYNAMIC_CAST:
		case CppTokens::EXPR_STATIC_CAST:
		case CppTokens::EXPR_CONST_CAST:
		case CppTokens::EXPR_REINTERPRET_CAST:
		case CppTokens::EXPR_SAFE_CAST:
			{
				auto expr = MakePtr<CastExpr>();
				expr->castType = CppCastType::SafeCast;
				switch ((CppTokens)cursor->token.token)
				{
				case CppTokens::EXPR_DYNAMIC_CAST:		expr->castType = CppCastType::DynamicCast;		 break;
				case CppTokens::EXPR_STATIC_CAST:		expr->castType = CppCastType::StaticCast;		 break;
				case CppTokens::EXPR_CONST_CAST:		expr->castType = CppCastType::ConstCast;		 break;
				case CppTokens::EXPR_REINTERPRET_CAST:	expr->castType = CppCastType::ReinterpretCast;	 break;
				}
				SkipToken(cursor);

				RequireToken(cursor, CppTokens::LT);
				expr->type = ParseType(pa, cursor);
				RequireToken(cursor, CppTokens::GT);
				RequireToken(cursor, CppTokens::LPARENTHESIS);
				expr->expr = ParseExpr(pa, true, cursor);
				RequireToken(cursor, CppTokens::RPARENTHESIS);
				return expr;
			}
		case CppTokens::EXPR_TYPEID:
			{
				SkipToken(cursor);
				auto expr = MakePtr<TypeidExpr>();
				RequireToken(cursor, CppTokens::LPARENTHESIS);
				{
					auto oldCursor = cursor;
					try
					{
						expr->expr = ParseExpr(pa, true, cursor);
						goto SUCCESS_EXPR;
					}
					catch (const StopParsingException&)
					{
						cursor = oldCursor;
					}
				}
				expr->type = ParseType(pa, cursor);
			SUCCESS_EXPR:
				RequireToken(cursor, CppTokens::RPARENTHESIS);
				return expr;
			}
			break;
		}

		{
			auto oldCursor = cursor;
			try
			{
				auto type = ParseLongType(pa, cursor);

				if (TestToken(cursor, CppTokens::COLON, CppTokens::COLON))
				{
					if (auto expr = TryParseChildExpr(pa, type, cursor))
					{
						return expr;
					}
				}

				if (TestToken(cursor, CppTokens::LPARENTHESIS))
				{
					auto expr = MakePtr<FuncAccessExpr>();
					expr->type = type;
					while (!TestToken(cursor, CppTokens::RPARENTHESIS))
					{
						expr->arguments.Add(ParseExpr(pa, false, cursor));
						if (TestToken(cursor, CppTokens::RPARENTHESIS))
						{
							break;
						}
						else
						{
							RequireToken(cursor, CppTokens::COMMA);
						}
					}
					return expr;
				}

				throw StopParsingException(cursor);
			}
			catch (const StopParsingException&)
			{
				cursor = oldCursor;
				goto GIVE_UP_CHILD_SYMBOL;
			}
		}
	GIVE_UP_CHILD_SYMBOL:

		if (TestToken(cursor, CppTokens::COLON, CppTokens::COLON))
		{
			if (auto expr = TryParseChildExpr(pa, MakePtr<RootType>(), cursor))
			{
				return expr;
			}
		}
		else
		{
			if (auto expr = ParseIdExpr(pa, cursor))
			{
				return expr;
			}
		}
	}
	throw StopParsingException(cursor);
}

/***********************************************************************
ParsePostUnaryExpr
***********************************************************************/

Ptr<Expr> ParsePostUnaryExpr(const ParsingArguments& pa, Ptr<CppTokenCursor>& cursor)
{
	auto expr = ParsePrimitiveExpr(pa, cursor);
	while (true)
	{
		if (!TestToken(cursor, CppTokens::DOT, CppTokens::MUL, false) && TestToken(cursor, CppTokens::DOT))
		{
			auto newExpr = MakePtr<FieldAccessExpr>();
			newExpr->type = CppFieldAccessType::Dot;
			newExpr->expr = expr;
			if (!ParseCppName(newExpr->name, cursor, false) && !ParseCppName(newExpr->name, cursor, true))
			{
				throw StopParsingException(cursor);
			}
			expr = newExpr;
		}
		else if (!TestToken(cursor, CppTokens::SUB, CppTokens::GT, CppTokens::MUL, false) && TestToken(cursor, CppTokens::SUB, CppTokens::GT))
		{
			auto newExpr = MakePtr<FieldAccessExpr>();
			newExpr->type = CppFieldAccessType::Arrow;
			newExpr->expr = expr;
			if (!ParseCppName(newExpr->name, cursor, false) && !ParseCppName(newExpr->name, cursor, true))
			{
				throw StopParsingException(cursor);
			}
			expr = newExpr;
		}
		else if (TestToken(cursor, CppTokens::LBRACKET))
		{
			auto newExpr = MakePtr<ArrayAccessExpr>();
			newExpr->expr = expr;
			newExpr->index = ParseExpr(pa, true, cursor);
			RequireToken(cursor, CppTokens::RBRACKET);
			expr = newExpr;
		}
		else if (TestToken(cursor, CppTokens::LPARENTHESIS))
		{
			auto newExpr = MakePtr<FuncAccessExpr>();
			newExpr->expr = expr;
			while (!TestToken(cursor, CppTokens::RPARENTHESIS))
			{
				newExpr->arguments.Add(ParseExpr(pa, false, cursor));
				if (TestToken(cursor, CppTokens::RPARENTHESIS))
				{
					break;
				}
				else
				{
					RequireToken(cursor, CppTokens::COMMA);
				}
			}
			expr = newExpr;
		}
		else
		{
			break;
		}
	}
	return expr;
}

/***********************************************************************
ParseExpr
***********************************************************************/

Ptr<Expr> ParseExpr(const ParsingArguments& pa, bool allowComma, Ptr<CppTokenCursor>& cursor)
{
	return ParsePostUnaryExpr(pa, cursor);
}