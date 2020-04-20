#ifndef VCZH_DOCUMENT_CPPDOC_AST_RESOLVING
#define VCZH_DOCUMENT_CPPDOC_AST_RESOLVING

#include "Ast.h"
#include "Ast_Decl.h"
#include "Ast_Type.h"
#include "Parser.h"

namespace symbol_type_resolving
{
	extern bool									AddInternal(ExprTsysList& list, const ExprTsysItem& item);
	extern void									AddInternal(ExprTsysList& list, ExprTsysList& items);
	extern bool									AddVar(ExprTsysList& list, const ExprTsysItem& item);
	extern bool									AddNonVar(ExprTsysList& list, const ExprTsysItem& item);
	extern void									AddNonVar(ExprTsysList& list, ExprTsysList& items);
	extern bool									AddTempValue(ExprTsysList& list, ITsys* tsys);
	extern void									AddTempValue(ExprTsysList& list, TypeTsysList& items);
	extern bool									AddType(ExprTsysList& list, ITsys* tsys);
	extern void									AddType(ExprTsysList& list, TypeTsysList& items);

	extern void									CreateUniversalInitializerType(const ParsingArguments& pa, Array<ExprTsysList>& argTypesList, ExprTsysList& result);
	extern void									CalculateValueFieldType(const ExprTsysItem* thisItem, Symbol* symbol, ITsys* fieldType, bool forFieldDeref, ExprTsysList& result);

	extern void									Promote(TsysPrimitive& primitive);
	extern TsysPrimitive						ArithmeticConversion(TsysPrimitive leftP, TsysPrimitive rightP);

	// Overloading

	extern void									FilterFieldsAndBestQualifiedFunctions(TsysCV thisCV, TsysRefType thisRef, ExprTsysList& funcTypes);
	extern void									FindQualifiedFunctors(const ParsingArguments& pa, TsysCV thisCV, TsysRefType thisRef, ExprTsysList& funcTypes, bool lookForOp);
	extern void									VisitOverloadedFunction(const ParsingArguments& pa, ExprTsysList& funcTypes, Array<ExprTsysItem>& argTypes, SortedList<vint>& boundedAnys, ExprTsysList& result, ExprTsysList* selectedFunctions = nullptr, bool* anyInvolved = nullptr);

	// Adl

	extern bool									IsAdlEnabled(const ParsingArguments& pa, Ptr<Resolving> resolving);
	extern void									SearchAdlClassesAndNamespaces(const ParsingArguments& pa, Symbol* symbol, SortedList<Symbol*>& nss, SortedList<Symbol*>& classes);
	extern void									SearchAdlClassesAndNamespaces(const ParsingArguments& pa, ITsys* type, SortedList<Symbol*>& nss, SortedList<Symbol*>& classes);
	extern void									SearchAdlFunction(const ParsingArguments& pa, SortedList<Symbol*>& nss, const WString& name, ExprTsysList& result);
}

#endif