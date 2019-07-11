#include "Ast_Resolving.h"

namespace symbol_totsys_impl
{
	ITsys*					ProcessPrimitiveType(const ParsingArguments& pa, PrimitiveType* self);
	ITsys*					ProcessReferenceType(const ParsingArguments& pa, ReferenceType* self, ExprTsysItem arg);
	ITsys*					ProcessArrayType(const ParsingArguments& pa, ArrayType* self, ExprTsysItem arg);
	ITsys*					ProcessDecorateType(const ParsingArguments& pa, DecorateType* self, ExprTsysItem arg);

	inline ExprTsysItem GetExprTsysItem(ITsys* arg)
	{
		return { nullptr,ExprTsysType::PRValue,arg };
	}

	inline ExprTsysItem GetExprTsysItem(ExprTsysItem arg)
	{
		return arg;
	}

	inline void AddResult(TypeTsysList& result, ExprTsysItem arg)
	{
		if (!result.Contains(arg.tsys))
		{
			result.Add(arg.tsys);
		}
	}

	template<typename TResult, typename T1, typename TProcess>
	bool ExpandPotentialVta(const ParsingArguments& pa, List<TResult>& result, List<T1>& items1, bool isVta1, TProcess&& process)
	{
		for (vint i = 0; i < items1.Count(); i++)
		{
			auto item1 = GetExprTsysItem(items1[i]);
			if (isVta1)
			{
				if (item1.tsys->GetType() == TsysType::Init)
				{
					const auto& init = item1.tsys->GetInit();
					Array<ExprTsysItem> params(item1.tsys->GetParamCount());
					for (vint j = 0; j < params.Count(); j++)
					{
						params[j] = GetExprTsysItem(process(ExprTsysItem(init.headers[j], item1.tsys->GetParam(j))));
					}
					AddResult(result, GetExprTsysItem(pa.tsys->InitOf(params)));
				}
				else
				{
					AddResult(result, GetExprTsysItem(pa.tsys->Any()));
				}
			}
			else
			{
				AddResult(result, GetExprTsysItem(process(item1)));
			}
		}
		return isVta1;
	}
}