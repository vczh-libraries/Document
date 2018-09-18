#include "Vlpp.h"

using namespace vl;

int wmain(vint argc, wchar_t* args[])
{
	unittest::UnitTest::RunAndDisposeTests();
	FinalizeGlobalStorage();
#ifdef VCZH_CHECK_MEMORY_LEAKS
	_CrtDumpMemoryLeaks();
#endif
	return 0;
}