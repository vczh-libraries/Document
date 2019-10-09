#include <Ast_Decl.h>
#include "Util.h"

TEST_CASE(TestParseGenericClass)
{
	auto input = LR"(
template<typename T>
struct Ptr
{
private:
	T* pointer = nullptr;
public:
	Ptr() = default;
	Ptr(T* _pointer):pointer(_pointer){}
	~Ptr(){delete pointer;}
	operator T*()const{return pointer;}
	T* operator->()const{return pointer;}
};
)";

	auto output = LR"(
template<typename T>
struct Ptr
{
	private pointer: T * = nullptr;
	public __forward __ctor $__ctor: __null () = default;
	public __ctor $__ctor: __null (_pointer: T *)
		: pointer(_pointer)
	{
	}
	public __dtor ~Ptr: __null ()
	{
		delete (pointer);
	}
	public __type $__type: T * () const
	{
		return pointer;
	}
	public operator ->: T * () const
	{
		return pointer;
	}
};
)";

	COMPILE_PROGRAM(program, pa, input);
	AssertProgram(program, output);

	AssertType(pa,		L"Ptr",			L"Ptr",			L"<::Ptr::[T]> ::Ptr<::Ptr::[T]>"	);
	AssertType(pa,		L"Ptr<int>",	L"Ptr<int>",	L"::Ptr<__int32>"					);
}

TEST_CASE(TestParseGenericClass_NestedUsedOutside)
{
	auto input = LR"(
template<typename TA>
struct GA
{
	struct CB
	{
		struct CC
		{
		};

		template<typename TC>
		struct GC
		{
		};
	};

	template<typename TB>
	struct GB
	{
		struct CC
		{
		};

		template<typename TC>
		struct GC
		{
		};
	};
};
)";
	COMPILE_PROGRAM(program, pa, input);

	AssertType(pa,		L"GA",								L"GA",									L"<::GA::[TA]> ::GA<::GA::[TA]>"															);
	AssertType(pa,		L"GA<int>::CB",						L"GA<int>::CB",							L"::GA<__int32> => ::GA::CB"																);
	AssertType(pa,		L"GA<int>::GB",						L"GA<int>::GB",							L"<::GA::GB::[TB]> ::GA<__int32> => ::GA::GB"												);
	AssertType(pa,		L"GA<int>::GB<bool>",				L"GA<int>::GB<bool>",					L"::GA<__int32> => ::GA::GB<bool>"															);
	AssertType(pa,		L"GA<int>::CB::CC",					L"GA<int>::CB::CC",						L"::GA<__int32> => ::GA::CB::CC"															);
	AssertType(pa,		L"GA<int>::CB::GC",					L"GA<int>::CB::GC",						L"<::GA::CB::GC::[TC]> ::GA<__int32> => ::GA::CB::GC<::GA::CB::GC::[TC]>"					);
	AssertType(pa,		L"GA<int>::CB::GC<float>",			L"GA<int>::CB::GC<float>",				L"::GA<__int32> => ::GA::CB::GC<float>"														);
	AssertType(pa,		L"GA<int>::GB<bool>::CC",			L"GA<int>::GB<bool>::CC",				L"::GA<__int32> => ::GA::GB<bool> => ::GA::GB::CC"											);
	AssertType(pa,		L"GA<int>::GB<bool>::GC",			L"GA<int>::GB<bool>::GC",				L"<::GA::GB::GC::[TC]> ::GA<__int32> => ::GA::GB<bool> => ::GA::GB::GC<::GA::GB::GC::[TC]>"	);
	AssertType(pa,		L"GA<int>::GB<bool>::GC<float>",	L"GA<int>::GB<bool>::GC<float>",		L"::GA<__int32> => ::GA::GB<bool> => ::GA::GB::GC<float>"									);
}

TEST_CASE(TestParseGenericClass_NestedThisType)
{
}