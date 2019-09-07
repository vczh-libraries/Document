#include <Ast_Decl.h>
#include "Util.h"

TEST_CASE(TestParseGenericFunction)
{
	auto input = LR"(
template<typename T, typename U>
auto P(T, U)->decltype(T{}+U{});

template<typename T, int ...ts>
auto F(T t)
{
	return {t, ts...};
}
)";

	auto output = LR"(
template<typename T, typename U>
__forward P: (auto->decltype((T{} + U{}))) (T, U);
template<typename T, int ...ts>
F: auto (t: T)
{
	return {t, ts...};
}
)";

	COMPILE_PROGRAM(program, pa, input);
	AssertProgram(program, output);

	AssertExpr(pa, L"P",					L"P",					L"<::P::[T], ::P::[U]> any_t __cdecl(::P::[T], ::P::[U]) * $PR"						);
	AssertExpr(pa, L"P<int, double>",		L"P<int, double>",		L"double __cdecl(__int32, double) * $PR"											);

	AssertExpr(pa, L"F",					L"F",					L"<::F::[T], ...*> any_t __cdecl(::F::[T]) * $PR"									);
	AssertExpr(pa, L"F<void*,1,2,3>",		L"F<void *, 1, 2, 3>",	L"{void * $L, __int32 $PR, __int32 $PR, __int32 $PR} __cdecl(void *) * $PR"			);
}

TEST_CASE(TestParseGenericFunction_ResolveTypeWithLocalVariables)
{
	auto input = LR"(
template<typename T>
auto F()
{
	T t;
	return t;
}
)";

	COMPILE_PROGRAM(program, pa, input);

	AssertExpr(pa, L"F",					L"F",					L"<::F::[T]> ::F::[T] __cdecl() * $PR"						);
	AssertExpr(pa, L"F<int>",				L"F<int>",				L"__int32 __cdecl() * $PR"									);
}

TEST_CASE(TestParseGenericFunction_ResolveTypeWithGenericAliases)
{
	auto input = LR"(
char G(char);
int G(int);
void* G(...);

template<typename T>
auto F(T t)
{
	template<typename U>
	using H = decltype(G((U){}));

	H<T> h;
	return {t, h};
}
)";

	COMPILE_PROGRAM(program, pa, input);

	AssertExpr(pa, L"F",					L"F",					L"<::F::[T]> {::F::[T] $L, char $L} __cdecl(::F::[T]) * $PR",
																	L"<::F::[T]> {::F::[T] $L, __int32 $L} __cdecl(::F::[T]) * $PR",
																	L"<::F::[T]> {::F::[T] $L, void * $L} __cdecl(::F::[T]) * $PR"		);

	AssertExpr(pa, L"F<char>",				L"F<char>",				L"{char $L, char $L} __cdecl(char) * $PR"							);
	AssertExpr(pa, L"F<int>",				L"F<int>",				L"{__int32 $L, __int32 $L} __cdecl(__int32) * $PR"					);
	AssertExpr(pa, L"F<char*>",				L"F<char *>",			L"{char * $L, void * $L} __cdecl(char *) * $PR"						);
}

TEST_CASE(TestParseGenericFunction_ConnectForward)
{
	auto input = LR"(
void F();
void F(){}

int F(...);
int F(...){}

template<typename T> T F(T);
template<typename T> T F(T t){}

template<typename T, typename U> T F(U*);
template<typename T, typename U> T F(U* pu){}

class C
{
	struct S{};

	void F();
	int F(...);
	template<typename T> T F(S, T);
	template<typename T, typename U> T F(S, U*);
};

void C::F(){}
int C::F(...){}
template<typename T> T C::F(S, T t){}
template<typename T, typename U> T C::F(S, U* pu){}
)";

	COMPILE_PROGRAM(program, pa, input);

	List<Ptr<Declaration>> fs;
	CopyFrom(
		fs,
		From(program->decls)
			.Where([](Ptr<Declaration> decl) {return decl->name.name == L"F"; })
	);
	CopyFrom(
		fs,
		From(pa.root->TryGetChildren_NFb(L"C")->Get(0)->GetImplDecl_NFb<ClassDeclaration>()->decls)
			.Select([](Tuple<CppClassAccessor, Ptr<Declaration>> t) {return t.f1; }),
		true
	);
	TEST_ASSERT(fs.Count() == 16);

	for (vint i = 0; i < 8; i++)
	{
		vint forward = i < 4 ? i * 2 : i + 12;
		vint impl = i < 4 ? i * 2 + 1 : i + 8;

		auto forwardDecl = fs[forward];
		auto implDecl = fs[impl];

		TEST_ASSERT(forwardDecl->symbol->GetForwardDecl_Fb() == forwardDecl);
		TEST_ASSERT(implDecl->symbol->GetImplDecl_NFb() == implDecl);
		TEST_ASSERT(forwardDecl->symbol->GetFunctionSymbol_Fb() == implDecl->symbol->GetFunctionSymbol_Fb());
	}
}

TEST_CASE(TestParseGenericFunction_ConnectForward_Overloading)
{
	auto input = LR"(
)";

	COMPILE_PROGRAM(program, pa, input);
}

TEST_CASE(TestParseGenericFunction_ConnectForward_Ambiguity)
{
	auto input = LR"(
)";

	COMPILE_PROGRAM(program, pa, input);
}

TEST_CASE(TestParseGenericFunction_CallFullInstantiated)
{
	auto input = LR"(
template<typename T = int, typename U = double>
auto P(T, U)->decltype(T{}+U{});

template<typename T, int ...ts>
auto F(T t)
{
	return {t, ts...};
}
)";

	COMPILE_PROGRAM(program, pa, input);
	
	AssertExpr(pa, L"P<>(1,2)",					L"P<>(1, 2)",						L"double $PR"													);
	AssertExpr(pa, L"P<int>(1,2)",				L"P<int>(1, 2)",					L"double $PR"													);
	AssertExpr(pa, L"P<int, double>(1,2)",		L"P<int, double>(1, 2)",			L"double $PR"													);
	AssertExpr(pa, L"F<void*,1,2,3>(nullptr)",	L"F<void *, 1, 2, 3>(nullptr)",		L"{void * $L, __int32 $PR, __int32 $PR, __int32 $PR} $PR"		);
}

TEST_CASE(TestParseGenericFunction_CallFullInstantiated_Overloading)
{
	auto input = LR"(
)";

	COMPILE_PROGRAM(program, pa, input);
}