#include <Parser.h>
#include <Ast_Type.h>

extern Ptr<RegexLexer>		GlobalCppLexer();
extern void					Log(Ptr<Type> type, StreamWriter& writer);

void AssertType(const WString& type, const WString& log)
{
	CppTokenReader reader(GlobalCppLexer(), type);
	auto cursor = reader.GetFirstToken();

	ParsingArguments pa;
	List<Ptr<Declarator>> declarators;
	ParseDeclarator(pa, DeclaratorRestriction::Zero, InitializerRestriction::Zero, cursor, declarators);
	TEST_ASSERT(!cursor);
	TEST_ASSERT(declarators.Count() == 1);
	TEST_ASSERT(!declarators[0]->name);
	TEST_ASSERT(declarators[0]->initializer == nullptr);

	auto output = GenerateToStream([&](StreamWriter& writer)
	{
		Log(declarators[0]->type, writer);
	});
	TEST_ASSERT(output == log);
}

TEST_CASE(TestParseType_Primitive)
{
#define TEST_PRIMITIVE_TYPE(TYPE)\
	AssertType(L#TYPE, L#TYPE);\
	AssertType(L"signed " L#TYPE, L"signed " L#TYPE);\
	AssertType(L"unsigned " L#TYPE, L"unsigned " L#TYPE);\

	TEST_PRIMITIVE_TYPE(auto);
	TEST_PRIMITIVE_TYPE(void);
	TEST_PRIMITIVE_TYPE(bool);
	TEST_PRIMITIVE_TYPE(char);
	TEST_PRIMITIVE_TYPE(wchar_t);
	TEST_PRIMITIVE_TYPE(char16_t);
	TEST_PRIMITIVE_TYPE(char32_t);
	TEST_PRIMITIVE_TYPE(short);
	TEST_PRIMITIVE_TYPE(int);
	TEST_PRIMITIVE_TYPE(__int8);
	TEST_PRIMITIVE_TYPE(__int16);
	TEST_PRIMITIVE_TYPE(__int32);
	TEST_PRIMITIVE_TYPE(__int64);
	TEST_PRIMITIVE_TYPE(long);
	TEST_PRIMITIVE_TYPE(long long);
	TEST_PRIMITIVE_TYPE(float);
	TEST_PRIMITIVE_TYPE(double);
	TEST_PRIMITIVE_TYPE(long double);

#undef TEST_PRIMITIVE_TYPE
}

TEST_CASE(TestParseType_Short)
{
	AssertType(L"decltype(0)", L"decltype(0)");
	AssertType(L"constexpr int", L"int constexpr");
	AssertType(L"const int", L"int const");
	AssertType(L"volatile int", L"int volatile");
}

TEST_CASE(TestParseType_Long)
{
	AssertType(L"int constexpr", L"int constexpr");
	AssertType(L"int const", L"int const");
	AssertType(L"int volatile", L"int volatile");
	AssertType(L"int ...", L"int...");
	AssertType(L"int<long, short<float, double>>", L"int<long, short<float, double>>");
}

TEST_CASE(TestParseType_ShortDeclarator)
{
	AssertType(L"int* __ptr32", L"int *");
	AssertType(L"int* __ptr64", L"int *");
	AssertType(L"int*", L"int *");
	AssertType(L"int &", L"int &");
	AssertType(L"int &&", L"int &&");
	AssertType(L"int & &&", L"int & &&");
}

TEST_CASE(TestParseType_LongDeclarator)
{
	AssertType(L"int[]", L"int []");
	AssertType(L"int[][]", L"int [] []");
	AssertType(L"int[1][2][3]", L"int [1] [2] [3]");
	AssertType(L"int(*&)[][]", L"int [] [] * &");

	AssertType(L"int()", L"int ()");
	AssertType(L"auto ()->int constexpr const volatile & && override noexcept throw()", L"(auto->int constexpr const volatile & &&) () override noexcept throw()");
	AssertType(L"auto ()constexpr const volatile & && ->int override noexcept throw()", L"(auto->int) () constexpr const volatile & && override noexcept throw()");

	AssertType(L"int __cdecl(int)", L"int (int) __cdecl");
	AssertType(L"int __clrcall(int)", L"int (int) __clrcall");
	AssertType(L"int __stdcall(int)", L"int (int) __stdcall");
	AssertType(L"int __fastcall(int)", L"int (int) __fastcall");
	AssertType(L"int __thiscall(int)", L"int (int) __thiscall");
	AssertType(L"int __vectorcall(int)", L"int (int) __vectorcall");
	AssertType(L"int(*)(int)", L"int (int) *");
	AssertType(L"int(__cdecl*)(int)", L"int (int) __cdecl *");

	AssertType(L"int(*[5])(int, int a, int b=0)", L"int (int, int a, int b = 0) * [5]");
	AssertType(L"int(&(*[5])(void))[10]", L"int [10] & () * [5]");
}