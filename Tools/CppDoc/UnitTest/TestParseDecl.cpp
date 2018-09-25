#include <Parser.h>
#include <Ast_Decl.h>

extern Ptr<RegexLexer>		GlobalCppLexer();
extern void					Log(Ptr<Declaration> decl, StreamWriter& writer);
extern void					Log(Ptr<Program> program, StreamWriter& writer);

void AssertProgram(const WString& type, const WString& log)
{
	CppTokenReader reader(GlobalCppLexer(), type);
	auto cursor = reader.GetFirstToken();

	ParsingArguments pa(new Symbol, nullptr);
	auto program = ParseProgram(pa, cursor);
	TEST_ASSERT(!cursor);

	auto output = GenerateToStream([&](StreamWriter& writer)
	{
		Log(program, writer);
	});

	StringReader srExpect(log);
	StringReader srActual(L"\r\n" + output);

	while (true)
	{
		TEST_ASSERT(srExpect.IsEnd() == srActual.IsEnd());
		if (srExpect.IsEnd()) break;
		
		auto expect = srExpect.ReadLine();
		auto actual = srActual.ReadLine();
		TEST_ASSERT(expect == actual);
	}
}

TEST_CASE(TestParseDecl_Namespaces)
{
	auto input = LR"(
namespace vl {}
namespace vl::presentation::controls {}
)";
	auto output = LR"(
namespace vl
{
}
namespace vl
{
	namespace presentation
	{
		namespace controls
		{
		}
	}
}
)";
	AssertProgram(input, output);
}

TEST_CASE(TestParseDecl_Enums)
{
	auto input = LR"(
enum e1;
enum e2 : int;
enum class e3;
enum class e4 : int;
enum e5
{
	x,
	y = 1
};
enum class e6
{
	x,
	y,
	z,
};
)";
	auto output = LR"(
__forward enum e1;
__forward enum e2 : int;
__forward enum class e3;
__forward enum class e4 : int;
enum e5
{
	x,
	y = 1,
};
enum class e6
{
	x,
	y,
	z,
};
)";
	AssertProgram(input, output);
}

TEST_CASE(TestParseDecl_Variables)
{
	auto input = LR"(
int x = 0;
int a, b = 0, c(0), d{0};
extern static mutable thread_local register int (*v1)();
)";
	auto output = LR"(
x: int = 0;
a: int;
b: int = 0;
c: int (0);
d: int {0};
__forward extern mutable register static thread_local v1: int () *;
)";
	AssertProgram(input, output);
}

TEST_CASE(TestParseDecl_Functions)
{
	auto input = LR"(
int Add(int a, int b);
extern friend static virtual explicit inline __forceinline int __stdcall Sub(int, int);
)";
	auto output = LR"(
__forward Add: int (a: int, b: int);
__forward explicit extern friend inline __forceinline static virtual Sub: int (int, int) __stdcall;
)";
	AssertProgram(input, output);
}