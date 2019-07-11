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
		vint					selectedIndex = -1;

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

	namespace impl
	{
		template<vint Index, typename T, typename ...Ts>
		struct SelectImpl
		{
			static inline auto& Do(T& t, Ts& ...ts)
			{
				return SelectImpl<Index - 1, Ts...>::Do(ts...);
			}
		};

		template<typename T, typename ...Ts>
		struct SelectImpl<0, T, Ts...>
		{
			static inline T& Do(T& t, Ts& ...ts)
			{
				return t;
			}
		};

		template<vint Index, typename ...Ts>
		auto& Select(Ts& ...ts)
		{
			return SelectImpl<Index, Ts...>::Do(ts...);
		}

		template<typename TInput>
		ExprTsysItem SelectInput(VtaInput<TInput> input, vint unboundedVtaIndex)
		{
			if (input.isVta)
			{
				auto tsys = GetExprTsysItem(input.items[input.selectedIndex]).tsys;
				return { tsys->GetInit().headers[unboundedVtaIndex],tsys->GetParam(unboundedVtaIndex) };
			}
			else
			{
				return GetExprTsysItem(input.items[input.selectedIndex]);
			}
		}

		template<typename TResult, typename TProcess, typename ...TInputs>
		struct ExpandPotentialVtaStep
		{
			template<vint Index>
			struct Step
			{
				static void Do(const ParsingArguments& pa, List<TResult>& result, vint unboundedVtaCount, TProcess&& process, VtaInput<TInputs>& ...inputs)
				{
					auto& input = Select<Index>(inputs...);
					for (vint i = 0; i < input.items.Count(); i++)
					{
						auto item = GetExprTsysItem(input.items[i]);
						if (input.isVta)
						{
							if (item.tsys->GetType() == TsysType::Init)
							{
								vint count = item.tsys->GetParamCount();
								if (unboundedVtaCount == -1)
								{
									unboundedVtaCount = count;
								}
								else if (unboundedVtaCount != count)
								{
									throw NotConvertableException();
								}
							}
							else
							{
								AddExprTsysItemToResult(result, GetExprTsysItem(pa.tsys->Any()));
								continue;
							}
						}

						input.selectedIndex = i;
						Step<Index + 1>::Do(pa, result, unboundedVtaCount, ForwardValue<TProcess&&>(process), inputs...);
					}
				}
			};

			template<>
			struct Step<sizeof...(TInputs)>
			{
				static void Do(const ParsingArguments& pa, List<TResult>& result, vint unboundedVtaCount, TProcess&& process, VtaInput<TInputs>& ...inputs)
				{
					if (unboundedVtaCount == -1)
					{
						AddExprTsysItemToResult(result, GetExprTsysItem(process(SelectInput(inputs, -1)...)));
					}
					else
					{
						Array<ExprTsysItem> params(unboundedVtaCount);
						for (vint i = 0; i < unboundedVtaCount; i++)
						{
							params[i] = GetExprTsysItem(process(SelectInput(inputs, i)...));
						}
						AddExprTsysItemToResult(result, GetExprTsysItem(pa.tsys->InitOf(params)));
					}
				}
			};
		};
	}

	template<typename TResult, typename TProcess, typename ...TInputs>
	bool ExpandPotentialVta(const ParsingArguments& pa, List<TResult>& result, TProcess&& process, VtaInput<TInputs> ...inputs)
	{
		using Step = typename impl::ExpandPotentialVtaStep<TResult, TProcess, TInputs...>::template Step<0>;
		Step::Do(pa, result, -1, ForwardValue<TProcess&&>(process), inputs...);
		return (inputs.isVta || ...);
	}
}