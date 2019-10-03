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
)";

	COMPILE_PROGRAM(program, pa, input);
	AssertProgram(program, output);
}