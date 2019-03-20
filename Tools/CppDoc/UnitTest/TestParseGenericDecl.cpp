#include <Ast_Decl.h>
#include "Util.h"

TEST_CASE(TestParseGenericDecl_TypeAlias)
{
	{
		auto input = LR"(
template<typename T>
using LRef = T&;

template<typename T>
using RRef = T&&;
)";
		auto output = LR"(
template<typename T>
using LRef = T &;
template<typename T>
using RRef = T &&;
)";
		COMPILE_PROGRAM(program, pa, input);
		AssertProgram(program, output);

		AssertType(L"LRef",				L"LRef",				L"<0:t> __arg<0> &",			pa);
		AssertType(L"RRef",				L"RRef",				L"<0:t> __arg<0> &&",			pa);
	}
}