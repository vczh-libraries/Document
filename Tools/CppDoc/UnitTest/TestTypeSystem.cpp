#include <Parser.h>

TEST_CASE(TestTypeSystem_Primitive)
{
	auto tsys = ITsysAlloc::Create();
#define TEST_PRIMITIVE_BYTES(TYPE, BYTES)\
	TEST_ASSERT(\
		tsys->PrimitiveOf({TsysPrimitiveType::TYPE,TsysBytes::BYTES})==\
		tsys->PrimitiveOf({TsysPrimitiveType::TYPE,TsysBytes::BYTES}))\

#define TEST_PRIMITIVE(TYPE)\
	TEST_PRIMITIVE_BYTES(TYPE, _1);\
	TEST_PRIMITIVE_BYTES(TYPE, _2);\
	TEST_PRIMITIVE_BYTES(TYPE, _4);\
	TEST_PRIMITIVE_BYTES(TYPE, _8)\

	TEST_PRIMITIVE(SInt);
	TEST_PRIMITIVE(UInt);
	TEST_PRIMITIVE(Float);
	TEST_PRIMITIVE(SChar);
	TEST_PRIMITIVE(UChar);
	TEST_PRIMITIVE(Bool);
	TEST_PRIMITIVE(Void);

#undef TEST_PRIMITIVE
#undef TEST_PRIMITIVE_BYTES

#define TEST_PRIMITIVE_BYTES(TYPE1, BYTES1, TYPE2, BYTES2)\
	TEST_ASSERT(\
		tsys->PrimitiveOf({ TsysPrimitiveType::TYPE1,TsysBytes::BYTES1 }) != \
		tsys->PrimitiveOf({ TsysPrimitiveType::TYPE2,TsysBytes::BYTES2 }))\

#define TEST_PRIMITIVE(TYPE1, TYPE2)\
	TEST_PRIMITIVE_BYTES(TYPE1, _1, TYPE2, _2);\
	TEST_PRIMITIVE_BYTES(TYPE1, _2, TYPE2, _4);\
	TEST_PRIMITIVE_BYTES(TYPE1, _4, TYPE2, _8);\
	TEST_PRIMITIVE_BYTES(TYPE1, _8, TYPE2, _1)\

	TEST_PRIMITIVE(SInt, UInt);
	TEST_PRIMITIVE(UInt, Float);
	TEST_PRIMITIVE(Float, SChar);
	TEST_PRIMITIVE(SChar, UChar);
	TEST_PRIMITIVE(UChar, Bool);
	TEST_PRIMITIVE(Bool, Void);
	TEST_PRIMITIVE(Void, SInt);

#undef TEST_PRIMITIVE
#undef TEST_PRIMITIVE_BYTES
}

TEST_CASE(TestTypeSystem_Decl)
{
	auto n1 = MakePtr<Symbol>();
	auto n2 = MakePtr<Symbol>();
	auto tsys = ITsysAlloc::Create();
	TEST_ASSERT(tsys->DeclOf(n1.Obj()) == tsys->DeclOf(n1.Obj()));
	TEST_ASSERT(tsys->DeclOf(n1.Obj()) != tsys->DeclOf(n2.Obj()));
}

TEST_CASE(TestTypeSystem_GenericArg)
{
	auto n1 = MakePtr<Symbol>();
	auto n2 = MakePtr<Symbol>();
	auto tsys = ITsysAlloc::Create();
	TEST_ASSERT(tsys->GenericArgOf(n1.Obj()) == tsys->GenericArgOf(n1.Obj()));
	TEST_ASSERT(tsys->GenericArgOf(n1.Obj()) != tsys->GenericArgOf(n2.Obj()));
}

TEST_CASE(TestTypeSystem_LRef)
{
	auto tsys = ITsysAlloc::Create();
	auto tvoid = tsys->PrimitiveOf({ TsysPrimitiveType::Void,TsysBytes::_1 });
	TEST_ASSERT(tvoid->LRefOf() == tvoid->LRefOf());
}

TEST_CASE(TestTypeSystem_RRef)
{
	auto tsys = ITsysAlloc::Create();
	auto tvoid = tsys->PrimitiveOf({ TsysPrimitiveType::Void,TsysBytes::_1 });
	TEST_ASSERT(tvoid->RRefOf() == tvoid->RRefOf());
}

TEST_CASE(TestTypeSystem_Ptr)
{
	auto tsys = ITsysAlloc::Create();
	auto tvoid = tsys->PrimitiveOf({ TsysPrimitiveType::Void,TsysBytes::_1 });
	TEST_ASSERT(tvoid->PtrOf() == tvoid->PtrOf());
}

TEST_CASE(TestTypeSystem_Array)
{
	auto tsys = ITsysAlloc::Create();
	auto tvoid = tsys->PrimitiveOf({ TsysPrimitiveType::Void,TsysBytes::_1 });
	TEST_ASSERT(tvoid->ArrayOf(1) == tvoid->ArrayOf(1));
	TEST_ASSERT(tvoid->ArrayOf(2) == tvoid->ArrayOf(2));
	TEST_ASSERT(tvoid->ArrayOf(1) != tvoid->ArrayOf(2));
}

TEST_CASE(TestTypeSystem_CV)
{
	auto tsys = ITsysAlloc::Create();
	auto tvoid = tsys->PrimitiveOf({ TsysPrimitiveType::Void,TsysBytes::_1 });

#define CV(INDEX) {((INDEX)>>1)%2==1, ((INDEX)>>0)%2==1}
	for (vint i = 0; i < 4; i++)
	{
		TEST_ASSERT(tvoid->CVOf(CV(i)) == tvoid->CVOf(CV(i)));
		for (vint j = 0; j < 4; j++)
		{
			if (i != j)
			{
				TEST_ASSERT(tvoid->CVOf(CV(i)) != tvoid->CVOf(CV(j)));
			}
		}
	}
#undef CV
}

TEST_CASE(TestTypeSystem_Member)
{
	auto tsys = ITsysAlloc::Create();
	auto tvoid = tsys->PrimitiveOf({ TsysPrimitiveType::Void,TsysBytes::_1 });
	auto tbool = tsys->PrimitiveOf({ TsysPrimitiveType::Bool,TsysBytes::_1 });
	auto tchar = tsys->PrimitiveOf({ TsysPrimitiveType::SChar,TsysBytes::_1 });
	TEST_ASSERT(tvoid->MemberOf(tbool) == tvoid->MemberOf(tbool));
	TEST_ASSERT(tvoid->MemberOf(tbool) != tvoid->MemberOf(tchar));
}

TEST_CASE(TestTypeSystem_Function)
{
	auto n = MakePtr<Symbol>();
	auto tsys = ITsysAlloc::Create();
	auto tvoid = tsys->PrimitiveOf({ TsysPrimitiveType::Void,TsysBytes::_1 });
	auto tdecl = tsys->DeclOf(n.Obj());
	auto tgarg = tsys->GenericArgOf(n.Obj());

	List<ITsys*> types1;
	types1.Add(tdecl);
	types1.Add(tgarg);

	List<ITsys*> types2;
	types2.Add(tgarg);
	types2.Add(tdecl);

	TEST_ASSERT(tvoid->FunctionOf(types1) == tvoid->FunctionOf(types1));
	TEST_ASSERT(tvoid->FunctionOf(types1) != tvoid->FunctionOf(types2));
}

TEST_CASE(TestTypeSystem_Generic)
{
	auto n = MakePtr<Symbol>();
	auto tsys = ITsysAlloc::Create();
	auto tvoid = tsys->PrimitiveOf({ TsysPrimitiveType::Void,TsysBytes::_1 });
	auto tdecl = tsys->DeclOf(n.Obj());
	auto tgarg = tsys->GenericArgOf(n.Obj());

	List<ITsys*> types1;
	types1.Add(tdecl);
	types1.Add(tgarg);

	List<ITsys*> types2;
	types2.Add(tgarg);
	types2.Add(tdecl);

	TEST_ASSERT(tvoid->GenericOf(types1) == tvoid->GenericOf(types1));
	TEST_ASSERT(tvoid->GenericOf(types1) != tvoid->GenericOf(types2));
}

TEST_CASE(TestTypeSystem_Type)
{
	auto n = MakePtr<Symbol>();
	auto tsys = ITsysAlloc::Create();
	auto tvoid = tsys->PrimitiveOf({ TsysPrimitiveType::Void,TsysBytes::_1 });
	auto tdecl = tsys->DeclOf(n.Obj());
	auto tgarg = tsys->GenericArgOf(n.Obj());

	TEST_ASSERT(tvoid->GetType() == TsysType::Primitive);
	TEST_ASSERT(tdecl->GetType() == TsysType::Decl);
	TEST_ASSERT(tgarg->GetType() == TsysType::GenericArg);

	TEST_ASSERT(tvoid->LRefOf()->GetType() == TsysType::LRef);
	TEST_ASSERT(tvoid->RRefOf()->GetType() == TsysType::RRef);
	TEST_ASSERT(tvoid->PtrOf()->GetType() == TsysType::Ptr);
	TEST_ASSERT(tvoid->ArrayOf(1)->GetType() == TsysType::Array);
	TEST_ASSERT(tvoid->CVOf({ false,false }) == tvoid);
	TEST_ASSERT(tvoid->CVOf({ true,true })->GetType() == TsysType::CV);
	TEST_ASSERT(tvoid->MemberOf(tdecl)->GetType() == TsysType::Member);

	List<ITsys*> types;
	types.Add(tdecl);
	types.Add(tgarg);
	TEST_ASSERT(tvoid->FunctionOf(types)->GetType() == TsysType::Function);
	TEST_ASSERT(tvoid->GenericOf(types)->GetType() == TsysType::Generic);
}