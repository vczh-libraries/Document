#include "Util.h"

TEST_FILE
{
	TEST_CATEGORY(L"Enums")
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

		TEST_CASE(L"Checking connections")
		{
			TEST_ASSERT(pa.root->TryGetChildren_NFb(L"a")->Count() == 1);
			auto _a = pa.root->TryGetChildren_NFb(L"a")->Get(0).childSymbol;
			TEST_ASSERT(_a->TryGetChildren_NFb(L"b")->Count() == 1);
			auto _b = _a->TryGetChildren_NFb(L"b")->Get(0).childSymbol;
			TEST_ASSERT(_b->TryGetChildren_NFb(L"A")->Count() == 1);
			auto _A = _b->TryGetChildren_NFb(L"A")->Get(0).childSymbol.Obj();

			TEST_ASSERT(_A->kind == symbol_component::SymbolKind::Enum);
			TEST_ASSERT(_A->GetImplDecl_NFb<EnumDeclaration>());
			TEST_ASSERT(_A->GetForwardDecls_N().Count() == 4);
			TEST_ASSERT(From(_A->GetForwardDecls_N()).Distinct().Count() == 4);
			for (vint i = 0; i < 4; i++)
			{
				TEST_ASSERT(_A->GetForwardDecls_N()[i].Cast<ForwardEnumDeclaration>());
				TEST_ASSERT(!_A->GetForwardDecls_N()[i].Cast<EnumDeclaration>());
			}
		});
	});

	TEST_CATEGORY(L"Variables")
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

		TEST_CASE(L"Checking connections")
		{
			TEST_ASSERT(pa.root->TryGetChildren_NFb(L"a")->Count() == 1);
			auto _a = pa.root->TryGetChildren_NFb(L"a")->Get(0).childSymbol;
			TEST_ASSERT(_a->TryGetChildren_NFb(L"b")->Count() == 1);
			auto _b = _a->TryGetChildren_NFb(L"b")->Get(0).childSymbol;
			TEST_ASSERT(_b->TryGetChildren_NFb(L"A")->Count() == 1);
			auto _x = _b->TryGetChildren_NFb(L"x")->Get(0).childSymbol.Obj();

			TEST_ASSERT(_x->kind == symbol_component::SymbolKind::Variable);
			TEST_ASSERT(_x->GetImplDecl_NFb<VariableDeclaration>());
			TEST_ASSERT(_x->GetForwardDecls_N().Count() == 4);
			TEST_ASSERT(From(_x->GetForwardDecls_N()).Distinct().Count() == 4);
			for (vint i = 0; i < 4; i++)
			{
				TEST_ASSERT(_x->GetForwardDecls_N()[i].Cast<ForwardVariableDeclaration>());
				TEST_ASSERT(!_x->GetForwardDecls_N()[i].Cast<VariableDeclaration>());
			}
		});
	});

	TEST_CATEGORY(L"Functions")
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

		TEST_CASE(L"Checking connections")
		{
			TEST_ASSERT(pa.root->TryGetChildren_NFb(L"a")->Count() == 1);
			auto _a = pa.root->TryGetChildren_NFb(L"a")->Get(0).childSymbol;
			TEST_ASSERT(_a->TryGetChildren_NFb(L"b")->Count() == 1);
			auto _b = _a->TryGetChildren_NFb(L"b")->Get(0).childSymbol;
			TEST_ASSERT(_b->TryGetChildren_NFb(L"A")->Count() == 1);
			auto _Add = _b->TryGetChildren_NFb(L"Add")->Get(0).childSymbol.Obj();

			TEST_ASSERT(_Add->kind == symbol_component::SymbolKind::FunctionSymbol);
			TEST_ASSERT(_Add->GetImplSymbols_F().Count() == 1);
			TEST_ASSERT(_Add->GetImplSymbols_F()[0]->GetImplDecl_NFb<FunctionDeclaration>());
			TEST_ASSERT(_Add->GetForwardSymbols_F().Count() == 4);
			for (vint i = 0; i < 4; i++)
			{
				TEST_ASSERT(_Add->GetForwardSymbols_F()[i]->GetForwardDecl_Fb().Cast<ForwardFunctionDeclaration>());
				TEST_ASSERT(!_Add->GetForwardSymbols_F()[i]->GetImplDecl_NFb<FunctionDeclaration>());
			}
		});
	});

	TEST_CATEGORY(L"Classes")
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

			TEST_CASE(L"Checking connections")
			{
				TEST_ASSERT(pa.root->TryGetChildren_NFb(L"a")->Count() == 1);
				auto _a = pa.root->TryGetChildren_NFb(L"a")->Get(0).childSymbol;
				TEST_ASSERT(_a->TryGetChildren_NFb(L"b")->Count() == 1);
				auto _b = _a->TryGetChildren_NFb(L"b")->Get(0).childSymbol;
				TEST_ASSERT(_b->TryGetChildren_NFb(L"A")->Count() == 1);
				auto _X = _b->TryGetChildren_NFb(L"X")->Get(0).childSymbol.Obj();

				switch (i)
				{
				case 0: TEST_ASSERT(_X->kind == symbol_component::SymbolKind::Class); break;
				case 1: TEST_ASSERT(_X->kind == symbol_component::SymbolKind::Struct); break;
				case 2: TEST_ASSERT(_X->kind == symbol_component::SymbolKind::Union); break;
				}
				TEST_ASSERT(_X->GetImplDecl_NFb<ClassDeclaration>());
				TEST_ASSERT(_X->GetForwardDecls_N().Count() == 4);
				TEST_ASSERT(From(_X->GetForwardDecls_N()).Distinct().Count() == 4);
				for (vint i = 0; i < 4; i++)
				{
					TEST_ASSERT(_X->GetForwardDecls_N()[i].Cast<ForwardClassDeclaration>());
					TEST_ASSERT(!_X->GetForwardDecls_N()[i].Cast<ClassDeclaration>());
				}
			});
		}
	});

	TEST_CATEGORY(L"Class members")
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

		TEST_CASE(L"Checking connections")
		{
			using Item = Tuple<CppClassAccessor, Ptr<Declaration>>;
			List<Ptr<Declaration>> inClassMembers;
			auto& inClassMembersUnfiltered = pa.root
				->TryGetChildren_NFb(L"a")->Get(0).childSymbol
				->TryGetChildren_NFb(L"b")->Get(0).childSymbol
				->TryGetChildren_NFb(L"Something")->Get(0).childSymbol
				->GetImplDecl_NFb<ClassDeclaration>()->decls;
			CopyFrom(inClassMembers, From(inClassMembersUnfiltered).Where([](Item item) {return !item.f1->implicitlyGeneratedMember; }).Select([](Item item) { return item.f1; }));
			TEST_ASSERT(inClassMembers.Count() == 13);

			auto& outClassMembers = pa.root
				->TryGetChildren_NFb(L"a")->Get(0).childSymbol
				->TryGetChildren_NFb(L"b")->Get(0).childSymbol
				->GetForwardDecls_N()[1].Cast<NamespaceDeclaration>()->decls;
			TEST_ASSERT(outClassMembers.Count() == 12);

			for (vint i = 0; i < 12; i++)
			{
				auto inClassDecl = inClassMembers[i + 1];
				auto outClassDecl = outClassMembers[i];

				if (i == 0)
				{
					auto symbol = inClassDecl->symbol;
					TEST_ASSERT(symbol == outClassDecl->symbol);
					TEST_ASSERT(symbol->kind == symbol_component::SymbolKind::Variable);

					TEST_ASSERT(symbol->GetImplDecl_NFb() == outClassDecl);
					TEST_ASSERT(symbol->GetForwardDecls_N().Count() == 1);
					TEST_ASSERT(symbol->GetForwardDecls_N()[0] == inClassDecl);
				}
				else
				{
					auto symbol = inClassDecl->symbol->GetFunctionSymbol_Fb();
					TEST_ASSERT(symbol->kind == symbol_component::SymbolKind::FunctionSymbol);

					TEST_ASSERT(symbol->GetImplSymbols_F().Count() == 1);
					TEST_ASSERT(symbol->GetImplSymbols_F()[0]->GetImplDecl_NFb() == outClassDecl);

					TEST_ASSERT(symbol->GetForwardSymbols_F().Count() == 1);
					TEST_ASSERT(symbol->GetForwardSymbols_F()[0]->GetForwardDecl_Fb() == inClassDecl);
				}
			}
		});
	});
}