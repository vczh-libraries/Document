#include <Ast_Decl.h>
#include "Util.h"

TEST_CASE(TestParseDecl_Namespaces)
{
	auto input = LR"(
namespace vl {}
namespace vl::presentation::controls {}
namespace {}
namespace vl
{
	int x;
	namespace
	{
		int y;
	}
}
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
namespace vl
{
	x: int;
	y: int;
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
	TEST_ASSERT(pa.root->TryGetChildren(L"a")->Count() == 1);
	TEST_ASSERT(pa.root->TryGetChildren(L"a")->Get(0)->TryGetChildren(L"b")->Count() == 1);
	TEST_ASSERT(pa.root->TryGetChildren(L"a")->Get(0)->TryGetChildren(L"b")->Get(0)->TryGetChildren(L"A")->Count() == 1);
	auto symbol = pa.root->TryGetChildren(L"a")->Get(0)->TryGetChildren(L"b")->Get(0)->TryGetChildren(L"A")->Get(0).Obj();

	TEST_ASSERT(symbol->kind == symbol_component::SymbolKind::Enum);
	TEST_ASSERT(symbol->GetImplDecl<EnumDeclaration>());
	TEST_ASSERT(symbol->GetForwardDecls().Count() == 4);
	TEST_ASSERT(From(symbol->GetForwardDecls()).Distinct().Count() == 4);
	for (vint i = 0; i < 4; i++)
	{
		TEST_ASSERT(symbol->GetForwardDecls()[i].Cast<ForwardEnumDeclaration>());
		TEST_ASSERT(!symbol->GetForwardDecls()[i].Cast<EnumDeclaration>());
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
	TEST_ASSERT(pa.root->TryGetChildren(L"a")->Count() == 1);
	TEST_ASSERT(pa.root->TryGetChildren(L"a")->Get(0)->TryGetChildren(L"b")->Count() == 1);
	TEST_ASSERT(pa.root->TryGetChildren(L"a")->Get(0)->TryGetChildren(L"b")->Get(0)->TryGetChildren(L"x")->Count() == 1);
	auto symbol = pa.root->TryGetChildren(L"a")->Get(0)->TryGetChildren(L"b")->Get(0)->TryGetChildren(L"x")->Get(0).Obj();

	TEST_ASSERT(symbol->kind == symbol_component::SymbolKind::Variable);
	TEST_ASSERT(symbol->GetImplDecl<VariableDeclaration>());
	TEST_ASSERT(symbol->GetForwardDecls().Count() == 4);
	TEST_ASSERT(From(symbol->GetForwardDecls()).Distinct().Count() == 4);
	for (vint i = 0; i < 4; i++)
	{
		TEST_ASSERT(symbol->GetForwardDecls()[i].Cast<ForwardVariableDeclaration>());
		TEST_ASSERT(!symbol->GetForwardDecls()[i].Cast<VariableDeclaration>());
	}
}

TEST_CASE(TestParseDecl_Functions)
{
	auto input = LR"(
int Add(int a, int b);
int Sub(int a, int b) = 0;
friend extern static virtual explicit inline __inline __forceinline int __stdcall Mul(int, int) { return 0; }
friend extern static virtual explicit inline __inline __forceinline int __stdcall Div(int, int) = 0 { return 0; }
)";
	auto output = LR"(
__forward Add: int (a: int, b: int);
__forward Sub: int (a: int, b: int) = 0;
explicit extern friend inline __inline __forceinline static virtual Mul: int (int, int) __stdcall
{
	return 0;
}
explicit extern friend inline __inline __forceinline static virtual Div: int (int, int) __stdcall = 0
{
	return 0;
}
)";
	AssertProgram(input, output);
}

TEST_CASE(TestParseDecl_ExternC)
{
	auto input = LR"(
extern "C" int x = 0;
extern "C" { int y, z = 0; int w; }
extern "C" int Add(int a, int b);
extern "C"
{
	int Mul(int a, int b);
	int Div(int a, int b);
}
)";
	auto output = LR"(
x: int = 0;
y: int;
z: int = 0;
w: int;
__forward Add: int (a: int, b: int);
__forward Mul: int (a: int, b: int);
__forward Div: int (a: int, b: int);
)";
	AssertProgram(input, output);
}

TEST_CASE(TestParseDecl_FunctionsConnectForward)
{
	auto input = LR"(
namespace a::b
{
	extern int Add(int x = 0, int y = 0);
	int Add(int a, int b);
}
namespace a::b
{
	int Add(int, int) { return 0; }
}
namespace a::b
{
	extern int Add(int, int);
	int Add(int = 0, int = 0);
}
)";
	COMPILE_PROGRAM(program, pa, input);
	TEST_ASSERT(pa.root->TryGetChildren(L"a")->Count() == 1);
	TEST_ASSERT(pa.root->TryGetChildren(L"a")->Get(0)->TryGetChildren(L"b")->Count() == 1);
	TEST_ASSERT(pa.root->TryGetChildren(L"a")->Get(0)->TryGetChildren(L"b")->Get(0)->TryGetChildren(L"Add")->Count() == 1);
	auto symbol = pa.root->TryGetChildren(L"a")->Get(0)->TryGetChildren(L"b")->Get(0)->TryGetChildren(L"Add")->Get(0).Obj();

	TEST_ASSERT(symbol->kind == symbol_component::SymbolKind::Function);
	TEST_ASSERT(symbol->GetImplDecl<FunctionDeclaration>());
	TEST_ASSERT(symbol->GetForwardDecls().Count() == 4);
	TEST_ASSERT(From(symbol->GetForwardDecls()).Distinct().Count() == 4);
	for (vint i = 0; i < 4; i++)
	{
		TEST_ASSERT(symbol->GetForwardDecls()[i].Cast<ForwardFunctionDeclaration>());
		TEST_ASSERT(!symbol->GetForwardDecls()[i].Cast<FunctionDeclaration>());
	}
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
		TEST_ASSERT(pa.root->TryGetChildren(L"a")->Count() == 1);
		TEST_ASSERT(pa.root->TryGetChildren(L"a")->Get(0)->TryGetChildren(L"b")->Count() == 1);
		TEST_ASSERT(pa.root->TryGetChildren(L"a")->Get(0)->TryGetChildren(L"b")->Get(0)->TryGetChildren(L"X")->Count() == 1);
		auto symbol = pa.root->TryGetChildren(L"a")->Get(0)->TryGetChildren(L"b")->Get(0)->TryGetChildren(L"X")->Get(0).Obj();

		switch (i)
		{
		case 0: TEST_ASSERT(symbol->kind == symbol_component::SymbolKind::Class); break;
		case 1: TEST_ASSERT(symbol->kind == symbol_component::SymbolKind::Struct); break;
		case 2: TEST_ASSERT(symbol->kind == symbol_component::SymbolKind::Union); break;
		}
		TEST_ASSERT(symbol->GetImplDecl<ClassDeclaration>());
		TEST_ASSERT(symbol->GetForwardDecls().Count() == 4);
		TEST_ASSERT(From(symbol->GetForwardDecls()).Distinct().Count() == 4);
		for (vint i = 0; i < 4; i++)
		{
			TEST_ASSERT(symbol->GetForwardDecls()[i].Cast<ForwardClassDeclaration>());
			TEST_ASSERT(!symbol->GetForwardDecls()[i].Cast<ClassDeclaration>());
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
	Vector(const Vector& v)=default;
	Vector(Vector&& v)=delete;
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
	public __forward __ctor $__ctor: __null (v: Vector const &) = default;
	public __forward __ctor $__ctor: __null (v: Vector &&) = delete;
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

TEST_CASE(TestParseDecl_ClassMemberConnectForward)
{
	auto input = LR"(
namespace a::b
{
	class Something
	{
	public:
		static const int a = 0;
		static const int b;

		Something();
		Something(const Something&);
		Something(Something&&);
		explicit Something(int);
		~Something();

		void Do();
		virtual void Do(int) = 0;
		explicit operator bool()const;
		explicit operator bool();
		Something operator++();
		Something operator++(int);
	};
}
namespace a::b
{
	const int Something::b = 0;
	Something::Something(){}
	Something::Something(const Something&){}
	Something::Something(Something&&){}
	Something::Something(int){}
	Something::~Something(){}
	void Something::Do(){}
	void Something::Do(int) {}
	Something::operator bool()const{}
	Something::operator bool(){}
	Something Something::operator++(){}
	Something Something::operator++(int){}
}
)";

	auto output = LR"(
namespace a
{
	namespace b
	{
		class Something
		{
			public static a: int const = 0;
			public __forward static b: int const;
			public __forward __ctor $__ctor: __null ();
			public __forward __ctor $__ctor: __null (Something const &);
			public __forward __ctor $__ctor: __null (Something &&);
			public __forward explicit __ctor $__ctor: __null (int);
			public __forward __dtor ~Something: __null ();
			public __forward Do: void ();
			public __forward virtual Do: void (int) = 0;
			public __forward explicit __type $__type: bool () const;
			public __forward explicit __type $__type: bool ();
			public __forward operator ++: Something ();
			public __forward operator ++: Something (int);
		};
	}
}
namespace a
{
	namespace b
	{
		b: int const (Something ::) = 0;
		__ctor $__ctor: __null () (Something ::)
		{
		}
		__ctor $__ctor: __null (Something const &) (Something ::)
		{
		}
		__ctor $__ctor: __null (Something &&) (Something ::)
		{
		}
		__ctor $__ctor: __null (int) (Something ::)
		{
		}
		__dtor ~Something: __null () (Something ::)
		{
		}
		Do: void () (Something ::)
		{
		}
		Do: void (int) (Something ::)
		{
		}
		__type $__type: bool () const (Something ::)
		{
		}
		__type $__type: bool () (Something ::)
		{
		}
		operator ++: Something () (Something ::)
		{
		}
		operator ++: Something (int) (Something ::)
		{
		}
	}
}
)";

	COMPILE_PROGRAM(program, pa, input);
	AssertProgram(program, output);

	using Item = Tuple<CppClassAccessor, Ptr<Declaration>>;
	List<Ptr<Declaration>> inClassMembers;
	auto& inClassMembersUnfiltered = pa.root->TryGetChildren(L"a")->Get(0)->TryGetChildren(L"b")->Get(0)->TryGetChildren(L"Something")->Get(0)->GetImplDecl<ClassDeclaration>()->decls;
	CopyFrom(inClassMembers, From(inClassMembersUnfiltered).Where([](Item item) {return !item.f1->implicitlyGeneratedMember; }).Select([](Item item) { return item.f1; }));
	TEST_ASSERT(inClassMembers.Count() == 13);

	auto& outClassMembers = pa.root->TryGetChildren(L"a")->Get(0)->TryGetChildren(L"b")->Get(0)->GetForwardDecls()[1].Cast<NamespaceDeclaration>()->decls;
	TEST_ASSERT(outClassMembers.Count() == 12);

	for (vint i = 0; i < 12; i++)
	{
		auto inClassDecl = inClassMembers[i + 1];
		auto outClassDecl = outClassMembers[i];
		TEST_ASSERT(inClassDecl->symbol == outClassDecl->symbol);

		auto symbol = inClassDecl->symbol;
		if (i == 0)
		{
			TEST_ASSERT(symbol->kind == symbol_component::SymbolKind::Variable);
		}
		else
		{
			TEST_ASSERT(symbol->kind == symbol_component::SymbolKind::Function);
		}
		TEST_ASSERT(symbol->GetImplDecl() == outClassDecl);
		TEST_ASSERT(symbol->GetForwardDecls().Count() == 1);
		TEST_ASSERT(symbol->GetForwardDecls()[0] == inClassDecl);
	}
}

TEST_CASE(TestParserDecl_CtorInitList)
{
	auto input = LR"(
struct Vector
{
	double x;
	double y;

	Vector() : x(0), y(0) {}
	Vector(int _x, int _y) : x(_x), y(_y) {}
};
static Vector operator+(Vector v1, Vector v2);
static Vector operator-(Vector v1, Vector v2);
)";
	auto output = LR"(
struct Vector
{
	public x: double;
	public y: double;
	public __ctor $__ctor: __null ()
		: x(0)
		, y(0)
	{
	}
	public __ctor $__ctor: __null (_x: int, _y: int)
		: x(_x)
		, y(_y)
	{
	}
};
__forward static operator +: Vector (v1: Vector, v2: Vector);
__forward static operator -: Vector (v1: Vector, v2: Vector);
)";
	AssertProgram(input, output);
}

TEST_CASE(TestParseDecl_ClassMemberScope)
{
	auto input = LR"(
namespace a
{
	struct X
	{
		enum class Y;
	};
	struct Z;
}
namespace b
{
	struct Z : a::X
	{
		Y Do(a::X, X, a::X::Y, X::Y, Y, Z);
	};
}
namespace b
{
	struct X;
	Z::Y Z::Do(a::X, X, a::X::Y, X::Y, Y, Z)
	{
		X x;
		Y y;
		Z z;
	}
}
)";
	auto output = LR"(
namespace a
{
	struct X
	{
		public __forward enum class Y;
	};
	__forward struct Z;
}
namespace b
{
	struct Z : public a :: X
	{
		public __forward Do: Y (a :: X, X, a :: X :: Y, X :: Y, Y, Z);
	};
}
namespace b
{
	__forward struct X;
	Do: Z :: Y (a :: X, X, a :: X :: Y, X :: Y, Y, Z) (Z ::)
	{
		x: X;
		y: Y;
		z: Z;
	}
}
)";

	SortedList<vint> accessed;
	auto recorder = BEGIN_ASSERT_SYMBOL
		ASSERT_SYMBOL(0, L"a", 11, 12, NamespaceDeclaration, 1, 10)
		ASSERT_SYMBOL(1, L"X", 11, 15, ClassDeclaration, 3, 8)
		ASSERT_SYMBOL(2, L"Y", 13, 2, ForwardEnumDeclaration, 5, 13)
		ASSERT_SYMBOL(3, L"a", 13, 7, NamespaceDeclaration, 1, 10)
		ASSERT_SYMBOL(4, L"X", 13, 10, ClassDeclaration, 3, 8)
		ASSERT_SYMBOL(5, L"X", 13, 13, ClassDeclaration, 3, 8)
		ASSERT_SYMBOL(6, L"a", 13, 16, NamespaceDeclaration, 1, 10)
		ASSERT_SYMBOL(7, L"X", 13, 19, ClassDeclaration, 3, 8)
		ASSERT_SYMBOL(8, L"Y", 13, 22, ForwardEnumDeclaration, 5, 13)
		ASSERT_SYMBOL(9, L"X", 13, 25, ClassDeclaration, 3, 8)
		ASSERT_SYMBOL(10, L"Y", 13, 28, ForwardEnumDeclaration, 5, 13)
		ASSERT_SYMBOL(11, L"Y", 13, 31, ForwardEnumDeclaration, 5, 13)
		ASSERT_SYMBOL(12, L"Z", 13, 34, ClassDeclaration, 11, 8)
		ASSERT_SYMBOL(13, L"Z", 19, 1, ClassDeclaration, 11, 8)
		ASSERT_SYMBOL(14, L"Y", 19, 4, ForwardEnumDeclaration, 5, 13)
		ASSERT_SYMBOL(15, L"Z", 19, 6, ClassDeclaration, 11, 8)
		ASSERT_SYMBOL(16, L"a", 19, 12, NamespaceDeclaration, 1, 10)
		ASSERT_SYMBOL(17, L"X", 19, 15, ClassDeclaration, 3, 8)
		ASSERT_SYMBOL(18, L"X", 19, 18, ClassDeclaration, 3, 8)
		ASSERT_SYMBOL(19, L"a", 19, 21, NamespaceDeclaration, 1, 10)
		ASSERT_SYMBOL(20, L"X", 19, 24, ClassDeclaration, 3, 8)
		ASSERT_SYMBOL(21, L"Y", 19, 27, ForwardEnumDeclaration, 5, 13)
		ASSERT_SYMBOL(22, L"X", 19, 30, ClassDeclaration, 3, 8)
		ASSERT_SYMBOL(23, L"Y", 19, 33, ForwardEnumDeclaration, 5, 13)
		ASSERT_SYMBOL(24, L"Y", 19, 36, ForwardEnumDeclaration, 5, 13)
		ASSERT_SYMBOL(25, L"Z", 19, 39, ClassDeclaration, 11, 8)
		ASSERT_SYMBOL(26, L"X", 21, 2, ClassDeclaration, 3, 8)
		ASSERT_SYMBOL(27, L"Y", 22, 2, ForwardEnumDeclaration, 5, 13)
		ASSERT_SYMBOL(28, L"Z", 23, 2, ClassDeclaration, 11, 8)
	END_ASSERT_SYMBOL;

	AssertProgram(input, output, recorder);
	TEST_ASSERT(accessed.Count() == 29);
}

TEST_CASE(TestParseDecl_Using_Namespace)
{
	auto input = LR"(
namespace a::b
{
	struct X {};
	enum class Y {};
}
namespace c
{
	using namespace a;
	using namespace a::b;
}
namespace c
{
	struct Z : X
	{
		a::b::Y y1;
		b::Y y2;
		Y y3;
	};
}
)";
	auto output = LR"(
namespace a
{
	namespace b
	{
		struct X
		{
		};
		enum class Y
		{
		};
	}
}
namespace c
{
	using namespace a;
	using namespace a :: b;
}
namespace c
{
	struct Z : public X
	{
		public y1: a :: b :: Y;
		public y2: b :: Y;
		public y3: Y;
	};
}
)";

	SortedList<vint> accessed;
	auto recorder = BEGIN_ASSERT_SYMBOL
		ASSERT_SYMBOL(0, L"a", 8, 17, NamespaceDeclaration, 1, 10)
		ASSERT_SYMBOL(1, L"a", 9, 17, NamespaceDeclaration, 1, 10)
		ASSERT_SYMBOL(2, L"b", 9, 20, NamespaceDeclaration, 1, 13)
		ASSERT_SYMBOL(3, L"X", 13, 12, ClassDeclaration, 3, 8)
		ASSERT_SYMBOL(4, L"a", 15, 2, NamespaceDeclaration, 1, 10)
		ASSERT_SYMBOL(5, L"b", 15, 5, NamespaceDeclaration, 1, 13)
		ASSERT_SYMBOL(6, L"Y", 15, 8, EnumDeclaration, 4, 12)
		ASSERT_SYMBOL(7, L"b", 16, 2, NamespaceDeclaration, 1, 13)
		ASSERT_SYMBOL(8, L"Y", 16, 5, EnumDeclaration, 4, 12)
		ASSERT_SYMBOL(9, L"Y", 17, 2, EnumDeclaration, 4, 12)
	END_ASSERT_SYMBOL;

	AssertProgram(input, output, recorder);
	TEST_ASSERT(accessed.Count() == 10);

	COMPILE_PROGRAM(program, pa, input);

	AssertExpr(pa, L"a::b::X()",				L"a :: b :: X()",				L"::a::b::X $PR");
	AssertExpr(pa, L"c::X()",					L"c :: X()",					L"::a::b::X $PR");
	AssertExpr(pa, L"c::b::X()",				L"c :: b :: X()",				L"::a::b::X $PR");

	AssertExpr(pa, L"a::b::Y()",				L"a :: b :: Y()",				L"::a::b::Y $PR");
	AssertExpr(pa, L"c::Y()",					L"c :: Y()",					L"::a::b::Y $PR");
	AssertExpr(pa, L"c::b::Y()",				L"c :: b :: Y()",				L"::a::b::Y $PR");
}

TEST_CASE(TestParseDecl_Using_Type)
{
	auto input = LR"(
namespace a::b
{
	struct X {};
	enum class Y {};
}
namespace c
{
	using a::b::X;
	using a::b::Y;
}
namespace c
{
	struct Z : X
	{
		friend X;
		a::b::Y y1;
		Y y3;
	};
}
)";
	auto output = LR"(
namespace a
{
	namespace b
	{
		struct X
		{
		};
		enum class Y
		{
		};
	}
}
namespace c
{
	using a :: b :: X;
	using a :: b :: Y;
}
namespace c
{
	struct Z : public X
	{
		public y1: a :: b :: Y;
		public y3: Y;
	};
}
)";

	SortedList<vint> accessed;
	auto recorder = BEGIN_ASSERT_SYMBOL
		ASSERT_SYMBOL(0, L"a", 8, 7, NamespaceDeclaration, 1, 10)
		ASSERT_SYMBOL(1, L"b", 8, 10, NamespaceDeclaration, 1, 13)
		ASSERT_SYMBOL(2, L"X", 8, 13, ClassDeclaration, 3, 8)
		ASSERT_SYMBOL(3, L"a", 9, 7, NamespaceDeclaration, 1, 10)
		ASSERT_SYMBOL(4, L"b", 9, 10, NamespaceDeclaration, 1, 13)
		ASSERT_SYMBOL(5, L"Y", 9, 13, EnumDeclaration, 4, 12)
		ASSERT_SYMBOL(6, L"X", 13, 12, ClassDeclaration, 3, 8)
		ASSERT_SYMBOL(7, L"a", 16, 2, NamespaceDeclaration, 1, 10)
		ASSERT_SYMBOL(8, L"b", 16, 5, NamespaceDeclaration, 1, 13)
		ASSERT_SYMBOL(9, L"Y", 16, 8, EnumDeclaration, 4, 12)
		ASSERT_SYMBOL(10, L"Y", 17, 2, EnumDeclaration, 4, 12)
	END_ASSERT_SYMBOL;

	AssertProgram(input, output, recorder);
	TEST_ASSERT(accessed.Count() == 11);

	COMPILE_PROGRAM(program, pa, input);
	
	AssertExpr(pa, L"a::b::X()",				L"a :: b :: X()",				L"::a::b::X $PR");
	AssertExpr(pa, L"c::X()",					L"c :: X()",					L"::a::b::X $PR");
	
	AssertExpr(pa, L"a::b::Y()",				L"a :: b :: Y()",				L"::a::b::Y $PR");
	AssertExpr(pa, L"c::Y()",					L"c :: Y()",					L"::a::b::Y $PR");
}

TEST_CASE(TestParseDecl_Using_Value_InNamespace)
{
	auto input = LR"(
namespace a
{
	int x;
	void y();
}
using a::x;
using a::y;
)";
	COMPILE_PROGRAM(program, pa, input);

	AssertExpr(pa, L"x",			L"x",			L"__int32 $L"			);
	AssertExpr(pa, L"y",			L"y",			L"void __cdecl() * $PR"	);
}

TEST_CASE(TestParseDecl_Using_Value_InClass)
{
	{
		auto input = LR"(
struct A
{
	char f(char);
};

struct B : A
{
	int f(int);
};

struct C : A
{
	using A::f;
	int f(int);
};
)";
		COMPILE_PROGRAM(program, pa, input);

		AssertExpr(pa, L"B().f('a')",			L"B().f('a')",			L"__int32 $PR"	);
		AssertExpr(pa, L"C().f('a')",			L"C().f('a')",			L"char $PR"		);
	}
	{
		auto input = LR"(
struct A
{
	static char f(char);
};

struct B : A
{
	static int f(int);
};

struct C : A
{
	using A::f;
	static int f(int);
};
)";
		COMPILE_PROGRAM(program, pa, input);

		AssertExpr(pa, L"B::f('a')",			L"B :: f('a')",			L"__int32 $PR"	);
		AssertExpr(pa, L"C::f('a')",			L"C :: f('a')",			L"char $PR"		);
	}
}

TEST_CASE(TestParseDecl_TypeAlias)
{
	{
		auto input = LR"(
using A = int;
using B = A(*)(A);

typedef struct S_
{
	struct T {};
} S, *pS;
using C = S;
using D = C::T;
typedef int a, b, c;
typedef C::T(*d)(A, B, pS);
typedef struct S_ S__;

a _a;
b _b;
c _c;
d _d;
)";
		auto output = LR"(
using_type A: int;
using_type B: A (A) *;
struct S_
{
	public struct T
	{
	};
};
using_type S: S_;
using_type pS: S_ *;
using_type C: S;
using_type D: C :: T;
using_type a: int;
using_type b: int;
using_type c: int;
using_type d: C :: T (A, B, pS) *;
using_type S__: S_;
_a: a;
_b: b;
_c: c;
_d: d;
)";
		COMPILE_PROGRAM(program, pa, input);
		AssertProgram(program, output);

		AssertExpr(pa, L"_a",				L"_a",					L"__int32 $L"														);
		AssertExpr(pa, L"_b",				L"_b",					L"__int32 $L"														);
		AssertExpr(pa, L"_c",				L"_c",					L"__int32 $L"														);
		AssertExpr(pa, L"_d",				L"_d",					L"::S_::T __cdecl(__int32, __int32 __cdecl(__int32) *, ::S_ *) * $L");
	}
}

TEST_CASE(TestParseDecl_NestedAnonymousClass)
{
	auto input = LR"(
struct Color
{
	union
	{
		struct
		{
			unsigned char r, g, b, a;
		};
		unsigned value;
	};
};
)";
	auto output = LR"(
struct Color
{
	public union
	{
		public struct
		{
			public r: unsigned char;
			public g: unsigned char;
			public b: unsigned char;
			public a: unsigned char;
		};
		public value: unsigned int;
	};
};
)";
	COMPILE_PROGRAM(program, pa, input);
	AssertProgram(program, output);

	AssertExpr(pa, L"Color().r",				L"Color().r",				L"unsigned __int8 $PR"	);
	AssertExpr(pa, L"Color().g",				L"Color().g",				L"unsigned __int8 $PR"	);
	AssertExpr(pa, L"Color().b",				L"Color().b",				L"unsigned __int8 $PR"	);
	AssertExpr(pa, L"Color().a",				L"Color().a",				L"unsigned __int8 $PR"	);
	AssertExpr(pa, L"Color().value",			L"Color().value",			L"unsigned __int32 $PR"	);
}

TEST_CASE(TestParseDecl_ClassFollowedVariables)
{
	auto input = LR"(
struct X
{
	int x;
} x, *px;
)";
	auto output = LR"(
struct X
{
	public x: int;
};
x: X;
px: X *;
)";
	COMPILE_PROGRAM(program, pa, input);
	AssertProgram(program, output);

	AssertExpr(pa, L"x",						L"x",						L"::X $L"		);
	AssertExpr(pa, L"x.x",						L"x.x",						L"__int32 $L"	);
	AssertExpr(pa, L"px",						L"px",						L"::X * $L"		);
	AssertExpr(pa, L"px->x",					L"px->x",					L"__int32 $L"	);
}

TEST_CASE(TestParseDecl_AnonymousClassFollowedVariables)
{
	auto input = LR"(
struct
{
	int x:1;
} x, *px;
)";
	auto output = LR"(
struct <anonymous>0
{
	public x: int;
};
x: <anonymous>0;
px: <anonymous>0 *;
)";
	COMPILE_PROGRAM(program, pa, input);
	AssertProgram(program, output);

	AssertExpr(pa, L"x",						L"x",						L"::<anonymous>0 $L"	);
	AssertExpr(pa, L"x.x",						L"x.x",						L"__int32 $L"			);
	AssertExpr(pa, L"px",						L"px",						L"::<anonymous>0 * $L"	);
	AssertExpr(pa, L"px->x",					L"px->x",					L"__int32 $L"			);
}

TEST_CASE(TestParseDecl_NestedAnonymousEnum)
{
	auto input = LR"(
enum
{
	X
};
enum : int
{
	Y
};
)";
	auto output = LR"(
enum <anonymous>0
{
	X,
};
enum <anonymous>1 : int
{
	Y,
};
)";
	COMPILE_PROGRAM(program, pa, input);
	AssertProgram(program, output);

	AssertExpr(pa, L"X",						L"X",						L"::<anonymous>0 $PR");
	AssertExpr(pa, L"Y",						L"Y",						L"::<anonymous>1 $PR");
}

TEST_CASE(TestParseDecl_EnumFollowedVariables)
{
	auto input = LR"(
enum class X
{
} x, *px;
)";
	auto output = LR"(
enum class X
{
};
x: X;
px: X *;
)";
	COMPILE_PROGRAM(program, pa, input);
	AssertProgram(program, output);

	AssertExpr(pa, L"x",						L"x",						L"::X $L"	);
	AssertExpr(pa, L"px",						L"px",						L"::X * $L"	);
}

TEST_CASE(TestParseDecl_AnonymousEnumFollowedVariables)
{
	auto input = LR"(
enum
{
} x, *px;
)";
	auto output = LR"(
enum <anonymous>0
{
};
x: <anonymous>0;
px: <anonymous>0 *;
)";
	COMPILE_PROGRAM(program, pa, input);
	AssertProgram(program, output);

	AssertExpr(pa, L"x",						L"x",						L"::<anonymous>0 $L"	);
	AssertExpr(pa, L"px",						L"px",						L"::<anonymous>0 * $L"	);
}

TEST_CASE(TestParseDecl_TypedefWithAnonymousClass)
{
	auto input = LR"(
typedef struct
{
	int x;
} X, *pX;

typedef struct
{
	X x;
	pX px;
} Y;
)";
	auto output = LR"(
struct <anonymous>0
{
	public x: int;
};
using_type X: <anonymous>0;
using_type pX: <anonymous>0 *;
struct <anonymous>1
{
	public x: X;
	public px: pX;
};
using_type Y: <anonymous>1;
)";
	COMPILE_PROGRAM(program, pa, input);
	AssertProgram(program, output);

	AssertExpr(pa, L"Y().x.x",					L"Y().x.x",					L"__int32 $PR"	);
	AssertExpr(pa, L"Y().px->x",				L"Y().px->x",				L"__int32 $L"	);
}

TEST_CASE(TestParseDecl_TypedefWithAnonymousEnum)
{
	auto input = LR"(
typedef enum
{
	A
} _A, *pA;

typedef enum : int
{
	B
} _B, *pB;

typedef enum C_
{
	C
} _C, *pC;

typedef enum class D_
{
	D
} _D, *pD;
)";
	auto output = LR"(
enum <anonymous>0
{
	A,
};
using_type _A: <anonymous>0;
using_type pA: <anonymous>0 *;
enum <anonymous>1 : int
{
	B,
};
using_type _B: <anonymous>1;
using_type pB: <anonymous>1 *;
enum C_
{
	C,
};
using_type _C: C_;
using_type pC: C_ *;
enum class D_
{
	D,
};
using_type _D: D_;
using_type pD: D_ *;
)";
	COMPILE_PROGRAM(program, pa, input);
	AssertProgram(program, output);

	AssertExpr(pa, L"A",						L"A",						L"::<anonymous>0 $PR"	);
	AssertExpr(pa, L"B",						L"B",						L"::<anonymous>1 $PR"	);
	AssertExpr(pa, L"C",						L"C",						L"::C_ $PR"				);
	AssertExpr(pa, L"_D::D",					L"_D :: D",					L"::D_ $PR"				);
}