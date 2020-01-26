#include "Ast_Resolving_IFT.h"

namespace symbol_type_resolving
{
	/***********************************************************************
	CollectFreeTypes:	Collect and check all involved free types from a type
						Stops at nested variadic types
	***********************************************************************/

	class CollectFreeTypesVisitor : public Object, public virtual ITypeVisitor
	{
	public:
		bool								involved = false;
		bool								insideVariant;
		const SortedList<Symbol*>&			freeTypeSymbols;
		SortedList<Type*>&					involvedTypes;

		CollectFreeTypesVisitor(bool _insideVariant, const SortedList<Symbol*>& _freeTypeSymbols, SortedList<Type*>& _involvedTypes)
			:insideVariant(_insideVariant)
			, freeTypeSymbols(_freeTypeSymbols)
			, involvedTypes(_involvedTypes)
		{
		}

		bool Execute(Type* type)
		{
			if (!type) return false;
			type->Accept(this);
			bool result = involved;
			involved = false;
			return result;
		}

		bool Execute(Ptr<Type>& type)
		{
			return Execute(type.Obj());
		}

		void Visit(PrimitiveType* self)override
		{
		}

		void Visit(ReferenceType* self)override
		{
			if (Execute(self->type)) involvedTypes.Add(self);
		}

		void Visit(ArrayType* self)override
		{
			if (Execute(self->type)) involvedTypes.Add(self);
		}

		void Visit(CallingConventionType* self)override
		{
			if (Execute(self->type)) involvedTypes.Add(self);
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
				else
				result = Execute(self->parameters[i].item->type);
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
			if (Execute(self->type)) involvedTypes.Add(self);
		}

		void Visit(RootType* self)override
		{
		}

		void Visit(IdType* self)override
		{
			if (self->resolving && self->resolving->resolvedSymbols.Count() == 1)
			{
				auto symbol = self->resolving->resolvedSymbols[0];
				if (freeTypeSymbols.Contains(symbol))
				{
					involved = true;
					involvedTypes.Add(self);
				}
			}
		}

		void Visit(ChildType* self)override
		{
			if (Execute(self->classType.Obj())) involvedTypes.Add(self);
		}

		void Visit(GenericType* self)override
		{
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
			}
			if ((involved = result)) involvedTypes.Add(self);
		}
	};

	void CollectFreeTypes(Ptr<Type> type, bool insideVariant, const SortedList<Symbol*>& freeTypeSymbols, SortedList<Type*>& involvedTypes)
	{
		if (type)
		{
			CollectFreeTypesVisitor visitor(insideVariant, freeTypeSymbols, involvedTypes);
			type->Accept(&visitor);
		}
	}
}