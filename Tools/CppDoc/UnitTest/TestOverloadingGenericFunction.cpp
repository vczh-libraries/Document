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

		template<typename R, typename... TArgs>
		using FunctionOf = R(*)(TArgs...);

		template<typename T, typename U>
		using MemberOf = T U::*;

		template<typename T>							auto LRef(T&)								-> Types<T>					;
		template<typename T>							auto RRef(T&&)								-> Types<T>					;
		template<typename T>							auto Pointer(T*)							-> Types<T>					;
		template<typename R, typename... TArgs>			auto Function(R(*)(TArgs...))				-> Types<R, TArgs...>		;
		template<typename T, typename U>				auto Member(T U::*)							-> Types<T, U>				;
		template<typename T>							auto C(const T)								-> Types<T>					;
		template<typename T>							auto V(volatile T)							-> Types<T>					;
		template<typename T>							auto CV(const volatile T)					-> Types<T>					;
		template<typename... TArgs>						auto VtaPtr(TArgs*...)						-> Types<TArgs...>			;
		template<typename... TArgs>						auto VtaTypes(Types<TArgs...>)				-> Types<TArgs...>			;
		template<typename R, typename... TArgs>			auto VtaFunc(Types<R(*)(TArgs*)...>)		-> Types<R, TArgs...>		;
		template<typename... TRs, typename... TArgs>	auto VtaFunc2(Types<TRs&(*)(TArgs*)...>)	-> Types<TRs..., TArgs...>	;
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

		ASSERT_OVERLOADING_SIMPLE(LRef(Value<bool &>()),									Types<bool>);
		ASSERT_OVERLOADING_SIMPLE(LRef(Value<bool const &>()),								Types<bool const>);
		ASSERT_OVERLOADING_SIMPLE(LRef(Value<bool volatile &>()),							Types<bool volatile>);
		ASSERT_OVERLOADING_SIMPLE(LRef(Value<bool const volatile &>()),						Types<bool const volatile>);

		ASSERT_OVERLOADING_SIMPLE(RRef(Value<bool &>()),									Types<bool &>);
		ASSERT_OVERLOADING_SIMPLE(RRef(Value<bool const &>()),								Types<bool const &>);
		ASSERT_OVERLOADING_SIMPLE(RRef(Value<bool volatile &>()),							Types<bool volatile &>);
		ASSERT_OVERLOADING_SIMPLE(RRef(Value<bool const volatile &>()),						Types<bool const volatile &>);
		ASSERT_OVERLOADING_SIMPLE(RRef(Value<bool &&>()),									Types<bool>);
		ASSERT_OVERLOADING_SIMPLE(RRef(Value<bool const &&>()),								Types<bool const>);
		ASSERT_OVERLOADING_SIMPLE(RRef(Value<bool volatile &&>()),							Types<bool volatile>);
		ASSERT_OVERLOADING_SIMPLE(RRef(Value<bool const volatile &&>()),					Types<bool const volatile>);

		ASSERT_OVERLOADING_SIMPLE(Pointer(Value<bool *>()),									Types<bool>);
		ASSERT_OVERLOADING_SIMPLE(Pointer(Value<bool const *>()),							Types<bool const>);
		ASSERT_OVERLOADING_SIMPLE(Pointer(Value<bool volatile *>()),						Types<bool volatile>);
		ASSERT_OVERLOADING_SIMPLE(Pointer(Value<bool const volatile *>()),					Types<bool const volatile>);
		
		ASSERT_OVERLOADING_SIMPLE(Function(Value<FunctionOf<bool>>()),						Types<bool>);
		ASSERT_OVERLOADING_SIMPLE(Function(Value<FunctionOf<bool, float, double>>()),		Types<bool, float, double>);
		ASSERT_OVERLOADING_SIMPLE(Member(Value<MemberOf<bool, Types<>>>()),					Types<bool, Types<>>);

		ASSERT_OVERLOADING_SIMPLE(C(Value<bool>()),											Types<bool>);
		ASSERT_OVERLOADING_SIMPLE(C(Value<bool const>()),									Types<bool>);
		ASSERT_OVERLOADING_SIMPLE(C(Value<bool volatile>()),								Types<bool>);
		ASSERT_OVERLOADING_SIMPLE(C(Value<bool const volatile>()),							Types<bool>);
		ASSERT_OVERLOADING_SIMPLE(V(Value<bool>()),											Types<bool>);
		ASSERT_OVERLOADING_SIMPLE(V(Value<bool const>()),									Types<bool>);
		ASSERT_OVERLOADING_SIMPLE(V(Value<bool volatile>()),								Types<bool>);
		ASSERT_OVERLOADING_SIMPLE(V(Value<bool const volatile>()),							Types<bool>);
		ASSERT_OVERLOADING_SIMPLE(CV(Value<bool>()),										Types<bool>);
		ASSERT_OVERLOADING_SIMPLE(CV(Value<bool const>()),									Types<bool>);
		ASSERT_OVERLOADING_SIMPLE(CV(Value<bool volatile>()),								Types<bool>);
		ASSERT_OVERLOADING_SIMPLE(CV(Value<bool const volatile>()),							Types<bool>);

		ASSERT_OVERLOADING_SIMPLE(
			VtaPtr(Value<bool *>(), Value<bool const *>(), Value<bool volatile *>()),
			Types<bool, bool const, bool volatile>
		);
		ASSERT_OVERLOADING_SIMPLE(
			VtaTypes(Value<Types<bool *, bool const *, bool volatile *>>()),
			Types<bool *, bool const *, bool volatile *>
		);
		ASSERT_OVERLOADING_SIMPLE(
			VtaFunc(Value<Types<FunctionOf<bool, float *>, FunctionOf<bool, double *>>>()),
			Types<bool, float, double>
		);
		ASSERT_OVERLOADING_SIMPLE(
			VtaFunc2(Value<Types<FunctionOf<bool &, float *>, FunctionOf<char &, double *>>>()),
			Types<bool, char, float, double>
		);
	});
}