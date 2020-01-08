#include "Ast.h"
#include "Ast_Decl.h"
#include "Ast_Type.h"
#include "Parser.h"

namespace symbol_type_resolving
{
	template<typename TForward>
	bool IsStaticSymbol(Symbol* symbol)
	{
		bool isStatic = false;
		switch (symbol->GetCategory())
		{
		case symbol_component::SymbolCategory::Normal:
			if (auto decl = symbol->GetImplDecl_NFb<TForward>())
			{
				isStatic |= decl->decoratorStatic;
			}
			{
				const auto& decls = symbol->GetForwardDecls_N();
				for (vint i = 0; i < decls.Count(); i++)
				{
					if (auto decl = decls[i].Cast<TForward>())
					{
						isStatic |= decl->decoratorStatic;
					}
				}
			}
			break;
		case symbol_component::SymbolCategory::FunctionBody:
			if (auto decl = symbol->GetAnyForwardDecl<TForward>())
			{
				isStatic |= decl->decoratorStatic;
			}
			break;
		case symbol_component::SymbolCategory::Function:
			{
				const auto& symbols = symbol->GetForwardSymbols_F();
				for (vint i = 0; i < symbols.Count(); i++)
				{
					isStatic |= IsStaticSymbol<TForward>(symbols[i].Obj());
				}
			}
			{
				const auto& symbols = symbol->GetImplSymbols_F();
				for (vint i = 0; i < symbols.Count(); i++)
				{
					isStatic |= IsStaticSymbol<TForward>(symbols[i].Obj());
				}
			}
			break;
		default:
			throw UnexpectedSymbolCategoryException();
		}
		return isStatic;
	}

	extern bool									AddInternal(ExprTsysList& list, const ExprTsysItem& item);
	extern void									AddInternal(ExprTsysList& list, ExprTsysList& items);
	extern bool									AddVar(ExprTsysList& list, const ExprTsysItem& item);
	extern bool									AddNonVar(ExprTsysList& list, const ExprTsysItem& item);
	extern void									AddNonVar(ExprTsysList& list, ExprTsysList& items);
	extern bool									AddTemp(ExprTsysList& list, ITsys* tsys);
	extern void									AddTemp(ExprTsysList& list, TypeTsysList& items);

	extern void									CreateUniversalInitializerType(const ParsingArguments& pa, Array<ExprTsysList>& argTypesList, ExprTsysList& result);
	extern void									CalculateValueFieldType(const ExprTsysItem* thisItem, Symbol* symbol, ITsys* fieldType, bool forFieldDeref, ExprTsysList& result);
	extern Nullable<ExprTsysItem>				AdjustThisItemForSymbol(const ParsingArguments& pa, ExprTsysItem thisItem, Symbol* symbol);
	extern void									VisitSymbol(const ParsingArguments& pa, Symbol* symbol, ExprTsysList& result);
	extern void									VisitSymbolForScope(const ParsingArguments& pa, const ExprTsysItem* thisItem, Symbol* symbol, ExprTsysList& result);
	extern void									VisitSymbolForField(const ParsingArguments& pa, const ExprTsysItem* thisItem, Symbol* symbol, ExprTsysList& result);

	extern Ptr<Resolving>						FindMembersByName(const ParsingArguments& pa, CppName& name, ResolveSymbolResult* totalRar, const ExprTsysItem& parentItem);
	extern void									VisitResolvedMember(const ParsingArguments& pa, const ExprTsysItem* thisItem, Ptr<Resolving> resolving, ExprTsysList& result, bool& hasVariadic, bool& hasNonVariadic);
	extern void									VisitResolvedMember(const ParsingArguments& pa, const ExprTsysItem* thisItem, Ptr<Resolving> resolving, ExprTsysList& result);
	extern void									VisitFunctors(const ParsingArguments& pa, const ExprTsysItem& parentItem, const WString& name, ExprTsysList& result);

	extern void									Promote(TsysPrimitive& primitive);
	extern TsysPrimitive						ArithmeticConversion(TsysPrimitive leftP, TsysPrimitive rightP);

	// EvaluateSymbol

	extern TypeTsysList&						EvaluateVarSymbol				(const ParsingArguments& invokerPa,	ForwardVariableDeclaration* varDecl,	ITsys* parentDeclType															);
	extern void									SetFuncTypeByReturnStat			(const ParsingArguments& pa,		FunctionDeclaration* funcDecl,			TypeTsysList& returnTypes,			TemplateArgumentContext* argumentsToApply	);
	extern TypeTsysList&						EvaluateFuncSymbol				(const ParsingArguments& invokerPa,	ForwardFunctionDeclaration* funcDecl,	ITsys* parentDeclType,				TemplateArgumentContext* argumentsToApply	);
	extern TypeTsysList&						EvaluateForwardClassSymbol		(const ParsingArguments& invokerPa,	ForwardClassDeclaration* classDecl,		ITsys* parentDeclType,				TemplateArgumentContext* argumentsToApply	);
	extern symbol_component::Evaluation&		EvaluateClassSymbol				(const ParsingArguments& invokerPa,	ClassDeclaration* classDecl,			ITsys* parentDeclType,				TemplateArgumentContext* argumentsToApply	);
	extern TypeTsysList&						EvaluateTypeAliasSymbol			(const ParsingArguments& invokerPa,	TypeAliasDeclaration* usingDecl,		ITsys* parentDeclType,				TemplateArgumentContext* argumentsToApply	);
	extern TypeTsysList&						EvaluateValueAliasSymbol		(const ParsingArguments& invokerPa,	ValueAliasDeclaration* usingDecl,		ITsys* parentDeclType,				TemplateArgumentContext* argumentsToApply	);
	extern ITsys*								EvaluateGenericArgumentSymbol	(Symbol* symbol);
	extern symbol_component::Evaluation&		EvaluateClassType				(const ParsingArguments& invokerPa, ITsys* classType);

	// Overloading

	extern void									FilterFieldsAndBestQualifiedFunctions(TsysCV thisCV, TsysRefType thisRef, ExprTsysList& funcTypes);
	extern void									FindQualifiedFunctors(const ParsingArguments& pa, TsysCV thisCV, TsysRefType thisRef, ExprTsysList& funcTypes, bool lookForOp);

	extern void									VisitOverloadedFunction(const ParsingArguments& pa, ExprTsysList& funcTypes, Array<ExprTsysItem>& argTypes, SortedList<vint>& boundedAnys, ExprTsysList& result, ExprTsysList* selectedFunctions = nullptr, bool* anyInvolved = nullptr);
	extern bool									IsAdlEnabled(const ParsingArguments& pa, Ptr<Resolving> resolving);
	extern void									SearchAdlClassesAndNamespaces(const ParsingArguments& pa, Symbol* symbol, SortedList<Symbol*>& nss, SortedList<Symbol*>& classes);
	extern void									SearchAdlClassesAndNamespaces(const ParsingArguments& pa, ITsys* type, SortedList<Symbol*>& nss, SortedList<Symbol*>& classes);
	extern void									SearchAdlFunction(const ParsingArguments& pa, SortedList<Symbol*>& nss, const WString& name, ExprTsysList& result);

	// Generic

	extern Ptr<TemplateSpec>					GetTemplateSpecFromSymbol(Symbol* symbol);
	extern ITsys*								GetTemplateArgumentKey(const TemplateSpec::Argument& argument, ITsysAlloc* tsys);
	extern void									CreateGenericFunctionHeader(const ParsingArguments& pa, Ptr<TemplateSpec> spec, TypeTsysList& params, TsysGenericFunction& genericFunction);
	extern void									ResolveGenericParameters(
													const ParsingArguments& invokerPa,
													TemplateArgumentContext& newTaContext,
													ITsys* genericFunction,
													Array<ExprTsysItem>& argumentTypes,
													Array<bool>& isTypes,
													Array<vint>& argSource,
													SortedList<vint>& boundedAnys,
													vint offset,
													bool allowPartialApply
												);
}