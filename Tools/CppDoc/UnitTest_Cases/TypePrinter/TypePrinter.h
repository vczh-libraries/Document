#pragma once

namespace type_printer
{
	template<typename T>
	struct TypePrinter;

	template<typename T>
	void PrintType()
	{
		TypePrinter<T>::PrintPrefix(nullptr);
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

	// primitive types

	template<typename T>
	struct PrimitiveTypePrinter
	{
		static void PrintPrefix(const char* delimiter)
		{
			printf(T::Name);
			if (delimiter) printf(delimiter);
		}

		static void PrintPostfix(bool)
		{
		}
	};

#define MAKE_PRIMITIVE_TYPE(NAME)											\
	template<>																\
	struct TypePrinter<NAME> : PrimitiveTypePrinter<TypePrinter<NAME>>		\
	{																		\
		static const char* const Name;										\
	};																		\
	const char* const TypePrinter<NAME>::Name = #NAME						\

	MAKE_PRIMITIVE_TYPE(bool);
	MAKE_PRIMITIVE_TYPE(char);
	MAKE_PRIMITIVE_TYPE(int);
	MAKE_PRIMITIVE_TYPE(float);
	MAKE_PRIMITIVE_TYPE(double);

#undef MAKE_PRIMITIVE_TYPE

	// references

	template<typename T>
	struct TypePrinter<T*> : WithPrintPostfix<T>
	{
		static void PrintPrefix(const char* delimiter)
		{ 
			TypePrinter<T>::PrintPrefix("");
			printf("*");
		}
	};

	template<typename T>
	struct TypePrinter<T&> : WithPrintPostfix<T>
	{
		static void PrintPrefix(const char* delimiter)
		{
			TypePrinter<T>::PrintPrefix("");
			printf("&");
		}
	};

	template<typename T>
	struct TypePrinter<T&&> : WithPrintPostfix<T>
	{
		static void PrintPrefix(const char* delimiter)
		{
			TypePrinter<T>::PrintPrefix("");
			printf("&&");
		}
	};

	// qualifieds

	template<typename T>
	struct TypePrinter<T const> : WithPrintPostfix<T>
	{
		static void PrintPrefix(const char* delimiter)
		{
			TypePrinter<T>::PrintPrefix(" ");
			printf("const");
			if (delimiter) printf(delimiter);
		}
	};

	template<typename T>
	struct TypePrinter<T volatile> : WithPrintPostfix<T>
	{
		static void PrintPrefix(const char* delimiter)
		{
			TypePrinter<T>::PrintPrefix(" ");
			printf("volatile");
			if (delimiter) printf(delimiter);
		}
	};

	template<typename T>
	struct TypePrinter<T const volatile> : WithPrintPostfix<T>
	{
		static void PrintPrefix(const char* delimiter)
		{
			TypePrinter<T>::PrintPrefix(" ");
			printf("const volatile");
			if (delimiter) printf(delimiter);
		}
	};

	// arrays

	template<typename T, int Size>
	struct ArrayTypePrinter
	{
		static void PrintPrefix(const char* delimiter)
		{
			TypePrinter<T>::PrintPrefix("");
			if (delimiter) printf("(");
		}

		static void PrintPostfix(bool top)
		{
			if (!top) printf(")");
			printf("[%d]", Size);
			TypePrinter<T>::PrintPostfix(false);
		}
	};

	template<typename T, int Size>
	struct TypePrinter<T[Size]> : ArrayTypePrinter<T, Size>
	{
	};

	template<typename T, int Size>
	struct TypePrinter<const T[Size]> : ArrayTypePrinter<const T, Size>
	{
	};

	template<typename T, int Size>
	struct TypePrinter<volatile T[Size]> : ArrayTypePrinter<volatile T, Size>
	{
	};

	template<typename T, int Size>
	struct TypePrinter<const volatile T[Size]> : ArrayTypePrinter<const volatile T, Size>
	{
	};

	// functions

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
			TypePrinter<T>::PrintPrefix(nullptr);
			TypePrinter<T>::PrintPostfix(true);
		}
	};

	template<typename T, typename... Ts>
	struct TypeListPrinter<T, Ts...>
	{
		static void PrintTypeList()
		{
			TypePrinter<T>::PrintPrefix(nullptr);
			TypePrinter<T>::PrintPostfix(true);
			printf(", ");
			TypeListPrinter<Ts...>::PrintTypeList();
		}
	};

	template<typename T, typename... Ps>
	struct TypePrinter<T(Ps...)>
	{
		static void PrintPrefix(const char* delimiter)
		{
			TypePrinter<T>::PrintPrefix(false);
			if (delimiter) printf("(");
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