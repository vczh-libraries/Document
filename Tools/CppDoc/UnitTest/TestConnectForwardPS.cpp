#include "Util.h"

TEST_FILE
{
	TEST_CATEGORY(L"Functions")
	{
		auto input = LR"(
template<typename T, typename U, typename... Ts>	void F										(T*, U(*...)(Ts&));
template<typename T, typename U, typename... Ts>	void F										(T, U(*...)(Ts));
template<typename T, typename U, typename... Ts>	void G										(T*, U(*...)(Ts&));
template<typename T, typename U, typename... Ts>	void G										(T, U(*...)(Ts));

template<>											void F<bool, bool, float, double>			(bool*, bool(*)(float&), bool(*)(double&));
template<>											void F<void, void, char, wchar_t, bool>		(void*, void(*)(char&), void(*)(wchar_t&), void(*)(bool&));
template<>											void F<char, char>							(char*);

template<>											void F<bool*, bool, float&, double&>		(bool*, bool(*)(float&), bool(*)(double&));
template<>											void F<void*, void, char&, wchar_t&, bool&>	(void*, void(*)(char&), void(*)(wchar_t&), void(*)(bool&));
template<>											void F<char*, char>							(char*);

template<>											void G<bool, bool, float, double>			(bool*, bool(*)(float&), bool(*)(double&));
template<>											void G<void, void, char, wchar_t, bool>		(void*, void(*)(char&), void(*)(wchar_t&), void(*)(bool&));
template<>											void G<char, char>							(char*);

template<>											void G<bool*, bool, float&, double&>		(bool*, bool(*)(float&), bool(*)(double&));
template<>											void G<void*, void, char&, wchar_t&, bool&>	(void*, void(*)(char&), void(*)(wchar_t&), void(*)(bool&));
template<>											void G<char*, char>							(char*);

template<typename U, typename V, typename... Us>	void F										(U*, V(*...)(Us&)){}
template<typename U, typename V, typename... Us>	void F										(U, V(*...)(Us)){}
template<typename U, typename V, typename... Us>	void G										(U*, V(*...)(Us&)){}
template<typename U, typename V, typename... Us>	void G										(U, V(*...)(Us)){}

template<>											void F<bool, bool, float, double>			(bool*, bool(*)(float&), bool(*)(double&)){}
template<>											void F<void, void, char, wchar_t, bool>		(void*, void(*)(char&), void(*)(wchar_t&), void(*)(bool&)){}
template<>											void F<char, char>							(char*){}

template<>											void F<bool*, bool, float&, double&>		(bool*, bool(*)(float&), bool(*)(double&)){}
template<>											void F<void*, void, char&, wchar_t&, bool&>	(void*, void(*)(char&), void(*)(wchar_t&), void(*)(bool&)){}
template<>											void F<char*, char>							(char*){}

template<>											void G<bool, bool, float, double>			(bool*, bool(*)(float&), bool(*)(double&)){}
template<>											void G<void, void, char, wchar_t, bool>		(void*, void(*)(char&), void(*)(wchar_t&), void(*)(bool&)){}
template<>											void G<char, char>							(char*){}

template<>											void G<bool*, bool, float&, double&>		(bool*, bool(*)(float&), bool(*)(double&)){}
template<>											void G<void*, void, char&, wchar_t&, bool&>	(void*, void(*)(char&), void(*)(wchar_t&), void(*)(bool&)){}
template<>											void G<char*, char>							(char*){}
)";

		COMPILE_PROGRAM(program, pa, input);

		List<Ptr<ForwardFunctionDeclaration>> ffs[2];
		List<Ptr<FunctionDeclaration>> fs[2];
		for (vint i = 0; i < program->decls.Count(); i++)
		{
			auto decl = program->decls[i];
			if (auto fdecl = decl.Cast<FunctionDeclaration>())
			{
				if (fdecl->name.name == L"F") fs[0].Add(fdecl);
				if (fdecl->name.name == L"G") fs[1].Add(fdecl);
			}
			else if (auto ffdecl = decl.Cast<ForwardFunctionDeclaration>())
			{
				if (ffdecl->name.name == L"F") ffs[0].Add(ffdecl);
				if (ffdecl->name.name == L"G") ffs[1].Add(ffdecl);
			}
		}

		TEST_CASE_ASSERT(ffs[0].Count() == 8);
		TEST_CASE_ASSERT(ffs[1].Count() == 8);
		TEST_CASE_ASSERT(fs[0].Count() == 8);
		TEST_CASE_ASSERT(fs[1].Count() == 8);
		for (vint i = 0; i < 2; i++)
		{
			TEST_CATEGORY(WString(L"Test function: ") + (i == 0 ? L"F" : L"G"))
			{
				TEST_CATEGORY(L"Check connections")
				{
					for (vint j = 0; j < 8; j++)
					{
						auto symbol = ffs[i][j]->symbol->GetFunctionSymbol_Fb();
						TEST_CASE_ASSERT(symbol == fs[i][j]->symbol->GetFunctionSymbol_Fb());
					}
				});

				for (vint j = 0; j < 2; j++)
				{
					TEST_CATEGORY(L"Check partial specialization relationship for group: " + itow(j))
					{
						auto primary = ffs[i][j]->symbol->GetFunctionSymbol_Fb();
						for (vint k = 0; k < 4; k++)
						{
							if (k == 0)
							{
								TEST_CASE_ASSERT(primary->IsPSPrimary_NF());
								TEST_CASE_ASSERT(primary->GetPSPrimaryDescendants_NF().Count() == 3);
								TEST_CASE_ASSERT(primary->GetPSChildren_NF().Count() == 3);
							}
							else
							{
								auto symbol = ffs[i][1 + j * 3 + k]->symbol->GetFunctionSymbol_Fb();
								TEST_CASE_ASSERT(symbol->GetPSPrimary_NF() == primary);
								TEST_CASE_ASSERT(symbol->GetPSParents_NF().Count() == 1);
								TEST_CASE_ASSERT(symbol->GetPSParents_NF()[0] == primary);
							}
						}
					});
				}
			});
		}
	});

	TEST_CATEGORY(L"Members")
	{
		auto input = LR"(
namespace ns
{
	struct A
	{
		template<int X, typename T>
		struct B
		{
			struct C
			{
				template<typename U, int Y>
				struct D
				{
					static const bool field;
					void Method(T, U);
					template<typename V> void Method(T, U*, V, V*);
					template<typename V> void Method(T*, U, V*, V);
				};

				template<int Y, typename U1, typename U2, typename U3>
				struct D<U1(*)(U2, U3), Y>
				{
					static const bool field;
					void Method(T, U1);
					template<typename V> void Method(T, U2*, V, V*);
					template<typename V> void Method(T*, U3, V*, V);
				};
			};
		};

		template<typename T1, typename T2, typename T3, int X>
		struct B<X, T1(*)(T2, T3)>
		{
			struct C
			{
				template<typename U, int Y>
				struct D
				{
					static const bool field;
					void Method(T1, U);
					template<typename V> void Method(T2, U*, V, V*);
					template<typename V> void Method(T3*, U, V*, V);
				};

				template<int Y, typename U1, typename U2, typename U3>
				struct D<U1(*)(U2, U3), Y>
				{
					static const bool field;
					void Method(T1, U1);
					template<typename V> void Method(T2, U2*, V, V*);
					template<typename V> void Method(T3*, U3, V*, V);
				};
			};
		};
	};
}

namespace ns
{
	template<int _1, typename X>								template<typename Y, int _2>														bool A::B<_1, X>::C::D<Y, _2>::field = false;
	template<int _1, typename X>								template<typename Y, int _2>														void A::B<_1, X>::C::D<Y, _2>::Method(X, Y){}
	template<int _1, typename X>								template<typename Y, int _2>								template<typename Z>	void A::B<_1, X>::C::D<Y, _2>::Method(X, Y*, Z, Z*){}
	template<int _1, typename X>								template<typename Y, int _2>								template<typename Z>	void A::B<_1, X>::C::D<Y, _2>::Method(X*, Y, Z*, Z){}

	template<int _1, typename X>								template<int _2, typename Y1, typename Y2, typename Y3>								bool A::B<_1, X>::C::D<Y1(*)(Y2, Y3), _2>::field = false;
	template<int _1, typename X>								template<int _2, typename Y1, typename Y2, typename Y3>								void A::B<_1, X>::C::D<Y1(*)(Y2, Y3), _2>::Method(X, Y1){}
	template<int _1, typename X>								template<int _2, typename Y1, typename Y2, typename Y3>		template<typename Z>	void A::B<_1, X>::C::D<Y1(*)(Y2, Y3), _2>::Method(X, Y2*, Z, Z*){}
	template<int _1, typename X>								template<int _2, typename Y1, typename Y2, typename Y3>		template<typename Z>	void A::B<_1, X>::C::D<Y1(*)(Y2, Y3), _2>::Method(X*, Y3, Z*, Z){}

	template<typename X1, typename X2, typename X3, int _1>		template<typename Y, int _2>														bool A::B<_1, X1(*)(X2, X3)>::C::D<Y, _2>::field = false;
	template<typename X1, typename X2, typename X3, int _1>		template<typename Y, int _2>														void A::B<_1, X1(*)(X2, X3)>::C::D<Y, _2>::Method(X1, Y){}
	template<typename X1, typename X2, typename X3, int _1>		template<typename Y, int _2>								template<typename Z>	void A::B<_1, X1(*)(X2, X3)>::C::D<Y, _2>::Method(X2, Y*, Z, Z*){}
	template<typename X1, typename X2, typename X3, int _1>		template<typename Y, int _2>								template<typename Z>	void A::B<_1, X1(*)(X2, X3)>::C::D<Y, _2>::Method(X3*, Y, Z*, Z){}

	template<typename X1, typename X2, typename X3, int _1>		template<int _2, typename Y1, typename Y2, typename Y3>								bool A::B<_1, X1(*)(X2, X3)>::C::D<Y1(*)(Y2, Y3), _2>::field = false;
	template<typename X1, typename X2, typename X3, int _1>		template<int _2, typename Y1, typename Y2, typename Y3>								void A::B<_1, X1(*)(X2, X3)>::C::D<Y1(*)(Y2, Y3), _2>::Method(X1, Y1){}
	template<typename X1, typename X2, typename X3, int _1>		template<int _2, typename Y1, typename Y2, typename Y3>		template<typename Z>	void A::B<_1, X1(*)(X2, X3)>::C::D<Y1(*)(Y2, Y3), _2>::Method(X2, Y2*, Z, Z*){}
	template<typename X1, typename X2, typename X3, int _1>		template<int _2, typename Y1, typename Y2, typename Y3>		template<typename Z>	void A::B<_1, X1(*)(X2, X3)>::C::D<Y1(*)(Y2, Y3), _2>::Method(X3*, Y3, Z*, Z){}
}
)";
		COMPILE_PROGRAM(program, pa, input);

		TEST_CATEGORY(L"Checking connections")
		{
			using Item = Tuple<CppClassAccessor, Ptr<Declaration>>;
			List<Ptr<Declaration>> inClassMembers;

			auto& inClassMembersUnfiltered1 = pa.root
				->TryGetChildren_NFb(L"ns")->Get(0).childSymbol
				->TryGetChildren_NFb(L"A")->Get(0).childSymbol
				->TryGetChildren_NFb(L"B")->Get(0).childSymbol
				->TryGetChildren_NFb(L"C")->Get(0).childSymbol
				->TryGetChildren_NFb(L"D")->Get(0).childSymbol
				->GetImplDecl_NFb<ClassDeclaration>()->decls;

			auto& inClassMembersUnfiltered2 = pa.root
				->TryGetChildren_NFb(L"ns")->Get(0).childSymbol
				->TryGetChildren_NFb(L"A")->Get(0).childSymbol
				->TryGetChildren_NFb(L"B")->Get(0).childSymbol
				->TryGetChildren_NFb(L"C")->Get(0).childSymbol
				->TryGetChildren_NFb(L"D@<[U1]([U2], [U3]) *, *>")->Get(0).childSymbol
				->GetImplDecl_NFb<ClassDeclaration>()->decls;

			auto& inClassMembersUnfiltered3 = pa.root
				->TryGetChildren_NFb(L"ns")->Get(0).childSymbol
				->TryGetChildren_NFb(L"A")->Get(0).childSymbol
				->TryGetChildren_NFb(L"B@<*, [T1]([T2], [T3]) *>")->Get(0).childSymbol
				->TryGetChildren_NFb(L"C")->Get(0).childSymbol
				->TryGetChildren_NFb(L"D")->Get(0).childSymbol
				->GetImplDecl_NFb<ClassDeclaration>()->decls;

			auto& inClassMembersUnfiltered4 = pa.root
				->TryGetChildren_NFb(L"ns")->Get(0).childSymbol
				->TryGetChildren_NFb(L"A")->Get(0).childSymbol
				->TryGetChildren_NFb(L"B@<*, [T1]([T2], [T3]) *>")->Get(0).childSymbol
				->TryGetChildren_NFb(L"C")->Get(0).childSymbol
				->TryGetChildren_NFb(L"D@<[U1]([U2], [U3]) *, *>")->Get(0).childSymbol
				->GetImplDecl_NFb<ClassDeclaration>()->decls;

#define FILTER_CONDITION .Where([](Item item) {return !item.f1->implicitlyGeneratedMember; }).Select([](Item item) { return item.f1; })
			CopyFrom(inClassMembers, From(inClassMembersUnfiltered1) FILTER_CONDITION, true);
			CopyFrom(inClassMembers, From(inClassMembersUnfiltered2) FILTER_CONDITION, true);
			CopyFrom(inClassMembers, From(inClassMembersUnfiltered3) FILTER_CONDITION, true);
			CopyFrom(inClassMembers, From(inClassMembersUnfiltered4) FILTER_CONDITION, true);
#undef FILTER_CONDITION
			TEST_CASE_ASSERT(inClassMembers.Count() == 16);

			auto& outClassMembers = pa.root
				->TryGetChildren_NFb(L"ns")->Get(0).childSymbol
				->GetForwardDecls_N()[1].Cast<NamespaceDeclaration>()->decls;
			TEST_CASE_ASSERT(outClassMembers.Count() == 16);

			for (vint c = 0; c < 4; c++)
			{
				TEST_CATEGORY(L"Category " + itow(c))
				{
					for (vint m = 0; m < 4; m++)
					{
						TEST_CATEGORY(L"Member " + itow(m))
						{
							vint i = c * 4 + m;
							auto inClassDecl = inClassMembers[i];
							auto outClassDecl = outClassMembers[i];

							if (m == 0)
							{
								auto symbol = inClassDecl->symbol;
								TEST_CASE_ASSERT(symbol->kind == symbol_component::SymbolKind::Variable);

								TEST_CASE_ASSERT(symbol->GetImplDecl_NFb() == outClassDecl);

								TEST_CASE_ASSERT(symbol->GetForwardDecls_N().Count() == 1);
								TEST_CASE_ASSERT(symbol->GetForwardDecls_N()[0] == inClassDecl);
							}
							else
							{
								auto symbol = inClassDecl->symbol->GetFunctionSymbol_Fb();
								TEST_CASE_ASSERT(symbol->kind == symbol_component::SymbolKind::FunctionSymbol);

								TEST_CASE_ASSERT(symbol->GetImplSymbols_F().Count() == 1);
								TEST_CASE_ASSERT(symbol->GetImplSymbols_F()[0]->GetImplDecl_NFb() == outClassDecl);

								TEST_CASE_ASSERT(symbol->GetForwardSymbols_F().Count() == 1);
								TEST_CASE_ASSERT(symbol->GetForwardSymbols_F()[0]->GetForwardDecl_Fb() == inClassDecl);
							}
						});
					}
				});
			}
		});
	});

	TEST_CATEGORY(L"Methods")
	{
		auto input = LR"(
namespace ns
{
	struct A
	{
		template<int X, typename T>
		struct B
		{
			struct C
			{
				template<typename U, int Y>
				struct D
				{
					template<typename V, typename... Vs>	V Method(Vs...);
					template<>								void Method<void, T, U>(T, U);
					template<>								void Method<void, T, U*, bool, bool*>(T, U*, bool, bool*);
					template<>								void Method<void, T*, U, bool*, bool>(T*, U, bool*, bool);
				};

				template<int Y, typename U1, typename U2, typename U3>
				struct D<U1(*)(U2, U3), Y>
				{
					template<typename V, typename... Vs>	V Method(Vs...);
					template<>								void Method<void, T, U1>(T, U1);
					template<>								void Method<void, T, U2*, bool, bool*>(T, U2*, bool, bool*);
					template<>								void Method<void, T*, U3, bool*, bool>(T*, U3, bool*, bool);
				};
			};
		};

		template<typename T1, typename T2, typename T3, int X>
		struct B<X, T1(*)(T2, T3)>
		{
			struct C
			{
				template<typename U, int Y>
				struct D
				{
					template<typename V, typename... Vs>	V Method(Vs...);
					template<>								void Method<void, T1, U>(T1, U);
					template<>								void Method<void, T2, U*, bool, bool*>(T2, U*, bool, bool*);
					template<>								void Method<void, T3*, U, bool*, bool>(T3*, U, bool*, bool);
				};

				template<int Y, typename U1, typename U2, typename U3>
				struct D<U1(*)(U2, U3), Y>
				{
					template<typename V, typename... Vs>	V Method(Vs...);
					template<>								void Method<void, T1, U1>(T1, U1);
					template<>								void Method<void, T2, U2*, bool, bool*>(T2, U2*, bool, bool*);
					template<>								void Method<void, T3*, U3, bool*, bool>(T3*, U3, bool*, bool);
				};
			};
		};
	};
}

namespace ns
{
	template<int _1, typename X>								template<typename Y, int _2>								template<typename Z, typename... Zs>	Z A::B<_1, X>::C::D<Y, _2>::Method(Zs...){}
	template<int _1, typename X>								template<typename Y, int _2>								template<>								void A::B<_1, X>::C::D<Y, _2>::Method<void, X, Y>(X, Y){}
	template<int _1, typename X>								template<typename Y, int _2>								template<>								void A::B<_1, X>::C::D<Y, _2>::Method<void, X, Y*, bool, bool*>(X, Y*, bool, bool*){}
	template<int _1, typename X>								template<typename Y, int _2>								template<>								void A::B<_1, X>::C::D<Y, _2>::Method<void, X*, Y, bool*, bool>(X*, Y, bool*, bool){}

	template<int _1, typename X>								template<int _2, typename Y1, typename Y2, typename Y3>		template<typename Z, typename... Zs>	Z A::B<_1, X>::C::D<Y1(*)(Y2, Y3), _2>::Method(Zs...){}
	template<int _1, typename X>								template<int _2, typename Y1, typename Y2, typename Y3>		template<>								void A::B<_1, X>::C::D<Y1(*)(Y2, Y3), _2>::Method<void, X, Y1>(X, Y1){}
	template<int _1, typename X>								template<int _2, typename Y1, typename Y2, typename Y3>		template<>								void A::B<_1, X>::C::D<Y1(*)(Y2, Y3), _2>::Method<void, X, Y2*, bool, bool*>(X, Y2*, bool, bool*){}
	template<int _1, typename X>								template<int _2, typename Y1, typename Y2, typename Y3>		template<>								void A::B<_1, X>::C::D<Y1(*)(Y2, Y3), _2>::Method<void, X*, Y3, bool*, bool>(X*, Y3, bool*, bool){}

	template<typename X1, typename X2, typename X3, int _1>		template<typename Y, int _2>								template<typename Z, typename... Zs>	Z A::B<_1, X1(*)(X2, X3)>::C::D<Y, _2>::Method(Zs...){}
	template<typename X1, typename X2, typename X3, int _1>		template<typename Y, int _2>								template<>								void A::B<_1, X1(*)(X2, X3)>::C::D<Y, _2>::Method<void, X1, Y>(X1, Y){}
	template<typename X1, typename X2, typename X3, int _1>		template<typename Y, int _2>								template<>								void A::B<_1, X1(*)(X2, X3)>::C::D<Y, _2>::Method<void, X2, Y*, bool, bool*>(X2, Y*, bool, bool*){}
	template<typename X1, typename X2, typename X3, int _1>		template<typename Y, int _2>								template<>								void A::B<_1, X1(*)(X2, X3)>::C::D<Y, _2>::Method<void, X3*, Y, bool*, bool>(X3*, Y, bool*, bool){}

	template<typename X1, typename X2, typename X3, int _1>		template<int _2, typename Y1, typename Y2, typename Y3>		template<typename Z, typename... Zs>	Z A::B<_1, X1(*)(X2, X3)>::C::D<Y1(*)(Y2, Y3), _2>::Method(Zs...){}
	template<typename X1, typename X2, typename X3, int _1>		template<int _2, typename Y1, typename Y2, typename Y3>		template<>								void A::B<_1, X1(*)(X2, X3)>::C::D<Y1(*)(Y2, Y3), _2>::Method<void, X1, Y1>(X1, Y1){}
	template<typename X1, typename X2, typename X3, int _1>		template<int _2, typename Y1, typename Y2, typename Y3>		template<>								void A::B<_1, X1(*)(X2, X3)>::C::D<Y1(*)(Y2, Y3), _2>::Method<void, X2, Y2*, bool, bool*>(X2, Y2*, bool, bool*){}
	template<typename X1, typename X2, typename X3, int _1>		template<int _2, typename Y1, typename Y2, typename Y3>		template<>								void A::B<_1, X1(*)(X2, X3)>::C::D<Y1(*)(Y2, Y3), _2>::Method<void, X3*, Y3, bool*, bool>(X3*, Y3, bool*, bool){}
}
)";
		COMPILE_PROGRAM(program, pa, input);

		TEST_CATEGORY(L"Checking connections")
		{
			using Item = Tuple<CppClassAccessor, Ptr<Declaration>>;
			List<Ptr<Declaration>> inClassMembers;

			auto& inClassMembersUnfiltered1 = pa.root
				->TryGetChildren_NFb(L"ns")->Get(0).childSymbol
				->TryGetChildren_NFb(L"A")->Get(0).childSymbol
				->TryGetChildren_NFb(L"B")->Get(0).childSymbol
				->TryGetChildren_NFb(L"C")->Get(0).childSymbol
				->TryGetChildren_NFb(L"D")->Get(0).childSymbol
				->GetImplDecl_NFb<ClassDeclaration>()->decls;

			auto& inClassMembersUnfiltered2 = pa.root
				->TryGetChildren_NFb(L"ns")->Get(0).childSymbol
				->TryGetChildren_NFb(L"A")->Get(0).childSymbol
				->TryGetChildren_NFb(L"B")->Get(0).childSymbol
				->TryGetChildren_NFb(L"C")->Get(0).childSymbol
				->TryGetChildren_NFb(L"D@<[U1]([U2], [U3]) *, *>")->Get(0).childSymbol
				->GetImplDecl_NFb<ClassDeclaration>()->decls;

			auto& inClassMembersUnfiltered3 = pa.root
				->TryGetChildren_NFb(L"ns")->Get(0).childSymbol
				->TryGetChildren_NFb(L"A")->Get(0).childSymbol
				->TryGetChildren_NFb(L"B@<*, [T1]([T2], [T3]) *>")->Get(0).childSymbol
				->TryGetChildren_NFb(L"C")->Get(0).childSymbol
				->TryGetChildren_NFb(L"D")->Get(0).childSymbol
				->GetImplDecl_NFb<ClassDeclaration>()->decls;

			auto& inClassMembersUnfiltered4 = pa.root
				->TryGetChildren_NFb(L"ns")->Get(0).childSymbol
				->TryGetChildren_NFb(L"A")->Get(0).childSymbol
				->TryGetChildren_NFb(L"B@<*, [T1]([T2], [T3]) *>")->Get(0).childSymbol
				->TryGetChildren_NFb(L"C")->Get(0).childSymbol
				->TryGetChildren_NFb(L"D@<[U1]([U2], [U3]) *, *>")->Get(0).childSymbol
				->GetImplDecl_NFb<ClassDeclaration>()->decls;

#define FILTER_CONDITION .Where([](Item item) {return !item.f1->implicitlyGeneratedMember; }).Select([](Item item) { return item.f1; })
			CopyFrom(inClassMembers, From(inClassMembersUnfiltered1) FILTER_CONDITION, true);
			CopyFrom(inClassMembers, From(inClassMembersUnfiltered2) FILTER_CONDITION, true);
			CopyFrom(inClassMembers, From(inClassMembersUnfiltered3) FILTER_CONDITION, true);
			CopyFrom(inClassMembers, From(inClassMembersUnfiltered4) FILTER_CONDITION, true);
#undef FILTER_CONDITION
			TEST_CASE_ASSERT(inClassMembers.Count() == 16);

			auto& outClassMembers = pa.root
				->TryGetChildren_NFb(L"ns")->Get(0).childSymbol
				->GetForwardDecls_N()[1].Cast<NamespaceDeclaration>()->decls;
			TEST_CASE_ASSERT(outClassMembers.Count() == 16);

			for (vint c = 0; c < 4; c++)
			{
				TEST_CATEGORY(L"Category " + itow(c))
				{
					Symbol* primary = nullptr;
					for (vint m = 0; m < 4; m++)
					{
						TEST_CATEGORY(L"Member " + itow(m))
						{
							vint i = c * 4 + m;
							auto inClassDecl = inClassMembers[i];
							auto outClassDecl = outClassMembers[i];

							auto symbol = inClassDecl->symbol->GetFunctionSymbol_Fb();
							TEST_CASE_ASSERT(symbol->kind == symbol_component::SymbolKind::FunctionSymbol);

							TEST_CASE_ASSERT(symbol->GetImplSymbols_F().Count() == 1);
							TEST_CASE_ASSERT(symbol->GetImplSymbols_F()[0]->GetImplDecl_NFb() == outClassDecl);

							TEST_CASE_ASSERT(symbol->GetForwardSymbols_F().Count() == 1);
							TEST_CASE_ASSERT(symbol->GetForwardSymbols_F()[0]->GetForwardDecl_Fb() == inClassDecl);

							if (m == 0)
							{
								primary = symbol;
								TEST_CASE_ASSERT(primary->IsPSPrimary_NF() == true);
								TEST_CASE_ASSERT(primary->GetPSPrimaryVersion_NF() == 3);
								TEST_CASE_ASSERT(primary->GetPSPrimaryDescendants_NF().Count() == 3);
							}
							else
							{
								TEST_CASE_ASSERT(symbol->GetPSPrimary_NF() == primary);
								TEST_CASE_ASSERT(symbol->GetPSParents_NF().Count() == 1);
								TEST_CASE_ASSERT(symbol->GetPSParents_NF()[0] == primary);
							}
						});
					}
				});
			}
		});
	});

	TEST_CATEGORY(L"Constructors and destructors")
	{
		auto input = LR"(
namespace ns
{
	struct A
	{
		template<int X, typename T>
		struct B
		{
			struct C
			{
				template<typename U, int Y>
				struct D
				{
															D();
															~D();
					template<typename V, typename... Vs>	D(V, Vs...);
					template<typename V, typename... Vs>	D(T, U, V, Vs...);
				};

				template<int Y, typename U1, typename U2, typename U3>
				struct D<U1(*)(U2, U3), Y>
				{
															D();
															~D();
					template<typename V, typename... Vs>	D(V, Vs...);
					template<typename V, typename... Vs>	D(T, U1, U2, U3, V, Vs...);
				};
			};
		};

		template<typename T1, typename T2, typename T3, int X>
		struct B<X, T1(*)(T2, T3)>
		{
			struct C
			{
				template<typename U, int Y>
				struct D
				{
															D();
															~D();
					template<typename V, typename... Vs>	D(V, Vs...);
					template<typename V, typename... Vs>	D(T1, T2, T3, U, V, Vs...);
				};

				template<int Y, typename U1, typename U2, typename U3>
				struct D<U1(*)(U2, U3), Y>
				{
															D();
															~D();
					template<typename V, typename... Vs>	D(V, Vs...);
					template<typename V, typename... Vs>	D(T1, T2, T3, U1, U2, U3, V, Vs...);
				};
			};
		};
	};
}

namespace ns
{
	template<int _1, typename X>								template<typename Y, int _2>																		A::B<_1, X>::C::D<Y, _2>::D(){}
	template<int _1, typename X>								template<typename Y, int _2>																		A::B<_1, X>::C::D<Y, _2>::~D(){}
	template<int _1, typename X>								template<typename Y, int _2>								template<typename Z, typename... Zs>	A::B<_1, X>::C::D<Y, _2>::D(Z, Zs...){}
	template<int _1, typename X>								template<typename Y, int _2>								template<typename Z, typename... Zs>	A::B<_1, X>::C::D<Y, _2>::D(X, Y, Z, Zs...){}

	template<int _1, typename X>								template<int _2, typename Y1, typename Y2, typename Y3>												A::B<_1, X>::C::D<Y1(*)(Y2, Y3), _2>::D(){}
	template<int _1, typename X>								template<int _2, typename Y1, typename Y2, typename Y3>												A::B<_1, X>::C::D<Y1(*)(Y2, Y3), _2>::~D(){}
	template<int _1, typename X>								template<int _2, typename Y1, typename Y2, typename Y3>		template<typename Z, typename... Zs>	A::B<_1, X>::C::D<Y1(*)(Y2, Y3), _2>::D(Z, Zs...){}
	template<int _1, typename X>								template<int _2, typename Y1, typename Y2, typename Y3>		template<typename Z, typename... Zs>	A::B<_1, X>::C::D<Y1(*)(Y2, Y3), _2>::D(X, Y1, Y2, Y3, Z, Zs...){}

	template<typename X1, typename X2, typename X3, int _1>		template<typename Y, int _2>																		A::B<_1, X1(*)(X2, X3)>::C::D<Y, _2>::D(){}
	template<typename X1, typename X2, typename X3, int _1>		template<typename Y, int _2>																		A::B<_1, X1(*)(X2, X3)>::C::D<Y, _2>::~D(){}
	template<typename X1, typename X2, typename X3, int _1>		template<typename Y, int _2>								template<typename Z, typename... Zs>	A::B<_1, X1(*)(X2, X3)>::C::D<Y, _2>::D(Z, Zs...){}
	template<typename X1, typename X2, typename X3, int _1>		template<typename Y, int _2>								template<typename Z, typename... Zs>	A::B<_1, X1(*)(X2, X3)>::C::D<Y, _2>::D(X1, X2, X3, Y, Z, Zs...){}

	template<typename X1, typename X2, typename X3, int _1>		template<int _2, typename Y1, typename Y2, typename Y3>												A::B<_1, X1(*)(X2, X3)>::C::D<Y1(*)(Y2, Y3), _2>::D(){}
	template<typename X1, typename X2, typename X3, int _1>		template<int _2, typename Y1, typename Y2, typename Y3>												A::B<_1, X1(*)(X2, X3)>::C::D<Y1(*)(Y2, Y3), _2>::~D(){}
	template<typename X1, typename X2, typename X3, int _1>		template<int _2, typename Y1, typename Y2, typename Y3>		template<typename Z, typename... Zs>	A::B<_1, X1(*)(X2, X3)>::C::D<Y1(*)(Y2, Y3), _2>::D(Z, Zs...){}
	template<typename X1, typename X2, typename X3, int _1>		template<int _2, typename Y1, typename Y2, typename Y3>		template<typename Z, typename... Zs>	A::B<_1, X1(*)(X2, X3)>::C::D<Y1(*)(Y2, Y3), _2>::D(X1, X2, X3, Y1, Y2, Y3, Z, Zs...){}
}
)";
		COMPILE_PROGRAM(program, pa, input);

		TEST_CATEGORY(L"Checking connections")
		{
			using Item = Tuple<CppClassAccessor, Ptr<Declaration>>;
			List<Ptr<Declaration>> inClassMembers;

			auto& inClassMembersUnfiltered1 = pa.root
				->TryGetChildren_NFb(L"ns")->Get(0).childSymbol
				->TryGetChildren_NFb(L"A")->Get(0).childSymbol
				->TryGetChildren_NFb(L"B")->Get(0).childSymbol
				->TryGetChildren_NFb(L"C")->Get(0).childSymbol
				->TryGetChildren_NFb(L"D")->Get(0).childSymbol
				->GetImplDecl_NFb<ClassDeclaration>()->decls;

			auto& inClassMembersUnfiltered2 = pa.root
				->TryGetChildren_NFb(L"ns")->Get(0).childSymbol
				->TryGetChildren_NFb(L"A")->Get(0).childSymbol
				->TryGetChildren_NFb(L"B")->Get(0).childSymbol
				->TryGetChildren_NFb(L"C")->Get(0).childSymbol
				->TryGetChildren_NFb(L"D@<[U1]([U2], [U3]) *, *>")->Get(0).childSymbol
				->GetImplDecl_NFb<ClassDeclaration>()->decls;

			auto& inClassMembersUnfiltered3 = pa.root
				->TryGetChildren_NFb(L"ns")->Get(0).childSymbol
				->TryGetChildren_NFb(L"A")->Get(0).childSymbol
				->TryGetChildren_NFb(L"B@<*, [T1]([T2], [T3]) *>")->Get(0).childSymbol
				->TryGetChildren_NFb(L"C")->Get(0).childSymbol
				->TryGetChildren_NFb(L"D")->Get(0).childSymbol
				->GetImplDecl_NFb<ClassDeclaration>()->decls;

			auto& inClassMembersUnfiltered4 = pa.root
				->TryGetChildren_NFb(L"ns")->Get(0).childSymbol
				->TryGetChildren_NFb(L"A")->Get(0).childSymbol
				->TryGetChildren_NFb(L"B@<*, [T1]([T2], [T3]) *>")->Get(0).childSymbol
				->TryGetChildren_NFb(L"C")->Get(0).childSymbol
				->TryGetChildren_NFb(L"D@<[U1]([U2], [U3]) *, *>")->Get(0).childSymbol
				->GetImplDecl_NFb<ClassDeclaration>()->decls;

#define FILTER_CONDITION .Where([](Item item) {return !item.f1->implicitlyGeneratedMember; }).Select([](Item item) { return item.f1; })
			CopyFrom(inClassMembers, From(inClassMembersUnfiltered1) FILTER_CONDITION, true);
			CopyFrom(inClassMembers, From(inClassMembersUnfiltered2) FILTER_CONDITION, true);
			CopyFrom(inClassMembers, From(inClassMembersUnfiltered3) FILTER_CONDITION, true);
			CopyFrom(inClassMembers, From(inClassMembersUnfiltered4) FILTER_CONDITION, true);
#undef FILTER_CONDITION
			TEST_CASE_ASSERT(inClassMembers.Count() == 16);

			auto& outClassMembers = pa.root
				->TryGetChildren_NFb(L"ns")->Get(0).childSymbol
				->GetForwardDecls_N()[1].Cast<NamespaceDeclaration>()->decls;
			TEST_CASE_ASSERT(outClassMembers.Count() == 16);

			for (vint c = 0; c < 4; c++)
			{
				TEST_CATEGORY(L"Category " + itow(c))
				{
					for (vint m = 0; m < 4; m++)
					{
						TEST_CATEGORY(L"Member " + itow(m))
						{
							vint i = c * 4 + m;
							auto inClassDecl = inClassMembers[i];
							auto outClassDecl = outClassMembers[i];

							auto symbol = inClassDecl->symbol->GetFunctionSymbol_Fb();
							TEST_CASE_ASSERT(symbol->kind == symbol_component::SymbolKind::FunctionSymbol);

							TEST_CASE_ASSERT(symbol->GetImplSymbols_F().Count() == 1);
							TEST_CASE_ASSERT(symbol->GetImplSymbols_F()[0]->GetImplDecl_NFb() == outClassDecl);

							TEST_CASE_ASSERT(symbol->GetForwardSymbols_F().Count() == 1);
							TEST_CASE_ASSERT(symbol->GetForwardSymbols_F()[0]->GetForwardDecl_Fb() == inClassDecl);
						});
					}
				});
			}
		});
	});

	TEST_CATEGORY(L"Methods with value template argument")
	{
		auto input = LR"(
namespace ns
{
	struct A
	{
		template<int X>
		struct B
		{
			struct C
			{
				template<int Y>
				struct D
				{
					template<int Z>		void Method();
					template<>			void Method<1>();
					template<>			void Method<2>();
					template<>			void Method<3>();
				};

				template<>
				struct D<2>
				{
					template<int Z>		void Method();
					template<>			void Method<1>();
					template<>			void Method<2>();
					template<>			void Method<3>();
				};
			};
		};

		template<>
		struct B<1>
		{
			struct C
			{
				template<int Y>
				struct D
				{
					template<int Z>		void Method();
					template<>			void Method<1>();
					template<>			void Method<2>();
					template<>			void Method<3>();
				};

				template<>
				struct D<2>
				{
					template<int Z>		void Method();
					template<>			void Method<1>();
					template<>			void Method<2>();
					template<>			void Method<3>();
				};
			};
		};
	};
}

namespace ns
{
	template<int _1>	template<int _2>	template<int _3>	void A::B<_1>::C::D<_2>::Method()		{}
	template<int _1>	template<int _2>	template<>			void A::B<_1>::C::D<_2>::Method<1>()	{}
	template<int _1>	template<int _2>	template<>			void A::B<_1>::C::D<_2>::Method<2>()	{}
	template<int _1>	template<int _2>	template<>			void A::B<_1>::C::D<_2>::Method<3>()	{}
				   
	template<int _1>	template<>			template<int _3>	void A::B<_1>::C::D<2>::Method()		{}
	template<int _1>	template<>			template<>			void A::B<_1>::C::D<2>::Method<1>()		{}
	template<int _1>	template<>			template<>			void A::B<_1>::C::D<2>::Method<2>()		{}
	template<int _1>	template<>			template<>			void A::B<_1>::C::D<2>::Method<3>()		{}

	template<>			template<int _2>	template<int _3>	void A::B<1>::C::D<_2>::Method()		{}
	template<>			template<int _2>	template<>			void A::B<1>::C::D<_2>::Method<1>()		{}
	template<>			template<int _2>	template<>			void A::B<1>::C::D<_2>::Method<2>()		{}
	template<>			template<int _2>	template<>			void A::B<1>::C::D<_2>::Method<3>()		{}

	template<>			template<>			template<int _3>	void A::B<1>::C::D<2>::Method()			{}
	template<>			template<>			template<>			void A::B<1>::C::D<2>::Method<1>()		{}
	template<>			template<>			template<>			void A::B<1>::C::D<2>::Method<2>()		{}
	template<>			template<>			template<>			void A::B<1>::C::D<2>::Method<3>()		{}
}
)";
		COMPILE_PROGRAM(program, pa, input);

		TEST_CATEGORY(L"Checking connections")
		{
			using Item = Tuple<CppClassAccessor, Ptr<Declaration>>;
			List<Ptr<Declaration>> inClassMembers;

			auto& inClassMembersUnfiltered1 = pa.root
				->TryGetChildren_NFb(L"ns")->Get(0).childSymbol
				->TryGetChildren_NFb(L"A")->Get(0).childSymbol
				->TryGetChildren_NFb(L"B")->Get(0).childSymbol
				->TryGetChildren_NFb(L"C")->Get(0).childSymbol
				->TryGetChildren_NFb(L"D")->Get(0).childSymbol
				->GetImplDecl_NFb<ClassDeclaration>()->decls;

			auto& inClassMembersUnfiltered2 = pa.root
				->TryGetChildren_NFb(L"ns")->Get(0).childSymbol
				->TryGetChildren_NFb(L"A")->Get(0).childSymbol
				->TryGetChildren_NFb(L"B")->Get(0).childSymbol
				->TryGetChildren_NFb(L"C")->Get(0).childSymbol
				->TryGetChildren_NFb(L"D@<*>")->Get(0).childSymbol
				->GetImplDecl_NFb<ClassDeclaration>()->decls;

			auto& inClassMembersUnfiltered3 = pa.root
				->TryGetChildren_NFb(L"ns")->Get(0).childSymbol
				->TryGetChildren_NFb(L"A")->Get(0).childSymbol
				->TryGetChildren_NFb(L"B@<*>")->Get(0).childSymbol
				->TryGetChildren_NFb(L"C")->Get(0).childSymbol
				->TryGetChildren_NFb(L"D")->Get(0).childSymbol
				->GetImplDecl_NFb<ClassDeclaration>()->decls;

			auto& inClassMembersUnfiltered4 = pa.root
				->TryGetChildren_NFb(L"ns")->Get(0).childSymbol
				->TryGetChildren_NFb(L"A")->Get(0).childSymbol
				->TryGetChildren_NFb(L"B@<*>")->Get(0).childSymbol
				->TryGetChildren_NFb(L"C")->Get(0).childSymbol
				->TryGetChildren_NFb(L"D@<*>")->Get(0).childSymbol
				->GetImplDecl_NFb<ClassDeclaration>()->decls;

#define FILTER_CONDITION .Where([](Item item) {return !item.f1->implicitlyGeneratedMember; }).Select([](Item item) { return item.f1; })
			CopyFrom(inClassMembers, From(inClassMembersUnfiltered1) FILTER_CONDITION, true);
			CopyFrom(inClassMembers, From(inClassMembersUnfiltered2) FILTER_CONDITION, true);
			CopyFrom(inClassMembers, From(inClassMembersUnfiltered3) FILTER_CONDITION, true);
			CopyFrom(inClassMembers, From(inClassMembersUnfiltered4) FILTER_CONDITION, true);
#undef FILTER_CONDITION
			TEST_CASE_ASSERT(inClassMembers.Count() == 16);

			auto& outClassMembers = pa.root
				->TryGetChildren_NFb(L"ns")->Get(0).childSymbol
				->GetForwardDecls_N()[1].Cast<NamespaceDeclaration>()->decls;
			TEST_CASE_ASSERT(outClassMembers.Count() == 16);

			for (vint c = 0; c < 4; c++)
			{
				TEST_CATEGORY(L"Category " + itow(c))
				{
					Symbol* primary = nullptr;
					for (vint m = 0; m < 4; m++)
					{
						TEST_CATEGORY(L"Member " + itow(m))
						{
							vint i = c * 4 + m;
							auto inClassDecl = inClassMembers[i];
							auto outClassDecl = outClassMembers[i];

							auto symbol = inClassDecl->symbol->GetFunctionSymbol_Fb();
							TEST_CASE_ASSERT(symbol->kind == symbol_component::SymbolKind::FunctionSymbol);

							TEST_CASE_ASSERT(symbol->GetImplSymbols_F().Count() == 1);
							TEST_CASE_ASSERT(symbol->GetImplSymbols_F()[0]->GetImplDecl_NFb() == outClassDecl);

							TEST_CASE_ASSERT(symbol->GetForwardSymbols_F().Count() == 1);
							TEST_CASE_ASSERT(symbol->GetForwardSymbols_F()[0]->GetForwardDecl_Fb() == inClassDecl);

							if (m == 0)
							{
								primary = symbol;
								TEST_CASE_ASSERT(primary->IsPSPrimary_NF() == true);
								TEST_CASE_ASSERT(primary->GetPSPrimaryVersion_NF() == 3);
								TEST_CASE_ASSERT(primary->GetPSPrimaryDescendants_NF().Count() == 3);
							}
							else
							{
								TEST_CASE_ASSERT(symbol->GetPSPrimary_NF() == primary);
								TEST_CASE_ASSERT(symbol->GetPSParents_NF().Count() == 1);
								TEST_CASE_ASSERT(symbol->GetPSParents_NF()[0] == primary);
							}
						});
					}
				});
			}
		});
	});
}