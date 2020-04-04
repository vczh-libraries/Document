#ifndef VCZH_DOCUMENT_CPPDOCTEST_TESTPARSE
#define VCZH_DOCUMENT_CPPDOCTEST_TESTPARSE

#include <Parser.h>
#include <VlppOS.h>
#include <Ast_Resolving.h>

using namespace vl::stream;
using namespace vl::filesystem;

extern Ptr<RegexLexer>		GlobalCppLexer();
extern void					Log(Ptr<Type> type, StreamWriter& writer);
extern void					Log(Ptr<Expr> expr, StreamWriter& writer);
extern void					Log(Ptr<Stat> stat, StreamWriter& writer, vint indentation);
extern void					Log(Ptr<Declaration> decl, StreamWriter& writer, vint indentation, bool semicolon);
extern void					Log(Ptr<Program> program, StreamWriter& writer);
extern void					Log(ITsys* tsys, StreamWriter& writer);

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
	TEST_CASE(L"[TYPE] " + WString(input))
	{
		const wchar_t* tsys[] = { logTsys...,L"" };
		AssertTypeInternal(input, log, tsys, (vint)(sizeof...(logTsys)), pa);
	});
}

template<typename... T>
void AssertType(const wchar_t* input, const wchar_t* log, T... logTsys)
{
	ParsingArguments pa(new Symbol(symbol_component::SymbolCategory::Normal), ITsysAlloc::Create(), nullptr);
	AssertType(pa, input, log, logTsys...);
}

template<typename... T>
void AssertExpr_NotTestCase(ParsingArguments& pa, const wchar_t* input, const wchar_t* log, T... logTsys)
{
	const wchar_t* tsys[] = { logTsys...,L"" };
	AssertExprInternal(input, log, tsys, (vint)(sizeof...(logTsys)), pa);
}

template<typename... T>
void AssertExpr(ParsingArguments& pa, const wchar_t* input, const wchar_t* log, T... logTsys)
{
	TEST_CASE(L"[EXPR] " + WString(input))
	{
		AssertExpr_NotTestCase(pa, input, log, logTsys...);
	});
}

template<typename... T>
void AssertExpr(const wchar_t* input, const wchar_t* log, T... logTsys)
{
	ParsingArguments pa(new Symbol(symbol_component::SymbolCategory::Normal), ITsysAlloc::Create(), nullptr);
	AssertExpr(pa, input, log, logTsys...);
}

#define TEST_DECL(...) __VA_ARGS__ wchar_t input[] = L#__VA_ARGS__

#define TOKEN_READER(INPUT)\
	Array<wchar_t> reader_array((vint)wcslen(INPUT) + 1);\
	memcpy(&reader_array[0], INPUT, (size_t)(reader_array.Count()) * sizeof(wchar_t));\
	WString reader_string(&reader_array[0], false);\
	CppTokenReader reader(GlobalCppLexer(), reader_string)\

#define COMPILE_PROGRAM_WITH_RECORDER(PROGRAM, PA, INPUT, RECORDER)\
	TOKEN_READER(INPUT);\
	auto cursor = reader.GetFirstToken();\
	ParsingArguments PA(new Symbol(symbol_component::SymbolCategory::Normal), ITsysAlloc::Create(), RECORDER);\
	Ptr<Program> PROGRAM;\
	TEST_CASE(L"ParseProgram + EvaluateProgram")\
	{\
		PROGRAM = ParseProgram(PA, cursor);\
		TEST_ASSERT(!cursor);\
		TEST_ASSERT(PROGRAM);\
		EvaluateProgram(PA, PROGRAM);\
	})\

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

template<typename T>
bool AssertSymbol(
	SortedList<vint>& accessed,
	CppName& name,
	List<Symbol*>& resolvedSymbols,
	bool reIndex,
	bool _reIndex,
	vint _index,
	WString _name,
	vint _tRow,
	vint _tCol,
	vint _pRow,
	vint _pCol
)
{
	if (reIndex == _reIndex && name.nameTokens[0].rowStart == _tRow && name.nameTokens[0].columnStart == _tCol)
	{
		TEST_ASSERT(name.name == _name);
		TEST_ASSERT(resolvedSymbols.Count() == 1);
		auto symbol = resolvedSymbols[0];
		auto decl = symbol->GetAnyForwardDecl<T>();
		TEST_ASSERT(decl);
		TEST_ASSERT(decl->name.name == _name || decl->name.name == L"operator " + _name);
		TEST_ASSERT(decl->name.nameTokens[0].rowStart == _pRow);
		TEST_ASSERT(decl->name.nameTokens[0].columnStart == _pCol);
		if (!accessed.Contains(_index)) accessed.Add(_index);
		return true;
	}
	return false;
}

template<>
inline bool AssertSymbol<void>(
	SortedList<vint>& accessed,
	CppName& name,
	List<Symbol*>& resolvedSymbols,
	bool reIndex,
	bool _reIndex,
	vint _index,
	WString _name,
	vint _tRow,
	vint _tCol,
	vint _pRow,
	vint _pCol
	)
{
	if (reIndex == _reIndex && name.nameTokens[0].rowStart == _tRow && name.nameTokens[0].columnStart == _tCol)
	{
		TEST_ASSERT(name.name == _name);
		TEST_ASSERT(resolvedSymbols.Count() == 1);
		auto symbol = resolvedSymbols[0];
		switch (symbol->kind)
		{
		case symbol_component::SymbolKind::GenericTypeArgument:
		case symbol_component::SymbolKind::GenericValueArgument:
			break;
		default:
			TEST_ASSERT(false);
		}

		if (symbol->GetParentScope()->kind == symbol_component::SymbolKind::Root)
		{
			// the declaration that should contain a TemplateSpec has not been created yet
			TEST_ASSERT(symbol->name == _name);
			TEST_ASSERT(_pRow == -1);
			TEST_ASSERT(_pCol == -1);
		}
		else
		{
			auto spec = symbol_type_resolving::GetTemplateSpecFromSymbol(symbol->GetParentScope());
			auto argTsys = symbol_type_resolving::EvaluateGenericArgumentSymbol(symbol);
			auto argIndex = argTsys->GetGenericArg().argIndex;
			auto arg = spec->arguments[argIndex];
			TEST_ASSERT(arg.name.name == _name);
			TEST_ASSERT(arg.name.nameTokens[0].rowStart == _pRow);
			TEST_ASSERT(arg.name.nameTokens[0].columnStart == _pCol);
		}
		if (!accessed.Contains(_index)) accessed.Add(_index);
		return true;
	}
	return false;
}

#define ASSERT_SYMBOL_INTERNAL(REINDEX, INDEX, NAME, TROW, TCOL, TYPE, PROW, PCOL)\
		if (AssertSymbol<TYPE>(accessed, name, resolvedSymbols, reIndex, REINDEX, INDEX, NAME, TROW, TCOL, PROW, PCOL)) \
		{ } else \

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

#define ASSERT_OVERLOADING_VERBOSE(INPUT, OUTPUT, OUTPUT_TSYS, ...)\
	RunOverloading<__VA_ARGS__, decltype((INPUT))>, \
	AssertExpr(pa, L#INPUT, OUTPUT, OUTPUT_TSYS)\

#define ASSERT_OVERLOADING_FORMATTED_VERBOSE(INPUT, OUTPUT_TSYS, ...)\
	ASSERT_OVERLOADING_VERBOSE(INPUT, L#INPUT, OUTPUT_TSYS, __VA_ARGS__)

#define ASSERT_OVERLOADING(INPUT, OUTPUT, ...)\
	ASSERT_OVERLOADING_VERBOSE(INPUT, OUTPUT, L#__VA_ARGS__ " $PR", __VA_ARGS__)\

#define ASSERT_OVERLOADING_LVALUE(INPUT, OUTPUT, ...)\
	ASSERT_OVERLOADING_VERBOSE(INPUT, OUTPUT, L#__VA_ARGS__ " $L", __VA_ARGS__)\

#define ASSERT_OVERLOADING_SIMPLE(INPUT, ...)\
	ASSERT_OVERLOADING(INPUT, L#INPUT, __VA_ARGS__)\

#define ASSERT_OVERLOADING_SIMPLE_LVALUE(INPUT, ...)\
	ASSERT_OVERLOADING_LVALUE(INPUT, L#INPUT, __VA_ARGS__)\

#endif