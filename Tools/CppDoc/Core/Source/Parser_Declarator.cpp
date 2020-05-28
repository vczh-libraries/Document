#include "Parser.h"
#include "Parser_Declarator.h"
#include "Ast_Type.h"
#include "Ast_Decl.h"

/***********************************************************************
ReplaceOutOfDeclaratorTypeVisitor
***********************************************************************/

template<typename TCreator>
class ReplaceOutOfDeclaratorTypeVisitor : public Object, public virtual ITypeVisitor
{
private:
	Ptr<CppTokenCursor>&			cursor;
	Ptr<Type>						typeToReplace;
	TCreator						typeCreator;

public:
	Ptr<Type>						createdType;

	ReplaceOutOfDeclaratorTypeVisitor(Ptr<CppTokenCursor>& _cursor, Ptr<Type> _typeToReplace, TCreator&& _typeCreator)
		:cursor(_cursor)
		, typeToReplace(_typeToReplace)
		, typeCreator(ForwardValue<TCreator>(_typeCreator))
	{
	}

	void Execute(Ptr<Type>& targetType)
	{
		if (targetType == typeToReplace)
		{
			createdType = typeCreator(targetType);
			targetType = createdType;
		}
		else
		{
			targetType->Accept(this);
		}
	}

	void Visit(PrimitiveType* self)override
	{
	}

	void Visit(ReferenceType* self)override
	{
		Execute(self->type);
	}

	void Visit(ArrayType* self)override
	{
		Execute(self->type);
	}

	void Visit(DecorateType* self)override
	{
		Execute(self->type);
	}

	void Visit(CallingConventionType* self)override
	{
		Execute(self->type);
	}

	void Visit(FunctionType* self)override
	{
		Execute(self->returnType);
	}

	void Visit(MemberType* self)override
	{
		Execute(self->type);
	}

	void Visit(DeclType* self)override
	{
	}

	void Visit(RootType* self)override
	{
	}

	void Visit(IdType* self)override
	{
	}

	void Visit(ChildType* self)override
	{
	}

	void Visit(GenericType* self)override
	{
	}
};

template<typename T>
ReplaceOutOfDeclaratorTypeVisitor<T> MakeReplacer(Ptr<CppTokenCursor>& _cursor, Ptr<Type> _typeToReplace, T&& _typeCreator)
{
	return { _cursor,_typeToReplace,ForwardValue<T>(_typeCreator) };
}

/***********************************************************************
ParseDeclaratorContext
***********************************************************************/

struct ParseDeclaratorContext
{
	PARSING_DECLARATOR_ARGUMENTS(, ;);
	bool					forceSpecialMethod;

	ParseDeclaratorContext(const ParsingDeclaratorArguments& pda, bool _forceSpecialMethod)
		: PARSING_DECLARATOR_COPY(pda.)
		, forceSpecialMethod(_forceSpecialMethod)
	{
	}
};

/***********************************************************************
ParseDeclaratorName
***********************************************************************/

bool ParseDeclaratorName(const ParsingArguments& pa, CppName& cppName, Ptr<Type> targetType, const ParseDeclaratorContext& pdc, Ptr<CppTokenCursor>& cursor)
{
	// forceSpecialMethod means this function is expected to accept only
	//   constructor declarators
	//   destructor declarators
	//   type conversion declarators
	// the common thing among them is not having a return type before the declarator
	if (ParseCppName(cppName, cursor, pdc.forceSpecialMethod))
	{
		if (pdc.forceSpecialMethod)
		{
			// special method can only appear
			//   when a declaration is right inside a class
			//   or it is a class member declaration out of a class

			WString containingClassName;
			if (pdc.classOfMemberInside)
			{
				containingClassName = pdc.classOfMemberInside->name.name;
			}
			else
			{
				auto classType = pdc.classOfMemberOutside;
				if (!classType)
				{
					if (auto memberType = targetType.Cast<MemberType>())
					{
						classType = memberType->classType;
					}
				}

				if (auto genericType = classType.Cast<GenericType>())
				{
					classType = genericType->type;
				}

				if (auto idType = classType.Cast<IdType>())
				{
					containingClassName = idType->name.name;
				}
				else if (auto childType = classType.Cast<ChildType>())
				{
					containingClassName = childType->name.name;
				}
			}

			switch (cppName.type)
			{
			case CppNameType::Normal:
				// IDENTIFIER should be a constructor name for a special method
				if (cppName.name == containingClassName && TestToken(cursor, CppTokens::LPARENTHESIS, false))
				{
					cppName.name = L"$__ctor";
					cppName.type = CppNameType::Constructor;
				}
				else
				{
					throw StopParsingException(cursor);
				}
				break;
			case CppNameType::Operator:
				// operator TYPE is assembled in ParseDeclaratorWithInitializer
				throw StopParsingException(cursor);
			case CppNameType::Destructor:
				// ~IDENTIFIER should be a destructor name for a special method
				if (cppName.name != L"~" + containingClassName)
				{
					throw StopParsingException(cursor);
				}
				break;
			}
		}
		else
		{
			switch (cppName.type)
			{
			case CppNameType::Operator:
				// operator PREDEFINED-OPERATOR is the only valid form for normal method if the first token is operator
				if (cppName.tokenCount == 1)
				{
					throw StopParsingException(cursor);
				}
				break;
			case CppNameType::Constructor:
			case CppNameType::Destructor:
				// constructors or destructors are not normal methods
				throw StopParsingException(cursor);
			}
		}
		return true;
	}
	else
	{
		return false;
	}
}

/***********************************************************************
ParseTypeBeforeDeclarator
***********************************************************************/

Ptr<Type> ParseTypeBeforeDeclarator(const ParsingArguments& pa, Ptr<Type> baselineType, const ParseDeclaratorContext& pdc, Ptr<CppTokenCursor>& cursor)
{
	if (TestToken(cursor, CppTokens::ALIGNAS))
	{
		// alignas (EXPRESSION) DECLARATOR
		RequireToken(cursor, CppTokens::LPARENTHESIS);
		ParseExpr(pa, pea_Argument(), cursor);
		RequireToken(cursor, CppTokens::RPARENTHESIS);
		return ParseTypeBeforeDeclarator(pa, baselineType, pdc, cursor);
	}
	else if (TestToken(cursor, CppTokens::MUL))
	{
		// * DECLARATOR
		// * __ptr32 DECLARATOR
		// * __ptr64 DECLARATOR
		TestToken(cursor, CppTokens::__PTR32) || TestToken(cursor, CppTokens::__PTR64);
		auto type = MakePtr<ReferenceType>();
		type->reference = CppReferenceType::Ptr;
		type->type = baselineType;
		return ParseTypeBeforeDeclarator(pa, type, pdc, cursor);
	}
	else if (TestToken(cursor, CppTokens::AND, CppTokens::AND))
	{
		// && DECLARATOR
		auto type = MakePtr<ReferenceType>();
		type->reference = CppReferenceType::RRef;
		type->type = baselineType;
		return ParseTypeBeforeDeclarator(pa, type, pdc, cursor);
	}
	else if (TestToken(cursor, CppTokens::AND))
	{
		// & DECLARATOR
		auto type = MakePtr<ReferenceType>();
		type->reference = CppReferenceType::LRef;
		type->type = baselineType;
		return ParseTypeBeforeDeclarator(pa, type, pdc, cursor);
	}
	else if (TestToken(cursor, CppTokens::CONST))
	{
		// const DECLARATOR
		// Cannot use AddCVType because
		// for nested ParseSingleDeclarator call
		// the targetType variable for calling ParseSingleDeclarator_Array or ParseSingleDeclarator_Function
		// may become not part in the final type chain of the declarator
		// DecorateType and ArrayType will be adjusted at the end
		auto type = MakePtr<DecorateType>();
		type->isConst = true;
		type->type = baselineType;
		return ParseTypeBeforeDeclarator(pa, type, pdc, cursor);
	}
	else if (TestToken(cursor, CppTokens::VOLATILE))
	{
		// volatile DECLARATOR
		auto type = MakePtr<DecorateType>();
		type->isVolatile = true;
		type->type = baselineType;
		return ParseTypeBeforeDeclarator(pa, type, pdc, cursor);
	}
	else
	{
		{
			auto oldCursor = cursor;
			TsysCallingConvention callingConvention;
			if (ParseCallingConvention(callingConvention, cursor))
			{
				if (TestToken(cursor, CppTokens::LPARENTHESIS, false))
				{
					// for __stdcall (
					// because there is no __stdcall (DECLARATOR), so it can only be __stdcall ( { PARAMETER ,...} )
					// so this __stdcall will be consumed by ParseLongDeclarator, so quit here
					// if any declarator name is required, an exception will be raised below
					cursor = oldCursor;
				}
				else
				{
					// __stdcall DECLARATOR
					auto type = MakePtr<CallingConventionType>();
					type->callingConvention = callingConvention;
					type->type = baselineType;
					return ParseTypeBeforeDeclarator(pa, type, pdc, cursor);
				}
			}
		}

		Ptr<Type> classType;
		{
			auto oldCursor = cursor;
			try
			{
				classType = ParseLongType(pa, cursor);
				RequireToken(cursor, CppTokens::COLON, CppTokens::COLON);
			}
			catch (const StopParsingException&)
			{
				classType = nullptr;
				cursor = oldCursor;
			}
		}

		if (classType)
		{
			// CLASS:: DECLARATOR
			auto type = MakePtr<MemberType>();
			type->classType = classType;
			type->type = baselineType;
			return ParseTypeBeforeDeclarator(pa, type, pdc, cursor);
		}
		else
		{
			return baselineType;
		}
	}
}

/***********************************************************************
ParseSingleDeclarator_Array
***********************************************************************/

bool ParseSingleDeclarator_Array(const ParsingArguments& pa, Ptr<Declarator> declarator, Ptr<Type> targetType, bool forParameter, Ptr<CppTokenCursor>& cursor)
{
	if (TestToken(cursor, CppTokens::LBRACKET))
	{
		Ptr<Expr> index;
		if (!TestToken(cursor, CppTokens::RBRACKET, false))
		{
			index = ParseExpr(pa, pea_Full(), cursor);
		}

		auto replacer = MakeReplacer(
			cursor,
			targetType,
			[=](Ptr<Type> typeToReplace)->Ptr<Type>
			{
				if (index || !forParameter)
				{
					auto type = MakePtr<ArrayType>();
					type->type = typeToReplace;
					type->expr = index;
					return type;
				}
				else
				{
					auto type = MakePtr<ReferenceType>();
					type->reference = CppReferenceType::Ptr;
					type->type = typeToReplace;
					return type;
				}
			}
		);
		replacer.Execute(declarator->type);

		RequireToken(cursor, CppTokens::RBRACKET);
		return true;
	}
	else
	{
		// there is no something like []( { PARAMETERS, ...} ), so we can stop here
		return false;
	}
}

/***********************************************************************
ParseSingleDeclarator_Function
***********************************************************************/

bool ParseSingleDeclarator_Function(const ParsingArguments& pa, Ptr<Declarator> declarator, Ptr<Type> targetType, bool forceSpecialMethod, bool allowSpecializationSpec, Ptr<CppTokenCursor>& cursor)
{
	// if it is not an array declarator, then there are only two possibilities
	//   1. it is a function declarator
	//   2. it is a declarator but not array or function
	// so we see if we can find __stdcall

	auto oldCursor = cursor;

	Ptr<SpecializationSpec> spec;
	auto callingConvention = TsysCallingConvention::None;

	if (allowSpecializationSpec)
	{
		ParseSpecializationSpec(pa, cursor, spec);
	}
	ParseCallingConvention(callingConvention, cursor);

	if (TestToken(cursor, CppTokens::LPARENTHESIS, false))
	{
		if (spec)
		{
			if (!declarator->name)
			{
				throw StopParsingException(cursor);
			}
			declarator->specializationSpec = spec;
		}

		if (!forceSpecialMethod || callingConvention != TsysCallingConvention::None)
		{
			// if we see (EXPRESSION, then this is an initializer, we stop here.
			// for special method, it should be a function, so we don't need to test for an initializer.
			// and there is also not possible to have an expression right after __stdcall.
			auto oldCursor = cursor;
			try
			{
				SkipToken(cursor);
				ParseExpr(pa, pea_Argument(), cursor);
				cursor = oldCursor;
				return true;
			}
			catch (const StopParsingException&)
			{
				cursor = oldCursor;
			}
		}

		SkipToken(cursor);
		Ptr<FunctionType> type;
		if (callingConvention != TsysCallingConvention::None)
		{
			// __stdcall should appear before "CLASS::"
			// so if we see "__stdcall (" here, then there is no "CLASS::" (maybe "CLASS::*" but we don't care)
			// we can simply decorate the function with __stdcall, no need to worry about adjusting the return type
			if (declarator->type != targetType)
			{
				throw StopParsingException(cursor);
			}

			type = MakePtr<FunctionType>();
			type->scopeSymbolToReuse = declarator->scopeSymbolToReuse;
			type->returnType = declarator->type;

			auto ccType = MakePtr<CallingConventionType>();
			ccType->callingConvention = callingConvention;
			ccType->type = type;
			declarator->type = ccType;
		}
		else
		{
			// otherwise, it is possible that the return type has already been decorated with __stdcall or CLASS::
			// we should move them out of the return type, and decorate the function type with them
			auto replacer = MakeReplacer(
				cursor,
				targetType,
				[declarator](Ptr<Type> typeToReplace)->Ptr<Type>
				{
					auto type = MakePtr<FunctionType>();
					type->scopeSymbolToReuse = declarator->scopeSymbolToReuse;
					type->returnType = typeToReplace;
					return AdjustReturnTypeWithMemberAndCC(type);
				}
			);

			replacer.Execute(declarator->type);
			type = GetTypeWithoutMemberAndCC(replacer.createdType).Cast<FunctionType>();
		}
		// here type is the FunctionType we are going to add information to, it is possible that type is not declarator->type

		{
			// (void) is treated as no parameter
			auto oldCursor = cursor;
			if (TestToken(cursor, CppTokens::TYPE_VOID) && TestToken(cursor, CppTokens::RPARENTHESIS))
			{
				goto FINISH_PARAMETER_LIST;
			}
			else
			{
				cursor = oldCursor;
			}
		}

		{
			// recognize parameters
			if (!TestToken(cursor, CppTokens::RPARENTHESIS))
			{
				while (true)
				{
					if (TestToken(cursor, CppTokens::DOT, CppTokens::DOT, CppTokens::DOT))
					{
						RequireToken(cursor, CppTokens::RPARENTHESIS);
						type->ellipsis = true;
						break;
					}

					{
						List<Ptr<Declarator>> declarators;
						while (SkipSpecifiers(cursor));
						declarators.Add(ParseNonMemberDeclarator(pa, pda_Param(true), cursor));

						List<Ptr<VariableDeclaration>> varDecls;
						BuildVariables(declarators, varDecls);
						type->parameters.Add({ varDecls[0],declarators[0]->ellipsis });

						// build parameter symbols to declarator->scopeSymbolToReuse, so that decltype(...) is correctly handled
						BuildSymbols(pa, varDecls, cursor);
					}

					if (!TestToken(cursor, CppTokens::COMMA))
					{
						RequireToken(cursor, CppTokens::RPARENTHESIS);
						break;
					}
				}
			}
		}
	FINISH_PARAMETER_LIST:

		while (true)
		{
			// we can put these things after a function declarator, they decorate *this, not any type inside the function
			//   constexpr
			//   const
			//   volatile
			//   &&
			//   &
			if (TestToken(cursor, CppTokens::CONSTEXPR))
			{
				type->qualifierConstExpr = true;
			}
			else if (TestToken(cursor, CppTokens::CONST))
			{
				type->qualifierConst = true;
			}
			else if (TestToken(cursor, CppTokens::VOLATILE))
			{
				type->qualifierVolatile = true;
			}
			else if (TestToken(cursor, CppTokens::AND, CppTokens::AND))
			{
				type->qualifierRRef = true;
			}
			else if (TestToken(cursor, CppTokens::AND))
			{
				type->qualifierLRef = true;
			}
			else if (TestToken(cursor, CppTokens::OVERRIDE))
			{
				// override
				type->decoratorOverride = true;
			}
			else if (TestToken(cursor, CppTokens::SUB, CppTokens::GT))
			{
				// auto SOMETHING -> TYPE
				if (auto primitiveType = type->returnType.Cast<PrimitiveType>())
				{
					if (primitiveType->primitive == CppPrimitiveType::_auto)
					{
						goto CONTINUE_RETURN_TYPE;
					}
				}
				throw StopParsingException(cursor);
			CONTINUE_RETURN_TYPE:

				type->decoratorReturnType = ParseType(pa, cursor);
			}
			else if (TestToken(cursor, CppTokens::NOEXCEPT))
			{
				if (TestToken(cursor, CppTokens::LPARENTHESIS))
				{
					ParseExpr(pa, pea_Full(), cursor);
					RequireToken(cursor, CppTokens::RPARENTHESIS);
				}
				// noexcept
				type->decoratorNoExcept = true;
			}
			else if (TestToken(cursor, CppTokens::THROW))
			{
				// throw ( { TYPE , ...} )
				type->decoratorThrow = true;

				RequireToken(cursor, CppTokens::LPARENTHESIS);
				if (!TestToken(cursor, CppTokens::RPARENTHESIS))
				{
					while (true)
					{
						type->exceptions.Add(ParseType(pa, cursor));

						if (!TestToken(cursor, CppTokens::COMMA))
						{
							RequireToken(cursor, CppTokens::RPARENTHESIS);
							break;
						}
					}
				}
			}
			else
			{
				return true;
			}
		}
	}
	else
	{
		cursor = oldCursor;
		return false;
	}
}

/***********************************************************************
ParseSingleDeclarator
***********************************************************************/

template<typename TCallback>
void InjectClassMemberCacheIfNecessary(const ParsingArguments& pa, const ParseDeclaratorContext& pdc, Ptr<Declarator> declarator, Ptr<CppTokenCursor>& cursor, TCallback&& callback)
{
	if (pdc.scopeSymbolToReuse)
	{
		if (pdc.scopeSymbolToReuse != pa.scopeSymbol)
		{
			throw StopParsingException(cursor);
		}
	}

	bool setClassMemberCacheToCurrentScope = false;
	if (!declarator->scopeSymbolToReuse)
	{
		// always ensure a symbol to reuse, since this declarator could be a function
		if (pdc.scopeSymbolToReuse)
		{
			declarator->scopeSymbolToReuse = pdc.scopeSymbolToReuse;
			if (declarator->classMemberCache && IsInTemplateHeader(pa))
			{
				// this scope is created from a template header
				// if we create a new symbol inside this scope, its parentScope will skip this scope
				pa.scopeSymbol->SetClassMemberCacheForTemplateSpecScope_N(declarator->classMemberCache);
				setClassMemberCacheToCurrentScope = true;
			}
		}
		else
		{
			declarator->scopeSymbolToReuse = MakePtr<Symbol>(pa.scopeSymbol, declarator->classMemberCache);
		}
	}

	auto newPa = pdc.scopeSymbolToReuse ? pa : pa.WithScope(declarator->scopeSymbolToReuse.Obj());
	callback(newPa);

	if (setClassMemberCacheToCurrentScope)
	{
		pa.scopeSymbol->SetClassMemberCacheForTemplateSpecScope_N(nullptr);
	}
}

Ptr<Declarator> ParseSingleDeclarator(const ParsingArguments& pa, Ptr<Type> baselineType, const ParseDeclaratorContext& pdc, Ptr<CppTokenCursor>& cursor)
{
	Ptr<Declarator> declarator;

	// a long declarator begins with more type decorations
	auto targetType = ParseTypeBeforeDeclarator(pa, baselineType, pdc, cursor);
	bool ellipsis = pdc.allowEllipsis ? TestToken(cursor, CppTokens::DOT, CppTokens::DOT, CppTokens::DOT) : false;

	if (pdc.dr != DeclaratorRestriction::Zero)
	{
		while (SkipSpecifiers(cursor));

		// there may be a declarator name
		CppName cppName;
		if (ParseDeclaratorName(pa, cppName, targetType, pdc, cursor))
		{
			declarator = new Declarator;
			declarator->type = targetType;
			declarator->ellipsis = ellipsis;
			declarator->name = cppName;
		}
	}

	if (pdc.allowBitField && TestToken(cursor, CppTokens::COLON))
	{
		// bit fields, just ignore
		ParseExpr(pa, pea_Argument(), cursor);
	}

	if (!declarator)
	{
		auto oldCursor = cursor;
		// if there is no declarator name, and the next token is (
		// then a declarator is possible to be placed after (, and it is also possible that this declarator still doesn't have a name

		// but this is not the case when we see ()
		if (TestToken(cursor, CppTokens::LPARENTHESIS) && TestToken(cursor, CppTokens::RPARENTHESIS))
		{
			cursor = oldCursor;
			goto READY_FOR_ARRAY_OR_FUNCTION;
		}

		// but this is not the case when we see (void)
		cursor = oldCursor;
		if (TestToken(cursor, CppTokens::LPARENTHESIS) && TestToken(cursor, CppTokens::TYPE_VOID) && TestToken(cursor, CppTokens::RPARENTHESIS))
		{
			cursor = oldCursor;
			goto READY_FOR_ARRAY_OR_FUNCTION;
		}

		// try to get a declarator name, if failed then we ignore and we assume there are parameters after (
		cursor = oldCursor;
		if (TestToken(cursor, CppTokens::LPARENTHESIS))
		{
			try
			{
				declarator = ParseSingleDeclarator(pa, targetType, pdc, cursor);
				RequireToken(cursor, CppTokens::RPARENTHESIS);
			}
			catch (const StopParsingException&)
			{
				declarator = nullptr;
				cursor = oldCursor;
				goto READY_FOR_ARRAY_OR_FUNCTION;
			}
		}
	}

READY_FOR_ARRAY_OR_FUNCTION:
	if (!declarator)
	{
		declarator = new Declarator;
		declarator->type = targetType;
		declarator->ellipsis = ellipsis;
	}

	// check if we have already done with the declarator name
	if (!declarator->name && pdc.dr == DeclaratorRestriction::One)
	{
		throw StopParsingException(cursor);
	}

	// recognize a class member declaration
	if (pdc.allowClassMemberCache)
	{
		if (pdc.classOfMemberInside)
		{
			declarator->classMemberCache = CreatePartialClassMemberCache(pa, pdc.classOfMemberInside->symbol, cursor);
		}
		else if (pdc.classOfMemberOutside)
		{
			declarator->classMemberCache = CreatePartialClassMemberCache(pa, pdc.classOfMemberOutside, pdc.specsOfMemberOutside, cursor);
		}
		else if (auto memberType = declarator->type.Cast<MemberType>())
		{
			declarator->classMemberCache = CreatePartialClassMemberCache(pa, memberType->classType, pdc.specsOfMemberOutside, cursor);
		}
	}

	InjectClassMemberCacheIfNecessary(pa, pdc, declarator, cursor, [&](const ParsingArguments& newPa)
	{
		// if there is [, we see an array declarator
		// an array could be multiple dimension
		if (TestToken(cursor, CppTokens::LBRACKET, false))
		{
			while (ParseSingleDeclarator_Array(newPa, declarator, targetType, pdc.forParameter, cursor));
		}
		else
		{
			ParseSingleDeclarator_Function(newPa, declarator, targetType, pdc.forceSpecialMethod, pdc.allowSpecializationSpec, cursor);
		}
	});
	return declarator;
}

/***********************************************************************
ParseInitializer
***********************************************************************/

Ptr<Initializer> ParseInitializer(const ParsingArguments& pa, Ptr<CppTokenCursor>& cursor, bool allowComma, bool allowGT)
{
	// = EXPRESSION
	// { { EXPRESSION , ...} }
	// ( { EXPRESSSION , ...} )
	auto initializer = MakePtr<Initializer>();

	if (TestToken(cursor, CppTokens::EQ))
	{
		initializer->initializerType = CppInitializerType::Equal;
	}
	else if (TestToken(cursor, CppTokens::LBRACE))
	{
		initializer->initializerType = CppInitializerType::Universal;
		if (TestToken(cursor, CppTokens::RBRACE))
		{
			return initializer;
		}
	}
	else if (TestToken(cursor, CppTokens::LPARENTHESIS))
	{
		initializer->initializerType = CppInitializerType::Constructor;
		if (TestToken(cursor, CppTokens::RPARENTHESIS))
		{
			return initializer;
		}
	}
	else
	{
		throw StopParsingException(cursor);
	}

	while (true)
	{
		auto argument = ParseExpr(pa, { allowComma,allowGT }, cursor);
		bool isVariadic = TestToken(cursor, CppTokens::DOT, CppTokens::DOT, CppTokens::DOT);
		initializer->arguments.Add({ argument,isVariadic });

		if (initializer->initializerType == CppInitializerType::Equal)
		{
			break;
		}
		else if (!TestToken(cursor, CppTokens::COMMA))
		{
			switch (initializer->initializerType)
			{
			case CppInitializerType::Universal:
				RequireToken(cursor, CppTokens::RBRACE);
				break;
			case CppInitializerType::Constructor:
				RequireToken(cursor, CppTokens::RPARENTHESIS);
				break;
			}
			break;
		}
	}

	return initializer;
}

/***********************************************************************
ParseDeclaratorWithInitializer
***********************************************************************/

void ParseDeclaratorWithInitializer(const ParsingArguments& pa, Ptr<Type> typeResult, const ParseDeclaratorContext& pdc, Ptr<CppTokenCursor>& cursor, List<Ptr<Declarator>>& declarators)
{
	while (true)
	{
		Ptr<Declarator> declarator;

		// try (TYPE::)operator TYPE
		Ptr<MemberType> typeOpMemberType;
		auto typeOpCursor = cursor;

		if (pdc.forceSpecialMethod)
		{
			try
			{
				auto type = ParseLongType(pa, cursor);
				RequireToken(cursor, CppTokens::COLON, CppTokens::COLON);
				typeOpMemberType = MakePtr<MemberType>();
				typeOpMemberType->classType = type;
			}
			catch (const StopParsingException&)
			{
				cursor = typeOpCursor;
			}
		}

		if (pdc.forceSpecialMethod && TestToken(cursor, CppTokens::OPERATOR, false))
		{
			if (declarators.Count() > 0)
			{
				throw StopParsingException(cursor);
			}

			CppName cppName;
			cppName.type = CppNameType::Operator;
			cppName.name = L"$__type";
			cppName.tokenCount = 1;
			cppName.nameTokens[0] = cursor->token;
			SkipToken(cursor);

			auto opPdc = pda_Decls(false, false);
			opPdc.classOfMemberInside = pdc.classOfMemberInside;
			opPdc.classOfMemberOutside = pdc.classOfMemberOutside;
			opPdc.scopeSymbolToReuse = pdc.scopeSymbolToReuse;
			if (typeOpMemberType && !opPdc.classOfMemberInside && !opPdc.classOfMemberOutside)
			{
				opPdc.classOfMemberOutside = typeOpMemberType->classType;
			}
			opPdc.dr = DeclaratorRestriction::Zero;

			typeResult = ParseLongType(pa, cursor);
			declarator = ParseSingleDeclarator(pa, typeResult, { opPdc,false }, cursor);
			declarator->name = cppName;
			if (typeOpMemberType)
			{
				typeOpMemberType->type = declarator->type;
				declarator->type = typeOpMemberType;
			}
		}
		else
		{
			if (typeOpMemberType)
			{
				typeOpMemberType = nullptr;
				cursor = typeOpCursor;
			}

			// if we have already recognize a type, we can parse multiple declarators with initializers
			auto newPdc = pdc;
			newPdc.dr = pdc.dr == DeclaratorRestriction::Many ? DeclaratorRestriction::One : pdc.dr;
			declarator = ParseSingleDeclarator(pa, typeResult, newPdc, cursor);
		}

		InjectClassMemberCacheIfNecessary(pa, pdc, declarator, cursor, [&](const ParsingArguments& newPa)
		{
			// function doesn't have initializer
			bool isFunction = GetTypeWithoutMemberAndCC(declarator->type).Cast<FunctionType>();
			if (!isFunction && declarator->name.type == CppNameType::Operator && declarator->name.tokenCount == 1)
			{
				throw StopParsingException(cursor);
			}

			Ptr<SpecializationSpec> spec;
			if (pdc.allowSpecializationSpec)
			{
				ParseSpecializationSpec(newPa, cursor, spec);
			}

			if (pdc.ir == InitializerRestriction::Optional && !isFunction)
			{
				if (TestToken(cursor, CppTokens::EQ, false) || TestToken(cursor, CppTokens::LPARENTHESIS, false))
				{
					declarator->initializer = ParseInitializer(newPa, cursor, pdc.allowComma, pdc.allowGtInInitializer);
				}
				else if (TestToken(cursor, CppTokens::LBRACE, false))
				{
					declarator->initializer = ParseInitializer(newPa, cursor, pdc.allowComma, true);
				}
			}

			if (spec)
			{
				if (!pdc.scopeSymbolToReuse)
				{
					throw StopParsingException(cursor);
				}
				if (!declarator->initializer || declarator->initializer->initializerType != CppInitializerType::Equal)
				{
					throw StopParsingException(cursor);
				}
				declarator->specializationSpec = spec;
			}
		});
		declarators.Add(declarator);

		// operator TYPE declarator could not be DeclaratorRestriction::Many
		if (declarator->name.type == CppNameType::Operator && declarator->name.tokenCount == 1)
		{
			break;
		}

		if (pdc.dr != DeclaratorRestriction::Many)
		{
			break;
		}
		else if (!TestToken(cursor, CppTokens::COMMA))
		{
			break;
		}
		else
		{
			// skip qualifiers after ","
			while (TestToken(cursor, CppTokens::CONST, false) || TestToken(cursor, CppTokens::VOLATILE, false))
			{
				SkipToken(cursor);
			}
		}
	}

	for (vint i = 0; i < declarators.Count(); i++)
	{
		auto declarator = declarators[i];
		NormalizeTypeChain(declarator->type);
	}
}

/***********************************************************************
ParseDeclarator
***********************************************************************/

void ParseDeclarator(const ParsingArguments& pa, const ParsingDeclaratorArguments& pda, bool trySpecialMember, Ptr<CppTokenCursor>& cursor, List<Ptr<Declarator>>& declarators)
{
	if (trySpecialMember && pda.dr == DeclaratorRestriction::Many)
	{
		// if we don't see trySpecialMember or Many, then the declarator is not for a declaration, so there is no special method
		auto oldCursor = cursor;
		{
			try
			{
				// try to parse a special method declarator
				ParseDeclaratorWithInitializer(pa, nullptr, { pda,true }, cursor, declarators);
			}
			catch (const StopParsingException&)
			{
				// ignore it if we failed
				goto TRY_NORMAL_DECLARATOR;
			}

			if (declarators.Count() == 0)
			{
				goto TRY_NORMAL_DECLARATOR;
			}
			for (vint i = 0; i < declarators.Count(); i++)
			{
				// the type of a special method should be a (maybe decorated) function, but its return type could be nullptr
				// so if the type is nullptr, then we are definitely failed
				if (!declarators[i]->type)
				{
					declarators.Clear();
					goto TRY_NORMAL_DECLARATOR;
				}
			}

			return;
		}

	TRY_NORMAL_DECLARATOR:
		cursor = oldCursor;
	}

	auto typeResult = ParseLongType(pa, cursor);
	ParseDeclaratorWithInitializer(pa, typeResult, { pda,false }, cursor, declarators);
}

/***********************************************************************
ParseDeclarator (Helpers)
***********************************************************************/

void ParseMemberDeclarator(const ParsingArguments& pa, const ParsingDeclaratorArguments& pda, Ptr<CppTokenCursor>& cursor, List<Ptr<Declarator>>& declarators)
{
	ParseDeclarator(pa, pda, true, cursor, declarators);
}

void ParseNonMemberDeclarator(const ParsingArguments& pa, const ParsingDeclaratorArguments& pda, Ptr<Type> type, Ptr<CppTokenCursor>& cursor, List<Ptr<Declarator>>& declarators)
{
	ParseDeclaratorWithInitializer(pa, type, { pda,false }, cursor, declarators);
}

void ParseNonMemberDeclarator(const ParsingArguments& pa, const ParsingDeclaratorArguments& pda, Ptr<CppTokenCursor>& cursor, List<Ptr<Declarator>>& declarators)
{
	ParseDeclarator(pa, pda, false, cursor, declarators);
}

Ptr<Declarator> ParseNonMemberDeclarator(const ParsingArguments& pa, const ParsingDeclaratorArguments& pda, Ptr<CppTokenCursor>& cursor)
{
	List<Ptr<Declarator>> declarators;
	ParseNonMemberDeclarator(pa, pda, cursor, declarators);
	if (declarators.Count() != 1) throw StopParsingException(cursor);
	return declarators[0];
}

Ptr<Type> ParseType(const ParsingArguments& pa, Ptr<CppTokenCursor>& cursor)
{
	return ParseNonMemberDeclarator(pa, pda_Type(), cursor)->type;
}