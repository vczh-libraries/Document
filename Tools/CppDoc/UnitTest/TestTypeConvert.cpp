#include <Parser.h>
#include "Util.h"

void AssertTypeConvertFromTemp(ParsingArguments& pa, const WString fromCppType, const WString& toCppType, TsysConv conv)
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

	ExprTsysType type;
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
	auto output = TestConvert(pa, toTypes[0], { nullptr,type,fromTypes[0] });
	TEST_ASSERT(output == conv);
}

#pragma warning (push)
#pragma warning (disable: 4076)
#pragma warning (disable: 4244)

template<typename TFrom, typename TTo, TsysConv Conv>
struct RunTypeConvert
{
	using TFromEntity = typename RemoveReference<TFrom>::Type;

	static void Test(...);
	static int Test(TTo);
	static TFromEntity entity;
	static const decltype(Test(static_cast<TFrom>(entity))) test = 0;
};

template<typename TFrom, typename TTo>
struct RunTypeConvert<TFrom, TTo, TsysConv::Illegal>
{
	using TFromEntity = typename RemoveReference<TFrom>::Type;

	static int Test(...);
	static void Test(TTo);
	static TFromEntity entity;
	static const decltype(Test(static_cast<TFrom>(entity))) test = 0;
};

#define TEST_DECL(SOMETHING) SOMETHING auto input = L#SOMETHING
#define TEST_CONV_TYPE(FROM, TO, CONV) RunTypeConvert<FROM, TO, TsysConv::CONV>::test, AssertTypeConvertFromTemp(pa, L#FROM, L#TO, TsysConv::CONV)

TEST_CASE(TestTypeConvert_Exact)
{
	ParsingArguments pa(new Symbol, ITsysAlloc::Create(), nullptr);
#define S Exact
	TEST_CONV_TYPE(int,						int,							S);
	TEST_CONV_TYPE(int,						const int,						S);
	TEST_CONV_TYPE(int,						volatile int,					S);
	TEST_CONV_TYPE(int,						const volatile int,				S);

	TEST_CONV_TYPE(int&,					int,							S);
	TEST_CONV_TYPE(int&,					const int,						S);
	TEST_CONV_TYPE(int&,					volatile int,					S);
	TEST_CONV_TYPE(int&,					const volatile int,				S);

	TEST_CONV_TYPE(const int&&,				int,							S);
	TEST_CONV_TYPE(const int&&,				const int,						S);
	TEST_CONV_TYPE(const int&&,				volatile int,					S);
	TEST_CONV_TYPE(const int&&,				const volatile int,				S);

	TEST_CONV_TYPE(int[10],					int*,							S);
	TEST_CONV_TYPE(int[10],					int*const,						S);
	TEST_CONV_TYPE(int[10],					int*volatile,					S);

	TEST_CONV_TYPE(const int[10],			const int* const,				S);
	TEST_CONV_TYPE(volatile int[10],		volatile int* volatile,			S);
#undef S
}

TEST_CASE(TestTypeConvert_TrivalConversion)
{
	ParsingArguments pa(new Symbol, ITsysAlloc::Create(), nullptr);
#define S TrivalConversion
	TEST_CONV_TYPE(int*,					const int*,								S);
	TEST_CONV_TYPE(int*,					volatile int*,							S);
	TEST_CONV_TYPE(int*,					const volatile int*,					S);
	TEST_CONV_TYPE(int*,					const volatile int* const volatile,		S);

	TEST_CONV_TYPE(int&,					const int&,								S);
	TEST_CONV_TYPE(int&,					volatile int&,							S);
	TEST_CONV_TYPE(int&,					const volatile int&,					S);

	TEST_CONV_TYPE(int&&,					const int&&,							S);
	TEST_CONV_TYPE(int&&,					volatile int&&,							S);
	TEST_CONV_TYPE(int&&,					const volatile int&&,					S);

	TEST_CONV_TYPE(char[10],				const char*volatile,					S);
	TEST_CONV_TYPE(char(&)[10],				const char(&)[10],						S);
#undef S
}

TEST_CASE(TestTypeConvert_IntegralPromotion)
{
	ParsingArguments pa(new Symbol, ITsysAlloc::Create(), nullptr);
#define S IntegralPromotion
	TEST_CONV_TYPE(short,					int,						S);
	TEST_CONV_TYPE(short,					unsigned int,				S);
	TEST_CONV_TYPE(unsigned short,			int,						S);
	TEST_CONV_TYPE(char,					wchar_t,					S);
	TEST_CONV_TYPE(char,					char16_t,					S);
	TEST_CONV_TYPE(char16_t,				char32_t,					S);
	TEST_CONV_TYPE(float,					double,						S);

	TEST_CONV_TYPE(const short&&,			int,						S);
	TEST_CONV_TYPE(const short&&,			unsigned int,				S);
	TEST_CONV_TYPE(const unsigned short&&,	int,						S);
	TEST_CONV_TYPE(const char,				wchar_t,					S);
	TEST_CONV_TYPE(const char,				char16_t,					S);
	TEST_CONV_TYPE(const char16_t,			char32_t,					S);
	TEST_CONV_TYPE(const float,				double,						S);

	TEST_CONV_TYPE(short,					const int&&,				S);
	TEST_CONV_TYPE(short,					const unsigned int&&,		S);
	TEST_CONV_TYPE(unsigned short,			const int&&,				S);
	TEST_CONV_TYPE(char,					const wchar_t,				S);
	TEST_CONV_TYPE(char,					const char16_t,				S);
	TEST_CONV_TYPE(char16_t,				const char32_t,				S);
	TEST_CONV_TYPE(float&&,					const double&&,				S);
#undef S
}

TEST_CASE(TestTypeConvert_StandardConversion)
{
	ParsingArguments pa(new Symbol, ITsysAlloc::Create(), nullptr);
#define S StandardConversion
	TEST_CONV_TYPE(signed int,				unsigned int,				S);
	TEST_CONV_TYPE(unsigned int,			signed int,					S);
	TEST_CONV_TYPE(signed char,				unsigned char,				S);
	TEST_CONV_TYPE(unsigned char,			signed char,				S);

	TEST_CONV_TYPE(int,						double,						S);
	TEST_CONV_TYPE(double,					int,						S);
	TEST_CONV_TYPE(char,					double,						S);
	TEST_CONV_TYPE(double,					char,						S);

	TEST_CONV_TYPE(int,						double&&,					S);
	TEST_CONV_TYPE(double,					int&&,						S);
	TEST_CONV_TYPE(char,					double&&,					S);
	TEST_CONV_TYPE(double,					char&&,						S);

	TEST_CONV_TYPE(int(*)(),				void*,						S);
	TEST_CONV_TYPE(int*,					void*,						S);
	TEST_CONV_TYPE(const int*,				const void*,				S);
	TEST_CONV_TYPE(const int*,				const void*,				S);
#undef S
}

TEST_CASE(TestTypeConvert_Illegal)
{
	ParsingArguments pa(new Symbol, ITsysAlloc::Create(), nullptr);
#define F Illegal
	TEST_CONV_TYPE(const int&,				int&,						F);
	TEST_CONV_TYPE(volatile int&,			int&,						F);
	TEST_CONV_TYPE(const int&,				volatile int&,				F);
	TEST_CONV_TYPE(const int*,				int*,						F);
	TEST_CONV_TYPE(volatile int*,			int*,						F);
	TEST_CONV_TYPE(const int*,				volatile int*,				F);

	TEST_CONV_TYPE(void*,					int*,						F);
	TEST_CONV_TYPE(const void*,				int*,						F);
	TEST_CONV_TYPE(const int*,				void*,						F);
	TEST_CONV_TYPE(int*,					int(&&)[],					F);
	TEST_CONV_TYPE(short&,					int&,						F);
	TEST_CONV_TYPE(short*,					int*,						F);

	TEST_CONV_TYPE(const int&,				double&&,					F);
	TEST_CONV_TYPE(const double&,			int&&,						F);
	TEST_CONV_TYPE(const char&,				double&&,					F);
	TEST_CONV_TYPE(const double&,			char&&,						F);

	TEST_CONV_TYPE(int**,					void**,						F);
	TEST_CONV_TYPE(int**,					const void**,				F);
	TEST_CONV_TYPE(int**,					const int**,				F);
	TEST_CONV_TYPE(const int**,				int**,						F);
	TEST_CONV_TYPE(int*[10],				void**,						F);
	TEST_CONV_TYPE(int*[10],				const void**,				F);
	TEST_CONV_TYPE(int*[10],				const int**,				F);
	TEST_CONV_TYPE(const int*[10],			int**,						F);
	TEST_CONV_TYPE(int*[10],				void*[10],					F);
	TEST_CONV_TYPE(int*[10],				const void*[10],			F);
	TEST_CONV_TYPE(int*[10],				const int*[10],				F);
	TEST_CONV_TYPE(const int*[10],			int*[10],					F);
#undef F
}

TEST_CASE(TestTypeConvert_Inheritance)
{
	TEST_DECL(
class Base {};
class Derived : public Base {};
	);
	COMPILE_PROGRAM(program, pa, input);

#define S StandardConversion
#define F Illegal
	
	TEST_CONV_TYPE(Derived*,			Base*,						S);
	TEST_CONV_TYPE(Derived&,			Base&,						S);
	TEST_CONV_TYPE(Derived&&,			Base&&,						S);

	TEST_CONV_TYPE(const Derived*,		const Base*,				S);
	TEST_CONV_TYPE(const Derived&,		const Base&,				S);
	TEST_CONV_TYPE(const Derived&&,		const Base&&,				S);

	TEST_CONV_TYPE(Derived*,			const Base*,				S);
	TEST_CONV_TYPE(Derived&,			const Base&,				S);
	TEST_CONV_TYPE(Derived&&,			const Base&&,				S);

	TEST_CONV_TYPE(Base*,				Derived*,					F);
	TEST_CONV_TYPE(Base&,				Derived&,					F);
	TEST_CONV_TYPE(Base&&,				Derived&&,					F);

	TEST_CONV_TYPE(const Base*,			const Derived*,				F);
	TEST_CONV_TYPE(const Base&,			const Derived&,				F);
	TEST_CONV_TYPE(const Base&,			const Derived&&,			F);
	TEST_CONV_TYPE(const Base&&,		const Derived&,				F);
	TEST_CONV_TYPE(const Base&&,		const Derived&&,			F);

	TEST_CONV_TYPE(const Base*,			Derived*,					F);
	TEST_CONV_TYPE(const Base&,			Derived&,					F);
	TEST_CONV_TYPE(const Base&,			Derived&&,					F);
	TEST_CONV_TYPE(const Base&&,		Derived&,					F);
	TEST_CONV_TYPE(const Base&&,		Derived&&,					F);

	TEST_CONV_TYPE(const Derived*,		Base*,						F);
	TEST_CONV_TYPE(const Derived&,		Base&,						F);
	TEST_CONV_TYPE(const Derived&,		Base&&,						F);
	TEST_CONV_TYPE(const Derived&&,		Base&,						F);
	TEST_CONV_TYPE(const Derived&&,		Base&&,						F);

	TEST_CONV_TYPE(Base*,				const Derived*,				F);
	TEST_CONV_TYPE(Base&,				const Derived&,				F);
	TEST_CONV_TYPE(Base&,				const Derived&&,			F);
	TEST_CONV_TYPE(Base&&,				const Derived&,				F);
	TEST_CONV_TYPE(Base&&,				const Derived&&,			F);
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

#define S UserDefinedConversion
#define F Illegal
	TEST_CONV_TYPE(const Source&,		const Target&,				S);
	TEST_CONV_TYPE(const Source&,		const Target&&,				S);
	TEST_CONV_TYPE(const Source&&,		const Target&,				S);
	TEST_CONV_TYPE(const Source&&,		const Target&&,				S);

	TEST_CONV_TYPE(const Source&,		volatile Target&&,			S);
	TEST_CONV_TYPE(const Source&&,		volatile Target&&,			S);
	TEST_CONV_TYPE(const Source&,		Target&,					F);
	TEST_CONV_TYPE(const Source&&,		Target&,					F);

	TEST_CONV_TYPE(Source&,				const Target&,				S);
	TEST_CONV_TYPE(Source&,				const Target&&,				S);
	TEST_CONV_TYPE(Source&&,			const Target&,				S);
	TEST_CONV_TYPE(Source&&,			const Target&&,				S);

	TEST_CONV_TYPE(Source&,				volatile Target&&,			S);
	TEST_CONV_TYPE(Source&&,			volatile Target&&,			S);
	TEST_CONV_TYPE(Source&,				Target&,					F);
	TEST_CONV_TYPE(Source&&,			Target&,					F);

	TEST_CONV_TYPE(const Source,		volatile Target,			S);
	TEST_CONV_TYPE(const Source&,		volatile Target,			S);
	TEST_CONV_TYPE(const Source&&,		volatile Target,			S);
	TEST_CONV_TYPE(Source,				volatile Target,			S);
	TEST_CONV_TYPE(Source&,				volatile Target,			S);
	TEST_CONV_TYPE(Source&&,			volatile Target,			S);

	TEST_CONV_TYPE(const Source,		const Target,				S);
	TEST_CONV_TYPE(const Source&,		const Target,				S);
	TEST_CONV_TYPE(const Source&&,		const Target,				S);
	TEST_CONV_TYPE(Source,				const Target,				S);
	TEST_CONV_TYPE(Source&,				const Target,				S);
	TEST_CONV_TYPE(Source&&,			const Target,				S);

	TEST_CONV_TYPE(const Source*,		const Target*,				F);
	TEST_CONV_TYPE(const Source*,		Target*,					F);
	TEST_CONV_TYPE(Source*,				const Target*,				F);
	TEST_CONV_TYPE(Source*,				Target*,					F);
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
	TEST_CONV_TYPE(const Source*,		const Target*,				F);
	TEST_CONV_TYPE(const Source&,		const Target&,				F);
	TEST_CONV_TYPE(const Source&,		const Target&&,				F);
	TEST_CONV_TYPE(const Source&&,		const Target&,				F);
	TEST_CONV_TYPE(const Source&&,		const Target&&,				F);

	TEST_CONV_TYPE(const Source*,		Target*,					F);
	TEST_CONV_TYPE(const Source&,		Target&,					F);
	TEST_CONV_TYPE(const Source&,		Target&&,					F);
	TEST_CONV_TYPE(const Source&&,		Target&,					F);
	TEST_CONV_TYPE(const Source&&,		Target&&,					F);

	TEST_CONV_TYPE(Source*,				const Target*,				F);
	TEST_CONV_TYPE(Source&,				const Target&,				F);
	TEST_CONV_TYPE(Source&,				const Target&&,				F);
	TEST_CONV_TYPE(Source&&,			const Target&,				F);
	TEST_CONV_TYPE(Source&&,			const Target&&,				F);

	TEST_CONV_TYPE(Source*,				Target*,					F);
	TEST_CONV_TYPE(Source&,				Target&,					F);
	TEST_CONV_TYPE(Source&,				Target&&,					F);
	TEST_CONV_TYPE(Source&&,			Target&,					F);
	TEST_CONV_TYPE(Source&&,			Target&&,					F);

	TEST_CONV_TYPE(const Source,		Target,						F);
	TEST_CONV_TYPE(const Source&,		Target,						F);
	TEST_CONV_TYPE(const Source&&,		Target,						F);
	TEST_CONV_TYPE(Source,				Target,						F);
	TEST_CONV_TYPE(Source&,				Target,						F);
	TEST_CONV_TYPE(Source&&,			Target,						F);

	TEST_CONV_TYPE(const Source,		const Target,				F);
	TEST_CONV_TYPE(const Source&,		const Target,				F);
	TEST_CONV_TYPE(const Source&&,		const Target,				F);
	TEST_CONV_TYPE(Source,				const Target,				F);
	TEST_CONV_TYPE(Source&,				const Target,				F);
	TEST_CONV_TYPE(Source&&,			const Target,				F);
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

#define S UserDefinedConversion
#define F Illegal
	TEST_CONV_TYPE(const Source&,		const TargetA&,				S);
	TEST_CONV_TYPE(const Source&,		const TargetA&&,			S);
	TEST_CONV_TYPE(const Source&&,		const TargetA&,				S);
	TEST_CONV_TYPE(const Source&&,		const TargetA&&,			S);

	TEST_CONV_TYPE(const Source&,		volatile TargetA&&,			S);
	TEST_CONV_TYPE(const Source&&,		volatile TargetA&&,			S);
	TEST_CONV_TYPE(const Source&,		TargetA&,					F);
	TEST_CONV_TYPE(const Source&&,		TargetA&,					F);

	TEST_CONV_TYPE(Source&,				const TargetA&,				S);
	TEST_CONV_TYPE(Source&,				const TargetA&&,			S);
	TEST_CONV_TYPE(Source&&,			const TargetA&,				S);
	TEST_CONV_TYPE(Source&&,			const TargetA&&,			S);

	TEST_CONV_TYPE(Source&,				volatile TargetA&&,			S);
	TEST_CONV_TYPE(Source&&,			volatile TargetA&&,			S);
	TEST_CONV_TYPE(Source&,				TargetA&,					F);
	TEST_CONV_TYPE(Source&&,			TargetA&,					F);

	TEST_CONV_TYPE(const Source,		volatile TargetA,			S);
	TEST_CONV_TYPE(const Source&,		volatile TargetA,			S);
	TEST_CONV_TYPE(const Source&&,		volatile TargetA,			S);
	TEST_CONV_TYPE(Source,				volatile TargetA,			S);
	TEST_CONV_TYPE(Source&,				volatile TargetA,			S);
	TEST_CONV_TYPE(Source&&,			volatile TargetA,			S);

	TEST_CONV_TYPE(const Source,		const TargetA,				S);
	TEST_CONV_TYPE(const Source&,		const TargetA,				S);
	TEST_CONV_TYPE(const Source&&,		const TargetA,				S);
	TEST_CONV_TYPE(Source,				const TargetA,				S);
	TEST_CONV_TYPE(Source&,				const TargetA,				S);
	TEST_CONV_TYPE(Source&&,			const TargetA,				S);

	TEST_CONV_TYPE(const Source*,		const TargetA*,				F);
	TEST_CONV_TYPE(const Source*,		TargetA*,					F);
	TEST_CONV_TYPE(Source*,				const TargetA*,				F);
	TEST_CONV_TYPE(Source*,				TargetA*,					F);

	TEST_CONV_TYPE(Source&,				const TargetB&,				S);
	TEST_CONV_TYPE(Source&,				const TargetB&&,			S);
	TEST_CONV_TYPE(Source&&,			const TargetB&,				S);
	TEST_CONV_TYPE(Source&&,			const TargetB&&,			S);

	TEST_CONV_TYPE(Source&,				volatile TargetB&&,			S);
	TEST_CONV_TYPE(Source&&,			volatile TargetB&&,			S);
	TEST_CONV_TYPE(Source&,				TargetB&,					F);
	TEST_CONV_TYPE(Source&&,			TargetB&,					F);

	TEST_CONV_TYPE(Source,				volatile TargetB,			S);
	TEST_CONV_TYPE(Source&,				volatile TargetB,			S);
	TEST_CONV_TYPE(Source&&,			volatile TargetB,			S);

	TEST_CONV_TYPE(Source,				const TargetB,				S);
	TEST_CONV_TYPE(Source&,				const TargetB,				S);
	TEST_CONV_TYPE(Source&&,			const TargetB,				S);

	TEST_CONV_TYPE(const Source*,		const TargetB*,				F);
	TEST_CONV_TYPE(const Source&,		const TargetB&,				F);
	TEST_CONV_TYPE(const Source&,		const TargetB&&,			F);
	TEST_CONV_TYPE(const Source&&,		const TargetB&,				F);
	TEST_CONV_TYPE(const Source&&,		const TargetB&&,			F);

	TEST_CONV_TYPE(const Source*,		TargetB*,					F);
	TEST_CONV_TYPE(const Source&,		TargetB&,					F);
	TEST_CONV_TYPE(const Source&,		TargetB&&,					F);
	TEST_CONV_TYPE(const Source&&,		TargetB&,					F);
	TEST_CONV_TYPE(const Source&&,		TargetB&&,					F);

	TEST_CONV_TYPE(Source*,				const TargetB*,				F);
	TEST_CONV_TYPE(Source*,				TargetB*,					F);

	TEST_CONV_TYPE(const Source,		TargetB,					F);
	TEST_CONV_TYPE(const Source&,		TargetB,					F);
	TEST_CONV_TYPE(const Source&&,		TargetB,					F);

	TEST_CONV_TYPE(const Source,		const TargetB,				F);
	TEST_CONV_TYPE(const Source&,		const TargetB,				F);
	TEST_CONV_TYPE(const Source&&,		const TargetB,				F);
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
	TEST_CONV_TYPE(const Source*,		const TargetA*,				F);
	TEST_CONV_TYPE(const Source&,		const TargetA&,				F);
	TEST_CONV_TYPE(const Source&,		const TargetA&&,			F);
	TEST_CONV_TYPE(const Source&&,		const TargetA&,				F);
	TEST_CONV_TYPE(const Source&&,		const TargetA&&,			F);

	TEST_CONV_TYPE(const Source*,		TargetA*,					F);
	TEST_CONV_TYPE(const Source&,		TargetA&,					F);
	TEST_CONV_TYPE(const Source&,		TargetA&&,					F);
	TEST_CONV_TYPE(const Source&&,		TargetA&,					F);
	TEST_CONV_TYPE(const Source&&,		TargetA&&,					F);

	TEST_CONV_TYPE(Source*,				const TargetA*,				F);
	TEST_CONV_TYPE(Source&,				const TargetA&,				F);
	TEST_CONV_TYPE(Source&,				const TargetA&&,			F);
	TEST_CONV_TYPE(Source&&,			const TargetA&,				F);
	TEST_CONV_TYPE(Source&&,			const TargetA&&,			F);

	TEST_CONV_TYPE(Source*,				TargetA*,					F);
	TEST_CONV_TYPE(Source&,				TargetA&,					F);
	TEST_CONV_TYPE(Source&,				TargetA&&,					F);
	TEST_CONV_TYPE(Source&&,			TargetA&,					F);
	TEST_CONV_TYPE(Source&&,			TargetA&&,					F);

	TEST_CONV_TYPE(const Source*,		const TargetB*,				F);
	TEST_CONV_TYPE(const Source&,		const TargetB&,				F);
	TEST_CONV_TYPE(const Source&,		const TargetB&&,			F);
	TEST_CONV_TYPE(const Source&&,		const TargetB&,				F);
	TEST_CONV_TYPE(const Source&&,		const TargetB&&,			F);

	TEST_CONV_TYPE(const Source*,		TargetB*,					F);
	TEST_CONV_TYPE(const Source&,		TargetB&,					F);
	TEST_CONV_TYPE(const Source&,		TargetB&&,					F);
	TEST_CONV_TYPE(const Source&&,		TargetB&,					F);
	TEST_CONV_TYPE(const Source&&,		TargetB&&,					F);

	TEST_CONV_TYPE(Source*,				const TargetB*,				F);
	TEST_CONV_TYPE(Source&,				const TargetB&,				F);
	TEST_CONV_TYPE(Source&,				const TargetB&&,			F);
	TEST_CONV_TYPE(Source&&,			const TargetB&,				F);
	TEST_CONV_TYPE(Source&&,			const TargetB&&,			F);

	TEST_CONV_TYPE(Source*,				TargetB*,					F);
	TEST_CONV_TYPE(Source&,				TargetB&,					F);
	TEST_CONV_TYPE(Source&,				TargetB&&,					F);
	TEST_CONV_TYPE(Source&&,			TargetB&,					F);
	TEST_CONV_TYPE(Source&&,			TargetB&&,					F);
#undef F
}

#undef TEST_DECL
#undef TEST_CONV_TYPE

#pragma warning (push)