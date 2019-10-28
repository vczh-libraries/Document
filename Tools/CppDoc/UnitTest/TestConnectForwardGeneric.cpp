#include <Ast_Decl.h>
#include "Util.h"

TEST_CASE(TestConnectForwardGeneric_Functions)
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

TEST_CASE(TestConnectForwardGeneric_FunctionsWithAmbiguity)
{
	auto input = LR"(
/*  0 */void F(int(&)[1]);
/*  1 */void F(int(&)[1]){}

/*  2 */void F(int(&)[2]);
/*  3 */void F(int(&)[2]){}

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
/* 16 */	void F(S, int(&)[2]);
/* 17 */	template<typename T> T F(S, T(&)[1]);
/* 18 */	template<typename T> T F(S, T(&)[2]);
/* 19 */	template<typename T, int U> T F(S, T(&)[U]);
		};

/* 10 */void C::F(S, int(&)[1]){}
/* 11 */void C::F(S, int(&)[2]){}
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

TEST_CASE(TestConnectForwardGeneric_Classes)
{
	auto input = LR"(
template<typename T>
struct X;

template<typename T>
struct X;

template<typename T>
struct X{};

template<typename T>
struct X;

template<typename T>
struct X;
)";
	COMPILE_PROGRAM(program, pa, input);

	TEST_ASSERT(pa.root->TryGetChildren_NFb(L"X")->Count() == 1);
	auto symbol = pa.root->TryGetChildren_NFb(L"X")->Get(0);

	TEST_ASSERT(symbol->GetImplDecl_NFb<ClassDeclaration>());
	TEST_ASSERT(symbol->GetForwardDecls_N().Count() == 4);
	TEST_ASSERT(From(symbol->GetForwardDecls_N()).Distinct().Count() == 4);
	for (vint i = 0; i < 4; i++)
	{
		TEST_ASSERT(symbol->GetForwardDecls_N()[i].Cast<ForwardClassDeclaration>());
		TEST_ASSERT(!symbol->GetForwardDecls_N()[i].Cast<ClassDeclaration>());
	}
}

TEST_CASE(TestConnectForwardGeneric_ClassMembers)
{
	auto input = LR"(
namespace ns
{
	struct A
	{
		template<typename T>
		struct B
		{
			struct C
			{
				template<typename U>
				struct D
				{
					void Method();
					template<typename V> void Method(V, V*);
					template<typename V> void Method(V*, V);
				};
			};
		};
	};
}

namespace ns
{
	template<typename X>	template<typename Y>							void A::B<X>::C::D<Y>::Method(){}
	template<typename X>	template<typename Y>	template<typaname Z>	void A::B<X>::C::D<Y>::Method(Z, Z*){}
	template<typename X>	template<typename Y>	template<typaname Z>	void A::B<X>::C::D<Y>::Method(Z*, Z){}
}
)";
	COMPILE_PROGRAM(program, pa, input);

	using Item = Tuple<CppClassAccessor, Ptr<Declaration>>;
	List<Ptr<Declaration>> inClassMembers;
	auto& inClassMembersUnfiltered = pa.root
		->TryGetChildren_NFb(L"ns")->Get(0)
		->TryGetChildren_NFb(L"A")->Get(0)
		->TryGetChildren_NFb(L"B")->Get(0)
		->TryGetChildren_NFb(L"C")->Get(0)
		->TryGetChildren_NFb(L"D")->Get(0)
		->GetImplDecl_NFb<ClassDeclaration>()->decls;

	CopyFrom(inClassMembers, From(inClassMembersUnfiltered).Where([](Item item) {return !item.f1->implicitlyGeneratedMember; }).Select([](Item item) { return item.f1; }));
	TEST_ASSERT(inClassMembers.Count() == 3);

	auto& outClassMembers = pa.root
		->TryGetChildren_NFb(L"ns")->Get(0)
		->GetForwardDecls_N()[1].Cast<NamespaceDeclaration>()->decls;
	TEST_ASSERT(outClassMembers.Count() == 3);

	for (vint i = 0; i < 3; i++)
	{
		auto inClassDecl = inClassMembers[i];
		auto outClassDecl = outClassMembers[i];

		auto symbol = inClassDecl->symbol->GetFunctionSymbol_Fb();
		TEST_ASSERT(symbol->kind == symbol_component::SymbolKind::FunctionSymbol);

		TEST_ASSERT(symbol->GetImplSymbols_F().Count() == 1);
		TEST_ASSERT(symbol->GetImplSymbols_F()[0]->GetImplDecl_NFb() == outClassDecl);

		TEST_ASSERT(symbol->GetForwardSymbols_F().Count() == 1);
		TEST_ASSERT(symbol->GetForwardSymbols_F()[0]->GetForwardDecl_Fb() == inClassDecl);
	}
}