#include "Util.h"

TEST_FILE
{
	TEST_CASE(L"Test type conversion with free generic argument")
	{
		auto contextInput = LR"(
struct S{};
class C{};

template<typename T, T Value>
using Context = T;
)";
		COMPILE_PROGRAM(program, pa, contextInput);

		const wchar_t* typeCodes[] = {
			/*  0 */ L"T",
			/*  1 */ L"const T",
			/*  2 */ L"volatile T",
			/*  3 */ L"const volatile T",

			/*  4 */ L"T*",
			/*  5 */ L"const T*",
			/*  6 */ L"volatile T*",
			/*  7 */ L"const volatile T*",

			/*  8 */ L"T&",
			/*  9 */ L"const T&",
			/* 10 */ L"volatile T&",
			/* 11 */ L"const volatile T&",

			/* 12 */ L"T&&",
			/* 13 */ L"const T&&",
			/* 14 */ L"volatile T&&",
			/* 15 */ L"const volatile T&&",

			/* 16 */ L"T[10]",
			/* 17 */ L"const T[10]",
			/* 18 */ L"volatile T[10]",
			/* 19 */ L"const volatile T[10]",

			/* 20 */ L"T(*)[10]",
			/* 21 */ L"const T(*)[10]",
			/* 22 */ L"volatile T(*)[10]",
			/* 23 */ L"const volatile T(*)[10]",

			/* 24 */ L"T(&)[10]",
			/* 25 */ L"const T(&)[10]",
			/* 26 */ L"volatile T(&)[10]",
			/* 27 */ L"const volatile T(&)[10]",

			/* 28 */ L"T(&&)[10]",
			/* 29 */ L"const T(&&)[10]",
			/* 30 */ L"volatile T(&&)[10]",
			/* 31 */ L"const volatile T(&&)[10]",

			/* 32 */ L"T C::*",
			/* 33 */ L"C T::*",
			/* 34 */ L"decltype({Value})",
			/* 35 */ L"decltype({Value,C()})",
			/* 36 */ L"decltype({C(),Value})",
			/* 37 */ L"T(*)(C)",
			/* 38 */ L"C(*)(T)",
			/* 39 */ L"T(*)()",
			/* 40 */ L"T(*[10])(C)",
			/* 41 */ L"C(*[10])(T)",
			/* 42 */ L"T(*[10])()",
			/* 43 */ L"T(*&)(C)",
			/* 44 */ L"C(*&)(T)",
			/* 45 */ L"T(*&)()",
			/* 46 */ L"T(*&&)(C)",
			/* 47 */ L"C(*&&)(T)",
			/* 48 */ L"T(*&&)()",
		};

		const int TypeCount = sizeof(typeCodes) / sizeof(*typeCodes);

		// E = Exact, * = Any, SPACE = Illegal

		// conversion from generic to concrete
		const wchar_t g2c[TypeCount][TypeCount + 1] = {
			/*      0       4       8       12      16      20      24      28      32                */
			/*  0 */L"****" L"****" L"****" L"****" L"****" L"****" L"****" L"****" L"*****************",
					L"****" L"****" L"****" L"****" L"****" L"****" L"****" L"****" L"*****************",
					L"****" L"****" L"****" L"****" L"****" L"****" L"****" L"****" L"*****************",
					L"****" L"****" L"****" L"****" L"****" L"****" L"****" L"****" L"*****************",

			/*  4 */L"    " L"****" L"    " L"    " L"    " L"****" L"    " L"    " L"     ***      ***",
					L"    " L" * *" L"    " L"    " L"    " L"    " L"    " L"    " L"                 ",
					L"    " L"  **" L"    " L"    " L"    " L"    " L"    " L"    " L"                 ",
					L"    " L"   *" L"    " L"    " L"    " L"    " L"    " L"    " L"                 ",

			/*  8 */L"****" L"****" L"****" L"****" L"****" L"****" L"****" L"****" L"*****************",
					L"****" L"****" L"****" L"****" L"****" L"****" L"****" L"****" L"*****************",
					L"****" L"****" L"****" L"****" L"****" L"****" L"****" L"****" L"*****************",
					L"****" L"****" L"****" L"****" L"****" L"****" L"****" L"****" L"*****************",

			/* 12 */L"****" L"****" L"****" L"****" L"****" L"****" L"****" L"****" L"*****************",
					L"****" L"****" L"****" L"****" L"****" L"****" L"****" L"****" L"*****************",
					L"****" L"****" L"****" L"****" L"****" L"****" L"****" L"****" L"*****************",
					L"****" L"****" L"****" L"****" L"****" L"****" L"****" L"****" L"*****************",

			/* 16 */L"    " L"****" L"    " L"    " L"****" L"****" L"    " L"****" L"     ******   ***",
					L"    " L" * *" L"    " L"    " L" * *" L"    " L"    " L" * *" L"                 ",
					L"    " L"  **" L"    " L"    " L"  **" L"    " L"    " L"  **" L"                 ",
					L"    " L"   *" L"    " L"    " L"   *" L"    " L"    " L"   *" L"                 ",

			/* 20 */L"    " L"    " L"    " L"    " L"    " L"****" L"    " L"    " L"                 ",
					L"    " L"    " L"    " L"    " L"    " L" * *" L"    " L"    " L"                 ",
					L"    " L"    " L"    " L"    " L"    " L"  **" L"    " L"    " L"                 ",
					L"    " L"    " L"    " L"    " L"    " L"   *" L"    " L"    " L"                 ",

			/* 24 */L"    " L"****" L"    " L"    " L"****" L"****" L"****" L"    " L"     ******      ",
					L"    " L" * *" L"    " L"    " L" * *" L"    " L" * *" L"    " L"                 ",
					L"    " L"  **" L"    " L"    " L"  **" L"    " L"  **" L"    " L"                 ",
					L"    " L"   *" L"    " L"    " L"   *" L"    " L"   *" L"    " L"                 ",

			/* 28 */L"    " L"****" L"    " L"    " L"****" L"****" L"    " L"****" L"     ******   ***",
					L"    " L" * *" L"    " L"    " L" * *" L"    " L"    " L" * *" L"                 ",
					L"    " L"  **" L"    " L"    " L"  **" L"    " L"    " L"  **" L"                 ",
					L"    " L"   *" L"    " L"    " L"   *" L"    " L"    " L"   *" L"                 ",

			/* 32 */L"    " L"    " L"    " L"    " L"    " L"    " L"    " L"    " L"*                ",
					L"    " L"    " L"    " L"    " L"    " L"    " L"    " L"    " L" *               ",
					L"****" L"****" L"****" L"****" L"****" L"****" L"****" L"****" L"*****************",
					L"    " L"    " L"    " L"    " L"    " L"    " L"    " L"    " L"   *             ",
					L"    " L"    " L"    " L"    " L"    " L"    " L"    " L"    " L"    *            ",
					L"    " L"    " L"    " L"    " L"    " L"    " L"    " L"    " L"     *        *  ",
					L"    " L"    " L"    " L"    " L"    " L"    " L"    " L"    " L"      *        * ",
					L"    " L"    " L"    " L"    " L"    " L"    " L"    " L"    " L"       *        *",
					L"    " L"    " L"    " L"    " L"    " L"    " L"    " L"    " L"        *        ",
					L"    " L"    " L"    " L"    " L"    " L"    " L"    " L"    " L"         *       ",
					L"    " L"    " L"    " L"    " L"    " L"    " L"    " L"    " L"          *      ",
					L"    " L"    " L"    " L"    " L"    " L"    " L"    " L"    " L"     *     *     ",
					L"    " L"    " L"    " L"    " L"    " L"    " L"    " L"    " L"      *     *    ",
					L"    " L"    " L"    " L"    " L"    " L"    " L"    " L"    " L"       *     *   ",
					L"    " L"    " L"    " L"    " L"    " L"    " L"    " L"    " L"     *        *  ",
					L"    " L"    " L"    " L"    " L"    " L"    " L"    " L"    " L"      *        * ",
					L"    " L"    " L"    " L"    " L"    " L"    " L"    " L"    " L"       *        *",
		};

		// conversion from concrete to generic
		const wchar_t c2g[TypeCount][TypeCount + 1] = {
			/*      0       4       8       12      16      20      24      28      32                */
			/*  0 */L"****" L"    " L"****" L"****" L"    " L"    " L"    " L"    " L"                 ",
					L"****" L"    " L"****" L"****" L"    " L"    " L"    " L"    " L"                 ",
					L"****" L"    " L"****" L"****" L"    " L"    " L"    " L"    " L"                 ",
					L"****" L"    " L"****" L"****" L"    " L"    " L"    " L"    " L"                 ",

			/*  4 */L"****" L"****" L"****" L"****" L"    " L"    " L"    " L"    " L"                 ",
					L"****" L"****" L"****" L"****" L"    " L"    " L"    " L"    " L"                 ",
					L"****" L"****" L"****" L"****" L"    " L"    " L"    " L"    " L"                 ",
					L"****" L"****" L"****" L"****" L"    " L"    " L"    " L"    " L"                 ",

			/*  8 */L"****" L"    " L"****" L"****" L"    " L"    " L"    " L"    " L"                 ",
					L"****" L"    " L"****" L"****" L"    " L"    " L"    " L"    " L"                 ",
					L"****" L"    " L"****" L"****" L"    " L"    " L"    " L"    " L"                 ",
					L"****" L"    " L"****" L"****" L"    " L"    " L"    " L"    " L"                 ",

			/* 12 */L"****" L"    " L"****" L"****" L"    " L"    " L"    " L"    " L"                 ",
					L"****" L"    " L"****" L"****" L"    " L"    " L"    " L"    " L"                 ",
					L"****" L"    " L"****" L"****" L"    " L"    " L"    " L"    " L"                 ",
					L"****" L"    " L"****" L"****" L"    " L"    " L"    " L"    " L"                 ",

			/* 16 */L"****" L"****" L"****" L"****" L"****" L"    " L"    " L"****" L"                 ",
					L"****" L"****" L"****" L"****" L"****" L"    " L"    " L"****" L"                 ",
					L"****" L"****" L"****" L"****" L"****" L"    " L"    " L"****" L"                 ",
					L"****" L"****" L"****" L"****" L"****" L"    " L"    " L"****" L"                 ",

			/* 20 */L"****" L"****" L"****" L"****" L"    " L"****" L"    " L"    " L"                 ",
					L"****" L"****" L"****" L"****" L"    " L"****" L"    " L"    " L"                 ",
					L"****" L"****" L"****" L"****" L"    " L"****" L"    " L"    " L"                 ",
					L"****" L"****" L"****" L"****" L"    " L"****" L"    " L"    " L"                 ",

			/* 24 */L"****" L"****" L"****" L"****" L"****" L"    " L"****" L"    " L"                 ",
					L"****" L"****" L"****" L"****" L"****" L"    " L"****" L"    " L"                 ",
					L"****" L"****" L"****" L"****" L"****" L"    " L"****" L"    " L"                 ",
					L"****" L"****" L"****" L"****" L"****" L"    " L"****" L"    " L"                 ",

			/* 28 */L"****" L"****" L"****" L"****" L"****" L"    " L"    " L"****" L"                 ",
					L"****" L"****" L"****" L"****" L"****" L"    " L"    " L"****" L"                 ",
					L"****" L"****" L"****" L"****" L"****" L"    " L"    " L"****" L"                 ",
					L"****" L"****" L"****" L"****" L"****" L"    " L"    " L"****" L"                 ",

			/* 32 */L"****" L"    " L"****" L"****" L"    " L"    " L"    " L"    " L"*                ",
					L"****" L"    " L"****" L"****" L"    " L"    " L"    " L"    " L" *               ",
					L"****" L"    " L"****" L"****" L"    " L"    " L"    " L"    " L"  *              ",
					L"****" L"    " L"****" L"****" L"    " L"    " L"    " L"    " L"   *             ",
					L"****" L"    " L"****" L"****" L"    " L"    " L"    " L"    " L"    *            ",
					L"****" L"****" L"****" L"****" L"    " L"    " L"    " L"    " L"     *        *  ",
					L"****" L"****" L"****" L"****" L"    " L"    " L"    " L"    " L"      *        * ",
					L"****" L"****" L"****" L"****" L"    " L"    " L"    " L"    " L"       *        *",
					L"****" L"****" L"****" L"****" L"****" L"    " L"    " L"****" L"        *        ",
					L"****" L"****" L"****" L"****" L"****" L"    " L"    " L"****" L"         *       ",
					L"****" L"****" L"****" L"****" L"****" L"    " L"    " L"****" L"          *      ",
					L"****" L"****" L"****" L"****" L"    " L"    " L"    " L"    " L"     *     *     ",
					L"****" L"****" L"****" L"****" L"    " L"    " L"    " L"    " L"      *     *    ",
					L"****" L"****" L"****" L"****" L"    " L"    " L"    " L"    " L"       *     *   ",
					L"****" L"****" L"****" L"****" L"    " L"    " L"    " L"    " L"     *        *  ",
					L"****" L"****" L"****" L"****" L"    " L"    " L"    " L"    " L"      *        * ",
					L"****" L"****" L"****" L"****" L"    " L"    " L"    " L"    " L"       *        *",
		};

		// conversion from generic to generic
		const wchar_t g2g[TypeCount][TypeCount + 1] = {
			/*      0       4       8       12      16      20      24      28      32                */
			/*  0 */L"****" L"****" L"****" L"****" L"****" L"****" L"****" L"****" L"*****************",
					L"****" L"****" L"****" L"****" L"****" L"****" L"****" L"****" L"*****************",
					L"****" L"****" L"****" L"****" L"****" L"****" L"****" L"****" L"*****************",
					L"****" L"****" L"****" L"****" L"****" L"****" L"****" L"****" L"*****************",

			/*  4 */L"****" L"****" L"****" L"****" L"    " L"****" L"    " L"    " L"     ***      ***",
					L"****" L"****" L"****" L"****" L"    " L"    " L"    " L"    " L"                 ",
					L"****" L"****" L"****" L"****" L"    " L"    " L"    " L"    " L"                 ",
					L"****" L"****" L"****" L"****" L"    " L"    " L"    " L"    " L"                 ",

			/*  8 */L"****" L"****" L"****" L"****" L"****" L"****" L"****" L"****" L"*****************",
					L"****" L"****" L"****" L"****" L"****" L"****" L"****" L"****" L"*****************",
					L"****" L"****" L"****" L"****" L"****" L"****" L"****" L"****" L"*****************",
					L"****" L"****" L"****" L"****" L"****" L"****" L"****" L"****" L"*****************",

			/* 12 */L"****" L"****" L"****" L"****" L"****" L"****" L"****" L"****" L"*****************",
					L"****" L"****" L"****" L"****" L"****" L"****" L"****" L"****" L"*****************",
					L"****" L"****" L"****" L"****" L"****" L"****" L"****" L"****" L"*****************",
					L"****" L"****" L"****" L"****" L"****" L"****" L"****" L"****" L"*****************",

			/* 16 */L"****" L"****" L"****" L"****" L"****" L"****" L"    " L"****" L"     ******   ***",
					L"****" L"****" L"****" L"****" L"****" L"    " L"    " L"****" L"                 ",
					L"****" L"****" L"****" L"****" L"****" L"    " L"    " L"****" L"                 ",
					L"****" L"****" L"****" L"****" L"****" L"    " L"    " L"****" L"                 ",

			/* 20 */L"****" L"****" L"****" L"****" L"    " L"****" L"    " L"    " L"                 ",
					L"****" L"****" L"****" L"****" L"    " L"****" L"    " L"    " L"                 ",
					L"****" L"****" L"****" L"****" L"    " L"****" L"    " L"    " L"                 ",
					L"****" L"****" L"****" L"****" L"    " L"****" L"    " L"    " L"                 ",

			/* 24 */L"****" L"****" L"****" L"****" L"****" L"****" L"****" L"    " L"     ******      ",
					L"****" L"****" L"****" L"****" L"****" L"    " L"****" L"    " L"                 ",
					L"****" L"****" L"****" L"****" L"****" L"    " L"****" L"    " L"                 ",
					L"****" L"****" L"****" L"****" L"****" L"    " L"****" L"    " L"                 ",

			/* 28 */L"****" L"****" L"****" L"****" L"****" L"****" L"    " L"****" L"     ******   ***",
					L"****" L"****" L"****" L"****" L"****" L"    " L"    " L"****" L"                 ",
					L"****" L"****" L"****" L"****" L"****" L"    " L"    " L"****" L"                 ",
					L"****" L"****" L"****" L"****" L"****" L"    " L"    " L"****" L"                 ",

			/* 32 */L"****" L"    " L"****" L"****" L"    " L"    " L"    " L"    " L"**               ",
					L"****" L"    " L"****" L"****" L"    " L"    " L"    " L"    " L"**               ",
					L"****" L"****" L"****" L"****" L"****" L"****" L"****" L"****" L"*****************",
					L"****" L"    " L"****" L"****" L"    " L"    " L"    " L"    " L"   **            ",
					L"****" L"    " L"****" L"****" L"    " L"    " L"    " L"    " L"   **            ",
					L"****" L"****" L"****" L"****" L"    " L"    " L"    " L"    " L"     **       ** ",
					L"****" L"****" L"****" L"****" L"    " L"    " L"    " L"    " L"     **       ** ",
					L"****" L"****" L"****" L"****" L"    " L"    " L"    " L"    " L"       *        *",
					L"****" L"****" L"****" L"****" L"****" L"    " L"    " L"****" L"        **       ",
					L"****" L"****" L"****" L"****" L"****" L"    " L"    " L"****" L"        **       ",
					L"****" L"****" L"****" L"****" L"****" L"    " L"    " L"****" L"          *      ",
					L"****" L"****" L"****" L"****" L"    " L"    " L"    " L"    " L"     **    **    ",
					L"****" L"****" L"****" L"****" L"    " L"    " L"    " L"    " L"     **    **    ",
					L"****" L"****" L"****" L"****" L"    " L"    " L"    " L"    " L"       *     *   ",
					L"****" L"****" L"****" L"****" L"    " L"    " L"    " L"    " L"     **       ** ",
					L"****" L"****" L"****" L"****" L"    " L"    " L"    " L"    " L"     **       ** ",
					L"****" L"****" L"****" L"****" L"    " L"    " L"    " L"    " L"       *        *",
		};

		ITsys* genericTypes[TypeCount];
		ITsys* intTypes[TypeCount];
		ITsys* structTypes[TypeCount];

		auto contextSymbol = pa.scopeSymbol->TryGetChildren_NFb(L"Context")->Get(0).Obj();
		auto spa = pa.WithScope(contextSymbol);
		{
			for (vint i = 0; i < TypeCount; i++)
			{
				TOKEN_READER(typeCodes[i]);
				auto cursor = reader.GetFirstToken();
				auto type = ParseType(spa, cursor);
				TEST_ASSERT(!cursor);

				TypeTsysList tsys;
				TypeToTsysNoVta(spa, type, tsys);
				TEST_ASSERT(tsys.Count() == 1);
				genericTypes[i] = tsys[0];
			}

			for (vint i = 0; i < TypeCount; i++)
			{
				TypeTsysList tsys;
				TemplateArgumentContext taContext;
				taContext.arguments.Add(genericTypes[0], pa.tsys->Int());
				intTypes[i] = genericTypes[i]->ReplaceGenericArgs(pa.AppendSingleLevelArgs(taContext));
			}

			for (vint i = 0; i < TypeCount; i++)
			{
				TypeTsysList tsys;
				TemplateArgumentContext taContext;
				taContext.arguments.Add(genericTypes[0], pa.tsys->DeclOf(pa.scopeSymbol->TryGetChildren_NFb(L"S")->Get(0).Obj()));
				structTypes[i] = genericTypes[i]->ReplaceGenericArgs(pa.AppendSingleLevelArgs(taContext));
			}
		}

		for (vint i = 0; i < TypeCount; i++)
		{
			auto fromType = genericTypes[i];
			for (vint j = 0; j < TypeCount; j++)
			{
				{
					auto toType = intTypes[j];
					auto result = TestTypeConversion(spa, toType, { nullptr,ExprTsysType::PRValue,fromType });
					auto expect = g2c[i][j];
					TEST_ASSERT(expect == L'*' ? result.anyInvolved : result.cat == TypeConvCat::Illegal);
				}
				{
					auto toType = structTypes[j];
					auto result = TestTypeConversion(spa, toType, { nullptr,ExprTsysType::PRValue,fromType });
					auto expect = g2c[i][j];
					TEST_ASSERT(expect == L'*' ? result.anyInvolved : result.cat == TypeConvCat::Illegal);
				}
			}
		}

		for (vint i = 0; i < TypeCount; i++)
		{
			{
				auto fromType = intTypes[i];
				for (vint j = 0; j < TypeCount; j++)
				{
					auto toType = genericTypes[j];
					auto result = TestTypeConversion(spa, toType, { nullptr,ExprTsysType::PRValue,fromType });
					auto expect = c2g[i][j];
					TEST_ASSERT(expect == L'*' ? result.anyInvolved : result.cat == TypeConvCat::Illegal);
				}
			}
			{
				auto fromType = structTypes[i];
				for (vint j = 0; j < TypeCount; j++)
				{
					auto toType = genericTypes[j];
					auto result = TestTypeConversion(spa, toType, { nullptr,ExprTsysType::PRValue,fromType });
					auto expect = c2g[i][j];
					TEST_ASSERT(expect == L'*' ? result.anyInvolved : result.cat == TypeConvCat::Illegal);
				}
			}
		}

		for (vint i = 0; i < TypeCount; i++)
		{
			auto fromType = genericTypes[i];
			for (vint j = 0; j < TypeCount; j++)
			{
				auto toType = genericTypes[j];
				auto result = TestTypeConversion(spa, toType, { nullptr,ExprTsysType::PRValue,fromType });
				auto expect = g2g[i][j];
				TEST_ASSERT(expect == L'*' ? result.anyInvolved : result.cat == TypeConvCat::Illegal);
			}
		}

		for (vint i = 0; i < TypeCount; i++)
		{
			auto fromType = genericTypes[i];
			auto toType = pa.tsys->Any();
			auto result = TestTypeConversion(spa, toType, { nullptr,ExprTsysType::PRValue,fromType });
			TEST_ASSERT(result.anyInvolved);
		}

		for (vint i = 0; i < TypeCount; i++)
		{
			auto fromType = pa.tsys->Any();
			auto toType = genericTypes[i];
			auto result = TestTypeConversion(spa, toType, { nullptr,ExprTsysType::PRValue,fromType });
			TEST_ASSERT(result.anyInvolved);
		}
	});
}