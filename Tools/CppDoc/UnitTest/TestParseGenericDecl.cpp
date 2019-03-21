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

template<typename T>
using Size = sizeof(T);

template<typename T>
using Ctor = T();
)";
		auto output = LR"(
template<typename T>
using LRef = T &;
template<typename T>
using RRef = T &&;
template<typename T>
using Size = sizeof(T);
template<typename T>
using Ctor = T();
)";
		COMPILE_PROGRAM(program, pa, input);
		AssertProgram(program, output);

		AssertType(L"LRef",				L"LRef",				L"<::LRef::typename[T]> ::LRef::typename[T] &",		pa);
		AssertType(L"RRef",				L"RRef",				L"<::RRef::typename[T]> ::RRef::typename[T] &&",	pa);
		AssertExpr(L"Size",				L"Size",				L"<::Size::typename[T]> unsigned __int32 $PR",		pa);
		AssertExpr(L"Ctor",				L"Ctor",				L"<::Ctor::typename[T]> ::Ctor::typename[T] $PR",	pa);
	}
}