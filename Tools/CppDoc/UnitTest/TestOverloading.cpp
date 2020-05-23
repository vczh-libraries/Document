#include "Util.h"

#pragma warning (push)
#pragma warning (disable: 5046)

TEST_FILE
{
	TEST_CATEGORY(L"References")
	{
		{
			TEST_DECL(
				struct X {};
				double F(const X&);
				bool F(X&);
				char F(X&&);
				X x;
			);
			COMPILE_PROGRAM(program, pa, input);

			ASSERT_OVERLOADING(F(static_cast<const X&>(x)),		L"F(static_cast<X const &>(x))",		double);
			ASSERT_OVERLOADING(F(static_cast<X&>(x)),			L"F(static_cast<X &>(x))",				bool);
			ASSERT_OVERLOADING(F(static_cast<X&&>(x)),			L"F(static_cast<X &&>(x))",				char);
			ASSERT_OVERLOADING_SIMPLE(F(x),																bool);
		}
	});

	TEST_CATEGORY(L"Arrays")
	{
		{
			TEST_DECL(
				double F(const volatile int*);
				float F(const int*);
				bool F(volatile int*);
				char F(int*);

				extern const volatile int a[1];
				extern const int b[1];
				extern volatile int c[1];
				extern int d[1];
			);
			COMPILE_PROGRAM(program, pa, input);

			ASSERT_OVERLOADING_SIMPLE(F(a),						double);
			ASSERT_OVERLOADING_SIMPLE(F(b),						float);
			ASSERT_OVERLOADING_SIMPLE(F(c),						bool);
			ASSERT_OVERLOADING_SIMPLE(F(d),						char);
		}
		{
			TEST_DECL(
				double F(const volatile int[]);
				float F(const int[]);
				bool F(volatile int[]);
				char F(int[]);

				extern const volatile int a[1];
				extern const int b[1];
				extern volatile int c[1];
				extern int d[1];
			);
			COMPILE_PROGRAM(program, pa, input);

			ASSERT_OVERLOADING_SIMPLE(F(a),						double);
			ASSERT_OVERLOADING_SIMPLE(F(b),						float);
			ASSERT_OVERLOADING_SIMPLE(F(c),						bool);
			ASSERT_OVERLOADING_SIMPLE(F(d),						char);
		}
	});

	TEST_CATEGORY(L"Enums")
	{
		{
			TEST_DECL(
				enum A { a };
				enum B { b };
				enum class C { c };
				enum class D { d };

				bool F(A);
				char F(B);
				wchar_t F(C);
				float F(D);
				double F(int);

				char G(char);
				bool G(int);
				double G(double);
			);
			COMPILE_PROGRAM(program, pa, input);

			ASSERT_OVERLOADING_SIMPLE(F(0),						double);
			ASSERT_OVERLOADING_SIMPLE(F(a),						bool);
			ASSERT_OVERLOADING_SIMPLE(F(b),						char);
			ASSERT_OVERLOADING_SIMPLE(F(A :: a),				bool);
			ASSERT_OVERLOADING_SIMPLE(F(B :: b),				char);
			ASSERT_OVERLOADING_SIMPLE(F(C :: c),				wchar_t);
			ASSERT_OVERLOADING_SIMPLE(F(D :: d),				float);

			ASSERT_OVERLOADING_SIMPLE(G('a'),					char);
			ASSERT_OVERLOADING_SIMPLE(G(0),						bool);
			ASSERT_OVERLOADING_SIMPLE(G(a),						bool);
			ASSERT_OVERLOADING_SIMPLE(G(b),						bool);
			ASSERT_OVERLOADING_SIMPLE(G(0.0),					double);
		}
	});

	TEST_CATEGORY(L"Default parameters")
	{
		{
			TEST_DECL(
				bool F(void*);
				char F(int, int);
				wchar_t F(int, double = 0);
			);
			COMPILE_PROGRAM(program, pa, input);

			ASSERT_OVERLOADING_SIMPLE(F(0),						wchar_t);
			ASSERT_OVERLOADING_SIMPLE(F(nullptr),				bool);
			ASSERT_OVERLOADING_SIMPLE(F(0, 0),					char);
			ASSERT_OVERLOADING_SIMPLE(F(0, 0.0),				wchar_t);
			ASSERT_OVERLOADING_SIMPLE(F(0, 0.0f),				wchar_t);
		}
	});

	TEST_CATEGORY(L"Ellipsis arguments")
	{
		{
			TEST_DECL(
				char F(int, ...);
				wchar_t F(double, ...);
				float F(int, double);
				double F(double, double);
			);
			COMPILE_PROGRAM(program, pa, input);

			ASSERT_OVERLOADING_SIMPLE(F(0, 0),					float);
			ASSERT_OVERLOADING_SIMPLE(F(0, 0.0),				float);
			ASSERT_OVERLOADING_SIMPLE(F(0, 0.0f),				float);

			ASSERT_OVERLOADING_SIMPLE(F(0.0, 0),				double);
			ASSERT_OVERLOADING_SIMPLE(F(0.0, 0.0),				double);
			ASSERT_OVERLOADING_SIMPLE(F(0.0, 0.0f),				double);

			ASSERT_OVERLOADING_SIMPLE(F(0),						char);
			ASSERT_OVERLOADING_SIMPLE(F(0.0),					wchar_t);
			ASSERT_OVERLOADING_SIMPLE(F(0, nullptr),			char);
			ASSERT_OVERLOADING_SIMPLE(F(0.0, nullptr),			wchar_t);
		}
	});

	TEST_CATEGORY(L"Inheritance")
	{
		{
			TEST_DECL(
				struct X {};
				struct Y {};
				struct Z : X {};

				double F(X&);
				bool F(Y&);
				double G(X&);
				bool G(Y&);
				char G(const Z&);
				Z z;
			);
			COMPILE_PROGRAM(program, pa, input);

			ASSERT_OVERLOADING_SIMPLE(F(z),						double);
			ASSERT_OVERLOADING_SIMPLE(G(z),						char);
		}
	});

	TEST_CATEGORY(L"Type conversions")
	{
		{
			TEST_DECL(
				struct X {};
				struct Y {};
				struct Z { operator X() { throw 0; } };

				double F(X);
				bool F(Y);
				double G(X);
				bool G(Y);
				char G(Z);
				Z z;
			);
			COMPILE_PROGRAM(program, pa, input);

			ASSERT_OVERLOADING_SIMPLE(F(z),						double);
			ASSERT_OVERLOADING_SIMPLE(G(z),						char);
		}

		{
			TEST_DECL(
				struct Z {};
				struct X { X(const Z&) {} };
				struct Y {};

				double F(X);
				bool F(Y);
				double G(X);
				bool G(Y);
				char G(Z);
				Z z;
			);
			COMPILE_PROGRAM(program, pa, input);

			ASSERT_OVERLOADING_SIMPLE(F(z),						double);
			ASSERT_OVERLOADING_SIMPLE(G(z),						char);
		}
	});

	TEST_CATEGORY(L"Universal initializations")
	{
		{
			TEST_DECL(
				struct A
				{
					A() {}
					A(int) {}
					A(int, void*) {}
				};

				struct B
				{
					B(void*) {}
					B(void*, int) {}
				};

				struct C
				{
					C(A, B, bool) {}
				};

				struct D
				{
					D(B, A, bool) {}
				};

				struct X
				{
					X() {}
					X(int) {}
					X(double) {}
				};

				struct Y
				{
					Y() {}
					Y(void*) {}
					Y(X) {}
				};

				bool F(const A&);
				void F(B&&);
				float F(A&);
				double F(B&);
				char F(C);
				wchar_t F(D);
				char16_t F(Y, void*);

				A a;
				B b{ nullptr };
			);
			COMPILE_PROGRAM(program, pa, input);

			ASSERT_OVERLOADING_SIMPLE(F({}),											bool);
			ASSERT_OVERLOADING_SIMPLE(F(1),												bool);
			ASSERT_OVERLOADING_SIMPLE(F({1}),											bool);
			ASSERT_OVERLOADING_SIMPLE(F({{1}}),											bool);
			ASSERT_OVERLOADING_SIMPLE(F({1, nullptr}),									bool);
			ASSERT_OVERLOADING_SIMPLE(F({{1}, {nullptr}}),								bool);
			ASSERT_OVERLOADING_SIMPLE(F({1, {}}),										bool);
			ASSERT_OVERLOADING_SIMPLE(F({{1}, {}}),										bool);
			ASSERT_OVERLOADING_SIMPLE(F({{}, nullptr}),									bool);
			ASSERT_OVERLOADING_SIMPLE(F({{}, {nullptr}}),								bool);

			ASSERT_OVERLOADING_SIMPLE(F({nullptr}),										void);
			ASSERT_OVERLOADING_SIMPLE(F({nullptr, 1}),									void);
			ASSERT_OVERLOADING_SIMPLE(F({{nullptr}, {1}}),								void);
			ASSERT_OVERLOADING_SIMPLE(F({nullptr, {}}),									void);
			ASSERT_OVERLOADING_SIMPLE(F({{nullptr}, {}}),								void);
			ASSERT_OVERLOADING_SIMPLE(F({{}, 1}),										void);
			ASSERT_OVERLOADING_SIMPLE(F({{}, {1}}),										void);

			ASSERT_OVERLOADING_SIMPLE(F(a),												float);
			ASSERT_OVERLOADING_SIMPLE(F(b),												double);

			ASSERT_OVERLOADING_SIMPLE(F({{}, nullptr, true}),							char);
			ASSERT_OVERLOADING_SIMPLE(F({{}, {nullptr, 1}, true}),						char);
			ASSERT_OVERLOADING_SIMPLE(F({{{1}, {}}, {{}, {1}}, true}),					char);
			ASSERT_OVERLOADING_SIMPLE(F({{{}, {nullptr}}, {{nullptr}, {}}, true}),		char);

			ASSERT_OVERLOADING_SIMPLE(F({nullptr, {}, true}),							wchar_t);
			ASSERT_OVERLOADING_SIMPLE(F({{nullptr, 1}, {}, true}),						wchar_t);
			ASSERT_OVERLOADING_SIMPLE(F({{{}, {1}}, {{1}, {}}, true}),					wchar_t);
			ASSERT_OVERLOADING_SIMPLE(F({{{nullptr}, {}}, {{}, {nullptr}}, true}),		wchar_t);

			ASSERT_OVERLOADING_SIMPLE(F({}, nullptr),									char16_t);
			ASSERT_OVERLOADING_SIMPLE(F(nullptr, nullptr),								char16_t);
			ASSERT_OVERLOADING_SIMPLE(F({nullptr}, nullptr),							char16_t);
			ASSERT_OVERLOADING_SIMPLE(F({{nullptr}}, nullptr),							char16_t);
			ASSERT_OVERLOADING_SIMPLE(F({{}}, nullptr),									char16_t);
			ASSERT_OVERLOADING_SIMPLE(F({{1}}, nullptr),								char16_t);
			ASSERT_OVERLOADING_SIMPLE(F({{1.0}}, nullptr),								char16_t);
			ASSERT_OVERLOADING_SIMPLE(F({{{1}}}, nullptr),								char16_t);
			ASSERT_OVERLOADING_SIMPLE(F({{{1.0}}}, nullptr),							char16_t);
		}
	});

	TEST_CATEGORY(L"Universal initializations + type conversions: Exact")
	{
		TEST_DECL(
			struct S
			{
				S(int) {}
			};

			bool F(int);
			char F(S);
		);
		COMPILE_PROGRAM(program, pa, input);
		ASSERT_OVERLOADING_SIMPLE(F(0),				bool);
		ASSERT_OVERLOADING_SIMPLE(F({0}),			bool);
	});

	TEST_CATEGORY(L"Universal initializations + type conversions: Trivial")
	{
		TEST_DECL(
			struct S
			{
				S(int) {}
			};

			bool F(const int&);
			char F(S);
		);
		COMPILE_PROGRAM(program, pa, input);
		ASSERT_OVERLOADING_SIMPLE(F(0),				bool);
		ASSERT_OVERLOADING_SIMPLE(F({0}),			bool);
	});

	TEST_CATEGORY(L"Universal initializations + type conversions: IntegralPromition")
	{
		TEST_DECL(
			struct S
			{
				S(int) {}
			};

			bool F(long long);
			char F(S);
		);
		COMPILE_PROGRAM(program, pa, input);
		ASSERT_OVERLOADING_SIMPLE(F(0),				bool);
		ASSERT_OVERLOADING_SIMPLE(F({0}),			bool);
	});

	TEST_CATEGORY(L"Universal initializations + type conversions: Standard")
	{
		TEST_DECL(
			struct S
			{
				S(int) {}
			};

			bool F(double);
			char F(S);
		);
		COMPILE_PROGRAM(program, pa, input);
		ASSERT_OVERLOADING_SIMPLE(F(0),				bool);
		ASSERT_OVERLOADING_SIMPLE(F({0}),			bool);
	});

	TEST_CATEGORY(L"Universal initializations + type conversions: UserDefined")
	{
		TEST_DECL(
			struct S
			{
				S(int) {}
			};

			bool F(...);
			char F(S);
		);
		COMPILE_PROGRAM(program, pa, input);
		ASSERT_OVERLOADING_SIMPLE(F(0),				char);
		ASSERT_OVERLOADING_SIMPLE(F({0}),			char);
	});

	TEST_CATEGORY(L"Initialization list")
	{
		// TODO:
	});

	TEST_CATEGORY(L"Overloading priority")
	{
		// TODO:
		// Mix
		//   invoke operator(CallOP)
		//   constructor(Ctor)
		//   universal initialization(UI)
		//   default paramter and variant argument together(DPVA)
		//   template function
	});
}

#pragma warning (pop)