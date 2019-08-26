#ifndef VCZH_DOCUMENT_CPPDOCTEST_TESTPARSE
#define VCZH_DOCUMENT_CPPDOCTEST_TESTPARSE

#include <Parser.h>

extern Ptr<RegexLexer>		GlobalCppLexer();
extern void					Log(Ptr<Type> type, StreamWriter& writer);
extern void					Log(Ptr<Expr> expr, StreamWriter& writer);
extern void					Log(Ptr<Stat> stat, StreamWriter& writer, vint indentation);
extern void					Log(Ptr<Declaration> decl, StreamWriter& writer, vint indentation, bool semicolon);
extern void					Log(Ptr<Program> program, StreamWriter& writer);
extern void					Log(ITsys* tsys, StreamWriter& writer);

extern void					RefineInput(wchar_t* input);
extern void					AssertMultilines(const WString& output, const WString& log);
extern void					AssertTypeInternal(const wchar_t* input, const wchar_t* log, const wchar_t** logTsys, vint count, ParsingArguments& pa);
extern void					AssertExprInternal(const wchar_t* input, const wchar_t* log, const wchar_t** logTsys, vint count, ParsingArguments& pa);
extern void					AssertStat(const wchar_t* input, const wchar_t* log);
extern void					AssertStat(ParsingArguments& pa, const wchar_t* input, const wchar_t* log);
extern void					AssertProgram(const wchar_t* input, const wchar_t* log, Ptr<IIndexRecorder> recorder = nullptr);
extern void					AssertProgram(Ptr<Program> program, const wchar_t* log);

template<typename... T>
void AssertType(ParsingArguments& pa, const wchar_t* input, const wchar_t* log, T... logTsys)
{
	const wchar_t* tsys[] = { logTsys...,L"" };
	AssertTypeInternal(input, log, tsys, (vint)(sizeof...(logTsys)), pa);
}

template<typename... T>
void AssertType(const wchar_t* input, const wchar_t* log, T... logTsys)
{
	ParsingArguments pa(new Symbol(symbol_component::SymbolCategory::Normal), ITsysAlloc::Create(), nullptr);
	AssertType(pa, input, log, logTsys...);
}

template<typename... T>
void AssertExpr(ParsingArguments& pa, const wchar_t* input, const wchar_t* log, T... logTsys)
{
	const wchar_t* tsys[] = { logTsys...,L"" };
	AssertExprInternal(input, log, tsys, (vint)(sizeof...(logTsys)), pa);
}

template<typename... T>
void AssertExpr(const wchar_t* input, const wchar_t* log, T... logTsys)
{
	ParsingArguments pa(new Symbol(symbol_component::SymbolCategory::Normal), ITsysAlloc::Create(), nullptr);
	AssertExpr(pa, input, log, logTsys...);
}

#define TEST_DECL(SOMETHING) SOMETHING wchar_t input[] = L#SOMETHING; RefineInput(input)

#define TOKEN_READER(INPUT)\
	Array<wchar_t> reader_array((vint)wcslen(INPUT) + 1);\
	memcpy(&reader_array[0], INPUT, (size_t)(reader_array.Count()) * sizeof(wchar_t));\
	RefineInput(&reader_array[0]);\
	WString reader_string(&reader_array[0], false);\
	CppTokenReader reader(GlobalCppLexer(), reader_string)\

#define COMPILE_PROGRAM_WITH_RECORDER(PROGRAM, PA, INPUT, RECORDER)\
	TOKEN_READER(INPUT);\
	auto cursor = reader.GetFirstToken();\
	ParsingArguments PA(new Symbol(symbol_component::SymbolCategory::Normal), ITsysAlloc::Create(), RECORDER);\
	auto PROGRAM = ParseProgram(PA, cursor);\
	TEST_ASSERT(!cursor);\
	TEST_ASSERT(PROGRAM);\
	EvaluateProgram(PA, PROGRAM)\

#define COMPILE_PROGRAM(PROGRAM, PA, INPUT) COMPILE_PROGRAM_WITH_RECORDER(PROGRAM, PA, INPUT, nullptr)

template<typename T>
class TestIndexRecorder : public Object, public IIndexRecorder
{
protected:
	T						callback;

public:
	TestIndexRecorder(T&& _callback)
		:callback(ForwardValue<T&&>(_callback))
	{
	}

	void Index(CppName& name, List<Symbol*>& resolvedSymbols)
	{
		callback(name, (List<Symbol*>&)resolvedSymbols, false);
	}

	void IndexOverloadingResolution(CppName& name, List<Symbol*>& resolvedSymbols)
	{
		callback(name, (List<Symbol*>&)resolvedSymbols, true);
	}

	void ExpectValueButType(CppName& name, List<Symbol*>& resolvedSymbols)
	{
		TEST_ASSERT(false);
	}
};

template<typename T>
Ptr<IIndexRecorder> CreateTestIndexRecorder(T&& callback)
{
	return new TestIndexRecorder<T>(ForwardValue<T&&>(callback));
}

#define BEGIN_ASSERT_SYMBOL \
	CreateTestIndexRecorder([&](CppName& name, List<Symbol*>& resolvedSymbols, bool reIndex)\
	{\
		TEST_ASSERT(name.tokenCount > 0);\

#define END_ASSERT_SYMBOL \
		TEST_ASSERT(false);\
	})\

#define ASSERT_SYMBOL_INTERNAL(REINDEX, INDEX, NAME, TROW, TCOL, TYPE, PROW, PCOL)\
		if (reIndex == REINDEX && name.nameTokens[0].rowStart == TROW && name.nameTokens[0].columnStart == TCOL)\
		{\
			TEST_ASSERT(name.name == NAME);\
			TEST_ASSERT(resolvedSymbols.Count() == 1);\
			auto symbol = resolvedSymbols[0];\
			auto decl = symbol->GetAnyForwardDecl_NFFb<TYPE>();\
			TEST_ASSERT(decl);\
			TEST_ASSERT(decl->name.name == NAME || decl->name.name == L"operator " NAME);\
			TEST_ASSERT(decl->name.nameTokens[0].rowStart == PROW);\
			TEST_ASSERT(decl->name.nameTokens[0].columnStart == PCOL);\
			if (!accessed.Contains(INDEX)) accessed.Add(INDEX);\
		} else\

#define ASSERT_SYMBOL(INDEX, NAME, TROW, TCOL, TYPE, PROW, PCOL)\
		ASSERT_SYMBOL_INTERNAL(false, INDEX, NAME, TROW, TCOL, TYPE, PROW, PCOL)

#define ASSERT_SYMBOL_OVERLOAD(INDEX, NAME, TROW, TCOL, TYPE, PROW, PCOL)\
		ASSERT_SYMBOL_INTERNAL(true, INDEX, NAME, TROW, TCOL, TYPE, PROW, PCOL)

template<typename T, typename U>
struct IntIfSameType
{
	using Type = void;
};

template<typename T>
struct IntIfSameType<T, T>
{
	using Type = int;
};

template<typename T, typename U>
void RunOverloading()
{
	typename IntIfSameType<T, U>::Type test = 0;
}

#define ASSERT_OVERLOADING(INPUT, OUTPUT, TYPE)\
	RunOverloading<TYPE, decltype(INPUT)>, \
	AssertExpr(pa, L#INPUT, OUTPUT, L#TYPE " $PR")\

#endif