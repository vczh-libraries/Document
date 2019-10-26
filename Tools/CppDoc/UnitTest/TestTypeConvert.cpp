#include "Util.h"

void AssertTypeConvert(ParsingArguments& pa, const WString fromCppType, const WString& toCppType, TypeConvCat conv, bool fromTemp)
{
	TypeTsysList fromTypes, toTypes;
	Ptr<Type> fromType, toType;
	{
		TOKEN_READER(fromCppType.Buffer());
		auto cursor = reader.GetFirstToken();
		fromType = ParseType(pa, cursor);
		TEST_ASSERT(cursor == nullptr);
	}
	{
		TOKEN_READER(toCppType.Buffer());
		auto cursor = reader.GetFirstToken();
		toType = ParseType(pa, cursor);
		TEST_ASSERT(cursor == nullptr);
	}

	TypeToTsysNoVta(pa, fromType, fromTypes);
	TypeToTsysNoVta(pa, toType, toTypes);
	TEST_ASSERT(fromTypes.Count() == 1);
	TEST_ASSERT(toTypes.Count() == 1);

	ExprTsysType type = ExprTsysType::LValue;
	if (fromTemp)
	{
		if (fromTypes[0]->GetType() == TsysType::LRef)
		{
			type = ExprTsysType::LValue;
		}
		else if (fromTypes[0]->GetType() == TsysType::RRef)
		{
			type = ExprTsysType::XValue;
		}
		else
		{
			type = ExprTsysType::PRValue;
		}
	}
	auto output = TestTypeConversion(pa, toTypes[0], { nullptr,type,fromTypes[0] });
	TEST_ASSERT(output.cat == conv);
}

#pragma warning (push)
#pragma warning (disable: 4076)
#pragma warning (disable: 4244)

template<typename TTo, TypeConvCat Conv>
struct RunTypeTester
{
	static void Test(...);
	static int Test(TTo);
};

template<typename TTo>
struct RunTypeTester<TTo, TypeConvCat::Illegal>
{
	static int Test(...);
	static void Test(TTo);
};

template<typename TFrom, typename TTo, typename Tester>
struct RunTypeConvert
{
	static TFrom entity;
	static const decltype(Tester::Test(entity)) test = 0;
};

template<typename TFrom, typename TTo, typename Tester>
struct RunTypeConvertFromTemp
{
	static typename RemoveReference<TFrom>::Type entity;
	static const decltype(Tester::Test(static_cast<TFrom>(entity))) test = 0;
};

#define TEST_CONV_TYPE(FROM, TO, CONV1, CONV2)\
	RunTypeConvert<FROM, TO, RunTypeTester<TO, TypeConvCat::CONV1>>::test,\
	RunTypeConvertFromTemp<FROM, TO, RunTypeTester<TO, TypeConvCat::CONV2>>::test,\
	AssertTypeConvert(pa, L#FROM, L#TO, TypeConvCat::CONV1, false),\
	AssertTypeConvert(pa, L#FROM, L#TO, TypeConvCat::CONV2, true)\

TEST_CASE(TestTypeConvert_Exact)
{
	ParsingArguments pa(new Symbol(symbol_component::SymbolCategory::Normal), ITsysAlloc::Create(), nullptr);
#define S Exact
	TEST_CONV_TYPE(int,						int,									S,	S);
	TEST_CONV_TYPE(int,						const int,								S,	S);
	TEST_CONV_TYPE(int,						volatile int,							S,	S);
	TEST_CONV_TYPE(int,						const volatile int,						S,	S);

	TEST_CONV_TYPE(int&,					int,									S,	S);
	TEST_CONV_TYPE(int&,					const int,								S,	S);
	TEST_CONV_TYPE(int&,					volatile int,							S,	S);
	TEST_CONV_TYPE(int&,					const volatile int,						S,	S);

	TEST_CONV_TYPE(const int&&,				int,									S,	S);
	TEST_CONV_TYPE(const int&&,				const int,								S,	S);
	TEST_CONV_TYPE(const int&&,				volatile int,							S,	S);
	TEST_CONV_TYPE(const int&&,				const volatile int,						S,	S);

	TEST_CONV_TYPE(int[10],					int*,									S,	S);
	TEST_CONV_TYPE(int[10],					int*const,								S,	S);
	TEST_CONV_TYPE(int[10],					int*volatile,							S,	S);

	TEST_CONV_TYPE(const int[10],			const int* const,						S,	S);
	TEST_CONV_TYPE(volatile int[10],		volatile int* volatile,					S,	S);
#undef S
}

TEST_CASE(TestTypeConvert_TrivalConversion)
{
	ParsingArguments pa(new Symbol(symbol_component::SymbolCategory::Normal), ITsysAlloc::Create(), nullptr);
#define S Trivial
#define F Illegal
	TEST_CONV_TYPE(int*,					const int*,								S,	S);
	TEST_CONV_TYPE(int*,					volatile int*,							S,	S);
	TEST_CONV_TYPE(int*,					const volatile int*,					S,	S);
	TEST_CONV_TYPE(int*,					const volatile int* const volatile,		S,	S);

	TEST_CONV_TYPE(int&,					const int&,								S,	S);
	TEST_CONV_TYPE(int&,					volatile int&,							S,	S);
	TEST_CONV_TYPE(int&,					const volatile int&,					S,	S);

	TEST_CONV_TYPE(int&&,					const int&&,							F,	S);
	TEST_CONV_TYPE(int&&,					volatile int&&,							F,	S);
	TEST_CONV_TYPE(int&&,					const volatile int&&,					F,	S);

	TEST_CONV_TYPE(char[10],				const char*volatile,					S,	S);
	TEST_CONV_TYPE(char(&)[10],				const char(&)[10],						S,	S);
#undef S
#undef F
}

TEST_CASE(TestTypeConvert_IntegralPromotion)
{
	TEST_DECL(
enum E{};
enum class C{};
	);
	COMPILE_PROGRAM(program, pa, input);

#define T Trivial
#define S IntegralPromotion
#define F Illegal
	TEST_CONV_TYPE(short,					int,									S,	S);
	TEST_CONV_TYPE(short,					unsigned int,							S,	S);
	TEST_CONV_TYPE(unsigned short,			int,									S,	S);
	TEST_CONV_TYPE(char,					wchar_t,								S,	S);
	TEST_CONV_TYPE(char,					char16_t,								S,	S);
	TEST_CONV_TYPE(char16_t,				char32_t,								S,	S);
	TEST_CONV_TYPE(float,					double,									S,	S);

	TEST_CONV_TYPE(const short&&,			int,									S,	S);
	TEST_CONV_TYPE(const short&&,			unsigned int,							S,	S);
	TEST_CONV_TYPE(const unsigned short&&,	int,									S,	S);
	TEST_CONV_TYPE(const char,				wchar_t,								S,	S);
	TEST_CONV_TYPE(const char,				char16_t,								S,	S);
	TEST_CONV_TYPE(const char16_t,			char32_t,								S,	S);
	TEST_CONV_TYPE(const float,				double,									S,	S);

	TEST_CONV_TYPE(short,					const int&&,							F,	S);
	TEST_CONV_TYPE(short,					const unsigned int&&,					F,	S);
	TEST_CONV_TYPE(unsigned short,			const int&&,							F,	S);
	TEST_CONV_TYPE(char,					const wchar_t,							S,	S);
	TEST_CONV_TYPE(char,					const char16_t,							S,	S);
	TEST_CONV_TYPE(char16_t,				const char32_t,							S,	S);
	TEST_CONV_TYPE(float&&,					const double&&,							F,	S);

	TEST_CONV_TYPE(E,						int,									T,	T);
	TEST_CONV_TYPE(E,						unsigned int,							S,	S);
	TEST_CONV_TYPE(const E&&,				char,									S,	S);
	TEST_CONV_TYPE(const E&&,				unsigned char,							S,	S);
	TEST_CONV_TYPE(E,						short,									S,	S);
	TEST_CONV_TYPE(E,						unsigned short,							S,	S);
	TEST_CONV_TYPE(E,						const wchar_t,							S,	S);
	TEST_CONV_TYPE(E,						const char16_t&,						S,	S);
	TEST_CONV_TYPE(E,						const char32_t,							S,	S);
	TEST_CONV_TYPE(E,						const char32_t&&,						F,	S);
	TEST_CONV_TYPE(E&&,						const char32_t&&,						F,	S);

	TEST_CONV_TYPE(int,						E,										F,	F);
	TEST_CONV_TYPE(unsigned int,			E,										F,	F);
	TEST_CONV_TYPE(char,					E,										F,	F);
	TEST_CONV_TYPE(unsigned char,			E,										F,	F);
	TEST_CONV_TYPE(short,					E,										F,	F);
	TEST_CONV_TYPE(unsigned short,			E,										F,	F);
	TEST_CONV_TYPE(wchar_t,					E,										F,	F);
	TEST_CONV_TYPE(char16_t,				E,										F,	F);
	TEST_CONV_TYPE(char32_t,				E,										F,	F);

	TEST_CONV_TYPE(C,						int,									F,	F);
	TEST_CONV_TYPE(C,						unsigned int,							F,	F);
	TEST_CONV_TYPE(C,						char,									F,	F);
	TEST_CONV_TYPE(C,						unsigned char,							F,	F);
	TEST_CONV_TYPE(C,						short,									F,	F);
	TEST_CONV_TYPE(C,						unsigned short,							F,	F);
	TEST_CONV_TYPE(C,						wchar_t,								F,	F);
	TEST_CONV_TYPE(C,						char16_t,								F,	F);
	TEST_CONV_TYPE(C,						char32_t,								F,	F);

	TEST_CONV_TYPE(int,						C,										F,	F);
	TEST_CONV_TYPE(unsigned int,			C,										F,	F);
	TEST_CONV_TYPE(char,					C,										F,	F);
	TEST_CONV_TYPE(unsigned char,			C,										F,	F);
	TEST_CONV_TYPE(short,					C,										F,	F);
	TEST_CONV_TYPE(unsigned short,			C,										F,	F);
	TEST_CONV_TYPE(wchar_t,					C,										F,	F);
	TEST_CONV_TYPE(char16_t,				C,										F,	F);
	TEST_CONV_TYPE(char32_t,				C,										F,	F);
#undef T
#undef S
#undef F
}

TEST_CASE(TestTypeConvert_StandardConversion)
{
	ParsingArguments pa(new Symbol(symbol_component::SymbolCategory::Normal), ITsysAlloc::Create(), nullptr);
#define S Standard
#define F Illegal
	TEST_CONV_TYPE(signed int,				unsigned int,							S,	S);
	TEST_CONV_TYPE(unsigned int,			signed int,								S,	S);
	TEST_CONV_TYPE(signed char,				unsigned char,							S,	S);
	TEST_CONV_TYPE(unsigned char,			signed char,							S,	S);

	TEST_CONV_TYPE(int,						double,									S,	S);
	TEST_CONV_TYPE(double,					int,									S,	S);
	TEST_CONV_TYPE(char,					double,									S,	S);
	TEST_CONV_TYPE(double,					char,									S,	S);

	TEST_CONV_TYPE(int,						double&&,								F,	S);
	TEST_CONV_TYPE(double,					int&&,									F,	S);
	TEST_CONV_TYPE(char,					double&&,								F,	S);
	TEST_CONV_TYPE(double,					char&&,									F,	S);

	TEST_CONV_TYPE(int(*)(),				void*,									S,	S);
	TEST_CONV_TYPE(int*,					void*,									S,	S);
	TEST_CONV_TYPE(const int*,				const void*,							S,	S);
	TEST_CONV_TYPE(const int*,				const void*,							S,	S);
#undef S
#undef F
}

TEST_CASE(TestTypeConvert_Illegal)
{
	ParsingArguments pa(new Symbol(symbol_component::SymbolCategory::Normal), ITsysAlloc::Create(), nullptr);
#define F Illegal
	TEST_CONV_TYPE(const int&,				int&,									F,	F);
	TEST_CONV_TYPE(volatile int&,			int&,									F,	F);
	TEST_CONV_TYPE(const int&,				volatile int&,							F,	F);
	TEST_CONV_TYPE(const int*,				int*,									F,	F);
	TEST_CONV_TYPE(volatile int*,			int*,									F,	F);
	TEST_CONV_TYPE(const int*,				volatile int*,							F,	F);

	TEST_CONV_TYPE(void*,					int*,									F,	F);
	TEST_CONV_TYPE(const void*,				int*,									F,	F);
	TEST_CONV_TYPE(const int*,				void*,									F,	F);
	TEST_CONV_TYPE(int*,					int(&&)[],								F,	F);
	TEST_CONV_TYPE(short&,					int&,									F,	F);
	TEST_CONV_TYPE(short*,					int*,									F,	F);

	TEST_CONV_TYPE(const int&,				double&&,								F,	F);
	TEST_CONV_TYPE(const double&,			int&&,									F,	F);
	TEST_CONV_TYPE(const char&,				double&&,								F,	F);
	TEST_CONV_TYPE(const double&,			char&&,									F,	F);

	TEST_CONV_TYPE(int**,					void**,									F,	F);
	TEST_CONV_TYPE(int**,					const void**,							F,	F);
	TEST_CONV_TYPE(int**,					const int**,							F,	F);
	TEST_CONV_TYPE(const int**,				int**,									F,	F);
	TEST_CONV_TYPE(int*[10],				void**,									F,	F);
	TEST_CONV_TYPE(int*[10],				const void**,							F,	F);
	TEST_CONV_TYPE(int*[10],				const int**,							F,	F);
	TEST_CONV_TYPE(const int*[10],			int**,									F,	F);
	TEST_CONV_TYPE(int*[10],				void*[10],								F,	F);
	TEST_CONV_TYPE(int*[10],				const void*[10],						F,	F);
	TEST_CONV_TYPE(int*[10],				const int*[10],							F,	F);
	TEST_CONV_TYPE(const int*[10],			int*[10],								F,	F);
#undef F
}

TEST_CASE(TestTypeConvert_Inheritance)
{
	TEST_DECL(
class Base {};
class Derived : public Base {};
	);
	COMPILE_PROGRAM(program, pa, input);

#define S Standard
#define F Illegal

	TEST_CONV_TYPE(Derived*,			Base*,										S,	S);
	TEST_CONV_TYPE(Derived&,			Base&,										S,	S);
	TEST_CONV_TYPE(Derived&&,			Base&&,										F,	S);

	TEST_CONV_TYPE(const Derived*,		const Base*,								S,	S);
	TEST_CONV_TYPE(const Derived&,		const Base&,								S,	S);
	TEST_CONV_TYPE(const Derived&&,		const Base&&,								F,	S);

	TEST_CONV_TYPE(Derived*,			const Base*,								S,	S);
	TEST_CONV_TYPE(Derived&,			const Base&,								S,	S);
	TEST_CONV_TYPE(Derived&&,			const Base&&,								F,	S);

	TEST_CONV_TYPE(Base*,				Derived*,									F,	F);
	TEST_CONV_TYPE(Base&,				Derived&,									F,	F);
	TEST_CONV_TYPE(Base&&,				Derived&&,									F,	F);

	TEST_CONV_TYPE(const Base*,			const Derived*,								F,	F);
	TEST_CONV_TYPE(const Base&,			const Derived&,								F,	F);
	TEST_CONV_TYPE(const Base&,			const Derived&&,							F,	F);
	TEST_CONV_TYPE(const Base&&,		const Derived&,								F,	F);
	TEST_CONV_TYPE(const Base&&,		const Derived&&,							F,	F);

	TEST_CONV_TYPE(const Base*,			Derived*,									F,	F);
	TEST_CONV_TYPE(const Base&,			Derived&,									F,	F);
	TEST_CONV_TYPE(const Base&,			Derived&&,									F,	F);
	TEST_CONV_TYPE(const Base&&,		Derived&,									F,	F);
	TEST_CONV_TYPE(const Base&&,		Derived&&,									F,	F);

	TEST_CONV_TYPE(const Derived*,		Base*,										F,	F);
	TEST_CONV_TYPE(const Derived&,		Base&,										F,	F);
	TEST_CONV_TYPE(const Derived&,		Base&&,										F,	F);
	TEST_CONV_TYPE(const Derived&&,		Base&,										F,	F);
	TEST_CONV_TYPE(const Derived&&,		Base&&,										F,	F);

	TEST_CONV_TYPE(Base*,				const Derived*,								F,	F);
	TEST_CONV_TYPE(Base&,				const Derived&,								F,	F);
	TEST_CONV_TYPE(Base&,				const Derived&&,							F,	F);
	TEST_CONV_TYPE(Base&&,				const Derived&,								F,	F);
	TEST_CONV_TYPE(Base&&,				const Derived&&,							F,	F);
#undef S
#undef F
}

TEST_CASE(TestTypeConvert_CtorConversion)
{
	TEST_DECL(
struct Source
{
};

struct Target
{
	Target(Source&&);
	Target(const Source&);
};
	);
	COMPILE_PROGRAM(program, pa, input);

#define S UserDefined
#define F Illegal
	TEST_CONV_TYPE(const Source&,		const Target&,								S,	S);
	TEST_CONV_TYPE(const Source&,		const Target&&,								S,	S);
	TEST_CONV_TYPE(const Source&&,		const Target&,								S,	S);
	TEST_CONV_TYPE(const Source&&,		const Target&&,								S,	S);

	TEST_CONV_TYPE(const Source&,		volatile Target&&,							S,	S);
	TEST_CONV_TYPE(const Source&&,		volatile Target&&,							S,	S);
	TEST_CONV_TYPE(const Source&,		Target&,									F,	F);
	TEST_CONV_TYPE(const Source&&,		Target&,									F,	F);

	TEST_CONV_TYPE(Source&,				const Target&,								S,	S);
	TEST_CONV_TYPE(Source&,				const Target&&,								S,	S);
	TEST_CONV_TYPE(Source&&,			const Target&,								S,	S);
	TEST_CONV_TYPE(Source&&,			const Target&&,								S,	S);

	TEST_CONV_TYPE(Source&,				volatile Target&&,							S,	S);
	TEST_CONV_TYPE(Source&&,			volatile Target&&,							S,	S);
	TEST_CONV_TYPE(Source&,				Target&,									F,	F);
	TEST_CONV_TYPE(Source&&,			Target&,									F,	F);

	TEST_CONV_TYPE(const Source,		volatile Target,							S,	S);
	TEST_CONV_TYPE(const Source&,		volatile Target,							S,	S);
	TEST_CONV_TYPE(const Source&&,		volatile Target,							S,	S);
	TEST_CONV_TYPE(Source,				volatile Target,							S,	S);
	TEST_CONV_TYPE(Source&,				volatile Target,							S,	S);
	TEST_CONV_TYPE(Source&&,			volatile Target,							S,	S);

	TEST_CONV_TYPE(const Source,		const Target,								S,	S);
	TEST_CONV_TYPE(const Source&,		const Target,								S,	S);
	TEST_CONV_TYPE(const Source&&,		const Target,								S,	S);
	TEST_CONV_TYPE(Source,				const Target,								S,	S);
	TEST_CONV_TYPE(Source&,				const Target,								S,	S);
	TEST_CONV_TYPE(Source&&,			const Target,								S,	S);

	TEST_CONV_TYPE(const Source*,		const Target*,								F,	F);
	TEST_CONV_TYPE(const Source*,		Target*,									F,	F);
	TEST_CONV_TYPE(Source*,				const Target*,								F,	F);
	TEST_CONV_TYPE(Source*,				Target*,									F,	F);
#undef S
#undef F
}

TEST_CASE(TestTypeConvert_CtorConversion_FailExplicit)
{
	TEST_DECL(
struct Source
{
};

struct Target
{
	explicit Target(Source&&);
	explicit Target(const Source&);
};
	);
	COMPILE_PROGRAM(program, pa, input);

#define F Illegal
	TEST_CONV_TYPE(const Source*,		const Target*,								F,	F);
	TEST_CONV_TYPE(const Source&,		const Target&,								F,	F);
	TEST_CONV_TYPE(const Source&,		const Target&&,								F,	F);
	TEST_CONV_TYPE(const Source&&,		const Target&,								F,	F);
	TEST_CONV_TYPE(const Source&&,		const Target&&,								F,	F);

	TEST_CONV_TYPE(const Source*,		Target*,									F,	F);
	TEST_CONV_TYPE(const Source&,		Target&,									F,	F);
	TEST_CONV_TYPE(const Source&,		Target&&,									F,	F);
	TEST_CONV_TYPE(const Source&&,		Target&,									F,	F);
	TEST_CONV_TYPE(const Source&&,		Target&&,									F,	F);

	TEST_CONV_TYPE(Source*,				const Target*,								F,	F);
	TEST_CONV_TYPE(Source&,				const Target&,								F,	F);
	TEST_CONV_TYPE(Source&,				const Target&&,								F,	F);
	TEST_CONV_TYPE(Source&&,			const Target&,								F,	F);
	TEST_CONV_TYPE(Source&&,			const Target&&,								F,	F);

	TEST_CONV_TYPE(Source*,				Target*,									F,	F);
	TEST_CONV_TYPE(Source&,				Target&,									F,	F);
	TEST_CONV_TYPE(Source&,				Target&&,									F,	F);
	TEST_CONV_TYPE(Source&&,			Target&,									F,	F);
	TEST_CONV_TYPE(Source&&,			Target&&,									F,	F);

	TEST_CONV_TYPE(const Source,		Target,										F,	F);
	TEST_CONV_TYPE(const Source&,		Target,										F,	F);
	TEST_CONV_TYPE(const Source&&,		Target,										F,	F);
	TEST_CONV_TYPE(Source,				Target,										F,	F);
	TEST_CONV_TYPE(Source&,				Target,										F,	F);
	TEST_CONV_TYPE(Source&&,			Target,										F,	F);

	TEST_CONV_TYPE(const Source,		const Target,								F,	F);
	TEST_CONV_TYPE(const Source&,		const Target,								F,	F);
	TEST_CONV_TYPE(const Source&&,		const Target,								F,	F);
	TEST_CONV_TYPE(Source,				const Target,								F,	F);
	TEST_CONV_TYPE(Source&,				const Target,								F,	F);
	TEST_CONV_TYPE(Source&&,			const Target,								F,	F);
#undef F
}

TEST_CASE(TestTypeConvert_OperatorConversion)
{
	TEST_DECL(
struct TargetA
{
};

struct TargetB
{
};

struct Source
{
	operator TargetA()const;
	operator TargetB();
};
	);
	COMPILE_PROGRAM(program, pa, input);

#define S UserDefined
#define F Illegal
	TEST_CONV_TYPE(const Source&,		const TargetA&,								S,	S);
	TEST_CONV_TYPE(const Source&,		const TargetA&&,							S,	S);
	TEST_CONV_TYPE(const Source&&,		const TargetA&,								S,	S);
	TEST_CONV_TYPE(const Source&&,		const TargetA&&,							S,	S);

	TEST_CONV_TYPE(const Source&,		volatile TargetA&&,							S,	S);
	TEST_CONV_TYPE(const Source&&,		volatile TargetA&&,							S,	S);
	TEST_CONV_TYPE(const Source&,		TargetA&,									F,	F);
	TEST_CONV_TYPE(const Source&&,		TargetA&,									F,	F);

	TEST_CONV_TYPE(Source&,				const TargetA&,								S,	S);
	TEST_CONV_TYPE(Source&,				const TargetA&&,							S,	S);
	TEST_CONV_TYPE(Source&&,			const TargetA&,								S,	S);
	TEST_CONV_TYPE(Source&&,			const TargetA&&,							S,	S);

	TEST_CONV_TYPE(Source&,				volatile TargetA&&,							S,	S);
	TEST_CONV_TYPE(Source&&,			volatile TargetA&&,							S,	S);
	TEST_CONV_TYPE(Source&,				TargetA&,									F,	F);
	TEST_CONV_TYPE(Source&&,			TargetA&,									F,	F);

	TEST_CONV_TYPE(const Source,		volatile TargetA,							S,	S);
	TEST_CONV_TYPE(const Source&,		volatile TargetA,							S,	S);
	TEST_CONV_TYPE(const Source&&,		volatile TargetA,							S,	S);
	TEST_CONV_TYPE(Source,				volatile TargetA,							S,	S);
	TEST_CONV_TYPE(Source&,				volatile TargetA,							S,	S);
	TEST_CONV_TYPE(Source&&,			volatile TargetA,							S,	S);

	TEST_CONV_TYPE(const Source,		const TargetA,								S,	S);
	TEST_CONV_TYPE(const Source&,		const TargetA,								S,	S);
	TEST_CONV_TYPE(const Source&&,		const TargetA,								S,	S);
	TEST_CONV_TYPE(Source,				const TargetA,								S,	S);
	TEST_CONV_TYPE(Source&,				const TargetA,								S,	S);
	TEST_CONV_TYPE(Source&&,			const TargetA,								S,	S);

	TEST_CONV_TYPE(const Source*,		const TargetA*,								F,	F);
	TEST_CONV_TYPE(const Source*,		TargetA*,									F,	F);
	TEST_CONV_TYPE(Source*,				const TargetA*,								F,	F);
	TEST_CONV_TYPE(Source*,				TargetA*,									F,	F);

	TEST_CONV_TYPE(Source&,				const TargetB&,								S,	S);
	TEST_CONV_TYPE(Source&,				const TargetB&&,							S,	S);
	TEST_CONV_TYPE(Source&&,			const TargetB&,								S,	S);
	TEST_CONV_TYPE(Source&&,			const TargetB&&,							S,	S);

	TEST_CONV_TYPE(Source&,				volatile TargetB&&,							S,	S);
	TEST_CONV_TYPE(Source&&,			volatile TargetB&&,							S,	S);
	TEST_CONV_TYPE(Source&,				TargetB&,									F,	F);
	TEST_CONV_TYPE(Source&&,			TargetB&,									F,	F);

	TEST_CONV_TYPE(Source,				volatile TargetB,							S,	S);
	TEST_CONV_TYPE(Source&,				volatile TargetB,							S,	S);
	TEST_CONV_TYPE(Source&&,			volatile TargetB,							S,	S);

	TEST_CONV_TYPE(Source,				const TargetB,								S,	S);
	TEST_CONV_TYPE(Source&,				const TargetB,								S,	S);
	TEST_CONV_TYPE(Source&&,			const TargetB,								S,	S);

	TEST_CONV_TYPE(const Source*,		const TargetB*,								F,	F);
	TEST_CONV_TYPE(const Source&,		const TargetB&,								F,	F);
	TEST_CONV_TYPE(const Source&,		const TargetB&&,							F,	F);
	TEST_CONV_TYPE(const Source&&,		const TargetB&,								F,	F);
	TEST_CONV_TYPE(const Source&&,		const TargetB&&,							F,	F);

	TEST_CONV_TYPE(const Source*,		TargetB*,									F,	F);
	TEST_CONV_TYPE(const Source&,		TargetB&,									F,	F);
	TEST_CONV_TYPE(const Source&,		TargetB&&,									F,	F);
	TEST_CONV_TYPE(const Source&&,		TargetB&,									F,	F);
	TEST_CONV_TYPE(const Source&&,		TargetB&&,									F,	F);

	TEST_CONV_TYPE(Source*,				const TargetB*,								F,	F);
	TEST_CONV_TYPE(Source*,				TargetB*,									F,	F);

	TEST_CONV_TYPE(const Source,		TargetB,									F,	F);
	TEST_CONV_TYPE(const Source&,		TargetB,									F,	F);
	TEST_CONV_TYPE(const Source&&,		TargetB,									F,	F);

	TEST_CONV_TYPE(const Source,		const TargetB,								F,	F);
	TEST_CONV_TYPE(const Source&,		const TargetB,								F,	F);
	TEST_CONV_TYPE(const Source&&,		const TargetB,								F,	F);
#undef S
#undef F
}

TEST_CASE(TestTypeConvert_OperatorConversion_FailExplicit)
{
	TEST_DECL(
struct TargetA
{
};

struct TargetB
{
};

struct Source
{
	explicit operator TargetA()const;
	explicit operator TargetB();
};
	);
	COMPILE_PROGRAM(program, pa, input);

#define F Illegal
	TEST_CONV_TYPE(const Source*,		const TargetA*,								F,	F);
	TEST_CONV_TYPE(const Source&,		const TargetA&,								F,	F);
	TEST_CONV_TYPE(const Source&,		const TargetA&&,							F,	F);
	TEST_CONV_TYPE(const Source&&,		const TargetA&,								F,	F);
	TEST_CONV_TYPE(const Source&&,		const TargetA&&,							F,	F);

	TEST_CONV_TYPE(const Source*,		TargetA*,									F,	F);
	TEST_CONV_TYPE(const Source&,		TargetA&,									F,	F);
	TEST_CONV_TYPE(const Source&,		TargetA&&,									F,	F);
	TEST_CONV_TYPE(const Source&&,		TargetA&,									F,	F);
	TEST_CONV_TYPE(const Source&&,		TargetA&&,									F,	F);

	TEST_CONV_TYPE(Source*,				const TargetA*,								F,	F);
	TEST_CONV_TYPE(Source&,				const TargetA&,								F,	F);
	TEST_CONV_TYPE(Source&,				const TargetA&&,							F,	F);
	TEST_CONV_TYPE(Source&&,			const TargetA&,								F,	F);
	TEST_CONV_TYPE(Source&&,			const TargetA&&,							F,	F);

	TEST_CONV_TYPE(Source*,				TargetA*,									F,	F);
	TEST_CONV_TYPE(Source&,				TargetA&,									F,	F);
	TEST_CONV_TYPE(Source&,				TargetA&&,									F,	F);
	TEST_CONV_TYPE(Source&&,			TargetA&,									F,	F);
	TEST_CONV_TYPE(Source&&,			TargetA&&,									F,	F);

	TEST_CONV_TYPE(const Source*,		const TargetB*,								F,	F);
	TEST_CONV_TYPE(const Source&,		const TargetB&,								F,	F);
	TEST_CONV_TYPE(const Source&,		const TargetB&&,							F,	F);
	TEST_CONV_TYPE(const Source&&,		const TargetB&,								F,	F);
	TEST_CONV_TYPE(const Source&&,		const TargetB&&,							F,	F);

	TEST_CONV_TYPE(const Source*,		TargetB*,									F,	F);
	TEST_CONV_TYPE(const Source&,		TargetB&,									F,	F);
	TEST_CONV_TYPE(const Source&,		TargetB&&,									F,	F);
	TEST_CONV_TYPE(const Source&&,		TargetB&,									F,	F);
	TEST_CONV_TYPE(const Source&&,		TargetB&&,									F,	F);

	TEST_CONV_TYPE(Source*,				const TargetB*,								F,	F);
	TEST_CONV_TYPE(Source&,				const TargetB&,								F,	F);
	TEST_CONV_TYPE(Source&,				const TargetB&&,							F,	F);
	TEST_CONV_TYPE(Source&&,			const TargetB&,								F,	F);
	TEST_CONV_TYPE(Source&&,			const TargetB&&,							F,	F);

	TEST_CONV_TYPE(Source*,				TargetB*,									F,	F);
	TEST_CONV_TYPE(Source&,				TargetB&,									F,	F);
	TEST_CONV_TYPE(Source&,				TargetB&&,									F,	F);
	TEST_CONV_TYPE(Source&&,			TargetB&,									F,	F);
	TEST_CONV_TYPE(Source&&,			TargetB&&,									F,	F);
#undef F
}

#undef TEST_CONV_TYPE

#pragma warning (push)