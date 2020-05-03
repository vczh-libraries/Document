#include <VlppOS.h>
#include "Symbol.h"
#include "Ast_Type.h"
#include "Ast_Decl.h"

using namespace vl::stream;

namespace symbol_idgen
{
	/***********************************************************************
	LogTypeVisitor
	***********************************************************************/

	class IdGenTypeVisitor : public Object, public virtual ITypeVisitor
	{
	private:
		StreamWriter&			writer;
		bool					forParameter = false;

	public:
		IdGenTypeVisitor(StreamWriter& _writer)
			:writer(_writer)
		{
		}

		static WString LogToString(Ptr<Type> type)
		{
			return GenerateToStream(
				[=](StreamWriter& writer)
				{
					IdGenTypeVisitor visitor(writer);
					visitor.Log(type);
				}
			);
		}

		void Log(Ptr<Type> type, bool _forParameter = false)
		{
			auto oldFP = forParameter;
			forParameter = _forParameter;
			type->Accept(this);
			forParameter = oldFP;
		}

		void Visit(PrimitiveType* self)override
		{
			switch (self->prefix)
			{
			case CppPrimitivePrefix::_signed:
				writer.WriteString(L"signed ");
				break;
			case CppPrimitivePrefix::_unsigned:
				writer.WriteString(L"unsigned ");
				break;
			}

			switch (self->primitive)
			{
			case CppPrimitiveType::_auto:			writer.WriteString(L"auto");		break;
			case CppPrimitiveType::_void:			writer.WriteString(L"void");		break;
			case CppPrimitiveType::_bool:			writer.WriteString(L"bool");		break;
			case CppPrimitiveType::_char:			writer.WriteString(L"char");		break;
			case CppPrimitiveType::_wchar_t:		writer.WriteString(L"wchar_t");		break;
			case CppPrimitiveType::_char16_t:		writer.WriteString(L"char16_t");	break;
			case CppPrimitiveType::_char32_t:		writer.WriteString(L"char32_t");	break;
			case CppPrimitiveType::_short:			writer.WriteString(L"short");		break;
			case CppPrimitiveType::_int:			writer.WriteString(L"int");			break;
			case CppPrimitiveType::___int8:			writer.WriteString(L"__int8");		break;
			case CppPrimitiveType::___int16:		writer.WriteString(L"__int16");		break;
			case CppPrimitiveType::___int32:		writer.WriteString(L"__int32");		break;
			case CppPrimitiveType::___int64:		writer.WriteString(L"__int64");		break;
			case CppPrimitiveType::_long:			writer.WriteString(L"long");		break;
			case CppPrimitiveType::_long_int:		writer.WriteString(L"long int");	break;
			case CppPrimitiveType::_long_long:		writer.WriteString(L"long long");	break;
			case CppPrimitiveType::_float:			writer.WriteString(L"float");		break;
			case CppPrimitiveType::_double:			writer.WriteString(L"double");		break;
			case CppPrimitiveType::_long_double:	writer.WriteString(L"long double");	break;
			default:
				throw 0;
			}
		}

		void Visit(ReferenceType* self)override
		{
			Log(self->type);
			switch (self->reference)
			{
			case CppReferenceType::Ptr:				writer.WriteString(L" *");	break;
			case CppReferenceType::LRef:			writer.WriteString(L" &");	break;
			case CppReferenceType::RRef:			writer.WriteString(L" &&");	break;
			default:
				throw 0;
			}
		}

		void Visit(ArrayType* self)override
		{
			Log(self->type);
			writer.WriteString(L" [");
			if (self->expr)
			{
				writer.WriteString(L"*");
			}
			writer.WriteString(L"]");
		}

		void Visit(CallingConventionType* self)override
		{
			Log(self->type);
		}

		void Visit(FunctionType* self)override
		{
			if (self->decoratorReturnType)
			{
				Log(self->decoratorReturnType);
			}
			else
			{
				Log(self->returnType);
			}

			writer.WriteString(L" (");
			for (vint i = 0; i < self->parameters.Count(); i++)
			{
				if (i != 0)
				{
					writer.WriteString(L", ");
				}

				Log(self->parameters[i].item->type, true);

				if (self->parameters[i].isVariadic)
				{
					writer.WriteString(L"...");
				}
			}
			writer.WriteChar(L')');
		}

		void Visit(MemberType* self)override
		{
			Log(self->type);
			writer.WriteString(L" (");
			Log(self->classType);
			writer.WriteString(L" ::)");
		}

		void Visit(DeclType* self)override
		{
			writer.WriteString(L"decltype(");
			if (self->expr)
			{
				writer.WriteString(L"*");
			}
			else
			{
				writer.WriteString(L"auto");
			}
			writer.WriteString(L")");
		}

		void Visit(DecorateType* self)override
		{
			Log(self->type);
			if (self->isConst)		writer.WriteString(L" const");
			if (self->isVolatile)	writer.WriteString(L" volatile");
		}

		void Visit(RootType* self)override
		{
			writer.WriteString(L"__root");
		}

		void Log(Symbol* symbol)
		{
			if (auto parent = symbol->GetParentScope())
			{
				Log(parent);
				writer.WriteString(L"::");
			}
			writer.WriteString(symbol->name);
		}

		void Visit(IdType* self)override
		{
			if (self->resolving && self->resolving->items.Count() > 0)
			{
				Log(self->resolving->items[0].symbol);
			}
			else
			{
				writer.WriteString(self->name.name);
			}
		}

		void Visit(ChildType* self)override
		{
			if (self->resolving && self->resolving->items.Count() > 0)
			{
				Log(self->resolving->items[0].symbol);
			}
			else
			{
				writer.WriteString(self->name.name);
			}
		}

		void Visit(GenericType* self)override
		{
			Log(Ptr<Type>(self->type));
			writer.WriteString(L"<");
			for (vint i = 0; i < self->arguments.Count(); i++)
			{
				if (i != 0)
				{
					writer.WriteString(L", ");
				}

				auto argument = self->arguments[i];
				if (argument.item.type)
				{
					Log(argument.item.type);
				}
				else
				{
					writer.WriteString(L"*");
				}

				if (argument.isVariadic)
				{
					writer.WriteString(L"...");
				}
			}
			writer.WriteString(L">");
		}
	};
}

WString Symbol::DecorateNameForSpecializationSpec(const WString& symbolName, Ptr<SpecializationSpec> spec)
{
	// TODO: [Cpp.md] `Symbol::DecorateNameForSpecializationSpec` generates unique name for declaractions with `specializationSpec`.
	//       Generate a postfix according to spec
	//       and only use a counter when there is still a conflict

	// sycn with SearchForFunctionWithSameSignature
	vint dupCounter = 1;
	while (true)
	{
		auto name = symbolName + L"<" + itow(dupCounter) + L">";
		if (!TryGetChildren_NFb(name))
		{
			return name;
		}
		dupCounter++;
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
				vint dupCounter = 1;
				while (true)
				{
					WString id = prefix + name + (dupCounter == 1 ? WString::Empty : itow(dupCounter));
					if (!ids.Keys().Contains(id))
					{
						ids.Add((uniqueId = id), this);
						nextPrefix = id + L"::";
						break;
					}
					dupCounter++;
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