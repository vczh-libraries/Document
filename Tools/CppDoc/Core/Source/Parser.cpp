#include "Parser.h"
#include "Ast_Decl.h"
#include "Ast_Expr.h"
#include "Ast_Resolving.h"

namespace symbol_component
{
/***********************************************************************
Evaluation
***********************************************************************/

	void Evaluation::Allocate()
	{
		mainTypeList = MakePtr<TypeTsysList>();
	}

	void Evaluation::AllocateExtra(vint count)
	{
		extraTypeLists.Clear();
		for (vint i = 0; i < count; i++)
		{
			extraTypeLists.Add(MakePtr<TypeTsysList>());
		}
	}

	void Evaluation::Clear()
	{
		mainTypeList = nullptr;
		extraTypeLists.Clear();
	}

	vint Evaluation::ExtraCount()
	{
		return extraTypeLists.Count();
	}

	TypeTsysList& Evaluation::Get()
	{
		return *mainTypeList.Obj();
	}

	TypeTsysList& Evaluation::GetExtra(vint index)
	{
		return *extraTypeLists[index].Obj();
	}

/***********************************************************************
SC_Data
***********************************************************************/

	SC_Data::SC_Data(SymbolCategory category)
	{
		Alloc(category);
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
		default:
			throw UnexpectedSymbolCategoryException();
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
		default:
			throw UnexpectedSymbolCategoryException();
		}
	}
}

/***********************************************************************
Symbol
***********************************************************************/

void CopySymbolChildren(Symbol* symbol, List<Ptr<Symbol>>& existingChildren)
{
	const auto& children = symbol->GetChildren_NFb();
	for (vint i = 0; i < children.Count(); i++)
	{
		CopyFrom(existingChildren, children.GetByIndex(i), true);
	}
}

void AddSymbolChildren(Symbol* symbol, List<Ptr<Symbol>>& existingChildren)
{
	for (vint i = 0; i < existingChildren.Count(); i++)
	{
		auto child = existingChildren[i];
		symbol->AddChildAndSetParent_NFb(child->name, child);
	}
}

WString Symbol::DecorateNameForSpecializationSpec(const WString& symbolName, Ptr<SpecializationSpec> spec)
{
	// TODO: [Cpp.md] Choose a good name for specializationSpec, no counter so that the name is stable
	vint i = 1;
	while (true)
	{
		auto name = symbolName + L"<" + itow(i) + L">";
		if (!TryGetChildren_NFb(name))
		{
			return name;
		}
		i++;
	}
}

void Symbol::ReuseTemplateSpecSymbol(Ptr<Symbol> templateSpecSymbol, symbol_component::SymbolCategory _category)
{
	// change templateSpecSymbol to _category only if it is not
	if (_category != symbol_component::SymbolCategory::Normal)
	{
		List<Ptr<Symbol>> existingChildren;
		CopySymbolChildren(templateSpecSymbol.Obj(), existingChildren);
		
		// reset category and clear everything
		templateSpecSymbol->SetCategory(_category);

		AddSymbolChildren(templateSpecSymbol.Obj(), existingChildren);
	}
}

Symbol* Symbol::CreateSymbolInternal(Ptr<Declaration> _decl, const WString& declName, Ptr<Symbol> templateSpecSymbol, symbol_component::SymbolKind _kind, symbol_component::SymbolCategory _category)
{
	// match parent "category" with child "_category"
	switch (_category)
	{
	case symbol_component::SymbolCategory::Normal:
	case symbol_component::SymbolCategory::Function:
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

	// if templateSpecSymbol is not offered for reusing, create a new one
	auto symbol = templateSpecSymbol;
	if (symbol)
	{
		ReuseTemplateSpecSymbol(symbol, _category);
	}
	else
	{
		symbol = MakePtr<Symbol>(_category);
	}
	symbol->name = declName;
	symbol->kind = _kind;

	// add the created symbol to the correct location
	if (category == symbol_component::SymbolCategory::Function)
	{
		if (_decl.Cast<FunctionDeclaration>())
		{
			categoryData.function.implSymbols.Add(symbol);
		}
		else
		{
			categoryData.function.forwardSymbols.Add(symbol);
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
	WString declName = _decl->name.name;
	if (auto classDecl = _decl.Cast<ForwardClassDeclaration>())
	{
		if (classDecl->specializationSpec)
		{
			declName = DecorateNameForSpecializationSpec(declName, classDecl->specializationSpec);
		}
	}
	else if (auto valueAliasDecl = _decl.Cast<ValueAliasDeclaration>())
	{
		if (valueAliasDecl->specializationSpec)
		{
			declName = DecorateNameForSpecializationSpec(declName, valueAliasDecl->specializationSpec);
		}
	}

	// check if this symbol hides other symbols correctly
	Symbol* targetSymbol = nullptr;
	if (auto pChildren = TryGetChildren_NFb(declName))
	{
		Symbol* symbolCStyle = nullptr;
		Symbol* symbolOther = nullptr;

		// if there are multiple symbols with the same name, then it could only be one enum/class/struct/union and one of other kind.
		for (vint i = 0; i < pChildren->Count(); i++)
		{
			auto symbol = pChildren->Get(0).Obj();
			switch (symbol->kind)
			{
			case CSTYLE_TYPE_SYMBOL_KIND:
				if (symbolCStyle) return nullptr;
				symbolCStyle = symbol;
				break;
			default:
				if (symbolOther) return nullptr;
				symbolOther = symbol;
			}
		}

		switch (kind)
		{
		case CSTYLE_TYPE_SYMBOL_KIND:
			if (symbolCStyle)
			{
				if (symbolCStyle->kind != kind) return nullptr;
				targetSymbol = symbolCStyle;
			}
			break;
		default:
			if (symbolOther)
			{
				if (symbolOther->kind != kind) return nullptr;
				targetSymbol = symbolOther;
			}
		}
	}

	// if a symbol of the same kind is found, it could be a symbol that only has forward declarations assigned to it so far
	if (targetSymbol)
	{
		if (templateSpecSymbol)
		{
			// if a template<...> is offered (not possible for forward declaration)
			switch (kind)
			{
			case CLASS_SYMBOL_KIND:
				// there should be no implementation
				if (targetSymbol->categoryData.normal.implDecl) return nullptr;
				if (targetSymbol->categoryData.normal.children.Count() > 0) return nullptr;
				break;
			default:
				// only generic class implementation can be added using this function
				return nullptr;
			}

			// move everything from templateSpecSymbol to symbol that is created by forward declarations
			if (templateSpecSymbol->GetParentScope() != this)
			{
				return nullptr;
			}

			List<Ptr<Symbol>> existingChildren;
			CopySymbolChildren(templateSpecSymbol.Obj(), existingChildren);
			AddSymbolChildren(targetSymbol, existingChildren);
			_decl->symbol = targetSymbol;
			return targetSymbol;
		}
		else
		{
			// if no template<...> is offered, return the symbol when there is only one with the same kind
			_decl->symbol = targetSymbol;
			return targetSymbol;
		}
	}
	else
	{
		// if the symbol has never been created
		if (templateSpecSymbol)
		{
			// if a template<...> is offered (not possible for forward declaration), reuse the symbol
			ReuseTemplateSpecSymbol(templateSpecSymbol, _category);
			templateSpecSymbol->name = declName;
			templateSpecSymbol->kind = kind;
			AddChildAndSetParent_NFb(templateSpecSymbol->name, templateSpecSymbol);
			_decl->symbol = templateSpecSymbol.Obj();
			return templateSpecSymbol.Obj();
		}
		else
		{
			// if no template<...> is offered, create a symbol
			return CreateSymbolInternal(_decl, declName, nullptr, kind, symbol_component::SymbolCategory::Normal);
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
	default:
		// FunctionBody's parent cannot be changed
		throw UnexpectedSymbolCategoryException();
	}
}

symbol_component::SC_PSShared* Symbol::GetPSShared()
{
	switch (category)
	{
	case symbol_component::SymbolCategory::Normal:
		return &categoryData.normal;
	case symbol_component::SymbolCategory::Function:
		return &categoryData.function;
	default:
		// FunctionBody's PSShared is in Function
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

Symbol::Symbol(Symbol* _parent, Ptr<symbol_component::ClassMemberCache> classMemberCache)
	:category(symbol_component::SymbolCategory::Normal)
	, categoryData(symbol_component::SymbolCategory::Normal)
{
	categoryData.normal.classMemberCache = classMemberCache;
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
	// change category and clear everything
	if (category == _category)
	{
		throw UnexpectedSymbolCategoryException();
	}
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
const List<Ptr<Symbol>>& Symbol::GetForwardSymbols_F()
{
	switch (category)
	{
	case symbol_component::SymbolCategory::Function:
		return categoryData.function.forwardSymbols;
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

Symbol* Symbol::GetFunctionSymbol_Fb()
{
	switch (category)
	{
	case symbol_component::SymbolCategory::FunctionBody:
		return categoryData.functionBody.functionSymbol;
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

Ptr<symbol_component::ClassMemberCache> Symbol::GetClassMemberCache_NFb()
{
	switch (category)
	{
	case symbol_component::SymbolCategory::Normal:
		return categoryData.normal.classMemberCache;
	case symbol_component::SymbolCategory::FunctionBody:
		return categoryData.functionBody.classMemberCache;
	default:
		throw UnexpectedSymbolCategoryException();
	}
}

Symbol* Symbol::GetPSPrimary_NF()
{
	if (IsPSPrimary_NF())
	{
		return this;
	}
	else
	{
		return GetPSShared()->psPrimary;
	}
}

vint Symbol::GetPSPrimaryVersion_NF()
{
	return GetPSPrimary_NF()->GetPSShared()->psVersion;
}

const List<Symbol*>& Symbol::GetPSPrimaryDescendants_NF()
{
	return GetPSPrimary_NF()->GetPSShared()->psDescendants;
}

const List<Symbol*>& Symbol::GetPSParents_NF()
{
	return GetPSShared()->psParents;
}

const List<Symbol*>& Symbol::GetPSChildren_NF()
{
	return GetPSShared()->psChildren;
}

bool Symbol::IsPSPrimary_NF()
{
	auto ps = GetPSShared();
	return !ps->psPrimary && ps->psDescendants.Count() > 0;
}

void Symbol::AssignPSPrimary_NF(Symbol* primary)
{
	switch (kind)
	{
	case CLASS_SYMBOL_KIND:
	case symbol_component::SymbolKind::FunctionSymbol:
	case symbol_component::SymbolKind::ValueAlias:
		if (primary->kind != kind)
		{
			throw UnexpectedSymbolCategoryException();
		}
		break;
	default:
		throw UnexpectedSymbolCategoryException();
	}

	auto ps = GetPSShared();
	auto pps = primary->GetPSShared();
	if (pps->psPrimary || pps->psParents.Count() > 0 || ps->psPrimary || pps->psDescendants.Contains(this))
	{
		throw UnexpectedSymbolCategoryException();
	}

	pps->psVersion++;
	pps->psDescendants.Add(this);
	ps->psPrimary = primary;
}

void Symbol::RemovePSParent_NF(Symbol* oldParent)
{
	auto ps = GetPSShared();
	if (!ps->psParents.Contains(oldParent)) return;

	auto pps = oldParent->GetPSShared();
	if (ps->psPrimary != oldParent && ps->psPrimary != pps->psPrimary) throw UnexpectedSymbolCategoryException();

	pps->psChildren.Remove(this);
	ps->psParents.Remove(oldParent);
}

void Symbol::AddPSParent_NF(Symbol* newParent)
{
	auto ps = GetPSShared();
	if (!ps->psParents.Contains(newParent)) return;

	auto pps = newParent->GetPSShared();
	if (ps->psPrimary != newParent && ps->psPrimary != pps->psPrimary) throw UnexpectedSymbolCategoryException();

	pps->psChildren.Add(this);
	ps->psParents.Add(newParent);
}

void Symbol::SetClassMemberCacheForTemplateSpecScope_N(Ptr<symbol_component::ClassMemberCache> classMemberCache)
{
	// ensure the current symbol is a scope created for TemplateSpec
	if (category != symbol_component::SymbolCategory::Normal && kind != symbol_component::SymbolKind::Root && categoryData.normal.parent == nullptr)
	{
		throw UnexpectedSymbolCategoryException();
	}

	// only following actions are allowed:
	//   set classMemberCache when it has not
	//   reset classMemberCache when it has
	if ((categoryData.normal.classMemberCache == nullptr) == (classMemberCache == nullptr))
	{
		throw UnexpectedSymbolCategoryException();
	}
	categoryData.normal.classMemberCache = classMemberCache;
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
	child->SetParent(nullptr);
	children.Remove(name, child);
}

Symbol* Symbol::CreateFunctionSymbol_NFb(Ptr<ForwardFunctionDeclaration> _decl)
{
	// add a new Function category symbol
	// Function can be overloaded, so don't need to check if there is any existing symbols of the same name
	auto symbol = MakePtr<Symbol>(symbol_component::SymbolCategory::Function);
	symbol->kind = symbol_component::SymbolKind::FunctionSymbol;
	symbol->name = _decl->name.name;
	if (_decl->specializationSpec)
	{
		symbol->name = DecorateNameForSpecializationSpec(symbol->name, _decl->specializationSpec);
	}
	AddChildAndSetParent_NFb(symbol->name, symbol);
	return symbol.Obj();
}

Symbol* Symbol::CreateFunctionForwardSymbol_F(Ptr<ForwardFunctionDeclaration> _decl, Ptr<Symbol> templateSpecSymbol)
{
	// Create a new FunctionBodySymbol inside a FunctionBody
	// Function can be overloaded, so don't need to check if there is any existing symbols of the same name
	// Use FunctionSymbol's name for FunctionBodySymbol
	auto symbol = CreateSymbolInternal(_decl, name, templateSpecSymbol, symbol_component::SymbolKind::FunctionBodySymbol, symbol_component::SymbolCategory::FunctionBody);
	symbol->categoryData.functionBody.forwardDecl = _decl;
	return symbol;
}

Symbol* Symbol::CreateFunctionImplSymbol_F(Ptr<FunctionDeclaration> _decl, Ptr<Symbol> templateSpecSymbol, Ptr<symbol_component::ClassMemberCache> classMemberCache)
{
	// Create a new FunctionBodySymbol inside a FunctionBody
	// Function can be overloaded, so don't need to check if there is any existing symbols of the same name
	// Use FunctionSymbol's name for FunctionBodySymbol
	auto symbol = CreateSymbolInternal(_decl, name, templateSpecSymbol, symbol_component::SymbolKind::FunctionBodySymbol, symbol_component::SymbolCategory::FunctionBody);
	symbol->categoryData.functionBody.implDecl = _decl;
	if (classMemberCache)
	{
		// assign classMemberCache to the created symbol
		symbol->categoryData.functionBody.classMemberCache = classMemberCache;
		classMemberCache->funcSymbol = symbol;
		classMemberCache->funcDecl = _decl;
	}
	return symbol;
}

Symbol* Symbol::AddForwardDeclToSymbol_NFb(Ptr<Declaration> _decl, symbol_component::SymbolKind kind)
{
	// Create a symbol if the name is not used
	// Or add _decl to the existing symbol of the specified name, ensuring that the symbol has a correct kind
	auto symbol = AddToSymbolInternal_NFb(_decl, kind, nullptr, symbol_component::SymbolCategory::Normal);
	if (!symbol) return nullptr;
	symbol->categoryData.normal.forwardDecls.Add(_decl);
	return symbol;
}

Symbol* Symbol::AddImplDeclToSymbol_NFb(Ptr<Declaration> _decl, symbol_component::SymbolKind kind, Ptr<Symbol> templateSpecSymbol)
{
	// Create a symbol if the name is not used
	// Or add _decl to the existing symbol of the specified name, ensuring that the symbol has a correct kind
	auto symbol = AddToSymbolInternal_NFb(_decl, kind, templateSpecSymbol, symbol_component::SymbolCategory::Normal);
	if (!symbol) return nullptr;

	// Fail if an implementation has been assigned
	if (symbol->categoryData.normal.implDecl) return nullptr;
	symbol->categoryData.normal.implDecl = _decl;
	return symbol;
}

Symbol* Symbol::CreateStatSymbol_NFb(Ptr<Stat> _stat)
{
	// There is no limitation on statement scopes
	auto symbol = MakePtr<Symbol>();
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
		ids.Add((uniqueId = prefix), this);
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
						ids.Add((uniqueId = id), this);
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
		for (vint i = 0; i < categoryData.function.forwardSymbols.Count(); i++)
		{
			auto symbol = categoryData.function.forwardSymbols[i].Obj();
			symbol->GenerateUniqueId(ids, uniqueId + L"[decl" + itow(i) + L"]");
		}
		for (vint i = 0; i < categoryData.function.implSymbols.Count(); i++)
		{
			auto symbol = categoryData.function.implSymbols[i].Obj();
			symbol->GenerateUniqueId(ids, uniqueId + L"[impl" + itow(i) + L"]");
		}
		return;
	default:
		throw UnexpectedSymbolCategoryException();
	}
}

/***********************************************************************
ParsingArguments
***********************************************************************/

ParsingArguments::ParsingArguments(Ptr<Symbol> _root, Ptr<ITsysAlloc> _tsys, Ptr<IIndexRecorder> _recorder)
	:root(_root)
	, scopeSymbol(_root.Obj())
	, tsys(_tsys)
	, recorder(_recorder)
{
}

ParsingArguments ParsingArguments::WithScope(Symbol* _scopeSymbol)const
{
	ParsingArguments pa(root, tsys, recorder);
	pa.program = program;
	pa.scopeSymbol = _scopeSymbol;
	pa.parentDeclType = AdjustDeclInstantForScope(_scopeSymbol, parentDeclType, false);
	pa.taContext = AdjustTaContextForScope(_scopeSymbol, taContext);

	while (_scopeSymbol)
	{
		if (_scopeSymbol == scopeSymbol)
		{
			pa.functionBodySymbol = functionBodySymbol;
			break;
		}
		else if (_scopeSymbol->kind == symbol_component::SymbolKind::FunctionBodySymbol)
		{
			pa.functionBodySymbol = _scopeSymbol;
			break;
		}
		else
		{
			_scopeSymbol = _scopeSymbol->GetParentScope();
		}
	}

	return pa;
}

ParsingArguments ParsingArguments::AppendSingleLevelArgs(TemplateArgumentContext& taContext)const
{
	ParsingArguments pa = *this;
	taContext.parent = pa.taContext;
	pa.parentDeclType = parentDeclType;
	pa.taContext = &taContext;
	return pa;
}

ParsingArguments ParsingArguments::AdjustForDecl(Symbol* declSymbol)const
{
	auto newPa = WithScope(declSymbol);
	if (taContext)
	{
		// find the nearest common ancestor for declSymbol and taContext
		auto scope = declSymbol;
		while (scope)
		{
			auto ta = taContext;
			while (ta)
			{
				if (ta->symbolToApply == scope)
				{
					newPa.taContext = ta;
					return newPa;
				}
				ta = ta->parent;
			}
			scope = scope->GetParentScope();
		}
	}

	newPa.taContext = nullptr;
	return newPa;
}

ParsingArguments ParsingArguments::AdjustForDecl(Symbol* declSymbol, ITsys* parentDeclType, bool forceOverrideForNull)const
{
	auto pa = AdjustForDecl(declSymbol);
	if (forceOverrideForNull || parentDeclType)
	{
		pa.parentDeclType = AdjustDeclInstantForScope(pa.scopeSymbol, parentDeclType, false);
	}
	return pa;
}

EvaluationKind ParsingArguments::GetEvaluationKind(Declaration* decl, Ptr<TemplateSpec> spec)const
{
	if (spec && taContext && decl->symbol == taContext->symbolToApply)
	{
		return EvaluationKind::Instantiated;
	}
	else
	{
		return IsGeneralEvaluation() ? EvaluationKind::General : EvaluationKind::GeneralUnderInstantiated;
	}
}

bool ParsingArguments::IsGeneralEvaluation()const
{
	return taContext == nullptr;
}

bool ParsingArguments::TryGetReplacedGenericArg(ITsys* arg, ITsys*& result)const
{
	auto current = taContext;
	while (current)
	{
		vint index = current->arguments.Keys().IndexOf(arg);
		if (index != -1)
		{
			result = current->arguments.Values()[index];
			return true;
		}
		current = current->parent;
	}
	return false;
}

TemplateArgumentContext* ParsingArguments::AdjustTaContextForScope(Symbol* scopeSymbol, TemplateArgumentContext* taContext)
{
	if (!taContext) return nullptr;

	// search the nearest scope associated with a TemplateSpec
	auto scopeWithTemplateSpec = scopeSymbol;
	while (scopeWithTemplateSpec)
	{
		if (symbol_type_resolving::GetTemplateSpecFromSymbol(scopeWithTemplateSpec))
		{
			break;
		}
		scopeWithTemplateSpec = scopeWithTemplateSpec->GetParentScope();
	}

	if (!scopeWithTemplateSpec) return nullptr;

	{
		// try to confirm taContext->symbolApply is a direct or indirect parent scope of scopeWithTemplateSpec
		auto loopingScope = scopeWithTemplateSpec;
		while (loopingScope && taContext->symbolToApply != loopingScope)
		{
			loopingScope = loopingScope->GetParentScope();
		}

		// in this case, taContext is just fine
		if (loopingScope) return taContext;
	}
	{
		// try to confirm scopeWithTemplateSpec is a direct or indirect parent scope of taContext
		auto loopingTaContext = taContext;
		while (loopingTaContext && loopingTaContext->symbolToApply != scopeWithTemplateSpec)
		{
			loopingTaContext = loopingTaContext->parent;
		}

		// in this case, we find the correct taContext
		return loopingTaContext;
	}
	return nullptr;
}

ITsys* ParsingArguments::AdjustDeclInstantForScope(Symbol* scopeSymbol, ITsys* parentDeclType, bool returnTypeOfScope)
{
	if (!parentDeclType)
	{
		return nullptr;
	}
	if (parentDeclType->GetType() != TsysType::DeclInstant)
	{
		throw L"Wrong parentDeclType";
	}

	auto scopeWithTemplateClass = FindParentClassSymbol(scopeSymbol, true);
	while (scopeWithTemplateClass)
	{
		if (symbol_type_resolving::GetTemplateSpecFromSymbol(scopeWithTemplateClass))
		{
			break;
		}
		scopeWithTemplateClass = FindParentClassSymbol(scopeWithTemplateClass, false);
	}

	while (parentDeclType)
	{
		const auto& di = parentDeclType->GetDeclInstant();
		if (di.declSymbol == scopeWithTemplateClass)
		{
			if (!returnTypeOfScope && scopeWithTemplateClass == scopeSymbol)
			{
				parentDeclType = di.parentDeclType;
			}
			break;
		}
		parentDeclType = di.parentDeclType;
	}
	return parentDeclType;
}

/***********************************************************************
EnsureFunctionBodyParsed
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

	void Visit(FriendClassDeclaration* self) override
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

	void Visit(StaticAssertDeclaration* self) override
	{
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
	/*
	PredefineType(program, pa, L"__m64", true, CppClassType::Union, symbol_component::SymbolKind::Struct);
	PredefineType(program, pa, L"__m128", true, CppClassType::Union, symbol_component::SymbolKind::Struct);
	PredefineType(program, pa, L"__m128d", true, CppClassType::Struct, symbol_component::SymbolKind::Struct);
	PredefineType(program, pa, L"__m128i", true, CppClassType::Union, symbol_component::SymbolKind::Union);
	PredefineType(program, pa, L"__m256", true, CppClassType::Union, symbol_component::SymbolKind::Union);
	PredefineType(program, pa, L"__m256d", true, CppClassType::Struct, symbol_component::SymbolKind::Struct);
	PredefineType(program, pa, L"__m256i", true, CppClassType::Union, symbol_component::SymbolKind::Union);
	PredefineType(program, pa, L"__m512", true, CppClassType::Union, symbol_component::SymbolKind::Union);
	PredefineType(program, pa, L"__m512d", true, CppClassType::Struct, symbol_component::SymbolKind::Struct);
	PredefineType(program, pa, L"__m512i", true, CppClassType::Union, symbol_component::SymbolKind::Union);
	*/

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

Symbol* FindParentClassSymbol(Symbol* symbol, bool includeThisSymbol)
{
	auto current = includeThisSymbol ? symbol : symbol->GetParentScope();
	while (current)
	{
		switch (current->kind)
		{
		case CLASS_SYMBOL_KIND:
			return current;
		}
		current = current->GetParentScope();
	}
	return nullptr;
}