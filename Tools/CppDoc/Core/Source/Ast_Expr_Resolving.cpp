#include "Ast_Expr_Resolving.h"

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
	IsStaticSymbol: Test if a symbol is a class member
	***********************************************************************/

	template<typename TForward>
	bool IsStaticSymbol(Symbol* symbol, Ptr<TForward> decl)
	{
		if (auto rootDecl = decl.Cast<typename TForward::ForwardRootType>())
		{
			if (rootDecl->decoratorStatic)
			{
				return true;
			}
			else
			{
				for (vint i = 0; i < symbol->forwardDeclarations.Count(); i++)
				{
					auto forwardSymbol = symbol->forwardDeclarations[i];
					for (vint j = 0; j < forwardSymbol->decls.Count(); j++)
					{
						if (IsStaticSymbol<TForward>(forwardSymbol, forwardSymbol->decls[j].Cast<TForward>()))
						{
							return true;
						}
					}
				}
				return false;
			}
		}
		else
		{
			return decl->decoratorStatic;
		}
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
	VisitSymbol: Fill a symbol to ExprTsysList
		thisItem: When afterScope==false
			it represents typeof(x) in x.name or typeof(&x) in x->name
			it could be null if it is initiated by IdExpr
	***********************************************************************/

	void VisitSymbol(ParsingArguments& pa, const ExprTsysItem* thisItem, Symbol* symbol, bool afterScope, ExprTsysList& result)
	{
		ITsys* classScope = nullptr;
		if (symbol->parent && symbol->parent->decls.Count() > 0)
		{
			if (auto decl = symbol->parent->decls[0].Cast<ClassDeclaration>())
			{
				classScope = pa.tsys->DeclOf(symbol->parent);
			}
		}

		if (!symbol->forwardDeclarationRoot)
		{
			for (vint j = 0; j < symbol->decls.Count(); j++)
			{
				auto decl = symbol->decls[j];
				if (auto varDecl = decl.Cast<ForwardVariableDeclaration>())
				{
					bool isStaticSymbol = IsStaticSymbol<ForwardVariableDeclaration>(symbol, varDecl);

					TypeTsysList candidates;
					if (varDecl->needResolveTypeFromInitializer)
					{
						if (!symbol->resolvedTypes)
						{
							symbol->resolvedTypes = MakePtr<TypeTsysList>();
							if (auto rootVarDecl = varDecl.Cast<VariableDeclaration>())
							{
								auto declType = MakePtr<DeclType>();
								declType->expr = rootVarDecl->initializer->arguments[0];

								TypeTsysList types;
								ParsingArguments newPa(pa, symbol->parent);
								TypeToTsys(newPa, declType, types);

								for (vint k = 0; k < types.Count(); k++)
								{
									auto type = types[k];

									if (auto primitiveType = varDecl->type.Cast<PrimitiveType>())
									{
										TsysCV cv;
										TsysRefType refType;
										type = type->GetEntity(cv, refType);
									}

									if (!symbol->resolvedTypes->Contains(type))
									{
										symbol->resolvedTypes->Add(type);
									}
								}
							}
						}
						CopyFrom(candidates, *symbol->resolvedTypes.Obj());
					}
					else
					{
						TypeToTsys(pa, varDecl->type, candidates);
					}

					for (vint k = 0; k < candidates.Count(); k++)
					{
						auto tsys = candidates[k];
						if (tsys->GetType() == TsysType::Member && tsys->GetClass() == classScope)
						{
							tsys = tsys->GetElement();
						}

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
				else if (auto enumItemDecl = decl.Cast<EnumItemDeclaration>())
				{
					auto tsys = pa.tsys->DeclOf(enumItemDecl->symbol->parent);
					AddInternal(result, { symbol,ExprTsysType::PRValue,tsys });
				}
				else if (auto funcDecl = decl.Cast<ForwardFunctionDeclaration>())
				{
					bool isStaticSymbol = IsStaticSymbol<ForwardFunctionDeclaration>(symbol, funcDecl);
					bool isMember = classScope && !isStaticSymbol;

					TypeTsysList candidates;
					if (funcDecl->needResolveTypeFromStatement)
					{
						throw 0;
					}
					else
					{
						TypeToTsys(pa, funcDecl->type, candidates, TsysCallingConvention::None, isMember);
					}

					for (vint k = 0; k < candidates.Count(); k++)
					{
						auto tsys = candidates[k];
						if (tsys->GetType() == TsysType::Member && tsys->GetClass() == classScope)
						{
							tsys = tsys->GetElement();
						}

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
				else
				{
					throw IllegalExprException();
				}
			}
		}
	}

	/***********************************************************************
	FindMembersByName: Fill all members of a name to ExprTsysList
	***********************************************************************/

	Ptr<Resolving> FindMembersByName(ParsingArguments& pa, CppName& name, ResolveSymbolResult* totalRar, const ExprTsysItem& parentItem)
	{
		TsysCV cv;
		TsysRefType refType;
		auto entity = parentItem.tsys->GetEntity(cv, refType);
		if (entity->GetType() == TsysType::Decl)
		{
			auto symbol = entity->GetDecl();
			ParsingArguments fieldPa(pa, symbol);
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
		if (funcType.symbol && funcType.symbol->decls.Count() > 0)
		{
			if (auto decl = funcType.symbol->decls[0].Cast<ForwardFunctionDeclaration>())
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
			if (target > candidate)
			{
				target = candidate;
			}
		}
		return target;
	}

	void FilterFunctionByConv(ExprTsysList& funcTypes, ArrayBase<TsysConv>& funcChoices)
	{
		auto target = FindMinConv(funcChoices);
		if (target == TsysConv::Illegal)
		{
			funcTypes.Clear();
			return;
		}

		for (vint i = funcTypes.Count() - 1; i >= 0; i--)
		{
			if (funcChoices[i] != target)
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

	void FindQualifiedFunctors(ParsingArguments& pa, TsysCV thisCV, TsysRefType thisRef, ExprTsysList& funcTypes, bool lookForOp)
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

	void VisitResolvedMember(ParsingArguments& pa, const ExprTsysItem* thisItem, Ptr<Resolving> resolving, ExprTsysList& result)
	{
		ExprTsysList varTypes, funcTypes;
		for (vint i = 0; i < resolving->resolvedSymbols.Count(); i++)
		{
			auto targetTypeList = &result;
			auto symbol = resolving->resolvedSymbols[i];
			if (symbol->decls.Count() == 1)
			{
				if (symbol->parent && symbol->parent->decls.Count() == 1 && symbol->parent->decls[0].Cast<ClassDeclaration>())
				{
					if (auto varDecl = symbol->decls[0].Cast<ForwardVariableDeclaration>())
					{
						if (!varDecl->decoratorStatic)
						{
							targetTypeList = &varTypes;
						}
					}
					else if (auto funcDecl = symbol->decls[0].Cast<ForwardFunctionDeclaration>())
					{
						if (!funcDecl->decoratorStatic)
						{
							targetTypeList = &funcTypes;
						}
					}
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

	void VisitDirectField(ParsingArguments& pa, ResolveSymbolResult& totalRar, const ExprTsysItem& parentItem, CppName& name, ExprTsysList& result)
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

	void VisitFunctors(ParsingArguments& pa, const ExprTsysItem& parentItem, const WString& name, ExprTsysList& result)
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
	VisitOverloadedFunction: Select good candidates from overloaded functions
	***********************************************************************/

	void VisitOverloadedFunction(ParsingArguments& pa, ExprTsysList& funcTypes, List<Ptr<ExprTsysList>>& argTypesList, ExprTsysList& result)
	{
		Array<vint> funcDPs(funcTypes.Count());
		for (vint i = 0; i < funcTypes.Count(); i++)
		{
			funcDPs[i] = 0;
			if (auto symbol = funcTypes[i].symbol)
			{
				if (symbol->decls.Count() == 1)
				{
					if (auto decl = symbol->decls[0].Cast<ForwardFunctionDeclaration>())
					{
						if (auto type = GetTypeWithoutMemberAndCC(decl->type).Cast<FunctionType>())
						{
							for (vint j = 0; j < type->parameters.Count(); j++)
							{
								if (type->parameters[j]->initializer)
								{
									funcDPs[i] = type->parameters.Count() - j;
									goto EXAMINE_NEXT_FUNCTION;
								}
							}
						EXAMINE_NEXT_FUNCTION:;
						}
					}
				}
			}
		}

		ExprTsysList validFuncTypes;
		for (vint i = 0; i < funcTypes.Count(); i++)
		{
			auto funcType = funcTypes[i];

			vint funcParamCount = funcType.tsys->GetParamCount();
			vint missParamCount = funcParamCount - argTypesList.Count();
			if (missParamCount > 0)
			{
				if (missParamCount > funcDPs[i])
				{
					continue;
				}
			}
			else if (missParamCount < 0)
			{
				if (!funcType.tsys->GetFunc().ellipsis)
				{
					continue;
				}
			}

			validFuncTypes.Add(funcType);
		}

		Array<bool> selectedIndices(validFuncTypes.Count());
		for (vint i = 0; i < validFuncTypes.Count(); i++)
		{
			selectedIndices[i] = true;
		}

		for (vint i = 0; i < argTypesList.Count(); i++)
		{
			Array<TsysConv> funcChoices(validFuncTypes.Count());

			for (vint j = 0; j < validFuncTypes.Count(); j++)
			{
				auto funcType = validFuncTypes[j];

				vint funcParamCount = funcType.tsys->GetParamCount();
				if (funcParamCount <= i)
				{
					funcChoices[j] = TsysConv::Ellipsis;
					continue;
				}

				auto paramType = funcType.tsys->GetParam(i);
				auto& argTypes = *argTypesList[i].Obj();

				auto bestChoice = TsysConv::Illegal;
				for (vint k = 0; k < argTypes.Count(); k++)
				{
					auto choice = TestConvert(pa, paramType, argTypes[k]);
					if ((vint)bestChoice > (vint)choice) bestChoice = choice;
				}
				funcChoices[j] = bestChoice;
			}

			auto min = FindMinConv(funcChoices);
			if (min == TsysConv::Illegal)
			{
				return;
			}

			for (vint j = 0; j < validFuncTypes.Count(); j++)
			{
				if (funcChoices[j] != min)
				{
					selectedIndices[j] = false;
				}
			}
		}

		for (vint i = 0; i < selectedIndices.Count(); i++)
		{
			if (selectedIndices[i])
			{
				AddTemp(result, validFuncTypes[i].tsys->GetElement());
			}
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