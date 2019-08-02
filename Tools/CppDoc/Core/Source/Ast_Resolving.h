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
		if (auto decl = symbol->definition.Cast<TForward>())
		{
			isStatic |= decl->decoratorStatic;
		}
		for (vint i = 0; i < symbol->declarations.Count(); i++)
		{
			if (auto decl = symbol->declarations[i].Cast<TForward>())
			{
				isStatic |= decl->decoratorStatic;
			}
		}
		return isStatic;
	}

	extern bool				AddInternal(ExprTsysList& list, const ExprTsysItem& item);
	extern void				AddInternal(ExprTsysList& list, ExprTsysList& items);
	extern bool				AddVar(ExprTsysList& list, const ExprTsysItem& item);
	extern bool				AddNonVar(ExprTsysList& list, const ExprTsysItem& item);
	extern void				AddNonVar(ExprTsysList& list, ExprTsysList& items);
	extern bool				AddTemp(ExprTsysList& list, ITsys* tsys);
	extern void				AddTemp(ExprTsysList& list, TypeTsysList& items);

	struct EvaluateSymbolContext
	{
		GenericArgContext	gaContext;
		TypeTsysList		evaluatedTypes;
	};

	extern void				CreateUniversalInitializerType(const ParsingArguments& pa, Array<ExprTsysList>& argTypesList, ExprTsysList& result);
	extern void				CalculateValueFieldType(const ExprTsysItem* thisItem, Symbol* symbol, ITsys* fieldType, bool forFieldDeref, ExprTsysList& result);
	extern void				EvaluateSymbol(const ParsingArguments& pa, ForwardVariableDeclaration* varDecl);
	extern bool				IsMemberFunction(const ParsingArguments& pa, ForwardFunctionDeclaration* funcDecl);
	extern void				FinishEvaluatingSymbol(const ParsingArguments& pa, FunctionDeclaration* funcDecl);
	extern void				EvaluateSymbol(const ParsingArguments& pa, ForwardFunctionDeclaration* funcDecl);
	extern void				EvaluateSymbol(const ParsingArguments& pa, ClassDeclaration* classDecl);
	extern void				EvaluateSymbol(const ParsingArguments& pa, TypeAliasDeclaration* usingDecl, EvaluateSymbolContext* esContext = nullptr);
	extern void				EvaluateSymbol(const ParsingArguments& pa, ValueAliasDeclaration* usingDecl, EvaluateSymbolContext* esContext = nullptr);
	extern void				VisitSymbol(const ParsingArguments& pa, Symbol* symbol, ExprTsysList& result);
	extern void				VisitSymbolForScope(const ParsingArguments& pa, const ExprTsysItem* thisItem, Symbol* symbol, ExprTsysList& result);
	extern void				VisitSymbolForField(const ParsingArguments& pa, const ExprTsysItem* thisItem, Symbol* symbol, ExprTsysList& result);

	extern Ptr<Resolving>	FindMembersByName(const ParsingArguments& pa, CppName& name, ResolveSymbolResult* totalRar, const ExprTsysItem& parentItem);
	extern void				VisitResolvedMember(const ParsingArguments& pa, Ptr<Resolving> resolving, ExprTsysList& result, bool& hasVariadic, bool& hasNonVariadic);
	extern void				VisitResolvedMember(const ParsingArguments& pa, const ExprTsysItem* thisItem, Ptr<Resolving> resolving, ExprTsysList& result);
	extern void				VisitFunctors(const ParsingArguments& pa, const ExprTsysItem& parentItem, const WString& name, ExprTsysList& result);

	extern void				Promote(TsysPrimitive& primitive);
	extern TsysPrimitive	ArithmeticConversion(TsysPrimitive leftP, TsysPrimitive rightP);

	// Overloading

	extern void				FilterFieldsAndBestQualifiedFunctions(TsysCV thisCV, TsysRefType thisRef, ExprTsysList& funcTypes);
	extern void				FindQualifiedFunctors(const ParsingArguments& pa, TsysCV thisCV, TsysRefType thisRef, ExprTsysList& funcTypes, bool lookForOp);

	extern void				VisitOverloadedFunction(const ParsingArguments& pa, ExprTsysList& funcTypes, Array<ExprTsysItem>& argTypes, ExprTsysList& result, ExprTsysList* selectedFunctions);
	extern bool				IsAdlEnabled(const ParsingArguments& pa, Ptr<Resolving> resolving);
	extern void				SearchAdlClassesAndNamespaces(const ParsingArguments& pa, Symbol* symbol, SortedList<Symbol*>& nss, SortedList<Symbol*>& classes);
	extern void				SearchAdlClassesAndNamespaces(const ParsingArguments& pa, ITsys* type, SortedList<Symbol*>& nss, SortedList<Symbol*>& classes);
	extern void				SearchAdlFunction(const ParsingArguments& pa, SortedList<Symbol*>& nss, const WString& name, ExprTsysList& result);

	// Generic

	extern void				CreateGenericFunctionHeader(Ptr<TemplateSpec> spec, TypeTsysList& params, TsysGenericFunction& genericFunction);
	extern bool				ResolveGenericParameters(const ParsingArguments& pa, ITsys* genericFunction, Array<ExprTsysItem>& argumentTypes, Array<bool>& isTypes, vint offset, GenericArgContext* newGaContext);
}