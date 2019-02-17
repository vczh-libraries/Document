#include "Ast.h"
#include "Ast_Decl.h"
#include "Ast_Type.h"
#include "Parser.h"

namespace symbol_type_resolving
{
	extern bool				AddInternal(ExprTsysList& list, const ExprTsysItem& item);
	extern void				AddInternal(ExprTsysList& list, ExprTsysList& items);
	extern bool				AddVar(ExprTsysList& list, const ExprTsysItem& item);
	extern bool				AddNonVar(ExprTsysList& list, const ExprTsysItem& item);
	extern void				AddNonVar(ExprTsysList& list, ExprTsysList& items);
	extern bool				AddTemp(ExprTsysList& list, ITsys* tsys);
	extern void				AddTemp(ExprTsysList& list, TypeTsysList& items);

	extern void				CalculateValueFieldType(const ExprTsysItem* thisItem, Symbol* symbol, ITsys* fieldType, bool forFieldDeref, ExprTsysList& result);
	extern void				VisitSymbol(ParsingArguments& pa, const ExprTsysItem* thisItem, Symbol* symbol, bool afterScope, ExprTsysList& result);


	extern TsysConv			FindMinConv(ArrayBase<TsysConv>& funcChoices);
	extern void				FilterFunctionByConv(ExprTsysList& funcTypes, ArrayBase<TsysConv>& funcChoices);
	extern void				FilterFieldsAndBestQualifiedFunctions(TsysCV thisCV, TsysRefType thisRef, ExprTsysList& funcTypes);
	extern void				FindQualifiedFunctors(ParsingArguments& pa, TsysCV thisCV, TsysRefType thisRef, ExprTsysList& funcTypes, bool lookForOp);

	extern void				VisitResolvedMember(ParsingArguments& pa, const ExprTsysItem* thisItem, Ptr<Resolving> resolving, ExprTsysList& result);
	extern void				VisitDirectField(ParsingArguments& pa, ResolveSymbolResult& totalRar, const ExprTsysItem& parentItem, CppName& name, ExprTsysList& result);
	extern void				VisitFunctors(ParsingArguments& pa, const ExprTsysItem& parentItem, const WString& name, ExprTsysList& result);
	extern void				VisitOverloadedFunction(ParsingArguments& pa, ExprTsysList& funcTypes, List<Ptr<ExprTsysList>>& argTypesList, ExprTsysList& result);

	extern void				SearchAdlClassesAndNamespaces(ParsingArguments& pa, ITsys* type, SortedList<Symbol*>& nss, SortedList<Symbol*>& classes);
	extern void				SearchAdlClassesAndNamespaces(ParsingArguments& pa, TypeTsysList& types, SortedList<Symbol*>& nss, SortedList<Symbol*>& classes);
	extern void				SearchAdlClassesAndNamespaces(ParsingArguments& pa, ExprTsysList& types, SortedList<Symbol*>& nss, SortedList<Symbol*>& classes);

	extern void				Promote(TsysPrimitive& primitive);
	extern TsysPrimitive	ArithmeticConversion(TsysPrimitive leftP, TsysPrimitive rightP);
}