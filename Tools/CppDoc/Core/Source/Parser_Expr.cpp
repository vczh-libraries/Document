#include <VlppOS.h>
#include "Parser_Declarator.h"
#include "Ast_Expr.h"
#include "Ast_Resolving.h"
#include "Symbol_Visit.h"
#include "Symbol_TemplateSpec.h"

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
		auto rsr = ResolveSymbolInContext(pa, cppName, false);
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
				pa.recorder->Index(expr->name, expr->resolving->items);
			}
			return expr;
		}
	}
	throw StopParsingException(cursor);
}

/***********************************************************************
ParseGenericOrChildExpr
***********************************************************************/

void CheckResolvingForGenericExpr(const ParsingArguments& pa, Ptr<Resolving> resolving, bool templateKeyword, bool& allowGenericPresence, bool& allowGenericAbsence)
{
	if (resolving)
	{
		allowGenericAbsence = true;

		for (vint i = 0; i < resolving->items.Count(); i++)
		{
			auto symbol = resolving->items[i].symbol;
			auto spec = symbol_type_resolving::GetTemplateSpecFromSymbol(symbol);

			if (spec)
			{
				// even if there is no argument, it means this name is a template
				allowGenericPresence = true;
			}
		}
	}
	else if (templateKeyword)
	{
		allowGenericPresence = true;
	}
	else
	{
		allowGenericPresence = true;
		allowGenericAbsence = true;
	}
}

bool PrepareArgumentsForGenericExpr(const ParsingArguments& pa, VariadicList<GenericArgument>& genericArguments, bool allowGenericPresence, bool allowGenericAbsence, Ptr<CppTokenCursor>& cursor)
{
	if (!allowGenericPresence) return false;
	bool hasGenericPart = false;
	{
		auto oldCursor = cursor;
		if (TestToken(cursor, CppTokens::LT))
		{
			try
			{
				ParseGenericArgumentsSkippedLT(pa, cursor, genericArguments, CppTokens::GT);
				hasGenericPart = true;
			}
			catch (const StopParsingException&)
			{
				cursor = oldCursor;
			}
		}
	}

	if (!hasGenericPart && !allowGenericAbsence)
	{
		throw StopParsingException(cursor);
	}

	return hasGenericPart;
}

Ptr<Category_Id_Child_Generic_Expr> TryParseGenericExpr(const ParsingArguments& pa, Ptr<Category_Id_Child_Expr> expr, bool templateKeyword, Ptr<CppTokenCursor>& cursor)
{
	bool allowGenericPresence = false;
	bool allowGenericAbsence = false;
	CheckResolvingForGenericExpr(pa, expr->resolving, templateKeyword, allowGenericPresence, allowGenericAbsence);

	VariadicList<GenericArgument> genericArguments;
	if (PrepareArgumentsForGenericExpr(pa, genericArguments, allowGenericPresence, allowGenericAbsence, cursor))
	{
		auto genericExpr = MakePtr<GenericExpr>();
		genericExpr->expr = expr;
		CopyFrom(genericExpr->arguments, genericArguments);
		return genericExpr;
	}
	else
	{
		return expr;
	}
}

Ptr<Category_Id_Child_Generic_Expr> TryParseGenericExpr(const ParsingArguments& pa, Ptr<Category_Id_Child_Generic_Root_Type> type, Ptr<CppTokenCursor>& cursor)
{
	while (true)
	{
		if (TestToken(cursor, CppTokens::COLON, CppTokens::COLON))
		{
			bool templateKeyword = TestToken(cursor, CppTokens::DECL_TEMPLATE);
			CppName cppName;

			// parse identifier
			if (!ParseCppName(cppName, cursor))
			{
				throw StopParsingException(cursor);
			}

			// check if we know what it is
			bool makeExpr = false;
			bool allowGenericPresence = false;
			bool allowGenericAbsence = false;
			auto rsr = ResolveChildSymbol(pa, type, cppName);
			if (rsr.types && !rsr.values)
			{
				throw StopParsingException(cursor);
			}

			CheckResolvingForGenericExpr(pa, rsr.values, templateKeyword, allowGenericPresence, allowGenericAbsence);
			if (rsr.values)
			{
				makeExpr = true;
			}

			// parse generic arguments
			VariadicList<GenericArgument> genericArguments;
			bool hasGenericPart = PrepareArgumentsForGenericExpr(pa, genericArguments, allowGenericPresence, allowGenericAbsence, cursor);

			// choose between ChildType and ChildExpr
			if (!makeExpr)
			{
				makeExpr = !TestToken(cursor, CppTokens::COLON, CppTokens::COLON, false);
			}

			if (makeExpr)
			{
				// if we decide to make a ChildExpr, return it
				auto childExpr = MakePtr<ChildExpr>();
				childExpr->classType = type;
				childExpr->name = cppName;
				childExpr->resolving = rsr.values;

				if (pa.recorder && childExpr->resolving)
				{
					pa.recorder->Index(childExpr->name, childExpr->resolving->items);
				}

				if (hasGenericPart)
				{
					auto genericExpr = MakePtr<GenericExpr>();
					genericExpr->expr = childExpr;
					CopyFrom(genericExpr->arguments, genericArguments);
					return genericExpr;
				}
				else
				{
					return childExpr;
				}
			}
			else
			{
				// if we decide to make a ChildType, continue until we get a ChildExpr
				auto childType = MakePtr<ChildType>();
				childType->classType = type;
				childType->name = cppName;

				if (hasGenericPart)
				{
					auto genericType = MakePtr<GenericType>();
					genericType->type = childType;
					CopyFrom(genericType->arguments, genericArguments);
					type = genericType;
				}
				else
				{
					type = childType;
				}
			}
		}
		else
		{
			throw StopParsingException(cursor);
		}
	}
}

/***********************************************************************
ParseNameOrCtorAccessExpr
***********************************************************************/

Ptr<Type> AppendArrayTypeForCtor(const ParsingArguments& pa, Ptr<Type> type, Ptr<CppTokenCursor>& cursor)
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
		arrayType->type = type;
		arrayType->expr = sizeExprs[i];
		type = arrayType;
	}

	return type;
}

Ptr<Expr> ParseNameOrCtorAccessExpr(const ParsingArguments& pa, Ptr<CppTokenCursor>& cursor)
{
	{
		auto oldCursor = cursor;
		try
		{
			auto type = ParseLongType(pa, cursor);
			type = AppendArrayTypeForCtor(pa, type, cursor);

			if (TestToken(cursor, CppTokens::COLON, CppTokens::COLON, false))
			{
				// if we see ::, it has to be a ChildExpr of a type
				if (auto baseType = type.Cast<Category_Id_Child_Generic_Root_Type>())
				{
					return TryParseGenericExpr(pa, baseType, cursor);
				}
				else
				{
					throw StopParsingException(cursor);
				}
			}

			if (TestToken(cursor, CppTokens::LPARENTHESIS, false) || TestToken(cursor, CppTokens::LBRACE, false))
			{
				// TYPE(EXPR, ...)
				// TYPE{EXPR, ...)
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

	if (TestToken(cursor, CppTokens::COLON, CppTokens::COLON, false))
	{
		// if we see ::, it has to be a ChildExpr of a global scope
		return TryParseGenericExpr(pa, MakePtr<RootType>(), cursor);
	}
	else
	{
		bool templateKeyword = TestToken(cursor, CppTokens::DECL_TEMPLATE);
		if (auto expr = ParseIdExpr(pa, cursor))
		{
			return TryParseGenericExpr(pa, expr, templateKeyword, cursor);
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
		case CppTokens::FLOATHEX:
		case CppTokens::CHAR:
			{
				// literal
				auto literal = MakePtr<LiteralExpr>();
				literal->tokens.Add(cursor->token);
				SkipToken(cursor);
				return literal;
			}
		case CppTokens::MACRO_LPREFIX:
		case CppTokens::STRING:
			{
				// string literla, could be multiple tokens
				auto literal = MakePtr<LiteralExpr>();
				while (
					cursor && (
						(CppTokens)cursor->token.token == CppTokens::MACRO_LPREFIX ||
						(CppTokens)cursor->token.token == CppTokens::STRING
						)
					)
				{
					literal->tokens.Add(cursor->token);
					SkipToken(cursor);
				}
				return literal;
			}
		case CppTokens::EXPR_THIS:
			{
				// this
				SkipToken(cursor);
				return MakePtr<ThisExpr>();
			}
		case CppTokens::EXPR_NULLPTR:
		case CppTokens::EXPR___NULLPTR:
			{
				// nullptr
				// __nullptr
				SkipToken(cursor);
				return MakePtr<NullptrExpr>();
			}
		case CppTokens::LPARENTHESIS:
			{
				// (a)
				SkipToken(cursor);
				auto expr = MakePtr<ParenthesisExpr>();
				expr->expr = ParseExpr(pa, pea_Full(), cursor);
				RequireToken(cursor, CppTokens::RPARENTHESIS);
				return expr;
			}
		case CppTokens::LBRACE:
			{
				// {EXPR, ...}
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
				// *_cast<TYPE>(EXPR)
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
				// typeid(EXPR)
				SkipToken(cursor);
				auto expr = MakePtr<TypeidExpr>();
				RequireToken(cursor, CppTokens::LPARENTHESIS);
				ParseTypeOrExpr(pa, pea_Full(), cursor, expr->type, expr->expr);
				RequireToken(cursor, CppTokens::RPARENTHESIS);
				return expr;
			}
		case CppTokens::LBRACKET:
			{
				// lambda expression
				SkipToken(cursor);
				auto expr = MakePtr<LambdaExpr>();

				if (!TestToken(cursor, CppTokens::RBRACKET))
				{
					while (true)
					{
						{
							if (TestToken(cursor, CppTokens::EQ))
							{
								if (expr->captures.Count() > 0) throw StopParsingException(cursor);
								if (expr->captureDefault != LambdaExpr::CaptureDefaultKind::None) throw StopParsingException(cursor);
								expr->captureDefault = LambdaExpr::CaptureDefaultKind::Copy;
								goto FINISH_CAPTURE_ITEM;
							}
						}
						{
							auto oldCursor = cursor;
							if (TestToken(cursor, CppTokens::AND))
							{
								if (TestToken(cursor, CppTokens::ID))
								{
									cursor = oldCursor;
								}
								else
								{
									if (expr->captures.Count() > 0) throw StopParsingException(cursor);
									if (expr->captureDefault != LambdaExpr::CaptureDefaultKind::None) throw StopParsingException(cursor);
									expr->captureDefault = LambdaExpr::CaptureDefaultKind::Ref;
									goto FINISH_CAPTURE_ITEM;
								}
							}
						}
						{
							if (TestToken(cursor, CppTokens::EXPR_THIS))
							{
								expr->captures.Add({ LambdaExpr::CaptureKind::RefThis });
								goto FINISH_CAPTURE_ITEM;
							}
						}
						{
							if (TestToken(cursor, CppTokens::MUL))
							{
								RequireToken(cursor, CppTokens::EXPR_THIS);
								expr->captures.Add({ LambdaExpr::CaptureKind::CopyThis });
								goto FINISH_CAPTURE_ITEM;
							}
						}
						{
							LambdaExpr::Capture capture;
							capture.kind = TestToken(cursor, CppTokens::AND) ? LambdaExpr::CaptureKind::Ref : LambdaExpr::CaptureKind::Copy;
							if (!ParseCppName(capture.name, cursor)) throw StopParsingException(cursor);
							if (capture.name.type != CppNameType::Normal) throw StopParsingException(cursor);
							capture.isVariadic = TestToken(cursor, CppTokens::DOT, CppTokens::DOT, CppTokens::DOT);
							expr->captures.Add(capture);
						}
					FINISH_CAPTURE_ITEM:
						if (!TestToken(cursor, CppTokens::COMMA))
						{
							RequireToken(cursor, CppTokens::RBRACKET);
							break;
						}
					}
				}

				auto newPa = pa.WithScope(pa.scopeSymbol->CreateExprSymbol_NFb(expr));
				if (TestToken(cursor, CppTokens::LPARENTHESIS, false))
				{
					expr->type = ParseFunctionType(newPa, MakePtr<PrimitiveType>(), cursor);
				}
				else
				{
					auto funcType = MakePtr<FunctionType>();
					funcType->returnType = MakePtr<PrimitiveType>();
					expr->type = funcType;
				}
				
				if (!TestToken(cursor, CppTokens::LBRACE, false)) throw StopParsingException(cursor);
				expr->statement = ParseStat(newPa, cursor);

				return expr;
			}
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
		if (TestToken(cursor, CppTokens::COLON, CppTokens::COLON, false))
		{
			if (auto baseType = type.Cast<Category_Id_Child_Generic_Root_Type>())
			{
				return TryParseGenericExpr(pa, baseType, cursor);
			}
		}
		throw StopParsingException(cursor);
	}
	catch (const StopParsingException&)
	{
	}
	cursor = oldCursor;

	bool templateKeyword = TestToken(cursor, CppTokens::DECL_TEMPLATE);
	auto idExpr = MakePtr<IdExpr>();
	if (!ParseCppName(idExpr->name, cursor, false) && !ParseCppName(idExpr->name, cursor, true))
	{
		throw StopParsingException(cursor);
	}
	return TryParseGenericExpr(pa, idExpr, templateKeyword, cursor);
}

Ptr<Expr> ParsePostfixUnaryExpr(const ParsingArguments& pa, Ptr<CppTokenCursor>& cursor)
{
	Ptr<Expr> expr;
	if (TestToken(cursor, CppTokens::NOEXCEPT, false))
	{
		// noexcept
		auto idExpr = MakePtr<IdExpr>();
		idExpr->name.type = CppNameType::Normal;
		idExpr->name.name = L"noexcept";
		idExpr->name.tokenCount = 1;
		idExpr->name.nameTokens[0] = cursor->token;
		SkipToken(cursor);

		expr = idExpr;
	}
	else
	{
		expr = ParsePrimitiveExpr(pa, cursor);
	}

	while (true)
	{
		if (!TestToken(cursor, CppTokens::DOT, CppTokens::MUL, false) && !TestToken(cursor, CppTokens::DOT, CppTokens::DOT, CppTokens::DOT, false) && TestToken(cursor, CppTokens::DOT))
		{
			// a . b
			auto newExpr = MakePtr<FieldAccessExpr>();
			newExpr->type = CppFieldAccessType::Dot;
			newExpr->expr = expr;
			newExpr->name = ParseFieldAccessNameExpr(pa, cursor);
			expr = newExpr;
		}
		else if (!TestToken(cursor, CppTokens::SUB, CppTokens::GT, CppTokens::MUL, false) && TestToken(cursor, CppTokens::SUB, CppTokens::GT, false))
		{
			// a -> b
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
			// a [b]
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
						// alignof (EXPR, ...)
						auto sizeType = MakePtr<PrimitiveType>();
						sizeType->prefix = CppPrimitivePrefix::_unsigned;
						sizeType->primitive = CppPrimitiveType::_int;
						returnType = sizeType;
					}
					else if (idExpr->name.name == L"noexcept")
					{
						// noexcept (EXPR, ...)
						auto boolType = MakePtr<PrimitiveType>();
						boolType->prefix = CppPrimitivePrefix::_none;
						boolType->primitive = CppPrimitiveType::_bool;
						returnType = boolType;
					}
					else if (INVLOC.StartsWith(idExpr->name.name, L"__is_", Locale::Normalization::None) || INVLOC.StartsWith(idExpr->name.name, L"__has_", Locale::Normalization::None))
					{
						// __is_* (EXPR, ...)
						// __has_* (EXPR, ...)
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
				// a (EXPR, ...)
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
			// a++
			// a--
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

Ptr<Expr> ParseNewExpr(const ParsingArguments& pa, bool globalOperator, Ptr<CppTokenCursor>& cursor)
{
	// new TYPE(EXPR, ...)
	// new TYPE[EXPR][EXPR]...
	// :: new (EXPR, ...) TYPE(EXPR, ...)
	auto newExpr = MakePtr<NewExpr>();
	newExpr->globalOperator = globalOperator;
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
	newExpr->type = AppendArrayTypeForCtor(pa, newExpr->type, cursor);

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
	return newExpr;
}

Ptr<Expr> ParsePrefixUnaryExpr(const ParsingArguments& pa, Ptr<CppTokenCursor>& cursor);

Ptr<Expr> ParseDeleteExpr(const ParsingArguments& pa, bool globalOperator, Ptr<CppTokenCursor>& cursor)
{
	// delete
	// :: delete []
	auto newExpr = MakePtr<DeleteExpr>();
	newExpr->globalOperator = globalOperator;
	if ((newExpr->arrayDelete = TestToken(cursor, CppTokens::LBRACKET)))
	{
		RequireToken(cursor, CppTokens::RBRACKET);
	}
	newExpr->expr = ParsePrefixUnaryExpr(pa, cursor);
	return newExpr;
}

Ptr<Expr> ParsePrefixUnaryExpr(const ParsingArguments& pa, Ptr<CppTokenCursor>& cursor)
{
	if (TestToken(cursor, CppTokens::EXPR_SIZEOF))
	{
		// sizeof(EXPR)
		// sizeof TYPE
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
		// ++ a
		// -- a
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
		// ~a
		// !a
		// +a
		// -a
		// &a
		// *a
		auto newExpr = MakePtr<PrefixUnaryExpr>();
		FillOperatorAndSkip(newExpr->opName, cursor, 1);
		FillOperator(newExpr->opName, newExpr->op);
		newExpr->operand = ParsePrefixUnaryExpr(pa, cursor);
		return newExpr;
	}
	else if (TestToken(cursor, CppTokens::NEW))
	{
		// new
		return ParseNewExpr(pa, false, cursor);
	}
	else if (TestToken(cursor, CppTokens::DELETE))
	{
		// delete
		return ParseDeleteExpr(pa, false, cursor);
	}
	else
	{
		{
			auto oldCursor = cursor;
			if (TestToken(cursor, CppTokens::COLON, CppTokens::COLON))
			{
				if (TestToken(cursor, CppTokens::NEW))
				{
					// ::new
					return ParseNewExpr(pa, true, cursor);
				}
				else if (TestToken(cursor, CppTokens::DELETE))
				{
					// ::delete
					return ParseDeleteExpr(pa, true, cursor);
				}
				else
				{
					cursor = oldCursor;
				}
			}
		}
		{
			auto oldCursor = cursor;
			try
			{
				// (TYPE)EXPR
				RequireToken(cursor, CppTokens::LPARENTHESIS);
				auto type = ParseType(pa, cursor);
				RequireToken(cursor, CppTokens::RPARENTHESIS);
				auto expr = ParsePrefixUnaryExpr(pa, cursor);

				auto newExpr = MakePtr<CastExpr>();
				newExpr->castType = CppCastType::CCast;
				newExpr->type = type;
				newExpr->expr = expr;
				return newExpr;
			}
			catch (const StopParsingException&)
			{
				cursor = oldCursor;
			}
		}
		return ParsePostfixUnaryExpr(pa, cursor);
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
			// a .* b
			FillOperatorAndSkip(opName, cursor, 2);
			precedence = 4;
		}
		else if (TestToken(cursor, CppTokens::SUB, CppTokens::GT, CppTokens::MUL, false))
		{
			// a ->* b
			FillOperatorAndSkip(opName, cursor, 3);
			precedence = 4;
		}
		else if (TestToken(cursor, CppTokens::MUL, false) && !TestToken(cursor, CppTokens::MUL, CppTokens::EQ, false))
		{
			// a * b
			FillOperatorAndSkip(opName, cursor, 1);
			precedence = 5;
		}
		else if (TestToken(cursor, CppTokens::DIV, false) && !TestToken(cursor, CppTokens::DIV, CppTokens::EQ, false))
		{
			// a / b
			FillOperatorAndSkip(opName, cursor, 1);
			precedence = 5;
		}
		else if (TestToken(cursor, CppTokens::PERCENT, false) && !TestToken(cursor, CppTokens::PERCENT, CppTokens::EQ, false))
		{
			// a % b
			FillOperatorAndSkip(opName, cursor, 1);
			precedence = 5;
		}
		else if (TestToken(cursor, CppTokens::ADD, false) && !TestToken(cursor, CppTokens::ADD, CppTokens::EQ, false))
		{
			// a + b
			FillOperatorAndSkip(opName, cursor, 1);
			precedence = 6;
		}
		else if (TestToken(cursor, CppTokens::SUB, false) && !TestToken(cursor, CppTokens::SUB, CppTokens::EQ, false))
		{
			// a - b
			FillOperatorAndSkip(opName, cursor, 1);
			precedence = 6;
		}
		else if (TestToken(cursor, CppTokens::LT, CppTokens::LT, false) && !TestToken(cursor, CppTokens::LT, CppTokens::LT, CppTokens::EQ, false))
		{
			// a << b
			FillOperatorAndSkip(opName, cursor, 2);
			precedence = 7;
		}
		else if (pea.allowGt && TestToken(cursor, CppTokens::GT, CppTokens::GT, false) && !TestToken(cursor, CppTokens::GT, CppTokens::GT, CppTokens::EQ, false))
		{
			// a >> b
			FillOperatorAndSkip(opName, cursor, 2);
			precedence = 7;
		}
		else if (TestToken(cursor, CppTokens::LT, CppTokens::EQ, false) || TestToken(cursor, CppTokens::GT, CppTokens::EQ, false))
		{
			// a <= b
			// a >= b
			FillOperatorAndSkip(opName, cursor, 2);
			precedence = 8;
		}
		else if (TestToken(cursor, CppTokens::LT, false) && !TestToken(cursor, CppTokens::LT, CppTokens::LT, CppTokens::EQ, false))
		{
			// a < b
			FillOperatorAndSkip(opName, cursor, 1);
			precedence = 8;
		}
		else if (pea.allowGt && TestToken(cursor, CppTokens::GT, false) && !TestToken(cursor, CppTokens::GT, CppTokens::GT, CppTokens::EQ, false))
		{
			// a > b
			FillOperatorAndSkip(opName, cursor, 1);
			precedence = 8;
		}
		else if (TestToken(cursor, CppTokens::EQ, CppTokens::EQ, false) || TestToken(cursor, CppTokens::NOT, CppTokens::EQ, false))
		{
			// a == b
			// a != b
			FillOperatorAndSkip(opName, cursor, 2);
			precedence = 9;
		}
		else if (TestToken(cursor, CppTokens::OR, CppTokens::OR, false))
		{
			// a || b
			FillOperatorAndSkip(opName, cursor, 2);
			precedence = 14;
		}
		else if (TestToken(cursor, CppTokens::OR, false) && !TestToken(cursor, CppTokens::OR, CppTokens::EQ, false))
		{
			// a | b
			FillOperatorAndSkip(opName, cursor, 1);
			precedence = 12;
		}
		else if (TestToken(cursor, CppTokens::AND, CppTokens::AND, false))
		{
			// a && b
			FillOperatorAndSkip(opName, cursor, 2);
			precedence = 13;
		}
		else if (TestToken(cursor, CppTokens::AND, false) && !TestToken(cursor, CppTokens::AND, CppTokens::EQ, false))
		{
			// a & b
			FillOperatorAndSkip(opName, cursor, 1);
			precedence = 10;
		}
		else if (TestToken(cursor, CppTokens::XOR, false) && !TestToken(cursor, CppTokens::XOR, CppTokens::EQ, false))
		{
			// a ^ b
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
		popped = newExpr;
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
		// a ? b : c
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

	// a ASSIGN_OP b
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
		// throw
		// throw EXPR
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