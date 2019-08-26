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

/***********************************************************************
SC_Data
***********************************************************************/

	SC_Data::SC_Data(SymbolCategory category)
	{
	}

	SC_Data::~SC_Data()
	{
	}

	void SC_Data::Alloc(SymbolCategory category)
	{
#ifdef VCZH_CHECK_MEMORY_LEAKS_NEW
#undef new
#endif
		switch (category)
		{
		case symbol_component::SymbolCategory::Normal:
			new(&normal) SC_Normal();
			break;
		case symbol_component::SymbolCategory::Function:
			new(&function) SC_Function();
			break;
		case symbol_component::SymbolCategory::FunctionBody:
			new(&functionBody) SC_FunctionBody();
			break;
		case symbol_component::SymbolCategory::Undecided:
			new(&undecided) SC_Undecided();
			break;
		}
#ifdef VCZH_CHECK_MEMORY_LEAKS_NEW
#define new VCZH_CHECK_MEMORY_LEAKS_NEW
#endif
	}

	void SC_Data::Free(SymbolCategory category)
	{
		switch (category)
		{
		case symbol_component::SymbolCategory::Normal:
			normal.~SC_Normal();
			break;
		case symbol_component::SymbolCategory::Function:
			function.~SC_Function();
			break;
		case symbol_component::SymbolCategory::FunctionBody:
			functionBody.~SC_FunctionBody();
			break;
		case symbol_component::SymbolCategory::Undecided:
			undecided.~SC_Undecided();
			break;
		}
	}
}

/***********************************************************************
Symbol
***********************************************************************/

Symbol* Symbol::CreateSymbolInternal_NFFb(Ptr<Declaration> _decl, symbol_component::SymbolKind _kind, symbol_component::SymbolCategory _category)
{
	switch (_category)
	{
	case symbol_component::SymbolCategory::Normal:
	case symbol_component::SymbolCategory::Function:
	case symbol_component::SymbolCategory::Undecided:
		if (category != symbol_component::SymbolCategory::Normal && category != symbol_component::SymbolCategory::FunctionBody)
		{
			throw UnexpectedSymbolCategoryException();
		}
		break;
	case symbol_component::SymbolCategory::FunctionBody:
		if (category != symbol_component::SymbolCategory::Function)
		{
			throw UnexpectedSymbolCategoryException();
		}
		break;
	}

	auto symbol = MakePtr<Symbol>(_category);
	symbol->name = _decl->name.name;
	symbol->kind = kind;

	if (category == symbol_component::SymbolCategory::Function)
	{
		if (_decl.Cast<FunctionDeclaration>())
		{
			categoryData.function.implSymbols.Add(symbol);
		}
		else
		{
			categoryData.function.declSymbols.Add(symbol);
		}
		symbol->categoryData.functionBody.functionSymbol = this;
	}
	else
	{
		AddChildAndSetParent_NFb(symbol->name, symbol);
	}

	_decl->symbol = symbol.Obj();
	return symbol.Obj();
}

Symbol* Symbol::AddToSymbolInternal_NFb(Ptr<Declaration> _decl, symbol_component::SymbolKind kind, Ptr<Symbol> templateSpecSymbol, symbol_component::SymbolCategory _category)
{
	if (auto pChildren = TryGetChildren_NFb(_decl->name.name))
	{
		if (templateSpecSymbol)
		{
			return nullptr;
		}

		if (pChildren->Count() != 1) return nullptr;
		auto symbol = pChildren->Get(0).Obj();
		if (symbol->kind != kind) return nullptr;
		_decl->symbol = symbol;
		return symbol;
	}
	else
	{
		if (templateSpecSymbol)
		{
			templateSpecSymbol->SetCategory(_category);
			templateSpecSymbol->name = _decl->name.name;
			templateSpecSymbol->kind = kind;
			AddChildAndSetParent_NFb(templateSpecSymbol->name, templateSpecSymbol);
			_decl->symbol = templateSpecSymbol.Obj();
			return templateSpecSymbol.Obj();
		}
		else
		{
			return CreateSymbolInternal_NFFb(_decl, kind, symbol_component::SymbolCategory::Normal);
		}
	}
}

void Symbol::SetParent(Symbol* parent)
{
	switch (category)
	{
	case symbol_component::SymbolCategory::Normal:
		categoryData.normal.parent = parent;
		break;
	case symbol_component::SymbolCategory::Function:
		categoryData.function.parent = parent;
		break;
	case symbol_component::SymbolCategory::Undecided:
		categoryData.undecided.parent = parent;
		break;
	default:
		throw UnexpectedSymbolCategoryException();
	}
}

Symbol::Symbol(symbol_component::SymbolCategory _category, Symbol* _parent)
	:category(_category)
	, categoryData(_category)
{
	if (_parent)
	{
		SetParent(_parent);
	}
}

Symbol::~Symbol()
{
	categoryData.Free(category);
}

symbol_component::SymbolCategory Symbol::GetCategory()
{
	return category;
}

void Symbol::SetCategory(symbol_component::SymbolCategory _category)
{
	categoryData.Free(category);
	category = _category;
	categoryData.Alloc(category);
}

Symbol* Symbol::GetParentScope()
{
	switch (category)
	{
	case symbol_component::SymbolCategory::Normal:
		return categoryData.normal.parent;
	case symbol_component::SymbolCategory::Function:
		return categoryData.function.parent;
	case symbol_component::SymbolCategory::FunctionBody:
		return categoryData.functionBody.functionSymbol->GetParentScope();
	case symbol_component::SymbolCategory::Undecided:
		return categoryData.undecided.parent;
	default:
		throw UnexpectedSymbolCategoryException();
	}
}

const List<Ptr<Symbol>>& Symbol::GetImplSymbols_F()
{
	switch (category)
	{
	case symbol_component::SymbolCategory::Function:
		return categoryData.function.implSymbols;
	default:
		throw UnexpectedSymbolCategoryException();
	}
}
const List<Ptr<Symbol>>& Symbol::GetDeclSymbols_F()
{
	switch (category)
	{
	case symbol_component::SymbolCategory::Function:
		return categoryData.function.declSymbols;
	default:
		throw UnexpectedSymbolCategoryException();
	}
}

Ptr<Declaration> Symbol::GetImplDecl_NFb()
{
	switch (category)
	{
	case symbol_component::SymbolCategory::Normal:
		return categoryData.normal.implDecl;
	case symbol_component::SymbolCategory::FunctionBody:
		return categoryData.functionBody.implDecl;
	default:
		throw UnexpectedSymbolCategoryException();
	}
}

Ptr<Declaration> Symbol::GetForwardDecl_Fb()
{
	switch (category)
	{
	case symbol_component::SymbolCategory::FunctionBody:
		return categoryData.functionBody.forwardDecl;
	default:
		throw UnexpectedSymbolCategoryException();
	}
}

const List<Ptr<Declaration>>& Symbol::GetForwardDecls_N()
{
	switch (category)
	{
	case symbol_component::SymbolCategory::Normal:
		return categoryData.normal.forwardDecls;
	default:
		throw UnexpectedSymbolCategoryException();
	}
}

const Ptr<Stat>& Symbol::GetStat_N()
{
	switch (category)
	{
	case symbol_component::SymbolCategory::Normal:
		return categoryData.normal.statement;
	default:
		throw UnexpectedSymbolCategoryException();
	}
}

Ptr<symbol_component::MethodCache> Symbol::GetMethodCache_Fb()
{
	switch (category)
	{
	case symbol_component::SymbolCategory::FunctionBody:
		return categoryData.functionBody.methodCache;
	default:
		throw UnexpectedSymbolCategoryException();
	}
}

symbol_component::Evaluation& Symbol::GetEvaluationForUpdating_NFb()
{
	switch (category)
	{
	case symbol_component::SymbolCategory::Normal:
		return categoryData.normal.evaluation;
	case symbol_component::SymbolCategory::FunctionBody:
		return categoryData.functionBody.evaluation;
	default:
		throw UnexpectedSymbolCategoryException();
	}
}

const symbol_component::SymbolGroup& Symbol::GetChildren_NFb()
{
	switch (category)
	{
	case symbol_component::SymbolCategory::Normal:
		return categoryData.normal.children;
	case symbol_component::SymbolCategory::FunctionBody:
		return categoryData.functionBody.children;
	default:
		throw UnexpectedSymbolCategoryException();
	}
}

const List<Ptr<Symbol>>* Symbol::TryGetChildren_NFb(const WString& name)
{
	const auto& children = GetChildren_NFb();
	vint index = children.Keys().IndexOf(name);
	if (index == -1) return nullptr;
	return &children.GetByIndex(index);
}

void Symbol::AddChild_NFb(const WString& name, const Ptr<Symbol>& child)
{
	auto& children = const_cast<symbol_component::SymbolGroup&>(GetChildren_NFb());
	children.Add(name, child);
}

void Symbol::AddChildAndSetParent_NFb(const WString& name, const Ptr<Symbol>& child)
{
	AddChild_NFb(name, child);
	child->SetParent(this);
}

void Symbol::RemoveChildAndResetParent_NFb(const WString& name, Symbol* child)
{
	auto& children = const_cast<symbol_component::SymbolGroup&>(GetChildren_NFb());
	children.Remove(name, child);
	child->SetParent(nullptr);
}

Symbol* Symbol::CreateFunctionSymbol_NFb(Ptr<ForwardFunctionDeclaration> _decl)
{
	auto symbol = MakePtr<Symbol>(symbol_component::SymbolCategory::Function);
	symbol->kind = symbol_component::SymbolKind::FunctionSymbol;
	symbol->name = _decl->name.name;
	AddChildAndSetParent_NFb(symbol->name, symbol);
	return symbol.Obj();
}

Symbol* Symbol::CreateFunctionForwardSymbol_F(Ptr<ForwardFunctionDeclaration> _decl, symbol_component::SymbolKind kind)
{
	auto symbol = CreateSymbolInternal_NFFb(_decl, kind, symbol_component::SymbolCategory::FunctionBody);
	symbol->categoryData.functionBody.forwardDecl = _decl;
	return symbol;
}

Symbol* Symbol::CreateFunctionImplSymbol_F(Ptr<FunctionDeclaration> _decl, symbol_component::SymbolKind kind, Ptr<symbol_component::MethodCache> methodCache)
{
	auto symbol = CreateSymbolInternal_NFFb(_decl, kind, symbol_component::SymbolCategory::FunctionBody);
	symbol->categoryData.functionBody.implDecl = _decl;
	if (methodCache)
	{
		symbol->categoryData.functionBody.methodCache = methodCache;
		methodCache->funcSymbol = symbol;
	}
	return symbol;
}

Symbol* Symbol::AddForwardDeclToSymbol_NFb(Ptr<Declaration> _decl, symbol_component::SymbolKind kind)
{
	auto symbol = AddToSymbolInternal_NFb(_decl, kind, nullptr, symbol_component::SymbolCategory::Normal);
	if (!symbol) return nullptr;
	if (symbol->categoryData.normal.implDecl) return nullptr;
	if (symbol->categoryData.normal.forwardDecls.Count() > 0) return nullptr;
	symbol->categoryData.normal.forwardDecls.Add(_decl);
	return symbol;
}

Symbol* Symbol::AddImplDeclToSymbol_NFb(Ptr<Declaration> _decl, symbol_component::SymbolKind kind, Ptr<Symbol> templateSpecSymbol)
{
	auto symbol = AddToSymbolInternal_NFb(_decl, kind, templateSpecSymbol, symbol_component::SymbolCategory::Normal);
	if (!symbol) return nullptr;
	if (symbol->categoryData.normal.implDecl) return nullptr;
	if (symbol->categoryData.normal.forwardDecls.Count() > 0) return nullptr;
	symbol->categoryData.normal.implDecl = _decl;
	return symbol;
}

Symbol* Symbol::CreateStatSymbol_NFb(Ptr<Stat> _stat)
{
	auto symbol = MakePtr<Symbol>(symbol_component::SymbolCategory::Normal);
	symbol->name = L"$";
	symbol->kind = symbol_component::SymbolKind::Statement;
	symbol->categoryData.normal.statement = _stat;
	AddChildAndSetParent_NFb(symbol->name, symbol);

	_stat->symbol = symbol.Obj();
	return symbol.Obj();
}

void Symbol::GenerateUniqueId(Dictionary<WString, Symbol*>& ids, const WString& prefix)
{
	if (uniqueId != L"")
	{
		throw UnexpectedSymbolCategoryException();
	}

	WString nextPrefix;

	if (category == symbol_component::SymbolCategory::FunctionBody)
	{
		uniqueId = prefix;
		nextPrefix = prefix + L"::";
	}
	else
	{
		switch (kind)
		{
		case symbol_component::SymbolKind::Root:
		case symbol_component::SymbolKind::Statement:
			nextPrefix = prefix;
			break;
		default:
			{
				vint counter = 1;
				while (true)
				{
					WString id = prefix + name + (counter == 1 ? WString::Empty : itow(counter));
					if (!ids.Keys().Contains(id))
					{
						uniqueId = id;
						ids.Add(id, this);
						nextPrefix = id + L"::";
						break;
					}
					counter++;
				}
			}
		}
	}

	switch (category)
	{
	case symbol_component::SymbolCategory::Normal:
	case symbol_component::SymbolCategory::FunctionBody:
		{
			const auto& children = GetChildren_NFb();
			for (vint i = 0; i < children.Count(); i++)
			{
				auto& symbols = children.GetByIndex(i);
				for (vint j = 0; j < symbols.Count(); j++)
				{
					auto symbol = symbols[j];
					// skip some copied enum item symbols
					if (symbol->GetParentScope() == this)
					{
						symbol->GenerateUniqueId(ids, nextPrefix);
					}
				}
			}
		}
		break;
	case symbol_component::SymbolCategory::Function:
		for (vint i = 0; i < categoryData.function.declSymbols.Count(); i++)
		{
			categoryData.function.declSymbols[i]->uniqueId = uniqueId;
		}
		for (vint i = 0; i < categoryData.function.implSymbols.Count(); i++)
		{
			auto symbol = categoryData.function.implSymbols[i].Obj();
			symbol->GenerateUniqueId(ids, uniqueId + L"[decl" + itow(i) + L"]");
		}
		return;
	default:
		throw UnexpectedSymbolCategoryException();
	}
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
			_context = _context->GetParentScope();
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

	void Visit(TypeAliasDeclaration* self) override
	{
	}

	void Visit(ValueAliasDeclaration* self) override
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
				item.f1 = ParseExpr(delayParse->pa, pea_Full(), delayParse->begin);
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

void PredefineType(Ptr<Program> program, const ParsingArguments& pa, const wchar_t* name, bool isForwardDeclaration, CppClassType classType, symbol_component::SymbolKind symbolKind)
{
	auto decl = isForwardDeclaration ? MakePtr<ForwardClassDeclaration>() : (Ptr<ForwardClassDeclaration>)MakePtr<ClassDeclaration>();
	decl->classType = classType;
	decl->name.name = name;
	program->decls.Insert(program->createdForwardDeclByCStyleTypeReference++, decl);
	if (isForwardDeclaration)
	{
		pa.root->AddForwardDeclToSymbol_NFb(decl, symbolKind);
	}
	else
	{
		pa.root->AddImplDeclToSymbol_NFb(decl, symbolKind);
	}
}

bool ParseTypeOrExpr(const ParsingArguments& pa, const ParsingExprArguments& pea, Ptr<CppTokenCursor>& cursor, Ptr<Type>& type, Ptr<Expr>& expr)
{
	auto oldCursor = cursor;
	try
	{
		expr = ParseExpr(pa, pea, cursor);
		return false;
	}
	catch (const StopParsingException&)
	{
	}
	cursor = oldCursor;
	type = ParseType(pa, cursor);
	return true;
}

Ptr<Program> ParseProgram(ParsingArguments& pa, Ptr<CppTokenCursor>& cursor)
{
	auto program = MakePtr<Program>();
	pa.program = program;

	// these types will be used before it is defined
	PredefineType(program, pa, L"__m64", false, CppClassType::Struct, symbol_component::SymbolKind::Struct);
	PredefineType(program, pa, L"__m128", false, CppClassType::Union, symbol_component::SymbolKind::Struct);
	PredefineType(program, pa, L"__m128d", true, CppClassType::Struct, symbol_component::SymbolKind::Struct);
	PredefineType(program, pa, L"__m128i", true, CppClassType::Union, symbol_component::SymbolKind::Union);
	PredefineType(program, pa, L"__m256", true, CppClassType::Union, symbol_component::SymbolKind::Union);
	PredefineType(program, pa, L"__m256d", true, CppClassType::Struct, symbol_component::SymbolKind::Struct);
	PredefineType(program, pa, L"__m256i", true, CppClassType::Union, symbol_component::SymbolKind::Union);
	PredefineType(program, pa, L"__m512", true, CppClassType::Union, symbol_component::SymbolKind::Union);
	PredefineType(program, pa, L"__m512d", true, CppClassType::Struct, symbol_component::SymbolKind::Struct);
	PredefineType(program, pa, L"__m512i", true, CppClassType::Union, symbol_component::SymbolKind::Union);

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