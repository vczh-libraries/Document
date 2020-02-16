#include "Util.h"

namespace Input__TestOverloadingGenericFunction_Method
{
	TEST_DECL
	(
		template<typename... Ts>
		struct MakeTuple
		{
			MakeTuple<int, Ts...> operator++() { return {}; }

			MakeTuple<Ts..., double> operator++(int) { return {}; }

			template<typename T>
			MakeTuple<Ts..., T> operator<<(T) { return {}; }

			template<typename T>
			MakeTuple<Ts..., T> operator[](T) { return {}; }

			template<typename... Us>
			MakeTuple<Ts..., Us...> operator+(MakeTuple<Us...>) { return {}; }

			template<typename... Us>
			MakeTuple<Ts..., Us...> With(MakeTuple<Us...>) { return {}; }
		};

		auto mt1 = MakeTuple<>()['a'][true][1.0];
		auto mt2 = ++++++MakeTuple<>();
		auto pt1 = &mt1;
	);
}

TEST_FILE
{
	TEST_CATEGORY(L"Generic operators")
	{
		using namespace Input__TestOverloadingGenericFunction_Method;
		COMPILE_PROGRAM(program, pa, input);

		ASSERT_OVERLOADING_VERBOSE(
			(++MakeTuple<>()),
			L"((++ MakeTuple<>()))",
			L"::MakeTuple<{__int32 $PR}> $PR",
			MakeTuple<int>
		);

		ASSERT_OVERLOADING_VERBOSE(
			(++++++MakeTuple<>()),
			L"((++ (++ (++ MakeTuple<>()))))",
			L"::MakeTuple<{__int32 $PR, __int32 $PR, __int32 $PR}> $PR",
			MakeTuple<int, int, int>
		);

		ASSERT_OVERLOADING_VERBOSE(
			(MakeTuple<>()++),
			L"((MakeTuple<>() ++))",
			L"::MakeTuple<{double $PR}> $PR",
			MakeTuple<double>
		);

		ASSERT_OVERLOADING_VERBOSE(
			(MakeTuple<>()++++++),
			L"((((MakeTuple<>() ++) ++) ++))",
			L"::MakeTuple<{double $PR, double $PR, double $PR}> $PR",
			MakeTuple<double, double, double>
		);

		ASSERT_OVERLOADING_VERBOSE(
			(MakeTuple<>() << 1),
			L"((MakeTuple<>() << 1))",
			L"::MakeTuple<{__int32 $PR}> $PR",
			MakeTuple<int>
		);

		ASSERT_OVERLOADING_VERBOSE(
			(MakeTuple<>() << 1 << 1. << 1.f << 'x' << L'x' << true),
			L"(((((((MakeTuple<>() << 1) << 1.) << 1.f) << 'x') << L'x') << true))",
			L"::MakeTuple<{__int32 $PR, double $PR, float $PR, char $PR, wchar_t $PR, bool $PR}> $PR",
			MakeTuple<int, double, float, char, wchar_t, bool>
		);

		ASSERT_OVERLOADING_VERBOSE(
			(MakeTuple<>()[0]),
			L"(MakeTuple<>()[0])",
			L"::MakeTuple<{__int32 $PR}> $PR",
			MakeTuple<int>
		);

		ASSERT_OVERLOADING_VERBOSE(
			(MakeTuple<>()[0][1][2]),
			L"(MakeTuple<>()[0][1][2])",
			L"::MakeTuple<{__int32 $PR, __int32 $PR, __int32 $PR}> $PR",
			MakeTuple<int, int, int>
		);

		ASSERT_OVERLOADING_VERBOSE(
			(MakeTuple<>()['a'][true][1.0] + ++++++MakeTuple<>()),
			L"((MakeTuple<>()['a'][true][1.0] + (++ (++ (++ MakeTuple<>())))))",
			L"::MakeTuple<{char $PR, bool $PR, double $PR, __int32 $PR, __int32 $PR, __int32 $PR}> $PR",
			MakeTuple<char, bool, double, int, int, int>
		);
	});
	TEST_CATEGORY(L"Generic methods")
	{
		using namespace Input__TestOverloadingGenericFunction_Method;
		COMPILE_PROGRAM(program, pa, input);

		ASSERT_OVERLOADING_FORMATTED_VERBOSE(
			mt1.With(mt2),
			L"::MakeTuple<{char $PR, bool $PR, double $PR, __int32 $PR, __int32 $PR, __int32 $PR}> $PR",
			MakeTuple<char, bool, double, int, int, int>
		);

		ASSERT_OVERLOADING_FORMATTED_VERBOSE(
			pt1->With(mt2),
			L"::MakeTuple<{char $PR, bool $PR, double $PR, __int32 $PR, __int32 $PR, __int32 $PR}> $PR",
			MakeTuple<char, bool, double, int, int, int>
		);

		ASSERT_OVERLOADING_FORMATTED_VERBOSE(
			mt1.operator +(mt2),
			L"::MakeTuple<{char $PR, bool $PR, double $PR, __int32 $PR, __int32 $PR, __int32 $PR}> $PR",
			MakeTuple<char, bool, double, int, int, int>
		);

		ASSERT_OVERLOADING_FORMATTED_VERBOSE(
			pt1->operator +(mt2),
			L"::MakeTuple<{char $PR, bool $PR, double $PR, __int32 $PR, __int32 $PR, __int32 $PR}> $PR",
			MakeTuple<char, bool, double, int, int, int>
		);
	});

	// test nullptr
	// use multiple levels of type arguments in function arguments
	// call methods/operators on "this"
}