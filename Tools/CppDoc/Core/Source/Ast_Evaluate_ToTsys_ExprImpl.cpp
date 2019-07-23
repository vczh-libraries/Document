#include "Ast_Resolving_ExpandPotentialVta.h"

using namespace symbol_type_resolving;

namespace symbol_totsys_impl
{
	//////////////////////////////////////////////////////////////////////////////////////
	// ProcessLiteralExpr
	//////////////////////////////////////////////////////////////////////////////////////
	
	void ProcessLiteralExpr(const ParsingArguments& pa, ExprTsysList& result, GenericArgContext* gaContext, LiteralExpr* self)
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

					AddTemp(result, pa.tsys->Zero());
					return;
				}
			NOT_ZERO:
#define COUNT_CHAR(NUM, UC, LC) ((_##NUM == UC || _##NUM == LC) ? 1 : 0)
#define COUNT_U(NUM) COUNT_CHAR(NUM, L'u', L'U')
#define COUNT_L(NUM) COUNT_CHAR(NUM, L'l', L'L')
				wchar_t _1 = token.length > 2 ? token.reading[token.length - 3] : 0;
				wchar_t _2 = token.length > 1 ? token.reading[token.length - 2] : 0;
				wchar_t _3 = token.reading[token.length - 1];
				vint us = COUNT_U(1) + COUNT_U(2) + COUNT_U(3);
				vint ls = COUNT_L(1) + COUNT_L(2) + COUNT_L(3);
				AddTemp(result, pa.tsys->PrimitiveOf({ (us > 0 ? TsysPrimitiveType::UInt : TsysPrimitiveType::SInt),{ls > 1 ? TsysBytes::_8 : TsysBytes::_4} }));
#undef COUNT_CHAR
#undef COUNT_U
#undef COUNT_L
			}
			return;
		case CppTokens::FLOAT:
			{
				auto& token = self->tokens[0];
				wchar_t _1 = token.reading[token.length - 1];
				if (_1 == L'f' || _1 == L'F')
				{
					AddTemp(result, pa.tsys->PrimitiveOf({ TsysPrimitiveType::Float, TsysBytes::_4 }));
				}
				else
				{
					AddTemp(result, pa.tsys->PrimitiveOf({ TsysPrimitiveType::Float, TsysBytes::_8 }));
				}
			}
			return;
		case CppTokens::STRING:
		case CppTokens::CHAR:
			{
				ITsys* tsysChar = nullptr;
				auto reading = self->tokens[0].reading;
				if (reading[0] == L'\"' || reading[0] == L'\'')
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
					AddTemp(result, tsysChar);
				}
				else
				{
					AddTemp(result, tsysChar->CVOf({ true,false })->ArrayOf(1)->LRefOf());
				}
			}
			return;
		case CppTokens::EXPR_TRUE:
		case CppTokens::EXPR_FALSE:
			AddTemp(result, pa.tsys->PrimitiveOf({ TsysPrimitiveType::Bool,TsysBytes::_1 }));
			return;
		}
		throw IllegalExprException();
	}

	//////////////////////////////////////////////////////////////////////////////////////
	// ProcessThisExpr
	//////////////////////////////////////////////////////////////////////////////////////

	void ProcessThisExpr(const ParsingArguments& pa, ExprTsysList& result, GenericArgContext* gaContext, ThisExpr* self)
	{
		if (auto funcSymbol = pa.funcSymbol)
		{
			if (auto methodCache = funcSymbol->methodCache)
			{
				if (auto thisType = methodCache->thisType)
				{
					AddTemp(result, thisType);
					return;
				}
			}
		}
		throw IllegalExprException();
	}

	//////////////////////////////////////////////////////////////////////////////////////
	// ProcessNullptrExpr
	//////////////////////////////////////////////////////////////////////////////////////
	
	void ProcessNullptrExpr(const ParsingArguments& pa, ExprTsysList& result, GenericArgContext* gaContext, NullptrExpr* self)
	{
		AddTemp(result, pa.tsys->Nullptr());
	}

	//////////////////////////////////////////////////////////////////////////////////////
	// ProcessTypeidExpr
	//////////////////////////////////////////////////////////////////////////////////////
	
	void ProcessTypeidExpr(const ParsingArguments& pa, ExprTsysList& result, GenericArgContext* gaContext, TypeidExpr* self)
	{
		if (self->type)
		{
			TypeTsysList types;
			TypeToTsysNoVta(pa, self->type, types, gaContext);
		}
		if (self->expr)
		{
			ExprTsysList types;
			ExprToTsys(pa, self->expr, types, gaContext);
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
			switch (ti->kind)
			{
			case symbol_component::SymbolKind::Class:
			case symbol_component::SymbolKind::Struct:
				AddInternal(result, { nullptr,ExprTsysType::LValue,pa.tsys->DeclOf(ti.Obj()) });
				return;
			}
		}
	}

	//////////////////////////////////////////////////////////////////////////////////////
	// ProcessSizeofExpr
	//////////////////////////////////////////////////////////////////////////////////////
	
	void ProcessSizeofExpr(const ParsingArguments& pa, ExprTsysList& result, GenericArgContext* gaContext, SizeofExpr* self)
	{
		if (self->type)
		{
			TypeTsysList types;
			bool typeIsVta = false;
			TypeToTsysInternal(pa, self->type, types, gaContext, typeIsVta);
		}
		if (self->expr)
		{
			ExprTsysList types;
			ExprToTsys(pa, self->expr, types, gaContext);
		}

		AddTemp(result, pa.tsys->Size());
	}

	//////////////////////////////////////////////////////////////////////////////////////
	// ProcessThrowExpr
	//////////////////////////////////////////////////////////////////////////////////////
	
	void ProcessThrowExpr(const ParsingArguments& pa, ExprTsysList& result, GenericArgContext* gaContext, ThrowExpr* self)
	{
		if (self->expr)
		{
			ExprTsysList types;
			ExprToTsys(pa, self->expr, types, gaContext);
		}

		AddTemp(result, pa.tsys->Void());
	}

	//////////////////////////////////////////////////////////////////////////////////////
	// ProcessDeleteExpr
	//////////////////////////////////////////////////////////////////////////////////////
	
	void ProcessDeleteExpr(const ParsingArguments& pa, ExprTsysList& result, GenericArgContext* gaContext, DeleteExpr* self)
	{
		{
			ExprTsysList types;
			ExprToTsys(pa, self->expr, types, gaContext);
		}

		AddTemp(result, pa.tsys->Void());
	}

	//////////////////////////////////////////////////////////////////////////////////////
	// ProcessParenthesisExpr
	//////////////////////////////////////////////////////////////////////////////////////

	void ProcessParenthesisExpr(const ParsingArguments& pa, ExprTsysList& result, ParenthesisExpr* self, ExprTsysItem arg)
	{
		if (arg.type == ExprTsysType::LValue)
		{
			AddTemp(result, arg.tsys->LRefOf());
		}
		else
		{
			AddTemp(result, arg.tsys);
		}
	}

	//////////////////////////////////////////////////////////////////////////////////////
	// ProcessParenthesisExpr
	//////////////////////////////////////////////////////////////////////////////////////

	void ProcessCastExpr(const ParsingArguments& pa, ExprTsysList& result, CastExpr* self, ExprTsysItem argType, ExprTsysItem argExpr)
	{
		AddTemp(result, argType.tsys);
	}

	//////////////////////////////////////////////////////////////////////////////////////
	// Indexing
	//////////////////////////////////////////////////////////////////////////////////////

	bool IsOperator(Symbol* symbol)
	{
		if (auto funcDecl = symbol->GetAnyForwardDecl<ForwardFunctionDeclaration>())
		{
			return funcDecl->name.type == CppNameType::Operator;
		}
		return false;
	}

	void AddSymbolToResolving(Ptr<Resolving>* resolving, Symbol* symbol, bool& first)
	{
		if (first)
		{
			*resolving = MakePtr<Resolving>();
		}
		first = false;

		auto& resolvedSymbols = (*resolving)->resolvedSymbols;
		if (!resolvedSymbols.Contains(symbol))
		{
			resolvedSymbols.Add(symbol);
		}
	}

	bool AddSymbolToNameResolving(const CppName& name, Ptr<Resolving>* resolving, Symbol* symbol, bool& first)
	{
		bool isOperator = IsOperator(symbol);
		if (!isOperator || name.type == CppNameType::Operator)
		{
			AddSymbolToResolving(resolving, symbol, first);
			return true;
		}
		return false;
	}

	bool AddSymbolToOperatorResolving(const CppName& name, Ptr<Resolving>* resolving, Symbol* symbol, bool& first)
	{
		bool isOperator = IsOperator(symbol);
		if (isOperator)
		{
			AddSymbolToResolving(resolving, symbol, first);
			return true;
		}
		return false;
	}

	void AddSymbolsToResolvings(GenericArgContext* gaContext, const CppName* name, Ptr<Resolving>* nameResolving, const CppName* op, Ptr<Resolving>* opResolving, ExprTsysList& symbols, bool& addedName, bool& addedOp)
	{
		if (gaContext) return;
		bool firstName = true;
		bool firstOp = true;

		for (vint i = 0; i < symbols.Count(); i++)
		{
			if (auto symbol = symbols[i].symbol)
			{
				if (name && AddSymbolToNameResolving(*name, nameResolving, symbol, firstName))
				{
					addedName = true;
					continue;
				}

				if (op && AddSymbolToOperatorResolving(*op, opResolving, symbol, firstOp))
				{
					addedOp = true;
				}
			}
		}
	}

	void AddSymbolsToOperatorResolving(GenericArgContext* gaContext, const CppName& op, Ptr<Resolving>& opResolving, ExprTsysList& symbols, bool& addedOp)
	{
		bool addedName = false;
		AddSymbolsToResolvings(gaContext, nullptr, nullptr, &op, &opResolving, symbols, addedName, addedOp);
	}
}