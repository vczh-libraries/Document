#include "Ast_Resolving.h"

namespace symbol_type_resolving
{
	/***********************************************************************
	Add: Add something to ExprTsysList
	***********************************************************************/

	bool AddInternal(ExprTsysList& list, const ExprTsysItem& item)
	{
		if (list.Contains(item)) return false;
		list.Add(item);
		return true;
	}

	void AddInternal(ExprTsysList& list, ExprTsysList& items)
	{
		for (vint i = 0; i < items.Count(); i++)
		{
			AddInternal(list, items[i]);
		}
	}

	bool AddVar(ExprTsysList& list, const ExprTsysItem& item)
	{
		return AddInternal(list, { item.symbol,ExprTsysType::LValue,item.tsys });
	}

	bool AddNonVar(ExprTsysList& list, const ExprTsysItem& item)
	{
		return AddInternal(list, { item.symbol,ExprTsysType::PRValue,item.tsys });
	}

	void AddNonVar(ExprTsysList& list, ExprTsysList& items)
	{
		for (vint i = 0; i < items.Count(); i++)
		{
			AddNonVar(list, items[i]);
		}
	}

	bool AddTemp(ExprTsysList& list, ITsys* tsys)
	{
		if (tsys->GetType() == TsysType::RRef)
		{
			return AddInternal(list, { nullptr,ExprTsysType::XValue,tsys });
		}
		else if (tsys->GetType() == TsysType::LRef)
		{
			return AddInternal(list, { nullptr,ExprTsysType::LValue,tsys });
		}
		else
		{
			return AddInternal(list, { nullptr,ExprTsysType::PRValue,tsys });
		}
	}

	void AddTemp(ExprTsysList& list, TypeTsysList& items)
	{
		for (vint i = 0; i < items.Count(); i++)
		{
			AddTemp(list, items[i]);
		}
	}

	/***********************************************************************
	CreateUniversalInitializerType: Create init types from element types
	***********************************************************************/

	void CreateUniversalInitializerType(const ParsingArguments& pa, Array<ExprTsysList>& argTypesList, Array<vint>& indices, vint index, ExprTsysList& result)
	{
		if (index == argTypesList.Count())
		{
			Array<ExprTsysItem> params(index);
			for (vint i = 0; i < index; i++)
			{
				params[i] = argTypesList[i][indices[i]];
			}
			AddInternal(result, { nullptr,ExprTsysType::PRValue,pa.tsys->InitOf(params) });
		}
		else
		{
			for (vint i = 0; i < argTypesList[index].Count(); i++)
			{
				indices[index] = i;
				CreateUniversalInitializerType(pa, argTypesList, indices, index + 1, result);
			}
		}
	}

	void CreateUniversalInitializerType(const ParsingArguments& pa, Array<ExprTsysList>& argTypesList, ExprTsysList& result)
	{
		Array<vint> indices(argTypesList.Count());
		CreateUniversalInitializerType(pa, argTypesList, indices, 0, result);
	}

	/***********************************************************************
	CalculateValueFieldType: Given thisItem, fill the field type fo ExprTsysList
		Value: t.f
		Ptr: t->f
	***********************************************************************/

	void CalculateValueFieldType(const ExprTsysItem* thisItem, Symbol* symbol, ITsys* fieldType, bool forFieldDeref, ExprTsysList& result)
	{
		TsysCV cv;
		TsysRefType refType;
		thisItem->tsys->GetEntity(cv, refType);

		auto type = fieldType->CVOf(cv);
		auto lrefType = forFieldDeref ? type->LRefOf() : type;

		if (refType == TsysRefType::LRef)
		{
			AddInternal(result, { symbol,ExprTsysType::LValue,lrefType });
		}
		else if (refType == TsysRefType::RRef)
		{
			if (thisItem->type == ExprTsysType::LValue)
			{
				AddInternal(result, { symbol,ExprTsysType::LValue,lrefType });
			}
			else
			{
				AddInternal(result, { symbol,ExprTsysType::XValue,type->RRefOf() });
			}
		}
		else if (forFieldDeref)
		{
			AddInternal(result, { symbol,ExprTsysType::LValue,lrefType });
		}
		else
		{
			AddInternal(result, { symbol,thisItem->type,type });
		}
	}

	/***********************************************************************
	EvaluateSymbol: Evaluate the declared type for a variable
	***********************************************************************/

	void EvaluateSymbol(const ParsingArguments& pa, ForwardVariableDeclaration* varDecl)
	{
		auto symbol = varDecl->symbol;
		auto& ev = symbol->evaluation;
		switch (ev.progress)
		{
		case symbol_component::EvaluationProgress::Evaluated: return;
		case symbol_component::EvaluationProgress::Evaluating: throw NotResolvableException();
		}

		ev.progress = symbol_component::EvaluationProgress::Evaluating;
		ev.Allocate();

		auto newPa = pa.WithContext(symbol->parent);

		if (varDecl->needResolveTypeFromInitializer)
		{
			if (auto rootVarDecl = dynamic_cast<VariableDeclaration*>(varDecl))
			{
				if (!rootVarDecl->initializer)
				{
					throw NotResolvableException();
				}

				ExprTsysList types;
				ExprToTsys(newPa, rootVarDecl->initializer->arguments[0], types);

				for (vint k = 0; k < types.Count(); k++)
				{
					auto type = ResolvePendingType(pa, rootVarDecl->type, types[k]);
					if (!ev.Get().Contains(type))
					{
						ev.Get().Add(type);
					}
				}
			}
			else
			{
				throw NotResolvableException();
			}
		}
		else
		{
			TypeToTsysNoVta(newPa, varDecl->type, ev.Get(), nullptr);
		}
		ev.progress = symbol_component::EvaluationProgress::Evaluated;
	}

	/***********************************************************************
	EvaluateSymbol: Evaluate the declared type for a function
	***********************************************************************/

	bool IsMemberFunction(const ParsingArguments& pa, ForwardFunctionDeclaration* funcDecl)
	{
		auto symbol = funcDecl->symbol;
		ITsys* classScope = nullptr;
		if (symbol->parent && symbol->parent->definition.Cast<ClassDeclaration>())
		{
			classScope = pa.tsys->DeclOf(symbol->parent);
		}

		bool isStaticSymbol = IsStaticSymbol<ForwardFunctionDeclaration>(symbol);
		return classScope && !isStaticSymbol;
	}

	void FinishEvaluatingSymbol(const ParsingArguments& pa, FunctionDeclaration* funcDecl)
	{
		auto symbol = funcDecl->symbol;
		auto& ev = symbol->evaluation;
		if (ev.Get().Count() == 0)
		{
			throw NotResolvableException();
		}
		else
		{
			auto newPa = pa.WithContext(symbol->parent);
			TypeTsysList returnTypes;
			CopyFrom(returnTypes, ev.Get());
			ev.Get().Clear();

			TypeTsysList processedReturnTypes;
			{
				auto funcType = GetTypeWithoutMemberAndCC(funcDecl->type).Cast<FunctionType>();
				auto pendingType = funcType->decoratorReturnType ? funcType->decoratorReturnType : funcType->returnType;
				for (vint i = 0; i < returnTypes.Count(); i++)
				{
					auto tsys = ResolvePendingType(newPa, pendingType, { nullptr,ExprTsysType::PRValue,returnTypes[i] });
					if (!processedReturnTypes.Contains(tsys))
					{
						processedReturnTypes.Add(tsys);
					}
				}
			}
			TypeToTsysAndReplaceFunctionReturnType(newPa, funcDecl->type, processedReturnTypes, ev.Get(), nullptr, IsMemberFunction(pa, funcDecl));
			ev.progress = symbol_component::EvaluationProgress::Evaluated;
		}
	}

	void EvaluateSymbol(const ParsingArguments& pa, ForwardFunctionDeclaration* funcDecl)
	{
		auto symbol = funcDecl->symbol;
		auto& ev = symbol->evaluation;
		switch (ev.progress)
		{
		case symbol_component::EvaluationProgress::Evaluated: return;
		case symbol_component::EvaluationProgress::Evaluating: throw NotResolvableException();
		}

		ev.progress = symbol_component::EvaluationProgress::Evaluating;
		
		if (funcDecl->needResolveTypeFromStatement)
		{
			if (auto rootFuncDecl = dynamic_cast<FunctionDeclaration*>(funcDecl))
			{
				EnsureFunctionBodyParsed(rootFuncDecl);
				auto funcPa = pa.WithContext(symbol);
				EvaluateStat(funcPa, rootFuncDecl->statement);
				if (ev.Count() == 0)
				{
					throw NotResolvableException();
				}
			}
			else
			{
				throw NotResolvableException();
			}
		}
		else
		{
			auto newPa = pa.WithContext(symbol->parent);
			ev.Allocate();
			TypeToTsysNoVta(newPa, funcDecl->type, ev.Get(), nullptr, IsMemberFunction(pa, funcDecl));
			ev.progress = symbol_component::EvaluationProgress::Evaluated;
		}
	}

	/***********************************************************************
	EvaluateSymbol: Evaluate base types for a class
	***********************************************************************/

	void EvaluateSymbol(const ParsingArguments& pa, ClassDeclaration* classDecl)
	{
		auto symbol = classDecl->symbol;
		auto& ev = symbol->evaluation;
		switch (ev.progress)
		{
		case symbol_component::EvaluationProgress::Evaluated: return;
		case symbol_component::EvaluationProgress::Evaluating: throw NotResolvableException();
		}

		ev.progress = symbol_component::EvaluationProgress::Evaluating;
		ev.Allocate(classDecl->baseTypes.Count());

		{
			auto newPa = pa.WithContext(symbol);
			for (vint i = 0; i < classDecl->baseTypes.Count(); i++)
			{
				TypeToTsysNoVta(newPa, classDecl->baseTypes[i].f1, ev.Get(i), nullptr);
			}
		}
		ev.progress = symbol_component::EvaluationProgress::Evaluated;
	}

	/***********************************************************************
	EvaluateSymbol: Evaluate the declared type for an alias
	***********************************************************************/

	void EvaluateSymbol(const ParsingArguments& pa, TypeAliasDeclaration* usingDecl, EvaluateSymbolContext* esContext)
	{
		auto symbol = usingDecl->symbol;
		if (!esContext)
		{
			auto& ev = symbol->evaluation;
			switch (ev.progress)
			{
			case symbol_component::EvaluationProgress::Evaluated: return;
			case symbol_component::EvaluationProgress::Evaluating: throw NotResolvableException();
			}
			ev.progress = symbol_component::EvaluationProgress::Evaluating;
			ev.Allocate();
		}

		auto newPa = pa.WithContext(symbol->parent);
		auto& evaluatedTypes = esContext ? esContext->evaluatedTypes : symbol->evaluation.Get();

		TypeTsysList types;
		TypeToTsysNoVta(newPa, usingDecl->type, types, (esContext ? &esContext->gaContext : nullptr));

		if (usingDecl->templateSpec && !esContext)
		{
			TsysGenericFunction genericFunction;
			TypeTsysList params;

			genericFunction.declSymbol = symbol;
			CreateGenericFunctionHeader(usingDecl->templateSpec, params, genericFunction);

			for (vint i = 0; i < types.Count(); i++)
			{
				evaluatedTypes.Add(types[i]->GenericFunctionOf(params, genericFunction));
			}
		}
		else
		{
			CopyFrom(evaluatedTypes, types);
		}

		if (evaluatedTypes.Count() == 0)
		{
			throw NotResolvableException();
		}

		if (!esContext)
		{
			auto& ev = symbol->evaluation;
			ev.progress = symbol_component::EvaluationProgress::Evaluated;
		}
	}

	/***********************************************************************
	EvaluateSymbol: Evaluate the declared value for an alias
	***********************************************************************/

	void EvaluateSymbol(const ParsingArguments& pa, ValueAliasDeclaration* usingDecl, EvaluateSymbolContext* esContext)
	{
		auto symbol = usingDecl->symbol;
		if (!esContext)
		{
			auto& ev = symbol->evaluation;
			switch (ev.progress)
			{
			case symbol_component::EvaluationProgress::Evaluated: return;
			case symbol_component::EvaluationProgress::Evaluating: throw NotResolvableException();
			}
			ev.progress = symbol_component::EvaluationProgress::Evaluating;
			ev.Allocate();
		}

		auto newPa = pa.WithContext(symbol->parent);
		auto& evaluatedTypes = esContext ? esContext->evaluatedTypes : symbol->evaluation.Get();

		TypeTsysList types;
		if (usingDecl->needResolveTypeFromInitializer)
		{
			ExprTsysList tsys;
			ExprToTsys(newPa, usingDecl->expr, tsys, (esContext ? &esContext->gaContext : nullptr));
			for (vint i = 0; i < tsys.Count(); i++)
			{
				auto entityType = tsys[i].tsys;
				if (entityType->GetType() == TsysType::Zero)
				{
					entityType = pa.tsys->Int();
				}
				if (!types.Contains(entityType))
				{
					types.Add(entityType);
				}
			}
		}
		else
		{
			TypeToTsysNoVta(newPa, usingDecl->type, types, (esContext ? &esContext->gaContext : nullptr));
		}

		if (usingDecl->templateSpec && !esContext)
		{
			TsysGenericFunction genericFunction;
			TypeTsysList params;

			genericFunction.declSymbol = symbol;
			CreateGenericFunctionHeader(usingDecl->templateSpec, params, genericFunction);

			for (vint i = 0; i < types.Count(); i++)
			{
				evaluatedTypes.Add(types[i]->GenericFunctionOf(params, genericFunction));
			}
		}
		else
		{
			CopyFrom(evaluatedTypes, types);
		}

		if (evaluatedTypes.Count() == 0)
		{
			throw NotResolvableException();
		}

		if (!esContext)
		{
			auto& ev = symbol->evaluation;
			ev.progress = symbol_component::EvaluationProgress::Evaluated;
		}
	}

	/***********************************************************************
	VisitSymbol: Fill a symbol to ExprTsysList
		thisItem: When afterScope==false
			it represents typeof(x) in x.name or typeof(&x) in x->name
			it could be null if it is initiated by IdExpr
	***********************************************************************/

	void VisitSymbol(const ParsingArguments& pa, const ExprTsysItem* thisItem, Symbol* symbol, bool afterScope, ExprTsysList& result)
	{
		ITsys* classScope = nullptr;
		if (symbol->parent && symbol->parent->definition.Cast<ClassDeclaration>())
		{
			classScope = pa.tsys->DeclOf(symbol->parent);
		}

		switch (symbol->kind)
		{
		case symbol_component::SymbolKind::Variable:
			{
				auto varDecl = symbol->GetAnyForwardDecl<ForwardVariableDeclaration>();
				EvaluateSymbol(pa, varDecl.Obj());
				bool isStaticSymbol = IsStaticSymbol<ForwardVariableDeclaration>(symbol);

				for (vint k = 0; k < symbol->evaluation.Get().Count(); k++)
				{
					auto tsys = symbol->evaluation.Get()[k];

					if (isStaticSymbol)
					{
						AddInternal(result, { symbol,ExprTsysType::LValue,tsys });
					}
					else if (afterScope)
					{
						if (classScope)
						{
							AddInternal(result, { symbol,ExprTsysType::PRValue,tsys->MemberOf(classScope) });
						}
						else
						{
							AddInternal(result, { symbol,ExprTsysType::LValue,tsys });
						}
					}
					else
					{
						if (thisItem)
						{
							CalculateValueFieldType(thisItem, symbol, tsys, false, result);
						}
						else
						{
							AddInternal(result, { symbol,ExprTsysType::LValue,tsys });
						}
					}
				}
			}
			break;
		case symbol_component::SymbolKind::Function:
			{
				auto funcDecl = symbol->GetAnyForwardDecl<ForwardFunctionDeclaration>();
				EvaluateSymbol(pa, funcDecl.Obj());
				bool isStaticSymbol = IsStaticSymbol<ForwardFunctionDeclaration>(symbol);
				bool isMember = classScope && !isStaticSymbol;

				for (vint k = 0; k < symbol->evaluation.Get().Count(); k++)
				{
					auto tsys = symbol->evaluation.Get()[k];

					if (isMember && afterScope)
					{
						tsys = tsys->MemberOf(classScope)->PtrOf();
					}
					else
					{
						tsys = tsys->PtrOf();
					}

					AddInternal(result, { symbol,ExprTsysType::PRValue,tsys });
				}
			}
			break;
		case symbol_component::SymbolKind::EnumItem:
			{
				auto tsys = pa.tsys->DeclOf(symbol->parent);
				AddInternal(result, { symbol,ExprTsysType::PRValue,tsys });
			}
			break;
		case symbol_component::SymbolKind::ValueAlias:
			{
				auto usingDecl = symbol->definition.Cast<ValueAliasDeclaration>();
				EvaluateSymbol(pa, usingDecl.Obj());
				AddTemp(result, symbol->evaluation.Get());
			}
			break;
		case symbol_component::SymbolKind::GenericValueArgument:
			{
				AddTemp(result, symbol->evaluation.Get());
			}
			break;
		default:
			throw IllegalExprException();
		}
	}

	/***********************************************************************
	FindMembersByName: Fill all members of a name to ExprTsysList
	***********************************************************************/

	Ptr<Resolving> FindMembersByName(const ParsingArguments& pa, CppName& name, ResolveSymbolResult* totalRar, const ExprTsysItem& parentItem)
	{
		TsysCV cv;
		TsysRefType refType;
		auto entity = parentItem.tsys->GetEntity(cv, refType);
		if (entity->GetType() == TsysType::Decl)
		{
			auto symbol = entity->GetDecl();
			auto fieldPa = pa.WithContext(symbol);
			auto rar = ResolveSymbol(fieldPa, name, SearchPolicy::ChildSymbol);
			if (totalRar) totalRar->Merge(rar);
			return rar.values;
		}
		return nullptr;
	}

	/***********************************************************************
	TestFunctionQualifier: Match this pointer's and functions' qualifiers
		Returns: Exact, TrivalConversion, Illegal
	***********************************************************************/

	TsysConv TestFunctionQualifier(TsysCV thisCV, TsysRefType thisRef, const ExprTsysItem& funcType)
	{
		if (funcType.symbol)
		{
			if (auto decl = funcType.symbol->GetAnyForwardDecl<ForwardFunctionDeclaration>())
			{
				if (auto declType = GetTypeWithoutMemberAndCC(decl->type).Cast<FunctionType>())
				{
					return ::TestFunctionQualifier(thisCV, thisRef, declType);
				}
			}
		}
		return TsysConv::Exact;
	}

	/***********************************************************************
	FilterFieldsAndBestQualifiedFunctions: Filter functions by their qualifiers
	***********************************************************************/

	TsysConv FindMinConv(ArrayBase<TsysConv>& funcChoices)
	{
		auto target = TsysConv::Illegal;
		for (vint i = 0; i < funcChoices.Count(); i++)
		{
			auto candidate = funcChoices[i];
			if (candidate != TsysConv::Any && target > candidate)
			{
				target = candidate;
			}
		}
		return target;
	}

	bool IsFunctionAcceptableByMinConv(TsysConv functionConv, TsysConv minConv)
	{
		switch (functionConv)
		{
		case TsysConv::Any:			return true;
		case TsysConv::Illegal:		return false;
		default:					return functionConv == minConv;
		}
	}

	void FilterFunctionByConv(ExprTsysList& funcTypes, ArrayBase<TsysConv>& funcChoices)
	{
		auto target = FindMinConv(funcChoices);
		for (vint i = funcTypes.Count() - 1; i >= 0; i--)
		{
			if (!IsFunctionAcceptableByMinConv(funcChoices[i], target))
			{
				funcTypes.RemoveAt(i);
			}
		}
	}

	void FilterFieldsAndBestQualifiedFunctions(TsysCV thisCV, TsysRefType thisRef, ExprTsysList& funcTypes)
	{
		Array<TsysConv> funcChoices(funcTypes.Count());

		for (vint i = 0; i < funcTypes.Count(); i++)
		{
			funcChoices[i] = TestFunctionQualifier(thisCV, thisRef, funcTypes[i]);
		}

		FilterFunctionByConv(funcTypes, funcChoices);
	}

	/***********************************************************************
	FindQualifiedFunctors: Remove everything that are not qualified functors (including functions and operator())
	***********************************************************************/

	void FindQualifiedFunctors(const ParsingArguments& pa, TsysCV thisCV, TsysRefType thisRef, ExprTsysList& funcTypes, bool lookForOp)
	{
		ExprTsysList expandedFuncTypes;
		List<TsysConv> funcChoices;

		for (vint i = 0; i < funcTypes.Count(); i++)
		{
			auto funcType = funcTypes[i];
			auto choice = TestFunctionQualifier(thisCV, thisRef, funcType);

			if (choice != TsysConv::Illegal)
			{
				TsysCV cv;
				TsysRefType refType;
				auto entityType = funcType.tsys->GetEntity(cv, refType);

				if (entityType->GetType() == TsysType::Decl && lookForOp)
				{
					ExprTsysList opResult;
					VisitFunctors(pa, funcType, L"operator ()", opResult);

					vint oldCount = expandedFuncTypes.Count();
					AddNonVar(expandedFuncTypes, opResult);
					vint newCount = expandedFuncTypes.Count();

					for (vint i = 0; i < (newCount - oldCount); i++)
					{
						funcChoices.Add(TsysConv::Exact);
					}
				}
				else if (entityType->GetType() == TsysType::Ptr)
				{
					entityType = entityType->GetElement();
					if (entityType->GetType() == TsysType::Function)
					{
						if (AddInternal(expandedFuncTypes, { funcType.symbol,funcType.type, entityType }))
						{
							funcChoices.Add(choice);
						}
					}
				}
			}
		}

		FilterFunctionByConv(expandedFuncTypes, funcChoices);
		CopyFrom(funcTypes, expandedFuncTypes);
	}

	/***********************************************************************
	VisitResolvedMember: Fill all resolved member symbol to ExprTsysList
	***********************************************************************/

	void VisitResolvedMember(const ParsingArguments& pa, const ExprTsysItem* thisItem, Ptr<Resolving> resolving, ExprTsysList& result)
	{
		ExprTsysList varTypes, funcTypes;
		for (vint i = 0; i < resolving->resolvedSymbols.Count(); i++)
		{
			auto targetTypeList = &result;
			auto symbol = resolving->resolvedSymbols[i];
			
			if (symbol->parent)
			{
				switch (symbol->parent->kind)
				{
				case symbol_component::SymbolKind::Class:
				case symbol_component::SymbolKind::Struct:
				case symbol_component::SymbolKind::Union:
					switch (symbol->kind)
					{
					case symbol_component::SymbolKind::Variable:
						if (!IsStaticSymbol<ForwardVariableDeclaration>(symbol))
						{
							targetTypeList = &varTypes;
						}
						break;
					case symbol_component::SymbolKind::Function:
						if (!IsStaticSymbol<ForwardFunctionDeclaration>(symbol))
						{
							targetTypeList = &funcTypes;
						}
						break;
					}
					break;
				}
			}

			VisitSymbol(pa, (targetTypeList == &result ? nullptr : thisItem), resolving->resolvedSymbols[i], false, *targetTypeList);
		}

		if (thisItem)
		{
			AddInternal(result, varTypes);

			TsysCV thisCv;
			TsysRefType thisRef;
			thisItem->tsys->GetEntity(thisCv, thisRef);
			FilterFieldsAndBestQualifiedFunctions(thisCv, thisRef, funcTypes);
			AddInternal(result, funcTypes);
		}
		else
		{
			AddInternal(result, varTypes);
			AddInternal(result, funcTypes);
		}
	}

	/***********************************************************************
	VisitDirectField: Find variables or qualified functions
	***********************************************************************/

	void VisitDirectField(const ParsingArguments& pa, ResolveSymbolResult& totalRar, const ExprTsysItem& parentItem, CppName& name, ExprTsysList& result)
	{
		TsysCV cv;
		TsysRefType refType;
		auto entity = parentItem.tsys->GetEntity(cv, refType);

		if (auto resolving = FindMembersByName(pa, name, &totalRar, parentItem))
		{
			VisitResolvedMember(pa, &parentItem, resolving, result);
		}
	}

	/***********************************************************************
	VisitFunctors: Find qualified functors (including functions and operator())
	***********************************************************************/

	void VisitFunctors(const ParsingArguments& pa, const ExprTsysItem& parentItem, const WString& name, ExprTsysList& result)
	{
		TsysCV cv;
		TsysRefType refType;
		parentItem.tsys->GetEntity(cv, refType);

		CppName opName;
		opName.name = name;
		if (auto resolving = FindMembersByName(pa, opName, nullptr, parentItem))
		{
			for (vint j = 0; j < resolving->resolvedSymbols.Count(); j++)
			{
				auto symbol = resolving->resolvedSymbols[j];
				VisitSymbol(pa, &parentItem, symbol, false, result);
			}
			FindQualifiedFunctors(pa, cv, refType, result, false);
		}
	}

	/***********************************************************************
	Promote: Integer promition
	***********************************************************************/

	void Promote(TsysPrimitive& primitive)
	{
		switch (primitive.type)
		{
		case TsysPrimitiveType::Bool: primitive.type = TsysPrimitiveType::SInt; break;
		case TsysPrimitiveType::SChar: primitive.type = TsysPrimitiveType::SInt; break;
		case TsysPrimitiveType::UChar: if (primitive.bytes < TsysBytes::_4) primitive.type = TsysPrimitiveType::UInt; break;
		case TsysPrimitiveType::UWChar: primitive.type = TsysPrimitiveType::UInt; break;
		}

		switch (primitive.type)
		{
		case TsysPrimitiveType::SInt:
			if (primitive.bytes < TsysBytes::_4)
			{
				primitive.bytes = TsysBytes::_4;
			}
			break;
		case TsysPrimitiveType::UInt:
			if (primitive.bytes < TsysBytes::_4)
			{
				primitive.type = TsysPrimitiveType::SInt;
				primitive.bytes = TsysBytes::_4;
			}
			break;
		}
	}

	/***********************************************************************
	ArithmeticConversion: Calculate the result type from two given primitive type
	***********************************************************************/

	bool FullyContain(TsysPrimitive large, TsysPrimitive small)
	{
		if (large.type == TsysPrimitiveType::Float && small.type != TsysPrimitiveType::Float)
		{
			return true;
		}

		if (large.type != TsysPrimitiveType::Float && small.type == TsysPrimitiveType::Float)
		{
			return false;
		}

		if (large.bytes <= small.bytes)
		{
			return false;
		}

		if (large.type == TsysPrimitiveType::Float && small.type == TsysPrimitiveType::Float)
		{
			return true;
		}

		bool sl = large.type == TsysPrimitiveType::SInt || large.type == TsysPrimitiveType::SChar;
		bool ss = small.type == TsysPrimitiveType::SInt || small.type == TsysPrimitiveType::SChar;

		return sl == ss || sl;
	}

	TsysPrimitive ArithmeticConversion(TsysPrimitive leftP, TsysPrimitive rightP)
	{
		if (FullyContain(leftP, rightP))
		{
			Promote(leftP);
			return leftP;
		}

		if (FullyContain(rightP, leftP))
		{
			Promote(rightP);
			return rightP;
		}

		Promote(leftP);
		Promote(rightP);

		TsysPrimitive primitive;
		primitive.bytes = leftP.bytes > rightP.bytes ? leftP.bytes : rightP.bytes;

		if (leftP.type == TsysPrimitiveType::Float || rightP.type == TsysPrimitiveType::Float)
		{
			primitive.type = TsysPrimitiveType::Float;
		}
		else if (leftP.bytes > rightP.bytes)
		{
			primitive.type = leftP.type;
		}
		else if (leftP.bytes < rightP.bytes)
		{
			primitive.type = rightP.type;
		}
		else
		{
			bool sl = leftP.type == TsysPrimitiveType::SInt || leftP.type == TsysPrimitiveType::SChar;
			bool sr = rightP.type == TsysPrimitiveType::SInt || rightP.type == TsysPrimitiveType::SChar;
			if (sl && !sr)
			{
				primitive.type = rightP.type;
			}
			else if (!sl && sr)
			{
				primitive.type = leftP.type;
			}
			else
			{
				primitive.type = leftP.type;
			}
		}

		return primitive;
	}
}