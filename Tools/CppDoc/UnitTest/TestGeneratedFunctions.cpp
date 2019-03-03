#include "Util.h"
#include "TestGeneratedFunctions_Input.h"
#include "TestGeneratedFunctions_Macro.h"
#include <type_traits>

using namespace std;

WString LoadGeneratedFunctionsCode()
{
	FilePath input = L"../UnitTest/TestGeneratedFunctions_Input.h";

	WString code;
	TEST_ASSERT(File(input).ReadAllTextByBom(code));

	return code;
}

TEST_CASE(TestGC_DeltedDueToUserDefined)
{
	COMPILE_PROGRAM(program, pa, LoadGeneratedFunctionsCode().Buffer());
}