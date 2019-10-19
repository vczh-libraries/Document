#include <stdio.h>
#include "Expr.h"

namespace calculator
{
#define EXPR_ACCEPT_VISITOR_IMPL(TYPE)\
	void TYPE::Accept(IExprVisitor* visitor)\
	{\
		visitor->Visit(this);\
	}\

	EXPR_TYPES(EXPR_ACCEPT_VISITOR_IMPL)
#undef EXPR_ACCEPT_VISITOR_IMPL

/***********************************************************************
Evaluation
***********************************************************************/

	class PrintVisitor : public IExprVisitor
	{
	public:
		void Visit(NumberExpr* self) override
		{
			printf("%g", self->number);
		}

		void Visit(UnaryExpr* self) override
		{
			switch (self->op)
			{
			case UnaryOperator::Neg: printf("-"); break;
			}
			self->operand->Accept(this);
		}

		void Visit(BinaryExpr* self) override
		{
			printf("(");
			self->left->Accept(this);
			switch (self->op)
			{
			case BinaryOperator::Add: printf("+"); break;
			case BinaryOperator::Sub: printf("-"); break;
			case BinaryOperator::Mul: printf("*"); break;
			case BinaryOperator::Div: printf("/"); break;
			}
			self->right->Accept(this);
			printf(")");
		}
	};

	void Print(const Expr::Ptr& expr)
	{
		PrintVisitor visitor;
		expr->Accept(&visitor);
	}

/***********************************************************************
Evaluation
***********************************************************************/

	class EvaluateVisitor : public IExprVisitor
	{
	public:
		double			result;

		void Visit(NumberExpr* self) override
		{
			result = self->number;
		}

		void Visit(UnaryExpr* self) override
		{
			self->operand->Accept(this);
			switch (self->op)
			{
			case UnaryOperator::Neg: result = -result; break;
			}
			result = -result;
		}

		void Visit(BinaryExpr* self) override
		{
			double left = (self->left->Accept(this), result);
			double right = (self->right->Accept(this), result);
			switch (self->op)
			{
			case BinaryOperator::Add: result = left + right; break;
			case BinaryOperator::Sub: result = left - right; break;
			case BinaryOperator::Mul: result = left * right; break;
			case BinaryOperator::Div: result = left / right; break;
			}
		}
	};

	double Evaluate(const Expr::Ptr& expr)
	{
		EvaluateVisitor visitor;
		expr->Accept(&visitor);
		return visitor.result;
	}
}