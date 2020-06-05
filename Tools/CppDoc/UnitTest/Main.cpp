#include <Lexer.h>

Ptr<RegexLexer> cppLexer;

Ptr<RegexLexer> GlobalCppLexer()
{
	return cppLexer;
}

int wmain(int argc, wchar_t* argv[])
{
	cppLexer = CreateCppLexer();
	int result = unittest::UnitTest::RunAndDisposeTests(argc, argv);
	cppLexer = nullptr;
	FinalizeGlobalStorage();
#ifdef VCZH_CHECK_MEMORY_LEAKS
	_CrtDumpMemoryLeaks();
#endif
	return result;
}