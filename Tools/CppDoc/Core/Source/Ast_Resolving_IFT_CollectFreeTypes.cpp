#include "Ast_Resolving_IFT.h"

using namespace symbol_type_resolving;

namespace infer_function_type
{
	/***********************************************************************
	CollectFreeTypes:	Collect and check all involved free types from a type
						Stops at nested variadic types
	***********************************************************************/

	class CollectFreeTypesVisitor : public Object, public virtual ITypeVisitor
	{
	public:
		bool								involved = false;
		const ParsingArguments&				pa;
		bool								includeParentDeclArguments;
		bool								insideVariant;
		const SortedList<Symbol*>&			freeTypeSymbols;
		SortedList<Type*>&					involvedTypes;
		SortedList<Expr*>&					involvedExprs;

		CollectFreeTypesVisitor(const ParsingArguments& _pa, bool _includeParentDeclArguments, bool _insideVariant, const SortedList<Symbol*>& _freeTypeSymbols, SortedList<Type*>& _involvedTypes, SortedList<Expr*>& _involvedExprs)
			:pa(_pa)
			, includeParentDeclArguments(_includeParentDeclArguments)
			, insideVariant(_insideVariant)
			, freeTypeSymbols(_freeTypeSymbols)
			, involvedTypes(_involvedTypes)
			, involvedExprs(_involvedExprs)
		{
		}

		bool Execute(Type* type)
		{
			if (!type) return false;
			bool oldInvolved = involved;
			involved = false;
			type->Accept(this);
			bool result = involved;
			involved = oldInvolved;
			return result;
		}

		bool Execute(Ptr<Type>& type)
		{
			return Execute(type.Obj());
		}

		bool Execute(Ptr<Expr>& expr)
		{
			if (auto idExpr = expr.Cast<IdExpr>())
			{
				if (idExpr->resolving && idExpr->resolving->resolvedSymbols.Count() == 1)
				{
					auto symbol = idExpr->resolving->resolvedSymbols[0];
					if (symbol->kind == symbol_component::SymbolKind::GenericValueArgument)
					{
						if (freeTypeSymbols.Contains(symbol))
						{
							involvedExprs.Add(expr.Obj());
							return true;
						}
					}
				}
			}
			return false;
		}

		void Visit(PrimitiveType* self)override
		{
		}

		void Visit(ReferenceType* self)override
		{
			bool result = Execute(self->type);
			if ((involved = result)) involvedTypes.Add(self);
		}

		void Visit(ArrayType* self)override
		{
			bool result = false;
			result = Execute(self->type) || result;
			result = Execute(self->expr) || result;
			if ((involved = result)) involvedTypes.Add(self);
		}

		void Visit(CallingConventionType* self)override
		{
			bool result = Execute(self->type);
			if ((involved = result)) involvedTypes.Add(self);
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
					if (insideVariant)
					{
						// does not support nested variadic arguments
						throw TypeCheckerException();
					}
				}
				result = Execute(self->parameters[i].item->type) || result;
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
			bool result = Execute(self->type);
			if ((involved = result)) involvedTypes.Add(self);
		}

		void Visit(RootType* self)override
		{
			// do not perform type inferencing against ::
		}

		void Visit(IdType* self)override
		{
			if (self->resolving && self->resolving->resolvedSymbols.Count() == 1)
			{
				auto patternSymbol = self->resolving->resolvedSymbols[0];
				if (freeTypeSymbols.Contains(patternSymbol))
				{
					involved = true;
					involvedTypes.Add(self);
				}
				else if(includeParentDeclArguments)
				{
					switch (patternSymbol->kind)
					{
					case symbol_component::SymbolKind::GenericTypeArgument:
					case symbol_component::SymbolKind::GenericValueArgument:
						{
							auto pattern = GetTemplateArgumentKey(patternSymbol, pa.tsys.Obj());
							ITsys* patternValue = nullptr;
							if (pa.TryGetReplacedGenericArg(pattern, patternValue))
							{
								involved = true;
								involvedTypes.Add(self);
							}
						}
						break;
					}
				}
			}
		}

		void Visit(ChildType* self)override
		{
			// do not perform type inferencing against Here::something
		}

		void Visit(GenericType* self)override
		{
			if (self->type->resolving && self->type->resolving->resolvedSymbols.Count() == 1)
			{
				// only perform type inferencing when the generic symbol is a class or a template template argument
				auto symbol = self->type->resolving->resolvedSymbols[0];
				switch (symbol->kind)
				{
				case CLASS_SYMBOL_KIND:
					{
						auto classDecl = symbol->GetAnyForwardDecl<ForwardClassDeclaration>();
						if (!classDecl || !classDecl->templateSpec)
						{
							return;
						}
					}
					break;
				case symbol_component::SymbolKind::GenericTypeArgument:
					{
						auto tsys = EvaluateGenericArgumentSymbol(symbol);
						if (tsys->GetType() != TsysType::GenericFunction || !tsys->GetGenericFunction().spec)
						{
							return;
						}
					}
					break;
				default:
					return;
				}

				bool result = false;
				result = Execute(self->type.Obj()) || result;
				for (vint i = 0; i < self->arguments.Count(); i++)
				{
					if (self->arguments[i].isVariadic)
					{
						if (insideVariant)
						{
							// does not support nested variadic arguments
							throw TypeCheckerException();
						}
					}
					result = Execute(self->arguments[i].item.type) || result;
					result = Execute(self->arguments[i].item.expr) || result;
				}
				if ((involved = result)) involvedTypes.Add(self);
			}
		}
	};

	void CollectFreeTypes(
		const ParsingArguments& pa,
		bool includeParentDeclArguments,
		Ptr<Type> type,
		Ptr<Expr> expr,
		bool insideVariant,
		const SortedList<Symbol*>& freeTypeSymbols,
		SortedList<Type*>& involvedTypes,
		SortedList<Expr*>& involvedExprs
	)
	{
		CollectFreeTypesVisitor visitor(pa, includeParentDeclArguments, insideVariant, freeTypeSymbols, involvedTypes, involvedExprs);
		if (type) type->Accept(&visitor);
		visitor.Execute(expr);
	}
}