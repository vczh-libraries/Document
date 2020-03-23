#include "Util.h"
#include <Ast_Resolving_PSO.h>

namespace Input__TestParsePartialSpecialization_PartialOrderEvaluation
{
	TEST_DECL(
		template<typename... Ts>
		struct Types;

		template<typename... Ts>
		constexpr auto Value = 0;

		template<>
		constexpr auto Value<> = 1;

		template<typename A>
		constexpr auto Value<A> = 2;

		template<typename A, typename B, typename C>
		constexpr auto Value<A, B, C> = 3;

		//-------------------------------------------------------------

		template<typename A, typename B>
		constexpr auto Value<A*, B> = 4;

		template<typename A, typename B>
		constexpr auto Value<A, B*> = 5;

		template<typename A, typename B>
		constexpr auto Value<A*, B*> = 6; // 4, 5, 23

		template<typename A, typename B>
		constexpr auto Value<const A*, B> = 7; // 4

		template<typename A, typename B>
		constexpr auto Value<A, const B*> = 8; // 5

		template<typename A, typename B>
		constexpr auto Value<const A*, const B*> = 9; // *6, *7, *8, *24

		//-------------------------------------------------------------

		template<typename A, typename B, typename C, typename... Ts>
		constexpr auto Value<Types<Ts...>, A, B, C> = 10;

		template<typename A, typename B, typename C>
		constexpr auto Value<Types<A, B, C>, A, B, C> = 11; // 10

		template<typename A, typename B, typename C>
		constexpr auto Value<Types<A, B, C>, C, B, A> = 12; // 10

		template<typename A, typename B>
		constexpr auto Value<Types<A, B, A>, A, B, A> = 13; // *11, *12

		template<typename A>
		constexpr auto Value<Types<A, A, A>, A, A, A> = 14; // *13

		template<typename A, typename B, typename C, typename... Ts, typename... Us>
		constexpr auto Value<void(*)(Ts..., A, Us...), B, C, A, Ts...> = 15;

		template<typename A, typename B, typename... Ts>
		constexpr auto Value<void(*)(Ts..., A, Ts...), A, B, A, Ts...> = 16; // 15

		template<typename A, typename B>
		constexpr auto Value<void(*)(float, double, A, char, wchar_t), A, B, A, float, double> = 17; // 15

		template<typename A, typename B>
		constexpr auto Value<void(*)(float, double, A, float, double), A, B, A, float, double> = 18; // *16

		template<typename A>
		constexpr auto Value<void(*)(float, double, A, char, wchar_t), A, A, A, float, double> = 19; // *17

		template<>
		constexpr auto Value<void(*)(float, double, bool, char, wchar_t), bool, bool, bool, float, double> = 20; // *19

		template<>
		constexpr auto Value<void(*)(float, double, bool, char, wchar_t), bool, int, bool, float, double> = 21; // *17

		template<>
		constexpr auto Value<void(*)(float, double, bool, char, wchar_t), char, wchar_t, bool, float, double> = 22; // 15

		//-------------------------------------------------------------

		template<typename... Ts>
		constexpr auto Value<Ts*...> = 23;

		template<typename... Ts>
		constexpr auto Value<const Ts*...> = 24; // 23

		template<typename A>
		constexpr auto Value<A*, A*> = 25; // *6

		template<typename A>
		constexpr auto Value<const A*, const A*> = 26; // *25, *9

		template<>
		constexpr auto Value<float*, double*> = 27; // *6

		template<>
		constexpr auto Value<float*, float*> = 28; // *25

		template<>
		constexpr auto Value<const float*, const double*> = 29; // *9

		template<>
		constexpr auto Value<const float*, const float*> = 30; // *26
	);
}

TEST_FILE
{
	TEST_CATEGORY(L"Functions")
	{
		auto input = LR"(
template<typename T> void Function(T){}
template<> int Function<int>(int);
template<> double Function<double>(double);
)";

	auto output = LR"(
template<typename T>
Function: void (T)
{
}
template<>
__forward Function<int>: int (int);
template<>
__forward Function<double>: double (double);
)";

		COMPILE_PROGRAM(program, pa, input);
		AssertProgram(program, output);
	});

	TEST_CATEGORY(L"Value aliases")
	{
		auto input = LR"(
template<typename T> auto Zero = (T)0;
template<> auto Zero<char> = '0';
template<> auto Zero<wchar_t> = L'0';
)";

		auto output = LR"(
template<typename T>
using_value Zero: auto = c_cast<T>(0);
template<>
using_value Zero<char>: auto = '0';
template<>
using_value Zero<wchar_t>: auto = L'0';
)";

		COMPILE_PROGRAM(program, pa, input);
		AssertProgram(program, output);

		Symbol* primary = nullptr;
		for (vint i = 0; i < program->decls.Count(); i++)
		{
			auto decl = program->decls[i];
			if (decl->name.name == L"Zero")
			{
				if (primary)
				{
					TEST_CASE_ASSERT(decl->symbol->GetPSPrimary_NF() == primary);
				}
				else
				{
					primary = decl->symbol;
					TEST_CASE_ASSERT(primary->GetPSPrimaryVersion_NF() == 2);
				}
			}
		}
	});

	TEST_CATEGORY(L"Classes")
	{
		auto input = LR"(
template<typename T>
struct Obj
{
};

template<typename T>
struct Obj<T*>
{
};

template<typename T, typename U>
struct Obj<T(*)(U)>
{
};
)";

		auto output = LR"(
template<typename T>
struct Obj
{
};
template<typename T>
struct Obj<T *>
{
};
template<typename T, typename U>
struct Obj<T (U) *>
{
};
)";

		COMPILE_PROGRAM(program, pa, input);
		AssertProgram(program, output);

		Symbol* primary = nullptr;
		for (vint i = 0; i < program->decls.Count(); i++)
		{
			auto decl = program->decls[i];
			if (decl->name.name == L"Obj")
			{
				if (primary)
				{
					TEST_CASE_ASSERT(decl->symbol->GetPSPrimary_NF() == primary);
				}
				else
				{
					primary = decl->symbol;
					TEST_CASE_ASSERT(primary->GetPSPrimaryVersion_NF() == 2);
				}
			}
		}
	});

	TEST_CATEGORY(L"Methods")
	{
		auto input = LR"(
template<typename T>
struct Obj
{
	template<typename U>
	void Method(T, U);

	template<>
	int Method<int>(T, int);
};
)";

		auto output = LR"(
template<typename T>
struct Obj
{
	public template<typename U>
	__forward Method: void (T, U);
	public template<>
	__forward Method<int>: int (T, int);
};
)";

		COMPILE_PROGRAM(program, pa, input);
		AssertProgram(program, output);
	});

	TEST_CATEGORY(L"Partial Order Evaluation")
	{
		using namespace Input__TestParsePartialSpecialization_PartialOrderEvaluation;
		COMPILE_PROGRAM(program, pa, input);

		Symbol* primary = nullptr;
		List<Ptr<ValueAliasDeclaration>> decls;
		for (vint i = 0; i < program->decls.Count(); i++)
		{
			auto decl = program->decls[i];
			if (decl->name.name == L"Value")
			{
				if (primary)
				{
					TEST_CASE_ASSERT(decl->symbol->GetPSPrimary_NF() == primary);
				}
				else
				{
					primary = decl->symbol;
					TEST_CASE_ASSERT(primary->GetPSPrimaryVersion_NF() == 30);
				}

				auto valueDecl = decl.Cast<ValueAliasDeclaration>();
				TEST_CASE_ASSERT(valueDecl);
				decls.Add(valueDecl);
			}
		}

		// calculate partial ordering relationship

		const Pair<vint, vint> yes_raw[] = {
			{4,6},
			{5,6},
			{23,6},
			{4,7},
			{5,8},
			{6,9},
			{7,9},
			{8,9},
			{24,9},
			{10,11},
			{10,12},
			{11,13},
			{12,13},
			{13,14},
			{15,16},
			{15,17},
			{16,18},
			{17,19},
			{19,20},
			{17,21},
			{15,22},
			{23,24},
			{6,25},
			{25,26},
			{9,26},
			{6,27},
			{25,28},
			{9,29},
			{26,30}
		};

		Group<vint, vint> parents, children, ancestors;

		for (auto p : yes_raw)
		{
			parents.Add(p.value, p.key);
			children.Add(p.key, p.value);
		}

		Func<void(vint, vint)> collectAncestors = [&](vint key, vint from)
		{
			vint index = parents.Keys().IndexOf(from);
			if (index == -1) return;
			auto& values = parents.GetByIndex(index);
			for (vint i = 0; i < values.Count(); i++)
			{
				vint to = values[i];
				if (!ancestors.Contains(key, to))
				{
					ancestors.Add(key, to);
					collectAncestors(key, to);
				}
			}
		};

		for (vint i = 0; i < parents.Count(); i++)
		{
			vint key = parents.Keys()[i];
			collectAncestors(key, key);
		}

		for (vint i = 0; i < parents.Count(); i++)
		{
			auto& values = const_cast<List<vint>&>(parents.GetByIndex(i));
			Sort<vint>(&values[0], values.Count(), [](vint a, vint b) { return a - b; });
		}

		for (vint i = 0; i < ancestors.Count(); i++)
		{
			auto& values = const_cast<List<vint>&>(ancestors.GetByIndex(i));
			Sort<vint>(&values[0], values.Count(), [](vint a, vint b) { return a - b; });
		}

		// test ordering function by comparing decls

#define ANCESTOR(A, B)	partial_specification_ordering::IsPSAncestor(pa, decls[A]->symbol, decls[A]->templateSpec, decls[A]->specializationSpec, decls[B]->symbol, decls[B]->templateSpec, decls[B]->specializationSpec)
#define YES(A, B)	TEST_CASE(itow(A)+L" <- " + itow(B)) { TEST_ASSERT(ANCESTOR(A, B) == true); });
#define NO(A, B)	TEST_CASE(itow(A)+L" <- " + itow(B)) { TEST_ASSERT(ANCESTOR(A, B) == false); });

		for (vint i = 1; i < decls.Count(); i++)
		{
			YES(0, i);
			NO(i, 0);
		}

		for (vint i = 0; i < decls.Count(); i++)
		{
			YES(i, i);
		}

		for (vint i = 0; i < ancestors.Count(); i++)
		{
			vint key = ancestors.Keys()[i];
			auto& values = ancestors.GetByIndex(i);
			for (vint j = 0; j < values.Count(); j++)
			{
				vint value = values[j];
				YES(value, key);
				NO(key, value);
			}
		}

#undef YES
#undef NO

		// test ordering for each declaration
	});
}