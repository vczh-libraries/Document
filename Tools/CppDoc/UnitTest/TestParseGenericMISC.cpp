#include "Util.h"

TEST_FILE
{
	TEST_CATEGORY(L"Default template argument on forward declarations")
	{
		auto input = LR"(
template<typename T, typename U>
struct Class
{
	static const U Value = nullptr;
};

template<typename T, typename U>
U Function()
{
	return nullptr;
}

template<typename A, typename B = A*>
struct Class;

template<typename A, typename B = A*>
B Function();
)";
		COMPILE_PROGRAM(program, pa, input);

		{
			auto classSymbol = pa.root->TryGetChildren_NFb(L"Class")->Get(0).childSymbol;
			TEST_CASE_ASSERT(classSymbol->GetForwardDecls_N().Count() == 1);
			TEST_CASE_ASSERT(classSymbol->GetImplDecl_NFb());
		}
		{
			auto classSymbol = pa.root->TryGetChildren_NFb(L"Function")->Get(0).childSymbol;
			TEST_CASE_ASSERT(classSymbol->GetForwardSymbols_F().Count() == 1);
			TEST_CASE_ASSERT(classSymbol->GetImplSymbols_F().Count() == 1);
		}
	});

	TEST_CATEGORY(L"Function type as template argument")
	{
		auto input = LR"(
template<typename T>
struct S
{
	static const T Value;
};

template<typename R, typename... Ps>
struct S<R(Ps...)>
{
	static const R Value;
};
)";
		COMPILE_PROGRAM(program, pa, input);

		AssertExpr(pa,	L"S<int>::Value",		L"S<int> :: Value",		L"__int32 const $L");
		AssertExpr(pa,	L"S<int()>::Value",		L"S<int ()> :: Value",	L"__int32 const $L");
	});
}