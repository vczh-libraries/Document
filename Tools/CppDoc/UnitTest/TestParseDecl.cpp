#include <Ast_Decl.h>
#include "Util.h"

TEST_CASE(TestParseDecl_Namespaces)
{
	auto input = LR"(
namespace vl {}
namespace vl::presentation::controls {}
)";
	auto output = LR"(
namespace vl
{
}
namespace vl
{
	namespace presentation
	{
		namespace controls
		{
		}
	}
}
)";
	AssertProgram(input, output);
}

TEST_CASE(TestParseDecl_Enums)
{
	auto input = LR"(
enum e1;
enum e2 : int;
enum class e3;
enum class e4 : int;
enum e5
{
	x,
	y = 1
};
enum class e6
{
	x,
	y,
	z,
};
)";
	auto output = LR"(
__forward enum e1;
__forward enum e2 : int;
__forward enum class e3;
__forward enum class e4 : int;
enum e5
{
	x,
	y = 1,
};
enum class e6
{
	x,
	y,
	z,
};
)";
	AssertProgram(input, output);
}

TEST_CASE(TestParseDecl_EnumsConnectForward)
{
	auto input = LR"(
namespace a::b
{
	enum class A;
	enum class A;
}
namespace a::b
{
	enum class A {};
}
namespace a::b
{
	enum class A;
	enum class A;
}
)";
	COMPILE_PROGRAM(program, pa, input);
	TEST_ASSERT(pa.root->children[L"a"].Count() == 1);
	TEST_ASSERT(pa.root->children[L"a"][0]->children[L"b"].Count() == 1);
	TEST_ASSERT(pa.root->children[L"a"][0]->children[L"b"][0]->children[L"A"].Count() == 5);
	const auto& symbols = pa.root->children[L"a"][0]->children[L"b"][0]->children[L"A"];

	for (vint i = 0; i < 5; i++)
	{
		auto& symbol = symbols[i];
		if (i == 2)
		{
			TEST_ASSERT(symbol->isForwardDeclaration == false);
			TEST_ASSERT(symbol->forwardDeclarationRoot == nullptr);
			TEST_ASSERT(symbol->forwardDeclarations.Count() == 4);
			TEST_ASSERT(symbol->forwardDeclarations[0] == symbols[0].Obj());
			TEST_ASSERT(symbol->forwardDeclarations[1] == symbols[1].Obj());
			TEST_ASSERT(symbol->forwardDeclarations[2] == symbols[3].Obj());
			TEST_ASSERT(symbol->forwardDeclarations[3] == symbols[4].Obj());
		}
		else
		{
			TEST_ASSERT(symbol->isForwardDeclaration == true);
			TEST_ASSERT(symbol->forwardDeclarationRoot == symbols[2].Obj());
			TEST_ASSERT(symbol->forwardDeclarations.Count() == 0);
		}
	}
}

TEST_CASE(TestParseDecl_Variables)
{
	auto input = LR"(
int x = 0;
int a, b = 0, c(0), d{0};
extern static mutable thread_local register int (*v1)();
)";
	auto output = LR"(
x: int = 0;
a: int;
b: int = 0;
c: int (0);
d: int {0};
__forward extern mutable register static thread_local v1: int () *;
)";
	AssertProgram(input, output);
}

TEST_CASE(TestParseDecl_VariablesConnectForward)
{
	auto input = LR"(
namespace a::b
{
	extern int x;
	extern int x;
}
namespace a::b
{
	int x = 0;
}
namespace a::b
{
	extern int x;
	extern int x;
}
)";
	COMPILE_PROGRAM(program, pa, input);
	TEST_ASSERT(pa.root->children[L"a"].Count() == 1);
	TEST_ASSERT(pa.root->children[L"a"][0]->children[L"b"].Count() == 1);
	TEST_ASSERT(pa.root->children[L"a"][0]->children[L"b"][0]->children[L"x"].Count() == 5);
	const auto& symbols = pa.root->children[L"a"][0]->children[L"b"][0]->children[L"x"];

	for (vint i = 0; i < 5; i++)
	{
		auto& symbol = symbols[i];
		if (i == 2)
		{
			TEST_ASSERT(symbol->isForwardDeclaration == false);
			TEST_ASSERT(symbol->forwardDeclarationRoot == nullptr);
			TEST_ASSERT(symbol->forwardDeclarations.Count() == 4);
			TEST_ASSERT(symbol->forwardDeclarations[0] == symbols[0].Obj());
			TEST_ASSERT(symbol->forwardDeclarations[1] == symbols[1].Obj());
			TEST_ASSERT(symbol->forwardDeclarations[2] == symbols[3].Obj());
			TEST_ASSERT(symbol->forwardDeclarations[3] == symbols[4].Obj());
		}
		else
		{
			TEST_ASSERT(symbol->isForwardDeclaration == true);
			TEST_ASSERT(symbol->forwardDeclarationRoot == symbols[2].Obj());
			TEST_ASSERT(symbol->forwardDeclarations.Count() == 0);
		}
	}
}

TEST_CASE(TestParseDecl_Functions)
{
	auto input = LR"(
int Add(int a, int b);
friend extern static virtual explicit inline __forceinline int __stdcall Sub(int, int) { return 0; }
)";
	auto output = LR"(
__forward Add: int (a: int, b: int);
explicit extern friend inline __forceinline static virtual Sub: int (int, int) __stdcall
{
	return 0;
}
)";
	AssertProgram(input, output);
}

TEST_CASE(TestParseDecl_FunctionsConnectForward)
{

}

TEST_CASE(TestParseDecl_Classes)
{
	auto input = LR"(
class C1;
struct S1;
union U1;

class C2: C1, public C1, protected C1, private C1 { int x; public: int a,b; protected: int c,d; private: int e,f; };
struct S2: S1, public S1, protected S1, private S1 { int x; public: int a,b; protected: int c,d; private: int e,f; };
union U2: U1, public U1, protected U1, private U1 {};

struct Point { int x; int y; };
)";
	auto output = LR"(
__forward class C1;
__forward struct S1;
__forward union U1;
class C2 : private C1, public C1, protected C1, private C1
{
	private x: int;
	public a: int;
	public b: int;
	protected c: int;
	protected d: int;
	private e: int;
	private f: int;
};
struct S2 : public S1, public S1, protected S1, private S1
{
	public x: int;
	public a: int;
	public b: int;
	protected c: int;
	protected d: int;
	private e: int;
	private f: int;
};
union U2 : public U1, public U1, protected U1, private U1
{
};
struct Point
{
	public x: int;
	public y: int;
};
)";
	AssertProgram(input, output);
}

TEST_CASE(TestParseDecl_ClassesConnectForward)
{
	const wchar_t* inputs[] = {
LR"(
namespace a::b
{
	class X;
	class X;
}
namespace a::b
{
	class X{};
}
namespace a::b
{
	class X;
	class X;
}
)",
LR"(
namespace a::b
{
	struct X;
	struct X;
}
namespace a::b
{
	struct X{};
}
namespace a::b
{
	struct X;
	struct X;
}
)",
LR"(
namespace a::b
{
	union X;
	union X;
}
namespace a::b
{
	union X{};
}
namespace a::b
{
	union X;
	union X;
}
)"
	};

	for (vint i = 0; i < 3; i++)
	{
		COMPILE_PROGRAM(program, pa, inputs[i]);
		TEST_ASSERT(pa.root->children[L"a"].Count() == 1);
		TEST_ASSERT(pa.root->children[L"a"][0]->children[L"b"].Count() == 1);
		TEST_ASSERT(pa.root->children[L"a"][0]->children[L"b"][0]->children[L"X"].Count() == 5);
		const auto& symbols = pa.root->children[L"a"][0]->children[L"b"][0]->children[L"X"];

		for (vint i = 0; i < 5; i++)
		{
			auto& symbol = symbols[i];
			if (i == 2)
			{
				TEST_ASSERT(symbol->isForwardDeclaration == false);
				TEST_ASSERT(symbol->forwardDeclarationRoot == nullptr);
				TEST_ASSERT(symbol->forwardDeclarations.Count() == 4);
				TEST_ASSERT(symbol->forwardDeclarations[0] == symbols[0].Obj());
				TEST_ASSERT(symbol->forwardDeclarations[1] == symbols[1].Obj());
				TEST_ASSERT(symbol->forwardDeclarations[2] == symbols[3].Obj());
				TEST_ASSERT(symbol->forwardDeclarations[3] == symbols[4].Obj());
			}
			else
			{
				TEST_ASSERT(symbol->isForwardDeclaration == true);
				TEST_ASSERT(symbol->forwardDeclarationRoot == symbols[2].Obj());
				TEST_ASSERT(symbol->forwardDeclarations.Count() == 0);
			}
		}
	}
}

TEST_CASE(TestParseDecl_Methods)
{
	auto input = LR"(
struct Vector
{
	double x = 0;
	double y = 0;

	__stdcall Vector();
	Vector(double _x, double _y);
	Vector(const Vector& v);
	Vector(Vector&& v);
	~Vector();

	operator bool()const;
	explicit operator double()const;
	Vector operator*(double z)const;
};
static Vector operator+(Vector v1, Vector v2);
static Vector operator-(Vector v1, Vector v2);
)";
	auto output = LR"(
struct Vector
{
	public x: double = 0;
	public y: double = 0;
	public __forward __ctor $__ctor: __null () __stdcall;
	public __forward __ctor $__ctor: __null (_x: double, _y: double);
	public __forward __ctor $__ctor: __null (v: Vector const &);
	public __forward __ctor $__ctor: __null (v: Vector &&);
	public __forward __dtor ~Vector: __null ();
	public __forward __type $__type: bool () const;
	public __forward explicit __type $__type: double () const;
	public __forward operator *: Vector (z: double) const;
};
__forward static operator +: Vector (v1: Vector, v2: Vector);
__forward static operator -: Vector (v1: Vector, v2: Vector);
)";
	AssertProgram(input, output);
}