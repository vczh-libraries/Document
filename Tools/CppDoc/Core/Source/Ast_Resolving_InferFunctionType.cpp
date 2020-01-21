#include "Ast_Resolving.h"

namespace symbol_type_resolving
{
	/***********************************************************************
	IsFreeType:	Check if this type contains template arguments that are to be inferred
	***********************************************************************/

	class IsFreeTypeVisitor : public Object, public virtual ITypeVisitor
	{
	public:
		bool							result = false;
		const SortedList<ITsys*>&		allArgs;

		IsFreeTypeVisitor(const SortedList<ITsys*>& _allArgs)
			:allArgs(_allArgs)
		{
		}

		void Visit(PrimitiveType* self)override
		{
		}

		void Visit(ReferenceType* self)override
		{
			self->type->Accept(this);
		}

		void Visit(ArrayType* self)override
		{
			self->type->Accept(this);
		}

		void Visit(CallingConventionType* self)override
		{
			self->type->Accept(this);
		}

		void Visit(FunctionType* self)override
		{
			if (self->decoratorReturnType)
			{
				self->decoratorReturnType->Accept(this);
			}
			else
			{
				self->returnType->Accept(this);
			}

			for (vint i = 0; i < self->parameters.Count(); i++)
			{
				self->parameters[i].item->type->Accept(this);
			}
		}

		void Visit(MemberType* self)override
		{
			self->classType->Accept(this);
			self->type->Accept(this);
		}

		void Visit(DeclType* self)override
		{
			if (!self->expr)
			{
				result = true;
			}
		}

		void Visit(DecorateType* self)override
		{
			self->type->Accept(this);
		}

		void Visit(RootType* self)override
		{
		}

		void Visit(IdType* self)override
		{
			// TODO: not implemented
			throw 0;
		}

		void Visit(ChildType* self)override
		{
			self->classType->Accept(this);
		}

		void Visit(GenericType* self)override
		{
			self->type->Accept(this);
			for (vint i = 0; i < self->arguments.Count(); i++)
			{
				if (auto type = self->arguments[i].item.type)
				{
					type->Accept(this);
				}
			}
		}
	};

	bool IsFreeType(Type* type, const Dictionary<ITsys*, vint>& allArgs)
	{
		if (!type) return false;
		IsFreeTypeVisitor visitor(allArgs.Keys());
		type->Accept(&visitor);
		return visitor.result;
	}

	bool IsFreeType(Ptr<Type> type, const Dictionary<ITsys*, vint>& allArgs)
	{
		return IsFreeType(type.Obj(), allArgs);
	}

	/***********************************************************************
	InferTemplateArgument:	Perform type inferencing by matching an function argument with its offerred argument
	***********************************************************************/

	class InferTemplateArgumentVisitor : public Object, public virtual ITypeVisitor
	{
	public:
		ITsys*							offeredType = nullptr;
		const ParsingArguments&			pa;
		TemplateArgumentContext&		taContext;
		Dictionary<ITsys*, vint>&		allArgs;

		InferTemplateArgumentVisitor(const ParsingArguments& _pa, TemplateArgumentContext& _taContext, Dictionary<ITsys*, vint>& _allArgs)
			:pa(_pa)
			, taContext(_taContext)
			, allArgs(_allArgs)
		{
		}

		void Execute(Ptr<Type> argumentType, ITsys* _offeredType)
		{
			InferTemplateArgumentVisitor visitor(pa, taContext, allArgs);
			visitor.offeredType = _offeredType;
			argumentType->Accept(&visitor);
		}

		void Visit(PrimitiveType* self)override
		{
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
			// TODO: not implemented
			throw 0;
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
		Dictionary<ITsys*, vint>& allArgs
	)
	{
		InferTemplateArgumentVisitor(pa, taContext, allArgs).Execute(argumentType, offeredType);
	}

	/***********************************************************************
	InferFunctionType:	Perform type inferencing for template function using both offered template and function arguments
						Ts(*)(X<Ts...>)... or Ts<X<Ts<Y>...>... is not supported, because of nested Ts...
	***********************************************************************/

	const vint COUNT_UNASSIGNED = -2;
	const vint COUNT_ANY = -1;
	const vint COUNT_ASSIGNED = 0;

	void InferTemplateArgumentsForFunctionType(
		const ParsingArguments& pa,
		ExprTsysItem functionItem,
		Ptr<FunctionType> functionType,
		List<ITsys*>& parameterAssignment,
		TemplateArgumentContext& taContext,
		Dictionary<ITsys*, vint>& allArgs
	)
	{
		// don't care about arguments for ellipsis
		for (vint i = 0; i < functionType->parameters.Count(); i++)
		{
			// see if any variadic value arguments can be determined
			// variadic value argument only care about the number of values
			auto parameter = functionType->parameters[i];
			if (parameter.isVariadic)
			{
				// TODO: not implemented
				throw TypeCheckerException();
			}
			else
			{
				InferTemplateArgument(pa, parameter.item->type, parameterAssignment[i], taContext, allArgs);
			}
		}
		throw TypeCheckerException();
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

							List<ITsys*> parameterAssignment;
							TemplateArgumentContext taContext;
							Dictionary<ITsys*, vint> allArgs;
							auto inferPa = pa.AdjustForDecl(gfi.declSymbol, gfi.parentDeclType, false);

							// cannot pass Ptr<FunctionType> to this function since the last filled argument could be variadic
							// known variadic function argument should be treated as separated arguments
							// ParsingArguments need to be adjusted so that we can evaluate each parameter type
							ResolveFunctionParameters(pa, parameterAssignment, functionType, argTypes, boundedAnys);

							for (vint i = 0; i < gfi.spec->arguments.Count(); i++)
							{
								auto argument = gfi.spec->arguments[i];
								auto pattern = GetTemplateArgumentKey(argument, pa.tsys.Obj());

								if (i < gfi.filledArguments)
								{
									auto value = functionItem.tsys->GetParam(i);
									taContext.arguments.Add(pattern, value);
									if (!argument.ellipsis)
									{
										allArgs.Add(pattern, COUNT_ASSIGNED);
									}
									else if (value->GetType() == TsysType::Any)
									{
										allArgs.Add(pattern, COUNT_ANY);
									}
									else
									{
										allArgs.Add(pattern, value->GetParamCount());
									}
								}
								else
								{
									if (argument.argumentType == CppTemplateArgumentType::Value)
									{
										if (argument.ellipsis)
										{
											// TODO: not implemented
											throw 0;
										}
										else
										{
											taContext.arguments.Add(pattern, nullptr);
										}
									}
									allArgs.Add(pattern, COUNT_UNASSIGNED);
								}
							}

							InferTemplateArgumentsForFunctionType(inferPa, functionItem, functionType, parameterAssignment, taContext, allArgs);
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