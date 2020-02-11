#include "Util.h"

namespace Input__TestOverloadingGenericFunction_Method
{
	TEST_DECL
	(
		template<typename... Ts>
		struct MakeTuple
		{
			template<typename T>
			MakeTuple<Ts..., T> operator<<(T);
		};
	);
}

TEST_FILE
{
	TEST_CATEGORY(L"Generic methods (simple)")
	{
		using namespace Input__TestOverloadingGenericFunction_Method;
		COMPILE_PROGRAM(program, pa, input);

		ASSERT_OVERLOADING_VERBOSE(
			(MakeTuple<>() << 1),
			L"((MakeTuple<>() << 1))",
			L"::Tuple<{__int32 $PR}> $PR",
			MakeTuple<int>
		);

		ASSERT_OVERLOADING_VERBOSE(
			(MakeTuple<>() << 1 << 1. << 1.f << 'x' << L'x' << true),
			L"(((((((MakeTuple<>() << 1) << 1.) << 1.f) << 'x') << L'x') << true))",
			L"::Tuple<{__int32 $PR, double $PR, float $PR, char $PR, wchar_t $PR, bool $PR}> $PR",
			MakeTuple<int, double, float, char, wchar_t, bool>
		);
	});
}