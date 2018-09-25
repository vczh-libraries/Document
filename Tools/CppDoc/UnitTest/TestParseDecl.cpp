#include <Parser.h>
#include <Ast_Decl.h>

extern Ptr<RegexLexer>		GlobalCppLexer();
extern void					Log(Ptr<Declaration> decl, StreamWriter& writer);
extern void					Log(Ptr<Program> program, StreamWriter& writer);

#define COMPILE_PROGRAM(PROGRAM, PA,INPUT)\
	CppTokenReader reader(GlobalCppLexer(), INPUT);\
	auto cursor = reader.GetFirstToken();\
	ParsingArguments PA(new Symbol, nullptr);\
	auto PROGRAM = ParseProgram(PA, cursor);\
	TEST_ASSERT(!cursor)\

void AssertProgram(const WString& input, const WString& log)
{
	COMPILE_PROGRAM(program, pa, input);

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

TEST_CASE(TestParseDecl_EnumsConnectForward)
{
	auto input = LR"(
namespace a::b
{
	enum class A;
	enum class A;
}
namespace a::b
{
	enum class A {};
}
namespace a::b
{
	enum class A;
	enum class A;
}
)";
	COMPILE_PROGRAM(program, pa, input);
	TEST_ASSERT(pa.root->children[L"a"].Count() == 1);
	TEST_ASSERT(pa.root->children[L"a"][0]->children[L"b"].Count() == 1);
	TEST_ASSERT(pa.root->children[L"a"][0]->children[L"b"][0]->children[L"A"].Count() == 5);
	const auto& symbols = pa.root->children[L"a"][0]->children[L"b"][0]->children[L"A"];

	for (vint i = 0; i < 5; i++)
	{
		auto& symbol = symbols[i];
		if (i == 2)
		{
			TEST_ASSERT(symbol->isForwardDeclaration == false);
			TEST_ASSERT(symbol->forwardDeclarationRoot == nullptr);
			TEST_ASSERT(symbol->forwardDeclarations.Count() == 4);
			TEST_ASSERT(symbol->forwardDeclarations[0] == symbols[0].Obj());
			TEST_ASSERT(symbol->forwardDeclarations[1] == symbols[1].Obj());
			TEST_ASSERT(symbol->forwardDeclarations[2] == symbols[3].Obj());
			TEST_ASSERT(symbol->forwardDeclarations[3] == symbols[4].Obj());
		}
		else
		{
			TEST_ASSERT(symbol->isForwardDeclaration == true);
			TEST_ASSERT(symbol->forwardDeclarationRoot == symbols[2].Obj());
			TEST_ASSERT(symbol->forwardDeclarations.Count() == 0);
		}
	}
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

TEST_CASE(TestParseDecl_VariablesConnectForward)
{
	auto input = LR"(
namespace a::b
{
	extern int x;
	extern int x;
}
namespace a::b
{
	int x = 0;
}
namespace a::b
{
	extern int x;
	extern int x;
}
)";
	COMPILE_PROGRAM(program, pa, input);
	TEST_ASSERT(pa.root->children[L"a"].Count() == 1);
	TEST_ASSERT(pa.root->children[L"a"][0]->children[L"b"].Count() == 1);
	TEST_ASSERT(pa.root->children[L"a"][0]->children[L"b"][0]->children[L"x"].Count() == 5);
	const auto& symbols = pa.root->children[L"a"][0]->children[L"b"][0]->children[L"x"];

	for (vint i = 0; i < 5; i++)
	{
		auto& symbol = symbols[i];
		if (i == 2)
		{
			TEST_ASSERT(symbol->isForwardDeclaration == false);
			TEST_ASSERT(symbol->forwardDeclarationRoot == nullptr);
			TEST_ASSERT(symbol->forwardDeclarations.Count() == 4);
			TEST_ASSERT(symbol->forwardDeclarations[0] == symbols[0].Obj());
			TEST_ASSERT(symbol->forwardDeclarations[1] == symbols[1].Obj());
			TEST_ASSERT(symbol->forwardDeclarations[2] == symbols[3].Obj());
			TEST_ASSERT(symbol->forwardDeclarations[3] == symbols[4].Obj());
		}
		else
		{
			TEST_ASSERT(symbol->isForwardDeclaration == true);
			TEST_ASSERT(symbol->forwardDeclarationRoot == symbols[2].Obj());
			TEST_ASSERT(symbol->forwardDeclarations.Count() == 0);
		}
	}
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

TEST_CASE(TestParseDecl_FunctionsConnectForward)
{

}