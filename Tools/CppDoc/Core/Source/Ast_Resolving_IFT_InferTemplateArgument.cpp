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
		const SortedList<Expr*>&			involvedExprs;

		InferTemplateArgumentVisitor(const ParsingArguments& _pa, TemplateArgumentContext& _taContext, TemplateArgumentContext& _variadicContext, const SortedList<Symbol*>& _freeTypeSymbols, const SortedList<Type*>& _involvedTypes, const SortedList<Expr*>& _involvedExprs)
			:pa(_pa)
			, taContext(_taContext)
			, variadicContext(_variadicContext)
			, freeTypeSymbols(_freeTypeSymbols)
			, involvedTypes(_involvedTypes)
			, involvedExprs(_involvedExprs)
		{
		}

		void ExecuteInvolvedOnce(Ptr<Type>& argumentType, ITsys* _offeredType, bool _exactMatch = true)
		{
			if (_offeredType->GetType() == TsysType::GenericFunction)
			{
				if (_offeredType->GetElement()->GetType() != TsysType::DeclInstant)
				{
					return;
				}
			}
			else if (_offeredType->IsUnknownType())
			{
				return;
			}

			auto oldot = offeredType;
			auto oldem = exactMatch;

			offeredType = _offeredType;
			exactMatch = _exactMatch;
			argumentType->Accept(this);

			offeredType = oldot;
			exactMatch = oldem;
		}

		void ExecuteOnce(Ptr<Type>& argumentType, ITsys* _offeredType, bool _exactMatch = true)
		{
			if (!involvedTypes.Contains(argumentType.Obj())) return;
			ExecuteInvolvedOnce(argumentType, _offeredType, _exactMatch);
		}

		void Execute(Ptr<Type>& argumentType, ITsys* _offeredType)
		{
			InferTemplateArgumentVisitor(pa, taContext, variadicContext, freeTypeSymbols, involvedTypes, involvedExprs)
				.ExecuteOnce(argumentType, _offeredType);
		}

		void Execute(Ptr<Expr>& expr)
		{
			if (!involvedExprs.Contains(expr.Obj())) return;
			auto idExpr = expr.Cast<IdExpr>();
			auto symbol = idExpr->resolving->resolvedSymbols[0];
			auto& outputContext = symbol->ellipsis ? variadicContext : taContext;

			// consistent with GetTemplateArgumentKey
			auto pattern = pa.tsys->DeclOf(symbol);
			SetInferredResult(outputContext, pattern, nullptr);
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
							ExecuteInvolvedOnce(self->type, offeredType->CVOf({ true,false }));
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
							break;
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
			Execute(self->expr);
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

			TypeTsysList parameterAssignment;
			{
				vint count = entity->GetParamCount();
				Array<ExprTsysItem> argumentTypes(count);
				SortedList<vint> boundedAnys;
				for (vint i = 0; i < count; i++)
				{
					argumentTypes[i] = { nullptr,ExprTsysType::PRValue,entity->GetParam(i) };
				}
				ResolveFunctionParameters(pa, parameterAssignment, taContext, freeTypeSymbols, self, argumentTypes, boundedAnys);
			}
			InferTemplateArgumentsForFunctionType(pa, self, parameterAssignment, taContext, variadicContext, freeTypeSymbols, true);
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
			// do not perform type inferencing against decltype(expr)
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
			// do not perform type inferencing against ::
		}

		void Visit(IdType* self)override
		{
			auto patternSymbol = self->resolving->resolvedSymbols[0];
			auto& outputContext = patternSymbol->ellipsis ? variadicContext : taContext;

			// consistent with GetTemplateArgumentKey
			auto pattern = EvaluateGenericArgumentSymbol(patternSymbol);

			switch (patternSymbol->kind)
			{
			case symbol_component::SymbolKind::GenericTypeArgument:
				{
					// const, volatile, &, && are discarded
					TsysCV refCV;
					TsysRefType refType;
					auto entity = exactMatch ? offeredType : offeredType->GetEntity(refCV, refType);
					SetInferredResult(outputContext, pattern, entity);
				}
				break;
			case symbol_component::SymbolKind::GenericValueArgument:
				{
					SetInferredResult(outputContext, pattern, nullptr);
				}
				break;
			}
		}

		void Visit(ChildType* self)override
		{
			// do not perform type inferencing against Here::something
		}

		void Visit(GenericType* self)override
		{
			auto genericSymbol = self->type->resolving->resolvedSymbols[0];

			Ptr<TemplateSpec> spec;
			if (auto classDecl = genericSymbol->GetAnyForwardDecl<ClassDeclaration>())
			{
				spec = classDecl->templateSpec;
			}
			else
			{
				spec = EvaluateGenericArgumentSymbol(genericSymbol)->GetGenericFunction().spec;
			}

			if (!spec)
			{
				// this should not happen since CollectFreeTypes should stop the type inferencing before reaching here
				throw TypeCheckerException();
			}

			bool genericSymbolInvolved = involvedTypes.Contains(self->type.Obj());

			TsysCV refCV;
			TsysRefType refType;
			auto entity = exactMatch || genericSymbolInvolved
				? offeredType
				: offeredType->GetEntity(refCV, refType);
			{
				if (entity->GetType() != TsysType::DeclInstant)
				{
					// only DeclInstance could be a instance of a template class
					// base class conversion has been considered in InferFunctionType
					throw TypeCheckerException();
				}

				if (entity->GetDeclInstant().declSymbol != genericSymbol)
				{
					// only when self->type is a template template argument, it has a different symbol
					if (!genericSymbolInvolved)
					{
						throw TypeCheckerException();
					}
				}

				if (!entity->GetDeclInstant().taContext)
				{
					// TODO: remove this constraint in the future, it is allowed to be empty for type of *this
					throw TypeCheckerException();
				}
			}

			if (genericSymbolInvolved)
			{
				// entity must be DeclInstant
				auto& di = entity->GetDeclInstant();
				if (auto classDecl = di.declSymbol->GetAnyForwardDecl<ClassDeclaration>())
				{
					auto& ev = EvaluateClassSymbol(pa, classDecl.Obj(), di.parentDeclType, nullptr);
					if (ev.Get().Count() == 1 && ev.Get()[0]->GetType() == TsysType::GenericFunction)
					{
						Ptr<Type> idType = self->type;
						ExecuteInvolvedOnce(idType, ev.Get()[0]);
					}
				}
			}

			TypeTsysList parameterAssignment;
			{
				vint count = 0;
				for (vint i = 0; i < spec->arguments.Count(); i++)
				{
					auto argument = spec->arguments[i];
					auto value = entity->GetParam(i);
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
						auto argument = spec->arguments[i];
						auto value = entity->GetParam(i);

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
				ResolveGenericTypeParameters(pa, parameterAssignment, taContext, freeTypeSymbols, self, argumentTypes, boundedAnys);
			}
			InferTemplateArgumentsForGenericType(pa, self, parameterAssignment, taContext, variadicContext, freeTypeSymbols);
		}
	};

	void InferTemplateArgument(
		const ParsingArguments& pa,
		Ptr<Type> typeToInfer,
		Ptr<Expr> exprToInfer,
		ITsys* offeredType,
		TemplateArgumentContext& taContext,
		TemplateArgumentContext& variadicContext,
		const SortedList<Symbol*>& freeTypeSymbols,
		const SortedList<Type*>& involvedTypes,
		const SortedList<Expr*>& involvedExprs,
		bool exactMatchForParameters
	)
	{
		InferTemplateArgumentVisitor visitor(pa, taContext, variadicContext, freeTypeSymbols, involvedTypes, involvedExprs);
		if (typeToInfer) visitor.ExecuteOnce(typeToInfer, offeredType, exactMatchForParameters);
		if (exprToInfer) visitor.Execute(exprToInfer);
	}
}