#include "Ast_Resolving.h"

namespace symbol_totsys_impl
{
	ITsys*					ProcessPrimitiveType(const ParsingArguments& pa, PrimitiveType* self);
	ITsys*					ProcessReferenceType(const ParsingArguments& pa, ReferenceType* self, ExprTsysItem arg);
	ITsys*					ProcessArrayType(const ParsingArguments& pa, ArrayType* self, ExprTsysItem arg);
	ITsys*					ProcessMemberType(const ParsingArguments& pa, MemberType* self, ExprTsysItem argType, ExprTsysItem argClass);
	ITsys*					ProcessDecorateType(const ParsingArguments& pa, DecorateType* self, ExprTsysItem arg);

	inline ExprTsysItem GetExprTsysItem(ITsys* arg)
	{
		return { nullptr,ExprTsysType::PRValue,arg };
	}

	inline ExprTsysItem GetExprTsysItem(ExprTsysItem arg)
	{
		return arg;
	}

	inline void AddExprTsysItemToResult(TypeTsysList& result, ExprTsysItem arg)
	{
		if (!result.Contains(arg.tsys))
		{
			result.Add(arg.tsys);
		}
	}

	template<typename T>
	struct VtaInput
	{
		List<T>&				items;
		bool					isVta;

		VtaInput(List<T>& _items, bool _isVta)
			:items(_items)
			, isVta(_isVta)
		{
		}
	};

	template<typename T>
	VtaInput<T> Input(List<T>& _items, bool _isVta)
	{
		return { _items,_isVta };
	}

	template<typename TResult, typename T1, typename TProcess>
	bool ExpandPotentialVta(const ParsingArguments& pa, List<TResult>& result, VtaInput<T1> input1, TProcess&& process)
	{
		for (vint i = 0; i < input1.items.Count(); i++)
		{
			auto item1 = GetExprTsysItem(input1.items[i]);
			if (input1.isVta)
			{
				if (item1.tsys->GetType() == TsysType::Init)
				{
					const auto& init = item1.tsys->GetInit();
					Array<ExprTsysItem> params(item1.tsys->GetParamCount());
					for (vint j = 0; j < params.Count(); j++)
					{
						params[j] = GetExprTsysItem(process(ExprTsysItem(init.headers[j], item1.tsys->GetParam(j))));
					}
					AddExprTsysItemToResult(result, GetExprTsysItem(pa.tsys->InitOf(params)));
				}
				else
				{
					AddExprTsysItemToResult(result, GetExprTsysItem(pa.tsys->Any()));
				}
			}
			else
			{
				AddExprTsysItemToResult(result, GetExprTsysItem(process(item1)));
			}
		}
		return input1.isVta;
	}
}