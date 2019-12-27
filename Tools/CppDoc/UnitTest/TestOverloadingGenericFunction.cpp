#include "Util.h"

namespace Input__TestOverloadingGenericFunction_TypeInfer
{
	TEST_DECL(
		const int ci = 0;
		volatile int vi = 0;
		const volatile int cvi = 0;

		template<typename T>
		auto Simple(T t) { return t; }

		template<typename T>
		auto Simple2(T a, T b = {}) { return a; }
		void Simple2(...) {}

		template<typename... Ts>
		struct Types {};

		template<typename... Ts>
		auto Variant(Ts... ts) { return Types<Ts...>(); }
	);
}

TEST_FILE
{
	TEST_CATEGORY(L"Template argument deduction (simple)")
	{
		using namespace Input__TestOverloadingGenericFunction_TypeInfer;
		COMPILE_PROGRAM(program, pa, input);

		ASSERT_OVERLOADING(Simple(1),			L"Simple(1)",			__int32);
		ASSERT_OVERLOADING(Simple(1.0),			L"Simple(1.0)",			double);
		ASSERT_OVERLOADING(Simple(1.f),			L"Simple(1.f)",			float);
		ASSERT_OVERLOADING(Simple(ci),			L"Simple(ci)",			__int32);
		ASSERT_OVERLOADING(Simple(vi),			L"Simple(vi)",			__int32);
		ASSERT_OVERLOADING(Simple(cvi),			L"Simple(cvi)",			__int32);

		ASSERT_OVERLOADING(Simple2(1),			L"Simple2(1)",			__int32);
		ASSERT_OVERLOADING(Simple2(1.0),		L"Simple2(1.0)",		double);
		ASSERT_OVERLOADING(Simple2(1.f),		L"Simple2(1.f)",		float);
		ASSERT_OVERLOADING(Simple2(ci),			L"Simple2(ci)",			__int32);
		ASSERT_OVERLOADING(Simple2(vi),			L"Simple2(vi)",			__int32);
		ASSERT_OVERLOADING(Simple2(cvi),		L"Simple2(cvi)",		__int32);

		ASSERT_OVERLOADING(Simple2(1, 2),		L"Simple2(1, 2)",		__int32);
		ASSERT_OVERLOADING(Simple2(1.0, 2.0),	L"Simple2(1.0, 2.0)",	double);
		ASSERT_OVERLOADING(Simple2(1.f, 2.f),	L"Simple2(1.f, 2.f)",	float);
		ASSERT_OVERLOADING(Simple2(ci, vi),		L"Simple2(ci, vi)",		__int32);
		ASSERT_OVERLOADING(Simple2(vi, cvi),	L"Simple2(vi, cvi)",	__int32);
		ASSERT_OVERLOADING(Simple2(cvi, ci),	L"Simple2(cvi, ci)",	__int32);

		ASSERT_OVERLOADING(Simple2(1, 2.0),		L"Simple2(1, 2.0)",		void);
		ASSERT_OVERLOADING(Simple2(1.0, 2.f),	L"Simple2(1.0, 2.f)",	void);
		ASSERT_OVERLOADING(Simple2(1.f, 2),		L"Simple2(1.f, 2)",		void);
	});

	TEST_CATEGORY(L"Template argument deduction (variant)")
	{
		using namespace Input__TestOverloadingGenericFunction_TypeInfer;
		COMPILE_PROGRAM(program, pa, input);

		ASSERT_OVERLOADING(Variant(1, 1.0, 1.f, ci, vi, cvi),			L"Variant(1, 1.0, 1.f, ci, vi, cvi)",			Types<__int32, double, float, __int32, __int32, __int32>);
	});
}