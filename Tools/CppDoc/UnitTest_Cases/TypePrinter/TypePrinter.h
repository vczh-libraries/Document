#pragma once

#include <stdio.h>

namespace type_printer
{
	template<typename T>
	struct TypePrinter;

	enum class TypeSource
	{
		TopLevel,
		Reference,
		Qualified,
		Array,
		Function,
	};

	template<typename T>
	void PrintType(bool lineBreak = true)
	{
		TypePrinter<T>::PrintPrefix(TypeSource::TopLevel);
		TypePrinter<T>::PrintPostfix(TypeSource::TopLevel);
		if (lineBreak) printf("\n");
	}

	// primitive types

	template<typename TThis>
	struct PrimitiveTypePrinter
	{
		static void PrintPrefix(TypeSource source)
		{
			printf(TThis::Name);
			if (source == TypeSource::Qualified) printf(" ");
		}

		static void PrintPostfix(TypeSource source)
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
	struct ReferenceTypePrinter
	{
		static void PrintPrefix(TypeSource source)
		{
			TypePrinter<T>::PrintPrefix(TypeSource::Reference);
			printf(TThis::Reference);
		}

		static void PrintPostfix(TypeSource source)
		{
			TypePrinter<T>::PrintPostfix(TypeSource::Reference);
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
	struct QualifiedTypePrinter
	{
		static void PrintPrefix(TypeSource source)
		{
			TypePrinter<T>::PrintPrefix(TypeSource::Qualified);
			printf(TThis::Qualifier);
		}

		static void PrintPostfix(TypeSource source)
		{
			TypePrinter<T>::PrintPostfix(TypeSource::Qualified);
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
		static void PrintPrefix(TypeSource source)
		{
			TypePrinter<T>::PrintPrefix(TypeSource::Array);
			if (source != TypeSource::TopLevel && source != TypeSource::Array) printf("(");
		}

		static void PrintPostfix(TypeSource source)
		{
			if (source != TypeSource::TopLevel && source != TypeSource::Array) printf(")");
			printf("[%d]", Size);
			TypePrinter<T>::PrintPostfix(TypeSource::Array);
		}
	};

#define MAKE_ARRAY_TYPE(QUALIFIER)															\
	template<typename T, int Size>															\
	struct TypePrinter<T QUALIFIER[Size]> : ArrayTypePrinter<T QUALIFIER, Size>				\
	{																						\
	}																						\

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
			PrintType<T>(false);
		}
	};

	template<typename T, typename... Ts>
	struct TypeListPrinter<T, Ts...>
	{
		static void PrintTypeList()
		{
			PrintType<T>(false);
			printf(", ");
			TypeListPrinter<Ts...>::PrintTypeList();
		}
	};

	template<typename T, typename... Ps>
	struct TypePrinter<T(Ps...)>
	{
		static void PrintPrefix(TypeSource source)
		{
			TypePrinter<T>::PrintPrefix(TypeSource::Function);
			if (source != TypeSource::TopLevel) printf("(");
		}

		static void PrintPostfix(TypeSource source)
		{
			if (source != TypeSource::TopLevel) printf(")");
			printf("(");
			TypeListPrinter<Ps...>::PrintTypeList();
			printf(")");
			TypePrinter<T>::PrintPostfix(TypeSource::Function);
		}
	};
}