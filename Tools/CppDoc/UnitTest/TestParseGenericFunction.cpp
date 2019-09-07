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
/*  0 */void F();
/*  1 */void F(){}

/*  2 */int F(...);
/*  3 */int F(...){}

/*  4 */template<typename T> T F(T);
/*  5 */template<typename T> T F(T t){}

/*  6 */template<typename T, typename U> T F(U*);
/*  7 */template<typename T, typename U> T F(U* pu){}

		class C
		{
			struct S{};
		
/* 12 */ 	void F(S);
/* 13 */ 	int F(S, ...);
/* 14 */ 	template<typename T> T F(S, T);
/* 15 */ 	template<typename T, typename U> T F(S, U*);
		};
 
/*  8 */ void C::F(S){}
/*  9 */ int C::F(S, ...){}
/* 10 */ template<typename T> T C::F(S, T t){}
/* 11 */ template<typename T, typename U> T C::F(S, U* pu){}
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
			.Select([](Tuple<CppClassAccessor, Ptr<Declaration>> t) {return t.f1; })
			.Where([](Ptr<Declaration> decl) {return decl->name.name == L"F"; }),
		true
	);
	TEST_ASSERT(fs.Count() == 16);

	for (vint i = 0; i < 8; i++)
	{
		vint forward = i < 4 ? i * 2 : i + 8;
		vint impl = i < 4 ? i * 2 + 1 : i + 4;

		auto forwardDecl = fs[forward];
		auto implDecl = fs[impl];

		TEST_ASSERT(forwardDecl->symbol->GetForwardDecl_Fb() == forwardDecl);
		TEST_ASSERT(implDecl->symbol->GetImplDecl_NFb() == implDecl);
		TEST_ASSERT(forwardDecl->symbol->GetFunctionSymbol_Fb() == implDecl->symbol->GetFunctionSymbol_Fb());
	}
}

TEST_CASE(TestParseGenericFunction_ConnectForward_Ambiguity)
{
	auto input = LR"(
/*  0 */void F(int(&)[1]);
/*  1 */void F(int(&)[1]){}

/*  2 */int F(int(&)[2]);
/*  3 */int F(int(&)[2]){}

/*  4 */template<typename T> T F(T(&)[1]);
/*  5 */template<typename T> T F(T(&)[1]){}

/*  6 */template<typename T> T F(T(&)[2]);
/*  7 */template<typename T> T F(T(&)[2]){}

/*  8 */template<typename T, int U> T F(T(&)[U]);
/*  9 */template<typename T, int U> T F(T(&)[U]){}

		class C
		{
			struct S{};

/* 15 */	void F(S, int(&)[1]);
/* 16 */	int F(S, int(&)[2]);
/* 17 */	template<typename T> T F(S, T(&)[1]);
/* 18 */	template<typename T> T F(S, T(&)[2]);
/* 19 */	template<typename T, int U> T F(S, T(&)[U]);
		};

/* 10 */void C::F(S, int(&)[1]){}
/* 11 */int C::F(S, int(&)[2]){}
/* 12 */template<typename T> T C::F(S, T(&)[1]){}
/* 13 */template<typename T> T C::F(S, T(&)[2]){}
/* 14 */template<typename T, int U> T C::F(S, T(&)[U]){}
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
		.Select([](Tuple<CppClassAccessor, Ptr<Declaration>> t) {return t.f1; })
		.Where([](Ptr<Declaration> decl) {return decl->name.name == L"F"; }),
		true
	);
	TEST_ASSERT(fs.Count() == 20);

	for (vint i = 0; i < 20; i++)
	{
		bool isForward
			= i < 10
			? i % 2 == 0
			: i >= 15
			;

		if (isForward)
		{
			TEST_ASSERT(fs[i]->symbol->GetForwardDecl_Fb() == fs[i]);
		}
		else
		{
			TEST_ASSERT(fs[i]->symbol->GetImplDecl_NFb() == fs[i]);
		}
	}

#define FUNC_FETCH(I)\
		auto funcSymbol = fs[I]->symbol->GetFunctionSymbol_Fb();\

#define FUNC_SYMBOL_COUNT(FORWARD, IMPL)\
		TEST_ASSERT(funcSymbol->GetForwardSymbols_F().Count() == FORWARD);\
		TEST_ASSERT(funcSymbol->GetImplSymbols_F().Count() == IMPL);\

#define FUNC_FORWARD(A, B)\
		TEST_ASSERT(funcSymbol->GetForwardSymbols_F()[A] == fs[B]->symbol);\

#define FUNC_IMPL(A, B)\
		TEST_ASSERT(funcSymbol->GetImplSymbols_F()[A] == fs[B]->symbol);\

	{
		FUNC_FETCH(0);
		FUNC_SYMBOL_COUNT(2, 2);
		FUNC_FORWARD(0, 0);
		FUNC_FORWARD(1, 2);
		FUNC_IMPL(0, 1);
		FUNC_IMPL(1, 3);
	}
	{
		FUNC_FETCH(4);
		FUNC_SYMBOL_COUNT(2, 2);
		FUNC_FORWARD(0, 4);
		FUNC_FORWARD(1, 6);
		FUNC_IMPL(0, 5);
		FUNC_IMPL(1, 7);
	}
	{
		FUNC_FETCH(8);
		FUNC_SYMBOL_COUNT(1, 1);
		FUNC_FORWARD(0, 8);
		FUNC_IMPL(0, 9);
	}

	{
		FUNC_FETCH(10);
		FUNC_SYMBOL_COUNT(2, 2);
		FUNC_FORWARD(0, 15);
		FUNC_FORWARD(1, 16);
		FUNC_IMPL(0, 10);
		FUNC_IMPL(1, 11);
	}
	{
		FUNC_FETCH(12);
		FUNC_SYMBOL_COUNT(2, 2);
		FUNC_FORWARD(0, 17);
		FUNC_FORWARD(1, 18);
		FUNC_IMPL(0, 12);
		FUNC_IMPL(1, 13);
	}
	{
		FUNC_FETCH(14);
		FUNC_SYMBOL_COUNT(1, 1);
		FUNC_FORWARD(0, 19);
		FUNC_IMPL(0, 14);
	}

#undef FUNC_FETCH
#undef FUNC_SYMBOL_COUNT
#undef FUNC_FORWARD
#undef FUNC_IMPL
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