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

	//template<typename TResult, typename T1, typename TProcess>
	//bool ExpandPotentialVta(const ParsingArguments& pa, List<TResult>& result, TProcess&& process, VtaInput<T1> input1)
	//{
	//	for (vint i = 0; i < input1.items.Count(); i++)
	//	{
	//		auto item1 = GetExprTsysItem(input1.items[i]);
	//		if (input1.isVta)
	//		{
	//			if (item1.tsys->GetType() == TsysType::Init)
	//			{
	//				const auto& init = item1.tsys->GetInit();
	//				Array<ExprTsysItem> params(item1.tsys->GetParamCount());
	//				for (vint j = 0; j < params.Count(); j++)
	//				{
	//					params[j] = GetExprTsysItem(process(ExprTsysItem(init.headers[j], item1.tsys->GetParam(j))));
	//				}
	//				AddExprTsysItemToResult(result, GetExprTsysItem(pa.tsys->InitOf(params)));
	//			}
	//			else
	//			{
	//				AddExprTsysItemToResult(result, GetExprTsysItem(pa.tsys->Any()));
	//			}
	//		}
	//		else
	//		{
	//			AddExprTsysItemToResult(result, GetExprTsysItem(process(item1)));
	//		}
	//	}
	//	return input1.isVta;
	//}

	namespace impl
	{
		template<typename T, typename ...Ts>
		struct SelectImpl
		{
			template<vint Index>
			struct Impl
			{
				static inline auto Do(T&& t, Ts&& ...ts)
				{
					using TImpl = typename SelectImpl<Ts...>::template Impl<Index - 1>;
					return TImpl::Do(ForwardValue<Ts&&>(ts)...);
				}
			};

			template<>
			struct Impl<0>
			{
				static inline T Do(T&& t, Ts&& ...ts)
				{
					return t;
				}
			};
		};

		template<vint Index, typename T, typename ...Ts>
		auto Select(T&& t, Ts&& ...ts)
		{
			using TImpl = typename SelectImpl<T, Ts...>::template Impl<Index>;
			return TImpl::Do(ForwardValue<T&&>(t), ForwardValue<Ts&&>(ts)...);
		}

		template<typename TInput>
		ExprTsysItem SelectInput(VtaInput<TInput> input, vint inputIndex, vint unboundedVtaIndex)
		{
			if (input.isVta)
			{
				auto tsys = GetExprTsysItem(input.items[inputIndex]).tsys;
				return { tsys->GetInit().headers[unboundedVtaIndex],tsys->GetParam(unboundedVtaIndex) };
			}
			else
			{
				return GetExprTsysItem(input.items[inputIndex]);
			}
		}

		template<typename TResult, typename TProcess, typename ...TInputs>
		struct ExpandPotentialVtaFinal;

		template<typename TResult, typename TProcess, typename TInput1>
		struct ExpandPotentialVtaFinal<TResult, TProcess, TInput1>
		{
			static ExprTsysItem Do(List<TResult>& result, vint(&inputIndex)[1], vint unboundedVtaIndex, TProcess&& process, VtaInput<TInput1> input1)
			{
				return GetExprTsysItem(process(
					SelectInput(input1, inputIndex[0], unboundedVtaIndex)
				));
			}
		};

		template<typename TResult, typename TProcess, typename TInput1, typename TInput2>
		struct ExpandPotentialVtaFinal<TResult, TProcess, TInput1, TInput2>
		{
			static ExprTsysItem Do(List<TResult>& result, vint(&inputIndex)[2], vint unboundedVtaIndex, TProcess&& process, VtaInput<TInput1> input1, VtaInput<TInput2> input2)
			{
				return GetExprTsysItem(process(
					SelectInput(input1, inputIndex[0], unboundedVtaIndex),
					SelectInput(input2, inputIndex[1], unboundedVtaIndex)
				));
			}
		};

		template<typename TResult, typename TProcess, typename ...TInputs>
		struct ExpandPotentialVtaStep
		{
			template<vint Index>
			struct Step
			{
				static void Do(const ParsingArguments& pa, List<TResult>& result, vint(&inputIndex)[sizeof...(TInputs)], vint unboundedVtaCount, TProcess&& process, VtaInput<TInputs> ...inputs)
				{
					auto input = Select<Index>(inputs...);
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

						inputIndex[Index] = i;
						Step<Index + 1>::Do(pa, result, inputIndex, unboundedVtaCount, ForwardValue<TProcess&&>(process), inputs...);
					}
				}
			};

			template<>
			struct Step<sizeof...(TInputs)>
			{
				static void Do(const ParsingArguments& pa, List<TResult>& result, vint(&inputIndex)[sizeof...(TInputs)], vint unboundedVtaCount, TProcess&& process, VtaInput<TInputs> ...inputs)
				{
					if (unboundedVtaCount == -1)
					{
						AddExprTsysItemToResult(result, ExpandPotentialVtaFinal<TResult, TProcess, TInputs...>::Do(result, inputIndex, -1, ForwardValue<TProcess&&>(process), inputs...));
					}
					else
					{
						Array<ExprTsysItem> params(unboundedVtaCount);
						for (vint i = 0; i < unboundedVtaCount; i++)
						{
							params[i] = ExpandPotentialVtaFinal<TResult, TProcess, TInputs...>::Do(result, inputIndex, i, ForwardValue<TProcess&&>(process), inputs...);
						}
						AddExprTsysItemToResult(result, GetExprTsysItem(pa.tsys->InitOf(params)));
					}
				}
			};

			static void Execute(const ParsingArguments& pa, List<TResult>& result, vint(&inputIndex)[sizeof...(TInputs)], vint unboundedVtaCount, TProcess&& process, VtaInput<TInputs> ...inputs)
			{
				Step<0>::Do(pa, result, inputIndex, unboundedVtaCount, ForwardValue<TProcess&&>(process), inputs...);
			}
		};
	}

	template<typename TResult, typename TProcess, typename ...TInputs>
	bool ExpandPotentialVta(const ParsingArguments& pa, List<TResult>& result, TProcess&& process, VtaInput<TInputs> ...inputs)
	{
		vint inputIndex[sizeof...(TInputs)];
		impl::ExpandPotentialVtaStep<TResult, TProcess, TInputs...>::Execute(pa, result, inputIndex, -1, ForwardValue<TProcess&&>(process), inputs...);
		return (inputs.isVta || ...);
	}
}