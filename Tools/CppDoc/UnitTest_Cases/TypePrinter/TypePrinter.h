#pragma once

namespace type_printer
{
	template<typename T>
	struct TypePrinter;

	template<typename T>
	void PrintType()
	{
		TypePrinter<T>::PrintPrefix(true);
		TypePrinter<T>::PrintPostfix(true);
		printf("\n");
	}

	template<typename T>
	struct WithPrintPostfix
	{
		static void PrintPostfix(bool)
		{
			TypePrinter<T>::PrintPostfix(false);
		}
	};

	struct WithoutPrintPostfix
	{
		static void PrintPostfix(bool) {}
	};

	template<>
	struct TypePrinter<bool> : WithoutPrintPostfix
	{
		static void PrintPrefix(bool top) { printf("bool"); }
	};

	template<>
	struct TypePrinter<char> : WithoutPrintPostfix
	{
		static void PrintPrefix(bool top) { printf("char"); }
	};

	template<>
	struct TypePrinter<int> : WithoutPrintPostfix
	{
		static void PrintPrefix(bool top) { printf("int"); }
	};

	template<>
	struct TypePrinter<float> : WithoutPrintPostfix
	{
		static void PrintPrefix(bool top) { printf("float"); }
	};

	template<>
	struct TypePrinter<double> : WithoutPrintPostfix
	{
		static void PrintPrefix(bool top) { printf("double"); }
	};

	template<typename T>
	struct TypePrinter<T*> : WithPrintPostfix<T>
	{
		static void PrintPrefix(bool top)
		{ 
			TypePrinter<T>::PrintPrefix(false);
			printf(" *");
		}
	};

	template<typename T>
	struct TypePrinter<T&> : WithPrintPostfix<T>
	{
		static void PrintPrefix(bool top)
		{
			TypePrinter<T>::PrintPrefix(false);
			printf(" &");
		}
	};

	template<typename T>
	struct TypePrinter<T&&> : WithPrintPostfix<T>
	{
		static void PrintPrefix(bool top)
		{
			TypePrinter<T>::PrintPrefix(false);
			printf(" &&");
		}
	};

	template<typename T>
	struct TypePrinter<T const> : WithPrintPostfix<T>
	{
		static void PrintPrefix(bool top)
		{
			TypePrinter<T>::PrintPrefix(false);
			printf(" const");
		}
	};

	template<typename T>
	struct TypePrinter<T volatile> : WithPrintPostfix<T>
	{
		static void PrintPrefix(bool top)
		{
			TypePrinter<T>::PrintPrefix(false);
			printf(" volatile");
		}
	};

	template<typename T, int Size>
	struct TypePrinter<T[Size]>
	{
		static void PrintPrefix(bool top)
		{
			TypePrinter<T>::PrintPrefix(false);
			if (!top) printf("(");
		}

		static void PrintPostfix(bool top)
		{
			if (!top) printf(")");
			printf("[%d]", Size);
			TypePrinter<T>::PrintPostfix(false);
		}
	};

	template<typename T, int Size>
	struct TypePrinter<const T[Size]> : TypePrinter<T[Size]>
	{
		static void PrintPrefix(bool top)
		{
			TypePrinter<const T>::PrintPrefix(false);
			if (!top) printf("(");
		}
	};

	template<typename T, int Size>
	struct TypePrinter<volatile T[Size]> : TypePrinter<T[Size]>
	{
		static void PrintPrefix(bool top)
		{
			TypePrinter<volatile T>::PrintPrefix(false);
			if (!top) printf("(");
		}
	};

	template<typename... Ps>
	struct TypeListPrinter;

	template<>
	struct TypeListPrinter<>
	{
		static void PrintTypeList() {}
	};

	template<typename T>
	struct TypeListPrinter<T>
	{
		static void PrintTypeList()
		{
			TypePrinter<T>::PrintPrefix(true);
			TypePrinter<T>::PrintPostfix(true);
		}
	};

	template<typename T, typename... Ts>
	struct TypeListPrinter<T, Ts...>
	{
		static void PrintTypeList()
		{
			TypePrinter<T>::PrintPrefix(true);
			TypePrinter<T>::PrintPostfix(true);
			printf(", ");
			TypeListPrinter<Ts...>::PrintTypeList();
		}
	};

	template<typename T, typename... Ps>
	struct TypePrinter<T(Ps...)>
	{
		static void PrintPrefix(bool top)
		{
			TypePrinter<T>::PrintPrefix(false);
			if (!top) printf("(");
		}

		static void PrintPostfix(bool top)
		{
			if (!top) printf(")");
			printf("(");
			TypeListPrinter<Ps...>::PrintTypeList();
			printf(")");
			TypePrinter<T>::PrintPostfix(false);
		}
	};
}