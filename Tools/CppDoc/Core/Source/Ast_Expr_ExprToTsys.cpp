#include "Ast.h"
#include "Ast_Expr.h"
#include "Ast_Decl.h"
#include "Ast_Type.h"
#include "Parser.h"

/***********************************************************************
ExprToTsys
***********************************************************************/

class ExprToTsysVisitor : public Object, public virtual IExprVisitor
{
public:
	ExprTsysList&			result;
	ParsingArguments&		pa;

	ExprToTsysVisitor(ParsingArguments& _pa, ExprTsysList& _result)
		:pa(_pa)
		, result(_result)
	{
	}

	/***********************************************************************
	Add: Add something to ExprTsysList
	***********************************************************************/

	static bool Add(ExprTsysList& list, const ExprTsysItem& item)
	{
		if (list.Contains(item)) return false;
		list.Add(item);
		return true;
	}

	static void Add(ExprTsysList& list, ITsys* tsys, bool addRRef)
	{
		if (addRRef) tsys = tsys->RRefOf();
		Add(list, { nullptr,tsys });
	}

	static void Add(ExprTsysList& toList, ExprTsysList& fromList)
	{
		for (vint i = 0; i < fromList.Count(); i++)
		{
			Add(toList, fromList[i]);
		}
	}

	static void Add(ExprTsysList& toList, TypeTsysList& fromList, bool addRRef)
	{
		for (vint i = 0; i < fromList.Count(); i++)
		{
			Add(toList, fromList[i], addRRef);
		}
	}

	/***********************************************************************
	IsStaticSymbol: Test if a symbol is a static class member
	***********************************************************************/

	template<typename TForward>
	static bool IsStaticSymbol(Symbol* symbol, Ptr<TForward> decl)
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
	VisitSymbol: Fill a symbol to ExprTsysList
	***********************************************************************/

	static void VisitSymbol(ParsingArguments& pa, Symbol* symbol, bool afterScope, TsysCV addedCV, ExprTsysList& result)
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
					TypeToTsys(pa, varDecl->type, candidates);
					for (vint k = 0; k < candidates.Count(); k++)
					{
						auto tsys = candidates[k];
						if (tsys->GetType() == TsysType::Member && tsys->GetClass() == classScope)
						{
							tsys = tsys->GetElement();
						}

						if (classScope && !isStaticSymbol && afterScope)
						{
							tsys = tsys->MemberOf(classScope);
						}
						else
						{
							if (tsys->GetType() == TsysType::RRef)
							{
								tsys = tsys->GetElement()->LRefOf();
							}
							tsys = tsys->CVOf(addedCV);
						}

						Add(result, { symbol, tsys });
					}
				}
				else if (auto funcDecl = decl.Cast<ForwardFunctionDeclaration>())
				{
					bool isStaticSymbol = IsStaticSymbol<ForwardFunctionDeclaration>(symbol, funcDecl);

					TypeTsysList candidates;
					TypeToTsys(pa, funcDecl->type, candidates);
					for (vint k = 0; k < candidates.Count(); k++)
					{
						auto tsys = candidates[k];
						if (tsys->GetType() == TsysType::Member && tsys->GetClass() == classScope)
						{
							tsys = tsys->GetElement();
						}

						if (classScope && !isStaticSymbol && afterScope)
						{
							tsys = tsys->MemberOf(classScope)->PtrOf();
						}
						else
						{
							tsys = tsys->PtrOf();
						}

						Add(result, { symbol, tsys });
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
	VisitResolvable: Resolve ResolvableExpr
	***********************************************************************/

	void VisitResolvable(ResolvableExpr* self, bool afterScope)
	{
		if (self->resolving)
		{
			for (vint i = 0; i < self->resolving->resolvedSymbols.Count(); i++)
			{
				VisitSymbol(pa, self->resolving->resolvedSymbols[i], afterScope, {}, result);
			}
		}
	}

	/***********************************************************************
	VisitNormalField: Fill all members of a name to ExprTsysList
	***********************************************************************/

	static void VisitNormalField(ParsingArguments& pa, CppName& name, ResolveSymbolResult* totalRar, TsysCV cv, ITsys* entity, ExprTsysList& result)
	{
		if (entity->GetType() == TsysType::Decl)
		{
			auto symbol = entity->GetDecl();
			ParsingArguments fieldPa(pa, symbol);
			auto rar = ResolveSymbol(fieldPa, name, SearchPolicy::ChildSymbol);
			if (totalRar) totalRar->Merge(rar);

			if (rar.values)
			{
				for (vint j = 0; j < rar.values->resolvedSymbols.Count(); j++)
				{
					auto symbol = rar.values->resolvedSymbols[j];
					VisitSymbol(pa, symbol, false, cv, result);
				}
			}
		}
	}

	/***********************************************************************
	TestFunctionQualifier: Match this pointer's and functions' qualifiers
		Returns: Exact, TrivalConversion, Illegal
	***********************************************************************/

	static TsysConv TestFunctionQualifier(TsysCV thisCV, TsysRefType thisRef, const ExprTsysItem& funcType)
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
	FilterFunctionByQualifier: Filter functions by their qualifiers
	***********************************************************************/

	static void FilterFunctionByQualifier(ExprTsysList& funcTypes, ArrayBase<TsysConv>& funcChoices)
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

	static void FilterFunctionByQualifier(TsysCV thisCV, TsysRefType thisRef, ExprTsysList& funcTypes)
	{
		Array<TsysConv> funcChoices(funcTypes.Count());

		for (vint i = 0; i < funcTypes.Count(); i++)
		{
			funcChoices[i] = TestFunctionQualifier(thisCV, thisRef, funcTypes[i]);
		}

		FilterFunctionByQualifier(funcTypes, funcChoices);
	}

	/***********************************************************************
	FindQualifiedFunctions: Remove everything that are not qualified functions
	***********************************************************************/

	static void FindQualifiedFunctions(ParsingArguments& pa, TsysCV thisCV, TsysRefType thisRef, ExprTsysList& funcTypes, bool lookForOp)
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
					CppName opName;
					opName.name = L"operator ()";
					ExprTsysList opResult;
					VisitNormalField(pa, opName, nullptr, cv, entityType, opResult);
					FindQualifiedFunctions(pa, cv, refType, opResult, false);

					vint oldCount = expandedFuncTypes.Count();
					Add(expandedFuncTypes, opResult);
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
						if (Add(expandedFuncTypes, { funcType.symbol,entityType }))
						{
							funcChoices.Add(choice);
						}
					}
				}
			}
		}

		FilterFunctionByQualifier(expandedFuncTypes, funcChoices);
		CopyFrom(funcTypes, expandedFuncTypes);
	}

	/***********************************************************************
	VisitOverloadedFunction: Select good candidates from overloaded functions
	***********************************************************************/

	static void VisitOverloadedFunction(ParsingArguments& pa, ExprTsysList& funcTypes, List<Ptr<ExprTsysList>>& argTypesList, ExprTsysList& result)
	{
		Array<TsysConv> funcChoices(funcTypes.Count());

		for (vint i = 0; i < funcTypes.Count(); i++)
		{
			auto funcType = funcTypes[i];
			if (funcType.tsys->GetParamCount() == argTypesList.Count())
			{
				auto worstChoice = TsysConv::Exact;

				for (vint j = 0; j < argTypesList.Count(); j++)
				{
					auto paramType = funcType.tsys->GetParam(j);
					auto& argTypes = *argTypesList[j].Obj();
					auto bestChoice = TsysConv::Illegal;

					for (vint k = 0; k < argTypes.Count(); k++)
					{
						auto choice = TestConvert(pa, paramType, argTypes[k].tsys);
						if ((vint)bestChoice > (vint)choice) bestChoice = choice;
					}

					if (worstChoice < bestChoice) worstChoice = bestChoice;
				}

				funcChoices[i] = worstChoice;
			}
			else
			{
				funcChoices[i] = TsysConv::Illegal;
			}
		}

		FilterFunctionByQualifier(funcTypes, funcChoices);
		for (vint i = 0; i < funcTypes.Count(); i++)
		{
			Add(result, funcTypes[i].tsys->GetElement(), true);
		}
	}

	/***********************************************************************
	VisitDirectField: Find variables or qualified functions
	***********************************************************************/

	static void VisitDirectField(ParsingArguments& pa, ResolveSymbolResult& totalRar, ITsys* parentType, CppName& name, ExprTsysList& result)
	{
		TsysCV cv;
		TsysRefType refType;
		auto entity = parentType->GetEntity(cv, refType);

		ExprTsysList fieldResult;
		VisitNormalField(pa, name, &totalRar, cv, entity, fieldResult);
		FilterFunctionByQualifier(cv, refType, fieldResult);
		Add(result, fieldResult);
	}

	/***********************************************************************
	Expressions
	***********************************************************************/

	void Visit(LiteralExpr* self)override
	{
		switch ((CppTokens)self->tokens[0].token)
		{
		case CppTokens::INT:
		case CppTokens::HEX:
		case CppTokens::BIN:
			{
				auto& token = self->tokens[0];
				{
					auto reading = token.reading;
					auto end = token.reading + token.length;
					if (reading[0] == L'0')
					{
						switch (reading[1])
						{
						case L'x':
						case L'X':
						case L'b':
						case L'B':
							reading += 2;
						}
					}

					while (reading < end)
					{
						if (L'1' <= *reading && *reading <= L'9')
						{
							goto NOT_ZERO;
						}
						reading++;
					}

					Add(result, pa.tsys->Zero(), false);
					return;
				}
			NOT_ZERO:
				wchar_t _1 = token.length > 1 ? token.reading[token.length - 2] : 0;
				wchar_t _2 = token.reading[token.length - 1];
				bool u = _1 == L'u' || _1 == L'U' || _2 == L'u' || _2 == L'U';
				bool l = _1 == L'l' || _1 == L'L' || _2 == L'l' || _2 == L'L';
				Add(result, pa.tsys->PrimitiveOf({ (u ? TsysPrimitiveType::UInt : TsysPrimitiveType::SInt),{l ? TsysBytes::_8 : TsysBytes::_4} }), true);
			}
			return;
		case CppTokens::FLOAT:
			{
				auto& token = self->tokens[0];
				wchar_t _1 = token.reading[token.length - 1];
				if (_1 == L'f' || _1 == L'F')
				{
					Add(result, pa.tsys->PrimitiveOf({ TsysPrimitiveType::Float, TsysBytes::_4 }), true);
				}
				else
				{
					Add(result, pa.tsys->PrimitiveOf({ TsysPrimitiveType::Float, TsysBytes::_8 }), true);
				}
			}
			return;
		case CppTokens::STRING:
		case CppTokens::CHAR:
			{
				ITsys* tsysChar = nullptr;
				auto reading = self->tokens[0].reading;
				if (reading[0] == L'\"' || reading[0]==L'\'')
				{
					tsysChar = pa.tsys->PrimitiveOf({ TsysPrimitiveType::SChar,TsysBytes::_1 });
				}
				else if (reading[0] == L'L')
				{
					tsysChar = pa.tsys->PrimitiveOf({ TsysPrimitiveType::UWChar,TsysBytes::_2 });
				}
				else if (reading[0] == L'U')
				{
					tsysChar = pa.tsys->PrimitiveOf({ TsysPrimitiveType::UChar,TsysBytes::_4 });
				}
				else if (reading[0] == L'u')
				{
					if (reading[1] == L'8')
					{
						tsysChar = pa.tsys->PrimitiveOf({ TsysPrimitiveType::SChar,TsysBytes::_1 });
					}
					else
					{
						tsysChar = pa.tsys->PrimitiveOf({ TsysPrimitiveType::UChar,TsysBytes::_2 });
					}
				}

				if (!tsysChar)
				{
					throw IllegalExprException();
				}

				if ((CppTokens)self->tokens[0].token == CppTokens::CHAR)
				{
					Add(result, tsysChar, true);
				}
				else
				{
					Add(result, tsysChar->CVOf({ true,false })->ArrayOf(1), true);
				}
			}
			return;
		case CppTokens::EXPR_TRUE:
		case CppTokens::EXPR_FALSE:
			Add(result, pa.tsys->PrimitiveOf({ TsysPrimitiveType::Bool,TsysBytes::_1 }), true);
			return;
		}
		throw IllegalExprException();
	}

	void Visit(ThisExpr* self)override
	{
		throw 0;
	}

	void Visit(NullptrExpr* self)override
	{
		Add(result, pa.tsys->Nullptr(), false);
	}

	void Visit(ParenthesisExpr* self)override
	{
		self->expr->Accept(this);
	}

	void Visit(CastExpr* self)override
	{
		{
			ExprTsysList types;
			ExprToTsys(pa, self->expr, types);
		}
		{
			TypeTsysList types;
			TypeToTsys(pa, self->type, types);
			Add(result, types, true);
		}
	}

	void Visit(TypeidExpr* self)override
	{
		if (self->type)
		{
			TypeTsysList types;
			TypeToTsys(pa, self->type, types);
		}
		if (self->expr)
		{
			ExprTsysList types;
			ExprToTsys(pa, self->expr, types);
		}

		auto global = pa.root.Obj();
		vint index = global->children.Keys().IndexOf(L"std");
		if (index == -1) return;
		auto& stds = global->children.GetByIndex(index);
		if (stds.Count() != 1) return;
		index = stds[0]->children.Keys().IndexOf(L"type_info");
		if (index == -1) return;
		auto& tis = stds[0]->children.GetByIndex(index);

		for (vint i = 0; i < tis.Count(); i++)
		{
			auto ti = tis[i];
			if (ti->decls.Count() > 0)
			{
				if (ti->decls[0].Cast<ClassDeclaration>())
				{
					Add(result, { ti.Obj(),pa.tsys->DeclOf(ti.Obj()) });
					return;
				}
			}
		}
	}

	void Visit(SizeofExpr* self)override
	{
		if (self->type)
		{
			TypeTsysList types;
			TypeToTsys(pa, self->type, types);
		}
		if (self->expr)
		{
			ExprTsysList types;
			ExprToTsys(pa, self->expr, types);
		}

		// TODO: Platform Specific
		Add(result, pa.tsys->PrimitiveOf({ TsysPrimitiveType::UInt,TsysBytes::_4 }), false);
	}

	void Visit(ThrowExpr* self)override
	{
		if (self->expr)
		{
			ExprTsysList types;
			ExprToTsys(pa, self->expr, types);
		}

		Add(result, pa.tsys->PrimitiveOf({ TsysPrimitiveType::Void,TsysBytes::_1 }), false);
	}

	void Visit(NewExpr* self)override
	{
		for (vint i = 0; i < self->placementArguments.Count(); i++)
		{
			ExprTsysList types;
			ExprToTsys(pa, self->placementArguments[i], types);
		}
		for (vint i = 0; i < self->arguments.Count(); i++)
		{
			ExprTsysList types;
			ExprToTsys(pa, self->arguments[i], types);
		}

		TypeTsysList types;
		TypeToTsys(pa, self->type, types);
		for (vint i = 0; i < types.Count(); i++)
		{
			Add(result, types[i]->PtrOf(), true);
		}
	}

	void Visit(DeleteExpr* self)override
	{
		{
			ExprTsysList types;
			ExprToTsys(pa, self->expr, types);
		}

		Add(result, pa.tsys->PrimitiveOf({ TsysPrimitiveType::Void,TsysBytes::_1 }), false);
	}

	void Visit(IdExpr* self)override
	{
		VisitResolvable(self, false);
	}

	void Visit(ChildExpr* self)override
	{
		VisitResolvable(self, true);
	}

	void Visit(FieldAccessExpr* self)override
	{
		ResolveSymbolResult totalRar;
		ExprTsysList parentTypes;
		ExprToTsys(pa, self->expr, parentTypes);

		if (self->type == CppFieldAccessType::Dot)
		{
			for (vint i = 0; i < parentTypes.Count(); i++)
			{
				VisitDirectField(pa, totalRar, parentTypes[i].tsys, self->name, result);
			}
		}
		else
		{
			SortedList<ITsys*> visitedDecls;
			for (vint i = 0; i < parentTypes.Count(); i++)
			{
				TsysCV cv;
				TsysRefType refType;
				auto entityType = parentTypes[i].tsys->GetEntity(cv, refType);

				if (entityType->GetType() == TsysType::Ptr)
				{
					VisitDirectField(pa, totalRar, entityType->GetElement(), self->name, result);
				}
				else if (entityType->GetType() == TsysType::Decl)
				{
					if (!visitedDecls.Contains(entityType))
					{
						visitedDecls.Add(entityType);

						CppName opName;
						opName.name = L"operator ->";
						ExprTsysList opResult;
						VisitNormalField(pa, opName, nullptr, cv, entityType, opResult);
						FindQualifiedFunctions(pa, cv, refType, opResult, false);
						for (vint j = 0; j < opResult.Count(); j++)
						{
							auto item = opResult[j];
							item.tsys = item.tsys->GetElement()->RRefOf();
							Add(parentTypes, item);
						}
					}
				}
			}
		}

		self->resolving = totalRar.values;
		if (pa.recorder)
		{
			if (totalRar.values)
			{
				pa.recorder->Index(self->name, totalRar.values);
			}
			if (totalRar.types)
			{
				pa.recorder->ExpectValueButType(self->name, totalRar.types);
			}
		}
	}

	void Visit(ArrayAccessExpr* self)override
	{
		List<Ptr<ExprTsysList>> argTypesList;
		{
			auto argTypes = MakePtr<ExprTsysList>();
			ExprToTsys(pa, self->index, *argTypes.Obj());
			argTypesList.Add(argTypes);
		}

		ExprTsysList arrayTypes, funcTypes;
		ExprToTsys(pa, self->expr, arrayTypes);

		for (vint i = 0; i < arrayTypes.Count(); i++)
		{
			auto arrayType = arrayTypes[i];

			TsysCV cv;
			TsysRefType refType;
			auto entityType = arrayType.tsys->GetEntity(cv, refType);

			if (entityType->GetType() == TsysType::Decl)
			{
				CppName opName;
				opName.name = L"operator []";
				ExprTsysList opResult;
				VisitNormalField(pa, opName, nullptr, cv, entityType, opResult);
				FindQualifiedFunctions(pa, cv, refType, opResult, false);
				Add(funcTypes, opResult);
			}
			else if (entityType->GetType() == TsysType::Array)
			{
				Add(result, entityType->GetElement(), false);
			}
			else if (entityType->GetType() == TsysType::Ptr)
			{
				Add(result, entityType->GetElement(), false);
			}
		}

		VisitOverloadedFunction(pa, funcTypes, argTypesList, result);
	}

	void Visit(FuncAccessExpr* self)override
	{
		List<Ptr<ExprTsysList>> argTypesList;
		for (vint i = 0; i < self->arguments.Count(); i++)
		{
			auto argTypes = MakePtr<ExprTsysList>();
			ExprToTsys(pa, self->arguments[i], *argTypes.Obj());
			argTypesList.Add(argTypes);
		}

		if (self->type)
		{
			TypeTsysList types;
			TypeToTsys(pa, self->type, types);
			Add(result, types, true);
		}
		else if (self->expr)
		{
			ExprTsysList funcTypes;
			ExprToTsys(pa, self->expr, funcTypes);

			FindQualifiedFunctions(pa, {}, TsysRefType::None, funcTypes, true);
			VisitOverloadedFunction(pa, funcTypes, argTypesList, result);
		}
	}

	void Visit(PostfixUnaryExpr* self)override
	{
		ExprTsysList types;
		ExprToTsys(pa, self->operand, types);
		for (vint i = 0; i < types.Count(); i++)
		{
			auto type = types[i].tsys;
			TsysCV cv;
			TsysRefType refType;
			auto entity = type->GetEntity(cv, refType);

			if (entity->GetType()==TsysType::Primitive)
			{
				auto primitive = entity->GetPrimitive();
				switch (primitive.type)
				{
				case TsysPrimitiveType::Bool:
					Add(result, type->LRefOf(), false);
					break;
				default:
					Add(result, type, false);
				}
			}
			else
			{
				throw 0;
			}
		}
	}

	void Visit(PrefixUnaryExpr* self)override
	{
		ExprTsysList types;
		ExprToTsys(pa, self->operand, types);
		for (vint i = 0; i < types.Count(); i++)
		{
			auto type = types[i].tsys;
			TsysCV cv;
			TsysRefType refType;
			auto entity = type->GetEntity(cv, refType);

			if (entity->GetType() == TsysType::Primitive)
			{
				switch (self->op)
				{
				case CppPrefixUnaryOp::Increase:
				case CppPrefixUnaryOp::Decrease:
					Add(result, type->LRefOf(), false);
					break;
				case CppPrefixUnaryOp::Revert:
				case CppPrefixUnaryOp::Positive:
				case CppPrefixUnaryOp::Negative:
					{
						auto primitive = entity->GetPrimitive();
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

						Add(result, pa.tsys->PrimitiveOf(primitive)->CVOf(cv), true);
					}
					break;
				case CppPrefixUnaryOp::Not:
					Add(result, pa.tsys->PrimitiveOf({ TsysPrimitiveType::Bool, TsysBytes::_1 }), true);
					break;
				case CppPrefixUnaryOp::AddressOf:
					if (type->GetType() == TsysType::LRef)
					{
						Add(result, type->GetElement()->PtrOf(), true);
					}
					else
					{
						Add(result, type->PtrOf(), true);
					}
					break;
				case CppPrefixUnaryOp::Dereference:
					throw 0;
				}
			}
			else
			{
				throw 0;
			}
		}
	}

	void Visit(BinaryExpr* self)override
	{
		ExprTsysList leftTypes, rightTypes;
		ExprToTsys(pa, self->left, leftTypes);
		ExprToTsys(pa, self->right, rightTypes);


		for (vint i = 0; i < leftTypes.Count(); i++)
		{
			auto leftType = leftTypes[i].tsys;
			TsysCV leftCV;
			TsysRefType leftRefType;
			auto leftEntity = leftType->GetEntity(leftCV, leftRefType);

			for (vint j = 0; j < rightTypes.Count(); j++)
			{
				auto rightType = rightTypes[j].tsys;
				TsysCV rightCV;
				TsysRefType rightRefType;
				auto rightEntity = rightType->GetEntity(rightCV, rightRefType);

				if (leftEntity->GetType() == TsysType::Primitive && rightEntity->GetType() == TsysType::Primitive)
				{
					switch (self->op)
					{
					case CppBinaryOp::LT:
					case CppBinaryOp::GT:
					case CppBinaryOp::LE:
					case CppBinaryOp::GE:
					case CppBinaryOp::EQ:
					case CppBinaryOp::NE:
						Add(result, pa.tsys->PrimitiveOf({ TsysPrimitiveType::Bool,TsysBytes::_1 }), true);
						break;
					case CppBinaryOp::Assign:
					case CppBinaryOp::MulAssign:
					case CppBinaryOp::DivAssign:
					case CppBinaryOp::ModAssign:
					case CppBinaryOp::AddAssign:
					case CppBinaryOp::SubAddisn:
					case CppBinaryOp::ShlAssign:
					case CppBinaryOp::ShrAssign:
					case CppBinaryOp::AndAssign:
					case CppBinaryOp::OrAssign:
					case CppBinaryOp::XorAssign:
						Add(result, leftType->LRefOf(), false);
						break;
					case CppBinaryOp::Comma:
						Add(result, rightTypes);
						break;
					default:
						throw 0;
					}
				}
				else
				{
					throw 0;
				}
			}
		}
	}

	void Visit(IfExpr* self)override
	{
		{
			ExprTsysList types;
			ExprToTsys(pa, self->condition, types);
		}
		ExprToTsys(pa, self->left, result);
		ExprToTsys(pa, self->right, result);
	}
};

// Resolve expressions to types
void ExprToTsys(ParsingArguments& pa, Ptr<Expr> e, ExprTsysList& tsys)
{
	if (!e) throw IllegalExprException();
	ExprToTsysVisitor visitor(pa, tsys);
	e->Accept(&visitor);
}