#include "Ast_Resolving_PSO.h"

namespace partial_specification_ordering
{

	class MatchPSArgumentVisitor : public Object, public virtual ITypeVisitor
	{
	public:
		const ParsingArguments&							pa;
		Dictionary<Symbol*, Ptr<MatchPSResult>>&		matchingResult;
		Dictionary<Symbol*, Ptr<MatchPSResult>>&		matchingResultVta;
		bool											insideVariant;
		const SortedList<Symbol*>&						freeTypeSymbols;
		SortedList<Type*>&								involvedTypes;
		SortedList<Expr*>&								involvedExprs;
		Ptr<Type>										childType;

		MatchPSArgumentVisitor(
			const ParsingArguments& _pa,
			Dictionary<Symbol*, Ptr<MatchPSResult>>& _matchingResult,
			Dictionary<Symbol*, Ptr<MatchPSResult>>& _matchingResultVta,
			bool _insideVariant,
			const SortedList<Symbol*>& _freeTypeSymbols,
			SortedList<Type*>& _involvedTypes,
			SortedList<Expr*>& _involvedExprs
		)
			:pa(_pa)
			, matchingResult(_matchingResult)
			, matchingResultVta(_matchingResultVta)
			, insideVariant(_insideVariant)
			, freeTypeSymbols(_freeTypeSymbols)
			, involvedTypes(_involvedTypes)
			, involvedExprs(_involvedExprs)
		{
		}

		void Execute(const Ptr<Type>& _ancestorType, const Ptr<Type>& _childType)
		{
			childType = _childType;
			_ancestorType->Accept(this);
		}

		void Execute(const Ptr<Expr>& expr)
		{
			if (!involvedExprs.Contains(expr.Obj())) return;
			// TODO: dealing with value
			throw 0;
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
				MatchPSAncestorArguments(pa, matchingResult, matchingResultVta, self, c.Obj(), insideVariant, freeTypeSymbols);
			}
			throw MatchPSFailureException();
		}

		void Visit(MemberType* self)override
		{
			if (auto c = childType.Cast<MemberType>())
			{
				Execute(self->classType, c->classType);
				Execute(self->type, c->type);
			}
			throw MatchPSFailureException();
		}

		void Visit(DeclType* self)override
		{
			if (auto c = childType.Cast<DeclType>())
			{
				return;
			}
			throw MatchPSFailureException();
		}

		void Visit(DecorateType* self)override
		{
			if (auto c = childType.Cast<DecorateType>())
			{
				if (self->isConst == c->isConst && self->isVolatile == c->isVolatile)
				{
					Execute(self->type, c->type);
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
				auto& output = patternSymbol->ellipsis ? matchingResult : matchingResultVta;

				switch (patternSymbol->kind)
				{
				case symbol_component::SymbolKind::GenericTypeArgument:
					{
						// TODO: pass information from outside
						auto result = MakePtr<MatchPSResult>();
						result->kind = MatchPSKind::Single;
						result->source.Add({ childType,false });

						vint index = output.Keys().IndexOf(patternSymbol);
						if (index == -1)
						{
							output.Add(patternSymbol, result);
							return;
						}
						else
						{
							auto assigned = output.Values()[index];
							if (result->kind != assigned->kind) goto FAIL;
							if (result->start != assigned->start) goto FAIL;
							if (result->stop != assigned->stop) goto FAIL;
							if (result->source.Count() != assigned->source.Count()) goto FAIL;
							{
								Dictionary<WString, WString> equivalentNames;
								for (vint i = 0; i < result->source.Count(); i++)
								{
									auto rt = result->source[i];
									auto at = assigned->source[i];
									if(rt.isVariadic!=at.isVariadic) goto FAIL;
									if (rt.item && at.item)
									{
										if (!IsSameResolvedType(rt.item, at.item, equivalentNames)) goto FAIL;
									}
									else if (rt.item || at.item)
									{
										goto FAIL;
									}
								}
							}
							return;
						}
					}
					break;
				case symbol_component::SymbolKind::GenericValueArgument:
					{
						// TODO: dealing with value
						throw 0;
					}
					break;
				}
			}
			else if (auto c = childType.Cast<Category_Id_Child_Type>())
			{
				if (self->resolving && c->resolving)
				{
					if (!From(self->resolving->resolvedSymbols).Intersect(c->resolving->resolvedSymbols).IsEmpty())
					{
						return;
					}
				}
			}
		FAIL:
			throw MatchPSFailureException();
		}

		void Visit(ChildType* self)override
		{
			if (auto c = childType.Cast<Category_Id_Child_Type>())
			{
				if (self->resolving && c->resolving)
				{
					if (!From(self->resolving->resolvedSymbols).Intersect(c->resolving->resolvedSymbols).IsEmpty())
					{
						return;
					}
				}
			}
			throw MatchPSFailureException();
		}

		void Visit(GenericType* self)override
		{
			if (auto c = childType.Cast<GenericType>())
			{
				Execute(self->type, c->type);
				MatchPSAncestorArguments(pa, matchingResult, matchingResultVta, self, c.Obj(), insideVariant, freeTypeSymbols);
			}
			throw MatchPSFailureException();
		}
	};

	/***********************************************************************
	MatchPSArgument: Match ancestor types with child types, nullptr means value
	***********************************************************************/

	void MatchPSArgument(
		const ParsingArguments& pa,
		Dictionary<Symbol*, Ptr<MatchPSResult>>& matchingResult,
		Dictionary<Symbol*, Ptr<MatchPSResult>>& matchingResultVta,
		Ptr<Type> ancestor,
		Ptr<Type> child,
		bool insideVariant,
		const SortedList<Symbol*>& freeTypeSymbols,
		SortedList<Type*>& involvedTypes,
		SortedList<Expr*>& involvedExprs
	)
	{
		MatchPSArgumentVisitor visitor(pa, matchingResult, matchingResultVta, insideVariant, freeTypeSymbols, involvedTypes, involvedExprs);
		visitor.Execute(ancestor, child);
	}
}