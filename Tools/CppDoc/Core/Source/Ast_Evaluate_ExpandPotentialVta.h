#ifndef VCZH_DOCUMENT_CPPDOC_AST_EVALUATE_EPV
#define VCZH_DOCUMENT_CPPDOC_AST_EVALUATE_EPV

#include "Ast_Resolving.h"
#include "Symbol_Visit.h"
#include "Ast_Expr.h"
#include "Ast_Type.h"

namespace symbol_totsys_impl
{
	//////////////////////////////////////////////////////////////////////////////////////
	// Types
	//////////////////////////////////////////////////////////////////////////////////////

	// Ast_Evaluate_ToTsys_IdImpl.cpp (identifier)
	void					CreateIdReferenceType(const ParsingArguments& pa, const Ptr<Resolving>& resolving, bool allowAny, bool allowVariadic, TypeTsysList& result, bool& isVta);
	void					ProcessChildType(const ParsingArguments& pa, ChildType* self, ExprTsysItem argClass, ExprTsysList& result);

	// Ast_Evaluate_ToTsys_TypeImpl.cpp (literal)
	ITsys*					ProcessPrimitiveType(const ParsingArguments& pa, PrimitiveType* self);

	// Ast_Evaluate_ToTsys_TypeImpl.cpp (unbounded)
	ITsys*					ProcessReferenceType(const ParsingArguments& pa, ReferenceType* self, ExprTsysItem arg);
	ITsys*					ProcessArrayType(const ParsingArguments& pa, ArrayType* self, ExprTsysItem arg);
	ITsys*					ProcessMemberType(const ParsingArguments& pa, MemberType* self, ExprTsysItem argType, ExprTsysItem argClass);
	ITsys*					ProcessDeclType(const ParsingArguments& pa, DeclType* self, ExprTsysItem arg);
	ITsys*					ProcessDecorateType(const ParsingArguments& pa, DecorateType* self, ExprTsysItem arg);

	// Ast_Evaluate_ToTsys_VariadicImpl.cpp (variadic)
	void					ProcessFunctionType(const ParsingArguments& pa, ExprTsysList& result, FunctionType* self, TsysCallingConvention cc, bool memberOf, Array<ExprTsysItem>& args, SortedList<vint>& boundedAnys);

	// Ast_Evaluate_ToTsys_GenericImpl.cpp (variadic)
	void					ProcessGenericType(const ParsingArguments& pa, ExprTsysList& result, GenericType* self, Array<bool>& isTypes, Array<ExprTsysItem>& args, Array<vint>& argSource, SortedList<vint>& boundedAnys);

	//////////////////////////////////////////////////////////////////////////////////////
	// Exprs
	//////////////////////////////////////////////////////////////////////////////////////

	// Ast_Evaluate_ToTsys_IdImpl.cpp (identifier)
	void					CreateIdReferenceExpr(const ParsingArguments& pa, const Ptr<Resolving>& resolving, ExprTsysList& result, const ExprTsysItem* childExprClassItem, bool allowAny, bool allowVariadic, bool& isVta);
	void					ProcessChildExpr(const ParsingArguments& pa, ExprTsysList& result, ChildExpr* self, ExprTsysItem argClass);
	void					ProcessFieldAccessExprForIdExpr(const ParsingArguments& pa, ExprTsysList& result, FieldAccessExpr* self, ExprTsysItem argParent, const Ptr<IdExpr>& idExpr, ResolveSymbolResult& totalRar, List<ResolvedItem>* ritems);
	void					ProcessFieldAccessExprForChildExpr(const ParsingArguments& pa, ExprTsysList& result, FieldAccessExpr* self, ExprTsysItem argParent, ExprTsysItem argClass, const Ptr<ChildExpr>& childExpr, ResolveSymbolResult& totalRar, List<ResolvedItem>* ritems);

	// Ast_Evaluate_ToTsys_ExprImpl.cpp (literal)
	void					ProcessLiteralExpr(const ParsingArguments& pa, ExprTsysList& result, LiteralExpr* self);
	void					ProcessThisExpr(const ParsingArguments& pa, ExprTsysList& result, ThisExpr* self);
	void					ProcessNullptrExpr(const ParsingArguments& pa, ExprTsysList& result, NullptrExpr* self);
	void					ProcessThrowExpr(const ParsingArguments& pa, ExprTsysList& result, ThrowExpr* self);

	// Ast_Evaluate_ToTsys_ExprImpl.cpp (unbounded)
	void					ProcessParenthesisExpr(const ParsingArguments& pa, ExprTsysList& result, ParenthesisExpr* self, ExprTsysItem arg);
	void					ProcessCastExpr(const ParsingArguments& pa, ExprTsysList& result, CastExpr* self, ExprTsysItem argType, ExprTsysItem argExpr);
	void					ProcessTypeidExpr(const ParsingArguments& pa, ExprTsysList& result, TypeidExpr* self);
	void					ProcessArrayAccessExpr(const ParsingArguments& pa, ExprTsysList& result, ArrayAccessExpr* self, ExprTsysItem argArray, ExprTsysItem argIndex, List<ResolvedItem>* ritems);
	void					ProcessPostfixUnaryExpr(const ParsingArguments& pa, ExprTsysList& result, PostfixUnaryExpr* self, ExprTsysItem arg, List<ResolvedItem>* ritems);
	void					ProcessPrefixUnaryExpr(const ParsingArguments& pa, ExprTsysList& result, PrefixUnaryExpr* self, ExprTsysItem arg, List<ResolvedItem>* ritems);
	void					ProcessBinaryExpr(const ParsingArguments& pa, ExprTsysList& result, BinaryExpr* self, ExprTsysItem argLeft, ExprTsysItem argRight, List<ResolvedItem>* ritems);
	void					ProcessIfExpr(const ParsingArguments& pa, ExprTsysList& result, IfExpr* self, ExprTsysItem argCond, ExprTsysItem argLeft, ExprTsysItem argRight);

	// Ast_Evaluate_ToTsys_VariadicImpl.cpp (variadic)
	void					ProcessCtorAccessExpr(const ParsingArguments& pa, ExprTsysList& result, CtorAccessExpr* self, Array<ExprTsysItem>& args, SortedList<vint>& boundedAnys);
	void					ProcessNewExpr(const ParsingArguments& pa, ExprTsysList& result, NewExpr* self, Array<ExprTsysItem>& args, SortedList<vint>& boundedAnys);
	void					ProcessUniversalInitializerExpr(const ParsingArguments& pa, ExprTsysList& result, UniversalInitializerExpr* self, Array<ExprTsysItem>& args, SortedList<vint>& boundedAnys);

	// Ast_Evaluate_ToTsys_GenericImpl.cpp (variadic)
	void					ProcessGenericExpr(const ParsingArguments& pa, ExprTsysList& result, GenericExpr* self, Array<bool>& isTypes, Array<ExprTsysItem>& args, Array<vint>& argSource, SortedList<vint>& boundedAnys);

	//////////////////////////////////////////////////////////////////////////////////////
	// Utilities
	//////////////////////////////////////////////////////////////////////////////////////

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
									throw TypeCheckerException();
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
	// ExpandPotentialVtaList
	//////////////////////////////////////////////////////////////////////////////////////

	namespace impl
	{
		template<typename TResult, typename TInput, typename TProcess>
		void ExpandPotentialVtaList(const ParsingArguments& pa, List<TResult>& result, Array<List<TInput>>& inputs, Array<bool>& isVtas, bool isBoundedVta, vint unboundedVtaCount, Array<vint>& tsysIndex, vint level, TProcess&& process)
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
					Array<vint> argSource(paramCount);
					vint currentParam = 0;

					for (vint i = 0; i < inputs.Count(); i++)
					{
						auto tsysItem = GetExprTsysItem(inputs[i][tsysIndex[i]]);
						if (isVtas[i] && tsysItem.tsys->GetType() != TsysType::Any)
						{
							vint paramVtaCount = tsysItem.tsys->GetParamCount();
							for (vint j = 0; j < paramVtaCount; j++)
							{
								params[currentParam] = { tsysItem.tsys->GetInit().headers[j], tsysItem.tsys->GetParam(j) };
								argSource[currentParam] = i;
								currentParam++;
							}
						}
						else
						{
							params[currentParam] = tsysItem;
							argSource[currentParam] = i;
							currentParam++;
						}
					}

					ExprTsysList processResult;
					process(processResult, params, -1, argSource, boundedAnys);
					AddExprTsysListToResult(result, processResult);
				}
				else
				{
					Array<vint> argSource(inputs.Count());
					for (vint i = 0; i < inputs.Count(); i++)
					{
						argSource[i] = i;
					}

					if (unboundedVtaCount == -1)
					{
						Array<ExprTsysItem> params(inputs.Count());
						for (vint j = 0; j < inputs.Count(); j++)
						{
							params[j] = GetExprTsysItem(inputs[j][tsysIndex[j]]);
						}

						ExprTsysList processResult;
						process(processResult, params, -1, argSource, boundedAnys);
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
							process(initParams[i], params, i, argSource, boundedAnys);
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
	void ExpandPotentialVtaList(const ParsingArguments& pa, List<TResult>& result, Array<List<TInput>>& inputs, Array<bool>& isVtas, bool isBoundedVta, vint unboundedVtaCount, TProcess&& process)
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
		if (inputs.Count() > 0)
		{
			memset(&tsysIndex[0], 0, sizeof(vint) * inputs.Count());
		}
		impl::ExpandPotentialVtaList(pa, result, inputs, isVtas, isBoundedVta, unboundedVtaCount, tsysIndex, 0, process);
	}

	//////////////////////////////////////////////////////////////////////////////////////
	// CheckVta
	//////////////////////////////////////////////////////////////////////////////////////

	template<typename TExpr, typename TInput>
	bool CheckVta(const VariadicList<TExpr>& arguments, Array<List<TInput>>& inputs, Array<bool>& isVtas, vint offset, bool& hasBoundedVta, vint& unboundedVtaCount)
	{
		bool hasUnboundedVta = false;
		for (vint i = 0; i < offset; i++)
		{
			if (isVtas[i])
			{
				hasUnboundedVta = true;
			}
		}

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
			throw TypeCheckerException();
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
								throw TypeCheckerException();
							}
						}
					}
				}
			}
		}

		return hasUnboundedVta;
	}

	//////////////////////////////////////////////////////////////////////////////////////
	// VariadicInput
	//////////////////////////////////////////////////////////////////////////////////////

	template<typename TInput>
	class VariadicInput
	{
	private:
		const ParsingArguments&		pa;

		Array<List<TInput>>			argItems;
		Array<bool>					isVtas;
		bool						hasBoundedVta = false;
		vint						unboundedVtaCount = -1;

		void Apply(TypeTsysList& result, bool& isVta, const Ptr<Type>& type, TypeToTsysConfig config)
		{
			TypeToTsysInternal(pa, type, result, isVta, config);
		}

		void Apply(TypeTsysList& result, bool& isVta, const Ptr<VariableDeclaration>& varDecl, TypeToTsysConfig config)
		{
			TypeToTsysInternal(pa, varDecl->type, result, isVta, config);
		}

		void Apply(ExprTsysList& result, bool& isVta, const Ptr<Type>& type, TypeToTsysConfig config)
		{
			TypeTsysList tsyses;
			TypeToTsysInternal(pa, type, tsyses, isVta, config);
			symbol_type_resolving::AddType(result, tsyses);
		}

		void Apply(ExprTsysList& result, bool& isVta, const Ptr<Expr>& expr, TypeToTsysConfig config)
		{
			ExprToTsysInternal(pa, expr, result, isVta);
		}

	public:
		VariadicInput(vint count, const ParsingArguments& _pa)
			:pa(_pa)
			, argItems(count)
			, isVtas(count)
		{
		}

		void ApplyTypes(vint index, const TypeTsysList& types, bool isVta)
		{
			for (vint i = 0; i < types.Count(); i++)
			{
				AddExprTsysItemToResult(argItems[index], GetExprTsysItem(types[i]));
			}
			isVtas[index] = isVta;
		}

		void ApplyTypes(vint index, const ExprTsysList& types, bool isVta)
		{
			for (vint i = 0; i < types.Count(); i++)
			{
				AddExprTsysItemToResult(argItems[index], types[i]);
			}
			isVtas[index] = isVta;
		}

		template<typename TNode>
		void ApplySingle(vint index, const Ptr<TNode>& node, TypeToTsysConfig config = {})
		{
			Apply(argItems[index], isVtas[index], node, config);
		}

		template<typename TNode>
		void ApplyVariadicList(vint index, const VariadicList<Ptr<TNode>>& nodes)
		{
			for (vint i = 0; i < nodes.Count(); i++)
			{
				ApplySingle<TNode>(i + index, nodes[i].item);
			}
		}

		void ApplyGenericArguments(vint index, Array<bool>& isTypes, const VariadicList<GenericArgument>& arguments)
		{
			for (vint i = 0; i < arguments.Count(); i++)
			{
				auto argument = arguments[i];
				if ((isTypes[i + index] = argument.item.type))
				{
					ApplySingle(i + index, argument.item.type);
				}
				else
				{
					ApplySingle(i + index, argument.item.expr);
				}
			}
		}

		template<typename TItem, typename TResult, typename TProcess>
		bool Expand(const VariadicList<TItem>* variadicList, List<TResult>& result, TProcess&& process)
		{
			bool isVta = CheckVta(
				*variadicList,
				argItems,
				isVtas,
				argItems.Count() - (variadicList ? variadicList->Count() : 0),
				hasBoundedVta,
				unboundedVtaCount
			);
			ExpandPotentialVtaList(pa, result, argItems, isVtas, hasBoundedVta, unboundedVtaCount, process);
			return isVta;
		}
	};
}

#endif