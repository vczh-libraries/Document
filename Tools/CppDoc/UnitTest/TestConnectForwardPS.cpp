#include "Util.h"

TEST_FILE
{
	TEST_CATEGORY(L"Functions")
	{
		auto input = LR"(
template<typename T, typename... Ts>	void F									(T*, T(*...)(Ts&));
template<typename T, typename... Ts>	void F									(T, T(*...)(Ts));
template<typename T, typename... Ts>	void G									(T*, T(*...)(Ts&));
template<typename T, typename... Ts>	void G									(T, T(*...)(Ts));

template<>								void F<bool, float, double>				(bool*, bool(*)(float&), bool(*)(double&));
template<>								void F<void, char, wchar_t, bool>		(void*, void(*)(char&), void(*)(wchar_t&), void(*)(bool&));
template<>								void F<char>							(char*);

template<>								void F<bool*, float&, double&>			(bool*, bool(*)(float), bool(*)(double));
template<>								void F<void*, char&, wchar_t&, bool&>	(void*, void(*)(char), void(*)(wchar_t), void(*)(bool));
template<>								void F<char>							(char*);

template<>								void G<bool, float, double>				(bool*, bool(*)(float&), bool(*)(double&));
template<>								void G<void, char, wchar_t, bool>		(void*, void(*)(char&), void(*)(wchar_t&), void(*)(bool&));
template<>								void G<char>							(char*);

template<>								void G<bool*, float&, double&>			(bool*, bool(*)(float), bool(*)(double));
template<>								void G<void*, char&, wchar_t&, bool&>	(void*, void(*)(char), void(*)(wchar_t), void(*)(bool));
template<>								void G<char>							(char*);

template<typename U, typename... Us>	void F									(U*, U(*...)(Us&)){}
template<typename U, typename... Us>	void F									(U, U(*...)(Us)){}
template<typename U, typename... Us>	void G									(U*, U(*...)(Us&)){}
template<typename U, typename... Us>	void G									(U, U(*...)(Us)){}

template<>								void F<bool, float, double>				(bool*, bool(*)(float&), bool(*)(double&)){}
template<>								void F<void, char, wchar_t, bool>		(void*, void(*)(char&), void(*)(wchar_t&), void(*)(bool&)){}
template<>								void F<char>							(char*){}

template<>								void F<bool*, float&, double&>			(bool*, bool(*)(float), bool(*)(double)){}
template<>								void F<void*, char&, wchar_t&, bool&>	(void*, void(*)(char), void(*)(wchar_t), void(*)(bool)){}
template<>								void F<char>							(char*){}

template<>								void G<bool, float, double>				(bool*, bool(*)(float&), bool(*)(double&)){}
template<>								void G<void, char, wchar_t, bool>		(void*, void(*)(char&), void(*)(wchar_t&), void(*)(bool&)){}
template<>								void G<char>							(char*){}

template<>								void G<bool*, float&, double&>			(bool*, bool(*)(float), bool(*)(double)){}
template<>								void G<void*, char&, wchar_t&, bool&>	(void*, void(*)(char), void(*)(wchar_t), void(*)(bool)){}
template<>								void G<char>							(char*){}
)";

		auto output = LR"(
template<typename T, typename ...Ts>
__forward F: void (T *, T (Ts &) *...);
template<typename T, typename ...Ts>
__forward F: void (T, T (Ts) *...);
template<typename T, typename ...Ts>
__forward G: void (T *, T (Ts &) *...);
template<typename T, typename ...Ts>
__forward G: void (T, T (Ts) *...);
template<>
__forward F<bool, float, double>: void (bool *, bool (float &) *, bool (double &) *);
template<>
__forward F<void, char, wchar_t, bool>: void (void *, void (char &) *, void (wchar_t &) *, void (bool &) *);
template<>
__forward F<char>: void (char *);
template<>
__forward F<bool *, float &, double &>: void (bool *, bool (float) *, bool (double) *);
template<>
__forward F<void *, char &, wchar_t &, bool &>: void (void *, void (char) *, void (wchar_t) *, void (bool) *);
template<>
__forward F<char>: void (char *);
template<>
__forward G<bool, float, double>: void (bool *, bool (float &) *, bool (double &) *);
template<>
__forward G<void, char, wchar_t, bool>: void (void *, void (char &) *, void (wchar_t &) *, void (bool &) *);
template<>
__forward G<char>: void (char *);
template<>
__forward G<bool *, float &, double &>: void (bool *, bool (float) *, bool (double) *);
template<>
__forward G<void *, char &, wchar_t &, bool &>: void (void *, void (char) *, void (wchar_t) *, void (bool) *);
template<>
__forward G<char>: void (char *);
template<typename U, typename ...Us>
F: void (U *, U (Us &) *...)
{
}
template<typename U, typename ...Us>
F: void (U, U (Us) *...)
{
}
template<typename U, typename ...Us>
G: void (U *, U (Us &) *...)
{
}
template<typename U, typename ...Us>
G: void (U, U (Us) *...)
{
}
template<>
F<bool, float, double>: void (bool *, bool (float &) *, bool (double &) *)
{
}
template<>
F<void, char, wchar_t, bool>: void (void *, void (char &) *, void (wchar_t &) *, void (bool &) *)
{
}
template<>
F<char>: void (char *)
{
}
template<>
F<bool *, float &, double &>: void (bool *, bool (float) *, bool (double) *)
{
}
template<>
F<void *, char &, wchar_t &, bool &>: void (void *, void (char) *, void (wchar_t) *, void (bool) *)
{
}
template<>
F<char>: void (char *)
{
}
template<>
G<bool, float, double>: void (bool *, bool (float &) *, bool (double &) *)
{
}
template<>
G<void, char, wchar_t, bool>: void (void *, void (char &) *, void (wchar_t &) *, void (bool &) *)
{
}
template<>
G<char>: void (char *)
{
}
template<>
G<bool *, float &, double &>: void (bool *, bool (float) *, bool (double) *)
{
}
template<>
G<void *, char &, wchar_t &, bool &>: void (void *, void (char) *, void (wchar_t) *, void (bool) *)
{
}
template<>
G<char>: void (char *)
{
}
)";

		COMPILE_PROGRAM(program, pa, input);
		AssertProgram(program, output);

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
			for (vint j = 0; j < 8; j++)
			{
				auto symbol = ffs[i][j]->symbol;
				TEST_CASE_ASSERT(symbol == fs[i][j]->symbol);
				symbol = symbol->GetFunctionSymbol_Fb();

				if (j < 2)
				{
					TEST_CASE_ASSERT(symbol->IsPSPrimary_NF());
					TEST_CASE_ASSERT(symbol->GetPSChildren_NF().Count() == 3);
				}
				else if (j < 5)
				{
					TEST_CASE_ASSERT(symbol->GetPSPrimary_NF() == ffs[i][0]->symbol->GetFunctionSymbol_Fb());
				}
				else if (j < 8)
				{
					TEST_CASE_ASSERT(symbol->GetPSPrimary_NF() == ffs[i][1]->symbol->GetFunctionSymbol_Fb());
				}
			}
		}
	});

	TEST_CATEGORY(L"Classes")
	{
		// TODO:
	});

	TEST_CATEGORY(L"Methods")
	{
		// TODO:
	});
}