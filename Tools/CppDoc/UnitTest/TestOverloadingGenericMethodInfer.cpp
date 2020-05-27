#include "Util.h"

namespace Input__TestOverloadingGenericMethodInfer_Method
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

			template<typename... Us>
			MakeTuple<Us...> ExtractFrom(MakeTuple<Ts..., Us...>) { return {}; }
		};

		template<typename T, typename... Ts>
		struct MakeTuple2
		{
			MakeTuple<T, Ts...> mt1;

			template<typename... Us>
			auto operator+(MakeTuple2<Us...> mt2) { return mt1 + mt2.mt1; }

			template<typename U, typename... Us>
			auto With(MakeTuple2<U, Us...> mt2) { return mt1.With(mt2.mt1); }

			template<typename... Us>
			auto ExtractFrom(MakeTuple2<T, Ts..., Us...> mt2) { return mt1.ExtractFrom(mt2.mt1); }
		};

		auto mt1 = MakeTuple<>()['a'][true][1.0];
		auto mt2 = ++++++MakeTuple<>();
		auto pt1 = &mt1;

		auto _mt1 = MakeTuple2<char, bool, double>();
		auto _mt2 = MakeTuple2<int, int, int>();
		auto _pt1 = &_mt1;
	);
}

TEST_FILE
{
	TEST_CATEGORY(L"Variable types")
	{
		using namespace Input__TestOverloadingGenericMethodInfer_Method;
		COMPILE_PROGRAM(program, pa, input);

		AssertExpr(pa,	L"mt1",		L"mt1",		L"::MakeTuple<{char $PR, bool $PR, double $PR}> $L"			);
		AssertExpr(pa,	L"mt2",		L"mt2",		L"::MakeTuple<{__int32 $PR, __int32 $PR, __int32 $PR}> $L"	);
		AssertExpr(pa,	L"pt1",		L"pt1",		L"::MakeTuple<{char $PR, bool $PR, double $PR}> * $L"		);

		AssertExpr(pa,	L"_mt1",	L"_mt1",	L"::MakeTuple2<char, {bool $PR, double $PR}> $L"			);
		AssertExpr(pa,	L"_mt2",	L"_mt2",	L"::MakeTuple2<__int32, {__int32 $PR, __int32 $PR}> $L"		);
		AssertExpr(pa,	L"_pt1",	L"_pt1",	L"::MakeTuple2<char, {bool $PR, double $PR}> * $L"			);
	});

	TEST_CATEGORY(L"Generic operators")
	{
		using namespace Input__TestOverloadingGenericMethodInfer_Method;
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
		using namespace Input__TestOverloadingGenericMethodInfer_Method;
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

		ASSERT_OVERLOADING_FORMATTED_VERBOSE(
			mt1.ExtractFrom(MakeTuple<char, bool, double, int, float, void>()),
			L"::MakeTuple<{__int32 $PR, float $PR, void $PR}> $PR",
			MakeTuple<int, float, void>
		);

		ASSERT_OVERLOADING_FORMATTED_VERBOSE(
			pt1->ExtractFrom(MakeTuple<char, bool, double, int, float, void>()),
			L"::MakeTuple<{__int32 $PR, float $PR, void $PR}> $PR",
			MakeTuple<int, float, void>
		);
	});

	TEST_CATEGORY(L"Generic this")
	{
		using namespace Input__TestOverloadingGenericMethodInfer_Method;
		COMPILE_PROGRAM(program, pa, input);

		ASSERT_OVERLOADING_VERBOSE(
			_mt1 + _mt2,
			L"(_mt1 + _mt2)",
			L"::MakeTuple<{char $PR, bool $PR, double $PR, __int32 $PR, __int32 $PR, __int32 $PR}> $PR",
			MakeTuple<char, bool, double, int, int, int>
		);

		ASSERT_OVERLOADING_FORMATTED_VERBOSE(
			_mt1.With(_mt2),
			L"::MakeTuple<{char $PR, bool $PR, double $PR, __int32 $PR, __int32 $PR, __int32 $PR}> $PR",
			MakeTuple<char, bool, double, int, int, int>
		);

		ASSERT_OVERLOADING_FORMATTED_VERBOSE(
			_pt1->With(_mt2),
			L"::MakeTuple<{char $PR, bool $PR, double $PR, __int32 $PR, __int32 $PR, __int32 $PR}> $PR",
			MakeTuple<char, bool, double, int, int, int>
		);

		ASSERT_OVERLOADING_FORMATTED_VERBOSE(
			_mt1.ExtractFrom(MakeTuple2<char, bool, double, int, float, void>()),
			L"::MakeTuple<{__int32 $PR, float $PR, void $PR}> $PR",
			MakeTuple<int, float, void>
		);

		ASSERT_OVERLOADING_FORMATTED_VERBOSE(
			_pt1->ExtractFrom(MakeTuple2<char, bool, double, int, float, void>()),
			L"::MakeTuple<{__int32 $PR, float $PR, void $PR}> $PR",
			MakeTuple<int, float, void>
		);
	});
}