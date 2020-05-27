#include "Util.h"

namespace Input__TestOverloadingGenericFunction_RecursiveOverloading
{
	TEST_DECL(
		struct VoidType
		{
			void* tag = nullptr;
		};

		struct IntType
		{
			int tag = 0;
		};

		struct BoolType
		{
			bool tag = false;
		};

		struct Cat
		{
			Cat* tag = nullptr;
		};

		struct Dog
		{
			Dog* tag = nullptr;
		};

		VoidType LastArg() { return {}; }
		IntType LastArg(int) { return {}; }
		BoolType LastArg(bool) { return {}; }

		template<typename T>
		auto LastArg(T t) { return t; }

		template<typename T, typename... Ts>
		auto LastArg(T t, Ts... ts)
		{
			return LastArg(ts...);
		}
	);
}

TEST_FILE
{
	TEST_CATEGORY(L"Recursive Template Functions Overloading")
	{
		using namespace Input__TestOverloadingGenericFunction_RecursiveOverloading;
		COMPILE_PROGRAM(program, pa, input);

		ASSERT_OVERLOADING_SIMPLE(
			LastArg().tag,
			void *
		);

		ASSERT_OVERLOADING_SIMPLE(
			LastArg(0).tag,
			__int32
		);

		ASSERT_OVERLOADING_SIMPLE(
			LastArg(false).tag,
			bool
		);

		ASSERT_OVERLOADING_FORMATTED_VERBOSE(
			LastArg(1, true, Cat()).tag,
			L"::Cat *",
			Cat *
		);

		ASSERT_OVERLOADING_FORMATTED_VERBOSE(
			LastArg(0, false, Dog()).tag,
			L"::Dog *",
			Dog *
		);

		ASSERT_OVERLOADING_SIMPLE(
			LastArg(Cat(), Dog(), 1).tag,
			__int32
		);

		ASSERT_OVERLOADING_SIMPLE(
			LastArg(Dog(), Cat(), true).tag,
			bool
		);
	});
}