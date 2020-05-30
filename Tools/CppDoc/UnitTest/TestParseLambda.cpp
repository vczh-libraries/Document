#include "Util.h"
#include <Ast_Expr.h>
#include <Ast_Stat.h>

static int a1 = 0;
static int a2 = 0;

TEST_FILE
{
	TEST_CATEGORY(L"Parsing lambda expressions")
	{
		auto input = LR"(
void f()
{
	auto a = []{};
	auto b = [](){};
	auto c = [=, *this, a](int x, int y)->void { return; };
	auto d = [&, this, &a](int x, int y)mutable throw(char, bool) noexcept(noexcept(1))->double { int z=1; return z; };
}
)";
		auto output = LR"(
f: void ()
{
	a: auto = [] auto () {};
	b: auto = [] auto () {};
	c: auto = [=, *this, a] (auto->void) (x: int, y: int) {
		return;
	};
	d: auto = [&, this, &a] (auto->double) (x: int, y: int) noexcept throw(char, bool) {
		z: int = 1;
		return z;
	};
}
)";
		AssertProgram(input, output);
	});

	TEST_CATEGORY(L"Re-index")
	{
		// TODO:
	});

	TEST_CATEGORY(L"Capturing int")
	{
		auto input = LR"(
int a1 = 0;
struct S
{
	int a2 = 0;
	void F()
	{
		int b = 0;
		int c = 0;
		int d = 0;

		volatile int& b2 = 0;
		volatile int& c2 = 0;
		volatile int& d2 = 0;

		const volatile int&& b3 = 0;
		const volatile int&& c3 = 0;
		const volatile int&& d3 = 0;

		auto l1 = [c, c2, c3, &d, &d2, &d3]		(int e)			{};
		auto l2 = [c, c2, c3, &d, &d2, &d3]		(int e)mutable	{};
		auto l3 = [=, *this, &d, &d2, &d3]		(int e)			{};
		auto l4 = [=, *this, &d, &d2, &d3]		(int e)mutable	{};
		auto l5 = [&, this, c, c2, c3]			(int e)			{};
		auto l6 = [&, this, c, c2, c3]			(int e)mutable	{};
	}
};
)";
		COMPILE_PROGRAM(program, ppa, input);

		auto stat = ppa.root
			->TryGetChildren_NFb(L"S")->Get(0).childSymbol
			->TryGetChildren_NFb(L"F")->Get(0).childSymbol->GetImplSymbols_F()[0]
			->TryGetChildren_NFb(L"$")->Get(0).childSymbol
			->GetStat_N().Cast<BlockStat>();

		int b = 0;
		int c = 0;
		int d = 0;

		volatile int& b2 = (volatile int&)a1;
		volatile int& c2 = (volatile int&)a1;
		volatile int& d2 = (volatile int&)a1;

		const volatile int&& b3 = (const volatile int&&)a2;
		const volatile int&& c3 = (const volatile int&&)a2;
		const volatile int&& d3 = (const volatile int&&)a2;

		{
			auto symbol = stat->symbol
				->TryGetChildren_NFb(L"l1")->Get(0).childSymbol
				->GetAnyForwardDecl<VariableDeclaration>()->initializer->arguments[0].item.Cast<LambdaExpr>()->symbol
				->TryGetChildren_NFb(L"$")->Get(0).childSymbol;
			auto pa = ppa.AdjustForDecl(symbol.Obj());
			auto test = [c, c2, c3, &d, &d2, &d3, &pa](int e)
			{
				(void)(a1, a2, c, d, c2, d2, c3, d3, e);
				ASSERT_OVERLOADING_SIMPLE_LVALUE((a1), __int32 &);
				ASSERT_OVERLOADING_SIMPLE_LVALUE((a2), __int32 &);
				ASSERT_OVERLOADING_SIMPLE_LVALUE((c), __int32 const &);
				ASSERT_OVERLOADING_SIMPLE_LVALUE((d), __int32 &);
				ASSERT_OVERLOADING_SIMPLE_LVALUE((c2), __int32 const volatile &);
				ASSERT_OVERLOADING_SIMPLE_LVALUE((d2), __int32 volatile &);
				ASSERT_OVERLOADING_SIMPLE_LVALUE((c3), __int32 const volatile &);
				ASSERT_OVERLOADING_SIMPLE_LVALUE((d3), __int32 const volatile &);
				ASSERT_OVERLOADING_SIMPLE_LVALUE((e), __int32 &);
			};
			TEST_CATEGORY(L"l1")
			{
				test(0);
			});
		}
		{
			auto symbol = stat->symbol
				->TryGetChildren_NFb(L"l2")->Get(0).childSymbol
				->GetAnyForwardDecl<VariableDeclaration>()->initializer->arguments[0].item.Cast<LambdaExpr>()->symbol
				->TryGetChildren_NFb(L"$")->Get(0).childSymbol;
			auto pa = ppa.AdjustForDecl(symbol.Obj());
			auto test = [c, c2, c3, &d, &d2, &d3, &pa](int e)mutable
			{
				(void)(a1, a2, c, d, c2, d2, c3, d3, e);
				ASSERT_OVERLOADING_SIMPLE_LVALUE((a1), __int32 &);
				ASSERT_OVERLOADING_SIMPLE_LVALUE((a2), __int32 &);
				ASSERT_OVERLOADING_SIMPLE_LVALUE((c), __int32 &);
				ASSERT_OVERLOADING_SIMPLE_LVALUE((d), __int32 &);
				ASSERT_OVERLOADING_SIMPLE_LVALUE((c2), __int32 volatile &);
				ASSERT_OVERLOADING_SIMPLE_LVALUE((d2), __int32 volatile &);
				ASSERT_OVERLOADING_SIMPLE_LVALUE((c3), __int32 const volatile &);
				ASSERT_OVERLOADING_SIMPLE_LVALUE((d3), __int32 const volatile &);
				ASSERT_OVERLOADING_SIMPLE_LVALUE((e), __int32 &);
			};
			TEST_CATEGORY(L"l2")
			{
				test(0);
			});
		}
		{
			auto symbol = stat->symbol
				->TryGetChildren_NFb(L"l3")->Get(0).childSymbol
				->GetAnyForwardDecl<VariableDeclaration>()->initializer->arguments[0].item.Cast<LambdaExpr>()->symbol
				->TryGetChildren_NFb(L"$")->Get(0).childSymbol;
			auto pa = ppa.AdjustForDecl(symbol.Obj());
			auto test = [=, &d, &d2, &d3, &pa](int e)
			{
				(void)(a1, a2, b, c, d, b2, c2, d2, b3, c3, d3, e);
				ASSERT_OVERLOADING_SIMPLE_LVALUE((a1), __int32 &);
				ASSERT_OVERLOADING_SIMPLE_LVALUE((a2), __int32 &);
				ASSERT_OVERLOADING_SIMPLE_LVALUE((b), __int32 const &);
				ASSERT_OVERLOADING_SIMPLE_LVALUE((c), __int32 const &);
				ASSERT_OVERLOADING_SIMPLE_LVALUE((d), __int32 &);
				ASSERT_OVERLOADING_SIMPLE_LVALUE((b2), __int32 const volatile &);
				ASSERT_OVERLOADING_SIMPLE_LVALUE((c2), __int32 const volatile &);
				ASSERT_OVERLOADING_SIMPLE_LVALUE((d2), __int32 volatile &);
				ASSERT_OVERLOADING_SIMPLE_LVALUE((b3), __int32 const volatile &);
				ASSERT_OVERLOADING_SIMPLE_LVALUE((c3), __int32 const volatile &);
				ASSERT_OVERLOADING_SIMPLE_LVALUE((d3), __int32 const volatile &);
				ASSERT_OVERLOADING_SIMPLE_LVALUE((e), __int32 &);
			};
			TEST_CATEGORY(L"l3")
			{
				test(0);
			});
		}
		{
			auto symbol = stat->symbol
				->TryGetChildren_NFb(L"l4")->Get(0).childSymbol
				->GetAnyForwardDecl<VariableDeclaration>()->initializer->arguments[0].item.Cast<LambdaExpr>()->symbol
				->TryGetChildren_NFb(L"$")->Get(0).childSymbol;
			auto pa = ppa.AdjustForDecl(symbol.Obj());
			auto test = [=, &d, &d2, &d3, &pa](int e)mutable
			{
				(void)(a1, a2, b, c, d, b2, c2, d2, b3, c3, d3, e);
				ASSERT_OVERLOADING_SIMPLE_LVALUE((a1), __int32 &);
				ASSERT_OVERLOADING_SIMPLE_LVALUE((a2), __int32 &);
				ASSERT_OVERLOADING_SIMPLE_LVALUE((b), __int32 &);
				ASSERT_OVERLOADING_SIMPLE_LVALUE((c), __int32 &);
				ASSERT_OVERLOADING_SIMPLE_LVALUE((d), __int32 &);
				ASSERT_OVERLOADING_SIMPLE_LVALUE((b2), __int32 volatile &);
				ASSERT_OVERLOADING_SIMPLE_LVALUE((c2), __int32 volatile &);
				ASSERT_OVERLOADING_SIMPLE_LVALUE((d2), __int32 volatile &);
				ASSERT_OVERLOADING_SIMPLE_LVALUE((b3), __int32 const volatile &);
				ASSERT_OVERLOADING_SIMPLE_LVALUE((c3), __int32 const volatile &);
				ASSERT_OVERLOADING_SIMPLE_LVALUE((d3), __int32 const volatile &);
				ASSERT_OVERLOADING_SIMPLE_LVALUE((e), __int32 &);
			};
			TEST_CATEGORY(L"l4")
			{
				test(0);
			});
		}
		{
			auto symbol = stat->symbol
				->TryGetChildren_NFb(L"l5")->Get(0).childSymbol
				->GetAnyForwardDecl<VariableDeclaration>()->initializer->arguments[0].item.Cast<LambdaExpr>()->symbol
				->TryGetChildren_NFb(L"$")->Get(0).childSymbol;
			auto pa = ppa.AdjustForDecl(symbol.Obj());
			auto test = [&, c, c2, c3](int e)
			{
				(void)(a1, a2, b, c, d, b2, c2, d2, b3, c3, d3, e);
				ASSERT_OVERLOADING_SIMPLE_LVALUE((a1), __int32 &);
				ASSERT_OVERLOADING_SIMPLE_LVALUE((a2), __int32 &);
				ASSERT_OVERLOADING_SIMPLE_LVALUE((b), __int32 &);
				ASSERT_OVERLOADING_SIMPLE_LVALUE((c), __int32 const &);
				ASSERT_OVERLOADING_SIMPLE_LVALUE((d), __int32 &);
				ASSERT_OVERLOADING_SIMPLE_LVALUE((b2), __int32 volatile &);
				ASSERT_OVERLOADING_SIMPLE_LVALUE((c2), __int32 const volatile &);
				ASSERT_OVERLOADING_SIMPLE_LVALUE((d2), __int32 volatile &);
				ASSERT_OVERLOADING_SIMPLE_LVALUE((b3), __int32 const volatile &);
				ASSERT_OVERLOADING_SIMPLE_LVALUE((c3), __int32 const volatile &);
				ASSERT_OVERLOADING_SIMPLE_LVALUE((d3), __int32 const volatile &);
				ASSERT_OVERLOADING_SIMPLE_LVALUE((e), __int32 &);
			};
			TEST_CATEGORY(L"l5")
			{
				test(0);
			});
		}
		{
			auto symbol = stat->symbol
				->TryGetChildren_NFb(L"l6")->Get(0).childSymbol
				->GetAnyForwardDecl<VariableDeclaration>()->initializer->arguments[0].item.Cast<LambdaExpr>()->symbol
				->TryGetChildren_NFb(L"$")->Get(0).childSymbol;
			auto pa = ppa.AdjustForDecl(symbol.Obj());
			auto test = [&, c, c2, c3](int e)mutable
			{
				(void)(a1, a2, b, c, d, b2, c2, d2, b3, c3, d3, e);
				ASSERT_OVERLOADING_SIMPLE_LVALUE((a1), __int32 &);
				ASSERT_OVERLOADING_SIMPLE_LVALUE((a2), __int32 &);
				ASSERT_OVERLOADING_SIMPLE_LVALUE((b), __int32 &);
				ASSERT_OVERLOADING_SIMPLE_LVALUE((c), __int32 &);
				ASSERT_OVERLOADING_SIMPLE_LVALUE((d), __int32 &);
				ASSERT_OVERLOADING_SIMPLE_LVALUE((b2), __int32 volatile &);
				ASSERT_OVERLOADING_SIMPLE_LVALUE((c2), __int32 volatile &);
				ASSERT_OVERLOADING_SIMPLE_LVALUE((d2), __int32 volatile &);
				ASSERT_OVERLOADING_SIMPLE_LVALUE((b3), __int32 const volatile &);
				ASSERT_OVERLOADING_SIMPLE_LVALUE((c3), __int32 const volatile &);
				ASSERT_OVERLOADING_SIMPLE_LVALUE((d3), __int32 const volatile &);
				ASSERT_OVERLOADING_SIMPLE_LVALUE((e), __int32 &);
			};
			TEST_CATEGORY(L"l6")
			{
				test(0);
			});
		}
	});

	TEST_CATEGORY(L"Capturing int in nested lambda")
	{
		// TODO:
	});
}