#include "Util.h"
#include <PSO.h>

namespace Input__TestParsePSOrder_PartialOrderEvaluation_Type
{
	TEST_DECL(
		template<typename... Ts>
		struct Types;

		template<typename... Ts>
		constexpr auto Value = 0;

		template<>
		constexpr auto Value<> = 1; // *24

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
		constexpr auto Value<void(*)(Ts..., A*, Us(...us)[]), B, C, A, Ts...> = 15;

		template<typename A, typename B, typename... Ts>
		constexpr auto Value<void(*)(Ts..., A[], Ts*...), A, B, A, Ts...> = 16; // 15

		template<typename A, typename B>
		constexpr auto Value<void(*)(float, double, A*, char[], wchar_t*), A, B, A, float, double> = 17; // 15

		template<typename A, typename B>
		constexpr auto Value<void(*)(float, double, A*, float*, double[]), A, B, A, float, double> = 18; // *16

		template<typename A>
		constexpr auto Value<void(*)(float, double, A*, char*, wchar_t*), A, A, A, float, double> = 19; // *17

		template<>
		constexpr auto Value<void(*)(float, double, bool[], char*, wchar_t[]), bool, bool, bool, float, double> = 20; // *19

		template<>
		constexpr auto Value<void(*)(float, double, bool[], char[], wchar_t*), bool, int, bool, float, double> = 21; // *17

		template<>
		constexpr auto Value<void(*)(float, double, bool[], char*, wchar_t*), char, wchar_t, bool, float, double> = 22; // 15

		//-------------------------------------------------------------

		template<typename... Ts>
		constexpr auto Value<Ts*...> = 23;

		template<typename... Ts>
		constexpr auto Value<const Ts*...> = 24;

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

namespace Input__TestParsePSOrder_PartialOrderEvaluation_Value
{
	TEST_DECL(
		template<typename T, typename U, int... vs>
		constexpr auto Value = 0;

		template<typename T>
		constexpr auto Value<bool, T, 1> = 1;

		template<typename T>
		constexpr auto Value<bool, T, 1, 2> = 2;

		template<typename T>
		constexpr auto Value<bool, T, 1, 2, 3> = 3;

		template<typename T>
		constexpr auto Value<T, char, 4> = 4;

		template<typename T>
		constexpr auto Value<T, char, 4, 5> = 5;

		template<typename T>
		constexpr auto Value<T, char, 4, 5, 6> = 6;

		template<>
		constexpr auto Value<bool, char, 7> = 7; // 1, 4

		template<>
		constexpr auto Value<bool, char, 7, 8> = 8; // 2, 5

		template<>
		constexpr auto Value<bool, char, 7, 8, 9> = 9; // 3, 6

		template<typename T, typename U, int... vs>
		constexpr auto Value<T(U, int(&...array)[vs]), int> = 10;

		template<typename T>
		constexpr auto Value<bool(T, int(&)[1]), int> = 11; // 10

		template<typename T>
		constexpr auto Value<bool(T, int(&)[1], int(&)[2]), int> = 12; // 10

		template<typename T>
		constexpr auto Value<bool(T, int(&)[1], int(&)[2], int(&)[3]), int> = 13; // 10

		template<typename T>
		constexpr auto Value<T(char, int(&)[1]), int> = 14; // 10

		template<typename T>
		constexpr auto Value<T(char, int(&)[1], int(&)[2]), int> = 15; // 10

		template<typename T>
		constexpr auto Value<T(char, int(&)[1], int(&)[2], int(&)[3]), int> = 16; // 10

		template<>
		constexpr auto Value<bool(char, int(&)[1]), int> = 17; // 11, 14

		template<>
		constexpr auto Value<bool(char, int(&)[1], int(&)[2]), int> = 18; // 12, 15

		template<>
		constexpr auto Value<bool(char, int(&)[1], int(&)[2], int(&)[3]), int> = 19; // 13, 16
	);
}

namespace Input__TestParsePSOrder_PartialOrderEvaluation_Qualifiers
{
	TEST_DECL(
		template<typename T>
		constexpr auto Value = 0;

		template<typename T>
		constexpr auto Value<const T> = 1;

		template<typename T>
		constexpr auto Value<volatile T> = 2;

		template<typename T>
		constexpr auto Value<const volatile T> = 3; // 1, 2

		template<typename T, int Size>
		constexpr auto Value<T[Size]> = 4;

		template<typename T, int Size>
		constexpr auto Value<const T[Size]> = 5; // 1, 4

		template<typename T, int Size>
		constexpr auto Value<volatile T[Size]> = 6; // 2, 4

		template<typename T, int Size>
		constexpr auto Value<const volatile T[Size]> = 7; // 3, 5, 6

		template<typename T, int Size1, int Size2>
		constexpr auto Value<T[Size1][Size2]> = 8; // 4

		template<typename T, int Size1, int Size2>
		constexpr auto Value<const T[Size1][Size2]> = 9; // 5, 8

		template<typename T, int Size1, int Size2>
		constexpr auto Value<volatile T[Size1][Size2]> = 10; // 6, 8

		template<typename T, int Size1, int Size2>
		constexpr auto Value<const volatile T[Size1][Size2]> = 11; // 7, 9, 10
	);
}

TEST_FILE
{
	TEST_CATEGORY(L"Functions")
	{
		auto input = LR"(
template<typename T> void Function(T){}
template<> int Function<float>(float);
template<> double Function<double>(double);
)";

	auto output = LR"(
template<typename T>
Function: void (T)
{
}
template<>
__forward Function<float>: int (float);
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
					auto& parents = decl->symbol->GetPSParents_NF();
					TEST_CASE_ASSERT(decl->symbol->GetPSPrimary_NF() == primary);
					TEST_CASE_ASSERT(parents.Count() == 1);
					TEST_CASE_ASSERT(parents[0] == primary);
				}
				else
				{
					primary = decl->symbol;
				}
			}
		}
		TEST_CASE_ASSERT(primary->GetPSPrimaryVersion_NF() == 2);
	});

	TEST_CATEGORY(L"Classes")
	{
		auto input = LR"(
template<typename T>
struct Obj
{
};

template<typename T>
struct Obj<T&>
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
struct Obj<T &>
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
					auto& parents = decl->symbol->GetPSParents_NF();
					TEST_CASE_ASSERT(decl->symbol->GetPSPrimary_NF() == primary);
					TEST_CASE_ASSERT(parents.Count() == 1);
					TEST_CASE_ASSERT(parents[0] == primary);
				}
				else
				{
					primary = decl->symbol;
				}
			}
		}
		TEST_CASE_ASSERT(primary->GetPSPrimaryVersion_NF() == 2);
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
	int Method<float>(T, float);

	template<>
	int Method<double>(T, double);
};
)";

		auto output = LR"(
template<typename T>
struct Obj
{
	public template<typename U>
	__forward Method: void (T, U);
	public template<>
	__forward Method<float>: int (T, float);
	public template<>
	__forward Method<double>: int (T, double);
};
)";

		COMPILE_PROGRAM(program, pa, input);
		AssertProgram(program, output);
	});

	auto testPartialOrder = [](Ptr<Program> program, const ParsingArguments& pa, const Pair<vint, vint> yesRaw[], vint yesCount)
	{
		// ensure all partial specializations point to the same primary symbol

		Symbol* primary = nullptr;
		List<Ptr<ValueAliasDeclaration>> decls;
		TEST_CATEGORY(L"Primary should record all descendants")
		{
			for (vint i = 0; i < program->decls.Count(); i++)
			{
				auto decl = program->decls[i];
				if (decl->name.name == L"Value")
				{
					if (primary)
					{
						TEST_CASE_ASSERT(decl->symbol->GetPSPrimary_NF() == primary);
						TEST_CASE_ASSERT(primary->GetPSPrimaryDescendants_NF().Contains(decl->symbol));
					}
					else
					{
						primary = decl->symbol;
					}

					auto valueDecl = decl.Cast<ValueAliasDeclaration>();
					TEST_CASE_ASSERT(valueDecl);
					decls.Add(valueDecl);
				}
			}

			TEST_CASE_ASSERT(primary->GetPSPrimaryVersion_NF() == decls.Count() - 1);
			TEST_CASE_ASSERT(primary->GetPSPrimaryDescendants_NF().Count() == decls.Count() - 1);
		});

		// calculate partial ordering relationship

		Group<vint, vint> parents, children, ancestors;

		for (vint i = 0; i < yesCount; i++)
		{
			auto p = yesRaw[i];
			parents.Add(p.value, p.key);
			children.Add(p.key, p.value);
		}

		// flatten partial ordering relationship

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
#define YES(A, B)	TEST_CASE(L"YES! " + itow(A) + L" <- " + itow(B)) { TEST_ASSERT(ANCESTOR(A, B) == true); });
#define NO(A, B)	TEST_CASE(L"NO!  " + itow(A) + L" <- " + itow(B)) { TEST_ASSERT(ANCESTOR(A, B) == false); });

		TEST_CATEGORY(L"Primary should be an ancestor of any partial specializations")
		{
			for (vint i = 1; i < decls.Count(); i++)
			{
				YES(0, i);
				NO(i, 0);
			}
		});

		TEST_CATEGORY(L"Ancestor relationship should be reflexive")
		{
			for (vint i = 0; i < decls.Count(); i++)
			{
				YES(i, i);
			}
		});

		TEST_CATEGORY(L"Ancestor relationship should be transitive but not symmetric")
		{
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
		});

		TEST_CATEGORY(L"All untested pairs should fail the test")
		{
			for (vint i = 1; i < decls.Count(); i++)
			{
				for (vint j = i + 1; j < decls.Count(); j++)
				{
					if (!ancestors.Contains(i, j) && !ancestors.Contains(j, i))
					{
						NO(i, j);
						NO(j, i);
					}
				}
			}
		});

#undef YES
#undef NO

		// test ordering for each declaration

		TEST_CATEGORY(L"Parent - Child relationship should correct")
		{
			for (vint i = 0; i < decls.Count(); i++)
			{
				auto symbol = decls[i]->symbol;
				vint index = parents.Keys().IndexOf(i);

				TEST_CASE(L"Parent: " + itow(i) + L" -> " + (index == -1 ? L"EMPTY" : 
					From(parents.GetByIndex(index))
					.Select(itow)
					.Aggregate([](const WString& a, const WString& b) { return a + L", " + b; })
					))
				{
					if (index == -1)
					{
						if (symbol == primary)
						{
							TEST_ASSERT(symbol->GetPSParents_NF().Count() == 0);
						}
						else
						{
							TEST_ASSERT(symbol->GetPSParents_NF().Count() == 1);
							TEST_ASSERT(symbol->GetPSParents_NF()[0] == primary);
						}
					}
					else
					{
						auto& ps = parents.GetByIndex(index);
						TEST_ASSERT(ps.Count() == symbol->GetPSParents_NF().Count());
						for (vint j = 0; j < ps.Count(); j++)
						{
							auto pSymbol = decls[ps[j]]->symbol;
							TEST_ASSERT(symbol->GetPSParents_NF().Contains(pSymbol));
							TEST_ASSERT(pSymbol->GetPSChildren_NF().Contains(symbol));
						}
					}
				});
			}
		});
	};

	TEST_CATEGORY(L"Partial Order Evaluation")
	{
		using namespace Input__TestParsePSOrder_PartialOrderEvaluation_Type;
		COMPILE_PROGRAM(program, pa, input);

		const Pair<vint, vint> yesRaw[] = {
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
			{24,1},
			{6,25},
			{25,26},
			{9,26},
			{6,27},
			{25,28},
			{9,29},
			{26,30}
		};

		testPartialOrder(program, pa, yesRaw, _countof(yesRaw));
	});

	TEST_CATEGORY(L"Partial Order Evaluation (value argument)")
	{
		using namespace Input__TestParsePSOrder_PartialOrderEvaluation_Value;
		COMPILE_PROGRAM(program, pa, input);

		const Pair<vint, vint> yesRaw[] = {
			{1,7},
			{2,8},
			{3,9},
			{4,7},
			{5,8},
			{6,9},
			{10,11},
			{10,12},
			{10,13},
			{10,14},
			{10,15},
			{10,16},
			{11,17},
			{12,18},
			{13,19},
			{14,17},
			{15,18},
			{16,19}
		};

		testPartialOrder(program, pa, yesRaw, _countof(yesRaw));
	});

	TEST_CATEGORY(L"Partial Order Evaluation (const volatile array)")
	{
		using namespace Input__TestParsePSOrder_PartialOrderEvaluation_Qualifiers;
		COMPILE_PROGRAM(program, pa, input);

		const Pair<vint, vint> yesRaw[] = {
			{1,3},
			{1,5},
			{2,3},
			{2,6},
			{3,7},
			{4,5},
			{4,8},
			{4,6},
			{5,7},
			{5,9},
			{6,7},
			{6,10},
			{7,11},
			{8,10},
			{8,9},
			{9,11},
			{10,11}
		};

		testPartialOrder(program, pa, yesRaw, _countof(yesRaw));
	});
}