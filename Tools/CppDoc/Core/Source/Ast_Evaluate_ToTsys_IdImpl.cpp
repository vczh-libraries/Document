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

	//////////////////////////////////////////////////////////////////////////////////////
	// SymbolListToTsys
	//////////////////////////////////////////////////////////////////////////////////////

	template<typename TGenerator>
	static bool SymbolListToTsys(const ParsingArguments& pa, TypeTsysList& result, GenericArgContext* gaContext, bool allowVariadic, TGenerator&& symbolGenerator)
	{
		bool hasVariadic = false;
		bool hasNonVariadic = false;
		symbolGenerator([&](Symbol* symbol)
		{
			TypeSymbolToTsys(pa, result, gaContext, symbol, allowVariadic, hasVariadic, hasNonVariadic);
		});

		if (hasVariadic && hasNonVariadic)
		{
			throw NotConvertableException();
		}
		return hasVariadic;
	}

	//////////////////////////////////////////////////////////////////////////////////////
	// CreateIdReferenceType
	//////////////////////////////////////////////////////////////////////////////////////

	void CreateIdReferenceType(const ParsingArguments& pa, GenericArgContext* gaContext, Ptr<Resolving> resolving, bool allowAny, bool allowVariadic, TypeTsysList& result, bool& isVta)
	{
		if (!resolving)
		{
			if (allowAny)
			{
				AddTsysToResult(result, pa.tsys->Any());
				return;
			}
			else
			{
				throw NotConvertableException();
			}
		}
		else if (resolving->resolvedSymbols.Count() == 0)
		{
			throw NotConvertableException();
		}

		isVta = SymbolListToTsys(pa, result, gaContext, allowVariadic, [&](auto receiver)
		{
			for (vint i = 0; i < resolving->resolvedSymbols.Count(); i++)
			{
				receiver(resolving->resolvedSymbols[i]);
			}
		});
	}

	//////////////////////////////////////////////////////////////////////////////////////
	// ResolveChildTypeWithGenericArguments
	//////////////////////////////////////////////////////////////////////////////////////

	template<typename TReceiver>
	void ResolveChildTypeWithGenericArguments(const ParsingArguments& pa, ChildType* self, ITsys* classType, SortedList<Symbol*>& visited, TReceiver&& receiver)
	{
		if (classType->GetType() == TsysType::Decl)
		{
			auto newPa = pa.WithContext(classType->GetDecl());
			auto rsr = ResolveSymbol(newPa, self->name, SearchPolicy::ChildSymbol);
			if (rsr.types)
			{
				for (vint i = 0; i < rsr.types->resolvedSymbols.Count(); i++)
				{
					auto symbol = rsr.types->resolvedSymbols[i];
					if (!visited.Contains(symbol))
					{
						visited.Add(symbol);
						receiver(symbol);
					}
				}
			}
		}
	}

	//////////////////////////////////////////////////////////////////////////////////////
	// ProcessChildType
	//////////////////////////////////////////////////////////////////////////////////////
	
	void ProcessChildType(const ParsingArguments& pa, GenericArgContext* gaContext, ChildType* self, ExprTsysItem argClass, ExprTsysList& result)
	{
		if (argClass.tsys->IsUnknownType())
		{
			result.Add(GetExprTsysItem(pa.tsys->Any()));
		}
		else
		{
			TypeTsysList childTypes;
			SortedList<Symbol*> visited;
			SymbolListToTsys(pa, childTypes, gaContext, false, [&](auto receiver)
			{
				ResolveChildTypeWithGenericArguments(pa, self, argClass.tsys, visited, receiver);
			});
			symbol_type_resolving::AddTemp(result, childTypes);
		}
	}
}