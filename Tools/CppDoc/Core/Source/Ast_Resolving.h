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

	extern void				CalculateValueFieldType(const ExprTsysItem* thisItem, Symbol* symbol, ITsys* fieldType, bool forFieldDeref, ExprTsysList& result);
	extern void				EvaluateSymbol(const ParsingArguments& pa, ForwardVariableDeclaration* varDecl);
	extern bool				IsMemberFunction(const ParsingArguments& pa, ForwardFunctionDeclaration* funcDecl);
	extern void				FinishEvaluatingSymbol(const ParsingArguments& pa, FunctionDeclaration* funcDecl);
	extern void				EvaluateSymbol(const ParsingArguments& pa, ForwardFunctionDeclaration* funcDecl);
	extern void				EvaluateSymbol(const ParsingArguments& pa, ClassDeclaration* classDecl);
	extern void				EvaluateSymbol(const ParsingArguments& pa, UsingDeclaration* usingDecl, EvaluateSymbolContext* esContext = nullptr);
	extern void				VisitSymbol(const ParsingArguments& pa, const ExprTsysItem* thisItem, Symbol* symbol, bool afterScope, ExprTsysList& result);

	extern TsysConv			FindMinConv(ArrayBase<TsysConv>& funcChoices);
	extern void				FilterFunctionByConv(ExprTsysList& funcTypes, ArrayBase<TsysConv>& funcChoices);
	extern void				FilterFieldsAndBestQualifiedFunctions(TsysCV thisCV, TsysRefType thisRef, ExprTsysList& funcTypes);
	extern void				FindQualifiedFunctors(const ParsingArguments& pa, TsysCV thisCV, TsysRefType thisRef, ExprTsysList& funcTypes, bool lookForOp);

	extern void				VisitResolvedMember(const ParsingArguments& pa, const ExprTsysItem* thisItem, Ptr<Resolving> resolving, ExprTsysList& result);
	extern void				VisitDirectField(const ParsingArguments& pa, ResolveSymbolResult& totalRar, const ExprTsysItem& parentItem, CppName& name, ExprTsysList& result);
	extern void				VisitFunctors(const ParsingArguments& pa, const ExprTsysItem& parentItem, const WString& name, ExprTsysList& result);
	extern void				VisitOverloadedFunction(const ParsingArguments& pa, ExprTsysList& funcTypes, List<Ptr<ExprTsysList>>& argTypesList, ExprTsysList& result, ExprTsysList* selectedFunctions);

	extern bool				IsAdlEnabled(const ParsingArguments& pa, Ptr<Resolving> resolving);
	extern void				SearchAdlClassesAndNamespaces(const ParsingArguments& pa, Symbol* symbol, SortedList<Symbol*>& nss, SortedList<Symbol*>& classes);
	extern void				SearchAdlClassesAndNamespaces(const ParsingArguments& pa, ITsys* type, SortedList<Symbol*>& nss, SortedList<Symbol*>& classes);
	extern void				SearchAdlClassesAndNamespaces(const ParsingArguments& pa, ExprTsysList& types, SortedList<Symbol*>& nss, SortedList<Symbol*>& classes);
	extern void				SerachAdlFunction(const ParsingArguments& pa, SortedList<Symbol*>& nss, const WString& name, ExprTsysList& result);

	extern void				Promote(TsysPrimitive& primitive);
	extern TsysPrimitive	ArithmeticConversion(TsysPrimitive leftP, TsysPrimitive rightP);

	extern void				CreateGenericFunctionHeader(Ptr<TemplateSpec> spec, TypeTsysList& params, TsysGenericFunction& genericFunction);
	extern void				ResolveGenericArguments(const ParsingArguments& pa, List<GenericArgument>& arguments, Array<Ptr<TypeTsysList>>& argumentTypes, GenericArgContext* gaContext);
	extern bool				ResolveGenericParameters(ITsys* genericFunction, Array<Ptr<TypeTsysList>>& argumentTypes, GenericArgContext* newGaContext);
}