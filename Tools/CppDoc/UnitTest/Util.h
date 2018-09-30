#ifndef VCZH_DOCUMENT_CPPDOCTEST_TESTPARSE
#define VCZH_DOCUMENT_CPPDOCTEST_TESTPARSE

#include <Parser.h>

extern Ptr<RegexLexer>		GlobalCppLexer();
extern void					Log(Ptr<Type> type, StreamWriter& writer);
extern void					Log(Ptr<Expr> expr, StreamWriter& writer);
extern void					Log(Ptr<Stat> stat, StreamWriter& writer, vint indentation);
extern void					Log(Ptr<Declaration> decl, StreamWriter& writer, vint indentation, bool semicolon);
extern void					Log(Ptr<Program> program, StreamWriter& writer);

extern void					AssertMultilines(const WString& output, const WString& log);
extern void					AssertType(const WString& input, const WString& log);
extern void					AssertType(const WString& input, const WString& log, ParsingArguments& pa);
extern void					AssertStat(const WString& input, const WString& log);
extern void					AssertStat(const WString& input, const WString& log, ParsingArguments& pa);
extern void					AssertProgram(const WString& input, const WString& log, Ptr<IIndexRecorder> recorder = nullptr);

#define COMPILE_PROGRAM_WITH_RECORDER(PROGRAM, PA, INPUT, RECORDER)\
	CppTokenReader reader(GlobalCppLexer(), INPUT);\
	auto cursor = reader.GetFirstToken();\
	ParsingArguments PA(new Symbol, RECORDER);\
	auto PROGRAM = ParseProgram(PA, cursor);\
	TEST_ASSERT(!cursor)\

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

	void Index(CppName& name, Ptr<Resolving> resolving)
	{
		callback(name, resolving);
	}
};

template<typename T>
Ptr<IIndexRecorder> CreateTestIndexRecorder(T&& callback)
{
	return new TestIndexRecorder<T>(ForwardValue<T&&>(callback));
}

#endif