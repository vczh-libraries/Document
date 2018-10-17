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
struct TsysInfo<T*>
{
	static ITsys* GetTsys(ITsysAlloc* tsys)
	{
		return TsysInfo<T>::GetTsys(tsys)->PtrOf();
	}
};

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

#define TEST_EACH_VAR_BOOL(F) F(b)
#define TEST_EACH_VAR_SINT(F) F(si8) F(si16) F(si32) F(si64)
#define TEST_EACH_VAR_UINT(F) F(ui8) F(ui16) F(ui32) F(ui64)
#define TEST_EACH_VAR_INT(F) TEST_EACH_VAR_SINT(F) TEST_EACH_VAR_UINT(F)
#define TEST_EACH_VAR_CHAR(F) F(sc) F(uc) F(wc) F(c16) F(c32)
#define TEST_EACH_VAR_FLOAT(F) F(f) F(d) F(ld)

#define TEST_EACH_VAR(F) TEST_EACH_VAR_BOOL(F) TEST_EACH_VAR_INT(F) TEST_EACH_VAR_CHAR(F) TEST_EACH_VAR_FLOAT(F)
#define TEST_EACH_VAR_NO_BOOL(F) TEST_EACH_VAR_INT(F) TEST_EACH_VAR_CHAR(F) TEST_EACH_VAR_FLOAT(F)
#define TEST_EACH_VAR_NO_BOOL_UNSIGNED(F) TEST_EACH_VAR_SINT(F) TEST_EACH_VAR_FLOAT(F)
#define TEST_EACH_VAR_NO_BOOL_FLOAT(F) TEST_EACH_VAR_INT(F) TEST_EACH_VAR_CHAR(F)
#define TEST_EACH_VAR_NO_FLOAT(F) TEST_EACH_VAR_BOOL(F) TEST_EACH_VAR_INT(F) TEST_EACH_VAR_CHAR(F)

/***********************************************************************
Test Cases
***********************************************************************/

#pragma warning (push)
#pragma warning (disable: 4101)

template<typename T>
void AssertPostfixUnary(ParsingArguments& pa, const WString& name, const WString& op)
{
	auto input = name + op;
	auto log = L"(" + name + L" " + op + L")";
	auto tsys = TsysInfo<T>::GetTsys(pa.tsys.Obj());
	auto logTsys = GenerateToStream([&](StreamWriter& writer) { Log(tsys, writer); });
	AssertExpr(input, log, logTsys);
}

TEST_CASE(TestIntegralPromotion_PostfixUnary)
{
	TEST_DECL_VARS;
	COMPILE_PROGRAM(program, pa, input);

#define TEST_VAR(NAME) AssertPostfixUnary<decltype((NAME++))>(pa, L#NAME, L"++");
	TEST_EACH_VAR(TEST_VAR)
#undef TEST_VAR

#define TEST_VAR(NAME) AssertPostfixUnary<decltype((NAME--))>(pa, L#NAME, L"--");
	TEST_EACH_VAR_NO_BOOL(TEST_VAR)
#undef TEST_VAR
}

template<typename T>
void AssertPrefixUnary(ParsingArguments& pa, const WString& name, const WString& op)
{
	auto input = op + name;
	auto log = L"(" + op + L" " + name + L")";
	auto tsys = TsysInfo<T>::GetTsys(pa.tsys.Obj());
	auto logTsys = GenerateToStream([&](StreamWriter& writer) { Log(tsys, writer); });
	AssertExpr(input, log, logTsys);
}

TEST_CASE(TestIntegralPromotion_PrefixUnary)
{
	TEST_DECL_VARS;
	COMPILE_PROGRAM(program, pa, input);

#define TEST_VAR(NAME) AssertPostfixUnary<decltype((++NAME))>(pa, L#NAME, L"++");
	TEST_EACH_VAR(TEST_VAR)
#undef TEST_VAR

#define TEST_VAR(NAME) AssertPostfixUnary<decltype((--NAME))>(pa, L#NAME, L"--");
	TEST_EACH_VAR_NO_BOOL(TEST_VAR)
#undef TEST_VAR

#define TEST_VAR(NAME) AssertPostfixUnary<decltype((~NAME))>(pa, L#NAME, L"~");
	TEST_EACH_VAR_NO_BOOL_FLOAT(TEST_VAR)
#undef TEST_VAR

#define TEST_VAR(NAME) AssertPostfixUnary<decltype((!NAME))>(pa, L#NAME, L"!");
	TEST_EACH_VAR(TEST_VAR)
#undef TEST_VAR

#define TEST_VAR(NAME) AssertPostfixUnary<decltype((-NAME))>(pa, L#NAME, L"-");
	TEST_EACH_VAR_NO_BOOL_UNSIGNED(TEST_VAR)
#undef TEST_VAR

#define TEST_VAR(NAME) AssertPostfixUnary<decltype((+NAME))>(pa, L#NAME, L"+");
	TEST_EACH_VAR(TEST_VAR)
#undef TEST_VAR

#define TEST_VAR(NAME) AssertPostfixUnary<decltype((&NAME))>(pa, L#NAME, L"&");
	TEST_EACH_VAR(TEST_VAR)
#undef TEST_VAR
}

template<typename T>
void AssertBinaryUnary(ParsingArguments& pa, const WString& name1, const WString& name2, const WString& op)
{
	auto input = name1 + op + name2;
	auto log = L"(" + name1 + L" " + op + L" " + name2 + L")";
	auto tsys = TsysInfo<T>::GetTsys(pa.tsys.Obj());
	auto logTsys = GenerateToStream([&](StreamWriter& writer) { Log(tsys, writer); });
	AssertExpr(input, log, logTsys);
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

#define TEST_VAR(NAME) AssertBinaryUnary<decltype((NAME=NAME))>(pa, L#NAME, L#NAME, L"=");
	TEST_EACH_VAR(TEST_VAR)
#undef TEST_VAR

#define TEST_VAR(NAME) AssertBinaryUnary<decltype((NAME*=NAME))>(pa, L#NAME, L#NAME, L"*=");
	TEST_EACH_VAR_NO_BOOL(TEST_VAR)
#undef TEST_VAR

#define TEST_VAR(NAME) AssertBinaryUnary<decltype((NAME/=NAME))>(pa, L#NAME, L#NAME, L"/=");
	TEST_EACH_VAR_NO_BOOL(TEST_VAR)
#undef TEST_VAR

#define TEST_VAR(NAME) AssertBinaryUnary<decltype((NAME%=NAME))>(pa, L#NAME, L#NAME, L"%=");
	TEST_EACH_VAR_NO_BOOL_FLOAT(TEST_VAR)
#undef TEST_VAR

#define TEST_VAR(NAME) AssertBinaryUnary<decltype((NAME+=NAME))>(pa, L#NAME, L#NAME, L"+=");
	TEST_EACH_VAR_NO_BOOL(TEST_VAR)
#undef TEST_VAR

#define TEST_VAR(NAME) AssertBinaryUnary<decltype((NAME-=NAME))>(pa, L#NAME, L#NAME, L"-=");
	TEST_EACH_VAR_NO_BOOL(TEST_VAR)
#undef TEST_VAR

#define TEST_VAR(NAME) AssertBinaryUnary<decltype((NAME<<=NAME))>(pa, L#NAME, L#NAME, L"<<=");
	TEST_EACH_VAR_NO_BOOL_FLOAT(TEST_VAR)
#undef TEST_VAR

#define TEST_VAR(NAME) AssertBinaryUnary<decltype((NAME>>=NAME))>(pa, L#NAME, L#NAME, L">>=");
	TEST_EACH_VAR_NO_BOOL_FLOAT(TEST_VAR)
#undef TEST_VAR

#define TEST_VAR(NAME) AssertBinaryUnary<decltype((NAME&=NAME))>(pa, L#NAME, L#NAME, L"&=");
	TEST_EACH_VAR_NO_FLOAT(TEST_VAR)
#undef TEST_VAR

#define TEST_VAR(NAME) AssertBinaryUnary<decltype((NAME|=NAME))>(pa, L#NAME, L#NAME, L"|=");
	TEST_EACH_VAR_NO_FLOAT(TEST_VAR)
#undef TEST_VAR

#define TEST_VAR(NAME) AssertBinaryUnary<decltype((NAME^=NAME))>(pa, L#NAME, L#NAME, L"^=");
	TEST_EACH_VAR_NO_FLOAT(TEST_VAR)
#undef TEST_VAR
}

#undef TEST_EACH_VAR_BOOL
#undef TEST_EACH_VAR_SINT
#undef TEST_EACH_VAR_UINT
#undef TEST_EACH_VAR_INT
#undef TEST_EACH_VAR_CHAR
#undef TEST_EACH_VAR_FLOAT
#undef TEST_EACH_VAR_NO_BOOL
#undef TEST_EACH_VAR_NO_BOOL_UNSIGNED
#undef TEST_EACH_VAR_NO_BOOL_FLOAT
#undef TEST_EACH_VAR_NO_FLOAT
#undef TEST_EACH_VAR
#undef TEST_DECL_VARS
#undef TEST_DECL

#pragma warning (push)