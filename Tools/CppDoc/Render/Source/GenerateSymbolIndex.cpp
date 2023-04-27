#include "Render.h"

/***********************************************************************
GenerateSymbolGroupUniqueId
***********************************************************************/

void SetUniqueId(
	Ptr<SymbolGroup> symbolGroup,
	WString id,
	SortedList<WString>& ids
)
{
	id = wupper(id);
	if (!ids.Contains(id))
	{
		symbolGroup->uniqueId = id;
		ids.Add(id);
	}
	else
	{
		vint index = 2;
		while (true)
		{
			auto newId = id + itow(index);
			if (!ids.Contains(newId))
			{
				symbolGroup->uniqueId = newId;
				ids.Add(newId);
				break;
			}
			else
			{
				index++;
			}
		}
	}
}

void GenerateSymbolGroupUniqueId(
	Ptr<SymbolGroup> symbolGroup,
	SymbolGroup* parentGroup,
	vint& fragmentCount,
	SortedList<WString>& ids
)
{
	if (symbolGroup->children.Count() > 0)
	{
		fragmentCount++;
	}

	Array<wchar_t> buffer(65);
	memset(&buffer[0], 0, sizeof(wchar_t) * buffer.Count());

	WString reference;
	switch (symbolGroup->kind)
	{
	case SymbolGroupKind::Symbol:
		reference = symbolGroup->symbol->uniqueId;
		break;
	case SymbolGroupKind::SymbolAndText:
		reference = symbolGroup->name + L"!" + symbolGroup->symbol->uniqueId;
		break;
	case SymbolGroupKind::Text:
		reference = symbolGroup->name + L"@" + parentGroup->symbol->uniqueId;
		break;
	default:
		throw UnexpectedSymbolCategoryException();
	}

	for (vint i = 0; i < buffer.Count() - 1; i++)
	{
		if (i == reference.Length()) break;
		switch (auto c = reference[i])
		{
		case ' ':case '<':case '>':case ':':case '\"':case '/':case '\\':case '|':case '?':case '*':
			buffer[i] = L'_';
			break;
		default:
			buffer[i] = c;
		}
	}

	auto uniqueId = wupper(&buffer[0]);
	SetUniqueId(symbolGroup, uniqueId, ids);

	for (vint i = 0; i < symbolGroup->children.Count(); i++)
	{
		GenerateSymbolGroupUniqueId(symbolGroup->children[i], symbolGroup.Obj(), fragmentCount, ids);
	}
}

/***********************************************************************
GenerateSymbolGroupForFileGroup
***********************************************************************/

bool IsDeclInFileGroup(
	Ptr<GlobalLinesRecord> global,
	Ptr<Declaration> decl,
	const WString& fileGroupPrefix
)
{
	vint index = global->declToFiles.Keys().IndexOf({ decl,nullptr });
	if (index != -1)
	{
		if (INVLOC.StartsWith(global->declToFiles.Values()[index].GetFullPath(), fileGroupPrefix, Locale::Normalization::IgnoreCase))
		{
			return true;
		}
	}
	return false;
}

bool IsSymbolInFileGroup(
	Ptr<GlobalLinesRecord> global,
	Symbol* context,
	const WString& fileGroupPrefix,
	bool& generateChildSymbols
)
{
	generateChildSymbols = false;
	switch (context->GetCategory())
	{
	case symbol_component::SymbolCategory::Normal:
		if (auto decl = context->GetImplDecl_NFb())
		{
			if (!decl->implicitlyGeneratedMember)
			{
				if (IsDeclInFileGroup(global, decl, fileGroupPrefix))
				{
					switch (context->kind)
					{
					case CLASS_SYMBOL_KIND:
						generateChildSymbols = true;
						break;
					}
					return true;
				}
			}
			return false;
		}
		for (vint i = 0; i < context->GetForwardDecls_N().Count(); i++)
		{
			auto decl = context->GetForwardDecls_N()[i];
			if (!decl->implicitlyGeneratedMember)
			{
				if (IsDeclInFileGroup(global, decl, fileGroupPrefix))
				{
					switch (context->kind)
					{
					case symbol_component::SymbolKind::Namespace:
						generateChildSymbols = true;
						break;
					}
					return true;
				}
			}
		}
		break;
	case symbol_component::SymbolCategory::Function:
		for (vint i = 0; i < context->GetImplSymbols_F().Count(); i++)
		{
			auto decl = context->GetImplSymbols_F()[i]->GetImplDecl_NFb();
			if (!decl->implicitlyGeneratedMember)
			{
				if (IsDeclInFileGroup(global, decl, fileGroupPrefix))
				{
					return true;
				}
			}
		}

		if (context->GetImplSymbols_F().Count() > 0)
		{
			return false;
		}

		for (vint i = 0; i < context->GetForwardSymbols_F().Count(); i++)
		{
			auto decl = context->GetForwardSymbols_F()[i]->GetForwardDecl_Fb();
			if (!decl->implicitlyGeneratedMember)
			{
				if (IsDeclInFileGroup(global, decl, fileGroupPrefix))
				{
					return true;
				}
			}
		}
		break;
	case symbol_component::SymbolCategory::FunctionBody:
		throw UnexpectedSymbolCategoryException();
	}
	return false;
}

Ptr<SymbolGroup> GenerateSymbolIndexForFileGroup(
	Ptr<GlobalLinesRecord> global,
	const WString& fileGroupPrefix,
	Symbol* context,
	SymbolGroup* parentGroup,
	Dictionary<Symbol*, Ptr<SymbolGroup>>& psContainers
)
{
	if (psContainers.Keys().Contains(context))
	{
		return nullptr;
	}

	bool generateChildSymbols = false;
	if (context->kind == symbol_component::SymbolKind::Root)
	{
		generateChildSymbols = true;
	}
	else
	{
		if (!IsSymbolInFileGroup(global, context, fileGroupPrefix, generateChildSymbols))
		{
			return nullptr;
		}
	}

	auto symbolGroup = Ptr(new SymbolGroup);
	symbolGroup->symbol = context;
	
	if (generateChildSymbols && context->kind != symbol_component::SymbolKind::Root)
	{
		symbolGroup->braces = true;
	}

	if (generateChildSymbols)
	{
		for (vint i = 0; i < context->GetChildren_NFb().Count(); i++)
		{
			auto& children = context->GetChildren_NFb().GetByIndex(i);
			for (vint j = 0; j < children.Count(); j++)
			{
				auto& childSymbol = children[j];
				if (!childSymbol.childExpr && !childSymbol.childType)
				{
					if (auto childGroup = GenerateSymbolIndexForFileGroup(global, fileGroupPrefix, childSymbol.childSymbol.Obj(), symbolGroup.Obj(), psContainers))
					{
						symbolGroup->children.Add(childGroup);
					}
				}
			}
		}
	}

	if (context->kind == symbol_component::SymbolKind::Root || context->kind == symbol_component::SymbolKind::Namespace)
	{
		if (symbolGroup->children.Count() == 0)
		{
			return nullptr;
		}
	}

	if (auto primary = context->GetPSPrimary_NF())
	{
		if (primary == context)
		{
			auto psGroup = Ptr(new SymbolGroup);
			psGroup->kind = SymbolGroupKind::Text;
			psGroup->name = L"Partial Specializations";

			symbolGroup->children.Insert(0, psGroup);
			psContainers.Add(context, psGroup);
		}
		else
		{
			if (!psContainers.Keys().Contains(primary))
			{
				bool unused = false;
				if (IsSymbolInFileGroup(global, primary, fileGroupPrefix, unused))
				{
					if (auto primaryGroup = GenerateSymbolIndexForFileGroup(global, fileGroupPrefix, primary, parentGroup, psContainers))
					{
						parentGroup->children.Add(primaryGroup);
					}
					else
					{
						throw UnexpectedSymbolCategoryException();
					}
				}
				else
				{
					auto psGroup = Ptr(new SymbolGroup);
					psGroup->kind = SymbolGroupKind::SymbolAndText;
					psGroup->name = L"(Partial Specializations)";
					psGroup->symbol = primary;

					parentGroup->children.Add(psGroup);
					psContainers.Add(primary, psGroup);
				}
			}

			auto container = psContainers[primary];
			container->children.Add(symbolGroup);
			return nullptr;
		}
	}

	return symbolGroup;
}

/***********************************************************************
GenerateSymbolGroup
***********************************************************************/

Ptr<SymbolGroup> GenerateSymbolGroup(
	Ptr<GlobalLinesRecord> global,
	IndexResult& result,
	FileGroupConfig& fileGroups,
	IProgressReporter* progressReporter
)
{
	auto rootGroup = Ptr(new SymbolGroup);
	vint fragmentCount = 0;
	{
		rootGroup->kind = SymbolGroupKind::Root;
		for (vint i = 0; i < fileGroups.Count(); i++)
		{
			auto prefix = fileGroups[i].get<0>();
			Dictionary<Symbol*, Ptr<SymbolGroup>> psContainers;
			if (auto fileGroup = GenerateSymbolIndexForFileGroup(global, prefix, result.pa.root.Obj(), nullptr, psContainers))
			{
				fileGroup->kind = SymbolGroupKind::Group;
				fileGroup->name = fileGroups[i].get<1>();
				rootGroup->children.Add(fileGroup);
			}
		}
	}
	{
		Regex extractLastWord(L"^(/.*/W)?(<word>/w+)/W*$");
		vint indexWord = extractLastWord.CaptureNames().IndexOf(L"word");
		for (vint i = 0; i < rootGroup->children.Count(); i++)
		{
			SortedList<WString> ids;
			auto fileGroup = rootGroup->children[i];
			auto match = extractLastWord.MatchHead(fileGroup->name);
			SetUniqueId(fileGroup, match->Groups()[indexWord][0].Value(), ids);

			for (vint j = 0; j < fileGroup->children.Count(); j++)
			{
				GenerateSymbolGroupUniqueId(fileGroup->children[j], fileGroup.Obj(), fragmentCount, ids);
			}
		}
	}

	return rootGroup;
}