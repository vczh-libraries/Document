#include "PSO.h"
#include "Ast_Expr.h"

namespace partial_specification_ordering
{

	class MatchPSArgumentVisitor : public Object, public virtual ITypeVisitor
	{
	public:
		const ParsingArguments&							pa;
		bool&											skipped;
		Dictionary<Symbol*, Ptr<MatchPSResult>>&		matchingResult;
		Dictionary<Symbol*, Ptr<MatchPSResult>>&		matchingResultVta;
		bool											insideVariant;
		const SortedList<Symbol*>&						freeAncestorSymbols;
		const SortedList<Symbol*>&						freeChildSymbols;
		SortedList<Type*>&								involvedTypes;
		SortedList<Expr*>&								involvedExprs;

		Ptr<Type>										childType;
		bool											forParameter = false;

		MatchPSArgumentVisitor(
			const ParsingArguments& _pa,
			bool& _skipped,
			Dictionary<Symbol*, Ptr<MatchPSResult>>& _matchingResult,
			Dictionary<Symbol*, Ptr<MatchPSResult>>& _matchingResultVta,
			bool _insideVariant,
			const SortedList<Symbol*>& _freeAncestorSymbols,
			const SortedList<Symbol*>& _freeChildSymbols,
			SortedList<Type*>& _involvedTypes,
			SortedList<Expr*>& _involvedExprs
		)
			:pa(_pa)
			, skipped(_skipped)
			, matchingResult(_matchingResult)
			, matchingResultVta(_matchingResultVta)
			, insideVariant(_insideVariant)
			, freeAncestorSymbols(_freeAncestorSymbols)
			, freeChildSymbols(_freeChildSymbols)
			, involvedTypes(_involvedTypes)
			, involvedExprs(_involvedExprs)
		{
		}

		bool AssignToArgument(Symbol* patternSymbol, const Ptr<MatchPSResult>& result, bool forParameter)
		{
			auto& output = patternSymbol->ellipsis ? matchingResultVta : matchingResult;
			vint index = output.Keys().IndexOf(patternSymbol);
			if (index == -1)
			{
				output.Add(patternSymbol, result);
				return true;
			}
			else
			{
				auto assigned = output.Values()[index];
				if (MatchPSResult::Compare(result, assigned, forParameter))
				{
					return true;
				}
			}
			return false;
		}

		bool AssignToValueArgument(Symbol* patternSymbol)
		{
			auto& output = patternSymbol->ellipsis ? matchingResultVta : matchingResult;
			vint index = output.Keys().IndexOf(patternSymbol);
			if (index == -1)
			{
				auto result = MakePtr<MatchPSResult>();
				result->source.Add(nullptr);
				output.Add(patternSymbol, result);
				return true;
			}
			else
			{
				auto assigned = output.Values()[index];
				if (assigned->source.Count() == 1 && !assigned->source[0])
				{
					return true;
				}
			}
			return false;
		}

		void Execute(const Ptr<Type>& _ancestorType, const Ptr<Type>& _childType, bool _forParameter = false)
		{
			childType = _childType;
			forParameter = _forParameter;
			_ancestorType->Accept(this);
		}

		void Execute(const Ptr<Expr>& expr)
		{
			if (!involvedExprs.Contains(expr.Obj())) return;
			if (auto idExpr = expr.Cast<IdExpr>())
			{
				auto patternSymbol = idExpr->resolving->resolvedSymbols[0];
				if (!AssignToValueArgument(patternSymbol))
				{
					throw MatchPSFailureException();
				}
			}
		}

		void Visit(PrimitiveType* self)override
		{
			if (auto c = childType.Cast<PrimitiveType>())
			{
				if (self->prefix == c->prefix && self->primitive == c->primitive)
				{
					return;
				}
			}
			throw MatchPSFailureException();
		}

		void Visit(ReferenceType* self)override
		{
			if (auto c = childType.Cast<ReferenceType>())
			{
				if (self->reference == c->reference)
				{
					Execute(self->type, c->type);
					return;
				}
			}
			throw MatchPSFailureException();
		}

		void Visit(ArrayType* self)override
		{
			if (auto c = childType.Cast<ArrayType>())
			{
				Execute(self->type, c->type);
				Execute(self->expr);
				return;
			}
			throw MatchPSFailureException();
		}

		void Visit(CallingConventionType* self)override
		{
			if (auto c = childType.Cast<CallingConventionType>())
			{
				if (self->callingConvention == c->callingConvention)
				{
					Execute(self->type, c->type);
					return;
				}
			}
			throw MatchPSFailureException();
		}

		void Visit(FunctionType* self)override
		{
			if (auto c = childType.Cast<FunctionType>())
			{
				auto sr = self->decoratorReturnType ? self->decoratorReturnType : self->returnType;
				auto cr = c->decoratorReturnType ? c->decoratorReturnType : c->returnType;
				Execute(sr, cr);
				MatchPSAncestorArguments(pa, skipped, matchingResult, matchingResultVta, self, c.Obj(), insideVariant, freeAncestorSymbols, freeChildSymbols);
				return;
			}
			throw MatchPSFailureException();
		}

		void Visit(MemberType* self)override
		{
			if (auto c = childType.Cast<MemberType>())
			{
				Execute(self->classType, c->classType);
				Execute(self->type, c->type);
				return;
			}
			throw MatchPSFailureException();
		}

		void Visit(DeclType* self)override
		{
			if (auto c = childType.Cast<DeclType>())
			{
				// TODO: [cpp.md] two expression should match
				return;
			}
		}

		void Visit(DecorateType* self)override
		{
			if (auto c = childType.Cast<DecorateType>())
			{
				if (self->isConst == c->isConst && self->isVolatile == c->isVolatile)
				{
					Execute(self->type, c->type);
					return;
				}
			}
			throw MatchPSFailureException();
		}

		void Visit(RootType* self)override
		{
			if (auto c = childType.Cast<RootType>())
			{
				return;
			}
			throw MatchPSFailureException();
		}

		void Visit(IdType* self)override
		{
			if (involvedTypes.Contains(self))
			{
				auto patternSymbol = self->resolving->resolvedSymbols[0];

				switch (patternSymbol->kind)
				{
				case symbol_component::SymbolKind::GenericTypeArgument:
					{
						auto result = MakePtr<MatchPSResult>();
						result->source.Add(childType);
						if (AssignToArgument(patternSymbol, result, forParameter))
						{
							return;
						}
					}
					break;
				case symbol_component::SymbolKind::GenericValueArgument:
					{
						if (AssignToValueArgument(patternSymbol))
						{
							return;
						}
					}
					break;
				}
			}
			else if (auto c = childType.Cast<Category_Id_Child_Type>())
			{
				if (self->resolving && c->resolving)
				{
					if (Resolving::ContainsSameSymbol(self->resolving, c->resolving))
					{
						return;
					}
				}
			}
			throw MatchPSFailureException();
		}

		void Visit(ChildType* self)override
		{
			if (auto c = childType.Cast<Category_Id_Child_Type>())
			{
				if (self->resolving && c->resolving)
				{
					if (Resolving::ContainsSameSymbol(self->resolving, c->resolving))
					{
						return;
					}
				}
				throw MatchPSFailureException();
			}
			// could be typename A::B
		}

		void Visit(GenericType* self)override
		{
			if (auto c = childType.Cast<GenericType>())
			{
				Execute(self->type, c->type);
				MatchPSAncestorArguments(pa, skipped, matchingResult, matchingResultVta, self, c.Obj(), insideVariant, freeAncestorSymbols, freeChildSymbols);
			}
			// could be typename A::template B<C>
		}
	};

	/***********************************************************************
	MatchPSArgument: Match ancestor types with child types, nullptr means value
	***********************************************************************/

	void MatchPSArgument(
		const ParsingArguments& pa,
		bool& skipped,
		Dictionary<Symbol*, Ptr<MatchPSResult>>& matchingResult,
		Dictionary<Symbol*, Ptr<MatchPSResult>>& matchingResultVta,
		GenericArgument ancestor,
		GenericArgument child,
		bool insideVariant,
		const SortedList<Symbol*>& freeAncestorSymbols,
		const SortedList<Symbol*>& freeChildSymbols,
		SortedList<Type*>& involvedTypes,
		SortedList<Expr*>& involvedExprs,
		bool forParameter
	)
	{
		MatchPSArgumentVisitor visitor(pa, skipped, matchingResult, matchingResultVta, insideVariant, freeAncestorSymbols, freeChildSymbols, involvedTypes, involvedExprs);
		if (ancestor.type && child.type)
		{
			visitor.Execute(ancestor.type, child.type, forParameter);
		}
		else if (ancestor.expr && child.expr)
		{
			visitor.Execute(ancestor.expr);
		}
		else
		{
			throw MatchPSFailureException();
		}
	}
}