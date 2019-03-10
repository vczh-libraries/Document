#include "Parser.h"
#include "Ast_Decl.h"
#include "Ast_Expr.h"

namespace symbol_component
{
/***********************************************************************
Evaluation
***********************************************************************/

	void Evaluation::Allocate(vint count)
	{
		typeList.Clear();
		for (vint i = 0; i < count; i++)
		{
			typeList.Add(MakePtr<TypeTsysList>());
		}
	}

	void Evaluation::Clear()
	{
		typeList.Clear();
	}

	vint Evaluation::Count()
	{
		return typeList.Count();
	}

	TypeTsysList& Evaluation::Get(vint index)
	{
		return *typeList[index].Obj();
	}
}

/***********************************************************************
Symbol
***********************************************************************/

Symbol* Symbol::CreateSymbolInternal(Ptr<Declaration> _decl, Symbol* existingSymbol, symbol_component::SymbolKind kind)
{
	if (!existingSymbol)
	{
		auto symbol = MakePtr<Symbol>();
		symbol->name = _decl->name.name;
		symbol->kind = kind;
		Add(symbol);
		existingSymbol = symbol.Obj();
	}

	_decl->symbol = existingSymbol;
	return existingSymbol;
}

Symbol* Symbol::AddToSymbolInternal(Ptr<Declaration> _decl, symbol_component::SymbolKind kind)
{
	vint index = children.Keys().IndexOf(_decl->name.name);
	if (index == -1)
	{
		return CreateSymbolInternal(_decl, nullptr, kind);
	}
	else
	{
		auto& symbols = children.GetByIndex(index);
		if (symbols.Count() != 1) return nullptr;
		auto symbol = symbols[0].Obj();
		if (symbol->kind != kind) return nullptr;
		_decl->symbol = symbol;
		return symbol;
	}
}

void Symbol::Add(Ptr<Symbol> child)
{
	child->parent = this;
	children.Add(child->name, child);
}

Symbol* Symbol::CreateForwardDeclSymbol(Ptr<Declaration> _decl, Symbol* existingSymbol, symbol_component::SymbolKind kind)
{
	existingSymbol = CreateSymbolInternal(_decl, existingSymbol, kind);
	existingSymbol->declarations.Add(_decl);
	return existingSymbol;
}

Symbol* Symbol::CreateDeclSymbol(Ptr<Declaration> _decl, Symbol* existingSymbol, symbol_component::SymbolKind kind)
{
	existingSymbol = CreateSymbolInternal(_decl, existingSymbol, kind);
	existingSymbol->definition = _decl;
	return existingSymbol;
}

Symbol* Symbol::AddForwardDeclToSymbol(Ptr<Declaration> _decl, symbol_component::SymbolKind kind)
{
	auto symbol = AddToSymbolInternal(_decl, kind);
	if (!symbol) return nullptr;
	symbol->declarations.Add(_decl);
	return symbol;
}

Symbol* Symbol::AddDeclToSymbol(Ptr<Declaration> _decl, symbol_component::SymbolKind kind)
{
	auto symbol = AddToSymbolInternal(_decl, kind);
	if (!symbol) return nullptr;
	if (symbol->definition) return nullptr;
	symbol->definition = _decl;
	return symbol;
}

Symbol* Symbol::CreateStatSymbol(Ptr<Stat> _stat)
{
	auto symbol = MakePtr<Symbol>();
	symbol->name = L"$";
	symbol->kind = symbol_component::SymbolKind::Statement;
	symbol->statement = _stat;
	Add(symbol);

	_stat->symbol = symbol.Obj();
	return symbol.Obj();
}

/***********************************************************************
ParsingArguments
***********************************************************************/

ParsingArguments::ParsingArguments()
{
}

ParsingArguments::ParsingArguments(Ptr<Symbol> _root, Ptr<ITsysAlloc> _tsys, Ptr<IIndexRecorder> _recorder)
	:root(_root)
	, context(_root.Obj())
	, tsys(_tsys)
	, recorder(_recorder)
{
}

ParsingArguments ParsingArguments::WithContext(Symbol* _context)const
{
	ParsingArguments pa(root, tsys, recorder);
	pa.program = program;
	pa.context = _context;

	while (_context)
	{
		if (_context == context)
		{
			pa.funcSymbol = funcSymbol;
			break;
		}
		else if (_context->kind == symbol_component::SymbolKind::Function)
		{
			pa.funcSymbol = _context;
			break;
		}
		else
		{
			_context = _context->parent;
		}
	}

	return pa;
}

/***********************************************************************
ParsingArguments
***********************************************************************/

class ProcessDelayParseDeclarationVisitor : public Object, public IDeclarationVisitor
{
public:
	void Visit(ForwardVariableDeclaration* self) override
	{
	}

	void Visit(ForwardFunctionDeclaration* self) override
	{
	}

	void Visit(ForwardEnumDeclaration* self) override
	{
	}

	void Visit(ForwardClassDeclaration* self) override
	{
	}

	void Visit(VariableDeclaration* self) override
	{
	}

	void Visit(FunctionDeclaration* self) override
	{
		EnsureFunctionBodyParsed(self);
	}

	void Visit(EnumItemDeclaration* self) override
	{
	}

	void Visit(EnumDeclaration* self) override
	{
	}

	void Visit(ClassDeclaration* self) override
	{
		for (vint i = 0; i < self->decls.Count(); i++)
		{
			self->decls[i].f1->Accept(this);
		}
	}

	void Visit(NestedAnonymousClassDeclaration* self) override
	{
		for (vint i = 0; i < self->decls.Count(); i++)
		{
			self->decls[i]->Accept(this);
		}
	}

	void Visit(UsingNamespaceDeclaration* self) override
	{
	}

	void Visit(UsingSymbolDeclaration* self) override
	{
	}

	void Visit(UsingDeclaration* self) override
	{
	}

	void Visit(NamespaceDeclaration* self) override
	{
		for (vint i = 0; i < self->decls.Count(); i++)
		{
			self->decls[i]->Accept(this);
		}
	}
};

void EnsureFunctionBodyParsed(FunctionDeclaration* funcDecl)
{
	if (funcDecl->delayParse)
	{
		auto delayParse = funcDecl->delayParse;
		funcDecl->delayParse = nullptr;

		if (TestToken(delayParse->begin, CppTokens::COLON))
		{
			while (true)
			{
				FunctionDeclaration::InitItem item;
				item.f0 = MakePtr<IdExpr>();
				if (!ParseCppName(item.f0->name, delayParse->begin))
				{
					throw StopParsingException(delayParse->begin);
				}
				if (item.f0->name.type != CppNameType::Normal)
				{
					throw StopParsingException(delayParse->begin);
				}

				RequireToken(delayParse->begin, CppTokens::LPARENTHESIS);
				item.f1 = ParseExpr(delayParse->pa, true, delayParse->begin);
				RequireToken(delayParse->begin, CppTokens::RPARENTHESIS);

				funcDecl->initList.Add(item);

				if (!TestToken(delayParse->begin, CppTokens::COMMA))
				{
					if (TestToken(delayParse->begin, CppTokens::LBRACE, false))
					{
						break;
					}
					else
					{
						throw StopParsingException(delayParse->begin);
					}
				}
			}
		}

		funcDecl->statement = ParseStat(delayParse->pa, delayParse->begin);
		if (delayParse->begin)
		{
			if (delayParse->end.reading != delayParse->begin->token.reading)
			{
				throw StopParsingException(delayParse->begin);
			}
		}
		else
		{
			if (delayParse->end.reading != nullptr)
			{
				throw StopParsingException(delayParse->begin);
			}
		}
	}
}

Ptr<Program> ParseProgram(ParsingArguments& pa, Ptr<CppTokenCursor>& cursor)
{
	auto program = MakePtr<Program>();
	pa.program = program;
	while (cursor)
	{
		ParseDeclaration(pa, cursor, program->decls);
	}

	ProcessDelayParseDeclarationVisitor visitor;
	for (vint i = 0; i < program->decls.Count(); i++)
	{
		program->decls[i]->Accept(&visitor);
	}
	return program;
}