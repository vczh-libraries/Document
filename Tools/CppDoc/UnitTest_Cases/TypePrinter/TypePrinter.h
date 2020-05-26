#pragma once

#include <stdio.h>

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

	// primitive types

	template<typename TThis>
	struct PrimitiveTypePrinter
	{
		static void PrintPrefix(const char* delimiter)
		{
			printf(TThis::Name);
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

	template<typename TThis, typename T>
	struct ReferenceTypePrinter : WithPrintPostfix<T>
	{
		static void PrintPrefix(const char* delimiter)
		{
			TypePrinter<T>::PrintPrefix("");
			printf(TThis::Reference);
			if (delimiter) printf(delimiter);
		}

		static void PrintPostfix(bool)
		{
			TypePrinter<T>::PrintPostfix(false);
		}
	};

#define MAKE_REFERENCE_TYPE(REFERENCE)														\
	template<typename T>																	\
	struct TypePrinter<T REFERENCE> : ReferenceTypePrinter<TypePrinter<T REFERENCE>, T>		\
	{																						\
		static const char* const Reference;													\
	};																						\
	template<typename T>																	\
	const char* const TypePrinter<T REFERENCE>::Reference = #REFERENCE						\

	MAKE_REFERENCE_TYPE(*);
	MAKE_REFERENCE_TYPE(&);
	MAKE_REFERENCE_TYPE(&&);

#undef MAKE_REFERENCE_TYPE

	// qualifieds

	template<typename TThis, typename T>
	struct QualifiedTypePrinter : WithPrintPostfix<T>
	{
		static void PrintPrefix(const char* delimiter)
		{
			TypePrinter<T>::PrintPrefix(" ");
			printf(TThis::Qualifier);
			if (delimiter) printf(delimiter);
		}

		static void PrintPostfix(bool)
		{
			TypePrinter<T>::PrintPostfix(false);
		}
	};

#define MAKE_QUALIFIED_TYPE(QUALIFIER)														\
	template<typename T>																	\
	struct TypePrinter<T QUALIFIER> : QualifiedTypePrinter<TypePrinter<T QUALIFIER>, T>		\
	{																						\
		static const char* const Qualifier;													\
	};																						\
	template<typename T>																	\
	const char* const TypePrinter<T QUALIFIER>::Qualifier = #QUALIFIER						\

	MAKE_QUALIFIED_TYPE(const);
	MAKE_QUALIFIED_TYPE(volatile);
	MAKE_QUALIFIED_TYPE(const volatile);

#undef MAKE_QUALIFIED_TYPE

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

#define MAKE_ARRAY_TYPE(QUALIFIER)											\
	template<typename T, int Size>											\
	struct TypePrinter<T QUALIFIER[Size]> : ArrayTypePrinter<T, Size>		\
	{																		\
	}																		\

#pragma warning (push)
#pragma warning (disable: 4003)
	MAKE_ARRAY_TYPE();
#pragma warning (pop)
	MAKE_ARRAY_TYPE(const);
	MAKE_ARRAY_TYPE(volatile);
	MAKE_ARRAY_TYPE(const volatile);

#undef MAKE_ARRAY_TYPE

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