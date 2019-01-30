#include "Parser.h"
#include "Ast_Expr.h"
#include "Ast_Type.h"

/***********************************************************************
FillOperatorAndSkip
***********************************************************************/

void FillOperatorAndSkip(CppName& name, Ptr<CppTokenCursor>& cursor, vint count)
{
	auto reading = cursor->token.reading;
	vint length = 0;

	name.type = CppNameType::Normal;
	name.tokenCount = count;
	for (vint i = 0; i < count; i++)
	{
		name.nameTokens[i] = cursor->token;
		length += cursor->token.length;
		SkipToken(cursor);
	}

	name.name = WString(reading, length);
}

void FillOperator(CppName& name, CppPostfixUnaryOp& op)
{
	if (name.name == L"++")		{	op = CppPostfixUnaryOp::Increase;		return;	}
	if (name.name == L"--")		{	op = CppPostfixUnaryOp::Decrease;		return;	}
	throw L"Invalid!";
}

void FillOperator(CppName& name, CppPrefixUnaryOp& op)
{
	if (name.name == L"++")		{	op = CppPrefixUnaryOp::Increase;		return;	}
	if (name.name == L"--")		{	op = CppPrefixUnaryOp::Decrease;		return;	}
	if (name.name == L"~")		{	op = CppPrefixUnaryOp::Revert;			return;	}
	if (name.name == L"!")		{	op = CppPrefixUnaryOp::Not;				return;	}
	if (name.name == L"-")		{	op = CppPrefixUnaryOp::Negative;		return;	}
	if (name.name == L"+")		{	op = CppPrefixUnaryOp::Positive;		return;	}
	if (name.name == L"&")		{	op = CppPrefixUnaryOp::AddressOf;		return;	}
	if (name.name == L"*")		{	op = CppPrefixUnaryOp::Dereference;		return;	}
	throw L"Invalid!";
}

void FillOperator(CppName& name, CppBinaryOp& op)
{
	if (name.name == L".*")		{	op = CppBinaryOp::ValueFieldDeref;		return;	}
	if (name.name == L"->*")	{	op = CppBinaryOp::PtrFieldDeref;		return;	}
	if (name.name == L"*")		{	op = CppBinaryOp::Mul;					return;	}
	if (name.name == L"/")		{	op = CppBinaryOp::Div;					return;	}
	if (name.name == L"%")		{	op = CppBinaryOp::Mod;					return;	}
	if (name.name == L"+")		{	op = CppBinaryOp::Add;					return;	}
	if (name.name == L"-")		{	op = CppBinaryOp::Sub;					return;	}
	if (name.name == L"<<")		{	op = CppBinaryOp::Shl;					return;	}
	if (name.name == L">>")		{	op = CppBinaryOp::Shr;					return;	}
	if (name.name == L"<")		{	op = CppBinaryOp::LT;					return;	}
	if (name.name == L">")		{	op = CppBinaryOp::GT;					return;	}
	if (name.name == L"<=")		{	op = CppBinaryOp::LE;					return;	}
	if (name.name == L">=")		{	op = CppBinaryOp::GE;					return;	}
	if (name.name == L"==")		{	op = CppBinaryOp::EQ;					return;	}
	if (name.name == L"!=")		{	op = CppBinaryOp::NE;					return;	}
	if (name.name == L"&")		{	op = CppBinaryOp::BitAnd;				return;	}
	if (name.name == L"|")		{	op = CppBinaryOp::BitOr;				return;	}
	if (name.name == L"&&")		{	op = CppBinaryOp::And;					return;	}
	if (name.name == L"||")		{	op = CppBinaryOp::Or;					return;	}
	if (name.name == L"^")		{	op = CppBinaryOp::Xor;					return;	}
	if (name.name == L"=")		{	op = CppBinaryOp::Assign;				return;	}
	if (name.name == L"*=")		{	op = CppBinaryOp::MulAssign;			return;	}
	if (name.name == L"/=")		{	op = CppBinaryOp::DivAssign;			return;	}
	if (name.name == L"%=")		{	op = CppBinaryOp::ModAssign;			return;	}
	if (name.name == L"+=")		{	op = CppBinaryOp::AddAssign;			return;	}
	if (name.name == L"-=")		{	op = CppBinaryOp::SubAddisn;			return;	}
	if (name.name == L"<<=")	{	op = CppBinaryOp::ShlAssign;			return;	}
	if (name.name == L">>=")	{	op = CppBinaryOp::ShrAssign;			return;	}
	if (name.name == L"&=")		{	op = CppBinaryOp::AndAssign;			return;	}
	if (name.name == L"|=")		{	op = CppBinaryOp::OrAssign;				return;	}
	if (name.name == L"^=")		{	op = CppBinaryOp::XorAssign;			return;	}
	if (name.name == L",")		{	op = CppBinaryOp::Comma;				return;	}
	throw L"Invalid!";
}

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

				if (TestToken(cursor, CppTokens::LPARENTHESIS, false) || TestToken(cursor, CppTokens::LBRACE, false))
				{
					auto closeToken = (CppTokens)cursor->token.token == CppTokens::LPARENTHESIS ? CppTokens::RPARENTHESIS : CppTokens::RBRACE;
					SkipToken(cursor);

					auto expr = MakePtr<CtorAccessExpr>();
					expr->type = type;
					expr->initializer = MakePtr<Initializer>();
					expr->initializer->initializerType = closeToken == CppTokens::RPARENTHESIS ? InitializerType::Constructor : InitializerType::Universal;

					if (!TestToken(cursor, closeToken))
					{
						while (true)
						{
							expr->initializer->arguments.Add(ParseExpr(pa, false, cursor));
							if (TestToken(cursor, closeToken))
							{
								break;
							}
							else
							{
								RequireToken(cursor, CppTokens::COMMA);
							}
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
ParsePostfixUnaryExpr
***********************************************************************/

Ptr<Expr> ParsePostfixUnaryExpr(const ParsingArguments& pa, Ptr<CppTokenCursor>& cursor)
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
			if (!TestToken(cursor, CppTokens::RPARENTHESIS))
			{
				while (true)
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
			}
			expr = newExpr;
		}
		else if (TestToken(cursor, CppTokens::ADD, CppTokens::ADD, false) || TestToken(cursor, CppTokens::SUB, CppTokens::SUB, false))
		{
			auto newExpr = MakePtr<PostfixUnaryExpr>();
			FillOperatorAndSkip(newExpr->opName, cursor, 2);
			FillOperator(newExpr->opName, newExpr->op);
			newExpr->operand = expr;
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
ParsePrefixUnaryExpr
***********************************************************************/

Ptr<Expr> ParsePrefixUnaryExpr(const ParsingArguments& pa, Ptr<CppTokenCursor>& cursor)
{
	if (TestToken(cursor, CppTokens::EXPR_SIZEOF))
	{
		auto newExpr = MakePtr<SizeofExpr>();
		auto oldCursor = cursor;
		try
		{
			RequireToken(cursor, CppTokens::LPARENTHESIS);
			newExpr->type = ParseType(pa, cursor);
			RequireToken(cursor, CppTokens::RPARENTHESIS);
			return newExpr;
		}
		catch (const StopParsingException&)
		{
			cursor = oldCursor;
		}
		newExpr->expr = ParsePrefixUnaryExpr(pa, cursor);
		return newExpr;
	}
	else if (TestToken(cursor, CppTokens::ADD, CppTokens::ADD, false) || TestToken(cursor, CppTokens::SUB, CppTokens::SUB, false))
	{
		auto newExpr = MakePtr<PrefixUnaryExpr>();
		FillOperatorAndSkip(newExpr->opName, cursor, 2);
		FillOperator(newExpr->opName, newExpr->op);
		newExpr->operand = ParsePrefixUnaryExpr(pa, cursor);
		return newExpr;
	}
	else if (
		TestToken(cursor, CppTokens::REVERT, false) ||
		TestToken(cursor, CppTokens::NOT, false) ||
		TestToken(cursor, CppTokens::ADD, false) ||
		TestToken(cursor, CppTokens::SUB, false) ||
		TestToken(cursor, CppTokens::AND, false) ||
		TestToken(cursor, CppTokens::MUL, false))
	{
		auto newExpr = MakePtr<PrefixUnaryExpr>();
		FillOperatorAndSkip(newExpr->opName, cursor, 1);
		FillOperator(newExpr->opName, newExpr->op);
		newExpr->operand = ParsePrefixUnaryExpr(pa, cursor);
		return newExpr;
	}
	else if (TestToken(cursor, CppTokens::NEW))
	{
		auto newExpr = MakePtr<NewExpr>();
		if (TestToken(cursor, CppTokens::LPARENTHESIS))
		{
			while (true)
			{
				newExpr->placementArguments.Add(ParseExpr(pa, false, cursor));
				if (TestToken(cursor, CppTokens::RPARENTHESIS))
				{
					break;
				}
				else
				{
					RequireToken(cursor, CppTokens::COMMA);
				}
			}
		}

		newExpr->type = ParseLongType(pa, cursor);
		if (TestToken(cursor, CppTokens::LPARENTHESIS, false) || TestToken(cursor, CppTokens::LBRACE, false))
		{
			auto closeToken = (CppTokens)cursor->token.token == CppTokens::LPARENTHESIS ? CppTokens::RPARENTHESIS : CppTokens::RBRACE;
			SkipToken(cursor);

			newExpr->initializer = MakePtr<Initializer>();
			newExpr->initializer->initializerType = closeToken == CppTokens::RPARENTHESIS ? InitializerType::Constructor : InitializerType::Universal;

			if (!TestToken(cursor, closeToken))
			{
				while (true)
				{
					newExpr->initializer->arguments.Add(ParseExpr(pa, false, cursor));
					if (TestToken(cursor, closeToken))
					{
						break;
					}
					else if (!TestToken(cursor, CppTokens::COMMA))
					{
						throw StopParsingException(cursor);
					}
				}
			}
		}
		else if (TestToken(cursor, CppTokens::LBRACKET, false))
		{
			List<Ptr<Expr>> sizeExprs;
			while (TestToken(cursor, CppTokens::LBRACKET))
			{
				sizeExprs.Add(ParseExpr(pa, true, cursor));
				RequireToken(cursor, CppTokens::RBRACKET);
			}

			for (vint i = sizeExprs.Count() - 1; i >= 0; i--)
			{
				auto arrayType = MakePtr<ArrayType>();
				arrayType->type = newExpr->type;
				arrayType->expr = sizeExprs[i];
				newExpr->type = arrayType;
			}
		}
		return newExpr;
	}
	else if (TestToken(cursor, CppTokens::DELETE))
	{
		auto newExpr = MakePtr<DeleteExpr>();
		if ((newExpr->arrayDelete = TestToken(cursor, CppTokens::LBRACKET)))
		{
			RequireToken(cursor, CppTokens::RBRACKET);
		}
		newExpr->expr = ParsePrefixUnaryExpr(pa, cursor);
		return newExpr;
	}
	else
	{
		Ptr<Type> type;
		auto oldCursor = cursor;
		try
		{
			RequireToken(cursor, CppTokens::LPARENTHESIS);
			type = ParseType(pa, cursor);
			RequireToken(cursor, CppTokens::RPARENTHESIS);
		}
		catch (const StopParsingException&)
		{
			cursor = oldCursor;
		}

		if (type)
		{
			auto newExpr = MakePtr<CastExpr>();
			newExpr->castType = CppCastType::CCast;
			newExpr->type = type;
			newExpr->expr = ParsePrefixUnaryExpr(pa, cursor);
			return newExpr;
		}
		else
		{
			return ParsePostfixUnaryExpr(pa, cursor);
		}
	}
}

/***********************************************************************
ParseBinaryExpr
***********************************************************************/

Ptr<Expr> ParseBinaryExpr(const ParsingArguments& pa, Ptr<CppTokenCursor>& cursor)
{
	List<Ptr<BinaryExpr>> binaryStack;
	auto popped = ParsePrefixUnaryExpr(pa, cursor);
	while (true)
	{
		CppName opName;
		vint precedence = -1;

		if (TestToken(cursor, CppTokens::DOT, CppTokens::MUL, false))
		{
			FillOperatorAndSkip(opName, cursor, 2);
			precedence = 4;
		}
		else if (TestToken(cursor, CppTokens::SUB, CppTokens::GT, CppTokens::MUL, false))
		{
			FillOperatorAndSkip(opName, cursor, 3);
			precedence = 4;
		}
		else if (TestToken(cursor, CppTokens::MUL, false) && !TestToken(cursor, CppTokens::MUL, CppTokens::EQ, false))
		{
			FillOperatorAndSkip(opName, cursor, 1);
			precedence = 5;
		}
		else if (TestToken(cursor, CppTokens::DIV, false) && !TestToken(cursor, CppTokens::DIV, CppTokens::EQ, false))
		{
			FillOperatorAndSkip(opName, cursor, 1);
			precedence = 5;
		}
		else if (TestToken(cursor, CppTokens::PERCENT, false) && !TestToken(cursor, CppTokens::PERCENT, CppTokens::EQ, false))
		{
			FillOperatorAndSkip(opName, cursor, 1);
			precedence = 5;
		}
		else if (TestToken(cursor, CppTokens::ADD, false) && !TestToken(cursor, CppTokens::ADD, CppTokens::EQ, false))
		{
			FillOperatorAndSkip(opName, cursor, 1);
			precedence = 6;
		}
		else if (TestToken(cursor, CppTokens::SUB, false) && !TestToken(cursor, CppTokens::SUB, CppTokens::EQ, false))
		{
			FillOperatorAndSkip(opName, cursor, 1);
			precedence = 6;
		}
		else if (TestToken(cursor, CppTokens::LT, CppTokens::LT, false) && !TestToken(cursor, CppTokens::LT, CppTokens::LT, CppTokens::EQ, false))
		{
			FillOperatorAndSkip(opName, cursor, 2);
			precedence = 7;
		}
		else if (TestToken(cursor, CppTokens::GT, CppTokens::GT, false) && !TestToken(cursor, CppTokens::GT, CppTokens::GT, CppTokens::EQ, false))
		{
			FillOperatorAndSkip(opName, cursor, 2);
			precedence = 7;
		}
		else if (TestToken(cursor, CppTokens::LT, CppTokens::EQ, false) || TestToken(cursor, CppTokens::GT, CppTokens::EQ, false))
		{
			FillOperatorAndSkip(opName, cursor, 2);
			precedence = 8;
		}
		else if (TestToken(cursor, CppTokens::LT, false) && !TestToken(cursor, CppTokens::LT, CppTokens::LT, CppTokens::EQ, false))
		{
			FillOperatorAndSkip(opName, cursor, 1);
			precedence = 8;
		}
		else if (TestToken(cursor, CppTokens::GT, false) && !TestToken(cursor, CppTokens::GT, CppTokens::GT, CppTokens::EQ, false))
		{
			FillOperatorAndSkip(opName, cursor, 1);
			precedence = 8;
		}
		else if (TestToken(cursor, CppTokens::EQ, CppTokens::EQ, false) || TestToken(cursor, CppTokens::NOT, CppTokens::EQ, false))
		{
			FillOperatorAndSkip(opName, cursor, 2);
			precedence = 9;
		}
		else if (TestToken(cursor, CppTokens::OR, CppTokens::OR, false))
		{
			FillOperatorAndSkip(opName, cursor, 2);
			precedence = 14;
		}
		else if (TestToken(cursor, CppTokens::OR, false) && !TestToken(cursor, CppTokens::OR, CppTokens::EQ, false))
		{
			FillOperatorAndSkip(opName, cursor, 1);
			precedence = 12;
		}
		else if (TestToken(cursor, CppTokens::AND, CppTokens::AND, false))
		{
			FillOperatorAndSkip(opName, cursor, 2);
			precedence = 13;
		}
		else if (TestToken(cursor, CppTokens::AND, false) && !TestToken(cursor, CppTokens::AND, CppTokens::EQ, false))
		{
			FillOperatorAndSkip(opName, cursor, 1);
			precedence = 10;
		}
		else if (TestToken(cursor, CppTokens::XOR, false) && !TestToken(cursor, CppTokens::XOR, CppTokens::EQ, false))
		{
			FillOperatorAndSkip(opName, cursor, 1);
			precedence = 11;
		}
		else
		{
			break;
		}

		while (binaryStack.Count() > 0)
		{
			auto last = binaryStack[binaryStack.Count() - 1];
			if (last->precedence < precedence)
			{
				popped = last;
				binaryStack.RemoveAt(binaryStack.Count() - 1);
			}
			else
			{
				break;
			}
		}

		auto newExpr = MakePtr<BinaryExpr>();
		newExpr->opName = opName;
		FillOperator(newExpr->opName, newExpr->op);
		newExpr->precedence = precedence;
		newExpr->left = popped;
		newExpr->right = ParsePrefixUnaryExpr(pa, cursor);

		if (binaryStack.Count() > 0)
		{
			binaryStack[binaryStack.Count() - 1]->right = newExpr;
		}
		binaryStack.Add(newExpr);
	}
	return binaryStack.Count() > 0 ? Ptr<Expr>(binaryStack[0]) : popped;
}

/***********************************************************************
ParseIfExpr
***********************************************************************/

Ptr<Expr> ParseIfExpr(const ParsingArguments& pa, Ptr<CppTokenCursor>& cursor)
{
	auto expr = ParseBinaryExpr(pa, cursor);
	if (TestToken(cursor, CppTokens::QUESTIONMARK))
	{
		auto newExpr = MakePtr<IfExpr>();
		newExpr->condition = expr;
		newExpr->left = ParseIfExpr(pa, cursor);
		RequireToken(cursor, CppTokens::COLON);
		newExpr->right = ParseIfExpr(pa, cursor);
		return newExpr;
	}
	else
	{
		return expr;
	}
}

/***********************************************************************
ParseAssignExpr
***********************************************************************/

Ptr<Expr> ParseAssignExpr(const ParsingArguments& pa, Ptr<CppTokenCursor>& cursor)
{
	auto expr = ParseIfExpr(pa, cursor);
	if (TestToken(cursor, CppTokens::EQ, false))
	{
		auto newExpr = MakePtr<BinaryExpr>();
		FillOperatorAndSkip(newExpr->opName, cursor, 1);
		FillOperator(newExpr->opName, newExpr->op);
		newExpr->precedence = 16;
		newExpr->left = expr;
		newExpr->right = ParseAssignExpr(pa, cursor);
		return newExpr;
	}
	else if (
		TestToken(cursor, CppTokens::MUL, CppTokens::EQ, false) ||
		TestToken(cursor, CppTokens::DIV, CppTokens::EQ, false) ||
		TestToken(cursor, CppTokens::PERCENT, CppTokens::EQ, false) ||
		TestToken(cursor, CppTokens::ADD, CppTokens::EQ, false) ||
		TestToken(cursor, CppTokens::SUB, CppTokens::EQ, false) ||
		TestToken(cursor, CppTokens::AND, CppTokens::EQ, false) ||
		TestToken(cursor, CppTokens::OR, CppTokens::EQ, false) ||
		TestToken(cursor, CppTokens::XOR, CppTokens::EQ, false))
	{
		auto newExpr = MakePtr<BinaryExpr>();
		FillOperatorAndSkip(newExpr->opName, cursor, 2);
		FillOperator(newExpr->opName, newExpr->op);
		newExpr->precedence = 16;
		newExpr->left = expr;
		newExpr->right = ParseAssignExpr(pa, cursor);
		return newExpr;
	}
	else if (
		TestToken(cursor, CppTokens::LT, CppTokens::LT, CppTokens::EQ, false) ||
		TestToken(cursor, CppTokens::GT, CppTokens::GT, CppTokens::EQ, false))
	{
		auto newExpr = MakePtr<BinaryExpr>();
		FillOperatorAndSkip(newExpr->opName, cursor, 3);
		FillOperator(newExpr->opName, newExpr->op);
		newExpr->precedence = 16;
		newExpr->left = expr;
		newExpr->right = ParseAssignExpr(pa, cursor);
		return newExpr;
	}
	else
	{
		return expr;
	}
}

/***********************************************************************
ParseThrowExpr
***********************************************************************/

Ptr<Expr> ParseThrowExpr(const ParsingArguments& pa, Ptr<CppTokenCursor>& cursor)
{
	if (TestToken(cursor, CppTokens::THROW))
	{
		auto newExpr = MakePtr<ThrowExpr>();
		if (!TestToken(cursor, CppTokens::SEMICOLON, false))
		{
			newExpr->expr = ParseAssignExpr(pa, cursor);
		}
		return newExpr;
	}
	else
	{
		return ParseAssignExpr(pa, cursor);
	}
}

/***********************************************************************
ParseExpr
***********************************************************************/

Ptr<Expr> ParseExpr(const ParsingArguments& pa, bool allowComma, Ptr<CppTokenCursor>& cursor)
{
	auto expr = ParseThrowExpr(pa, cursor);
	while (allowComma)
	{
		if (TestToken(cursor, CppTokens::COMMA, false))
		{
			auto newExpr = MakePtr<BinaryExpr>();
			FillOperatorAndSkip(newExpr->opName, cursor, 1);
			FillOperator(newExpr->opName, newExpr->op);
			newExpr->precedence = 18;
			newExpr->left = expr;
			newExpr->right = ParseThrowExpr(pa, cursor);
			expr = newExpr;
		}
		else
		{
			break;
		}
	}
	return expr;
}