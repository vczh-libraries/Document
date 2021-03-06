#include "IFT.h"
#include "AP.h"
#include "EvaluateSymbol.h"

using namespace symbol_type_resolving;

namespace infer_function_type
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
		ITsys**								lastAssignedVta;
		SortedList<ITsys*>&					hardcodedPatterns;

		InferTemplateArgumentVisitor(
			const ParsingArguments& _pa,
			TemplateArgumentContext& _taContext,
			TemplateArgumentContext& _variadicContext,
			const SortedList<Symbol*>& _freeTypeSymbols,
			const SortedList<Type*>& _involvedTypes,
			const SortedList<Expr*>& _involvedExprs,
			ITsys** _lastAssignedVta,
			SortedList<ITsys*>& _hardcodedPatterns
		)
			:pa(_pa)
			, taContext(_taContext)
			, variadicContext(_variadicContext)
			, freeTypeSymbols(_freeTypeSymbols)
			, involvedTypes(_involvedTypes)
			, involvedExprs(_involvedExprs)
			, lastAssignedVta(_lastAssignedVta)
			, hardcodedPatterns(_hardcodedPatterns)
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
			else if (_offeredType->IsUnknownType() || _offeredType->GetType()==TsysType::Nullptr)
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
			InferTemplateArgumentVisitor(pa, taContext, variadicContext, freeTypeSymbols, involvedTypes, involvedExprs, lastAssignedVta, hardcodedPatterns)
				.ExecuteOnce(argumentType, _offeredType);
		}

		void Execute(Ptr<Expr>& expr)
		{
			if (!involvedExprs.Contains(expr.Obj())) return;
			auto idExpr = expr.Cast<IdExpr>();
			auto patternSymbol = Resolving::EnsureSingleSymbol(idExpr->resolving);
			auto& outputContext = patternSymbol->ellipsis ? variadicContext : taContext;
			auto pattern = GetTemplateArgumentKey(patternSymbol);
			SetInferredResult(pa, outputContext, pattern, nullptr, lastAssignedVta, hardcodedPatterns);
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

				if (offeredType->GetType() == TsysType::Array && offeredType->GetParamCount() > 1)
				{
					ExecuteInvolvedOnce(elementType, offeredType->GetElement()->ArrayOf(offeredType->GetParamCount() - 1));
				}
				else
				{
					ExecuteInvolvedOnce(elementType, offeredType->GetElement());
				}
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

				if (entity->GetType() == TsysType::Array && entity->GetParamCount() > 1)
				{
					ExecuteInvolvedOnce(elementType, entity->GetElement()->ArrayOf(entity->GetParamCount() - 1));
				}
				else
				{
					ExecuteInvolvedOnce(elementType, entity->GetElement());
				}
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
			// match "const T" on "const int[]" should get "int[]"
			// match "T[]" on "int(const)[]" should get "const int"
			VisitArrayOrPtr(self->type, TsysType::Array);
			Execute(self->expr);
		}

		void Visit(DecorateType* self)override
		{
			// match "const T" on "const int[]" should get "int[]"
			// match "T[]" on "int(const)[]" should get "const int"
			TsysCV cv;
			TsysRefType refType;
			auto entity = GetArrayEntity(offeredType, cv, refType);

			if (exactMatch)
			{
				if (refType != TsysRefType::None) throw TypeCheckerException();

				bool expectAny = false;
				if (self->isConst && !cv.isGeneralConst) expectAny = true;
				if (self->isVolatile && !cv.isVolatile) expectAny = true;
				if (expectAny && (entity->GetType() != TsysType::Any && entity->GetType() != TsysType::GenericArg))
				{
					throw TypeCheckerException();
				}

				if (expectAny)
				{
					ExecuteInvolvedOnce(self->type, pa.tsys->Any());
				}
				else
				{
					if (self->isConst) cv.isGeneralConst = false;
					if (self->isVolatile) cv.isVolatile = false;
					ExecuteInvolvedOnce(self->type, entity->CVOf(cv));
				}
			}
			else
			{
				ExecuteInvolvedOnce(self->type, entity);
			}
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
					argumentTypes[i] = { {},entity->GetParam(i) };
				}
				assign_parameters::ResolveFunctionParameters(pa, parameterAssignment, taContext, freeTypeSymbols, nullptr, self, argumentTypes, boundedAnys);
			}
			InferTemplateArgumentsForFunctionType(pa, self, parameterAssignment, taContext, variadicContext, freeTypeSymbols, true, lastAssignedVta, hardcodedPatterns);
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

		void Visit(RootType* self)override
		{
			// do not perform type inferencing against ::
		}

		void Visit(IdType* self)override
		{
			auto patternSymbol = Resolving::EnsureSingleSymbol(self->resolving);
			auto& outputContext = patternSymbol->ellipsis ? variadicContext : taContext;
			auto pattern = GetTemplateArgumentKey(patternSymbol);

			switch (patternSymbol->kind)
			{
			case symbol_component::SymbolKind::GenericTypeArgument:
				{
					// const, volatile, &, && are discarded
					TsysCV refCV;
					TsysRefType refType;
					auto entity = exactMatch ? offeredType : offeredType->GetEntity(refCV, refType);
					if (entity->GetType() == TsysType::Zero)
					{
						entity = pa.tsys->Int();
					}
					SetInferredResult(pa, outputContext, pattern, entity, lastAssignedVta, hardcodedPatterns);
				}
				break;
			case symbol_component::SymbolKind::GenericValueArgument:
				{
					SetInferredResult(pa, outputContext, pattern, nullptr, lastAssignedVta, hardcodedPatterns);
				}
				break;
			}
		}

		void Visit(ChildType* self)override
		{
			// do not perform type inferencing against Here::something
		}

		void VisitGenericType(
			GenericType* self,
			Symbol* genericSymbol,
			const Ptr<TemplateSpec>& spec,
			ITsys* entity,
			bool genericSymbolInvolved)
		{
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
					// this should not happen, even *this will have taContext filled with arguments itselves
					throw TypeCheckerException();
				}
			}

			if (genericSymbolInvolved)
			{
				// entity must be DeclInstant
				auto& di = entity->GetDeclInstant();
				if (auto classDecl = di.declSymbol->GetAnyForwardDecl<ForwardClassDeclaration>())
				{
					auto& classTsys = EvaluateForwardClassSymbol(pa, classDecl.Obj(), di.parentDeclType, nullptr);
					if (classTsys.Count() == 1 && classTsys[0]->GetType() == TsysType::GenericFunction)
					{
						Ptr<Type> idType = self->type;
						ExecuteInvolvedOnce(idType, classTsys[0]);
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
						if (value->GetType() == TsysType::Init)
						{
							count += value->GetParamCount();
						}
						else
						{
							count += 1;
						}
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
							if (value->GetType() == TsysType::Init)
							{
								for (vint j = 0; j < value->GetParamCount(); j++)
								{
									argumentTypes[index++] = { value->GetInit().headers[j],value->GetParam(j) };
								}
							}
							else
							{
								boundedAnys.Add(index);
								argumentTypes[index++] = { nullptr,ExprTsysType::PRValue,value };
							}
						}
						else
						{
							argumentTypes[index++] = { nullptr,ExprTsysType::PRValue,value };
						}
					}
				}
				assign_parameters::ResolveGenericTypeParameters(pa, parameterAssignment, taContext, freeTypeSymbols, self, argumentTypes, boundedAnys);
			}
			InferTemplateArgumentsForGenericType(pa, self, parameterAssignment, taContext, variadicContext, freeTypeSymbols, lastAssignedVta, hardcodedPatterns);
		}

		void Visit(GenericType* self)override
		{
			auto genericSymbol = Resolving::EnsureSingleSymbol(self->type->resolving);

			auto spec = GetTemplateSpecFromSymbol(genericSymbol);
			if (!spec)
			{
				spec = EvaluateGenericArgumentType(genericSymbol)->GetGenericFunction().spec;
			}

			if (!spec)
			{
				// this should not happen since CollectFreeTypes should stop the type inferencing before reaching here
				throw TypeCheckerException();
			}

			bool genericSymbolInvolved = involvedTypes.Contains(self->type.Obj());

			TsysCV refCV;
			TsysRefType refType;
			auto entityUnensured = exactMatch || genericSymbolInvolved
				? offeredType
				: offeredType->GetEntity(refCV, refType);

			bool enumerated = false;
			EnumerateClassPrimaryInstances(pa, entityUnensured, false, [&](ITsys* entity)
			{
				if (!enumerated)
				{
					enumerated = true;
					VisitGenericType(self, genericSymbol, spec, entity, genericSymbolInvolved);
				}
				else
				{
					throw L"No primary instance is enumerated!";
				}
				return false;
			});

			if (!enumerated)
			{
				throw L"No primary instance is enumerated!";
			}
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
		bool exactMatchForParameters,
		ITsys** lastAssignedVta,
		SortedList<ITsys*>& hardcodedPatterns
	)
	{
		InferTemplateArgumentVisitor visitor(pa, taContext, variadicContext, freeTypeSymbols, involvedTypes, involvedExprs, lastAssignedVta, hardcodedPatterns);
		if (typeToInfer) visitor.ExecuteOnce(typeToInfer, offeredType, exactMatchForParameters);
		if (exprToInfer) visitor.Execute(exprToInfer);
	}
}