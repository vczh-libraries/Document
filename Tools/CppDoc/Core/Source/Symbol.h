#ifndef VCZH_DOCUMENT_CPPDOC_SYMBOL
#define VCZH_DOCUMENT_CPPDOC_SYMBOL

#include "TypeSystem.h"

struct CppName;
class Symbol;
class SpecializationSpec;
class Declaration;
class ForwardFunctionDeclaration;
class FunctionDeclaration;
class Stat;
class Program;

// the form or location of the expression is incorrect
struct IllegalExprException {};

// unable to recover from type checking
struct TypeCheckerException {};

// call unsupported method of Symbol, usually because of the symbol category
struct UnexpectedSymbolCategoryException {};

// stop evaluating function body, because the current context is to evaluating the return type
struct FinishEvaluatingReturnType {};

/***********************************************************************
Symbol
***********************************************************************/

namespace symbol_component
{
	struct ClassMemberCache;

	enum class EvaluationProgress
	{
		NotEvaluated,
		Evaluating,
		Evaluated,
		RecursiveFound,
	};

	enum class SymbolKind
	{
		Enum,
		Class,
		Struct,
		Union,
		TypeAlias,
		GenericTypeArgument,

		/*
		For any non-class enum,
		there will be another symbol in the parent scope of the Enum symbol,
		having the same implementation declaration with this EnumItem symbol
		*/
		EnumItem,
		FunctionSymbol,			// Category == Function
		FunctionBodySymbol,		// Category == FunctionBody
		Variable,
		ValueAlias,
		GenericValueArgument,

		Namespace,
		Statement,
		Root,
	};

#define CLASS_SYMBOL_KIND						\
	symbol_component::SymbolKind::Class:		\
	case symbol_component::SymbolKind::Struct:	\
	case symbol_component::SymbolKind::Union	\

#define CSTYLE_TYPE_SYMBOL_KIND					\
	symbol_component::SymbolKind::Enum:			\
	case CLASS_SYMBOL_KIND						\

	struct Evaluation
	{
	private:
		Ptr<TypeTsysList>							mainTypeList;
		List<Ptr<TypeTsysList>>						extraTypeLists;

	public:
		Evaluation() = default;
		Evaluation(const Evaluation&) = delete;
		Evaluation(Evaluation&&) = delete;

		// this flag is set only in ParseDeclaration_Class_NotConsumeSemicolon and EvaluateClassSymbol
		// EvaluateClassSymbol will skip evaluating base types when it is true
		bool										skipEvaluatingBaseTypes = false;

		EvaluationProgress							progress = EvaluationProgress::NotEvaluated;
		bool										isVariadic = false;

		void										Allocate();
		void										AllocateExtra(vint extraCount);
		void										Clear();
		vint										ExtraCount();
		TypeTsysList&								Get();
		TypeTsysList&								GetExtra(vint index);
		void										ReplaceExtra(vint index, Ptr<TypeTsysList> tsysList);
	};

	enum class SymbolCategory
	{
		/*
		could be
			Root
			TemplateSpec(Kind is Root temporarily)
				a scope created by TemplateSpec temporary,
				before the actual type of the declaration is parsed,
				and then convert to other Normal category kind or FunctionBody category kind
			BlockStat
				the name is always "$"
			Namespace
				all namespace declarations are forward declarations, there is no implementation declaration
			Everything else
				has one implementation declaration and multiple forward declarations
		*/
		Normal,

		/*
		A function. The symbol could be
			the scope created by TemplateSpec if it is a generic function, symbols of both type arguments and function arguments are put in this scope
			the scope created by (Forward)FunctionDeclaration, function arguments are put in this scope
		Has either one implementation declaration or one forward declaration
		Symbol name could be
			"$__ctor" for constructors
			"$__type" for type conversion operator
			"~TYPE" for destructors
			the function name itself
		*/
		FunctionBody,

		/*
		All FunctionBody that are the same function
		There could be multiple implementation FunctionBody, if types cannot be distinguished by the compiler
			e.g. overloadings on a dependency type, only constants are different. Constants are treated as identical to each other in the compiler.
		*/
		Function,
	};

	using SymbolGroup = Group<WString, Ptr<Symbol>>;

	// classes and value aliases cannot overload, all FunctionBodySymbol of the same signature has been put under the same FunctionSymbol
	// so psPrimary does not need to be a list
	struct SC_PSShared
	{
		vint										psVersion = 0;			// sync with psDescendants.Count()
		Symbol*										psPrimary = nullptr;	// primary declaration of this partial specialization
		List<Symbol*>								psDescendants;			// all descedant partial specialization of this primary declaration
		List<Symbol*>								psParents;				// parent partial specializations in partial order
		List<Symbol*>								psChildren;				// child partial specializations in partial order
	};

	struct SC_Normal : SC_PSShared
	{
		Symbol*										parent = nullptr;
		Ptr<Declaration>							implDecl;
		List<Ptr<Declaration>>						forwardDecls;
		Ptr<ClassMemberCache>						classMemberCache;
		Ptr<Stat>									statement;
		SymbolGroup									children;
		Evaluation									evaluation;			// type of this symbol, or type of base types of a class, when all template arguments are unassigned
	};

	struct SC_FunctionBody
	{
		Symbol*										functionSymbol;
		Ptr<FunctionDeclaration>					implDecl;
		Ptr<ForwardFunctionDeclaration>				forwardDecl;
		Ptr<ClassMemberCache>						classMemberCache;
		SymbolGroup									children;
		Evaluation									evaluation;			// type of this symbol, when all template arguments are unassigned
	};

	struct SC_Function : SC_PSShared
	{
		Symbol*										parent = nullptr;
		List<Ptr<Symbol>>							implSymbols;
		List<Ptr<Symbol>>							forwardSymbols;
	};

	union SC_Data
	{
		SC_Normal									normal;
		SC_FunctionBody								functionBody;
		SC_Function									function;

		SC_Data(SymbolCategory category);
		~SC_Data();

		void										Alloc(SymbolCategory category);
		void										Free(SymbolCategory category);
	};

	struct SG_Cache
	{
		Ptr<Array<ITsys*>>							parentDeclTypeAndParams;
		Ptr<Evaluation>								cachedEvaluation;

		static vint Compare(const SG_Cache& a, const SG_Cache& b)
		{
			return CompareEnumerable(*a.parentDeclTypeAndParams.Obj(), *b.parentDeclTypeAndParams.Obj());
		}
#define OPERATOR_COMPARE(OP)\
		bool operator OP(const SG_Cache& cache)const\
		{\
			return SG_Cache::Compare(*this, cache) OP 0;\
		}\

		OPERATOR_COMPARE(> )
		OPERATOR_COMPARE(>= )
		OPERATOR_COMPARE(< )
		OPERATOR_COMPARE(<= )
		OPERATOR_COMPARE(== )
		OPERATOR_COMPARE(!= )
#undef OPERATOR_COMPARE
	};
}

class Symbol : public Object
{
private:
	symbol_component::SymbolCategory				category;
	symbol_component::SC_Data						categoryData;

	WString											DecorateNameForSpecializationSpec(const WString& symbolName, Ptr<SpecializationSpec> spec);
	void											ReuseTemplateSpecSymbol(Ptr<Symbol> templateSpecSymbol, symbol_component::SymbolCategory _category);
	Symbol*											CreateSymbolInternal(Ptr<Declaration> _decl, const WString& declName, Ptr<Symbol> templateSpecSymbol, symbol_component::SymbolKind _kind, symbol_component::SymbolCategory _category);
	Symbol*											AddToSymbolInternal_NFb(Ptr<Declaration> _decl, symbol_component::SymbolKind kind, Ptr<Symbol> templateSpecSymbol, symbol_component::SymbolCategory _category);
	void											SetParent(Symbol* parent);
	symbol_component::SC_PSShared*					GetPSShared();

public:
	symbol_component::SymbolKind					kind = symbol_component::SymbolKind::Root;
	bool											ellipsis = false;							// for variant template argument and function argument
	Symbol*											declSymbolForGenericArg = nullptr;			// for template argument
	WString											name;
	WString											uniqueId;
	List<Symbol*>									usingNss;
	SortedList<symbol_component::SG_Cache>			genericCaches;

public:
	Symbol(symbol_component::SymbolCategory _category, Symbol* _parent = nullptr);
	Symbol(Symbol* _parent = nullptr, Ptr<symbol_component::ClassMemberCache> classMemberCache = nullptr);
	~Symbol();

	symbol_component::SymbolCategory				GetCategory();
	void											SetCategory(symbol_component::SymbolCategory _category);

	Symbol*											GetParentScope();							//	Normal	FunctionBody	Function
	const List<Ptr<Symbol>>&						GetImplSymbols_F();							//							Function
	const List<Ptr<Symbol>>&						GetForwardSymbols_F();						//							Function
	Ptr<Declaration>								GetImplDecl_NFb();							//	Normal	FunctionBody
	symbol_component::Evaluation&					GetEvaluationForUpdating_NFb();				//	Normal	FunctionBody
	const symbol_component::SymbolGroup&			GetChildren_NFb();							//	Normal	FunctionBody
	const List<Ptr<Declaration>>&					GetForwardDecls_N();						//	Normal
	const Ptr<Stat>&								GetStat_N();								//	Normal
	Symbol*											GetFunctionSymbol_Fb();						//			FunctionBody
	Ptr<Declaration>								GetForwardDecl_Fb();						//			FunctionBody
	Ptr<symbol_component::ClassMemberCache>			GetClassMemberCache_NFb();					//			FunctionBody

	Symbol*											GetPSPrimary_NF();							//	Normal					Function
	vint											GetPSPrimaryVersion_NF();					//	Normal					Function
	const List<Symbol*>&							GetPSPrimaryDescendants_NF();				//	Normal					Function
	const List<Symbol*>&							GetPSParents_NF();							//	Normal					Function
	const List<Symbol*>&							GetPSChildren_NF();							//	Normal					Function
	bool											IsPSPrimary_NF();							//	Normal					Function
	void											AssignPSPrimary_NF(Symbol* primary);		//	Normal					Function
	void											RemovePSParent_NF(Symbol* oldParent);		//	Normal					Function
	void											AddPSParent_NF(Symbol* newParent);			//	Normal					Function

	void											SetClassMemberCacheForTemplateSpecScope_N(Ptr<symbol_component::ClassMemberCache> classMemberCache);
	const List<Ptr<Symbol>>*						TryGetChildren_NFb(const WString& name);
	void											AddChild_NFb(const WString& name, const Ptr<Symbol>& child);
	void											AddChildAndSetParent_NFb(const WString& name, const Ptr<Symbol>& child);
	void											RemoveChildAndResetParent_NFb(const WString& name, Symbol* child);

	template<typename T>
	const Ptr<T> GetImplDecl_NFb()
	{
		return GetImplDecl_NFb().Cast<T>();
	}

	Symbol*											CreateFunctionSymbol_NFb(Ptr<ForwardFunctionDeclaration> _decl);
	Symbol*											CreateFunctionForwardSymbol_F(Ptr<ForwardFunctionDeclaration> _decl, Ptr<Symbol> templateSpecSymbol);
	Symbol*											CreateFunctionImplSymbol_F(Ptr<FunctionDeclaration> _decl, Ptr<Symbol> templateSpecSymbol, Ptr<symbol_component::ClassMemberCache> classMemberCache);
	Symbol*											AddForwardDeclToSymbol_NFb(Ptr<Declaration> _decl, symbol_component::SymbolKind kind);
	Symbol*											AddImplDeclToSymbol_NFb(Ptr<Declaration> _decl, symbol_component::SymbolKind kind, Ptr<Symbol> templateSpecSymbol = nullptr, Ptr<symbol_component::ClassMemberCache> classMemberCache = nullptr);
	Symbol*											CreateStatSymbol_NFb(Ptr<Stat> _stat);

	template<typename T>
	Ptr<T> GetAnyForwardDecl()
	{
		switch (category)
		{
		case symbol_component::SymbolCategory::Normal:
			{
				auto decl = categoryData.normal.implDecl.Cast<T>();
				if (decl) return decl;
				for (vint i = 0; i < categoryData.normal.forwardDecls.Count(); i++)
				{
					if ((decl = categoryData.normal.forwardDecls[i].Cast<T>()))
					{
						return decl;
					}
				}
				return nullptr;
			}
		case symbol_component::SymbolCategory::FunctionBody:
			{
				auto decl = categoryData.functionBody.implDecl.Cast<T>();
				return decl ? decl : categoryData.functionBody.forwardDecl.Cast<T>();
			}
		case symbol_component::SymbolCategory::Function:
			{
				Ptr<T> decl;
				for (vint i = 0; i < categoryData.function.forwardSymbols.Count(); i++)
				{
					if ((decl = categoryData.function.forwardSymbols[i]->GetAnyForwardDecl<T>()))
					{
						return decl;
					}
				}
				for (vint i = 0; i < categoryData.function.implSymbols.Count(); i++)
				{
					if ((decl = categoryData.function.implSymbols[i]->GetAnyForwardDecl<T>()))
					{
						return decl;
					}
				}
				return nullptr;
			}
		default:
			throw UnexpectedSymbolCategoryException();
		}
	}

	void											GenerateUniqueId(Dictionary<WString, Symbol*>& ids, const WString& prefix);
};

template<typename T>
Ptr<T> TryGetDeclFromType(ITsys* type)
{
	if (type->GetType() == TsysType::Decl || type->GetType() == TsysType::DeclInstant)
	{
		return type->GetDecl()->GetImplDecl_NFb<T>();
	}
	return nullptr;
}

template<typename T>
Ptr<T> TryGetForwardDeclFromType(ITsys* type)
{
	if (type->GetType() == TsysType::Decl || type->GetType() == TsysType::DeclInstant)
	{
		return type->GetDecl()->GetAnyForwardDecl<T>();
	}
	return nullptr;
}

/***********************************************************************
Parser Objects
***********************************************************************/

struct ResolvedItem;

class IIndexRecorder : public virtual Interface
{
public:
	virtual void									Index(CppName& name, List<ResolvedItem>& resolvedSymbols) = 0;
	virtual void									IndexOverloadingResolution(CppName& name, List<ResolvedItem>& resolvedSymbols) = 0;
	virtual void									ExpectValueButType(CppName& name, List<ResolvedItem>& resolvedSymbols) = 0;
};

enum class EvaluationKind
{
	General,
	Instantiated,
	GeneralUnderInstantiated,
};

struct TemplateArgumentContext
{
private:
	Symbol*											symbolToApply = nullptr;
	Array<ITsys*>									assignedArguments;
	Array<bool>										assignedKeys;
	vint											availableCount = 0;

	void											InitArguments(vint argumentCount);
public:
	TemplateArgumentContext*						parent = nullptr;

	//	Keys:
	//		Type, Value								:	GenericArg(TemplateArgument)
	//	Values:
	//		Single									:	Anything (nullptr for value argument)
	//		MultipleValues							:	{Values ...}
	//		UnknownAmountOfMultipleValues			:	any_t

	//	evaluation result if template arguments of this symbol are unassigned, but template arguments of parent scopes are assigned
	Dictionary<Symbol*, Ptr<symbol_component::Evaluation>>		symbolEvaluations;

	TemplateArgumentContext(Symbol* _symbolToApply, vint argumentCount);
	TemplateArgumentContext(TemplateArgumentContext* prototypeContext, bool copyArguments);

	Symbol*											GetSymbolToApply()const;
	vint											GetArgumentCount()const;
	ITsys*											GetKey(vint index)const;
	ITsys*											GetValue(vint index)const;
	ITsys*											GetValueByKey(ITsys* key)const;
	vint											GetAvailableArgumentCount()const;
	bool											IsArgumentAvailable(vint index)const;
	bool											TryGetValueByKey(ITsys* key, ITsys*& value)const;
	void											SetValueByKey(ITsys* key, ITsys* value);
	void											ReplaceValueByKey(ITsys* key, ITsys* value);
};

struct ParsingArguments
{
	Ptr<Symbol>										root;
	Ptr<Program>									program;

	Symbol*											scopeSymbol = nullptr;
	Symbol*											functionBodySymbol = nullptr;
	ITsys*											parentDeclType = nullptr;
	TemplateArgumentContext*						taContext = nullptr;
	Ptr<ITsysAlloc>									tsys;
	Ptr<IIndexRecorder>								recorder;

	ParsingArguments() = default;
	ParsingArguments(const ParsingArguments&) = default;
	ParsingArguments(ParsingArguments&&) = default;
	ParsingArguments(Ptr<Symbol> _root, Ptr<ITsysAlloc> _tsys, Ptr<IIndexRecorder> _recorder);

	ParsingArguments&								operator=(const ParsingArguments&) = default;
	ParsingArguments&								operator=(ParsingArguments&&) = default;

	ParsingArguments								WithScope(Symbol* _scopeSymbol)const;
	ParsingArguments								AppendSingleLevelArgs(TemplateArgumentContext& taContext)const;
	ParsingArguments								AdjustForDecl(Symbol* declSymbol)const;
	ParsingArguments								AdjustForDecl(Symbol* declSymbol, ITsys* parentDeclType)const;

	EvaluationKind									GetEvaluationKind(Declaration* decl, Ptr<TemplateSpec> spec)const;
	bool											IsGeneralEvaluation()const;
	bool											TryGetReplacedGenericArg(ITsys* arg, ITsys*& result)const;

	static TemplateArgumentContext*					AdjustTaContextForScope(Symbol* scopeSymbol, TemplateArgumentContext* taContext);
	static ITsys*									AdjustDeclInstantForScope(Symbol* scopeSymbol, ITsys* parentDeclType, bool returnTypeOfScope);
};

/***********************************************************************
Parser Functions
***********************************************************************/

extern Symbol*										FindParentClassSymbol(Symbol* symbol, bool includeThisSymbol);
extern Symbol*										FindParentTemplateClassSymbol(Symbol* symbol);

#endif