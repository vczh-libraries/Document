#include "Util.h"

namespace Input__TestParsePSValueAlias_FullInstantiation
{
	TEST_DECL(
		template<typename A, typename... Ts> struct FuncType;

		template<typename A, typename B, typename... Ts>
		constexpr auto Value = false;

		template<typename A, typename B, typename... Ts>
		constexpr auto Value<A*, B*, Ts...> = 'c';

		template<typename A, typename B, typename... Ts>
		constexpr auto Value<A, B, Ts*...> = L'c';

		template<typename A, typename B, typename... Ts>
		constexpr auto Value<A*, B*, Ts*...> = 0.f;

		template<typename A, typename B, typename C, typename D>
		constexpr auto Value<A*, B*, C*, D*> = 0.0;

		template<typename A, typename B>
		constexpr auto Value<char*, wchar_t*, A*, B*> = (char*)nullptr;

		template<typename A, typename B>
		constexpr auto Value<A*, B*, float*, double*> = (wchar_t*)nullptr;

		template<>
		constexpr auto Value<char*, wchar_t*, float*, double*> = (bool*)nullptr;

		template<typename A, typename B>
		constexpr auto Value<A(*)(float), B(*)(double)> = (char**)nullptr;

		template<typename A, typename B>
		constexpr auto Value<char(*)(A), wchar_t(*)(B)> = (wchar_t**)nullptr;

		template<>
		constexpr auto Value<char(*)(float), wchar_t(*)(double)> = (bool**)nullptr;

		template<typename A, typename B>
		constexpr auto Value<A(*)(B), A(*)(B), A(*)(B), A(*)(B)> = (float**)nullptr;

		template<typename A, typename B>
		constexpr auto Value<A(*)(B), B(*)(A), A(*)(A), B(*)(B)> = (double**)nullptr;

		template<typename A, typename B, typename... Ts>
		constexpr auto Value<A(*)(Ts...), B(*)(Ts...), A(*)(B(*...bs)(Ts))> = (void**)nullptr;

		template<typename A, typename B>
		constexpr auto Value<FuncType<A, float>, FuncType<B, double>> = (char**)nullptr;

		template<typename A, typename B>
		constexpr auto Value<FuncType<char, A>, FuncType<wchar_t, B>> = (wchar_t**)nullptr;

		template<>
		constexpr auto Value<FuncType<char, float>, FuncType<wchar_t, double>> = (bool**)nullptr;

		template<typename A, typename B>
		constexpr auto Value<FuncType<A, B>, FuncType<A, B>, FuncType<A, B>, FuncType<A, B>> = (float**)nullptr;

		template<typename A, typename B>
		constexpr auto Value<FuncType<A, B>, FuncType<B, A>, FuncType<A, A>, FuncType<B, B>> = (double**)nullptr;

		template<typename A, typename B, typename... Ts>
		constexpr auto Value<FuncType<A, Ts...>, FuncType<B, Ts...>, FuncType<A, FuncType<B, Ts>...>> = (void**)nullptr;
	);
}

TEST_FILE
{
	TEST_CATEGORY(L"Full instantiation")
	{
		using namespace Input__TestParsePSValueAlias_FullInstantiation;
		COMPILE_PROGRAM(program, pa, input);

		AssertExpr(pa, L"Value",		L"Value",				L"<::Value::[A], ::Value::[B], ...::Value::[Ts]> any_t $PR"	);

		ASSERT_OVERLOADING_SIMPLE_LVALUE((Value<bool, void, int>),										bool const &		);
		ASSERT_OVERLOADING_SIMPLE_LVALUE((Value<char, wchar_t, int>),									bool const &		);
		ASSERT_OVERLOADING_SIMPLE_LVALUE((Value<bool *, void *, char, wchar_t>),						char const &		);
		ASSERT_OVERLOADING_SIMPLE_LVALUE((Value<char, wchar_t>),										wchar_t const &		);
		ASSERT_OVERLOADING_SIMPLE_LVALUE((Value<bool, void, char *, wchar_t *>),						wchar_t const &		);
		ASSERT_OVERLOADING_SIMPLE_LVALUE((Value<bool *, void *>),										float const &		);
		ASSERT_OVERLOADING_SIMPLE_LVALUE((Value<bool *, void *, char *, wchar_t *, int *>),				float const &		);
		ASSERT_OVERLOADING_SIMPLE_LVALUE((Value<bool *, void *, char *, wchar_t *>),					double const &		);
		ASSERT_OVERLOADING_SIMPLE_LVALUE((Value<char *, wchar_t *, bool *, void *>),					char * const &		);
		ASSERT_OVERLOADING_SIMPLE_LVALUE((Value<bool *, void *, float *, double *>),					wchar_t * const &	);
		ASSERT_OVERLOADING_SIMPLE_LVALUE((Value<char *, wchar_t *, float *, double *>),					bool * const &		);

		ASSERT_OVERLOADING_LVALUE(
			(Value<float(*)(float), double(*)(double)>),
			L"(Value<float (float) *, double (double) *>)",
			char * * const &
		);
		ASSERT_OVERLOADING_LVALUE(
			(Value<char(*)(char), wchar_t(*)(wchar_t)>),
			L"(Value<char (char) *, wchar_t (wchar_t) *>)",
			wchar_t * * const &
		);
		ASSERT_OVERLOADING_LVALUE(
			(Value<char(*)(float), wchar_t(*)(double)>),
			L"(Value<char (float) *, wchar_t (double) *>)",
			bool * * const &
		);
		ASSERT_OVERLOADING_LVALUE(
			(Value<char(*)(float), char(*)(float), char(*)(float), char(*)(float)>),
			L"(Value<char (float) *, char (float) *, char (float) *, char (float) *>)",
			float * * const &
		);
		ASSERT_OVERLOADING_LVALUE(
			(Value<char(*)(float), float(*)(char), char(*)(char), float(*)(float)>),
			L"(Value<char (float) *, float (char) *, char (char) *, float (float) *>)",
			double * * const &
		);
		ASSERT_OVERLOADING_LVALUE(
			(Value<char(*)(float, double), wchar_t(*)(float, double), char(*)(wchar_t(*)(float), wchar_t(*)(double))>),
			L"(Value<char (float, double) *, wchar_t (float, double) *, char (wchar_t (float) *, wchar_t (double) *) *>)",
			void * * const &
		);

		ASSERT_OVERLOADING_SIMPLE_LVALUE(
			(Value<FuncType<float, float>, FuncType<double, double>>),
			char * * const &
		);
		ASSERT_OVERLOADING_SIMPLE_LVALUE(
			(Value<FuncType<char, char>, FuncType<wchar_t, wchar_t>>),
			wchar_t * * const &
		);
		ASSERT_OVERLOADING_SIMPLE_LVALUE(
			(Value<FuncType<char, float>, FuncType<wchar_t, double>>),
			bool * * const &
		);
		ASSERT_OVERLOADING_SIMPLE_LVALUE(
			(Value<FuncType<char, float>, FuncType<char, float>, FuncType<char, float>, FuncType<char, float>>),
			float * * const &
		);
		ASSERT_OVERLOADING_SIMPLE_LVALUE(
			(Value<FuncType<char, float>, FuncType<float, char>, FuncType<char, char>, FuncType<float, float>>),
			double * * const &
		);
		ASSERT_OVERLOADING_SIMPLE_LVALUE(
			(Value<FuncType<char, float, double>, FuncType<wchar_t, float, double>, FuncType<char, FuncType<wchar_t, float>, FuncType<wchar_t, double>>>),
			void * * const &
		);
	});

	TEST_CATEGORY(L"Full instantiation in template class")
	{
		// TODO:
	});

	TEST_CATEGORY(L"Partial instantiation in class")
	{
		// TODO:
	});
}