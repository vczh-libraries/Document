#include "Ast_Resolving.h"

namespace symbol_type_resolving
{

	/***********************************************************************
	VisitOverloadedFunction: Select good candidates from overloaded functions
	***********************************************************************/

	void VisitOverloadedFunction(const ParsingArguments& pa, ExprTsysList& funcTypes, List<Ptr<ExprTsysList>>& argTypesList, ExprTsysList& result)
	{
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
	IsAdlEnabled: Check if argument-dependent lookup is considered according to the unqualified lookup result
	***********************************************************************/

	bool IsAdlEnabled(const ParsingArguments& pa, Ptr<Resolving> resolving)
	{
		for (vint i = 0; i < resolving->resolvedSymbols.Count(); i++)
		{
			auto symbol = resolving->resolvedSymbols[i];
			if (symbol->GetAnyForwardDecl<ForwardFunctionDeclaration>())
			{
				auto parent = symbol->parent;
				while (parent)
				{
					switch (parent->kind)
					{
					case symbol_component::SymbolKind::Root:
					case symbol_component::SymbolKind::Namespace:
						parent = parent->parent;
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
		}

		void Visit(VariadicTemplateArgumentType* self)override
		{
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
			else if (auto classDecl = symbol->declaration.Cast<ClassDeclaration>())
			{
				if (!classes.Contains(symbol))
				{
					classes.Add(symbol);
				}

				if (symbol == firstSymbol)
				{
					auto classPa = pa.WithContextNoFunction(symbol);

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
			symbol = symbol->parent;
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
		case TsysType::Generic:
			SearchAdlClassesAndNamespaces(pa, type->GetElement(), nss, classes);
			for (vint i = 0; i < type->GetParamCount(); i++)
			{
				SearchAdlClassesAndNamespaces(pa, type->GetParam(i), nss, classes);
			}
			break;
		}
	}

	void SearchAdlClassesAndNamespaces(const ParsingArguments& pa, ExprTsysList& types, SortedList<Symbol*>& nss, SortedList<Symbol*>& classes)
	{
		for (vint i = 0; i < types.Count(); i++)
		{
			SearchAdlClassesAndNamespaces(pa, types[i].tsys, nss, classes);
		}
	}

	/***********************************************************************
	SerachAdlFunction: Find functions in namespaces
	***********************************************************************/

	void SerachAdlFunction(const ParsingArguments& pa, SortedList<Symbol*>& nss, const WString& name, ExprTsysList& result)
	{
		for (vint i = 0; i < nss.Count(); i++)
		{
			auto ns = nss[i];
			vint index = ns->children.Keys().IndexOf(name);
			if (index != -1)
			{
				auto& children = ns->children.GetByIndex(index);
				for (vint j = 0; j < children.Count(); j++)
				{
					auto child = children[j].Obj();
					if (child->kind == symbol_component::SymbolKind::Function)
					{
						VisitSymbol(pa, nullptr, child, false, result);
					}
				}
			}
		}
	}
}