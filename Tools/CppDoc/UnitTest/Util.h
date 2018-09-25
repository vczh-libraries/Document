#ifndef VCZH_DOCUMENT_CPPDOCTEST_TESTPARSE
#define VCZH_DOCUMENT_CPPDOCTEST_TESTPARSE

#include <Parser.h>

extern Ptr<RegexLexer>		GlobalCppLexer();
extern void					Log(Ptr<Type> type, StreamWriter& writer);
extern void					Log(Ptr<Expr> expr, StreamWriter& writer);
extern void					Log(Ptr<Stat> stat, StreamWriter& writer);
extern void					Log(Ptr<Declaration> decl, StreamWriter& writer);
extern void					Log(Ptr<Program> program, StreamWriter& writer);

extern void					AssertType(const WString& type, const WString& log);
extern void					AssertProgram(const WString& input, const WString& log);

#define COMPILE_PROGRAM(PROGRAM, PA,INPUT)\
	CppTokenReader reader(GlobalCppLexer(), INPUT);\
	auto cursor = reader.GetFirstToken();\
	ParsingArguments PA(new Symbol, nullptr);\
	auto PROGRAM = ParseProgram(PA, cursor);\
	TEST_ASSERT(!cursor)\

#endif