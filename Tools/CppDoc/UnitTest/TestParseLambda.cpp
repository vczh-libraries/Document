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
	c: auto = [=, *this] (auto->void) (x: int, y: int) {
		return;
	};
	d: auto = [&, this] (auto->double) (x: int, y: int) noexcept throw(char, bool) {
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

	TEST_CATEGORY(L"Capturing")
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

		auto l1 = [c, &d]			(int e)			{};
		auto l2 = [c, &d]			(int e)mutable	{};
		auto l3 = [=, *this, c, &d]	(int e)			{};
		auto l4 = [=, *this, c, &d]	(int e)mutable	{};
		auto l5 = [&, this, c, &d]	(int e)			{};
		auto l6 = [&, this, c, &d]	(int e)mutable	{};
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

		{
			auto symbol = stat->symbol
				->TryGetChildren_NFb(L"l1")->Get(0).childSymbol
				->GetAnyForwardDecl<VariableDeclaration>()->initializer->arguments[0].item.Cast<LambdaExpr>()->symbol
				->TryGetChildren_NFb(L"$")->Get(0).childSymbol;
			auto pa = ppa.AdjustForDecl(symbol.Obj());
			auto test = [c, &d, &pa](int e)
			{
				(void)(a1, a2, c, d, e);
				ASSERT_OVERLOADING_SIMPLE_LVALUE(a1, __int32);
				ASSERT_OVERLOADING_SIMPLE_LVALUE(a2, __int32);
				ASSERT_OVERLOADING_SIMPLE_LVALUE(c, __int32);
				ASSERT_OVERLOADING_SIMPLE_LVALUE(d, __int32 &);
				ASSERT_OVERLOADING_SIMPLE_LVALUE(e, __int32);

				ASSERT_OVERLOADING_SIMPLE_LVALUE((a1), __int32 &);
				ASSERT_OVERLOADING_SIMPLE_LVALUE((a2), __int32 &);
				ASSERT_OVERLOADING_SIMPLE_LVALUE((c), __int32 const &);
				ASSERT_OVERLOADING_SIMPLE_LVALUE((d), __int32 &);
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
			auto test = [c, &d, &pa](int e)mutable
			{
				(void)(a1, a2, c, d, e);
				ASSERT_OVERLOADING_SIMPLE_LVALUE(a1, __int32);
				ASSERT_OVERLOADING_SIMPLE_LVALUE(a2, __int32);
				ASSERT_OVERLOADING_SIMPLE_LVALUE(c, __int32);
				ASSERT_OVERLOADING_SIMPLE_LVALUE(d, __int32 &);
				ASSERT_OVERLOADING_SIMPLE_LVALUE(e, __int32);

				ASSERT_OVERLOADING_SIMPLE_LVALUE((a1), __int32 &);
				ASSERT_OVERLOADING_SIMPLE_LVALUE((a2), __int32 &);
				ASSERT_OVERLOADING_SIMPLE_LVALUE((c), __int32 &);
				ASSERT_OVERLOADING_SIMPLE_LVALUE((d), __int32 &);
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
			auto test = [=, &d, &pa](int e)
			{
				(void)(a1, a2, b, c, d, e);
				ASSERT_OVERLOADING_SIMPLE_LVALUE(a1, __int32);
				ASSERT_OVERLOADING_SIMPLE_LVALUE(a2, __int32);
				ASSERT_OVERLOADING_SIMPLE_LVALUE(b, __int32);
				ASSERT_OVERLOADING_SIMPLE_LVALUE(c, __int32);
				ASSERT_OVERLOADING_SIMPLE_LVALUE(d, __int32 &);
				ASSERT_OVERLOADING_SIMPLE_LVALUE(e, __int32);

				ASSERT_OVERLOADING_SIMPLE_LVALUE((a1), __int32 &);
				ASSERT_OVERLOADING_SIMPLE_LVALUE((a2), __int32 &);
				ASSERT_OVERLOADING_SIMPLE_LVALUE((b), __int32 const &);
				ASSERT_OVERLOADING_SIMPLE_LVALUE((c), __int32 const &);
				ASSERT_OVERLOADING_SIMPLE_LVALUE((d), __int32 &);
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
			auto test = [=, &d, &pa](int e)mutable
			{
				(void)(a1, a2, b, c, d, e);
				ASSERT_OVERLOADING_SIMPLE_LVALUE(a1, __int32);
				ASSERT_OVERLOADING_SIMPLE_LVALUE(a2, __int32);
				ASSERT_OVERLOADING_SIMPLE_LVALUE(b, __int32);
				ASSERT_OVERLOADING_SIMPLE_LVALUE(c, __int32);
				ASSERT_OVERLOADING_SIMPLE_LVALUE(d, __int32 &);
				ASSERT_OVERLOADING_SIMPLE_LVALUE(e, __int32);

				ASSERT_OVERLOADING_SIMPLE_LVALUE((a1), __int32 &);
				ASSERT_OVERLOADING_SIMPLE_LVALUE((a2), __int32 &);
				ASSERT_OVERLOADING_SIMPLE_LVALUE((b), __int32 &);
				ASSERT_OVERLOADING_SIMPLE_LVALUE((c), __int32 &);
				ASSERT_OVERLOADING_SIMPLE_LVALUE((d), __int32 &);
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
			auto test = [&, c](int e)
			{
				(void)(a1, a2, b, c, d, e);
				ASSERT_OVERLOADING_SIMPLE_LVALUE(a1, __int32);
				ASSERT_OVERLOADING_SIMPLE_LVALUE(a2, __int32);
				ASSERT_OVERLOADING_SIMPLE_LVALUE(b, __int32 &);
				ASSERT_OVERLOADING_SIMPLE_LVALUE(c, __int32);
				ASSERT_OVERLOADING_SIMPLE_LVALUE(d, __int32 &);
				ASSERT_OVERLOADING_SIMPLE_LVALUE(e, __int32);

				ASSERT_OVERLOADING_SIMPLE_LVALUE((a1), __int32 &);
				ASSERT_OVERLOADING_SIMPLE_LVALUE((a2), __int32 &);
				ASSERT_OVERLOADING_SIMPLE_LVALUE((b), __int32 &);
				ASSERT_OVERLOADING_SIMPLE_LVALUE((c), __int32 const &);
				ASSERT_OVERLOADING_SIMPLE_LVALUE((d), __int32 &);
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
			auto test = [&, c](int e)mutable
			{
				(void)(a1, a2, b, c, d, e);
				ASSERT_OVERLOADING_SIMPLE_LVALUE(a1, __int32);
				ASSERT_OVERLOADING_SIMPLE_LVALUE(a2, __int32);
				ASSERT_OVERLOADING_SIMPLE_LVALUE(b, __int32 &);
				ASSERT_OVERLOADING_SIMPLE_LVALUE(c, __int32);
				ASSERT_OVERLOADING_SIMPLE_LVALUE(d, __int32 &);
				ASSERT_OVERLOADING_SIMPLE_LVALUE(e, __int32);

				ASSERT_OVERLOADING_SIMPLE_LVALUE((a1), __int32 &);
				ASSERT_OVERLOADING_SIMPLE_LVALUE((a2), __int32 &);
				ASSERT_OVERLOADING_SIMPLE_LVALUE((b), __int32 &);
				ASSERT_OVERLOADING_SIMPLE_LVALUE((c), __int32 &);
				ASSERT_OVERLOADING_SIMPLE_LVALUE((d), __int32 &);
				ASSERT_OVERLOADING_SIMPLE_LVALUE((e), __int32 &);
			};
			TEST_CATEGORY(L"l6")
			{
				test(0);
			});
		}
	});
}