#include "Util.h"

namespace INPUT__TestParsePSMethod_SFINAE
{
	TEST_DECL(
		struct A { using X = A*; };
		struct B { using Y = B*; };
		struct C { using Z = C*; };

		template<typename T, typename U = T*>
		struct Struct;

		template<typename T>
		struct Struct<T, typename T::X>
		{
			template<typename T>
			static auto F(T, typename T::X = nullptr) { return true; }

			template<typename T>
			static auto F(T, typename T::Y = nullptr) { return 1; }

			template<typename T>
			static auto F(T, typename T::Z = nullptr) { return 'c'; }
		};

		template<typename T>
		struct Struct<T, typename T::Y>
		{
			template<typename T>
			static auto F(T, typename T::X = nullptr) { return 0.f; }

			template<typename T>
			static auto F(T, typename T::Y = nullptr) { return 0.0; }

			template<typename T>
			static auto F(T, typename T::Z = nullptr) { return L'c'; }
		};

		template<typename T>
		struct Struct<T, typename T::Z>
		{
			template<typename T>
			static auto F(T, typename T::X = nullptr) { return (bool*)nullptr; }

			template<typename T>
			static auto F(T, typename T::Y = nullptr) { return (bool**)nullptr; }

			template<typename T>
			static auto F(T, typename T::Z = nullptr) { return (bool***)nullptr; }
		};
	);
}

TEST_FILE
{

	TEST_CATEGORY(L"SFINAE")
	{
		using namespace INPUT__TestParsePSMethod_SFINAE;
		COMPILE_PROGRAM(program, pa, input);

		ASSERT_OVERLOADING_SIMPLE(
			Struct<A> :: F(A()),
			bool
		);

		ASSERT_OVERLOADING_SIMPLE(
			Struct<A> :: F(B()),
			__int32
		);

		ASSERT_OVERLOADING_SIMPLE(
			Struct<A> :: F(C()),
			char
		);

		ASSERT_OVERLOADING_SIMPLE(
			Struct<B> :: F(A()),
			float
		);

		ASSERT_OVERLOADING_SIMPLE(
			Struct<B> :: F(B()),
			double
		);

		ASSERT_OVERLOADING_SIMPLE(
			Struct<B> :: F(C()),
			wchar_t
		);

		ASSERT_OVERLOADING_SIMPLE(
			Struct<C> :: F(A()),
			bool *
		);

		ASSERT_OVERLOADING_SIMPLE(
			Struct<C> :: F(B()),
			bool * *
		);

		ASSERT_OVERLOADING_SIMPLE(
			Struct<C> :: F(C()),
			bool * * *
		);
	});
}