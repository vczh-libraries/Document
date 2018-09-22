#include <Lexer.h>

Ptr<RegexLexer> cppLexer;

Ptr<RegexLexer> GlobalCppLexer()
{
	return cppLexer;
}

int wmain(vint argc, wchar_t* args[])
{
	cppLexer = CreateCppLexer();
	unittest::UnitTest::RunAndDisposeTests();
	cppLexer = nullptr;
	FinalizeGlobalStorage();
#ifdef VCZH_CHECK_MEMORY_LEAKS
	_CrtDumpMemoryLeaks();
#endif
	return 0;
}