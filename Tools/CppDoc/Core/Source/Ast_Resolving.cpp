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
	VisitSymbol: Fill a symbol to ExprTsysList
		thisItem:
			it represents typeof(x) in x.name or typeof(*x) in x->name
			it could be null if it is initiated by IdExpr (visitMemberKind == InScope)
	***********************************************************************/

	enum class VisitMemberKind
	{
		MemberAfterType,
		MemberAfterValue,
		InScope,
	};

	void VisitSymbolInternalWithCorrectThisItem(const ParsingArguments& pa, const ExprTsysItem* thisItem, Symbol* symbol, VisitMemberKind visitMemberKind, ExprTsysList& result, bool allowVariadic, bool& hasVariadic, bool& hasNonVariadic)
	{
		Symbol* classScope = nullptr;
		if (auto parent = symbol->GetParentScope())
		{
			if (parent->GetImplDecl_NFb<ClassDeclaration>())
			{
				classScope = parent;
			}
		}

		ITsys* parentDeclType = nullptr;
		ITsys* thisEntity = nullptr;
		if (thisItem)
		{
			thisEntity = GetThisEntity(thisItem->tsys);
			if (thisEntity->GetType() == TsysType::DeclInstant)
			{
				parentDeclType = thisEntity;
			}
		}

		if (classScope && !thisEntity)
		{
			throw NotConvertableException();
		}

		switch (symbol->kind)
		{
		case symbol_component::SymbolKind::Variable:
			{
				auto varDecl = symbol->GetAnyForwardDecl<ForwardVariableDeclaration>();
				auto& evTypes = EvaluateVarSymbol(pa, varDecl.Obj(), parentDeclType);
				bool isStaticSymbol = IsStaticSymbol<ForwardVariableDeclaration>(symbol);

				for (vint k = 0; k < evTypes.Count(); k++)
				{
					auto tsys = evTypes[k];

					if (isStaticSymbol)
					{
						AddInternal(result, { symbol,ExprTsysType::LValue,tsys });
					}
					else
					{
						switch (visitMemberKind)
						{
						case VisitMemberKind::MemberAfterType:
							{
								if (classScope)
								{
									AddInternal(result, { symbol,ExprTsysType::PRValue,tsys->MemberOf(thisEntity) });
								}
								else
								{
									AddInternal(result, { symbol,ExprTsysType::LValue,tsys });
								}
							}
							break;
						case VisitMemberKind::MemberAfterValue:
							{
								CalculateValueFieldType(thisItem, symbol, tsys, false, result);
							}
							break;
						case VisitMemberKind::InScope:
							{
								AddInternal(result, { symbol,ExprTsysType::LValue,tsys });
							}
							break;
						}
					}
				}
				hasNonVariadic = true;
			}
			return;
		case symbol_component::SymbolKind::FunctionSymbol:
			{
				auto funcDecl = symbol->GetAnyForwardDecl<ForwardFunctionDeclaration>();
				auto& evTypes = EvaluateFuncSymbol(pa, funcDecl.Obj(), parentDeclType, nullptr);
				bool isStaticSymbol = IsStaticSymbol<ForwardFunctionDeclaration>(symbol);
				bool isMember = classScope && !isStaticSymbol;

				for (vint k = 0; k < evTypes.Count(); k++)
				{
					auto tsys = evTypes[k];

					if (isMember && visitMemberKind == VisitMemberKind::MemberAfterType)
					{
						tsys = tsys->MemberOf(thisEntity)->PtrOf();
					}
					else
					{
						tsys = tsys->PtrOf();
					}

					AddInternal(result, { symbol,ExprTsysType::PRValue,tsys });
				}
				hasNonVariadic = true;
			}
			return;
		case symbol_component::SymbolKind::EnumItem:
			{
				auto tsys = pa.tsys->DeclOf(symbol->GetParentScope());
				AddInternal(result, { symbol,ExprTsysType::PRValue,tsys });
				hasNonVariadic = true;
			}
			return;
		case symbol_component::SymbolKind::ValueAlias:
			{
				auto usingDecl = symbol->GetImplDecl_NFb<ValueAliasDeclaration>();
				auto& evTypes = EvaluateValueAliasSymbol(pa, usingDecl.Obj(), parentDeclType, nullptr);
				AddTemp(result, evTypes);
				hasNonVariadic = true;
			}
			return;
		case symbol_component::SymbolKind::GenericValueArgument:
			{
				if (symbol->ellipsis)
				{
					if (!allowVariadic)
					{
						throw NotConvertableException();
					}
					hasVariadic = true;

					auto argumentKey = pa.tsys->DeclOf(symbol);
					ITsys* replacedType = nullptr;
					if (pa.TryGetReplacedGenericArg(argumentKey, replacedType))
					{
						if (!replacedType)
						{
							throw NotConvertableException();
						}

						switch (replacedType->GetType())
						{
						case TsysType::Init:
							{
								TypeTsysList tsys;
								tsys.SetLessMemoryMode(false);

								Array<ExprTsysList> initArgs(replacedType->GetParamCount());
								for (vint j = 0; j < initArgs.Count(); j++)
								{
									AddTemp(initArgs[j], EvaluateGenericArgumentSymbol(symbol)->ReplaceGenericArgs(pa));
								}
								CreateUniversalInitializerType(pa, initArgs, result);
							}
							break;
						case TsysType::Any:
							AddTemp(result, pa.tsys->Any());
							break;
						default:
							throw NotConvertableException();
						}
					}
					else
					{
						AddTemp(result, pa.tsys->Any());
					}
				}
				else
				{
					AddTemp(result, EvaluateGenericArgumentSymbol(symbol)->ReplaceGenericArgs(pa));
					hasNonVariadic = true;
				}
			}
			return;
		}
		throw IllegalExprException();
	}

	void VisitSymbolInternal(const ParsingArguments& pa, const ExprTsysItem* thisItem, Symbol* symbol, VisitMemberKind visitMemberKind, ExprTsysList& result, bool allowVariadic, bool& hasVariadic, bool& hasNonVariadic)
	{
		switch (symbol->GetParentScope()->kind)
		{
		case symbol_component::SymbolKind::Class:
		case symbol_component::SymbolKind::Struct:
		case symbol_component::SymbolKind::Union:
			break;
		default:
			thisItem = nullptr;
		}

		if (thisItem)
		{
			auto thisType = ApplyExprTsysType(thisItem->tsys, thisItem->type);
			TsysCV thisCv;
			TsysRefType thisRef;
			auto entity = thisType->GetEntity(thisCv, thisRef);

			List<ITsys*> visiting;
			SortedList<ITsys*> visited;
			visiting.Add(entity);

			bool found = false;
			for (vint i = 0; i < visiting.Count(); i++)
			{
				auto currentThis = visiting[i];
				if (visited.Contains(currentThis))
				{
					continue;
				}
				visited.Add(currentThis);

				switch (currentThis->GetType())
				{
				case TsysType::Decl:
				case TsysType::DeclInstant:
					break;
				default:
					throw NotConvertableException();
				}

				auto thisSymbol = currentThis->GetDecl();
				if (symbol->GetParentScope() == thisSymbol)
				{
					found = true;
					ExprTsysItem baseItem(thisSymbol, (thisItem ? thisItem->type : ExprTsysType::PRValue), CvRefOf(currentThis, thisCv, thisRef));
					VisitSymbolInternalWithCorrectThisItem(pa, &baseItem, symbol, visitMemberKind, result, allowVariadic, hasVariadic, hasNonVariadic);
				}

				auto& ev = symbol_type_resolving::EvaluateClassType(pa, currentThis);
				for (vint j = 0; j < ev.ExtraCount(); j++)
				{
					CopyFrom(visiting, ev.GetExtra(j), true);
				}
			}

			if (!found)
			{
				throw NotConvertableException();
			}
		}
		else
		{
			VisitSymbolInternalWithCorrectThisItem(pa, nullptr, symbol, visitMemberKind, result, allowVariadic, hasVariadic, hasNonVariadic);
		}
	}

	void VisitSymbol(const ParsingArguments& pa, Symbol* symbol, ExprTsysList& result)
	{
		bool hasVariadic = false;
		bool hasNonVariadic = false;
		VisitSymbolInternal(pa, nullptr, symbol, VisitMemberKind::InScope, result, false, hasVariadic, hasNonVariadic);
	}

	void VisitSymbolForScope(const ParsingArguments& pa, const ExprTsysItem* thisItem, Symbol* symbol, ExprTsysList& result)
	{
		bool hasVariadic = false;
		bool hasNonVariadic = false;
		VisitSymbolInternal(pa, thisItem, symbol, VisitMemberKind::MemberAfterType, result, false, hasVariadic, hasNonVariadic);
	}

	void VisitSymbolForField(const ParsingArguments& pa, const ExprTsysItem* thisItem, Symbol* symbol, ExprTsysList& result)
	{
		bool hasVariadic = false;
		bool hasNonVariadic = false;
		VisitSymbolInternal(pa, thisItem, symbol, VisitMemberKind::MemberAfterValue, result, false, hasVariadic, hasNonVariadic);
	}

	/***********************************************************************
	FindMembersByName: Fill all members of a name to ExprTsysList
	***********************************************************************/

	Ptr<Resolving> FindMembersByName(const ParsingArguments& pa, CppName& name, ResolveSymbolResult* totalRar, const ExprTsysItem& parentItem)
	{
		TsysCV cv;
		TsysRefType refType;
		auto entity = parentItem.tsys->GetEntity(cv, refType);
		if (entity->GetType() == TsysType::Decl || entity->GetType() == TsysType::DeclInstant)
		{
			auto symbol = entity->GetDecl();
			auto fieldPa = pa.WithScope(symbol);
			auto rar = ResolveSymbol(fieldPa, name, SearchPolicy::ChildSymbolFromOutside);
			if (totalRar) totalRar->Merge(rar);
			return rar.values;
		}
		return nullptr;
	}

	/***********************************************************************
	VisitResolvedMember: Fill all resolved member symbol to ExprTsysList
	***********************************************************************/

	void VisitResolvedMemberInternal(const ParsingArguments& pa, const ExprTsysItem* thisItem, Ptr<Resolving> resolving, ExprTsysList& result, bool allowVariadic, bool& hasVariadic, bool& hasNonVariadic)
	{
		ExprTsysList varTypes, funcTypes;
		for (vint i = 0; i < resolving->resolvedSymbols.Count(); i++)
		{
			auto targetTypeList = &result;
			auto symbol = resolving->resolvedSymbols[i];
			
			bool parentScopeIsClass = false;
			if (auto parent = symbol->GetParentScope())
			{
				switch (parent->kind)
				{
				case symbol_component::SymbolKind::Class:
				case symbol_component::SymbolKind::Struct:
				case symbol_component::SymbolKind::Union:
					parentScopeIsClass = true;
					switch (symbol->kind)
					{
					case symbol_component::SymbolKind::Variable:
						if (!IsStaticSymbol<ForwardVariableDeclaration>(symbol))
						{
							targetTypeList = &varTypes;
						}
						break;
					case symbol_component::SymbolKind::FunctionSymbol:
						if (!IsStaticSymbol<ForwardFunctionDeclaration>(symbol))
						{
							targetTypeList = &funcTypes;
						}
						break;
					}
					break;
				}
			}

			auto thisItemForSymbol = parentScopeIsClass ? thisItem : nullptr;
			VisitSymbolInternal(pa, thisItemForSymbol, resolving->resolvedSymbols[i], (thisItemForSymbol ? VisitMemberKind::MemberAfterValue : VisitMemberKind::InScope), *targetTypeList, allowVariadic, hasVariadic, hasNonVariadic);
		}

		if (varTypes.Count() > 0)
		{
			// varTypes has non-static member fields
			AddInternal(result, varTypes);
		}

		if (funcTypes.Count() > 0)
		{
			if (thisItem)
			{
				// funcTypes has non-static member functions
				TsysCV thisCv;
				TsysRefType thisRef;
				thisItem->tsys->GetEntity(thisCv, thisRef);
				FilterFieldsAndBestQualifiedFunctions(thisCv, thisRef, funcTypes);
				AddInternal(result, funcTypes);
			}
			else
			{
				AddInternal(result, funcTypes);
			}
		}
	}

	void VisitResolvedMember(const ParsingArguments& pa, const ExprTsysItem* thisItem, Ptr<Resolving> resolving, ExprTsysList& result, bool& hasVariadic, bool& hasNonVariadic)
	{
		VisitResolvedMemberInternal(pa, thisItem, resolving, result, true, hasVariadic, hasNonVariadic);
	}

	void VisitResolvedMember(const ParsingArguments& pa, const ExprTsysItem* thisItem, Ptr<Resolving> resolving, ExprTsysList& result)
	{
		bool hasVariadic = false;
		bool hasNonVariadic = false;
		VisitResolvedMemberInternal(pa, thisItem, resolving, result, false, hasVariadic, hasNonVariadic);
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
				VisitSymbolForField(pa, &parentItem, symbol, result);
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