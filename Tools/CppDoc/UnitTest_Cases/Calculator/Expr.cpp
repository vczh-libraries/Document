#include <intrin.h>
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

	void Expr::Ptr::Inc()
	{
		if (expr)
		{
			_InterlockedIncrement(&expr->counter);
		}
	}

	void Expr::Ptr::Dec()
	{
		if (expr)
		{
			if (_InterlockedDecrement(&expr->counter) == 0)
			{
				delete expr;
				expr = nullptr;
			}
		}
	}

	Expr::Ptr::Ptr()
	{
		Inc();
	}

	Expr::Ptr::Ptr(Expr* _expr)
		:expr(_expr)
	{
		Inc();
	}

	Expr::Ptr::Ptr(const Ptr& ptr)
		:expr(ptr.expr)
	{
		Inc();
	}

	Expr::Ptr::Ptr(Ptr&& ptr)
		:expr(ptr.expr)
	{
		ptr.expr = nullptr;
	}

	Expr::Ptr::~Ptr()
	{
		Dec();
	}

	Expr::Ptr& Expr::Ptr::operator=(const Ptr& ptr)
	{
		if (this == &ptr) return *this;
		Dec();
		expr = ptr.expr;
		Inc();
		return *this;
	}

	Expr::Ptr& Expr::Ptr::operator=(Ptr&& ptr)
	{
		if (this == &ptr) return *this;
		Dec();
		expr = ptr.expr;
		ptr.expr = nullptr;
		return *this;
	}

/***********************************************************************
Evaluation
***********************************************************************/

	class PrintVisitor : public IExprVisitor
	{
	public:
		void Visit(NumberExpr* self) override
		{
			printf("%f", self->number);
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
			case BinaryOperator::Add:result = left + right; break;
			case BinaryOperator::Sub:result = left - right; break;
			case BinaryOperator::Mul:result = left * right; break;
			case BinaryOperator::Div:result = left / right; break;
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