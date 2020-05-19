#include "Util.h"

namespace INPUT__TestParsePSClass_ToPrimary1
{
	TEST_DECL(
		template<typename T, typename U>
		constexpr auto Match = false;

		template<typename T>
		constexpr auto Match<T, T> = "true";

		template<typename T>
		struct Scope;

		template<typename T>
		struct Scope<T*>
		{
			auto RunMatch()
			{
				return Match<Scope<T*>&, decltype(*this)>;
			}
		};
	);
}

namespace INPUT__TestParsePSClass_ToPrimary2
{
	TEST_DECL(
		auto Match(...) { return false; }

		template<typename T>
		auto Match(T, T) { return "true"; }

		template<typename T>
		struct Scope;

		template<typename T>
		struct Scope<T*>
		{
			auto RunMatch()
			{
				return Match(Scope<T*>(), *this);
			}
		};
	);
}

namespace INPUT__TestParsePSClass_ToPrimary3
{
	TEST_DECL(
		template<typename T>
		struct Scope;

		auto Match(...) { return false; }

		template<typename T>
		auto Match(Scope<T>) { return T(); }

		template<typename T>
		struct Scope<T*>
		{
			auto RunMatch1()
			{
				return Match(Scope<T*>());
			}

			auto RunMatch2()
			{
				return Match(*this);
			}
		};
	);
}

namespace INPUT__TestParsePSClass_ToPrimary4
{
	TEST_DECL(
		template<typename T>
		struct Scope;

		template<typename T>
		struct Scope<T*>;

		auto Match(...) { return false; }

		auto Match(const Scope<void*>&) { return (void*)nullptr; }

		template<typename T>
		struct Scope<T*>
		{
			auto RunMatch1()
			{
				return Match(Scope<T*>());
			}

			auto RunMatch2()
			{
				return Match(*this);
			}
		};
	);
}

namespace INPUT__TestParsePSClass_ClassRef
{
	TEST_DECL(
		template<typename T>
		struct A
		{
			template<typename U>
			struct B
			{
				static auto F() { return B(); }
				static auto G() { return A(); }
			};

			template<typename U>
			struct B<U*>
			{
				static auto F() { return B(); }
				static auto G() { return A(); }
			};
		};

		template<typename T>
		struct A<T*>
		{
			template<typename U>
			struct B
			{
				static auto F() { return B(); }
				static auto G() { return A(); }
			};

			template<typename U>
			struct B<U*>
			{
				static auto F() { return B(); }
				static auto G() { return A(); }
			};
		};
	);
}

TEST_FILE
{
	TEST_CATEGORY(L"To Primary 1")
	{
		using namespace INPUT__TestParsePSClass_ToPrimary1;
		COMPILE_PROGRAM(program, pa, input);

		ASSERT_OVERLOADING_SIMPLE(
			Scope<void *>().RunMatch(),
			char const *
		);
	});

	TEST_CATEGORY(L"To Primary 2")
	{
		using namespace INPUT__TestParsePSClass_ToPrimary2;
		COMPILE_PROGRAM(program, pa, input);

		ASSERT_OVERLOADING_SIMPLE(
			Scope<void *>().RunMatch(),
			char const *
		);
	});

	TEST_CATEGORY(L"To Primary 3")
	{
		using namespace INPUT__TestParsePSClass_ToPrimary3;
		COMPILE_PROGRAM(program, pa, input);

		ASSERT_OVERLOADING_SIMPLE(
			Scope<void *>().RunMatch1(),
			void *
		);

		ASSERT_OVERLOADING_SIMPLE(
			Scope<void *>().RunMatch2(),
			void *
		);
	});

	TEST_CATEGORY(L"To Primary 4")
	{
		using namespace INPUT__TestParsePSClass_ToPrimary4;
		COMPILE_PROGRAM(program, pa, input);

		ASSERT_OVERLOADING_SIMPLE(
			Scope<void *>().RunMatch1(),
			void *
		);

		ASSERT_OVERLOADING_SIMPLE(
			Scope<void *>().RunMatch2(),
			void *
		);
	});

	TEST_CATEGORY(L"Class Reference")
	{
		using namespace INPUT__TestParsePSClass_ClassRef;
		COMPILE_PROGRAM(program, pa, input);

		ASSERT_OVERLOADING_FORMATTED_VERBOSE(
			A<char> :: B<double> :: F(),
			L"::A<char>::B<double> $PR",
			A<char>::B<double>
		);

		ASSERT_OVERLOADING_FORMATTED_VERBOSE(
			A<char> :: B<double *> :: F(),
			L"::A<char>::B@<[U] *><double> $PR",
			A<char>::B<double*>
		);

		ASSERT_OVERLOADING_FORMATTED_VERBOSE(
			A<char *> :: B<double> :: F(),
			L"::A@<[T] *><char>::B<double> $PR",
			A<char*>::B<double>
		);

		ASSERT_OVERLOADING_FORMATTED_VERBOSE(
			A<char *> :: B<double *> :: F(),
			L"::A@<[T] *><char>::B@<[U] *><double> $PR",
			A<char*>::B<double*>
		);

		ASSERT_OVERLOADING_FORMATTED_VERBOSE(
			A<char> :: B<double> :: G(),
			L"::A<char> $PR",
			A<char>
		);

		ASSERT_OVERLOADING_FORMATTED_VERBOSE(
			A<char> :: B<double *> :: G(),
			L"::A<char> $PR",
			A<char>
		);

		ASSERT_OVERLOADING_FORMATTED_VERBOSE(
			A<char *> :: B<double> :: G(),
			L"::A@<[T] *><char> $PR",
			A<char*>
		);

		ASSERT_OVERLOADING_FORMATTED_VERBOSE(
			A<char *> :: B<double *> :: G(),
			L"::A@<[T] *><char> $PR",
			A<char*>
		);
	});

	TEST_CATEGORY(L"SFINAE")
	{
		const wchar_t* input = LR"(
struct A { using X = A*&; };
struct B { using Y = B*&; };
struct C { using Z = C*&; };

template<typename T, typename U = T&>
struct Type;

template<typename T>
struct Type<T*, typename T::X> { static const auto Value = 0.f; };

template<typename T>
struct Type<T*, typename T::Y> { static const auto Value = 0.0; };

template<typename T>
struct Type<T*, typename T::Z> { static const auto Value = 'c'; };
)";
		COMPILE_PROGRAM(program, pa, input);

		AssertExpr(pa,	L"Type<A*>::Value",		L"Type<A *> :: Value",		L"float const $L"	);
		AssertExpr(pa,	L"Type<B*>::Value",		L"Type<B *> :: Value",		L"double const $L"	);
		AssertExpr(pa,	L"Type<C*>::Value",		L"Type<C *> :: Value",		L"char const $L"	);
	});

	TEST_CATEGORY(L"Declaration after evaluation")
	{
		auto input = LR"(
template<typename T>
struct X;

X<int> x;

template<typename U>
struct X<int>
{
	int* y;
};
)";
		COMPILE_PROGRAM(program, pa, input);

		AssertExpr(pa,		L"x.y",			L"x.y",				L"__int32 * $L"	);
	});
}