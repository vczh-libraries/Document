#include "Ast_Evaluate_ExpandPotentialVta.h"

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
		bool isVta1 = false;
		TypeToTsysInternal(pa, self->type, items1, isVta1);
		isVta = ExpandPotentialVta(pa, result, [this, self](ExprTsysItem arg1)
		{
			return ProcessArrayType(pa, self, arg1);
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
		TypeToTsysInternal(pa, self->type, types, typesVta, { config.idExprToInstant, true, config.cc });
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
	// RootType
	//////////////////////////////////////////////////////////////////////////////////////

	void Visit(RootType* self)override
	{
		throw NotConvertableException();
	}

	//////////////////////////////////////////////////////////////////////////////////////
	// IdType
	//////////////////////////////////////////////////////////////////////////////////////

	void Visit(IdType* self)override
	{
		CreateIdReferenceType(pa, self->resolving, false, true, result, isVta);
	}

	//////////////////////////////////////////////////////////////////////////////////////
	// ChildType
	//////////////////////////////////////////////////////////////////////////////////////

	void Visit(ChildType* self)override
	{
		if (!IsResolvedToType(self->classType))
		{
			CreateIdReferenceType(pa, self->resolving, true, false, result, isVta);
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
		variadicInput.ApplySingle<Type>(0, self->type, TypeToTsysConfig::ExpectTemplate());
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
	if (!t) throw NotConvertableException();
	TypeToTsysVisitor visitor(pa, tsys, nullptr, config);
	t->Accept(&visitor);
	isVta = visitor.isVta;

	if (dynamic_cast<IdType*>(t) && config.idExprToInstant)
	{
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

		List<ITsys*> classTypes;
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
						if (classType->GetDecl()->GetAnyForwardDecl<ForwardClassDeclaration>()->templateSpec)
						{
							if (!classTypes.Contains(classType))
							{
								classTypes.Add(classType);
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
							if (decl->templateSpec)
							{
								auto& ev = EvaluateForwardClassSymbol(rootPa, decl.Obj(), nullptr, nullptr);
								for (vint i = 0; i < ev.Count(); i++)
								{
									auto classType = ev[i];
									if (classType->GetType() == TsysType::GenericFunction)
									{
										classType = classType->GetElement();
									}
									if (!classTypes.Contains(classType))
									{
										classTypes.Add(classType);
									}
								}
							}
						}
						break;
					}
					current = current->GetParentScope();
				}
			}
		}

		if (classTypes.Count() > 0)
		{
			ParsingArguments rootPa = pa.WithScope(pa.root.Obj());
			for (vint i = 0; i < genericTypes.Count(); i++)
			{
				auto& targetTsys = tsys[genericTypes[i]];
				auto targetDecl = targetTsys->GetElement()->GetDecl();
				auto classDecl = targetDecl->GetImplDecl_NFb<ClassDeclaration>();
				if (classDecl)
				{
					auto& evTypes = EvaluateForwardClassSymbol(rootPa, classDecl.Obj(), nullptr, nullptr);
					if (evTypes[0] == targetTsys)
					{
						for (vint j = 0; j < classTypes.Count(); j++)
						{
							auto classType = classTypes[j];
							if (classType->GetDecl() == targetDecl)
							{
								targetTsys = classType->ReplaceGenericArgs(pa);
								goto FINISH;
							}
						}
					}
				}
			FINISH:;
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
		throw NotConvertableException();
	}
}

void TypeToTsysNoVta(const ParsingArguments& pa, Ptr<Type> t, TypeTsysList& tsys, TypeToTsysConfig config)
{
	TypeToTsysNoVta(pa, t.Obj(), tsys, config);
}

void TypeToTsysAndReplaceFunctionReturnType(const ParsingArguments& pa, Ptr<Type> t, TypeTsysList& returnTypes, TypeTsysList& tsys, bool memberOf)
{
	if (!t) throw NotConvertableException();
	TypeToTsysVisitor visitor(pa, tsys, &returnTypes, TypeToTsysConfig::MemberOf(memberOf));
	t->Accept(&visitor);
	if (visitor.isVta)
	{
		throw NotConvertableException();
	}
}