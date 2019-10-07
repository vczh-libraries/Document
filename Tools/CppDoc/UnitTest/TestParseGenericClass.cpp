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
	AssertType(pa,		L"Ptr<int>",	L"Ptr<int>",	L"::Ptr<int>"						);
}