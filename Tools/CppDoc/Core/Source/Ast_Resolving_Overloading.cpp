#include "Ast_Resolving.h"

namespace symbol_type_resolving
{
	/***********************************************************************
	TestFunctionQualifier: Match this pointer's and functions' qualifiers
		Returns: Exact, TrivalConversion, Illegal
	***********************************************************************/

	TypeConv TestFunctionQualifier(TsysCV thisCV, TsysRefType thisRef, const ExprTsysItem& funcType)
	{
		if (funcType.symbol)
		{
			if (auto decl = funcType.symbol->GetAnyForwardDecl<ForwardFunctionDeclaration>())
			{
				if (auto declType = GetTypeWithoutMemberAndCC(decl->type).Cast<FunctionType>())
				{
					return ::TestFunctionQualifier(thisCV, thisRef, declType);
				}
			}
		}
		return TypeConvCat::Exact;
	}

	/***********************************************************************
	FilterFieldsAndBestQualifiedFunctions: Filter functions by their qualifiers
	***********************************************************************/

	TypeConv FindMinConv(ArrayBase<TypeConv>& funcChoices)
	{
		auto target = TypeConv::Max();
		for (vint i = 0; i < funcChoices.Count(); i++)
		{
			auto candidate = funcChoices[i];
			if (!candidate.anyInvolved && TypeConv::CompareIgnoreAny(candidate, target) == -1)
			{
				target = candidate;
			}
		}
		return target;
	}

	bool IsFunctionAcceptableByMinConv(TypeConv functionConv, TypeConv minConv)
	{
		if (functionConv.cat == TypeConvCat::Illegal) return false;
		if (functionConv.anyInvolved) return true;
		return TypeConv::CompareIgnoreAny(functionConv, minConv) == 0;
	}

	void FilterFunctionByConv(ExprTsysList& funcTypes, ArrayBase<TypeConv>& funcChoices)
	{
		auto target = FindMinConv(funcChoices);
		for (vint i = funcTypes.Count() - 1; i >= 0; i--)
		{
			if (!IsFunctionAcceptableByMinConv(funcChoices[i], target))
			{
				funcTypes.RemoveAt(i);
			}
		}
	}

	void FilterFieldsAndBestQualifiedFunctions(TsysCV thisCV, TsysRefType thisRef, ExprTsysList& funcTypes)
	{
		Array<TypeConv> funcChoices(funcTypes.Count());

		for (vint i = 0; i < funcTypes.Count(); i++)
		{
			funcChoices[i] = TestFunctionQualifier(thisCV, thisRef, funcTypes[i]);
		}

		FilterFunctionByConv(funcTypes, funcChoices);
	}

	/***********************************************************************
	FindQualifiedFunctors: Remove everything that are not qualified functors (including functions and operator())
	***********************************************************************/

	void FindQualifiedFunctors(const ParsingArguments& pa, TsysCV thisCV, TsysRefType thisRef, ExprTsysList& funcTypes, bool lookForOp)
	{
		ExprTsysList expandedFuncTypes;
		List<TypeConv> funcChoices;

		for (vint i = 0; i < funcTypes.Count(); i++)
		{
			auto funcType = funcTypes[i];
			auto choice = TestFunctionQualifier(thisCV, thisRef, funcType);

			if (choice.cat != TypeConvCat::Illegal)
			{
				TsysCV cv;
				TsysRefType refType;
				auto entityType = funcType.tsys->GetEntity(cv, refType);

				if ((entityType->GetType() == TsysType::Decl || entityType->GetType() == TsysType::DeclInstant) && lookForOp)
				{
					ExprTsysList opResult;
					VisitFunctors(pa, funcType, L"operator ()", opResult);

					vint oldCount = expandedFuncTypes.Count();
					AddNonVar(expandedFuncTypes, opResult);
					vint newCount = expandedFuncTypes.Count();

					for (vint i = 0; i < (newCount - oldCount); i++)
					{
						funcChoices.Add(TypeConvCat::Exact);
					}
				}
				else if (entityType->GetType() == TsysType::GenericFunction)
				{
					if (auto symbol = funcTypes[i].symbol)
					{
						if (symbol->kind == symbol_component::SymbolKind::FunctionSymbol || symbol->kind == symbol_component::SymbolKind::FunctionBodySymbol)
						{
							if (AddInternal(expandedFuncTypes, { funcType.symbol,funcType.type, entityType }))
							{
								funcChoices.Add(choice);
							}
						}
					}
				}
				else if (entityType->GetType() == TsysType::Ptr)
				{
					if (entityType->GetElement()->GetType() == TsysType::Function)
					{
						if (AddInternal(expandedFuncTypes, { funcType.symbol,funcType.type, entityType }))
						{
							funcChoices.Add(choice);
						}
					}
				}
			}
		}

		FilterFunctionByConv(expandedFuncTypes, funcChoices);
		CopyFrom(funcTypes, expandedFuncTypes);
	}

	/***********************************************************************
	VisitOverloadedFunction: Select good candidates from overloaded functions
	***********************************************************************/

	bool IsAcceptableWithVariadicInput(const ParsingArguments& pa, ExprTsysItem funcItem, Array<ExprTsysItem>& argTypes, SortedList<vint>& boundedAnys)
	{
		return true;
	}

	Nullable<ExprTsysItem> InferFunctionType(const ParsingArguments& pa, ExprTsysItem funcType, Array<ExprTsysItem>& argTypes, SortedList<vint>& boundedAnys)
	{
		switch (funcType.tsys->GetType())
		{
		case TsysType::Function:
			return funcType;
		case TsysType::LRef:
		case TsysType::RRef:
		case TsysType::CV:
		case TsysType::Ptr:
			return InferFunctionType(pa, { funcType,funcType.tsys->GetElement() }, argTypes, boundedAnys);
		case TsysType::GenericFunction:
			throw NotConvertableException();
		default:
			return {};
		}
	}

	void VisitOverloadedFunction(const ParsingArguments& pa, ExprTsysList& funcTypes, Array<ExprTsysItem>& argTypes, SortedList<vint>& boundedAnys, ExprTsysList& result, ExprTsysList* selectedFunctions, bool* anyInvolved)
	{
		bool withVariadicInput = boundedAnys.Count() > 0;

		// funcDPs: functionIndex(funcTypes) -> number of parameters with default value
		Array<vint> funcDPs(funcTypes.Count());
		for (vint i = 0; i < funcTypes.Count(); i++)
		{
			funcDPs[i] = 0;
			if (auto symbol = funcTypes[i].symbol)
			{
				if (auto decl = symbol->GetAnyForwardDecl<ForwardFunctionDeclaration>())
				{
					if (auto type = GetTypeWithoutMemberAndCC(decl->type).Cast<FunctionType>())
					{
						for (vint j = 0; j < type->parameters.Count(); j++)
						{
							if (type->parameters[j].isVariadic)
							{
								// TODO: Support selecting overloading functions with variadic template arguments in the future
								throw NotConvertableException();
							}
							if (type->parameters[j].item->initializer)
							{
								funcDPs[i] = type->parameters.Count() - j;
								break;
							}
						}
					}
				}
			}
		}

		ExprTsysList validFuncTypes;

		if (withVariadicInput)
		{
			// if number of arguments is unknown, we only check if a function has too few parameters.
			for (vint i = 0; i < funcTypes.Count(); i++)
			{
				if (auto inferredFuncType = InferFunctionType(pa, funcTypes[i], argTypes, boundedAnys))
				{
					auto funcType = inferredFuncType.Value();
					if (funcType.tsys->GetParamCount() < argTypes.Count() - boundedAnys.Count())
					{
						if (!funcType.tsys->GetFunc().ellipsis)
						{
							continue;
						}
					}
					validFuncTypes.Add(funcType);
				}
			}
		}
		else
		{
			for (vint i = 0; i < funcTypes.Count(); i++)
			{
				if (auto inferredFuncType = InferFunctionType(pa, funcTypes[i], argTypes, boundedAnys))
				{
					auto funcType = inferredFuncType.Value();
					vint funcParamCount = funcType.tsys->GetParamCount();
					vint missParamCount = funcParamCount - argTypes.Count();
					if (missParamCount > 0)
					{
						// if arguments are not enough, we check about parameters with default value
						if (missParamCount > funcDPs[i])
						{
							continue;
						}
					}
					else if (missParamCount < 0)
					{
						// if arguments is too many, we check about ellipsis
						if (!funcType.tsys->GetFunc().ellipsis)
						{
							continue;
						}
					}

					validFuncTypes.Add(funcType);
				}
			}
		}

		Array<bool> selectedIndices(validFuncTypes.Count());

		if (withVariadicInput)
		{
			// if number of arguments is unknown, we perform an inaccurate test, only filtering away a few inappropriate candidates
			for (vint i = 0; i < validFuncTypes.Count(); i++)
			{
				selectedIndices[i] = IsAcceptableWithVariadicInput(pa, validFuncTypes[i], argTypes, boundedAnys);
			}
		}
		else
		{
			// if number of arguments is known, we need to pick the best candidate
			for (vint i = 0; i < validFuncTypes.Count(); i++)
			{
				selectedIndices[i] = true;
			}

			vint minLoopCount = argTypes.Count();
			if (minLoopCount < 1) minLoopCount = 1;

			for (vint i = 0; i < minLoopCount; i++)
			{
				Array<TypeConv> funcChoices(validFuncTypes.Count());

				for (vint j = 0; j < validFuncTypes.Count(); j++)
				{
					auto funcType = validFuncTypes[j];

					// best candidates for this arguments are chosen
					if (i < argTypes.Count())
					{
						vint funcParamCount = funcType.tsys->GetParamCount();
						if (funcParamCount <= i)
						{
							funcChoices[j] = TypeConvCat::Ellipsis;
							continue;
						}

						auto paramType = funcType.tsys->GetParam(i);
						funcChoices[j] = TestTypeConversion(pa, paramType, argTypes[i]);
					}
					else
					{
						// if there is not argument, we say functions without ellipsis are better
						funcChoices[j] = funcType.tsys->GetFunc().ellipsis ? TypeConvCat::Ellipsis : TypeConvCat::Exact;
					}
				}

				// the best candidate should be the best in for all arguments at the same time
				auto min = FindMinConv(funcChoices);
				for (vint j = 0; j < validFuncTypes.Count(); j++)
				{
					auto choice = funcChoices[j];
					if (IsFunctionAcceptableByMinConv(choice, min))
					{
						if (choice.anyInvolved)
						{
							if (anyInvolved)
							{
								*anyInvolved = choice.anyInvolved;
							}
						}
					}
					else
					{
						selectedIndices[j] = false;
					}
				}
			}
		}

		for (vint i = 0; i < selectedIndices.Count(); i++)
		{
			if (selectedIndices[i])
			{
				if(selectedFunctions)
				{
					selectedFunctions->Add(validFuncTypes[i]);
				}
				AddTemp(result, validFuncTypes[i].tsys->GetElement());
			}
		}
	}

	/***********************************************************************
	IsAdlEnabled: Check if argument-dependent lookup is considered according to the unqualified lookup result
	***********************************************************************/

	bool IsAdlEnabled(const ParsingArguments& pa, Ptr<Resolving> resolving)
	{
		for (vint i = 0; i < resolving->resolvedSymbols.Count(); i++)
		{
			auto symbol = resolving->resolvedSymbols[i];
			if (symbol->GetAnyForwardDecl<ForwardFunctionDeclaration>())
			{
				auto parent = symbol->GetParentScope();
				while (parent)
				{
					switch (parent->kind)
					{
					case symbol_component::SymbolKind::Root:
					case symbol_component::SymbolKind::Namespace:
						parent = parent->GetParentScope();
						break;
					default:
						return false;
					}
				}
			}
			else
			{
				return false;
			}
		}
		return true;
	}

	/***********************************************************************
	SearchAdlClassesAndNamespaces: Preparing for argument-dependent lookup
	***********************************************************************/

	class SearchBaseTypeAdlClassesAndNamespacesVisitor : public Object, public virtual ITypeVisitor
	{
	public:
		const ParsingArguments&		pa;
		SortedList<Symbol*>&		nss;
		SortedList<Symbol*>&		classes;

		SearchBaseTypeAdlClassesAndNamespacesVisitor(const ParsingArguments& _pa, SortedList<Symbol*>& _nss, SortedList<Symbol*>& _classes)
			:pa(_pa)
			, nss(_nss)
			, classes(_classes)
		{
		}

		void Resolve(Ptr<Resolving> resolving)
		{
			for (vint i = 0; i < resolving->resolvedSymbols.Count(); i++)
			{
				SearchAdlClassesAndNamespaces(pa, resolving->resolvedSymbols[i], nss, classes);
			}
		}

		void Visit(PrimitiveType* self)override
		{
		}

		void Visit(ReferenceType* self)override
		{
		}

		void Visit(ArrayType* self)override
		{
		}

		void Visit(CallingConventionType* self)override
		{
		}

		void Visit(FunctionType* self)override
		{
		}

		void Visit(MemberType* self)override
		{
		}

		void Visit(DeclType* self)override
		{
		}

		void Visit(DecorateType* self)override
		{
		}

		void Visit(RootType* self)override
		{
		}

		void Visit(IdType* self)override
		{
			Resolve(self->resolving);
		}

		void Visit(ChildType* self)override
		{
			Resolve(self->resolving);
		}

		void Visit(GenericType* self)override
		{
			self->type->Accept(this);
			for (vint i = 0; i < self->arguments.Count(); i++)
			{
				auto argument = self->arguments[i];
				if (argument.item.type)
				{
					argument.item.type->Accept(this);
				}
			}
		}
	};

	void SearchAdlClassesAndNamespaces(const ParsingArguments& pa, Symbol* symbol, SortedList<Symbol*>& nss, SortedList<Symbol*>& classes)
	{
		auto firstSymbol = symbol;
		while (symbol)
		{
			if (symbol->kind == symbol_component::SymbolKind::Namespace)
			{
				if (!nss.Contains(symbol))
				{
					nss.Add(symbol);
				}
			}
			else if (auto classDecl = symbol->GetCategory() == symbol_component::SymbolCategory::Normal ? symbol->GetImplDecl_NFb<ClassDeclaration>() : nullptr)
			{
				if (!classes.Contains(symbol))
				{
					classes.Add(symbol);
				}

				if (symbol == firstSymbol)
				{
					auto classPa = pa.WithScope(symbol);

					SearchBaseTypeAdlClassesAndNamespacesVisitor visitor(classPa, nss, classes);
					for (vint i = 0; i < classDecl->baseTypes.Count(); i++)
					{
						classDecl->baseTypes[i].f1->Accept(&visitor);
					}
				}
			}
			else
			{
				break;
			}
			symbol = symbol->GetParentScope();
		}
	}

	void SearchAdlClassesAndNamespaces(const ParsingArguments& pa, ITsys* type, SortedList<Symbol*>& nss, SortedList<Symbol*>& classes)
	{
		switch (type->GetType())
		{
		case TsysType::LRef:
		case TsysType::RRef:
		case TsysType::Ptr:
		case TsysType::Array:
		case TsysType::CV:
			SearchAdlClassesAndNamespaces(pa, type->GetElement(), nss, classes);
			break;
		case TsysType::Function:
			SearchAdlClassesAndNamespaces(pa, type->GetElement(), nss, classes);
			for (vint i = 0; i < type->GetParamCount(); i++)
			{
				SearchAdlClassesAndNamespaces(pa, type->GetParam(i), nss, classes);
			}
			break;
		case TsysType::Member:
			SearchAdlClassesAndNamespaces(pa, type->GetElement(), nss, classes);
			SearchAdlClassesAndNamespaces(pa, type->GetClass(), nss, classes);
			break;
		case TsysType::Decl:
			SearchAdlClassesAndNamespaces(pa, type->GetDecl(), nss, classes);
			break;
		case TsysType::DeclInstant:
			{
				SearchAdlClassesAndNamespaces(pa, type->GetDecl(), nss, classes);
				for (vint i = 0; i < type->GetParamCount(); i++)
				{
					SearchAdlClassesAndNamespaces(pa, type->GetParam(i), nss, classes);
				}
				const auto& di = type->GetDeclInstant();
				if (di.parentDeclType)
				{
					SearchAdlClassesAndNamespaces(pa, di.parentDeclType, nss, classes);
				}
			}
			break;
		}
	}

	/***********************************************************************
	SearchAdlFunction: Find functions in namespaces
	***********************************************************************/

	void SearchAdlFunction(const ParsingArguments& pa, SortedList<Symbol*>& nss, const WString& name, ExprTsysList& result)
	{
		for (vint i = 0; i < nss.Count(); i++)
		{
			auto ns = nss[i];
			if (auto pChildren = ns->TryGetChildren_NFb(name))
			{
				for (vint j = 0; j < pChildren->Count(); j++)
				{
					auto child = pChildren->Get(j).Obj();
					if (child->kind == symbol_component::SymbolKind::FunctionSymbol)
					{
						VisitSymbol(pa, child, result);
					}
				}
			}
		}
	}
}