#include "Util.h"

namespace Input__TestOverloadingGenericFunction_TypeInferSimple
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
	);
}

namespace Input__TestOverloadingGenericFunction_TypeInferVariant
{
	TEST_DECL(
		const int ci = 0;
		volatile int vi = 0;
		const volatile int cvi = 0;

		template<typename... Ts>
		struct Types {};

		template<typename... Ts>
		auto Variant(Ts... ts) { return Types<Ts...>(); }
	);
}

namespace Input__TestOverloadingGenericFunction_TypeInferKinds
{
	TEST_DECL(
		template<typename... Ts>
		struct Types {};

		template<typename T>
		T Value();

		template<typename T>							auto LRef(T&)								-> T					;
		template<typename T>							auto RRef(T&&)								-> T					;
		template<typename T>							auto Pointer(T*)							-> T					;
		template<typename R, typename... TArgs>			auto Function(R(*)(TArgs...))				-> Types<R, TArgs...>	;
		template<typename T, typename U>				auto Member(T U::*)							-> Types<T, U>			;
		template<typename T>							auto C(const T)								-> T					;
		template<typename T>							auto V(volatile T)							-> T					;
		template<typename T>							auto CV(const volatile T)					-> T					;
		template<typename... TArgs>						auto VtaPtr(TArgs*...)						-> Types<TArgs...>		;
		template<typename... TArgs>						auto VtaTypes(Types<TArgs...>)				-> Types<TArgs...>		;
		template<typename R, typename... TArgs>			auto VtaFunc(Types<R(*)(TArgs*)...>)		-> Types<TArgs...>		;
		template<typename... TRs, typename... TArgs>	auto VtaFunc2(Types<TRs(*)(TArgs*)...>)		-> Types<TArgs...>		;
	);
}

TEST_FILE
{
	TEST_CATEGORY(L"Template argument deduction (simple)")
	{
		using namespace Input__TestOverloadingGenericFunction_TypeInferSimple;
		COMPILE_PROGRAM(program, pa, input);

		ASSERT_OVERLOADING_SIMPLE(Simple(1),					__int32);
		ASSERT_OVERLOADING_SIMPLE(Simple(1.0),					double);
		ASSERT_OVERLOADING_SIMPLE(Simple(1.f),					float);
		ASSERT_OVERLOADING_SIMPLE(Simple(ci),					__int32);
		ASSERT_OVERLOADING_SIMPLE(Simple(vi),					__int32);
		ASSERT_OVERLOADING_SIMPLE(Simple(cvi),					__int32);

		ASSERT_OVERLOADING_SIMPLE(Simple2(1),					__int32);
		ASSERT_OVERLOADING_SIMPLE(Simple2(1.0),					double);
		ASSERT_OVERLOADING_SIMPLE(Simple2(1.f),					float);
		ASSERT_OVERLOADING_SIMPLE(Simple2(ci),					__int32);
		ASSERT_OVERLOADING_SIMPLE(Simple2(vi),					__int32);
		ASSERT_OVERLOADING_SIMPLE(Simple2(cvi),					__int32);

		ASSERT_OVERLOADING_SIMPLE(Simple2(1, 2),				__int32);
		ASSERT_OVERLOADING_SIMPLE(Simple2(1.0, 2.0),			double);
		ASSERT_OVERLOADING_SIMPLE(Simple2(1.f, 2.f),			float);
		ASSERT_OVERLOADING_SIMPLE(Simple2(ci, vi),				__int32);
		ASSERT_OVERLOADING_SIMPLE(Simple2(vi, cvi),				__int32);
		ASSERT_OVERLOADING_SIMPLE(Simple2(cvi, ci),				__int32);

		ASSERT_OVERLOADING_SIMPLE(Simple2(1, 2.0),				void);
		ASSERT_OVERLOADING_SIMPLE(Simple2(1.0, 2.f),			void);
		ASSERT_OVERLOADING_SIMPLE(Simple2(1.f, 2),				void);
	});

	TEST_CATEGORY(L"Template argument deduction (variant)")
	{
		using namespace Input__TestOverloadingGenericFunction_TypeInferVariant;
		COMPILE_PROGRAM(program, pa, input);

		ASSERT_OVERLOADING_SIMPLE((Variant(1, 1.0, 1.f, ci, vi, cvi)),			Types<__int32, double, float, __int32, __int32, __int32>);
	});

	TEST_CATEGORY(L"Template argument deduction (kinds)")
	{
		using namespace Input__TestOverloadingGenericFunction_TypeInferKinds;
		COMPILE_PROGRAM(program, pa, input);
	});
}