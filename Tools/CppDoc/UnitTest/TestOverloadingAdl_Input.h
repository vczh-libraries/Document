namespace test_overloading_adl
{
	namespace x
	{
		struct A
		{
			int w;

			struct B
			{
				int w;
			};
		};
	}

	namespace y
	{
		struct C
		{
			int w;

			struct D : x::A::B
			{
				int w;
			};
		};
	}

	namespace z
	{
		struct E : x::A::B
		{
			int w;

			struct F
			{
				int w;
			};
		};
	}

	namespace x
	{
		extern double F(const y::C::D&);
		extern double F(const z::E::F&);
		extern void pF(void(*)(A));
		extern void pF(void(*)(A::B));
		extern float pG(int A::*);
		extern float pG(int A::B::*);
	}

	namespace y
	{
		extern bool F(...);
		extern void pF(void(*)(C));
		extern void pF(void(*)(C::D));
		extern float pG(int C::*);
		extern float pG(int C::D::*);
	}

	namespace z
	{
		extern bool F(...);
		extern void pF(void(*)(E));
		extern void pF(void(*)(E::F));
		extern float pG(int E::*);
		extern float pG(int E::F::*);
	}

	void(*pFA)(x::A);
	void(*pFB)(x::A::B);
	void(*pFC)(y::C);
	void(*pFD)(y::C::D);
	void(*pFE)(z::E);
	void(*pFF)(z::E::F);
}