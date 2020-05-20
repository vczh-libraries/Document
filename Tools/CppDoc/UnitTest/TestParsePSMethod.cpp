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
		auto input = LR"(
template<typename T>
struct A
{
	template<typename U>
	struct B
	{
		static void F() {}
		static void G();
	};

	template<typename U>
	struct B<U*>
	{
		static void F() {}
		static void G();
	};
};

template<typename T>
struct A<T*>
{
	template<typename U>
	struct B
	{
		static void F() {}
		static void G();
	};

	template<typename U>
	struct B<U*>
	{
		static void F() {}
		static void G();
	};
};

template<typename X>	template<typename Y>	void A<X>::B<Y>::G()	{}
template<typename X>	template<typename Y>	void A<X>::B<Y*>::G()	{}
template<typename X>	template<typename Y>	void A<X*>::B<Y>::G()	{}
template<typename X>	template<typename Y>	void A<X*>::B<Y*>::G()	{}
)";
		COMPILE_PROGRAM(program, pa, input);

		const wchar_t* codes[] = {
			L"A<char>::B<double>",
			L"A<char>::B<double*>",
			L"A<char*>::B<double>",
			L"A<char*>::B<double*>",
		};

		const wchar_t* as[] = {
			L"::A<char>",
			L"::A<char>",
			L"::A@<[T] *><char>",
			L"::A@<[T] *><char>",
		};

		const wchar_t* bs[] = {
			L"::A<char>::B<double>",
			L"::A<char>::B@<[U] *><double>",
			L"::A@<[T] *><char>::B<double>",
			L"::A@<[T] *><char>::B@<[U] *><double>",
		};

		for (vint i = 0; i < sizeof(codes) / sizeof(*codes); i++)
		{
			TEST_CATEGORY(codes[i])
			{
				TOKEN_READER(codes[i]);
				auto cursor = reader.GetFirstToken();

				auto type = ParseType(pa, cursor);
				TEST_CASE_ASSERT(!cursor);

				TypeTsysList tsys;
				TypeToTsysNoVta(pa, type, tsys);
				TEST_CASE_ASSERT(tsys.Count() == 1);
				TEST_CASE_ASSERT(tsys[0]->GetType() == TsysType::DeclInstant);

				auto psr = symbol_type_resolving::EvaluateClassPSRecord(pa, tsys[0]);
				TEST_CASE_ASSERT(psr);
				TEST_CASE_ASSERT(psr->evaluatedTypes.Count() == 1);

				auto psTsys = psr->evaluatedTypes[0];
				auto& di = psTsys->GetDeclInstant();
				{
					auto statSymbol = psTsys->GetDecl()
						->TryGetChildren_NFb(L"F")->Get(0)
						->GetImplSymbols_F()[0]
						->TryGetChildren_NFb(L"$")->Get(0);
					auto funcPa = pa.AdjustForDecl(statSymbol.Obj(), di.parentDeclType);

					AssertType(funcPa,	L"A",	L"A",	as[i]);
					AssertType(funcPa,	L"B",	L"B",	bs[i]);
				}
				{
					auto statSymbol = psTsys->GetDecl()
						->TryGetChildren_NFb(L"G")->Get(0)
						->GetImplSymbols_F()[0]
						->TryGetChildren_NFb(L"$")->Get(0);
					auto funcPa = pa.AdjustForDecl(statSymbol.Obj(), di.parentDeclType);

					AssertType(funcPa,	L"A",	L"A",	as[i]);
					AssertType(funcPa,	L"B",	L"B",	bs[i]);
				}
			});
		}
	});

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