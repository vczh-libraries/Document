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

	template<typename TDecl, typename TForward>
	bool IsStaticSymbol(Symbol* symbol, Ptr<TForward> decl)
	{
		if (auto rootDecl = decl.Cast<TDecl>())
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
						if (IsStaticSymbol(forwardSymbol, forwardSymbol->decls[j].Cast<TForward>()))
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

	void VisitResolvable(ResolvableExpr* self, bool afterScope)
	{
		if (self->resolving)
		{
			for (vint i = 0; i < self->resolving->resolvedSymbols.Count(); i++)
			{
				auto symbol = self->resolving->resolvedSymbols[i];
				if (!symbol->forwardDeclarationRoot)
				{
					for (vint j = 0; j < symbol->decls.Count(); j++)
					{
						auto decl = symbol->decls[j];
						if (auto varDecl = decl.Cast<ForwardVariableDeclaration>())
						{
							bool isStaticSymbol = IsStaticSymbol<VariableDeclaration, ForwardVariableDeclaration>(symbol, varDecl);

							List<ITsys*> candidates;
							TypeToTsys(pa, varDecl->type, candidates);
							for (vint k = 0; k < candidates.Count(); k++)
							{
								auto tsys = candidates[k];
								if (tsys->GetType() == TsysType::Member)
								{
									tsys = tsys->GetElement();
								}
								tsys = tsys->LRefOf();
								if (!result.Contains(tsys))
								{
									result.Add(tsys);
								}
							}
						}
						else if (auto funcDecl = decl.Cast<ForwardFunctionDeclaration>())
						{
							bool isStaticSymbol = IsStaticSymbol<FunctionDeclaration, ForwardFunctionDeclaration>(symbol, varDecl);

							List<ITsys*> candidates;
							TypeToTsys(pa, varDecl->type, candidates);
							for (vint k = 0; k < candidates.Count(); k++)
							{
								auto tsys = candidates[k];
								if (tsys->GetType() == TsysType::Member)
								{
									tsys = tsys->GetElement();
								}
								tsys = tsys->PtrOf();
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
};

// Resolve expressions to types
void ExprToTsys(ParsingArguments& pa, Ptr<Expr> e, List<ITsys*>& tsys)
{
	if (!e) throw IllegalExprException();
	ExprToTsysVisitor visitor(pa, tsys);
	e->Accept(&visitor);
}