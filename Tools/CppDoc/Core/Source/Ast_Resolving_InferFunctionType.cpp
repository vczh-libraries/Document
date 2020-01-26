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

	void InferTemplateArgumentsForFunctionType(
		const ParsingArguments& pa,
		FunctionType* functionType,
		List<ITsys*>& parameterAssignment,
		TemplateArgumentContext& taContext,
		const SortedList<Symbol*>& freeTypeSymbols,
		bool exactMatchForParameters
	);

	/***********************************************************************
	CollectFreeTypes:	Collect and check all involved free types from a type
						Stops at nested variadic types
	***********************************************************************/

	class CollectFreeTypesVisitor : public Object, public virtual ITypeVisitor
	{
	public:
		bool								involved = false;
		bool								insideVariant;
		const SortedList<Symbol*>&			freeTypeSymbols;
		SortedList<Type*>&					involvedTypes;

		CollectFreeTypesVisitor(bool _insideVariant, const SortedList<Symbol*>& _freeTypeSymbols, SortedList<Type*>& _involvedTypes)
			:insideVariant(_insideVariant)
			, freeTypeSymbols(_freeTypeSymbols)
			, involvedTypes(_involvedTypes)
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
				if (self->parameters[i].isVariadic)
				{
					// does not support nested variadic arguments
					throw TypeCheckerException();
				}
				else
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
				if (self->arguments[i].isVariadic)
				{
					// does not support nested variadic arguments
					throw TypeCheckerException();
				}
				else
				{
					result = Execute(self->arguments[i].item.type) || result;
				}
			}
			if ((involved = result)) involvedTypes.Add(self);
		}
	};

	void CollectFreeTypes(Type* type, bool insideVariant, const SortedList<Symbol*>& freeTypeSymbols, SortedList<Type*>& involvedTypes)
	{
		if (type)
		{
			CollectFreeTypesVisitor visitor(insideVariant, freeTypeSymbols, involvedTypes);
			type->Accept(&visitor);
		}
	}

	void CollectFreeTypes(Ptr<Type> type, bool insideVariant, const SortedList<Symbol*>& freeTypeSymbols, SortedList<Type*>& involvedTypes)
	{
		CollectFreeTypes(type.Obj(), insideVariant, freeTypeSymbols, involvedTypes);
	}

	/***********************************************************************
	InferTemplateArgument:	Perform type inferencing by matching an function argument with its offerred argument
	***********************************************************************/

	void SetInferredResult(TemplateArgumentContext& taContext, ITsys* pattern, ITsys* type)
	{
		vint index = taContext.arguments.Keys().IndexOf(pattern);
		if (index == -1)
		{
			// if this argument is not inferred, use the result
			taContext.arguments.Add(pattern, type);
		}
		else if (type->GetType() != TsysType::Any)
		{
			// if this argument is inferred, it requires the same result if both of them are not any_t
			auto inferred = taContext.arguments.Values()[index];
			if (inferred->GetType() == TsysType::Any)
			{
				taContext.arguments.Set(pattern, type);
			}
			else if (type != inferred)
			{
				throw TypeCheckerException();
			}
		}
	}

	class InferTemplateArgumentVisitor : public Object, public virtual ITypeVisitor
	{
	public:
		ITsys*								offeredType = nullptr;
		bool								exactMatch = false;
		const ParsingArguments&				pa;
		TemplateArgumentContext&			taContext;
		TemplateArgumentContext&			variadicContext;
		const SortedList<Symbol*>&			freeTypeSymbols;
		const SortedList<Type*>&			involvedTypes;

		InferTemplateArgumentVisitor(const ParsingArguments& _pa, TemplateArgumentContext& _taContext, TemplateArgumentContext& _variadicContext, const SortedList<Symbol*>& _freeTypeSymbols, const SortedList<Type*>& _involvedTypes)
			:pa(_pa)
			, taContext(_taContext)
			, variadicContext(_variadicContext)
			, freeTypeSymbols(_freeTypeSymbols)
			, involvedTypes(_involvedTypes)
		{
		}

		void ExecuteInvolvedOnce(Ptr<Type>& argumentType, ITsys* _offeredType, bool _exactMatch = true)
		{
			offeredType = _offeredType;
			exactMatch = _exactMatch;
			if (!offeredType->IsUnknownType())
			{
				argumentType->Accept(this);
			}
		}

		void ExecuteOnce(Ptr<Type>& argumentType, ITsys* _offeredType, bool _exactMatch = true)
		{
			if (!involvedTypes.Contains(argumentType.Obj())) return;
			ExecuteInvolvedOnce(argumentType, _offeredType, _exactMatch);
		}

		void Execute(Ptr<Type>& argumentType, ITsys* _offeredType)
		{
			InferTemplateArgumentVisitor(pa, taContext, variadicContext, freeTypeSymbols, involvedTypes)
				.ExecuteOnce(argumentType, _offeredType);
		}

		void Visit(PrimitiveType* self)override
		{
		}

		void VisitArrayOrPtr(Ptr<Type>& elementType, TsysType tsysType)
		{
			if (exactMatch)
			{
				if (offeredType->GetType() != tsysType)
				{
					throw TypeCheckerException();
				}
				ExecuteInvolvedOnce(elementType, offeredType->GetElement());
			}
			else
			{
				TsysCV cv;
				TsysRefType refType;
				auto entity = offeredType->GetEntity(cv, refType);
				if (entity->GetType() != TsysType::Ptr && entity->GetType() != TsysType::Array)
				{
					throw TypeCheckerException();
				}
				ExecuteInvolvedOnce(elementType, entity->GetElement());
			}
		}

		void Visit(ReferenceType* self)override
		{
			switch (self->reference)
			{
			case CppReferenceType::Ptr:
				VisitArrayOrPtr(self->type, TsysType::Ptr);
				break;
			case CppReferenceType::LRef:
				{
					if (exactMatch)
					{
						switch (offeredType->GetType())
						{
						case TsysType::LRef:
							ExecuteInvolvedOnce(self->type, offeredType->GetElement());
							break;
						default:
							throw TypeCheckerException();
						}
					}
					else
					{
						switch (offeredType->GetType())
						{
						case TsysType::LRef:
							ExecuteInvolvedOnce(self->type, offeredType->GetElement());
							break;
						case TsysType::RRef:
							throw TypeCheckerException();
						default:
							ExecuteInvolvedOnce(self->type, offeredType->CVOf({ true,false })->LRefOf());
						}
					}
				}
				break;
			case CppReferenceType::RRef:
				{
					if (exactMatch)
					{
						switch (offeredType->GetType())
						{
						case TsysType::LRef:
							ExecuteInvolvedOnce(self->type, offeredType);
							break;
						case TsysType::RRef:
							ExecuteInvolvedOnce(self->type, offeredType->GetElement());
							break;
						default:
							throw TypeCheckerException();
						}
					}
					else
					{
						switch (offeredType->GetType())
						{
						case TsysType::LRef:
							ExecuteInvolvedOnce(self->type, offeredType);
							break;
						case TsysType::RRef:
							ExecuteInvolvedOnce(self->type, offeredType->GetElement());
						default:
							ExecuteInvolvedOnce(self->type, offeredType);
						}
					}
				}
				break;
			}
		}

		void Visit(ArrayType* self)override
		{
			VisitArrayOrPtr(self->type, TsysType::Array);
		}

		void Visit(CallingConventionType* self)override
		{
			self->type->Accept(this);
		}

		void Visit(FunctionType* self)override
		{
			auto entity = offeredType;
			if (!exactMatch)
			{
				TsysCV cv;
				TsysRefType refType;
				offeredType->GetEntity(cv, refType);
				if (entity->GetType() == TsysType::Ptr && entity->GetElement()->GetType() == TsysType::Function)
				{
					entity = entity->GetElement();
				}
			}
			if (entity->GetType() != TsysType::Member)
			{
				throw TypeCheckerException();
			}

			ExecuteOnce(self->decoratorReturnType ? self->decoratorReturnType : self->returnType, entity->GetElement());

			List<ITsys*> parameterAssignment;
			{
				vint count = entity->GetParamCount();
				Array<ExprTsysItem> argumentTypes(count);
				SortedList<vint> boundedAnys;
				for (vint i = 0; i < count; i++)
				{
					argumentTypes[i] = { nullptr,ExprTsysType::PRValue,entity->GetParam(i) };
				}
				ResolveFunctionParameters(pa, parameterAssignment, self, argumentTypes, boundedAnys);
			}
			InferTemplateArgumentsForFunctionType(pa, self, parameterAssignment, taContext, freeTypeSymbols, true);
		}

		void Visit(MemberType* self)override
		{
			TsysCV cv;
			TsysRefType refType;
			auto entity = offeredType->GetEntity(cv, refType);
			if (entity->GetType() != TsysType::Member)
			{
				throw TypeCheckerException();
			}
			ExecuteOnce(self->classType, entity->GetClass());
			ExecuteOnce(self->type, entity->GetElement());
		}

		void Visit(DeclType* self)override
		{
		}

		void Visit(DecorateType* self)override
		{
			TsysCV cv;
			TsysRefType refType;
			auto entity = offeredType->GetEntity(cv, refType);
			if(exactMatch)
			{
				if (refType != TsysRefType::None) throw TypeCheckerException();
				if (self->isConst != cv.isGeneralConst) throw TypeCheckerException();
				if (self->isVolatile != cv.isVolatile) throw TypeCheckerException();
				ExecuteInvolvedOnce(self->type, entity);
			}
			else
			{
				ExecuteInvolvedOnce(self->type, entity);
			}
		}

		void Visit(RootType* self)override
		{
		}

		void Visit(IdType* self)override
		{
			auto patternSymbol = self->resolving->resolvedSymbols[0];
			auto& outputContext = patternSymbol->ellipsis ? variadicContext : taContext;

			// consistent with GetTemplateArgumentKey
			auto pattern = EvaluateGenericArgumentSymbol(patternSymbol);

			// const, volatile, &, && are discarded
			TsysCV refCV;
			TsysRefType refType;
			auto entity = exactMatch ? offeredType : offeredType->GetEntity(refCV, refType);
			SetInferredResult(outputContext, pattern, entity);
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
		TemplateArgumentContext& variadicContext,
		const SortedList<Symbol*>& freeTypeSymbols,
		const SortedList<Type*>& involvedTypes,
		bool exactMatchForParameters
	)
	{
		InferTemplateArgumentVisitor(pa, taContext, variadicContext, freeTypeSymbols, involvedTypes)
			.ExecuteOnce(argumentType, offeredType, exactMatchForParameters);
	}

	/***********************************************************************
	InferFunctionType:	Perform type inferencing for template function using both offered template and function arguments
						Ts(*)(X<Ts...>)... or Ts<X<Ts<Y>...>... is not supported, because of nested Ts...
	***********************************************************************/

	void InferTemplateArgumentsForFunctionType(
		const ParsingArguments& pa,
		FunctionType* functionType,
		List<ITsys*>& parameterAssignment,
		TemplateArgumentContext& taContext,
		const SortedList<Symbol*>& freeTypeSymbols,
		bool exactMatchForParameters
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

				SortedList<Type*> involvedTypes;
				CollectFreeTypes(parameter.item->type, parameter.isVariadic, freeTypeSymbols, involvedTypes);

				// get all affected arguments
				List<ITsys*> vas;
				List<ITsys*> nvas;
				for (vint j = 0; j < involvedTypes.Count(); j++)
				{
					if (auto idType = dynamic_cast<IdType*>(involvedTypes[j]))
					{
						auto patternSymbol = idType->resolving->resolvedSymbols[0];
						auto pattern = EvaluateGenericArgumentSymbol(patternSymbol);
						if (patternSymbol->ellipsis)
						{
							vas.Add(pattern);
						}
						else
						{
							nvas.Add(pattern);
						}
					}
				}

				// infer all affected types to any_t, result will be overrided if more precise types are inferred
				for (vint j = 0; j < vas.Count(); j++)
				{
					SetInferredResult(taContext, vas[j], pa.tsys->Any());
				}
				for (vint j = 0; j < nvas.Count(); j++)
				{
					SetInferredResult(taContext, nvas[j], pa.tsys->Any());
				}

				if (assignedTsys->GetType() != TsysType::Any)
				{
					if (parameter.isVariadic)
					{
						// for variadic parameter
						vint count = assignedTsys->GetParamCount();
						if (count == 0)
						{
							// if the assigned argument is an empty list, infer all variadic arguments to empty
							Array<ExprTsysItem> params;
							auto init = pa.tsys->InitOf(params);
							for (vint j = 0; j < vas.Count(); j++)
							{
								SetInferredResult(taContext, vas[j], init);
							}
						}
						else
						{
							// if the assigned argument is a non-empty list
							Dictionary<ITsys*, Ptr<Array<ExprTsysItem>>> variadicResults;
							for (vint j = 0; j < vas.Count(); j++)
							{
								variadicResults.Add(vas[j], MakePtr<Array<ExprTsysItem>>(count));
							}

							// run each item in the list
							for (vint j = 0; j < count; j++)
							{
								auto assignedTsysItem = ApplyExprTsysType(assignedTsys->GetParam(j), assignedTsys->GetInit().headers[j].type);
								TemplateArgumentContext variadicContext;
								InferTemplateArgument(pa, parameter.item->type, assignedTsysItem, taContext, variadicContext, freeTypeSymbols, involvedTypes, exactMatchForParameters);
								for (vint k = 0; k < variadicContext.arguments.Count(); k++)
								{
									auto key = variadicContext.arguments.Keys()[k];
									auto value = variadicContext.arguments.Values()[k];
									auto result = variadicResults[key];
									result->Set(j, { nullptr,ExprTsysType::PRValue,value });
								}
							}

							// aggregate them
							for (vint j = 0; j < vas.Count(); j++)
							{
								auto pattern = vas[j];
								auto& params = *variadicResults[pattern].Obj();
								auto init = pa.tsys->InitOf(params);
								SetInferredResult(taContext, pattern, init);
							}
						}
					}
					else
					{
						// for non-variadic parameter, run the assigned argument
						TemplateArgumentContext unusedVariadicContext;
						InferTemplateArgument(pa, parameter.item->type, assignedTsys, taContext, unusedVariadicContext, freeTypeSymbols, involvedTypes, exactMatchForParameters);
						if (unusedVariadicContext.arguments.Count() > 0)
						{
							throw TypeCheckerException();
						}
					}
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

							auto inferPa = pa.AdjustForDecl(gfi.declSymbol, gfi.parentDeclType, false);

							// cannot pass Ptr<FunctionType> to this function since the last filled argument could be variadic
							// known variadic function argument should be treated as separated arguments
							// ParsingArguments need to be adjusted so that we can evaluate each parameter type
							ResolveFunctionParameters(pa, parameterAssignment, functionType.Obj(), argTypes, boundedAnys);

							// fill freeTypeSymbols with all template arguments
							// fill freeTypeAssignments with only assigned template arguments
							for (vint i = 0; i < gfi.spec->arguments.Count(); i++)
							{
								auto argument = gfi.spec->arguments[i];
								auto pattern = GetTemplateArgumentKey(argument, pa.tsys.Obj());
								auto patternSymbol = TemplateArgumentPatternToSymbol(pattern);
								freeTypeSymbols.Add(patternSymbol);
							}

							// type inferencing
							InferTemplateArgumentsForFunctionType(inferPa, functionType.Obj(), parameterAssignment, taContext, freeTypeSymbols, false);

							// fill template value arguments
							for (vint i = 0; i < gfi.spec->arguments.Count(); i++)
							{
								auto argument = gfi.spec->arguments[i];
								if (argument.argumentType == CppTemplateArgumentType::Value)
								{
									auto pattern = GetTemplateArgumentKey(argument, pa.tsys.Obj());
									auto patternSymbol = TemplateArgumentPatternToSymbol(pattern);
									// TODO: not implemented
									// InferTemplateArgumentsForFunctionType need to handle this
									throw 0;
									/*
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
									*/
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