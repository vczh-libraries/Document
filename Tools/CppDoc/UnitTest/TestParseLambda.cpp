#include "Util.h"

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
		// TODO:
	});
}