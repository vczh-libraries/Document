#ifndef VCZH_DOCUMENT_CPPDOC_SYMBOL_RESOLVE
#define VCZH_DOCUMENT_CPPDOC_SYMBOL_RESOLVE

#include "Ast.h"

/***********************************************************************
Resolving Functions
***********************************************************************/

enum class SearchPolicy
{
	SymbolAccessableInScope,							// search a name in a bounding scope of the current checking place
	SymbolAccessableInScope_CStyleTypeReference,		// like above but it is explicitly required to be a type using "struct X"
	DirectChildSymbolFromOutside,						// search scope::name, not touching symbols in base types
	ChildSymbolFromOutside,								// search scope::name
	ChildSymbolFromSubClass,							// search the base class from the following two policy
	ChildSymbolFromMemberInside,						// search a name in a class containing the current member, when the member is declared inside the class
	ChildSymbolFromMemberOutside,						// search a name in a class containing the current member, when the member is declared outside the class
};

struct ResolveSymbolResult
{
	Ptr<Resolving>									values;
	Ptr<Resolving>									types;

	void											Merge(Ptr<Resolving>& to, Ptr<Resolving> from);
	void											Merge(const ResolveSymbolResult& rar);
};

extern ResolveSymbolResult							ResolveSymbol(const ParsingArguments& pa, CppName& name, SearchPolicy policy);
extern ResolveSymbolResult							ResolveChildSymbol(const ParsingArguments& pa, Ptr<Type> classType, CppName& name);
extern ResolveSymbolResult							ResolveDirectChildSymbol(const ParsingArguments& pa, CppName& name);

#endif