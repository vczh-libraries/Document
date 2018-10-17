#include <Parser.h>
#include "Util.h"

/***********************************************************************
Predefined Types
***********************************************************************/

ITsys* GetTsysFromCppType(ITsysAlloc* tsys, const WString& cppType)
{
	ParsingArguments pa(nullptr, tsys, nullptr);
	CppTokenReader reader(GlobalCppLexer(), cppType);
	auto cursor = reader.GetFirstToken();
	auto type = ParseType(pa, cursor);
	TEST_ASSERT(!cursor);

	TypeTsysList types;
	TypeToTsys(pa, type, types);
	TEST_ASSERT(types.Count() == 1);
	return types[0];
}

template<typename> struct TsysInfo;

template<typename T>
struct TsysInfo<T&>
{
	static ITsys* GetTsys(ITsysAlloc* tsys)
	{
		return TsysInfo<T>::GetTsys(tsys)->LRefOf();
	}
};

template<typename T>
struct TsysInfo<T&&>
{
	static ITsys* GetTsys(ITsysAlloc* tsys)
	{
		return TsysInfo<T>::GetTsys(tsys)->RRefOf();
	}
};

#define DEFINE_TSYS(TYPE)\
template<> struct TsysInfo<TYPE>\
{\
	static ITsys* GetTsys(ITsysAlloc* tsys)\
	{\
		static auto cache = GetTsysFromCppType(tsys, L#TYPE);\
		return cache;\
	}\
}\

DEFINE_TSYS(bool);
DEFINE_TSYS(signed __int8);
DEFINE_TSYS(signed __int16);
DEFINE_TSYS(signed __int32);
DEFINE_TSYS(signed __int64);
DEFINE_TSYS(unsigned __int8);
DEFINE_TSYS(unsigned __int16);
DEFINE_TSYS(unsigned __int32);
DEFINE_TSYS(unsigned __int64);
DEFINE_TSYS(wchar_t);
DEFINE_TSYS(char16_t);
DEFINE_TSYS(char32_t);
DEFINE_TSYS(float);
DEFINE_TSYS(double);
DEFINE_TSYS(long double);

#undef DEFINE_TSYS

/***********************************************************************
Macros
***********************************************************************/

#define TEST_DECL(SOMETHING) SOMETHING auto input = L#SOMETHING

#define TEST_DECL_VARS			\
	TEST_DECL(					\
		bool b;					\
		signed __int8 si8;		\
		signed __int16 si16;	\
		signed __int32 si32;	\
		signed __int64 si64;	\
		unsigned __int8 ui8;	\
		unsigned __int16 ui16;	\
		unsigned __int32 ui32;	\
		unsigned __int64 ui64;	\
		signed char sc;			\
		unsigned char uc;		\
		wchar_t wc;				\
		char16_t c16;			\
		char32_t c32;			\
		float f;				\
		double d;				\
		long double ld;			\
	)

#define TEST_EACH_VAR(F) F(b) F(si8) F(si16) F(si32) F(si64) F(ui8) F(ui16) F(ui32) F(ui64) F(sc) F(uc) F(wc) F(c16) F(c32) F(f) F(d) F(ld)

/***********************************************************************
Test Cases
***********************************************************************/

#pragma warning (push)
#pragma warning (disable: 4101)

TEST_CASE(TestIntegralPromotion_PostfixUnary)
{
	TEST_DECL_VARS;
	COMPILE_PROGRAM(program, pa, input);

#define TEST_VAR(NAME)\
	{\
		auto input = L#NAME L"++";\
		auto log = L"(" L#NAME L" ++)";\
		auto tsys = TsysInfo<decltype(NAME ++)>::GetTsys(pa.tsys.Obj());\
		auto logTsys = GenerateToStream([&](StreamWriter& writer){ Log(tsys, writer); });\
		AssertExpr(input, log, logTsys);\
	}
	TEST_EACH_VAR(TEST_VAR)
#undef TEST_VAR
}

TEST_CASE(TestIntegralPromotion_PrefixUnary)
{
	TEST_DECL_VARS;
	COMPILE_PROGRAM(program, pa, input);
}

TEST_CASE(TestIntegralPromotion_BinaryBool)
{
	TEST_DECL_VARS;
	COMPILE_PROGRAM(program, pa, input);
}

TEST_CASE(TestIntegralPromotion_BinaryInt)
{
	TEST_DECL_VARS;
	COMPILE_PROGRAM(program, pa, input);
}

TEST_CASE(TestIntegralPromotion_BinaryFloat)
{
	TEST_DECL_VARS;
	COMPILE_PROGRAM(program, pa, input);
}

TEST_CASE(TestIntegralPromotion_Assignment)
{
	TEST_DECL_VARS;
	COMPILE_PROGRAM(program, pa, input);
}

#undef TEST_EACH_VAR
#undef TEST_DECL_VARS
#undef TEST_DECL

#pragma warning (push)