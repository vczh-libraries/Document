#pragma once

#define EXPR_TYPES(F)\
	F(NumberExpr)\
	F(UnaryExpr)\
	F(BinaryExpr)\

#define EXPR_ACCEPT_VISITOR void Accept(IExprVisitor* visitor) override

namespace calculator
{
#define DEFINE_EXPR(TYPE) class TYPE;
	EXPR_TYPES(DEFINE_EXPR)
#undef DEFINE_EXPR

	class IExprVisitor
	{
	public:
		virtual ~IExprVisitor() = default;

#define VISIT_EXPR(TYPE) virtual void Visit(TYPE* self) = 0;
		EXPR_TYPES(VISIT_EXPR)
#undef VISIT_EXPR
	};

	class Expr
	{
	private:
		volatile long		counter = 0;
	public:
		class Ptr
		{
		private:
			Expr*			expr;

			void			Inc();
			void			Dec();
		public:
			Ptr();
			Ptr(Expr* _expr);
			Ptr(const Ptr& ptr);
			Ptr(Ptr&& ptr);
			~Ptr();

			Ptr&			operator=(const Ptr& ptr);
			Ptr&			operator=(Ptr&& ptr);

			Expr* operator->() const
			{
				return expr;
			}
		};

		virtual ~Expr() = default;
		virtual void		Accept(IExprVisitor* visitor) = 0;
	};

	class NumberExpr : public Expr
	{
	public:
		EXPR_ACCEPT_VISITOR;

		double				number;
	};

	enum class UnaryOperator
	{
		Neg,
	};

	class UnaryExpr : public Expr
	{
	public:
		EXPR_ACCEPT_VISITOR;

		UnaryOperator		op;
		Expr::Ptr			operand;
	};

	enum class BinaryOperator
	{
		Add,
		Sub,
		Mul,
		Div,
	};

	class BinaryExpr : public Expr
	{
	public:
		EXPR_ACCEPT_VISITOR;

		BinaryOperator		op;
		Expr::Ptr			left;
		Expr::Ptr			right;
	};

	extern void				Print(const Expr::Ptr& expr);
	extern double			Evaluate(const Expr::Ptr& expr);
}