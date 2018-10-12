#include <Parser.h>
#include "Util.h"

void AssertTypeConvert(ParsingArguments& pa, const WString fromCppType, const WString& toCppType, TsysConv conv)
{
	TypeTsysList fromTypes, toTypes;
	Ptr<Type> fromType, toType;
	{
		CppTokenReader reader(GlobalCppLexer(), fromCppType);
		auto cursor = reader.GetFirstToken();
		fromType = ParseType(pa, cursor);
		TEST_ASSERT(cursor == nullptr);
	}
	{
		CppTokenReader reader(GlobalCppLexer(), toCppType);
		auto cursor = reader.GetFirstToken();
		toType = ParseType(pa, cursor);
		TEST_ASSERT(cursor == nullptr);
	}

	TypeToTsys(pa, fromType, fromTypes);
	TypeToTsys(pa, toType, toTypes);
	TEST_ASSERT(fromTypes.Count() == 1);
	TEST_ASSERT(toTypes.Count() == 1);

	auto output = TestConvert(toTypes[0], fromTypes[0]);
	TEST_ASSERT(output == conv);
}

void AssertTypeConvert(const WString fromCppType, const WString& toCppType, TsysConv conv)
{
	ParsingArguments pa(new Symbol, ITsysAlloc::Create(), nullptr);
	AssertTypeConvert(pa, fromCppType, toCppType, conv);
}

TEST_CASE(TestTypeConvert_Exact)
{
#define TEST_CONV(FROM, TO) AssertTypeConvert(L#FROM, L#TO, TsysConv::Exact)
	TEST_CONV(int, int);
	TEST_CONV(int, int&);
	TEST_CONV(int&, int);

	TEST_CONV(int, constexpr int);
	TEST_CONV(int, const int);
	TEST_CONV(int, volatile int);
	TEST_CONV(int, constexpr const volatile int);

	TEST_CONV(int[10], int*);
	TEST_CONV(int[10], int*constexpr);
	TEST_CONV(int[10], int*const);
	TEST_CONV(int[10], int*volatile);

	TEST_CONV(const int[10], const int* const);
	TEST_CONV(volatile int[10], volatile int* volatile);
#undef TEST_CONV
}

TEST_CASE(TestTypeConvert_TrivalConversion)
{
#define TEST_CONV(FROM, TO) AssertTypeConvert(L#FROM, L#TO, TsysConv::TrivalConversion)
	TEST_CONV(int*, constexpr int*);
	TEST_CONV(int*, const int*);
	TEST_CONV(int*, volatile int*);
	TEST_CONV(int*, constexpr const volatile int*);
	TEST_CONV(int*, constexpr const volatile int* constexpr const volatile);

	TEST_CONV(int&, constexpr int&);
	TEST_CONV(int&, const int&);
	TEST_CONV(int&, volatile int&);
	TEST_CONV(int&, constexpr const volatile int&);

	TEST_CONV(int&&, constexpr int&&);
	TEST_CONV(int&&, const int&&);
	TEST_CONV(int&&, volatile int&&);
	TEST_CONV(int&&, constexpr const volatile int&&);

	TEST_CONV(char[10], const char*volatile);
	TEST_CONV(char(&)[10], const char(&)[10]);
#undef TEST_CONV
}

TEST_CASE(TestTypeConvert_IntegralPromotion)
{
#define TEST_CONV(FROM, TO) AssertTypeConvert(L#FROM, L#TO, TsysConv::IntegralPromotion)
	TEST_CONV(short, int);
	TEST_CONV(short, unsigned int);
	TEST_CONV(unsigned short, int);
	TEST_CONV(char, wchar_t);
	TEST_CONV(char, char16_t);
	TEST_CONV(char16_t, unsigned char32_t);
	TEST_CONV(float, double);
#undef TEST_CONV
}

TEST_CASE(TestTypeConvert_StandardConversion)
{
#define TEST_CONV(FROM, TO) AssertTypeConvert(L#FROM, L#TO, TsysConv::StandardConversion)
	TEST_CONV(signed int, unsigned int);
	TEST_CONV(unsigned int, signed int);
	TEST_CONV(signed char, unsigned char);
	TEST_CONV(unsigned char, signed char);
	TEST_CONV(signed wchar_t, unsigned wchar_t);
	TEST_CONV(unsigned wchar_t, signed wchar_t);
	TEST_CONV(signed char16_t, unsigned char16_t);
	TEST_CONV(unsigned char16_t, signed char16_t);
	TEST_CONV(signed char32_t, unsigned char32_t);
	TEST_CONV(unsigned char32_t, signed char32_t);

	TEST_CONV(int, double);
	TEST_CONV(double, int);
	TEST_CONV(char, double);
	TEST_CONV(double, char);

	TEST_CONV(int*, void*);
	TEST_CONV(int(*)(), void*);
#undef TEST_CONV
}

TEST_CASE(TestTypeConvert_UserDefinedConversion)
{
#define TEST_CONV(FROM, TO) AssertTypeConvert(L#FROM, L#TO, TsysConv::UserDefinedConversion)
#undef TEST_CONV
}

TEST_CASE(TestTypeConvert_Illegal)
{
#define TEST_CONV(FROM, TO) AssertTypeConvert(L#FROM, L#TO, TsysConv::Illegal)
#undef TEST_CONV
}