#include "Ast_Resolving_PSO.h"

namespace partial_specification_ordering
{

	class MatchPSArgumentVisitor : public Object, public virtual ITypeVisitor
	{
	public:
		const ParsingArguments&				pa;
		bool								insideVariant;
		const SortedList<Symbol*>&			freeTypeSymbols;
		SortedList<Type*>&					involvedTypes;
		SortedList<Expr*>&					involvedExprs;
		Ptr<Type>							childType;

		MatchPSArgumentVisitor(const ParsingArguments& _pa, bool _insideVariant, const SortedList<Symbol*>& _freeTypeSymbols, SortedList<Type*>& _involvedTypes, SortedList<Expr*>& _involvedExprs, Ptr<Type> _childType)
			:pa(_pa)
			, insideVariant(_insideVariant)
			, freeTypeSymbols(_freeTypeSymbols)
			, involvedTypes(_involvedTypes)
			, involvedExprs(_involvedExprs)
			, childType(_childType)
		{
		}

		void Visit(PrimitiveType* self)override
		{
			throw 0;
		}

		void Visit(ReferenceType* self)override
		{
			throw 0;
		}

		void Visit(ArrayType* self)override
		{
			throw 0;
		}

		void Visit(CallingConventionType* self)override
		{
			throw 0;
		}

		void Visit(FunctionType* self)override
		{
			throw 0;
		}

		void Visit(MemberType* self)override
		{
			throw 0;
		}

		void Visit(DeclType* self)override
		{
			// do not perform type inferencing against decltype(expr)
		}

		void Visit(DecorateType* self)override
		{
			throw 0;
		}

		void Visit(RootType* self)override
		{
			// do not perform type inferencing against ::
		}

		void Visit(IdType* self)override
		{
			throw 0;
		}

		void Visit(ChildType* self)override
		{
			// do not perform type inferencing against Here::something
		}

		void Visit(GenericType* self)override
		{
			throw 0;
		}
	};

	/***********************************************************************
	MatchPSArgument: Match ancestor types with child types, nullptr means value
	***********************************************************************/

	void MatchPSArgument(
		const ParsingArguments& pa,
		const Dictionary<Symbol*, Ptr<MatchPSResult>>& matchingResult,
		const Dictionary<Symbol*, Ptr<MatchPSResult>>& matchingResultVta,
		Ptr<Type> ancestor,
		Ptr<Type> child,
		bool insideVariant,
		const SortedList<Symbol*>& freeTypeSymbols,
		SortedList<Type*>& involvedTypes,
		SortedList<Expr*>& involvedExprs
	)
	{
		MatchPSArgumentVisitor visitor(pa, insideVariant, freeTypeSymbols, involvedTypes, involvedExprs, child);
		ancestor->Accept(&visitor);
	}
}