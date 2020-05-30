#include "Ast_Evaluate_ExpandPotentialVta.h"
#include "Parser_Declarator.h"
#include "EvaluateSymbol.h"

using namespace symbol_type_resolving;
using namespace symbol_totsys_impl;

/***********************************************************************
TypeToTsys
  PrimitiveType				: literal
  ReferenceType				: unbounded
  ArrayType					: unbounded
  CallingConventionType		: unbounded		*
  FunctionType				: variant
  MemberType				: unbounded
  DeclType					: unbounded
  DecorateType				: unbounded
  RootType					: literal		*
  IdType					: identifier
  ChildType					: unbounded
  GenericType				: variant
***********************************************************************/

class TypeToTsysVisitor : public Object, public virtual ITypeVisitor
{
public:
	TypeTsysList&				result;
	bool						isVta = false;

	const ParsingArguments&		pa;
	TypeTsysList*				returnTypes;
	TypeToTsysConfig			config;

	TypeToTsysVisitor(const ParsingArguments& _pa, TypeTsysList& _result, TypeTsysList* _returnTypes, TypeToTsysConfig _config)
		:pa(_pa)
		, result(_result)
		, returnTypes(_returnTypes)
		, config(_config)
	{
	}

	void AddResult(ITsys* tsys)
	{
		AddTsysToResult(result, tsys);
	}

	//////////////////////////////////////////////////////////////////////////////////////
	// PrimitiveType
	//////////////////////////////////////////////////////////////////////////////////////

	void Visit(PrimitiveType* self)override
	{
		AddResult(ProcessPrimitiveType(pa, self));
	}

	//////////////////////////////////////////////////////////////////////////////////////
	// ReferenceType
	//////////////////////////////////////////////////////////////////////////////////////

	void Visit(ReferenceType* self)override
	{
		TypeTsysList items1;
		bool isVta1 = false;
		TypeToTsysInternal(pa, self->type, items1, isVta1);
		isVta = ExpandPotentialVta(pa, result, [this, self](ExprTsysItem arg1)
		{
			return ProcessReferenceType(pa, self, arg1);
		}, Input(items1, isVta1));
	}

	//////////////////////////////////////////////////////////////////////////////////////
	// ArrayType
	//////////////////////////////////////////////////////////////////////////////////////

	void Visit(ArrayType* self)override
	{
		TypeTsysList items1;
		ExprTsysList items2;
		bool isVta1 = false, isVta2 = false;

		TypeToTsysInternal(pa, self->type, items1, isVta1);
		if (self->expr)
		{
			ExprToTsysInternal(pa, self->expr, items2, isVta2);
		}
		else
		{
			items2.Add({ nullptr,ExprTsysType::PRValue,pa.tsys->Any() });
		}

		isVta = ExpandPotentialVta(pa, result, [this, self](ExprTsysItem arg1, ExprTsysItem arg2)
		{
			return ProcessArrayType(pa, self, arg1);
		}, Input(items1, isVta1), Input(items2, isVta2));
	}

	//////////////////////////////////////////////////////////////////////////////////////
	// DecorateType
	//////////////////////////////////////////////////////////////////////////////////////

	void Visit(DecorateType* self)override
	{
		TypeTsysList items1;
		bool isVta1 = false;
		TypeToTsysInternal(pa, self->type, items1, isVta1);
		isVta = ExpandPotentialVta(pa, result, [this, self](ExprTsysItem arg1)
		{
			return ProcessDecorateType(pa, self, arg1);
		}, Input(items1, isVta1));
	}

	//////////////////////////////////////////////////////////////////////////////////////
	// CallingConventionType
	//////////////////////////////////////////////////////////////////////////////////////

	void Visit(CallingConventionType* self)override
	{
		auto oldCc = config.cc;
		config.cc = self->callingConvention;

		self->type->Accept(this);
		config.cc = oldCc;
	}

	//////////////////////////////////////////////////////////////////////////////////////
	// FunctionType
	//////////////////////////////////////////////////////////////////////////////////////

	void Visit(FunctionType* self)override
	{
		VariadicInput<ITsys*> variadicInput(self->parameters.Count() + 1, pa);
		if (returnTypes)
		{
			variadicInput.ApplyTypes(0, *returnTypes, false);
		}
		else if (self->decoratorReturnType)
		{
			variadicInput.ApplySingle(0, self->decoratorReturnType);
		}
		else if (self->returnType)
		{
			variadicInput.ApplySingle(0, self->returnType);
		}
		else
		{
			TypeTsysList types;
			types.Add(pa.tsys->Void());
			variadicInput.ApplyTypes(0, types, false);
		}
		variadicInput.ApplyVariadicList(1, self->parameters);

		isVta = variadicInput.Expand(&self->parameters, result,
			[this, self](ExprTsysList& processResult, Array<ExprTsysItem>& args, vint unboundedVtaIndex, Array<vint>& argSource, SortedList<vint>& boundedAnys)
			{
				ProcessFunctionType(pa, processResult, self, config.cc, config.memberOf, args, boundedAnys);
			});
	}

	//////////////////////////////////////////////////////////////////////////////////////
	// MemberType
	//////////////////////////////////////////////////////////////////////////////////////

	void Visit(MemberType* self)override
	{
		TypeTsysList types, classTypes;
		bool typesVta = false;
		bool classTypesVta = false;
		TypeToTsysInternal(pa, self->type, types, typesVta, { config.idTypeNoPrimaryClass, config.idTypeNoGenericFunction, true, config.cc });
		TypeToTsysInternal(pa, self->classType, classTypes, classTypesVta);
		isVta = ExpandPotentialVta(pa, result, [this, self](ExprTsysItem argType, ExprTsysItem argClass)
		{
			return ProcessMemberType(pa, self, argType, argClass);
		}, Input(types, typesVta), Input(classTypes, classTypesVta));
	}

	//////////////////////////////////////////////////////////////////////////////////////
	// DeclType
	//////////////////////////////////////////////////////////////////////////////////////

	void Visit(DeclType* self)override
	{
		if (self->expr)
		{
			ExprTsysList types;
			bool typesVta = false;
			ExprToTsysInternal(pa, self->expr, types, typesVta);
			isVta = ExpandPotentialVta(pa, result, [this, self](ExprTsysItem arg1)
			{
				return ProcessDeclType(pa, self, arg1);
			}, Input(types, typesVta));
		}
	}

	//////////////////////////////////////////////////////////////////////////////////////
	// RootType
	//////////////////////////////////////////////////////////////////////////////////////

	void Visit(RootType* self)override
	{
		throw TypeCheckerException();
	}

	//////////////////////////////////////////////////////////////////////////////////////
	// IdType
	//////////////////////////////////////////////////////////////////////////////////////

	void Visit(IdType* self)override
	{
		auto resolving = ResolveInCurrentContext(pa, self->name, self->resolving, [&]()
		{
			return ResolveSymbolInContext(pa, self->name, self->cStyleTypeReference).types;
		});

		CreateIdReferenceType(pa, resolving, false, true, result, isVta);
	}

	//////////////////////////////////////////////////////////////////////////////////////
	// ChildType
	//////////////////////////////////////////////////////////////////////////////////////

	void Visit(ChildType* self)override
	{
		if (!IsResolvedToType(self->classType))
		{
			auto resolving = ResolveInCurrentContext(pa, self->name, self->resolving, [&]()
			{
				return ResolveChildSymbol(pa, self->classType, self->name).types;
			});

			CreateIdReferenceType(pa, resolving, true, false, result, isVta);
			return;
		}

		TypeTsysList classTypes;
		bool classVta = false;
		TypeToTsysInternal(pa, self->classType, classTypes, classVta);

		isVta = ExpandPotentialVtaMultiResult(pa, result, [this, self](ExprTsysList& processResult, ExprTsysItem argClass)
		{
			ProcessChildType(pa, self, argClass, processResult);
		}, Input(classTypes, classVta));
	}

	//////////////////////////////////////////////////////////////////////////////////////
	// GenericType
	//////////////////////////////////////////////////////////////////////////////////////

	void Visit(GenericType* self)override
	{
		vint count = self->arguments.Count() + 1;
		Array<bool> isTypes(count);
		isTypes[0] = false;

		VariadicInput<ExprTsysItem> variadicInput(count, pa);
		variadicInput.ApplySingle<Type>(0, self->type, TypeToTsysConfig::ToBeAppliedTemplateArguments());
		variadicInput.ApplyGenericArguments(1, isTypes, self->arguments);
		isVta = variadicInput.Expand(&self->arguments, result,
			[this, self, &isTypes](ExprTsysList& processResult, Array<ExprTsysItem>& args, vint unboundedVtaIndex, Array<vint>& argSource, SortedList<vint>& boundedAnys)
			{
				ProcessGenericType(pa, processResult, self, isTypes, args, argSource, boundedAnys);
			});
	}
};

// Convert type AST to type system object

void TypeToTsysInternal(const ParsingArguments& pa, Type* t, TypeTsysList& tsys, bool& isVta, TypeToTsysConfig config)
{
	if (!t) throw TypeCheckerException();
	TypeToTsysVisitor visitor(pa, tsys, nullptr, config);
	t->Accept(&visitor);
	isVta = visitor.isVta;

	if (dynamic_cast<IdType*>(t))
	{
		ParsingArguments rootPa = pa.WithScope(pa.root.Obj());

		if (!config.idTypeNoPrimaryClass)
		{
			// when config.idTypeNoPrimaryClass == false
			// if an IdType evaluates to a Decl or DeclInstant to an instance of a partial specialized class
			// it is converted to the primary class template
			// because when we call A<...> in a partial specialization of A<T>, we want to primary class so that we can apply arguments to the correct type

			for (vint i = 0; i < tsys.Count(); i++)
			{
				auto classType = tsys[i];
				if (classType->GetType() == TsysType::GenericFunction)
				{
					classType = classType->GetElement();
				}

				ITsys* parentDeclType = nullptr;
				switch (classType->GetType())
				{
				case TsysType::DeclInstant:
					parentDeclType = classType->GetDeclInstant().parentDeclType;
				case TsysType::Decl:
					{
						auto classSymbol = classType->GetDecl();
						if (auto primarySymbol = classSymbol->GetPSPrimary_NF())
						{
							if (primarySymbol != classSymbol)
							{
								if (auto classDecl = primarySymbol->GetAnyForwardDecl<ForwardClassDeclaration>())
								{
									auto& evTypes = EvaluateForwardClassSymbol(rootPa, classDecl.Obj(), parentDeclType, nullptr);
									tsys[i] = evTypes.Get(0);
								}
							}
						}
					}
					break;
				}
			}
		}

		if (config.idTypeNoGenericFunction)
		{
			// when config.idTypeNoGenericFunction == true
			// if an IdType evaluates to a GenericFunction of a template class
			// it removes the GenericFunction layer and get its DeclInstant instead
			// because we want any A except A<...> to be converted to A<T>, when the code is in class template<typename T> class T.

			// find all indices of GenericFunction
			List<vint> genericTypes;
			for (vint i = 0; i < tsys.Count(); i++)
			{
				if (tsys[i]->GetType() == TsysType::GenericFunction)
				{
					switch (tsys[i]->GetElement()->GetType())
					{
					case TsysType::DeclInstant:
						genericTypes.Add(i);
						break;
					}
				}
			}
			if (genericTypes.Count() == 0) return;

			// find a containing classes
			Dictionary<Symbol*, TemplateArgumentContext*> genericContainerClasses;
			{
				auto rootPa = pa.WithScope(pa.root.Obj());
				auto current = pa.scopeSymbol;
				while (current)
				{
					if (auto cache = current->GetClassMemberCache_NFb())
					{
						for (vint i = 0; i < cache->containerClassTypes.Count(); i++)
						{
							auto classType = cache->containerClassTypes[i];
							auto classSymbol = classType->GetDecl();
							if (auto spec = classSymbol->GetAnyForwardDecl<ForwardClassDeclaration>()->templateSpec)
							{
								if (spec->arguments.Count() > 0)
								{
									genericContainerClasses.Add(classSymbol, classType->GetDeclInstant().taContext.Obj());
								}
							}
						}
						current = cache->parentScope;
					}
					else
					{
						switch (current->kind)
						{
						case CLASS_SYMBOL_KIND:
							{
								auto decl = current->GetAnyForwardDecl<ForwardClassDeclaration>();
								if (decl->templateSpec && decl->templateSpec->arguments.Count() > 0)
								{
									genericContainerClasses.Add(current, nullptr);
								}
							}
							break;
						}
						current = current->GetParentScope();
					}
				}
			}
			if (genericContainerClasses.Count() == 0) return;

			// if there is any GenericFunction to any of containing classes
			// convert it to DeclInstant
			
			for (vint i = 0; i < genericTypes.Count(); i++)
			{
				auto& targetTsys = tsys[genericTypes[i]];
				auto targetDecl = targetTsys->GetElement()->GetDecl();
				auto classDecl = targetDecl->GetAnyForwardDecl<ForwardClassDeclaration>();
				if (classDecl)
				{
					vint index = genericContainerClasses.Keys().IndexOf(targetDecl);
					if (index != -1)
					{
						auto diTsys = targetTsys->GetElement();
						auto& di = diTsys->GetDeclInstant();
						auto ata = pa.taContext;
						while (ata && ata->GetSymbolToApply() != targetDecl)
						{
							ata = ata->parent;
						}

						if (!ata)
						{
							ata = genericContainerClasses.Values()[index];
						}

						if (ata)
						{
							auto& evTypes = EvaluateForwardClassSymbol(rootPa, classDecl.Obj(), di.parentDeclType, ata);
							auto evTsys = evTypes[0];
							if (evTsys->GetType() == TsysType::GenericFunction)
							{
								evTsys = evTsys->GetElement();
							}
							targetTsys = evTsys;
						}
						else
						{
							targetTsys = targetTsys->GetElement();
						}
					}
				}
			}
		}
	}
}

void TypeToTsysInternal(const ParsingArguments& pa, Ptr<Type> t, TypeTsysList& tsys, bool& isVta, TypeToTsysConfig config)
{
	TypeToTsysInternal(pa, t.Obj(), tsys, isVta, config);
}

void TypeToTsysNoVta(const ParsingArguments& pa, Type* t, TypeTsysList& tsys, TypeToTsysConfig config)
{
	bool isVta = false;
	TypeToTsysInternal(pa, t, tsys, isVta, config);
	if (isVta)
	{
		throw TypeCheckerException();
	}
}

void TypeToTsysNoVta(const ParsingArguments& pa, Ptr<Type> t, TypeTsysList& tsys, TypeToTsysConfig config)
{
	TypeToTsysNoVta(pa, t.Obj(), tsys, config);
}

void TypeToTsysAndReplaceFunctionReturnType(const ParsingArguments& pa, Ptr<Type> t, TypeTsysList& returnTypes, TypeTsysList& tsys, bool memberOf)
{
	if (!t) throw TypeCheckerException();
	TypeToTsysVisitor visitor(pa, tsys, &returnTypes, TypeToTsysConfig::MemberOf(memberOf));
	t->Accept(&visitor);
	if (visitor.isVta)
	{
		throw TypeCheckerException();
	}
}