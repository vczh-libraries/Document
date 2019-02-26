#include "Parser.h"
#include "Ast_Decl.h"

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

void Symbol::Add(Ptr<Symbol> child)
{
	child->parent = this;
	children.Add(child->name, child);
}

Symbol* Symbol::CreateForwardDeclSymbol(Ptr<Declaration> _decl, Symbol* existingSymbol, symbol_component::SymbolKind kind)
{
	auto symbol = CreateSymbolInternal(_decl, existingSymbol, kind);
	symbol->definitions.Add(_decl);
	return symbol;
}

Symbol* Symbol::CreateDeclSymbol(Ptr<Declaration> _decl, Symbol* existingSymbol, symbol_component::SymbolKind kind)
{
	auto symbol = CreateSymbolInternal(_decl, existingSymbol, kind);
	existingSymbol->declaration = _decl;
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

ParsingArguments ParsingArguments::WithContext(Symbol* context)const
{
	ParsingArguments pa(root, tsys, recorder);
	pa.funcSymbol = funcSymbol;
	pa.context = context;
	return pa;
}

ParsingArguments ParsingArguments::WithContextNoFunction(Symbol* context)const
{
	ParsingArguments pa(root, tsys, recorder);
	pa.funcSymbol = nullptr;
	pa.context = context;
	return pa;
}

ParsingArguments ParsingArguments::WithContextAndFunction(Symbol* context, Symbol* newFuncSymbol)const
{
	ParsingArguments pa(root, tsys, recorder);
	pa.funcSymbol = newFuncSymbol;
	pa.context = context;
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

	void Visit(TypeAliasDeclaration* self) override
	{
	}

	void Visit(UsingNamespaceDeclaration* self) override
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

		funcDecl->statement = ParseStat(delayParse->pa, delayParse->begin);
		if (delayParse->begin)
		{
			if (delayParse->end.reading != delayParse->begin->token.reading)
			{
				throw StopParsingException();
			}
		}
		else
		{
			if (delayParse->end.reading != nullptr)
			{
				throw StopParsingException();
			}
		}
	}
}

Ptr<Program> ParseProgram(const ParsingArguments& pa, Ptr<CppTokenCursor>& cursor)
{
	auto program = MakePtr<Program>();
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