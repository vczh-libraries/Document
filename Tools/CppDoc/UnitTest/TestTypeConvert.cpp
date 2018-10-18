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
#define TEST_CONV_TYPE_(FROM, TO, CONV) AssertTypeConvertFromTemp(pa, L#FROM, L#TO, TsysConv::CONV)
#define TEST_CONV_TYPE(FROM, TO, CONV) RunTypeConvert<FROM, TO, TsysConv::CONV>::test, TEST_CONV_TYPE_(FROM, TO, CONV)

TEST_CASE(TestTypeConvert_Exact)
{
	ParsingArguments pa(new Symbol, ITsysAlloc::Create(), nullptr);
#define TEST_CONV(FROM, TO) TEST_CONV_TYPE(FROM, TO, Exact)
	TEST_CONV(int,					int,							);
	TEST_CONV(int,					const int,						);
	TEST_CONV(int,					volatile int,					);
	TEST_CONV(int,					const volatile int,				);

	TEST_CONV(int&,					int,							);
	TEST_CONV(int&,					const int,						);
	TEST_CONV(int&,					volatile int,					);
	TEST_CONV(int&,					const volatile int,				);

	TEST_CONV(const int&&,			int,							);
	TEST_CONV(const int&&,			const int,						);
	TEST_CONV(const int&&,			volatile int,					);
	TEST_CONV(const int&&,			const volatile int,				);

	TEST_CONV(int[10],				int*,							);
	TEST_CONV(int[10],				int*const,						);
	TEST_CONV(int[10],				int*volatile,					);

	TEST_CONV(const int[10],		const int* const,				);
	TEST_CONV(volatile int[10],		volatile int* volatile,			);
#undef TEST_CONV
}

TEST_CASE(TestTypeConvert_TrivalConversion)
{
	ParsingArguments pa(new Symbol, ITsysAlloc::Create(), nullptr);
#define TEST_CONV(FROM, TO) TEST_CONV_TYPE(FROM, TO, TrivalConversion)
	TEST_CONV(int*,					const int*,								);
	TEST_CONV(int*,					volatile int*,							);
	TEST_CONV(int*,					const volatile int*,					);
	TEST_CONV(int*,					const volatile int* const volatile,		);

	TEST_CONV(int&,					const int&,								);
	TEST_CONV(int&,					volatile int&,							);
	TEST_CONV(int&,					const volatile int&,					);

	TEST_CONV(int&&,				const int&&,							);
	TEST_CONV(int&&,				volatile int&&,							);
	TEST_CONV(int&&,				const volatile int&&,					);

	TEST_CONV(char[10],				const char*volatile,					);
	TEST_CONV(char(&)[10],			const char(&)[10],						);
#undef TEST_CONV
}

TEST_CASE(TestTypeConvert_IntegralPromotion)
{
	ParsingArguments pa(new Symbol, ITsysAlloc::Create(), nullptr);
#define TEST_CONV(FROM, TO) TEST_CONV_TYPE(FROM, TO, IntegralPromotion)
	TEST_CONV(short,					int,						);
	TEST_CONV(short,					unsigned int,				);
	TEST_CONV(unsigned short,			int,						);
	TEST_CONV(char,						wchar_t,					);
	TEST_CONV(char,						char16_t,					);
	TEST_CONV(char16_t,					char32_t,					);
	TEST_CONV(float,					double,						);

	TEST_CONV(const short&&,			int,						);
	TEST_CONV(const short&&,			unsigned int,				);
	TEST_CONV(const unsigned short&&,	int,						);
	TEST_CONV(const char,				wchar_t,					);
	TEST_CONV(const char,				char16_t,					);
	TEST_CONV(const char16_t,			char32_t,					);
	TEST_CONV(const float,				double,						);

	TEST_CONV(short,					const int&&,				);
	TEST_CONV(short,					const unsigned int&&,		);
	TEST_CONV(unsigned short,			const int&&,				);
	TEST_CONV(char,						const wchar_t,				);
	TEST_CONV(char,						const char16_t,				);
	TEST_CONV(char16_t,					const char32_t,				);
	TEST_CONV(float&&,					const double&&,				);
#undef TEST_CONV
}

TEST_CASE(TestTypeConvert_StandardConversion)
{
	ParsingArguments pa(new Symbol, ITsysAlloc::Create(), nullptr);
#define TEST_CONV(FROM, TO) TEST_CONV_TYPE(FROM, TO, StandardConversion)
	TEST_CONV(signed int,				unsigned int,				);
	TEST_CONV(unsigned int,				signed int,					);
	TEST_CONV(signed char,				unsigned char,				);
	TEST_CONV(unsigned char,			signed char,				);

	TEST_CONV(int,						double,						);
	TEST_CONV(double,					int,						);
	TEST_CONV(char,						double,						);
	TEST_CONV(double,					char,						);

	TEST_CONV(int,						double&&,					);
	TEST_CONV(double,					int&&,						);
	TEST_CONV(char,						double&&,					);
	TEST_CONV(double,					char&&,						);

	TEST_CONV(int(*)(),					void*,						);
	TEST_CONV(int*,						void*,						);
	TEST_CONV(const int*,				const void*,				);
	TEST_CONV(const int*,				const void*,				);
#undef TEST_CONV
}

TEST_CASE(TestTypeConvert_Illegal)
{
	ParsingArguments pa(new Symbol, ITsysAlloc::Create(), nullptr);
#define TEST_CONV_(FROM, TO) TEST_CONV_TYPE_(FROM, TO, Illegal)
#define TEST_CONV(FROM, TO) TEST_CONV_TYPE(FROM, TO, Illegal)
	TEST_CONV(const int&,				int&,						);
	TEST_CONV(volatile int&,			int&,						);
	TEST_CONV(const int&,				volatile int&,				);
	TEST_CONV(const int*,				int*,						);
	TEST_CONV(volatile int*,			int*,						);
	TEST_CONV(const int*,				volatile int*,				);

	TEST_CONV(void*,					int*,						);
	TEST_CONV(const void*,				int*,						);
	TEST_CONV(const int*,				void*,						);
	TEST_CONV_(int*,					int[],						);
	TEST_CONV(short&,					int&,						);
	TEST_CONV(short*,					int*,						);

	TEST_CONV(const int&,				double&&,					);
	TEST_CONV(const double&,			int&&,						);
	TEST_CONV(const char&,				double&&,					);
	TEST_CONV(const double&,			char&&,						);

	TEST_CONV(int**,					void**,						);
	TEST_CONV(int**,					const void**,				);
	TEST_CONV(int**,					const int**,				);
	TEST_CONV(const int**,				int**,						);
	TEST_CONV(int*[10],					void**,						);
	TEST_CONV(int*[10],					const void**,				);
	TEST_CONV(int*[10],					const int**,				);
	TEST_CONV(const int*[10],			int**,						);
	TEST_CONV(int*[10],					void*[10],					);
	TEST_CONV(int*[10],					const void*[10],			);
	TEST_CONV(int*[10],					const int*[10],				);
	TEST_CONV(const int*[10],			int*[10],					);
#undef TEST_CONV
#undef TEST_CONV_
}

TEST_CASE(TestTypeConvert_Inheritance)
{
	TEST_DECL(
		class Base {};
		class Derived : public Base {};
,	);
	COMPILE_PROGRAM(program, pa, input);

	{
#define TEST_CONV(FROM, TO) TEST_CONV_TYPE(FROM, TO, StandardConversion)
		TEST_CONV(Derived*,				Base*,						);
		TEST_CONV(Derived&,				Base&,						);
		TEST_CONV(Derived&&,			Base&&,						);

		TEST_CONV(const Derived*,		const Base*,				);
		TEST_CONV(const Derived&,		const Base&,				);
		TEST_CONV(const Derived&&,		const Base&&,				);

		TEST_CONV(Derived*,				const Base*,				);
		TEST_CONV(Derived&,				const Base&,				);
		TEST_CONV(Derived&&,			const Base&&,				);
#undef TEST_CONV
	}

	{
#define TEST_CONV(FROM, TO) TEST_CONV_TYPE(FROM, TO, Illegal)
		TEST_CONV(Base*,				Derived*,					);
		TEST_CONV(Base&,				Derived&,					);
		TEST_CONV(Base&&,				Derived&&,					);

		TEST_CONV(const Base*,			const Derived*,				);
		TEST_CONV(const Base&,			const Derived&,				);
		TEST_CONV(const Base&,			const Derived&&,			);
		TEST_CONV(const Base&&,			const Derived&,				);
		TEST_CONV(const Base&&,			const Derived&&,			);

		TEST_CONV(const Base*,			Derived*,					);
		TEST_CONV(const Base&,			Derived&,					);
		TEST_CONV(const Base&,			Derived&&,					);
		TEST_CONV(const Base&&,			Derived&,					);
		TEST_CONV(const Base&&,			Derived&&,					);

		TEST_CONV(const Derived*,		Base*,						);
		TEST_CONV(const Derived&,		Base&,						);
		TEST_CONV(const Derived&,		Base&&,						);
		TEST_CONV(const Derived&&,		Base&,						);
		TEST_CONV(const Derived&&,		Base&&,						);

		TEST_CONV(Base*,				const Derived*,				);
		TEST_CONV(Base&,				const Derived&,				);
		TEST_CONV(Base&,				const Derived&&,			);
		TEST_CONV(Base&&,				const Derived&,				);
		TEST_CONV(Base&&,				const Derived&&,			);
#undef TEST_CONV
	}
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
,	);
	COMPILE_PROGRAM(program, pa, input);

	{
#define TEST_CONV(FROM, TO) TEST_CONV_TYPE(FROM, TO, UserDefinedConversion)
		TEST_CONV(const Source&,		const Target&,				);
		TEST_CONV(const Source&,		const Target&&,				);
		TEST_CONV(const Source&&,		const Target&,				);
		TEST_CONV(const Source&&,		const Target&&,				);

		TEST_CONV(const Source&,		volatile Target&&,			);
		TEST_CONV(const Source&&,		volatile Target&&,			);

		TEST_CONV(Source&,				const Target&,				);
		TEST_CONV(Source&,				const Target&&,				);
		TEST_CONV(Source&&,				const Target&,				);
		TEST_CONV(Source&&,				const Target&&,				);

		TEST_CONV(Source&,				volatile Target&&,			);
		TEST_CONV(Source&&,				volatile Target&&,			);
#undef TEST_CONV
	}

	{
#define TEST_CONV(FROM, TO) TEST_CONV_TYPE(FROM, TO, UserDefinedConversion)
		TEST_CONV(const Source,			volatile Target,			);
		TEST_CONV(const Source&,		volatile Target,			);
		TEST_CONV(const Source&&,		volatile Target,			);
		TEST_CONV(Source,				volatile Target,			);
		TEST_CONV(Source&,				volatile Target,			);
		TEST_CONV(Source&&,				volatile Target,			);

		TEST_CONV(const Source,			const Target,				);
		TEST_CONV(const Source&,		const Target,				);
		TEST_CONV(const Source&&,		const Target,				);
		TEST_CONV(Source,				const Target,				);
		TEST_CONV(Source&,				const Target,				);
		TEST_CONV(Source&&,				const Target,				);
#undef TEST_CONV
	}

	{
#define TEST_CONV(FROM, TO) TEST_CONV_TYPE(FROM, TO, Illegal)
		TEST_CONV(const Source*,		const Target*,				);

		TEST_CONV(const Source*,		Target*,					);
		TEST_CONV(const Source&,		Target&,					);
		TEST_CONV(const Source&&,		Target&,					);

		TEST_CONV(Source*,				const Target*,				);

		TEST_CONV(Source*,				Target*,					);
		TEST_CONV(Source&,				Target&,					);
		TEST_CONV(Source&&,				Target&,					);
#undef TEST_CONV
	}
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
,	);
	COMPILE_PROGRAM(program, pa, input);

	{
#define TEST_CONV(FROM, TO) TEST_CONV_TYPE(FROM, TO, Illegal)
		TEST_CONV(const Source*,		const Target*,				);
		TEST_CONV(const Source&,		const Target&,				);
		TEST_CONV(const Source&,		const Target&&,				);
		TEST_CONV(const Source&&,		const Target&,				);
		TEST_CONV(const Source&&,		const Target&&,				);

		TEST_CONV(const Source*,		Target*,					);
		TEST_CONV(const Source&,		Target&,					);
		TEST_CONV(const Source&,		Target&&,					);
		TEST_CONV(const Source&&,		Target&,					);
		TEST_CONV(const Source&&,		Target&&,					);

		TEST_CONV(Source*,				const Target*,				);
		TEST_CONV(Source&,				const Target&,				);
		TEST_CONV(Source&,				const Target&&,				);
		TEST_CONV(Source&&,				const Target&,				);
		TEST_CONV(Source&&,				const Target&&,				);

		TEST_CONV(Source*,				Target*,					);
		TEST_CONV(Source&,				Target&,					);
		TEST_CONV(Source&,				Target&&,					);
		TEST_CONV(Source&&,				Target&,					);
		TEST_CONV(Source&&,				Target&&,					);
#undef TEST_CONV
	}

	{
#define TEST_CONV(FROM, TO) TEST_CONV_TYPE(FROM, TO, Illegal)
		TEST_CONV(const Source,			Target,						);
		TEST_CONV(const Source&,		Target,						);
		TEST_CONV(const Source&&,		Target,						);
		TEST_CONV(Source,				Target,						);
		TEST_CONV(Source&,				Target,						);
		TEST_CONV(Source&&,				Target,						);

		TEST_CONV(const Source,			const Target,				);
		TEST_CONV(const Source&,		const Target,				);
		TEST_CONV(const Source&&,		const Target,				);
		TEST_CONV(Source,				const Target,				);
		TEST_CONV(Source&,				const Target,				);
		TEST_CONV(Source&&,				const Target,				);
#undef TEST_CONV
	}
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
,	);
	COMPILE_PROGRAM(program, pa, input);

	{
#define TEST_CONV(FROM, TO) TEST_CONV_TYPE(FROM, TO, UserDefinedConversion)
		TEST_CONV(const Source&,		const TargetA&,				);
		TEST_CONV(const Source&,		const TargetA&&,			);
		TEST_CONV(const Source&&,		const TargetA&,				);
		TEST_CONV(const Source&&,		const TargetA&&,			);

		TEST_CONV(const Source&,		volatile TargetA&&,			);
		TEST_CONV(const Source&&,		volatile TargetA&&,			);

		TEST_CONV(Source&,				const TargetA&,				);
		TEST_CONV(Source&,				const TargetA&&,			);
		TEST_CONV(Source&&,				const TargetA&,				);
		TEST_CONV(Source&&,				const TargetA&&,			);

		TEST_CONV(Source&,				volatile TargetA&&,			);
		TEST_CONV(Source&&,				volatile TargetA&&,			);
#undef TEST_CONV
	}

	{
#define TEST_CONV(FROM, TO) TEST_CONV_TYPE(FROM, TO, UserDefinedConversion)
		TEST_CONV(const Source,			volatile TargetA,			);
		TEST_CONV(const Source&,		volatile TargetA,			);
		TEST_CONV(const Source&&,		volatile TargetA,			);
		TEST_CONV(Source,				volatile TargetA,			);
		TEST_CONV(Source&,				volatile TargetA,			);
		TEST_CONV(Source&&,				volatile TargetA,			);

		TEST_CONV(const Source,			const TargetA,				);
		TEST_CONV(const Source&,		const TargetA,				);
		TEST_CONV(const Source&&,		const TargetA,				);
		TEST_CONV(Source,				const TargetA,				);
		TEST_CONV(Source&,				const TargetA,				);
		TEST_CONV(Source&&,				const TargetA,				);
#undef TEST_CONV
	}

	{
#define TEST_CONV(FROM, TO) TEST_CONV_TYPE(FROM, TO, Illegal)
		TEST_CONV(const Source*,		const TargetA*,				);

		TEST_CONV(const Source*,		TargetA*,					);
		TEST_CONV(const Source&,		TargetA&,					);
		TEST_CONV(const Source&&,		TargetA&,					);

		TEST_CONV(Source*,				const TargetA*,				);

		TEST_CONV(Source*,				TargetA*,					);
		TEST_CONV(Source&,				TargetA&,					);
		TEST_CONV(Source&&,				TargetA&,					);
#undef TEST_CONV
	}

	{
#define TEST_CONV(FROM, TO) TEST_CONV_TYPE(FROM, TO, UserDefinedConversion)
		TEST_CONV(Source&,				const TargetB&,				);
		TEST_CONV(Source&,				const TargetB&&,			);
		TEST_CONV(Source&&,				const TargetB&,				);
		TEST_CONV(Source&&,				const TargetB&&,			);

		TEST_CONV(Source&,				volatile TargetB&&,			);
		TEST_CONV(Source&&,				volatile TargetB&&,			);
#undef TEST_CONV
	}

	{
#define TEST_CONV(FROM, TO) TEST_CONV_TYPE(FROM, TO, UserDefinedConversion)
		TEST_CONV(Source,				volatile TargetB,			);
		TEST_CONV(Source&,				volatile TargetB,			);
		TEST_CONV(Source&&,				volatile TargetB,			);

		TEST_CONV(Source,				const TargetB,				);
		TEST_CONV(Source&,				const TargetB,				);
		TEST_CONV(Source&&,				const TargetB,				);
#undef TEST_CONV
	}

	{
#define TEST_CONV(FROM, TO) TEST_CONV_TYPE(FROM, TO, Illegal)
		TEST_CONV(const Source*,		const TargetB*,				);
		TEST_CONV(const Source&,		const TargetB&,				);
		TEST_CONV(const Source&,		const TargetB&&,			);
		TEST_CONV(const Source&&,		const TargetB&,				);
		TEST_CONV(const Source&&,		const TargetB&&,			);

		TEST_CONV(const Source*,		TargetB*,					);
		TEST_CONV(const Source&,		TargetB&,					);
		TEST_CONV(const Source&,		TargetB&&,					);
		TEST_CONV(const Source&&,		TargetB&,					);
		TEST_CONV(const Source&&,		TargetB&&,					);

		TEST_CONV(Source*,				const TargetB*,				);

		TEST_CONV(Source*,				TargetB*,					);
		TEST_CONV(Source&,				TargetB&,					);
		TEST_CONV(Source&&,				TargetB&,					);
#undef TEST_CONV
	}

	{
#define TEST_CONV(FROM, TO) TEST_CONV_TYPE(FROM, TO, Illegal)
		TEST_CONV(const Source,			TargetB,					);
		TEST_CONV(const Source&,		TargetB,					);
		TEST_CONV(const Source&&,		TargetB,					);

		TEST_CONV(const Source,			const TargetB,				);
		TEST_CONV(const Source&,		const TargetB,				);
		TEST_CONV(const Source&&,		const TargetB,				);
#undef TEST_CONV
	}
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
,	);
	COMPILE_PROGRAM(program, pa, input);

	{
#define TEST_CONV(FROM, TO) TEST_CONV_TYPE(FROM, TO, Illegal)
		TEST_CONV(const Source*,		const TargetA*,				);
		TEST_CONV(const Source&,		const TargetA&,				);
		TEST_CONV(const Source&,		const TargetA&&,			);
		TEST_CONV(const Source&&,		const TargetA&,				);
		TEST_CONV(const Source&&,		const TargetA&&,			);

		TEST_CONV(const Source*,		TargetA*,					);
		TEST_CONV(const Source&,		TargetA&,					);
		TEST_CONV(const Source&,		TargetA&&,					);
		TEST_CONV(const Source&&,		TargetA&,					);
		TEST_CONV(const Source&&,		TargetA&&,					);

		TEST_CONV(Source*,				const TargetA*,				);
		TEST_CONV(Source&,				const TargetA&,				);
		TEST_CONV(Source&,				const TargetA&&,			);
		TEST_CONV(Source&&,				const TargetA&,				);
		TEST_CONV(Source&&,				const TargetA&&,			);

		TEST_CONV(Source*,				TargetA*,					);
		TEST_CONV(Source&,				TargetA&,					);
		TEST_CONV(Source&,				TargetA&&,					);
		TEST_CONV(Source&&,				TargetA&,					);
		TEST_CONV(Source&&,				TargetA&&,					);
#undef TEST_CONV
	}

	{
#define TEST_CONV(FROM, TO) TEST_CONV_TYPE(FROM, TO, Illegal)
		TEST_CONV(const Source*,		const TargetB*,				);
		TEST_CONV(const Source&,		const TargetB&,				);
		TEST_CONV(const Source&,		const TargetB&&,			);
		TEST_CONV(const Source&&,		const TargetB&,				);
		TEST_CONV(const Source&&,		const TargetB&&,			);

		TEST_CONV(const Source*,		TargetB*,					);
		TEST_CONV(const Source&,		TargetB&,					);
		TEST_CONV(const Source&,		TargetB&&,					);
		TEST_CONV(const Source&&,		TargetB&,					);
		TEST_CONV(const Source&&,		TargetB&&,					);

		TEST_CONV(Source*,				const TargetB*,				);
		TEST_CONV(Source&,				const TargetB&,				);
		TEST_CONV(Source&,				const TargetB&&,			);
		TEST_CONV(Source&&,				const TargetB&,				);
		TEST_CONV(Source&&,				const TargetB&&,			);

		TEST_CONV(Source*,				TargetB*,					);
		TEST_CONV(Source&,				TargetB&,					);
		TEST_CONV(Source&,				TargetB&&,					);
		TEST_CONV(Source&&,				TargetB&,					);
		TEST_CONV(Source&&,				TargetB&&,					);
#undef TEST_CONV
	}
}

#undef TEST_DECL
#undef TEST_CONV_TYPE_
#undef TEST_CONV_TYPE

#pragma warning (push)