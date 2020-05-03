#include "Symbol.h"

namespace symbol_idgen
{
}

WString Symbol::DecorateNameForSpecializationSpec(const WString& symbolName, Ptr<SpecializationSpec> spec)
{
	// TODO: [Cpp.md] `Symbol::DecorateNameForSpecializationSpec` generates unique name for declaractions with `specializationSpec`.
	//       Generate a postfix according to spec
	//       and only use a counter when there is still a conflict

	// sycn with SearchForFunctionWithSameSignature
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
				// TODO: [Cpp.md] `Symbol::GenerateUniqueId` generates unique name for overloaded functions. Optional: Name doesn't include a counter.
				//       Generate a postfix according to return type and argument types for Function category
				//       and only use a counter when there is still a conflict
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