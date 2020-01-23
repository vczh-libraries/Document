#include "Ast_Resolving.h"

namespace symbol_type_resolving
{
	Symbol* TemplateArgumentPatternToSymbol(ITsys* tsys)
	{
		switch (tsys->GetType())
		{
		case TsysType::GenericArg:
			return tsys->GetGenericArg().argSymbol;
		default:
			throw TypeCheckerException();
		}
	}

	/***********************************************************************
	CollectFreeTypes:	Collect and check all involved free types from a type
						Stops at nested variadic types
	***********************************************************************/

	const vint FREE_TYPE_UNASSIGNED = -2;
	const vint FREE_TYPE_ANY = -1;
	const vint FREE_TYPE_ASSIGNED = 0;

	class CollectFreeTypesVisitor : public Object, public virtual ITypeVisitor
	{
	public:
		bool								involved = false;
		vint								assignment;
		const SortedList<Symbol*>&			freeTypeSymbols;
		SortedList<Type*>&					involvedTypes;
		Dictionary<Symbol*, vint>&			freeTypeAssignments;

		CollectFreeTypesVisitor(vint _assignment, const SortedList<Symbol*>& _freeTypeSymbols, SortedList<Type*>& _involvedTypes, Dictionary<Symbol*, vint>& _freeTypeAssignments)
			:assignment(_assignment)
			, freeTypeSymbols(_freeTypeSymbols)
			, involvedTypes(_involvedTypes)
			, freeTypeAssignments(_freeTypeAssignments)
		{
		}

		bool Execute(Type* type)
		{
			if (!type) return false;
			type->Accept(this);
			bool result = involved;
			involved = false;
			return result;
		}

		bool Execute(Ptr<Type>& type)
		{
			return Execute(type.Obj());
		}

		void Visit(PrimitiveType* self)override
		{
		}

		void Visit(ReferenceType* self)override
		{
			if (Execute(self->type)) involvedTypes.Add(self);
		}

		void Visit(ArrayType* self)override
		{
			if (Execute(self->type)) involvedTypes.Add(self);
		}

		void Visit(CallingConventionType* self)override
		{
			if (Execute(self->type)) involvedTypes.Add(self);
		}

		void Visit(FunctionType* self)override
		{
			bool result = false;
			result = Execute(self->decoratorReturnType) || result;
			result = Execute(self->returnType) || result;
			for (vint i = 0; i < self->parameters.Count(); i++)
			{
				if (!self->parameters[i].isVariadic)
				{
					result = Execute(self->parameters[i].item->type);
				}
			}
			if ((involved = result)) involvedTypes.Add(self);
		}

		void Visit(MemberType* self)override
		{
			bool result = false;
			result = Execute(self->classType) || result;
			result = Execute(self->type) || result;
			if ((involved = result)) involvedTypes.Add(self);
		}

		void Visit(DeclType* self)override
		{
			// do not perform type inferencing against decltype(expr)
		}

		void Visit(DecorateType* self)override
		{
			if (Execute(self->type)) involvedTypes.Add(self);
		}

		void Visit(RootType* self)override
		{
		}

		void Visit(IdType* self)override
		{
			if (self->resolving && self->resolving->resolvedSymbols.Count() == 1)
			{
				auto symbol = self->resolving->resolvedSymbols[0];
				if (freeTypeSymbols.Contains(symbol))
				{
					involved = true;
					involvedTypes.Add(self);

					vint normalizedAssignment = symbol->ellipsis ? assignment : FREE_TYPE_ASSIGNED;
					vint index = freeTypeAssignments.Keys().IndexOf(symbol);
					if (index == -1)
					{
						freeTypeAssignments.Add(symbol, normalizedAssignment);
					}
					else
					{
						vint value = freeTypeAssignments.Values()[index];
						if (value == FREE_TYPE_UNASSIGNED || value == normalizedAssignment)
						{
							freeTypeAssignments.Set(symbol, normalizedAssignment);
						}
						else
						{
							// conflict with previous inferred result
							throw TypeCheckerException();
						}
					}
				}
			}
		}

		void Visit(ChildType* self)override
		{
			if (Execute(self->classType.Obj())) involvedTypes.Add(self);
		}

		void Visit(GenericType* self)override
		{
			bool result = false;
			result = Execute(self->type.Obj()) || result;
			for (vint i = 0; i < self->arguments.Count(); i++)
			{
				if (!self->arguments[i].isVariadic)
				{
					result = Execute(self->arguments[i].item.type) || result;
				}
			}
			if ((involved = result)) involvedTypes.Add(self);
		}
	};

	void CollectFreeTypes(Type* type, vint assignment, const SortedList<Symbol*>& freeTypeSymbols, SortedList<Type*>& involvedTypes, Dictionary<Symbol*, vint>& freeTypeAssignments)
	{
		if (type)
		{
			CollectFreeTypesVisitor visitor(assignment, freeTypeSymbols, involvedTypes, freeTypeAssignments);
			type->Accept(&visitor);
		}
	}

	void CollectFreeTypes(Ptr<Type> type, vint assignment, const SortedList<Symbol*>& freeTypeSymbols, SortedList<Type*>& involvedTypes, Dictionary<Symbol*, vint>& freeTypeAssignments)
	{
		CollectFreeTypes(type.Obj(), assignment, freeTypeSymbols, involvedTypes, freeTypeAssignments);
	}

	/***********************************************************************
	InferTemplateArgument:	Perform type inferencing by matching an function argument with its offerred argument
	***********************************************************************/

	class InferTemplateArgumentVisitor : public Object, public virtual ITypeVisitor
	{
	public:
		ITsys*								offeredType = nullptr;
		bool								exactMatch = false;
		const ParsingArguments&				pa;
		TemplateArgumentContext&			taContext;
		const SortedList<Symbol*>&			freeTypeSymbols;
		const SortedList<Type*>&			involvedTypes;
		const Dictionary<Symbol*, vint>&	freeTypeAssignments;

		InferTemplateArgumentVisitor(const ParsingArguments& _pa, TemplateArgumentContext& _taContext, const SortedList<Symbol*>& _freeTypeSymbols, const SortedList<Type*>& _involvedTypes, const Dictionary<Symbol*, vint>& _freeTypeAssignments)
			:pa(_pa)
			, taContext(_taContext)
			, freeTypeSymbols(_freeTypeSymbols)
			, involvedTypes(_involvedTypes)
			, freeTypeAssignments(_freeTypeAssignments)
		{
		}

		void ExecuteOnce(Ptr<Type> argumentType, ITsys* _offeredType, bool _exactMatch)
		{
			if (!involvedTypes.Contains(argumentType.Obj())) return;
			offeredType = _offeredType;
			exactMatch = exactMatch;
			argumentType->Accept(this);
		}

		void Execute(Ptr<Type> argumentType, ITsys* _offeredType)
		{
			InferTemplateArgumentVisitor(pa, taContext, freeTypeSymbols, involvedTypes, freeTypeAssignments)
				.ExecuteOnce(argumentType, _offeredType, true);
		}

		void Visit(PrimitiveType* self)override
		{
			if(!involvedTypes.Contains(self)) return;
			// TODO: not implemented
			throw 0;
		}

		void Visit(ReferenceType* self)override
		{
			// TODO: not implemented
			throw 0;
		}

		void Visit(ArrayType* self)override
		{
			// TODO: not implemented
			throw 0;
		}

		void Visit(CallingConventionType* self)override
		{
			// TODO: not implemented
			throw 0;
		}

		void Visit(FunctionType* self)override
		{
			// TODO: not implemented
			throw 0;
		}

		void Visit(MemberType* self)override
		{
			// TODO: not implemented
			throw 0;
		}

		void Visit(DeclType* self)override
		{
			// TODO: not implemented
			throw 0;
		}

		void Visit(DecorateType* self)override
		{
			// TODO: not implemented
			throw 0;
		}

		void Visit(RootType* self)override
		{
			// TODO: not implemented
			throw 0;
		}

		void Visit(IdType* self)override
		{
			auto patternSymbol = self->resolving->resolvedSymbols[0];
			// consistent with GetTemplateArgumentKey
			if (patternSymbol->ellipsis)
			{
				auto pattern = pa.tsys->DeclOf(patternSymbol);
				// TODO: not implemented
				throw 0;
			}
			else
			{
				auto pattern = EvaluateGenericArgumentSymbol(patternSymbol);

				// const, volatile, &, && are discarded
				TsysCV refCV;
				TsysRefType refType;
				auto entity = exactMatch ? offeredType : offeredType->GetEntity(refCV, refType);

				vint index = taContext.arguments.Keys().IndexOf(pattern);
				if (index == -1)
				{
					// if this argument is not inferred, use the result
					taContext.arguments.Add(pattern, entity);
				}
				else
				{
					// if this argument is inferred, it requires the same result
					auto inferred = taContext.arguments.Values()[index];
					if (entity != inferred)
					{
						throw TypeCheckerException();
					}
				}
			}
		}

		void Visit(ChildType* self)override
		{
			// TODO: not implemented
			throw 0;
		}

		void Visit(GenericType* self)override
		{
			// TODO: not implemented
			throw 0;
		}
	};

	void InferTemplateArgument(
		const ParsingArguments& pa,
		Ptr<Type> argumentType,
		ITsys* offeredType,
		TemplateArgumentContext& taContext,
		const SortedList<Symbol*>& freeTypeSymbols,
		const SortedList<Type*>& involvedTypes,
		const Dictionary<Symbol*, vint>& freeTypeAssignments
	)
	{
		InferTemplateArgumentVisitor(pa, taContext, freeTypeSymbols, involvedTypes, freeTypeAssignments)
			.ExecuteOnce(argumentType, offeredType, false);
	}

	/***********************************************************************
	InferFunctionType:	Perform type inferencing for template function using both offered template and function arguments
						Ts(*)(X<Ts...>)... or Ts<X<Ts<Y>...>... is not supported, because of nested Ts...
	***********************************************************************/

	void InferTemplateArgumentsForFunctionType(
		const ParsingArguments& pa,
		ExprTsysItem functionItem,
		Ptr<FunctionType> functionType,
		List<ITsys*>& parameterAssignment,
		TemplateArgumentContext& taContext,
		const SortedList<Symbol*>& freeTypeSymbols,
		Dictionary<Symbol*, vint>& freeTypeAssignments
	)
	{
		// don't care about arguments for ellipsis
		for (vint i = 0; i < functionType->parameters.Count(); i++)
		{
			// if default value is used, skip it
			if (auto assignedTsys = parameterAssignment[i])
			{
				// see if any variadic value arguments can be determined
				// variadic value argument only care about the number of values
				auto parameter = functionType->parameters[i];
				vint assignment = FREE_TYPE_ASSIGNED;
				if (parameter.isVariadic)
				{
					if (assignedTsys->GetType() == TsysType::Any)
					{
						assignment = FREE_TYPE_ANY;
					}
					else
					{
						assignment = assignedTsys->GetParamCount();
					}
				}

				SortedList<Type*> involvedTypes;
				CollectFreeTypes(parameter.item->type, assignment, freeTypeSymbols, involvedTypes, freeTypeAssignments);
				if (parameter.isVariadic)
				{
					// TODO: not implemented
					throw 0;
				}
				else
				{
					InferTemplateArgument(pa, parameter.item->type, assignedTsys, taContext, freeTypeSymbols, involvedTypes, freeTypeAssignments);
				}
			}
		}
	}

	Nullable<ExprTsysItem> InferFunctionType(const ParsingArguments& pa, ExprTsysItem functionItem, Array<ExprTsysItem>& argTypes, SortedList<vint>& boundedAnys)
	{
		switch (functionItem.tsys->GetType())
		{
		case TsysType::Function:
			return functionItem;
		case TsysType::LRef:
		case TsysType::RRef:
		case TsysType::CV:
		case TsysType::Ptr:
			return InferFunctionType(pa, { functionItem,functionItem.tsys->GetElement() }, argTypes, boundedAnys);
		case TsysType::GenericFunction:
			if (auto symbol = functionItem.symbol)
			{
				if (auto decl = symbol->GetAnyForwardDecl<ForwardFunctionDeclaration>())
				{
					if (auto functionType = GetTypeWithoutMemberAndCC(decl->type).Cast<FunctionType>())
					{
						try
						{
							auto gfi = functionItem.tsys->GetGenericFunction();
							if (gfi.filledArguments != 0) throw TypeCheckerException();

							List<ITsys*>						parameterAssignment;
							TemplateArgumentContext				taContext;
							SortedList<Symbol*>					freeTypeSymbols;
							Dictionary<Symbol*, vint>			freeTypeAssignments;

							auto inferPa = pa.AdjustForDecl(gfi.declSymbol, gfi.parentDeclType, false);

							// cannot pass Ptr<FunctionType> to this function since the last filled argument could be variadic
							// known variadic function argument should be treated as separated arguments
							// ParsingArguments need to be adjusted so that we can evaluate each parameter type
							ResolveFunctionParameters(pa, parameterAssignment, functionType, argTypes, boundedAnys);

							// fill freeTypeSymbols with all template arguments
							// fill freeTypeAssignments with only assigned template arguments
							for (vint i = 0; i < gfi.spec->arguments.Count(); i++)
							{
								auto argument = gfi.spec->arguments[i];
								auto pattern = GetTemplateArgumentKey(argument, pa.tsys.Obj());
								auto patternSymbol = TemplateArgumentPatternToSymbol(pattern);
								freeTypeSymbols.Add(patternSymbol);

								if (i < gfi.filledArguments)
								{
									auto value = functionItem.tsys->GetParam(i);
									taContext.arguments.Add(pattern, value);

									vint assignment =
										!argument.ellipsis ? FREE_TYPE_ASSIGNED :
										value->GetType() == TsysType::Any ? FREE_TYPE_ANY :
										value->GetParamCount();
									freeTypeAssignments.Add(patternSymbol, assignment);
								}
							}

							// type inferencing
							InferTemplateArgumentsForFunctionType(inferPa, functionItem, functionType, parameterAssignment, taContext, freeTypeSymbols, freeTypeAssignments);

							// fill template value arguments
							for (vint i = 0; i < gfi.spec->arguments.Count(); i++)
							{
								auto argument = gfi.spec->arguments[i];
								if (argument.argumentType == CppTemplateArgumentType::Value)
								{
									auto pattern = GetTemplateArgumentKey(argument, pa.tsys.Obj());
									auto patternSymbol = TemplateArgumentPatternToSymbol(pattern);
									vint index = freeTypeAssignments.Keys().IndexOf(patternSymbol);
									if (index == -1)
									{
										throw TypeCheckerException();
									}
									else if (argument.ellipsis)
									{
										// consistent with ResolveGenericParameters
										vint assignment = freeTypeAssignments.Values()[index];
										if (assignment == FREE_TYPE_ANY)
										{
											taContext.arguments.Add(pattern, pa.tsys->Any());
										}
										else
										{
											Array<ExprTsysItem> items(assignment);
											for (vint j = 0; j < assignment; j++)
											{
												items[j] = { nullptr,ExprTsysType::PRValue,nullptr };
											}
											auto init = pa.tsys->InitOf(items);
											taContext.arguments.Add(pattern, init);
										}
									}
									else
									{
										taContext.arguments.Add(pattern, nullptr);
									}
								}
							}

							taContext.symbolToApply = gfi.declSymbol;
							auto& tsys = EvaluateFuncSymbol(inferPa, decl.Obj(), inferPa.parentDeclType, &taContext);
							if (tsys.Count() == 0)
							{
								return {};
							}
							else if (tsys.Count() == 1)
							{
								return { { functionItem,tsys[0] } };
							}
							else
							{
								// unable to handle multiple types
								throw IllegalExprException();
							}
						}
						catch (const TypeCheckerException&)
						{
							return {};
						}
					}
				}
			}
		default:
			if (functionItem.tsys->IsUnknownType())
			{
				return functionItem;
			}
			return {};
		}
	}
}