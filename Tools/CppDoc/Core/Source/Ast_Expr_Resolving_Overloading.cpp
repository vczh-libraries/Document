#include "Ast_Expr_Resolving.h"

namespace symbol_type_resolving
{

	/***********************************************************************
	VisitOverloadedFunction: Select good candidates from overloaded functions
	***********************************************************************/

	void VisitOverloadedFunction(ParsingArguments& pa, ExprTsysList& funcTypes, List<Ptr<ExprTsysList>>& argTypesList, ExprTsysList& result)
	{
		Array<vint> funcDPs(funcTypes.Count());
		for (vint i = 0; i < funcTypes.Count(); i++)
		{
			funcDPs[i] = 0;
			if (auto symbol = funcTypes[i].symbol)
			{
				if (symbol->decls.Count() == 1)
				{
					if (auto decl = symbol->decls[0].Cast<ForwardFunctionDeclaration>())
					{
						if (auto type = GetTypeWithoutMemberAndCC(decl->type).Cast<FunctionType>())
						{
							for (vint j = 0; j < type->parameters.Count(); j++)
							{
								if (type->parameters[j]->initializer)
								{
									funcDPs[i] = type->parameters.Count() - j;
									goto EXAMINE_NEXT_FUNCTION;
								}
							}
						EXAMINE_NEXT_FUNCTION:;
						}
					}
				}
			}
		}

		ExprTsysList validFuncTypes;
		for (vint i = 0; i < funcTypes.Count(); i++)
		{
			auto funcType = funcTypes[i];

			vint funcParamCount = funcType.tsys->GetParamCount();
			vint missParamCount = funcParamCount - argTypesList.Count();
			if (missParamCount > 0)
			{
				if (missParamCount > funcDPs[i])
				{
					continue;
				}
			}
			else if (missParamCount < 0)
			{
				if (!funcType.tsys->GetFunc().ellipsis)
				{
					continue;
				}
			}

			validFuncTypes.Add(funcType);
		}

		Array<bool> selectedIndices(validFuncTypes.Count());
		for (vint i = 0; i < validFuncTypes.Count(); i++)
		{
			selectedIndices[i] = true;
		}

		for (vint i = 0; i < argTypesList.Count(); i++)
		{
			Array<TsysConv> funcChoices(validFuncTypes.Count());

			for (vint j = 0; j < validFuncTypes.Count(); j++)
			{
				auto funcType = validFuncTypes[j];

				vint funcParamCount = funcType.tsys->GetParamCount();
				if (funcParamCount <= i)
				{
					funcChoices[j] = TsysConv::Ellipsis;
					continue;
				}

				auto paramType = funcType.tsys->GetParam(i);
				auto& argTypes = *argTypesList[i].Obj();

				auto bestChoice = TsysConv::Illegal;
				for (vint k = 0; k < argTypes.Count(); k++)
				{
					auto choice = TestConvert(pa, paramType, argTypes[k]);
					if ((vint)bestChoice > (vint)choice) bestChoice = choice;
				}
				funcChoices[j] = bestChoice;
			}

			auto min = FindMinConv(funcChoices);
			if (min == TsysConv::Illegal)
			{
				return;
			}

			for (vint j = 0; j < validFuncTypes.Count(); j++)
			{
				if (funcChoices[j] != min)
				{
					selectedIndices[j] = false;
				}
			}
		}

		for (vint i = 0; i < selectedIndices.Count(); i++)
		{
			if (selectedIndices[i])
			{
				AddTemp(result, validFuncTypes[i].tsys->GetElement());
			}
		}
	}

	/***********************************************************************
	SearchAdlClassesAndNamespaces: Preparing for argument-dependent lookup
	***********************************************************************/

	void SearchAdlClassesAndNamespaces(ParsingArguments& pa, Symbol* symbol, SortedList<Symbol*>& nss, SortedList<Symbol*>& classes);

	class SearchBaseTypeAdlClassesAndNamespacesVisitor : public Object, public virtual ITypeVisitor
	{
	public:
		ParsingArguments&			pa;
		SortedList<Symbol*>&		nss;
		SortedList<Symbol*>&		classes;

		SearchBaseTypeAdlClassesAndNamespacesVisitor(ParsingArguments& _pa, SortedList<Symbol*>& _nss, SortedList<Symbol*>& _classes)
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
		}

		void Visit(VariadicTemplateArgumentType* self)override
		{
		}
	};

	void SearchAdlClassesAndNamespaces(ParsingArguments& pa, Symbol* symbol, SortedList<Symbol*>& nss, SortedList<Symbol*>& classes)
	{
		while (symbol)
		{
			if (symbol->decls.Count() > 0)
			{
				if (auto classDecl = symbol->decls[0].Cast<ClassDeclaration>())
				{
					if (!classes.Contains(symbol))
					{
						classes.Add(symbol);
					}

					ParsingArguments classPa(pa, symbol);
					classPa.funcSymbol = nullptr;

					SearchBaseTypeAdlClassesAndNamespacesVisitor visitor(classPa, nss, classes);
					for (vint i = 0; i < classDecl->baseTypes.Count(); i++)
					{
						classDecl->baseTypes[i].f1->Accept(&visitor);
					}
				}
				else if (auto namespaceDecl = symbol->decls[0].Cast<NamespaceDeclaration>())
				{
					if (!nss.Contains(symbol))
					{
						nss.Add(symbol);
					}
					break;
				}
				else
				{
					break;
				}
			}
			else
			{
				break;
			}
			symbol = symbol->parent;
		}
	}

	void SearchAdlClassesAndNamespaces(ParsingArguments& pa, ITsys* type, SortedList<Symbol*>& nss, SortedList<Symbol*>& classes)
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
		case TsysType::Generic:
			SearchAdlClassesAndNamespaces(pa, type->GetElement(), nss, classes);
			for (vint i = 0; i < type->GetParamCount(); i++)
			{
				SearchAdlClassesAndNamespaces(pa, type->GetParam(i), nss, classes);
			}
			break;
		}
	}

	void SearchAdlClassesAndNamespaces(ParsingArguments& pa, TypeTsysList& types, SortedList<Symbol*>& nss, SortedList<Symbol*>& classes)
	{
		for (vint i = 0; i < types.Count(); i++)
		{
			SearchAdlClassesAndNamespaces(pa, types[i], nss, classes);
		}
	}

	void SearchAdlClassesAndNamespaces(ParsingArguments& pa, ExprTsysList& types, SortedList<Symbol*>& nss, SortedList<Symbol*>& classes)
	{
		for (vint i = 0; i < types.Count(); i++)
		{
			SearchAdlClassesAndNamespaces(pa, types[i].tsys, nss, classes);
		}
	}
}