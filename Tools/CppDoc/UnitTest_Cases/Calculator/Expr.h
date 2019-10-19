#pragma once

#include <intrin.h>

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

	template<typename T>
	class Ptr
	{
	private:
		T*				reference = nullptr;

		void			Inc();
		void			Dec();
	public:
		Ptr()
		{
		}

		Ptr(T* _reference)
			:reference(_reference)
		{
			Inc();
		}

		Ptr(const Ptr<T>& ptr)
			:reference(ptr.reference)
		{
			Inc();
		}

		Ptr(Ptr<T>&& ptr)
			:reference(ptr.reference)
		{
			ptr.reference = nullptr;
		}

		~Ptr()
		{
			Dec();
		}

		Ptr& operator=(const Ptr& ptr)
		{
			if (this == &ptr) return *this;
			Dec();
			reference = ptr.reference;
			Inc();
			return *this;
		}

		Ptr& operator=(Ptr&& ptr)
		{
			if (this == &ptr) return *this;
			Dec();
			reference = ptr.reference;
			ptr.reference = nullptr;
			return *this;
		}

		T* operator->() const
		{
			return reference;
		}
	};

	template<typename X>
	void Ptr<X>::Inc()
	{
		if (reference)
		{
			_InterlockedIncrement(&reference->counter);
		}
	}

	template<typename X>
	void Ptr<X>::Dec()
	{
		if (reference)
		{
			if (_InterlockedDecrement(&reference->counter) == 0)
			{
				delete reference;
				reference = nullptr;
			}
		}
	}

	class Expr
	{
	private:
		friend class Ptr<Expr>;
		volatile long		counter = 0;
	public:
		using Ptr = Ptr<Expr>;

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