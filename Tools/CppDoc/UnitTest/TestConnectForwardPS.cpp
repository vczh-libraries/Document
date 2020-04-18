#include "Util.h"

TEST_FILE
{
	TEST_CATEGORY(L"Functions")
	{
		auto input = LR"(
template<typename T, typename U, typename... Ts>	void F										(T*, U(*...)(Ts&));
template<typename T, typename U, typename... Ts>	void F										(T, U(*...)(Ts));
template<typename T, typename U, typename... Ts>	void G										(T*, U(*...)(Ts&));
template<typename T, typename U, typename... Ts>	void G										(T, U(*...)(Ts));

template<>											void F<bool, bool, float, double>			(bool*, bool(*)(float&), bool(*)(double&));
template<>											void F<void, void, char, wchar_t, bool>		(void*, void(*)(char&), void(*)(wchar_t&), void(*)(bool&));
template<>											void F<char, char>							(char*);

template<>											void F<bool*, bool, float&, double&>		(bool*, bool(*)(float&), bool(*)(double&));
template<>											void F<void*, void, char&, wchar_t&, bool&>	(void*, void(*)(char&), void(*)(wchar_t&), void(*)(bool&));
template<>											void F<char*, char>							(char*);

template<>											void G<bool, bool, float, double>			(bool*, bool(*)(float&), bool(*)(double&));
template<>											void G<void, void, char, wchar_t, bool>		(void*, void(*)(char&), void(*)(wchar_t&), void(*)(bool&));
template<>											void G<char, char>							(char*);

template<>											void G<bool*, bool, float&, double&>		(bool*, bool(*)(float&), bool(*)(double&));
template<>											void G<void*, void, char&, wchar_t&, bool&>	(void*, void(*)(char&), void(*)(wchar_t&), void(*)(bool&));
template<>											void G<char*, char>							(char*);

template<typename U, typename V, typename... Us>	void F										(U*, V(*...)(Us&)){}
template<typename U, typename V, typename... Us>	void F										(U, V(*...)(Us)){}
template<typename U, typename V, typename... Us>	void G										(U*, V(*...)(Us&)){}
template<typename U, typename V, typename... Us>	void G										(U, V(*...)(Us)){}

template<>											void F<bool, bool, float, double>			(bool*, bool(*)(float&), bool(*)(double&)){}
template<>											void F<void, void, char, wchar_t, bool>		(void*, void(*)(char&), void(*)(wchar_t&), void(*)(bool&)){}
template<>											void F<char, char>							(char*){}

template<>											void F<bool*, bool, float&, double&>		(bool*, bool(*)(float&), bool(*)(double&)){}
template<>											void F<void*, void, char&, wchar_t&, bool&>	(void*, void(*)(char&), void(*)(wchar_t&), void(*)(bool&)){}
template<>											void F<char*, char>							(char*){}

template<>											void G<bool, bool, float, double>			(bool*, bool(*)(float&), bool(*)(double&)){}
template<>											void G<void, void, char, wchar_t, bool>		(void*, void(*)(char&), void(*)(wchar_t&), void(*)(bool&)){}
template<>											void G<char, char>							(char*){}

template<>											void G<bool*, bool, float&, double&>		(bool*, bool(*)(float&), bool(*)(double&)){}
template<>											void G<void*, void, char&, wchar_t&, bool&>	(void*, void(*)(char&), void(*)(wchar_t&), void(*)(bool&)){}
template<>											void G<char*, char>							(char*){}
)";

		COMPILE_PROGRAM(program, pa, input);

		List<Ptr<ForwardFunctionDeclaration>> ffs[2];
		List<Ptr<FunctionDeclaration>> fs[2];
		for (vint i = 0; i < program->decls.Count(); i++)
		{
			auto decl = program->decls[i];
			if (auto fdecl = decl.Cast<FunctionDeclaration>())
			{
				if (fdecl->name.name == L"F") fs[0].Add(fdecl);
				if (fdecl->name.name == L"G") fs[1].Add(fdecl);
			}
			else if (auto ffdecl = decl.Cast<ForwardFunctionDeclaration>())
			{
				if (ffdecl->name.name == L"F") ffs[0].Add(ffdecl);
				if (ffdecl->name.name == L"G") ffs[1].Add(ffdecl);
			}
		}

		TEST_CASE_ASSERT(ffs[0].Count() == 8);
		TEST_CASE_ASSERT(ffs[1].Count() == 8);
		TEST_CASE_ASSERT(fs[0].Count() == 8);
		TEST_CASE_ASSERT(fs[1].Count() == 8);
		for (vint i = 0; i < 2; i++)
		{
			TEST_CATEGORY(WString(L"Test function: ") + (i == 0 ? L"F" : L"G"))
			{
				TEST_CATEGORY(L"Check connections")
				{
					for (vint j = 0; j < 8; j++)
					{
						auto symbol = ffs[i][j]->symbol->GetFunctionSymbol_Fb();
						TEST_CASE_ASSERT(symbol == fs[i][j]->symbol->GetFunctionSymbol_Fb());
					}
				});

				for (vint j = 0; j < 2; j++)
				{
					TEST_CATEGORY(L"Check partial specialization relationship for group: " + itow(j))
					{
						auto primary = ffs[i][j]->symbol->GetFunctionSymbol_Fb();
						for (vint k = 0; k < 4; k++)
						{
							if (k == 0)
							{
								TEST_CASE_ASSERT(primary->IsPSPrimary_NF());
								TEST_CASE_ASSERT(primary->GetPSPrimaryDescendants_NF().Count() == 3);
								TEST_CASE_ASSERT(primary->GetPSChildren_NF().Count() == 3);
							}
							else
							{
								auto symbol = ffs[i][1 + j * 3 + k]->symbol->GetFunctionSymbol_Fb();
								TEST_CASE_ASSERT(symbol->GetPSPrimary_NF() == primary);
								TEST_CASE_ASSERT(symbol->GetPSParents_NF().Count() == 1);
								TEST_CASE_ASSERT(symbol->GetPSParents_NF()[0] == primary);
							}
						}
					});
				}
			});
		}
	});

	TEST_CATEGORY(L"Methods")
	{
		// TODO:
	});
}