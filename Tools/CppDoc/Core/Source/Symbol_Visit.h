#ifndef VCZH_DOCUMENT_CPPDOC_SYMBOL_VISIT
#define VCZH_DOCUMENT_CPPDOC_SYMBOL_VISIT

#include "Symbol_Resolve.h"

namespace symbol_type_resolving
{
	extern ExprTsysItem							AdjustThisItemForSymbol(const ParsingArguments& pa, ExprTsysItem thisItem, ResolvedItem item);
	extern void									VisitSymbol(const ParsingArguments& pa, ResolvedItem item, ExprTsysList& result);
	extern void									VisitSymbolForScope(const ParsingArguments& pa, const ExprTsysItem* thisItem, ResolvedItem item, ExprTsysList& result);
	extern void									VisitSymbolForField(const ParsingArguments& pa, const ExprTsysItem* thisItem, ResolvedItem item, ExprTsysList& result);

	extern Ptr<Resolving>						FindMembersByName(const ParsingArguments& pa, CppName& name, ResolveSymbolResult* totalRar, const ExprTsysItem& parentItem);
	extern void									VisitResolvedMember(const ParsingArguments& pa, const ExprTsysItem* thisItem, Ptr<Resolving> resolving, ExprTsysList& result, bool& hasVariadic, bool& hasNonVariadic);
	extern void									VisitResolvedMember(const ParsingArguments& pa, const ExprTsysItem* thisItem, Ptr<Resolving> resolving, ExprTsysList& result);
	extern void									VisitFunctors(const ParsingArguments& pa, const ExprTsysItem& parentItem, const WString& name, ExprTsysList& result);
}

#endif