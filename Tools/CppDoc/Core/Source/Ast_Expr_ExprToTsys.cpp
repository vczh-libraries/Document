#include "Ast.h"
#include "Ast_Expr.h"
#include "Ast_Decl.h"
#include "Parser.h"

/***********************************************************************
ExprToTsys
***********************************************************************/

class ExprToTsysVisitor : public Object, public virtual IExprVisitor
{
public:
	List<ITsys*>&			result;
	ParsingArguments&		pa;

	ExprToTsysVisitor(ParsingArguments& _pa, List<ITsys*>& _result)
		:pa(_pa)
		, result(_result)
	{
	}

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

	static void VisitSymbol(ParsingArguments& pa, Symbol* symbol, bool afterScope, List<ITsys*>& result)
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

					List<ITsys*> candidates;
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
							tsys = tsys->LRefOf();
						}

						if (!result.Contains(tsys))
						{
							result.Add(tsys);
						}
					}
				}
				else if (auto funcDecl = decl.Cast<ForwardFunctionDeclaration>())
				{
					bool isStaticSymbol = IsStaticSymbol<ForwardFunctionDeclaration>(symbol, funcDecl);

					List<ITsys*> candidates;
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

						if (!result.Contains(tsys))
						{
							result.Add(tsys);
						}
					}
				}
				else
				{
					throw IllegalExprException();
				}
			}
		}
	}

	void VisitResolvable(ResolvableExpr* self, bool afterScope)
	{
		if (self->resolving)
		{
			for (vint i = 0; i < self->resolving->resolvedSymbols.Count(); i++)
			{
				VisitSymbol(pa, self->resolving->resolvedSymbols[i], afterScope, result);
			}
		}
	}

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

					result.Add(pa.tsys->Zero());
					return;
				}
			NOT_ZERO:
				wchar_t _1 = token.length > 1 ? token.reading[token.length - 2] : 0;
				wchar_t _2 = token.reading[token.length - 1];
				bool u = _1 == L'u' || _1 == L'U' || _2 == L'u' || _2 == L'U';
				bool l = _1 == L'l' || _1 == L'L' || _2 == L'l' || _2 == L'L';
				result.Add(pa.tsys->PrimitiveOf({ (u ? TsysPrimitiveType::UInt : TsysPrimitiveType::SInt),{l ? TsysBytes::_8 : TsysBytes::_4} }));
			}
			return;
		case CppTokens::FLOAT:
			{
				auto& token = self->tokens[0];
				wchar_t _1 = token.reading[token.length - 1];
				if (_1 == L'f' || _1 == L'F')
				{
					result.Add(pa.tsys->PrimitiveOf({ TsysPrimitiveType::Float, TsysBytes::_4 }));
				}
				else
				{
					result.Add(pa.tsys->PrimitiveOf({ TsysPrimitiveType::Float, TsysBytes::_8 }));
				}
			}
			return;
		case CppTokens::STRING:
		case CppTokens::CHAR:
			{
				ITsys* tsysChar = 0;
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
					result.Add(tsysChar);
				}
				else
				{
					result.Add(tsysChar->CVOf({ false,true,false })->ArrayOf(1));
				}
			}
			return;
		case CppTokens::EXPR_TRUE:
		case CppTokens::EXPR_FALSE:
			result.Add(pa.tsys->PrimitiveOf({ TsysPrimitiveType::Bool,TsysBytes::_1 }));
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
		result.Add(pa.tsys->Nullptr());
	}

	void Visit(ParenthesisExpr* self)override
	{
		throw 0;
	}

	void Visit(CastExpr* self)override
	{
		throw 0;
	}

	void Visit(TypeidExpr* self)override
	{
		throw 0;
	}

	void Visit(IdExpr* self)override
	{
		VisitResolvable(self, false);
	}

	void Visit(ChildExpr* self)override
	{
		VisitResolvable(self, true);
	}

	void VisitNormalField(CppName& name, ResolveSymbolResult& totalRar, TsysCV cv, TsysRefType refType, ITsys* entity)
	{
		if (entity->GetType() == TsysType::Decl)
		{
			auto symbol = entity->GetDecl();
			ParsingArguments fieldPa(pa, symbol);
			auto rar = ResolveSymbol(fieldPa, name, SearchPolicy::ChildSymbol);
			totalRar.Merge(rar);

			if (rar.values)
			{
				List<ITsys*> fieldTypes;
				for (vint j = 0; j < rar.values->resolvedSymbols.Count(); j++)
				{
					auto symbol = rar.values->resolvedSymbols[j];
					VisitSymbol(pa, symbol, false, fieldTypes);
				}

				for (vint j = 0; j < fieldTypes.Count(); j++)
				{
					auto tsys = fieldTypes[j]->CVOf(cv);
					if (!result.Contains(tsys))
					{
						result.Add(tsys);
					}
				}
			}
		}
	}

	void Visit(FieldAccessExpr* self)override
	{
		ResolveSymbolResult totalRar;
		List<ITsys*> parentTypes;
		ExprToTsys(pa, self->expr, parentTypes);

		if (self->type == CppFieldAccessType::Dot)
		{
			for (vint i = 0; i < parentTypes.Count(); i++)
			{
				TsysCV cv;
				TsysRefType refType;
				auto entity = parentTypes[i]->GetEntity(cv, refType);

				if (self->type == CppFieldAccessType::Dot)
				{
					VisitNormalField(self->name, totalRar, cv, refType, entity);
				}
			}
		}
		else
		{
			for (vint i = 0; i < parentTypes.Count(); i++)
			{
				TsysCV cv;
				TsysRefType refType;
				auto entity = parentTypes[i]->GetEntity(cv, refType);

				if (self->type == CppFieldAccessType::Arrow)
				{
					if (entity->GetType() == TsysType::Ptr)
					{
						entity = entity->GetElement()->GetEntity(cv, refType);
						VisitNormalField(self->name, totalRar, cv, refType, entity);
					}
					else if (entity->GetType() == TsysType::Decl)
					{
						throw 0;
					}
				}
			}
		}

		self->resolving = totalRar.values;
		if (totalRar.types && pa.recorder)
		{
			pa.recorder->ExpectValueButType(self->name, totalRar.types);
		}
	}

	void Visit(ArrayAccessExpr* self)override
	{
		throw 0;
	}

	void Visit(FuncAccessExpr* self)override
	{
		List<Ptr<List<ITsys*>>> argTypesList;
		for (vint i = 0; i < self->arguments.Count(); i++)
		{
			auto argTypes = MakePtr<List<ITsys*>>();
			ExprToTsys(pa, self->arguments[i], *argTypes.Obj());
			argTypesList.Add(argTypes);
		}

		if (self->type)
		{
			TypeToTsys(pa, self->type, result);
		}
		else if (self->expr)
		{
			List<ITsys*> funcTypes;
			ExprToTsys(pa, self->expr, funcTypes);

			for (vint i = funcTypes.Count() - 1; i >= 0; i--)
			{
				auto funcType = funcTypes[i];

				TsysCV cv;
				TsysRefType refType;
				auto entityType = funcType->GetEntity(cv, refType);

				if (entityType->GetType() == TsysType::Decl)
				{
					throw 0;
				}
				else if (entityType->GetType() == TsysType::Ptr)
				{
					entityType = entityType->GetElement();
					if (entityType->GetType() == TsysType::Function)
					{
						funcTypes[i] = entityType;
						continue;
					}
				}

				funcTypes.RemoveAt(i);
			}

			Array<TsysConv> funcChoices(funcTypes.Count());
			vint counters[2] = { 0,0 };

			for (vint i = 0; i < funcTypes.Count(); i++)
			{
				auto funcType = funcTypes[i];
				if (funcType->GetParamCount() == argTypesList.Count())
				{
					auto worstChoice = TsysConv::Direct;

					for (vint j = 0; j < argTypesList.Count(); j++)
					{
						auto paramType = funcType->GetParam(j);
						auto& argTypes = *argTypesList[j].Obj();
						auto bestChoice = TsysConv::Illegal;

						for (vint k = 0; k < argTypes.Count(); k++)
						{
							auto choice = paramType->TestParameter(argTypes[k]);
							if ((vint)bestChoice > (vint)choice) bestChoice = choice;
						}

						if ((vint)worstChoice < (vint)bestChoice) worstChoice = bestChoice;
					}

					funcChoices[i] = worstChoice;
				}
				else
				{
					funcChoices[i] = TsysConv::Illegal;
				}

				if (funcChoices[i] != TsysConv::Illegal)
				{
					counters[(vint)funcChoices[i]]++;
				}
			}

			for (vint i = 0; i < sizeof(counters) / sizeof(*counters); i++)
			{
				if (counters[i] > 0)
				{
					for (vint j = 0; j < funcTypes.Count(); j++)
					{
						if ((vint)funcChoices[j] == i)
						{
							result.Add(funcTypes[j]->GetElement());
						}
					}
					return;
				}
			}
		}
	}
};

// Resolve expressions to types
void ExprToTsys(ParsingArguments& pa, Ptr<Expr> e, List<ITsys*>& tsys)
{
	if (!e) throw IllegalExprException();
	ExprToTsysVisitor visitor(pa, tsys);
	e->Accept(&visitor);
}