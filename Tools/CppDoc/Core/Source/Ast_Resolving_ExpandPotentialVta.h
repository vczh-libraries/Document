#include "Ast_Resolving.h"
#include "Ast_Expr.h"

namespace symbol_totsys_impl
{
	//////////////////////////////////////////////////////////////////////////////////////
	// Types
	//////////////////////////////////////////////////////////////////////////////////////

	// Ast_Evaluate_ToTsys_IdImpl.cpp
	void					CreateIdReferenceType(const ParsingArguments& pa, GenericArgContext* gaContext, Ptr<Resolving> resolving, bool allowAny, bool allowVariadic, TypeTsysList& result, bool& isVta);
	void					ProcessChildType(const ParsingArguments& pa, GenericArgContext* gaContext, ChildType* self, ExprTsysItem argClass, ExprTsysList& result);

	// Ast_Evaluate_ToTsys_TypeImpl.cpp (literal)
	ITsys*					ProcessPrimitiveType(const ParsingArguments& pa, PrimitiveType* self);

	// Ast_Evaluate_ToTsys_TypeImpl.cpp (unbounded)
	ITsys*					ProcessReferenceType(const ParsingArguments& pa, ReferenceType* self, ExprTsysItem arg);
	ITsys*					ProcessArrayType(const ParsingArguments& pa, ArrayType* self, ExprTsysItem arg);
	ITsys*					ProcessMemberType(const ParsingArguments& pa, MemberType* self, ExprTsysItem argType, ExprTsysItem argClass);
	ITsys*					ProcessDeclType(const ParsingArguments& pa, DeclType* self, ExprTsysItem arg);
	ITsys*					ProcessDecorateType(const ParsingArguments& pa, DecorateType* self, ExprTsysItem arg);

	//////////////////////////////////////////////////////////////////////////////////////
	// Exprs
	//////////////////////////////////////////////////////////////////////////////////////

	// Ast_Evaluate_ToTsys_ExprImpl.cpp (literal)
	void					ProcessLiteralExpr(const ParsingArguments& pa, ExprTsysList& result, GenericArgContext* gaContext, LiteralExpr* self);
	void					ProcessThisExpr(const ParsingArguments& pa, ExprTsysList& result, GenericArgContext* gaContext, ThisExpr* self);
	void					ProcessNullptrExpr(const ParsingArguments& pa, ExprTsysList& result, GenericArgContext* gaContext, NullptrExpr* self);
	void					ProcessTypeidExpr(const ParsingArguments& pa, ExprTsysList& result, GenericArgContext* gaContext, TypeidExpr* self);
	void					ProcessSizeofExpr(const ParsingArguments& pa, ExprTsysList& result, GenericArgContext* gaContext, SizeofExpr* self);
	void					ProcessThrowExpr(const ParsingArguments& pa, ExprTsysList& result, GenericArgContext* gaContext, ThrowExpr* self);
	void					ProcessDeleteExpr(const ParsingArguments& pa, ExprTsysList& result, GenericArgContext* gaContext, DeleteExpr* self);

	// Ast_Evaluate_ToTsys_ExprImpl.cpp (unbounded)
	void					ProcessParenthesisExpr(const ParsingArguments& pa, ExprTsysList& result, ParenthesisExpr* self, ExprTsysItem arg);
	void					ProcessCastExpr(const ParsingArguments& pa, ExprTsysList& result, CastExpr* self, ExprTsysItem argType, ExprTsysItem argExpr);
	void					ProcessArrayAccessExpr(const ParsingArguments& pa, ExprTsysList& result, GenericArgContext* gaContext, ArrayAccessExpr* self, ExprTsysItem argArray, ExprTsysItem argIndex, bool& indexed);
	void					ProcessPostfixUnaryExpr(const ParsingArguments& pa, ExprTsysList& result, GenericArgContext* gaContext, PostfixUnaryExpr* self, ExprTsysItem arg, bool& indexed);
	void					ProcessPrefixUnaryExpr(const ParsingArguments& pa, ExprTsysList& result, GenericArgContext* gaContext, PrefixUnaryExpr* self, ExprTsysItem arg, bool& indexed);
	void					ProcessBinaryExpr(const ParsingArguments& pa, ExprTsysList& result, GenericArgContext* gaContext, BinaryExpr* self, ExprTsysItem argLeft, ExprTsysItem argRight, bool& indexed);
	void					ProcessIfExpr(const ParsingArguments& pa, ExprTsysList& result, GenericArgContext* gaContext, IfExpr* self, ExprTsysItem argCond, ExprTsysItem argLeft, ExprTsysItem argRight);

	//////////////////////////////////////////////////////////////////////////////////////
	// Indexing
	//////////////////////////////////////////////////////////////////////////////////////

	void					AddSymbolsToResolvings(GenericArgContext* gaContext, const CppName* name, Ptr<Resolving>* nameResolving, const CppName* op, Ptr<Resolving>* opResolving, ExprTsysList& symbols, bool& addedName, bool& addedOp);
	void					AddSymbolsToOperatorResolving(GenericArgContext* gaContext, const CppName& op, Ptr<Resolving>& opResolving, ExprTsysList& symbols, bool& addedOp);

	//////////////////////////////////////////////////////////////////////////////////////
	// Utilities
	//////////////////////////////////////////////////////////////////////////////////////

	inline bool IsResolvedToType(Ptr<Type> type)
	{
		if (type.Cast<RootType>())
		{
			return false;
		}

		if (auto resolvableType = type.Cast<ResolvableType>())
		{
			if (resolvableType->resolving)
			{
				auto& symbols = resolvableType->resolving->resolvedSymbols;
				for (vint i = 0; i < symbols.Count(); i++)
				{
					if (symbols[i]->kind != symbol_component::SymbolKind::Namespace)
					{
						return true;
					}
				}
				return false;
			}
		}

		return true;
	}

	inline ExprTsysItem GetExprTsysItem(ITsys* arg)
	{
		return { nullptr,ExprTsysType::PRValue,arg };
	}

	inline ExprTsysItem GetExprTsysItem(ExprTsysItem arg)
	{
		return arg;
	}

	inline void AddTsysToResult(TypeTsysList& result, ITsys* tsys)
	{
		if (!result.Contains(tsys))
		{
			result.Add(tsys);
		}
	}

	inline void AddExprTsysItemToResult(TypeTsysList& result, ExprTsysItem arg)
	{
		AddTsysToResult(result, arg.tsys);
	}

	inline void AddExprTsysListToResult(TypeTsysList& result, ExprTsysList& args)
	{
		for (vint i = 0; i < args.Count(); i++)
		{
			AddExprTsysItemToResult(result, args[i]);
		}
	}

	inline void AddExprTsysItemToResult(ExprTsysList& result, ExprTsysItem arg)
	{
		if (!result.Contains(arg))
		{
			result.Add(arg);
		}
	}

	inline void AddExprTsysListToResult(ExprTsysList& result, ExprTsysList& args)
	{
		for (vint i = 0; i < args.Count(); i++)
		{
			AddExprTsysItemToResult(result, args[i]);
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

	//////////////////////////////////////////////////////////////////////////////////////
	// ExpandPotentialVta(MultiResult)
	//////////////////////////////////////////////////////////////////////////////////////

	namespace impl
	{
		template<vint Index, typename T, typename ...Ts>
		struct SelectImpl
		{
			static __forceinline auto& Do(T& t, Ts& ...ts)
			{
				return SelectImpl<Index - 1, Ts...>::Do(ts...);
			}
		};

		template<typename T, typename ...Ts>
		struct SelectImpl<0, T, Ts...>
		{
			static __forceinline T& Do(T& t, Ts& ...ts)
			{
				return t;
			}
		};

		template<vint Index, typename ...Ts>
		__forceinline auto& Select(Ts& ...ts)
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
						ExprTsysList processResult;
						process(processResult, SelectInput(inputs, -1)...);
						AddExprTsysListToResult(result, processResult);
					}
					else
					{
						Array<ExprTsysList> initParams(unboundedVtaCount);
						for (vint i = 0; i < unboundedVtaCount; i++)
						{
							process((ExprTsysList&)initParams[i], SelectInput(inputs, i)...);
						}

						ExprTsysList initTypes;
						symbol_type_resolving::CreateUniversalInitializerType(pa, initParams, initTypes);
						AddExprTsysListToResult(result, initTypes);
					}
				}
			};
		};
	}

	template<typename TResult, typename TProcess, typename ...TInputs>
	bool ExpandPotentialVtaMultiResult(const ParsingArguments& pa, List<TResult>& result, TProcess&& process, VtaInput<TInputs> ...inputs)
	{
		using Step = typename impl::ExpandPotentialVtaStep<TResult, TProcess, TInputs...>::template Step<0>;
		Step::Do(pa, result, -1, ForwardValue<TProcess&&>(process), inputs...);
		return (inputs.isVta || ...);
	}

	template<typename TResult, typename TProcess, typename ...TInputs>
	bool ExpandPotentialVta(const ParsingArguments& pa, List<TResult>& result, TProcess&& process, VtaInput<TInputs> ...inputs)
	{
		return ExpandPotentialVtaMultiResult(pa, result, [&](ExprTsysList& processResult, auto ...args)
		{
			processResult.Add(GetExprTsysItem(process(args...)));
		}, inputs...);
	}

	//////////////////////////////////////////////////////////////////////////////////////
	// CheckVta
	//////////////////////////////////////////////////////////////////////////////////////

	template<typename TExpr, typename TInput>
	static void CheckVta(VariadicList<TExpr>& arguments, Array<List<TInput>>& inputs, Array<bool>& isVtas, vint offset, bool& hasBoundedVta, bool& hasUnboundedVta, vint& unboundedVtaCount)
	{
		for (vint i = offset; i < inputs.Count(); i++)
		{
			if (isVtas[i])
			{
				if (arguments[i - offset].isVariadic)
				{
					hasBoundedVta = true;
				}
				else
				{
					hasUnboundedVta = true;
				}
			}
		}

		if (hasBoundedVta && hasUnboundedVta)
		{
			throw NotConvertableException();
		}

		if (hasUnboundedVta)
		{
			for (vint i = 0; i < inputs.Count(); i++)
			{
				if (isVtas[i])
				{
					for (vint j = 0; j < inputs[i].Count(); j++)
					{
						auto tsys = GetExprTsysItem(inputs[i][j]).tsys;
						if (tsys->GetType() == TsysType::Init)
						{
							vint currentVtaCount = tsys->GetParamCount();
							if (unboundedVtaCount == -1)
							{
								unboundedVtaCount = currentVtaCount;
							}
							else if (unboundedVtaCount != currentVtaCount)
							{
								throw NotConvertableException();
							}
						}
					}
				}
			}
		}
	}

	//////////////////////////////////////////////////////////////////////////////////////
	// ExpandPotentialVtaList
	//////////////////////////////////////////////////////////////////////////////////////

	namespace impl
	{
		template<typename TResult, typename TInput, typename TProcess>
		static void ExpandPotentialVtaList(const ParsingArguments& pa, List<TResult>& result, Array<List<TInput>>& inputs, Array<bool>& isVtas, bool isBoundedVta, vint unboundedVtaCount, Array<vint>& tsysIndex, vint level, TProcess&& process)
		{
			if (level == inputs.Count())
			{
				SortedList<vint> boundedAnys;
				if (isBoundedVta)
				{
					vint paramCount = 0;
					for (vint i = 0; i < inputs.Count(); i++)
					{
						if (isVtas[i])
						{
							auto tsysVta = GetExprTsysItem(inputs[i][tsysIndex[i]]).tsys;
							if (tsysVta->GetType() == TsysType::Any)
							{
								boundedAnys.Add(paramCount++);
							}
							else
							{
								paramCount += tsysVta->GetParamCount();
							}
						}
						else
						{
							paramCount++;
						}
					}

					Array<ExprTsysItem> params(paramCount);
					vint currentParam = 0;
					for (vint i = 0; i < inputs.Count(); i++)
					{
						auto tsysItem = GetExprTsysItem(inputs[i][tsysIndex[i]]);
						if (isVtas[i] && tsysItem.tsys->GetType() != TsysType::Any)
						{
							vint paramVtaCount = tsysItem.tsys->GetParamCount();
							for (vint j = 0; j < paramVtaCount; j++)
							{
								params[currentParam++] = { tsysItem.tsys->GetInit().headers[j], tsysItem.tsys->GetParam(j) };
							}
						}
						else
						{
							params[currentParam++] = tsysItem;
						}
					}

					ExprTsysList processResult;
					process(processResult, params, boundedAnys);
					AddExprTsysListToResult(result, processResult);
				}
				else
				{
					if (unboundedVtaCount == -1)
					{
						Array<ExprTsysItem> params(inputs.Count());
						for (vint j = 0; j < inputs.Count(); j++)
						{
							params[j] = GetExprTsysItem(inputs[j][tsysIndex[j]]);
						}

						ExprTsysList processResult;
						process(processResult, params, boundedAnys);
						AddExprTsysListToResult(result, processResult);
					}
					else
					{
						Array<ExprTsysList> initParams(unboundedVtaCount);
						for (vint i = 0; i < unboundedVtaCount; i++)
						{
							Array<ExprTsysItem> params(inputs.Count());
							for (vint j = 0; j < inputs.Count(); j++)
							{
								auto tsysItem = GetExprTsysItem(inputs[j][tsysIndex[j]]);
								if (isVtas[j])
								{
									params[j] = { tsysItem.tsys->GetInit().headers[i], tsysItem.tsys->GetParam(i) };
								}
								else
								{
									params[j] = tsysItem;
								}
							}
							process(initParams[i], params, boundedAnys);
						}

						ExprTsysList processResult;
						symbol_type_resolving::CreateUniversalInitializerType(pa, initParams, processResult);
						AddExprTsysListToResult(result, processResult);
					}
				}
			}
			else
			{
				vint levelCount = inputs[level].Count();
				for (vint i = 0; i < levelCount; i++)
				{
					tsysIndex[level] = i;
					ExpandPotentialVtaList<TResult, TInput, TProcess>(pa, result, inputs, isVtas, isBoundedVta, unboundedVtaCount, tsysIndex, level + 1, process);
				}
			}
		}
	}

	template<typename TResult, typename TInput, typename TProcess>
	static void ExpandPotentialVtaList(const ParsingArguments& pa, List<TResult>& result, Array<List<TInput>>& inputs, Array<bool>& isVtas, bool isBoundedVta, vint unboundedVtaCount, TProcess&& process)
	{
		if (!isBoundedVta)
		{
			for (vint i = 0; i < inputs.Count(); i++)
			{
				if (isVtas[i])
				{
					for (vint j = 0; j < inputs[i].Count(); j++)
					{
						if (GetExprTsysItem(inputs[i][j]).tsys->GetType() != TsysType::Init)
						{
							AddExprTsysItemToResult(result, GetExprTsysItem(pa.tsys->Any()));
							return;
						}
					}
				}
			}
		}

		Array<vint> tsysIndex(inputs.Count());
		memset(&tsysIndex[0], 0, sizeof(vint) * inputs.Count());
		impl::ExpandPotentialVtaList(pa, result, inputs, isVtas, isBoundedVta, unboundedVtaCount, tsysIndex, 0, process);
	}
}