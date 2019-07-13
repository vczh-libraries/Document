#include "Ast_Resolving_ExpandPotentialVta.h"

namespace symbol_totsys_impl
{
	//////////////////////////////////////////////////////////////////////////////////////
	// TypeSymbolToTsys
	//////////////////////////////////////////////////////////////////////////////////////

	void TypeSymbolToTsys(const ParsingArguments& pa, TypeTsysList& result, GenericArgContext* gaContext, Symbol* symbol, bool allowVariadic, bool& hasVariadic, bool& hasNonVariadic)
	{
		switch (symbol->kind)
		{
		case symbol_component::SymbolKind::Enum:
		case symbol_component::SymbolKind::Class:
		case symbol_component::SymbolKind::Struct:
		case symbol_component::SymbolKind::Union:
			AddTsysToResult(result, pa.tsys->DeclOf(symbol));
			hasNonVariadic = true;
			return;
		case symbol_component::SymbolKind::TypeAlias:
			{
				auto usingDecl = symbol->definition.Cast<TypeAliasDeclaration>();
				symbol_type_resolving::EvaluateSymbol(pa, usingDecl.Obj());
				auto& types = symbol->evaluation.Get();
				for (vint j = 0; j < types.Count(); j++)
				{
					AddTsysToResult(result, types[j]);
				}
				hasNonVariadic = true;
			}
			return;
		case symbol_component::SymbolKind::GenericTypeArgument:
			{
				auto& types = symbol->evaluation.Get();
				for (vint j = 0; j < types.Count(); j++)
				{
					if (gaContext)
					{
						auto type = types[j];
						vint index = gaContext->arguments.Keys().IndexOf(type);
						if (index != -1)
						{
							auto& replacedTypes = gaContext->arguments.GetByIndex(index);
							for (vint k = 0; k < replacedTypes.Count(); k++)
							{
								if (symbol->ellipsis)
								{
									if (!allowVariadic)
									{
										throw NotConvertableException();
									}
									auto replacedType = replacedTypes[k];
									if (replacedType->GetType() == TsysType::Any || replacedType->GetType() == TsysType::Init)
									{
										AddTsysToResult(result, replacedTypes[k]);
										hasVariadic = true;
									}
									else
									{
										throw NotConvertableException();
									}
								}
								else
								{
									AddTsysToResult(result, replacedTypes[k]);
									hasNonVariadic = true;
								}
							}
							continue;
						}
					}

					if (symbol->ellipsis)
					{
						if (!allowVariadic)
						{
							throw NotConvertableException();
						}
						AddTsysToResult(result, pa.tsys->Any());
						hasVariadic = true;
					}
					else
					{
						AddTsysToResult(result, types[j]);
						hasNonVariadic = true;
					}
				}
			}
			return;
		}
		throw NotConvertableException();
	}
}