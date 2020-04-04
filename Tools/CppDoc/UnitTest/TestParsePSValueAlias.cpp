#include "Util.h"

namespace Input__TestParsePSValueAlias_FullInstantiation
{
	TEST_DECL(
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

		// TODO: matching functions and generic types
		// TODO: matching patterns that requires retry
	);
}

TEST_FILE
{
	TEST_CATEGORY(L"Full instantiation")
	{
		using namespace Input__TestParsePSValueAlias_FullInstantiation;
		COMPILE_PROGRAM(program, pa, input);

		ASSERT_OVERLOADING_SIMPLE((Value<bool, void, int>),										bool const &		);
		ASSERT_OVERLOADING_SIMPLE((Value<char, wchar_t, int>),									bool const &		);
		ASSERT_OVERLOADING_SIMPLE((Value<bool *, void *, char, wchar_t>),						char const &		);
		ASSERT_OVERLOADING_SIMPLE((Value<char, wchar_t>),										wchar_t const &		);
		ASSERT_OVERLOADING_SIMPLE((Value<bool, void, char *, wchar_t *>),						wchar_t const &		);
		ASSERT_OVERLOADING_SIMPLE((Value<bool *, void*>),										float const &		);
		ASSERT_OVERLOADING_SIMPLE((Value<bool *, void *, char *, wchar_t *, int *>),			float const &		);
		ASSERT_OVERLOADING_SIMPLE((Value<bool *, void *, char *, wchar_t *>),					double const &		);
		ASSERT_OVERLOADING_SIMPLE((Value<char *, wchar_t *, bool *, void *>),					char * const &		);
		ASSERT_OVERLOADING_SIMPLE((Value<bool *, void *, float *, double *>),					wchar_t * const &	);
		ASSERT_OVERLOADING_SIMPLE((Value<char *, wchar_t *, float *, double *>),				bool * const &		);
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