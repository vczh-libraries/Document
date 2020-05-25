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

namespace Input__TestParsePSValueAlias_MemberInstantiation
{
	TEST_DECL(
		template<typename A, typename... Ts> struct FuncType;

		template<typename TFloat, typename TDouble>
		struct Struct
		{
			template<typename A, typename B, typename... Ts>
			static bool const Value = false;

			template<typename A, typename B, typename... Ts>
			static char const Value<A*, B*, Ts...> = 'c';

			template<typename A, typename B, typename... Ts>
			static wchar_t const Value<A, B, Ts*...> = L'c';

			template<typename A, typename B, typename... Ts>
			static float const Value<A*, B*, Ts*...> = 0.f;

			template<typename A, typename B, typename C, typename D>
			static double const Value<A*, B*, C*, D*> = 0.0;

			template<typename A, typename B>
			static char* const Value<char*, wchar_t*, A*, B*> = nullptr;

			template<typename A, typename B>
			static wchar_t* const Value<A*, B*, TFloat*, TDouble*> = nullptr;

			template<>
			static bool* const Value<char*, wchar_t*, TFloat*, TDouble*> = nullptr;

			template<typename A, typename B>
			static char** const Value<A(*)(TFloat), B(*)(TDouble)> = nullptr;

			template<typename A, typename B>
			static wchar_t** const Value<char(*)(A), wchar_t(*)(B)> = nullptr;

			template<>
			static bool** const Value<char(*)(TFloat), wchar_t(*)(TDouble)> = nullptr;

			template<typename A, typename B>
			static TFloat** const Value<A(*)(B), A(*)(B), A(*)(B), A(*)(B)> = nullptr;

			template<typename A, typename B>
			static TDouble** const Value<A(*)(B), B(*)(A), A(*)(A), B(*)(B)> = nullptr;

			template<typename A, typename B, typename... Ts>
			static void** const Value<A(*)(Ts...), B(*)(Ts...), A(*)(B(*...bs)(Ts))> = nullptr;

			template<typename A, typename B>
			static char** const Value<FuncType<A, TFloat>, FuncType<B, TDouble>> = nullptr;

			template<typename A, typename B>
			static wchar_t** const Value<FuncType<char, A>, FuncType<wchar_t, B>> = nullptr;

			template<>
			static bool** const Value<FuncType<char, TFloat>, FuncType<wchar_t, TDouble>> = nullptr;

			template<typename A, typename B>
			static TFloat** const Value<FuncType<A, B>, FuncType<A, B>, FuncType<A, B>, FuncType<A, B>> = nullptr;

			template<typename A, typename B>
			static TDouble** const Value<FuncType<A, B>, FuncType<B, A>, FuncType<A, A>, FuncType<B, B>> = nullptr;

			template<typename A, typename B, typename... Ts>
			static void** const Value<FuncType<A, Ts...>, FuncType<B, Ts...>, FuncType<A, FuncType<B, Ts>...>> = nullptr;
		};
	);
}

TEST_FILE
{
	TEST_CATEGORY(L"Full instantiation")
	{
		using namespace Input__TestParsePSValueAlias_FullInstantiation;
		COMPILE_PROGRAM(program, pa, input);

		AssertExpr(pa,
			L"Value",
			L"Value",
			L"<::Value::[A], ::Value::[B], ...::Value::[Ts]> any_t $PR"
		);

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
		using namespace Input__TestParsePSValueAlias_MemberInstantiation;
		COMPILE_PROGRAM(program, pa, input);

		AssertExpr(pa,
			L"Struct<float, double>::Value",
			L"Struct<float, double> :: Value",
			L"<::Struct::Value::[A], ::Struct::Value::[B], ...::Struct::Value::[Ts]> any_t $PR"
		);

		ASSERT_OVERLOADING_SIMPLE_LVALUE((Struct<float, double> :: Value<bool, void, int>),										bool const &		);
		ASSERT_OVERLOADING_SIMPLE_LVALUE((Struct<float, double> :: Value<char, wchar_t, int>),									bool const &		);
		ASSERT_OVERLOADING_SIMPLE_LVALUE((Struct<float, double> :: Value<bool *, void *, char, wchar_t>),						char const &		);
		ASSERT_OVERLOADING_SIMPLE_LVALUE((Struct<float, double> :: Value<char, wchar_t>),										wchar_t const &		);
		ASSERT_OVERLOADING_SIMPLE_LVALUE((Struct<float, double> :: Value<bool, void, char *, wchar_t *>),						wchar_t const &		);
		ASSERT_OVERLOADING_SIMPLE_LVALUE((Struct<float, double> :: Value<bool *, void *>),										float const &		);
		ASSERT_OVERLOADING_SIMPLE_LVALUE((Struct<float, double> :: Value<bool *, void *, char *, wchar_t *, int *>),				float const &		);
		ASSERT_OVERLOADING_SIMPLE_LVALUE((Struct<float, double> :: Value<bool *, void *, char *, wchar_t *>),					double const &		);
		ASSERT_OVERLOADING_SIMPLE_LVALUE((Struct<float, double> :: Value<char *, wchar_t *, bool *, void *>),					char * const &		);
		ASSERT_OVERLOADING_SIMPLE_LVALUE((Struct<float, double> :: Value<bool *, void *, float *, double *>),					wchar_t * const &	);
		ASSERT_OVERLOADING_SIMPLE_LVALUE((Struct<float, double> :: Value<char *, wchar_t *, float *, double *>),					bool * const &		);

		ASSERT_OVERLOADING_LVALUE(
			(Struct<float, double> :: Value<float(*)(float), double(*)(double)>),
			L"(Struct<float, double> :: Value<float (float) *, double (double) *>)",
			char * * const &
		);
		ASSERT_OVERLOADING_LVALUE(
			(Struct<float, double> :: Value<char(*)(char), wchar_t(*)(wchar_t)>),
			L"(Struct<float, double> :: Value<char (char) *, wchar_t (wchar_t) *>)",
			wchar_t * * const &
		);
		ASSERT_OVERLOADING_LVALUE(
			(Struct<float, double> :: Value<char(*)(float), wchar_t(*)(double)>),
			L"(Struct<float, double> :: Value<char (float) *, wchar_t (double) *>)",
			bool * * const &
		);
		ASSERT_OVERLOADING_LVALUE(
			(Struct<float, double> :: Value<char(*)(float), char(*)(float), char(*)(float), char(*)(float)>),
			L"(Struct<float, double> :: Value<char (float) *, char (float) *, char (float) *, char (float) *>)",
			float * * const &
		);
		ASSERT_OVERLOADING_LVALUE(
			(Struct<float, double> :: Value<char(*)(float), float(*)(char), char(*)(char), float(*)(float)>),
			L"(Struct<float, double> :: Value<char (float) *, float (char) *, char (char) *, float (float) *>)",
			double * * const &
		);
		ASSERT_OVERLOADING_LVALUE(
			(Struct<float, double> :: Value<char(*)(float, double), wchar_t(*)(float, double), char(*)(wchar_t(*)(float), wchar_t(*)(double))>),
			L"(Struct<float, double> :: Value<char (float, double) *, wchar_t (float, double) *, char (wchar_t (float) *, wchar_t (double) *) *>)",
			void * * const &
		);

		ASSERT_OVERLOADING_SIMPLE_LVALUE(
			(Struct<float, double> :: Value<FuncType<float, float>, FuncType<double, double>>),
			char * * const &
		);
		ASSERT_OVERLOADING_SIMPLE_LVALUE(
			(Struct<float, double> :: Value<FuncType<char, char>, FuncType<wchar_t, wchar_t>>),
			wchar_t * * const &
		);
		ASSERT_OVERLOADING_SIMPLE_LVALUE(
			(Struct<float, double> :: Value<FuncType<char, float>, FuncType<wchar_t, double>>),
			bool * * const &
		);
		ASSERT_OVERLOADING_SIMPLE_LVALUE(
			(Struct<float, double> :: Value<FuncType<char, float>, FuncType<char, float>, FuncType<char, float>, FuncType<char, float>>),
			float * * const &
		);
		ASSERT_OVERLOADING_SIMPLE_LVALUE(
			(Struct<float, double> :: Value<FuncType<char, float>, FuncType<float, char>, FuncType<char, char>, FuncType<float, float>>),
			double * * const &
		);
		ASSERT_OVERLOADING_SIMPLE_LVALUE(
			(Struct<float, double> :: Value<FuncType<char, float, double>, FuncType<wchar_t, float, double>, FuncType<char, FuncType<wchar_t, float>, FuncType<wchar_t, double>>>),
			void * * const &
		);
	});

	TEST_CATEGORY(L"Partial instantiation in class")
	{
		const wchar_t* input = LR"(
template<typename T, typename U>
constexpr auto Value = false;

template<typename T, typename U>
constexpr auto Value<T*, U> = 0.f;

template<typename T, typename U>
constexpr auto Value<T, U*> = 0.0;

template<typename T, typename U>
constexpr auto Value<T*, U*> = 'c';

template<typename T, typename U>
constexpr auto A = Value<T*, U>;

template<typename T, typename U>
constexpr auto B = Value<T, U*>;

template<typename T, typename U>
constexpr auto C = Value<T*, U*>;
)";
		COMPILE_PROGRAM(program, pa, input);

		AssertExpr(pa,	L"A",	L"A",
			L"<::A::[T], ::A::[U]> bool const & $PR",
			L"<::A::[T], ::A::[U]> float const & $PR",
			L"<::A::[T], ::A::[U]> double const & $PR",
			L"<::A::[T], ::A::[U]> char const & $PR"
		);

		AssertExpr(pa,	L"B",	L"B",
			L"<::B::[T], ::B::[U]> bool const & $PR",
			L"<::B::[T], ::B::[U]> float const & $PR",
			L"<::B::[T], ::B::[U]> double const & $PR",
			L"<::B::[T], ::B::[U]> char const & $PR"
		);

		AssertExpr(pa,	L"C",	L"C",
			L"<::C::[T], ::C::[U]> float const & $PR",
			L"<::C::[T], ::C::[U]> double const & $PR",
			L"<::C::[T], ::C::[U]> char const & $PR"
		);
	});

	TEST_CATEGORY(L"SFINAE")
	{
		const wchar_t* input = LR"(
struct A { using X = A*&; };
struct B { using Y = B*&; };
struct C { using Z = C*&; };

template<typename T, typename U = T&>
constexpr auto Value = false;

template<typename T>
constexpr auto Value<T*, typename T::X> = 0.f;

template<typename T>
constexpr auto Value<T*, typename T::Y> = 0.0;

template<typename T>
constexpr auto Value<T*, typename T::Z> = 'c';
)";
		COMPILE_PROGRAM(program, pa, input);

		AssertExpr(pa,	L"Value<A*>",	L"Value<A *>",	L"float const & $PR"	);
		AssertExpr(pa,	L"Value<B*>",	L"Value<B *>",	L"double const & $PR"	);
		AssertExpr(pa,	L"Value<C*>",	L"Value<C *>",	L"char const & $PR"		);
	});

	TEST_CATEGORY(L"Re-index")
	{
		auto input = LR"(
template<typename T, typename... Ts>
constexpr auto F = false;

template<>
constexpr auto F<float, char, wchar_t> = 0.f;

template<>
constexpr auto F<double, wchar_t, char> = 0.0;

auto x1 = F<float, char, wchar_t>;
auto x2 = F<float, wchar_t, char>;
auto x3 = F<double, char, wchar_t>;
auto x4 = F<double, wchar_t, char>;
)";

		SortedList<vint> accessed;
		auto recorder = BEGIN_ASSERT_SYMBOL
			ASSERT_SYMBOL			(0, L"F", 10, 10, ValueAliasDeclaration, 2, 15)
			ASSERT_SYMBOL			(1, L"F", 11, 10, ValueAliasDeclaration, 2, 15)
			ASSERT_SYMBOL			(2, L"F", 12, 10, ValueAliasDeclaration, 2, 15)
			ASSERT_SYMBOL			(3, L"F", 13, 10, ValueAliasDeclaration, 2, 15)
			ASSERT_SYMBOL_OVERLOAD	(4, L"F", 10, 10, ValueAliasDeclaration, 5, 15)
			ASSERT_SYMBOL_OVERLOAD	(5, L"F", 11, 10, ValueAliasDeclaration, 2, 15)
			ASSERT_SYMBOL_OVERLOAD	(6, L"F", 12, 10, ValueAliasDeclaration, 2, 15)
			ASSERT_SYMBOL_OVERLOAD	(7, L"F", 13, 10, ValueAliasDeclaration, 8, 15)
		END_ASSERT_SYMBOL;

		COMPILE_PROGRAM_WITH_RECORDER(program, pa, input, recorder);
		TEST_CASE_ASSERT(accessed.Count() == 8);
	});
}