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
			template<typename T>
			struct B
			{
				static auto F() { return B(); }
			};

			template<typename T>
			struct B<T*>
			{
				static auto F() { return B(); }
			};
		};

		template<typename T>
		struct A<T*>
		{
			template<typename T>
			struct B
			{
				static auto F() { return B(); }
			};

			template<typename T>
			struct B<T*>
			{
				static auto F() { return B(); }
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
			L"::A<char>::B@<double *> $PR",
			A<char>::B<double*>
		);

		ASSERT_OVERLOADING_FORMATTED_VERBOSE(
			A<char *> :: B<double> :: F(),
			L"::A@<char *>::B<double> $PR",
			A<char*>::B<double>
		);

		ASSERT_OVERLOADING_FORMATTED_VERBOSE(
			A<char *> :: B<double *> :: F(),
			L"::A@<char *>::B@<double *> $PR",
			A<char*>::B<double*>
		);
	});
}