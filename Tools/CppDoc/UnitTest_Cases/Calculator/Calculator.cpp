#include <assert.h>
#include <stdlib.h>
#include "Calculator.h"

namespace calculator
{
	Expr::Ptr ParseExpr(const char*& input);

	Expr::Ptr ParseTerm(const char*& input)
	{
		if (*input == '(')
		{
			input++;
			auto expr = ParseExpr(input);
			assert(*input == ')');
			input++;
			return expr;
		}
		else if (*input == '-')
		{
			input++;
			auto expr = new UnaryExpr;
			expr->op = UnaryOperator::Neg;
			expr->operand = ParseTerm(input);
			return expr;
		}
		else
		{
			auto expr = new NumberExpr;
			expr->number = strtod(input, (char**)&input);
			return expr;
		}
	}

	Expr::Ptr ParseFactor(const char*& input)
	{
		auto expr = ParseTerm(input);
		while (*input == '*' || *input == '/')
		{
			auto binaryExpr = new BinaryExpr;
			binaryExpr->left = expr;
			binaryExpr->op = *input++ == '*' ? BinaryOperator::Mul : BinaryOperator::Div;
			binaryExpr->right = ParseTerm(input);
			expr = binaryExpr;
		}
		return expr;
	}

	Expr::Ptr ParseExpr(const char*& input)
	{
		auto expr = ParseTerm(input);
		while (*input == '+' || *input == '-')
		{
			auto binaryExpr = new BinaryExpr;
			binaryExpr->left = expr;
			binaryExpr->op = *input++ == '+' ? BinaryOperator::Add : BinaryOperator::Sub;
			binaryExpr->right = ParseTerm(input);
			expr = binaryExpr;
		}
		return expr;
	}

	Expr::Ptr Parse(const char* input)
	{
		auto expr = ParseExpr(input);
		assert(!*input);
		return expr;
	}
}