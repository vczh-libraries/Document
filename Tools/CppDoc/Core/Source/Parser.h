#ifndef VCZH_DOCUMENT_CPPDOC_PARSER
#define VCZH_DOCUMENT_CPPDOC_PARSER

#include "Lexer.h"
#include "Ast.h"

/***********************************************************************
Symbol
***********************************************************************/

class TemplateSpec;
class SpecializationSpec;
class ClassDeclaration;
class FunctionDeclaration;
class ForwardFunctionDeclaration;
class Symbol;

namespace symbol_component
{
	struct ClassMemberCache
	{
		/*
		for void A::B::C::F(){}, it will be [C, B, A]
		*/
		List<ITsys*>				containerClassTypes;

		/*
		true for members defined inside a class, so that type arguments reachable in classSymbols are reachable here
		false for outside
		*/
		bool						symbolDefinedInsideClass;

		/*
		The scope to jump to after all symbols in classSymbols are scanned
		for members defined inside a class, this is the parent scope of the most outside class
		for members defined outside a class, this is the scope where the member is defined
		*/
		Symbol*						parentScope;

		// applicable where this cache is in a function
		ITsys*						thisType = nullptr;
		Symbol*						funcSymbol = nullptr;
		Ptr<FunctionDeclaration>	funcDecl;
	};

	enum class EvaluationProgress
	{
		NotEvaluated,
		Evaluating,
		Evaluated,
		Undecided,
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

		EvaluationProgress							progress = EvaluationProgress::NotEvaluated;

		void										Allocate();
		void										AllocateExtra(vint extraCount);
		void										Clear();
		vint										ExtraCount();
		TypeTsysList&								Get();
		TypeTsysList&								GetExtra(vint index);
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
	bool											ellipsis = false;		// for variant template argument and function argument
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
	Symbol*											AddImplDeclToSymbol_NFb(Ptr<Declaration> _decl, symbol_component::SymbolKind kind, Ptr<Symbol> templateSpecSymbol = nullptr);
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

class IIndexRecorder : public virtual Interface
{
public:
	virtual void									Index(CppName& name, List<Symbol*>& resolvedSymbols) = 0;
	virtual void									IndexOverloadingResolution(CppName& name, List<Symbol*>& resolvedSymbols) = 0;
	virtual void									ExpectValueButType(CppName& name, List<Symbol*>& resolvedSymbols) = 0;
};

enum class EvaluationKind
{
	General,
	Instantiated,
	GeneralUnderInstantiated,
};

struct TemplateArgumentContext
{
	TemplateArgumentContext*									parent = nullptr;
	Symbol*														symbolToApply = nullptr;

	//	Keys:
	//		Type												:	GenericArg(Decl(TemplateArgument))
	//		Value												:	Decl(TemplateArgument)
	//	Values:
	//		Single												:	Anything (nullptr for value argument)
	//		MultipleValues										:	{Values ...}
	//		UnknownAmountOfMultipleValues						:	any_t
	Dictionary<ITsys*, ITsys*>									arguments;

	//	evaluation result if template arguments of this symbol are unassigned, but template arguments of parent scopes are assigned
	Dictionary<Symbol*, Ptr<symbol_component::Evaluation>>		symbolEvaluations;
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
	ParsingArguments								AdjustForDecl(Symbol* declSymbol, ITsys* parentDeclType, bool forceOverrideForNull)const;

	EvaluationKind									GetEvaluationKind(Declaration* decl, Ptr<TemplateSpec> spec)const;
	bool											IsGeneralEvaluation()const;
	bool											TryGetReplacedGenericArg(ITsys* arg, ITsys*& result)const;

	static TemplateArgumentContext*					AdjustTaContextForScope(Symbol* scopeSymbol, TemplateArgumentContext* taContext);
	static ITsys*									AdjustDeclInstantForScope(Symbol* scopeSymbol, ITsys* parentDeclType, bool returnTypeOfScope);
};

class DelayParse : public Object
{
public:
	ParsingArguments								pa;
	Ptr<CppTokenReader>								reader;
	Ptr<CppTokenCursor>								begin;
	RegexToken										end;
};

struct StopParsingException
{
	Ptr<CppTokenCursor>								position;

	StopParsingException(Ptr<CppTokenCursor> _position) :position(_position) {}
};

/***********************************************************************
Parser Functions
***********************************************************************/

enum class DeclaratorRestriction
{
	Zero,
	Optional,
	One,
	Many,
};

enum class InitializerRestriction
{
	Zero,
	Optional,
};

class FunctionType;
class ClassDeclaration;
class VariableDeclaration;

// Parser_ResolveSymbol.cpp
enum class SearchPolicy
{
	SymbolAccessableInScope,							// search a name in a bounding scope of the current checking place
	SymbolAccessableInScope_CStyleTypeReference,		// like above but it is explicitly required to be a type using "struct X"
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
extern ResolveSymbolResult							ResolveSymbol(const ParsingArguments& pa, CppName& name, SearchPolicy policy, ResolveSymbolResult input = {});
extern ResolveSymbolResult							ResolveChildSymbol(const ParsingArguments& pa, Ptr<Type> classType, CppName& name, ResolveSymbolResult input = {});

// Parser_Misc.cpp
extern bool											SkipSpecifiers(Ptr<CppTokenCursor>& cursor);
extern bool											ParseCppName(CppName& name, Ptr<CppTokenCursor>& cursor, bool forceSpecialMethod = false);
extern Ptr<Type>									GetTypeWithoutMemberAndCC(Ptr<Type> type);
extern Ptr<Type>									ReplaceTypeInMemberAndCC(Ptr<Type>& type, Ptr<Type> typeToReplace);
extern Ptr<Type>									AdjustReturnTypeWithMemberAndCC(Ptr<FunctionType> functionType);
extern bool											ParseCallingConvention(TsysCallingConvention& callingConvention, Ptr<CppTokenCursor>& cursor);

// Parser_Type.cpp
enum class ShortTypeTypenameKind
{
	No,
	Yes,
	Implicit,
};
extern Ptr<Type>									ParseShortType(const ParsingArguments& pa, ShortTypeTypenameKind typenameKind, Ptr<CppTokenCursor>& cursor);
extern Ptr<Type>									ParseLongType(const ParsingArguments& pa, Ptr<CppTokenCursor>& cursor);

#define PARSING_DECLARATOR_ARGUMENTS(PREFIX, DELIMITER)												\
	ClassDeclaration*								PREFIX##containingClass			DELIMITER		\
	Ptr<Symbol>										PREFIX##scopeSymbolToReuse		DELIMITER		\
	bool											PREFIX##forParameter			DELIMITER		\
	DeclaratorRestriction							PREFIX##dr						DELIMITER		\
	InitializerRestriction							PREFIX##ir						DELIMITER		\
	bool											PREFIX##allowBitField			DELIMITER		\
	bool											PREFIX##allowEllipsis			DELIMITER		\
	bool											PREFIX##allowComma				DELIMITER		\
	bool											PREFIX##allowGtInInitializer	DELIMITER		\
	bool											PREFIX##allowSpecializationSpec					\

#define PARSING_DECLARATOR_COPY(PREFIX)										\
	containingClass(PREFIX##containingClass)								\
	, scopeSymbolToReuse(PREFIX##scopeSymbolToReuse)						\
	, forParameter(PREFIX##forParameter)									\
	, dr(PREFIX##dr)														\
	, ir(PREFIX##ir)														\
	, allowBitField(PREFIX##allowBitField)									\
	, allowEllipsis(PREFIX##allowEllipsis)									\
	, allowComma(PREFIX##allowComma)										\
	, allowGtInInitializer(PREFIX##allowGtInInitializer)					\
	, allowSpecializationSpec(PREFIX##allowSpecializationSpec)				\

// Parser_Declarator.cpp
struct ParsingDeclaratorArguments
{
	PARSING_DECLARATOR_ARGUMENTS(, ;);

#define __ ,
	ParsingDeclaratorArguments(PARSING_DECLARATOR_ARGUMENTS(_, __))
		: PARSING_DECLARATOR_COPY(_)
	{
	}
#undef __
};

#define PDA_HEADER(NAME) inline ParsingDeclaratorArguments	pda_##NAME
//																					class		symbol		param			declarator-count					initializer-count					bitfield		ellipsis		comma		=a>b	spec
PDA_HEADER(Type)			()										{	return {	nullptr,	nullptr,	false,			DeclaratorRestriction::Zero,		InitializerRestriction::Zero,		false,			false,			false,		false,	false	}; } // Type
PDA_HEADER(VarType)			()										{	return {	nullptr,	nullptr,	false,			DeclaratorRestriction::Optional,	InitializerRestriction::Zero,		false,			false,			false,		false,	false	}; } // Type or Variable without Initializer
PDA_HEADER(VarInit)			()										{	return {	nullptr,	nullptr,	false,			DeclaratorRestriction::One,			InitializerRestriction::Optional,	false,			false,			false,		true,	false	}; } // Variable with Initializer
PDA_HEADER(VarNoInit)		()										{	return {	nullptr,	nullptr,	false,			DeclaratorRestriction::One,			InitializerRestriction::Zero,		false,			false,			false,		false,	false	}; } // Variable without Initializer
PDA_HEADER(Param)			(bool forParameter)						{	return {	nullptr,	nullptr,	forParameter,	DeclaratorRestriction::Optional,	InitializerRestriction::Optional,	false,			forParameter,	false,		true,	false	}; } // Parameter
PDA_HEADER(TemplateParam)	()										{	return {	nullptr,	nullptr,	false,			DeclaratorRestriction::Optional,	InitializerRestriction::Optional,	false,			true,			false,		false,	false	}; } // Parameter
PDA_HEADER(Decls)			(bool allowBitField, bool allowComma)	{	return {	nullptr,	nullptr,	false,			DeclaratorRestriction::Many,		InitializerRestriction::Optional,	allowBitField,	false,			allowComma,	true,	true	}; } // Declarations
PDA_HEADER(Typedefs)		()										{	return {	nullptr,	nullptr,	false,			DeclaratorRestriction::Many,		InitializerRestriction::Zero,		false,			false,			false,		false,	false	}; } // Declarations after typedef keyword
#undef PDA_HEADER

extern void											ParseMemberDeclarator(const ParsingArguments& pa, const ParsingDeclaratorArguments& pda, Ptr<CppTokenCursor>& cursor, List<Ptr<Declarator>>& declarators);
extern void											ParseNonMemberDeclarator(const ParsingArguments& pa, const ParsingDeclaratorArguments& pda, Ptr<Type> type, Ptr<CppTokenCursor>& cursor, List<Ptr<Declarator>>& declarators);
extern void											ParseNonMemberDeclarator(const ParsingArguments& pa, const ParsingDeclaratorArguments& pda, Ptr<CppTokenCursor>& cursor, List<Ptr<Declarator>>& declarators);
extern Ptr<Declarator>								ParseNonMemberDeclarator(const ParsingArguments& pa, const ParsingDeclaratorArguments& pda, Ptr<CppTokenCursor>& cursor);
extern Ptr<Type>									ParseType(const ParsingArguments& pa, Ptr<CppTokenCursor>& cursor);

// Parser_Template.cpp
extern void											ParseTemplateSpec(const ParsingArguments& pa, Ptr<CppTokenCursor>& cursor, Ptr<Symbol>& specSymbol, Ptr<TemplateSpec>& spec);
extern void											ValidateForRootTemplateSpec(Ptr<TemplateSpec>& spec, Ptr<CppTokenCursor>& cursor, bool forPS, bool forFunction);
extern void											ParseSpecializationSpec(const ParsingArguments& pa, Ptr<CppTokenCursor>& cursor, Ptr<SpecializationSpec>& spec);
extern void											ParseGenericArgumentsSkippedLT(const ParsingArguments& pa, Ptr<CppTokenCursor>& cursor, VariadicList<GenericArgument>& arguments, CppTokens ending, bool allowVariadicOnAllArguments = true);

// Parser_Declaration.cpp
extern void											ParseDeclaration(const ParsingArguments& pa, Ptr<CppTokenCursor>& cursor, List<Ptr<Declaration>>& output);
extern void											BuildVariables(List<Ptr<Declarator>>& declarators, List<Ptr<VariableDeclaration>>& varDecls);
extern void											BuildSymbols(const ParsingArguments& pa, List<Ptr<VariableDeclaration>>& varDecls, Ptr<CppTokenCursor>& cursor);
extern void											BuildSymbols(const ParsingArguments& pa, VariadicList<Ptr<VariableDeclaration>>& varDecls, Ptr<CppTokenCursor>& cursor);
extern void											BuildVariablesAndSymbols(const ParsingArguments& pa, List<Ptr<Declarator>>& declarators, List<Ptr<VariableDeclaration>>& varDecls, Ptr<CppTokenCursor>& cursor);
extern Ptr<VariableDeclaration>						BuildVariableAndSymbol(const ParsingArguments& pa, Ptr<Declarator> declarator, Ptr<CppTokenCursor>& cursor);

// Parser_Expr.cpp
struct ParsingExprArguments
{
	bool											allowComma;
	bool											allowGt;
};

inline ParsingExprArguments							pea_Full()
	{	return { true, true }; }
inline ParsingExprArguments							pea_Argument()
	{	return { false, true }; }
inline ParsingExprArguments							pea_GenericArgument()
	{	return { false, false }; }

extern Ptr<Expr>									ParseExpr(const ParsingArguments& pa, const ParsingExprArguments& pea, Ptr<CppTokenCursor>& cursor);

// Parser_Stat.cpp
extern Ptr<Stat>									ParseStat(const ParsingArguments& pa, Ptr<CppTokenCursor>& cursor);

// Parser.cpp
extern void											EnsureFunctionBodyParsed(FunctionDeclaration* funcDecl);
extern bool											ParseTypeOrExpr(const ParsingArguments& pa, const ParsingExprArguments& pea, Ptr<CppTokenCursor>& cursor, Ptr<Type>& type, Ptr<Expr>& expr);
extern Ptr<Program>									ParseProgram(ParsingArguments& pa, Ptr<CppTokenCursor>& cursor);
extern Symbol*										FindParentClassSymbol(Symbol* symbol, bool includeThisSymbol);

/***********************************************************************
Helpers
***********************************************************************/

// Test if the next token's content matches the expected value
__forceinline bool TestToken(Ptr<CppTokenCursor>& cursor, const wchar_t* content, bool autoSkip = true)
{
	vint length = (vint)wcslen(content);
	if (cursor && cursor->token.length == length && wcsncmp(cursor->token.reading, content, length) == 0)
	{
		if (autoSkip) cursor = cursor->Next();
		return true;
	}
	return false;
}

// Test if the next token's type matches the expected value
__forceinline bool TestToken(Ptr<CppTokenCursor>& cursor, CppTokens token1, bool autoSkip = true)
{
	if (cursor && (CppTokens)cursor->token.token == token1)
	{
		if (autoSkip) cursor = cursor->Next();
		return true;
	}
	return false;
}

#define TEST_AND_SKIP(TOKEN)\
	if (TestToken(current, TOKEN, false) && current->token.start == start)\
	{\
		start += current->token.length;\
		current = current->Next();\
	}\
	else\
	{\
		return false;\
	}\

// Test if next two tokens' types match expected value, and there should not be spaces between tokens
__forceinline bool TestToken(Ptr<CppTokenCursor>& cursor, CppTokens token1, CppTokens token2, bool autoSkip = true)
{
	if (auto current = cursor)
	{
		vint start = current->token.start;
		TEST_AND_SKIP(token1);
		TEST_AND_SKIP(token2);
		if (autoSkip) cursor = current;
		return true;
	}
	return false;
}

// Test if next three tokens' types match expected value, and there should not be spaces between tokens
__forceinline bool TestToken(Ptr<CppTokenCursor>& cursor, CppTokens token1, CppTokens token2, CppTokens token3, bool autoSkip = true)
{
	if (auto current = cursor)
	{
		vint start = current->token.start;
		TEST_AND_SKIP(token1);
		TEST_AND_SKIP(token2);
		TEST_AND_SKIP(token3);
		if (autoSkip) cursor = current;
		return true;
	}
	return false;
}

// Throw exception if failed to test
__forceinline void RequireToken(Ptr<CppTokenCursor>& cursor, const wchar_t* content)
{
	if (!TestToken(cursor, content))
	{
		throw StopParsingException(cursor);
	}
}

// Throw exception if failed to test
__forceinline void RequireToken(Ptr<CppTokenCursor>& cursor, CppTokens token1)
{
	if (!TestToken(cursor, token1))
	{
		throw StopParsingException(cursor);
	}
}

// Throw exception if failed to test
__forceinline void RequireToken(Ptr<CppTokenCursor>& cursor, CppTokens token1, CppTokens token2)
{
	if (!TestToken(cursor, token1, token2))
	{
		throw StopParsingException(cursor);
	}
}

// Throw exception if failed to test
__forceinline void RequireToken(Ptr<CppTokenCursor>& cursor, CppTokens token1, CppTokens token2, CppTokens token3)
{
	if (!TestToken(cursor, token1, token2, token3))
	{
		throw StopParsingException(cursor);
	}
}

// Skip one token
__forceinline void SkipToken(Ptr<CppTokenCursor>& cursor)
{
	if (cursor)
	{
		cursor = cursor->Next();
	}
	else
	{
		throw StopParsingException(cursor);
	}
}

#endif