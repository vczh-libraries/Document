#include <VlppOS.h>
#include "Ast_Expr.h"
#include "Ast_Resolving.h"

/***********************************************************************
FillOperatorAndSkip
***********************************************************************/

void FillOperatorAndSkip(CppName& name, Ptr<CppTokenCursor>& cursor, vint count)
{
	auto reading = cursor->token.reading;
	vint length = 0;

	name.type = CppNameType::Operator;
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
			if (!rsr.values && !TestToken(cursor, CppTokens::LPARENTHESIS, false))
			{
				// unknown name is acceptable only when it could be resolved by argument-dependent lookup
				// so "(" has to be right after this name
				throw StopParsingException(cursor);
			}
			auto expr = MakePtr<IdExpr>();
			expr->name = cppName;
			expr->resolving = rsr.values;
			if (pa.recorder && expr->resolving)
			{
				pa.recorder->Index(expr->name, expr->resolving->resolvedSymbols);
			}
			return expr;
		}
	}
	throw StopParsingException(cursor);
}

/***********************************************************************
TryParseChildExpr
***********************************************************************/

Ptr<ChildExpr> TryParseChildExpr(const ParsingArguments& pa, Ptr<Type> classType, bool& templateKeyword, Ptr<CppTokenCursor>& cursor)
{
	templateKeyword = TestToken(cursor, CppTokens::DECL_TEMPLATE);
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
				pa.recorder->Index(type->name, type->resolving->resolvedSymbols);
			}
			return type;
		}
		else if (templateKeyword)
		{
			auto type = MakePtr<ChildExpr>();
			type->classType = classType;
			type->name = cppName;
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
ParseNameOrCtorAccessExpr
***********************************************************************/

Ptr<GenericExpr> ParseGenericExprSkippedLT(const ParsingArguments& pa, Ptr<CppTokenCursor>& cursor, Ptr<Category_Id_Child_Expr> catIdChildExpr)
{
	// EXPR< { TYPE/EXPR ...} >
	auto expr = MakePtr<GenericExpr>();
	expr->expr = catIdChildExpr;
	ParseGenericArgumentsSkippedLT(pa, cursor, expr->arguments, CppTokens::GT);
	return expr;
}

Ptr<Category_Id_Child_Generic_Expr> TryParseGenericExpr(const ParsingArguments& pa, Ptr<CppTokenCursor>& cursor, Ptr<Category_Id_Child_Expr> catIdChildExpr, bool templateKeyword, bool tryGenericForEmptyResolving)
{
	if (!TestToken(cursor, CppTokens::LT, false))
	{
		if (templateKeyword)
		{
			throw StopParsingException(cursor);
		}
		else
		{
			return catIdChildExpr;
		}
	}

	bool allowRollback = false;
	if (catIdChildExpr->resolving)
	{
		List<Symbol*> genericSymbols;
		for (vint i = 0; i < catIdChildExpr->resolving->resolvedSymbols.Count(); i++)
		{
			auto symbol = catIdChildExpr->resolving->resolvedSymbols[i];
			if (symbol_type_resolving::GetTemplateSpecFromSymbol(symbol))
			{
				genericSymbols.Add(symbol);
			}
		}

		if (genericSymbols.Count() == 0) return catIdChildExpr;
		CopyFrom(catIdChildExpr->resolving->resolvedSymbols, genericSymbols);
	}
	else if (!templateKeyword)
	{
		if (tryGenericForEmptyResolving)
		{
			// TODO: This is not safe, need to evaluate the parent expression instead of just try
			allowRollback = true;
		}
		else
		{
			return catIdChildExpr;
		}
	}


	if (allowRollback)
	{
		auto oldCursor = cursor;
		try
		{
			SkipToken(cursor);
			return ParseGenericExprSkippedLT(pa, cursor, catIdChildExpr);
		}
		catch (const StopParsingException&)
		{
			cursor = oldCursor;
			return catIdChildExpr;
		}
	}
	else
	{
		SkipToken(cursor);
		return ParseGenericExprSkippedLT(pa, cursor, catIdChildExpr);
	}
}

Ptr<Expr> ParseNameOrCtorAccessExpr(const ParsingArguments& pa, Ptr<CppTokenCursor>& cursor)
{
	{
		auto oldCursor = cursor;
		try
		{
			auto type = ParseLongType(pa, cursor);

			if (TestToken(cursor, CppTokens::COLON, CppTokens::COLON))
			{
				bool templateKeyword = false;
				if (auto expr = TryParseChildExpr(pa, type, templateKeyword, cursor))
				{
					return TryParseGenericExpr(pa, cursor, expr, templateKeyword, false);
				}
			}

			if (TestToken(cursor, CppTokens::LPARENTHESIS, false) || TestToken(cursor, CppTokens::LBRACE, false))
			{
				auto closeToken = (CppTokens)cursor->token.token == CppTokens::LPARENTHESIS ? CppTokens::RPARENTHESIS : CppTokens::RBRACE;
				SkipToken(cursor);

				auto expr = MakePtr<CtorAccessExpr>();
				expr->type = type;
				expr->initializer = MakePtr<Initializer>();
				expr->initializer->initializerType = closeToken == CppTokens::RPARENTHESIS ? CppInitializerType::Constructor : CppInitializerType::Universal;

				if (!TestToken(cursor, closeToken))
				{
					while (true)
					{
						auto argument = ParseExpr(pa, pea_Argument(), cursor);
						bool isVariadic = TestToken(cursor, CppTokens::DOT, CppTokens::DOT, CppTokens::DOT);
						expr->initializer->arguments.Add({ argument,isVariadic });
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
		bool templateKeyword = false;
		if (auto expr = TryParseChildExpr(pa, MakePtr<RootType>(), templateKeyword, cursor))
		{
			if (templateKeyword)
			{
				throw StopParsingException(cursor);
			}
			return TryParseGenericExpr(pa, cursor, expr, false, false);
		}
	}
	else
	{
		if (auto expr = ParseIdExpr(pa, cursor))
		{
			return TryParseGenericExpr(pa, cursor, expr, false, false);
		}
	}
	throw StopParsingException(cursor);
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
		case CppTokens::EXPR___NULLPTR:
			{
				SkipToken(cursor);
				return MakePtr<NullptrExpr>();
			}
		case CppTokens::LPARENTHESIS:
			{
				SkipToken(cursor);
				auto expr = MakePtr<ParenthesisExpr>();
				expr->expr = ParseExpr(pa, pea_Full(), cursor);
				RequireToken(cursor, CppTokens::RPARENTHESIS);
				return expr;
			}
		case CppTokens::LBRACE:
			{
				SkipToken(cursor);
				auto expr = MakePtr<UniversalInitializerExpr>();
				while (!TestToken(cursor, CppTokens::RBRACE))
				{
					auto argument = ParseExpr(pa, pea_Argument(), cursor);
					bool isVariadic = TestToken(cursor, CppTokens::DOT, CppTokens::DOT, CppTokens::DOT);
					expr->arguments.Add({ argument,isVariadic });
					if (TestToken(cursor, CppTokens::RBRACE))
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
				expr->expr = ParseExpr(pa, pea_Full(), cursor);
				RequireToken(cursor, CppTokens::RPARENTHESIS);
				return expr;
			}
		case CppTokens::EXPR_TYPEID:
			{
				SkipToken(cursor);
				auto expr = MakePtr<TypeidExpr>();
				RequireToken(cursor, CppTokens::LPARENTHESIS);
				ParseTypeOrExpr(pa, pea_Full(), cursor, expr->type, expr->expr);
				RequireToken(cursor, CppTokens::RPARENTHESIS);
				return expr;
			}
			break;
		}
	}
	return ParseNameOrCtorAccessExpr(pa, cursor);
}

/***********************************************************************
ParsePostfixUnaryExpr
***********************************************************************/

Ptr<Category_Id_Child_Generic_Expr> ParseFieldAccessNameExpr(const ParsingArguments& pa, Ptr<CppTokenCursor>& cursor)
{
	auto oldCursor = cursor;
	try
	{
		auto type = ParseLongType(pa, cursor);
		RequireToken(cursor, CppTokens::COLON, CppTokens::COLON);
		bool templateKeyword = false;
		if (auto childExpr = TryParseChildExpr(pa, type, templateKeyword, cursor))
		{
			return TryParseGenericExpr(pa, cursor, childExpr, templateKeyword, false);
		}
	}
	catch (const StopParsingException&)
	{
	}
	cursor = oldCursor;

	auto idExpr = MakePtr<IdExpr>();
	if (!ParseCppName(idExpr->name, cursor, false) && !ParseCppName(idExpr->name, cursor, true))
	{
		throw StopParsingException(cursor);
	}
	return TryParseGenericExpr(pa, cursor, idExpr, false, true);
}

Ptr<Expr> ParsePostfixUnaryExpr(const ParsingArguments& pa, Ptr<CppTokenCursor>& cursor)
{
	auto expr = ParsePrimitiveExpr(pa, cursor);
	while (true)
	{
		if (!TestToken(cursor, CppTokens::DOT, CppTokens::MUL, false) && !TestToken(cursor, CppTokens::DOT, CppTokens::DOT, CppTokens::DOT, false) && TestToken(cursor, CppTokens::DOT))
		{
			auto newExpr = MakePtr<FieldAccessExpr>();
			newExpr->type = CppFieldAccessType::Dot;
			newExpr->expr = expr;
			newExpr->name = ParseFieldAccessNameExpr(pa, cursor);
			expr = newExpr;
		}
		else if (!TestToken(cursor, CppTokens::SUB, CppTokens::GT, CppTokens::MUL, false) && TestToken(cursor, CppTokens::SUB, CppTokens::GT, false))
		{
			auto newExpr = MakePtr<FieldAccessExpr>();
			{
				newExpr->opName.type = CppNameType::Operator;
				newExpr->opName.name = L"->";
				newExpr->opName.tokenCount = 2;
				newExpr->opName.nameTokens[0] = cursor->token;
				SkipToken(cursor);
				newExpr->opName.nameTokens[1] = cursor->token;
				SkipToken(cursor);
			}
			newExpr->type = CppFieldAccessType::Arrow;
			newExpr->expr = expr;
			newExpr->name = ParseFieldAccessNameExpr(pa, cursor);
			expr = newExpr;
		}
		else if (TestToken(cursor, CppTokens::LBRACKET, false))
		{
			auto newExpr = MakePtr<ArrayAccessExpr>();
			{
				newExpr->opName.type = CppNameType::Operator;
				newExpr->opName.name = L"[]";
				newExpr->opName.tokenCount = 1;
				newExpr->opName.nameTokens[0] = cursor->token;
				SkipToken(cursor);
			}
			newExpr->expr = expr;
			newExpr->index = ParseExpr(pa, pea_Full(), cursor);

			RequireToken(cursor, CppTokens::RBRACKET);
			expr = newExpr;
		}
		else if (TestToken(cursor, CppTokens::LPARENTHESIS, false))
		{
			if (auto idExpr = expr.Cast<IdExpr>())
			{
				if (!idExpr->resolving)
				{
					Ptr<Type> returnType;
					if (idExpr->name.name == L"alignof")
					{
						auto sizeType = MakePtr<PrimitiveType>();
						sizeType->prefix = CppPrimitivePrefix::_unsigned;
						sizeType->primitive = CppPrimitiveType::_int;
						returnType = sizeType;
					}
					else if (INVLOC.StartsWith(idExpr->name.name, L"__is_", Locale::Normalization::None) || INVLOC.StartsWith(idExpr->name.name, L"__has_", Locale::Normalization::None))
					{
						auto boolType = MakePtr<PrimitiveType>();
						boolType->prefix = CppPrimitivePrefix::_none;
						boolType->primitive = CppPrimitiveType::_bool;
						returnType = boolType;
					}

					if (returnType)
					{
						auto builtinExpr = MakePtr<BuiltinFuncAccessExpr>();
						builtinExpr->name = idExpr;
						builtinExpr->returnType = returnType;

						SkipToken(cursor);
						ParseGenericArgumentsSkippedLT(pa, cursor, builtinExpr->arguments, CppTokens::RPARENTHESIS);

						expr = builtinExpr;
						continue;
					}
				}
			}
			{
				auto newExpr = MakePtr<FuncAccessExpr>();
				{
					newExpr->opName.type = CppNameType::Operator;
					newExpr->opName.name = L"()";
					newExpr->opName.tokenCount = 1;
					newExpr->opName.nameTokens[0] = cursor->token;
				}
				newExpr->expr = expr;

				SkipToken(cursor);
				if (!TestToken(cursor, CppTokens::RPARENTHESIS))
				{
					while (true)
					{
						auto argument = ParseExpr(pa, pea_Argument(), cursor);
						bool isVariadic = TestToken(cursor, CppTokens::DOT, CppTokens::DOT, CppTokens::DOT);
						newExpr->arguments.Add({ argument,isVariadic });
						if (!TestToken(cursor, CppTokens::COMMA))
						{
							RequireToken(cursor, CppTokens::RPARENTHESIS);
							break;
						}
					}
				}
				expr = newExpr;
			}
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
		newExpr->ellipsis = TestToken(cursor, CppTokens::DOT, CppTokens::DOT, CppTokens::DOT);
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
			newExpr->type = nullptr;
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
				auto argument = ParseExpr(pa, pea_Argument(), cursor);
				bool isVariadic = TestToken(cursor, CppTokens::DOT, CppTokens::DOT, CppTokens::DOT);
				newExpr->placementArguments.Add({ argument,isVariadic });
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
			newExpr->initializer->initializerType = closeToken == CppTokens::RPARENTHESIS ? CppInitializerType::Constructor : CppInitializerType::Universal;

			if (!TestToken(cursor, closeToken))
			{
				while (true)
				{
					auto argument = ParseExpr(pa, pea_Argument(), cursor);
					bool isVariadic = TestToken(cursor, CppTokens::DOT, CppTokens::DOT, CppTokens::DOT);
					newExpr->initializer->arguments.Add({ argument,isVariadic });
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
				sizeExprs.Add(ParseExpr(pa, pea_Full(), cursor));
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
			type = nullptr;
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

Ptr<Expr> ParseBinaryExpr(const ParsingArguments& pa, const ParsingExprArguments& pea, Ptr<CppTokenCursor>& cursor)
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
		else if (pea.allowGt && TestToken(cursor, CppTokens::GT, false) && !TestToken(cursor, CppTokens::GT, CppTokens::GT, CppTokens::EQ, false))
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

Ptr<Expr> ParseIfExpr(const ParsingArguments& pa, const ParsingExprArguments& pea, Ptr<CppTokenCursor>& cursor)
{
	auto expr = ParseBinaryExpr(pa, pea, cursor);
	if (TestToken(cursor, CppTokens::QUESTIONMARK))
	{
		auto newExpr = MakePtr<IfExpr>();
		newExpr->condition = expr;
		newExpr->left = ParseIfExpr(pa, pea, cursor);
		RequireToken(cursor, CppTokens::COLON);
		newExpr->right = ParseIfExpr(pa, pea, cursor);
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

Ptr<Expr> ParseAssignExpr(const ParsingArguments& pa, const ParsingExprArguments& pea, Ptr<CppTokenCursor>& cursor)
{
	auto expr = ParseIfExpr(pa, pea, cursor);
	if (TestToken(cursor, CppTokens::EQ, false))
	{
		auto newExpr = MakePtr<BinaryExpr>();
		FillOperatorAndSkip(newExpr->opName, cursor, 1);
		FillOperator(newExpr->opName, newExpr->op);
		newExpr->precedence = 16;
		newExpr->left = expr;
		newExpr->right = ParseAssignExpr(pa, pea, cursor);
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
		newExpr->right = ParseAssignExpr(pa, pea, cursor);
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
		newExpr->right = ParseAssignExpr(pa, pea, cursor);
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

Ptr<Expr> ParseThrowExpr(const ParsingArguments& pa, const ParsingExprArguments& pea, Ptr<CppTokenCursor>& cursor)
{
	if (TestToken(cursor, CppTokens::THROW))
	{
		auto newExpr = MakePtr<ThrowExpr>();
		if (!TestToken(cursor, CppTokens::SEMICOLON, false))
		{
			newExpr->expr = ParseAssignExpr(pa, pea, cursor);
		}
		return newExpr;
	}
	else
	{
		return ParseAssignExpr(pa, pea, cursor);
	}
}

/***********************************************************************
ParseExpr
***********************************************************************/

Ptr<Expr> ParseExpr(const ParsingArguments& pa, const ParsingExprArguments& pea, Ptr<CppTokenCursor>& cursor)
{
	auto expr = ParseThrowExpr(pa, pea, cursor);
	while (pea.allowComma)
	{
		if (TestToken(cursor, CppTokens::COMMA, false))
		{
			auto newExpr = MakePtr<BinaryExpr>();
			FillOperatorAndSkip(newExpr->opName, cursor, 1);
			FillOperator(newExpr->opName, newExpr->op);
			newExpr->precedence = 18;
			newExpr->left = expr;
			newExpr->right = ParseThrowExpr(pa, pea, cursor);
			expr = newExpr;
		}
		else
		{
			break;
		}
	}
	return expr;
}