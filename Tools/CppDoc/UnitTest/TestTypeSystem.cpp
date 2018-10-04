#include <TypeSystem.h>

TEST_CASE(TestTypeSystem_Primitive)
{
	auto tsys = ITsysAlloc::Create();
#define TEST_PRIMITIVE_BYTES(TYPE, BYTES) TEST_ASSERT(tsys->PrimitiveOf({TsysPrimitiveType::TYPE,TsysBytes::BYTES})==tsys->PrimitiveOf({TsysPrimitiveType::TYPE,TsysBytes::BYTES}))
#define TEST_PRIMITIVE(TYPE)\
	TEST_PRIMITIVE_BYTES(TYPE, _1);\
	TEST_PRIMITIVE_BYTES(TYPE, _2);\
	TEST_PRIMITIVE_BYTES(TYPE, _4);\
	TEST_PRIMITIVE_BYTES(TYPE, _8);\
	TEST_PRIMITIVE_BYTES(TYPE, _10);\
	TEST_PRIMITIVE_BYTES(TYPE, _16)

	TEST_PRIMITIVE(Signed);
	TEST_PRIMITIVE(Unsigned);
	TEST_PRIMITIVE(Float);
	TEST_PRIMITIVE(Char);
	TEST_PRIMITIVE(Bool);
	TEST_PRIMITIVE(Void);

#undef TEST_PRIMITIVE
#undef TEST_PRIMITIVE_BYTES
}

TEST_CASE(TestTypeSystem_Decl)
{
}

TEST_CASE(TestTypeSystem_GenericArg)
{
}

TEST_CASE(TestTypeSystem_LRef)
{
}

TEST_CASE(TestTypeSystem_RRef)
{
}

TEST_CASE(TestTypeSystem_Ptr)
{
}

TEST_CASE(TestTypeSystem_Array)
{
}

TEST_CASE(TestTypeSystem_CV)
{
}

TEST_CASE(TestTypeSystem_Member)
{
}

TEST_CASE(TestTypeSystem_Function)
{
}

TEST_CASE(TestTypeSystem_Generic)
{
}