#include "Util.h"

namespace INPUT__TestParsePSFunction_Functions
{
	TEST_DECL(
		void R();

		template<typename T>
		T R(T);

		template<typename T, typename U, typename... Ts>
		auto R(T, U u, Ts... ts) { return R(u, ts...); }

		template<typename T, typename U, typename... Ts>
		auto F(T, U, Ts*...) -> decltype(R(Ts()...));

		template<>
		void F<char, wchar_t>(char, wchar_t);

		template<>
		float F<char, wchar_t, float>(char, wchar_t, float*);

		template<>
		double F<char, wchar_t, float, double>(char, wchar_t, float*, double*);

		template<>
		bool F<char, wchar_t, float, double, bool>(char, wchar_t, float*, double*, bool*);

		float* pf = nullptr;
		double* pd = nullptr;
		bool* pb = nullptr;
	);
}

namespace INPUT__TestParsePSFunction_SFINAE
{
	TEST_DECL(
		struct A { using X = A*; };
		struct B { using Y = B*; };
		struct C { using Z = C*; };

		template<typename T>
		char F(T, typename T::X);

		template<typename T>
		wchar_t F(T, typename T::Y);

		template<typename T>
		bool F(T, typename T::Z);

		template<>
		char F<A>(A, A*);

		template<>
		wchar_t F<B>(B, B*);

		template<>
		bool F<C>(C, C*);

		template<typename T>
		void F(T, ...);

		A a;
		B b;
		C c;
	);
}

TEST_FILE
{
	TEST_CATEGORY(L"Return type trait")
	{
		using namespace INPUT__TestParsePSFunction_Functions;
		COMPILE_PROGRAM(program, pa, input);

		ASSERT_OVERLOADING_SIMPLE(R(),						void	);
		ASSERT_OVERLOADING_SIMPLE(R(1.f),					float	);
		ASSERT_OVERLOADING_SIMPLE(R(1.f, 1.0),				double	);
		ASSERT_OVERLOADING_SIMPLE(R(1.f, 1.0, false),		bool	);
		ASSERT_OVERLOADING_SIMPLE(R(1.f, 1.0, false, 'c'),	char	);
	});

	TEST_CATEGORY(L"Partial specialization relationship")
	{
		using namespace INPUT__TestParsePSFunction_Functions;
		COMPILE_PROGRAM(program, pa, input);

		Symbol* primary = nullptr;
		for (vint i = 0; i < program->decls.Count(); i++)
		{
			if(auto decl=program->decls[i].Cast<ForwardFunctionDeclaration>())
			{
				if (decl->name.name == L"F")
				{
					if (primary)
					{
						TEST_CASE_ASSERT(decl->symbol->GetFunctionSymbol_Fb()->GetPSPrimary_NF() == primary);
					}
					else
					{
						primary = decl->symbol->GetFunctionSymbol_Fb();
						TEST_CASE_ASSERT(primary->IsPSPrimary_NF());
						TEST_CASE_ASSERT(primary->GetPSPrimaryDescendants_NF().Count() == 4);
					}
				}
			}
		}
	});

	TEST_CATEGORY(L"Instantiate template functions")
	{
		using namespace INPUT__TestParsePSFunction_Functions;
		COMPILE_PROGRAM(program, pa, input);

		ASSERT_OVERLOADING_VERBOSE(
			(&F<char, wchar_t>),
			L"((& F<char, wchar_t>))",
			L"void __cdecl(char, wchar_t) * $PR",
			void(*)(char, wchar_t)
		);

		ASSERT_OVERLOADING_VERBOSE(
			(&F<char, wchar_t, float>),
			L"((& F<char, wchar_t, float>))",
			L"float __cdecl(char, wchar_t, float *) * $PR",
			float(*)(char, wchar_t, float*)
		);

		ASSERT_OVERLOADING_VERBOSE(
			(&F<char, wchar_t, float, double>),
			L"((& F<char, wchar_t, float, double>))",
			L"double __cdecl(char, wchar_t, float *, double *) * $PR",
			double(*)(char, wchar_t, float*, double*)
		);

		ASSERT_OVERLOADING_VERBOSE(
			(&F<char, wchar_t, float, double, bool>),
			L"((& F<char, wchar_t, float, double, bool>))",
			L"bool __cdecl(char, wchar_t, float *, double *, bool *) * $PR",
			bool(*)(char, wchar_t, float*, double*, bool*)
		);
	});

	TEST_CATEGORY(L"Calling template functions")
	{
		using namespace INPUT__TestParsePSFunction_Functions;
		COMPILE_PROGRAM(program, pa, input);

		ASSERT_OVERLOADING_SIMPLE(F('c', L'c'),				void	);
		ASSERT_OVERLOADING_SIMPLE(F('c', L'c', pf),			float	);
		ASSERT_OVERLOADING_SIMPLE(F('c', L'c', pf, pd),		double	);
		ASSERT_OVERLOADING_SIMPLE(F('c', L'c', pf, pd, pb),	bool	);
	});

	TEST_CATEGORY(L"SFINAE")
	{
		using namespace INPUT__TestParsePSFunction_SFINAE;
		COMPILE_PROGRAM(program, pa, input);

		List<Symbol*> fs;
		for (vint i = 0; i < program->decls.Count(); i++)
		{
			if (auto decl = program->decls[i].Cast<ForwardFunctionDeclaration>())
			{
				if (decl->name.name == L"F")
				{
					fs.Add(decl->symbol->GetFunctionSymbol_Fb());
				}
			}
		}

		TEST_CASE_ASSERT(fs.Count() == 7);
		for (vint i = 0; i < 6; i++)
		{
			if (i < 3)
			{
				TEST_CASE_ASSERT(fs[i]->IsPSPrimary_NF());
				TEST_CASE_ASSERT(fs[i]->GetPSPrimaryDescendants_NF().Count() == 1);
			}
			else
			{
				TEST_CASE_ASSERT(fs[i]->GetPSPrimary_NF() == fs[i - 3]);
			}
		}

		ASSERT_OVERLOADING(F(a, &a),	L"F(a, (& a))",		char	);
		ASSERT_OVERLOADING(F(a, &b),	L"F(a, (& b))",		void	);
		ASSERT_OVERLOADING(F(a, &c),	L"F(a, (& c))",		void	);
		ASSERT_OVERLOADING(F(b, &a),	L"F(b, (& a))",		void	);
		ASSERT_OVERLOADING(F(b, &b),	L"F(b, (& b))",		wchar_t	);
		ASSERT_OVERLOADING(F(b, &c),	L"F(b, (& c))",		void	);
		ASSERT_OVERLOADING(F(c, &a),	L"F(c, (& a))",		void	);
		ASSERT_OVERLOADING(F(c, &b),	L"F(c, (& b))",		void	);
		ASSERT_OVERLOADING(F(c, &c),	L"F(c, (& c))",		bool	);
	});
}