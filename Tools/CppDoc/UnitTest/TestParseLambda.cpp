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
		auto input = LR"(
int f(int a, int x)
{
	return [=](int b){ return a + b; }(x);
}
)";

		SortedList<vint> accessed;
		auto recorder = BEGIN_ASSERT_SYMBOL
			ASSERT_SYMBOL			(0, L"a", 3, 27, VariableDeclaration, 1, 10)
			ASSERT_SYMBOL			(1, L"b", 3, 31, VariableDeclaration, 3, 16)
			ASSERT_SYMBOL			(2, L"x", 3, 36, VariableDeclaration, 1, 17)
		END_ASSERT_SYMBOL;

		COMPILE_PROGRAM_WITH_RECORDER(program, pa, input, recorder);
		TEST_CASE_ASSERT(accessed.Count() == 3);
	});

	auto getInsideLambda = [](Ptr<BlockStat> stat, const WString& name)
	{
		return stat->symbol
			->TryGetChildren_NFb(name)->Get(0).childSymbol
			->GetAnyForwardDecl<VariableDeclaration>()->initializer
			->arguments[0].item.Cast<LambdaExpr>()->statement
			.Cast<BlockStat>();
	};

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
			auto symbol = getInsideLambda(stat, L"l1")->symbol;
			auto pa = ppa.AdjustForDecl(symbol);
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
			auto symbol = getInsideLambda(stat, L"l2")->symbol;
			auto pa = ppa.AdjustForDecl(symbol);
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
			auto symbol = getInsideLambda(stat, L"l3")->symbol;
			auto pa = ppa.AdjustForDecl(symbol);
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
			auto symbol = getInsideLambda(stat, L"l4")->symbol;
			auto pa = ppa.AdjustForDecl(symbol);
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
			auto symbol = getInsideLambda(stat, L"l5")->symbol;
			auto pa = ppa.AdjustForDecl(symbol);
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
			auto symbol = getInsideLambda(stat, L"l6")->symbol;
			auto pa = ppa.AdjustForDecl(symbol);
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
		auto input = LR"(
int a1 = 0;
struct S
{
	int a2 = 0;
	void F()
	{
		int b = 0;
		volatile int& c = 0;
		const volatile int&& d = 0;

		auto l1 = [=](int e)
		{
			auto m1 = [=](int f)		{};
			auto m2 = [=](int f)mutable	{};
			auto m3 = [&](int f)		{};
		};

		auto l2 = [=](int e)mutable
		{
			auto m1 = [=](int f)		{};
			auto m2 = [=](int f)mutable	{};
			auto m3 = [&](int f)		{};
		};

		auto l3 = [&](int e)
		{
			auto m1 = [=](int f)		{};
			auto m2 = [=](int f)mutable	{};
			auto m3 = [&](int f)		{};
		};
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
		volatile int& c = (volatile int&)b;
		const volatile int&& d = (const volatile int&&)b;

		{
			auto statl = getInsideLambda(stat, L"l1");
			auto testl = [=, &ppa](int e)
			{
				{
					auto statm = getInsideLambda(statl, L"m1");
					auto pa = ppa.AdjustForDecl(statm->symbol);
					auto testm = [=, &pa](int f)
					{
						(void)(b, c, d, e, f);
						ASSERT_OVERLOADING_SIMPLE_LVALUE((b), __int32 const &);
						ASSERT_OVERLOADING_SIMPLE_LVALUE((c), __int32 const volatile &);
						ASSERT_OVERLOADING_SIMPLE_LVALUE((d), __int32 const volatile &);
						ASSERT_OVERLOADING_SIMPLE_LVALUE((e), __int32 const &);
						ASSERT_OVERLOADING_SIMPLE_LVALUE((f), __int32 &);
					};
					TEST_CATEGORY(L"m1")
					{
						testm(0);
					});
				}
				{
					auto statm = getInsideLambda(statl, L"m2");
					auto pa = ppa.AdjustForDecl(statm->symbol);
					auto testm = [=, &pa](int f)mutable
					{
						(void)(b, c, d, e, f);
						ASSERT_OVERLOADING_SIMPLE_LVALUE((b), __int32 &);
						ASSERT_OVERLOADING_SIMPLE_LVALUE((c), __int32 volatile &);
						ASSERT_OVERLOADING_SIMPLE_LVALUE((d), __int32 const volatile &);
						ASSERT_OVERLOADING_SIMPLE_LVALUE((e), __int32 &);
						ASSERT_OVERLOADING_SIMPLE_LVALUE((f), __int32 &);
					};
					TEST_CATEGORY(L"m2")
					{
						testm(0);
					});
				}
				{
					auto statm = getInsideLambda(statl, L"m3");
					auto pa = ppa.AdjustForDecl(statm->symbol);
					auto testm = [&](int f)
					{
						(void)(b, c, d, e, f);
						ASSERT_OVERLOADING_SIMPLE_LVALUE((b), __int32 const &);
						ASSERT_OVERLOADING_SIMPLE_LVALUE((c), __int32 const volatile &);
						ASSERT_OVERLOADING_SIMPLE_LVALUE((d), __int32 const volatile &);
						ASSERT_OVERLOADING_SIMPLE_LVALUE((e), __int32 &);
						ASSERT_OVERLOADING_SIMPLE_LVALUE((f), __int32 &);
					};
					TEST_CATEGORY(L"m3")
					{
						testm(0);
					});
				}
			};
			TEST_CATEGORY(L"l1")
			{
				testl(0);
			});
		}
		{
			auto statl = getInsideLambda(stat, L"l2");
			auto testl = [=, &ppa](int e)mutable
			{
				{
					auto statm = getInsideLambda(statl, L"m1");
					auto pa = ppa.AdjustForDecl(statm->symbol);
					auto testm = [=, &pa](int f)
					{
						(void)(b, c, d, e, f);
						ASSERT_OVERLOADING_SIMPLE_LVALUE((b), __int32 const &);
						ASSERT_OVERLOADING_SIMPLE_LVALUE((c), __int32 const volatile &);
						ASSERT_OVERLOADING_SIMPLE_LVALUE((d), __int32 const volatile &);
						ASSERT_OVERLOADING_SIMPLE_LVALUE((e), __int32 const &);
						ASSERT_OVERLOADING_SIMPLE_LVALUE((f), __int32 &);
					};
					TEST_CATEGORY(L"m1")
					{
						testm(0);
					});
				}
				{
					auto statm = getInsideLambda(statl, L"m2");
					auto pa = ppa.AdjustForDecl(statm->symbol);
					auto testm = [=, &pa](int f)mutable
					{
						(void)(b, c, d, e, f);
						ASSERT_OVERLOADING_SIMPLE_LVALUE((b), __int32 &);
						ASSERT_OVERLOADING_SIMPLE_LVALUE((c), __int32 volatile &);
						ASSERT_OVERLOADING_SIMPLE_LVALUE((d), __int32 const volatile &);
						ASSERT_OVERLOADING_SIMPLE_LVALUE((e), __int32 &);
						ASSERT_OVERLOADING_SIMPLE_LVALUE((f), __int32 &);
					};
					TEST_CATEGORY(L"m2")
					{
						testm(0);
					});
				}
				{
					auto statm = getInsideLambda(statl, L"m3");
					auto pa = ppa.AdjustForDecl(statm->symbol);
					auto testm = [&](int f)
					{
						(void)(b, c, d, e, f);
						ASSERT_OVERLOADING_SIMPLE_LVALUE((b), __int32 &);
						ASSERT_OVERLOADING_SIMPLE_LVALUE((c), __int32 volatile &);
						ASSERT_OVERLOADING_SIMPLE_LVALUE((d), __int32 const volatile &);
						ASSERT_OVERLOADING_SIMPLE_LVALUE((e), __int32 &);
						ASSERT_OVERLOADING_SIMPLE_LVALUE((f), __int32 &);
					};
					TEST_CATEGORY(L"m3")
					{
						testm(0);
					});
				}
			};
			TEST_CATEGORY(L"l2")
			{
				testl(0);
			});
		}
		{
			auto statl = getInsideLambda(stat, L"l3");
			auto testl = [&](int e)
			{
				{
					auto statm = getInsideLambda(statl, L"m1");
					auto pa = ppa.AdjustForDecl(statm->symbol);
					auto testm = [=, &pa](int f)
					{
						(void)(b, c, d, e, f);
						ASSERT_OVERLOADING_SIMPLE_LVALUE((b), __int32 const &);
						ASSERT_OVERLOADING_SIMPLE_LVALUE((c), __int32 const volatile &);
						ASSERT_OVERLOADING_SIMPLE_LVALUE((d), __int32 const volatile &);
						ASSERT_OVERLOADING_SIMPLE_LVALUE((e), __int32 const &);
						ASSERT_OVERLOADING_SIMPLE_LVALUE((f), __int32 &);
					};
					TEST_CATEGORY(L"m1")
					{
						testm(0);
					});
				}
				{
					auto statm = getInsideLambda(statl, L"m2");
					auto pa = ppa.AdjustForDecl(statm->symbol);
					auto testm = [=, &pa](int f)mutable
					{
						(void)(b, c, d, e, f);
						ASSERT_OVERLOADING_SIMPLE_LVALUE((b), __int32 &);
						ASSERT_OVERLOADING_SIMPLE_LVALUE((c), __int32 volatile &);
						ASSERT_OVERLOADING_SIMPLE_LVALUE((d), __int32 const volatile &);
						ASSERT_OVERLOADING_SIMPLE_LVALUE((e), __int32 &);
						ASSERT_OVERLOADING_SIMPLE_LVALUE((f), __int32 &);
					};
					TEST_CATEGORY(L"m2")
					{
						testm(0);
					});
				}
				{
					auto statm = getInsideLambda(statl, L"m3");
					auto pa = ppa.AdjustForDecl(statm->symbol);
					auto testm = [&](int f)
					{
						(void)(b, c, d, e, f);
						ASSERT_OVERLOADING_SIMPLE_LVALUE((b), __int32 &);
						ASSERT_OVERLOADING_SIMPLE_LVALUE((c), __int32 volatile &);
						ASSERT_OVERLOADING_SIMPLE_LVALUE((d), __int32 const volatile &);
						ASSERT_OVERLOADING_SIMPLE_LVALUE((e), __int32 &);
						ASSERT_OVERLOADING_SIMPLE_LVALUE((f), __int32 &);
					};
					TEST_CATEGORY(L"m3")
					{
						testm(0);
					});
				}
			};
			TEST_CATEGORY(L"l3")
			{
				testl(0);
			});
		}
	});

	TEST_CATEGORY(L"Capturing expression")
	{
		auto input = LR"(
struct S
{
	S* t;
};

void F()
{
	auto l1 = [s = S(), t = s.t]
	{
	};

	auto l1 = [s = S(), t = s.t]mutable
	{
	};
}
)";
		COMPILE_PROGRAM(program, ppa, input);

		auto statF = ppa.root
			->TryGetChildren_NFb(L"F")->Get(0).childSymbol->GetImplSymbols_F()[0]
			->TryGetChildren_NFb(L"$")->Get(0).childSymbol
			->GetStat_N().Cast<BlockStat>();
		{
			auto lambdaSymbol = getInsideLambda(statF, L"l")->symbol;
			auto pa = ppa.AdjustForDecl(lambdaSymbol);

			AssertExpr(pa,	L"s",		L"s",		L"::S const $L"		);
			AssertExpr(pa,	L"t",		L"t",		L"::S * const $L"	);
		}
		{
			auto lambdaSymbol = getInsideLambda(statF, L"l")->symbol;
			auto pa = ppa.AdjustForDecl(lambdaSymbol);

			AssertExpr(pa,	L"s",		L"s",		L"::S $L"			);
			AssertExpr(pa,	L"t",		L"t",		L"::S * $L"			);
		}
	});
}