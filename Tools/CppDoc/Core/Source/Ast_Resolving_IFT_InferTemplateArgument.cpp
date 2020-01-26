#include "Ast_Resolving_IFT.h"

namespace symbol_type_resolving
{
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
			if (entity->GetType() != TsysType::Function)
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
			if (!self->type->resolving || self->type->resolving->resolvedSymbols.Count() != 1)
			{
				throw TypeCheckerException();
			}

			auto genericSymbol = self->type->resolving->resolvedSymbols[0];
			auto classDecl = genericSymbol->GetAnyForwardDecl<ClassDeclaration>();
			if (!classDecl || !classDecl->templateSpec)
			{
				// only do type inferencing for instance of template classes
				// it could be an instance of a template type alias, which is not supported
				throw TypeCheckerException();
			}

			if (involvedTypes.Contains(self->type.Obj()))
			{
				// TODO: remove this constraint in the future, it could be a template template argument, or a generic class inside another involved class
				throw 0;
			}

			auto entity = offeredType;
			if (exactMatch)
			{
				if (entity->GetType() != TsysType::DeclInstant)
				{
					// only DeclInstance could be a instance of a template class
					throw TypeCheckerException();
				}

				if (entity->GetDeclInstant().declSymbol != genericSymbol)
				{
					// TODO: only check this when self->type is not involved
					throw TypeCheckerException();
				}

				if (!entity->GetDeclInstant().taContext)
				{
					// TODO: remove this constraint in the future, it is allowed to be empty for type of *this
					throw TypeCheckerException();
				}
			}
			else
			{
				TsysCV refCV;
				TsysRefType refType;
				entity = entity->GetEntity(refCV, refType);

				if (entity->GetType() != TsysType::DeclInstant)
				{
					// TODO: check base class of entity, it could also be Decl
					throw TypeCheckerException();
				}

				if (entity->GetDeclInstant().declSymbol != genericSymbol)
				{
					// TODO: only check this when self->type is not involved
					throw TypeCheckerException();
				}

				if (!entity->GetDeclInstant().taContext)
				{
					// TODO: remove this constraint in the future, it is allowed to be empty for type of *this
					throw TypeCheckerException();
				}
			}

			List<ITsys*> parameterAssignment;
			{
				auto spec = classDecl->templateSpec;

				vint count = 0;
				for (vint i = 0; i < spec->arguments.Count(); i++)
				{
					auto argument = spec->arguments[0];
					auto value = entity->GetParam(0);
					if (argument.ellipsis)
					{
						if (value->GetType() != TsysType::Init)
						{
							// it is impossible to have any_t here
							throw TypeCheckerException();
						}
						count += value->GetParamCount();
					}
					else
					{
						count++;
					}
				}

				Array<ExprTsysItem> argumentTypes(count);
				SortedList<vint> boundedAnys;

				{
					vint index = 0;
					const auto& di = entity->GetDeclInstant();
					for (vint i = 0; i < spec->arguments.Count(); i++)
					{
						auto argument = spec->arguments[0];
						auto value = entity->GetParam(0);

						if (argument.ellipsis)
						{
							for (vint j = 0; j < value->GetParamCount(); j++)
							{
								argumentTypes[index++] = { nullptr,ExprTsysType::PRValue,value->GetParam(j) };
							}
						}
						else
						{
							argumentTypes[index++] = { nullptr,ExprTsysType::PRValue,value };
						}
					}
				}
				ResolveGenericTypeParameters(pa, parameterAssignment, self, argumentTypes, boundedAnys);
			}
			InferTemplateArgumentsForGenericType(pa, self, parameterAssignment, taContext, freeTypeSymbols);
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
}