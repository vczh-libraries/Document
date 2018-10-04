#include "Ast.h"
#include "Ast_Decl.h"
#include "Ast_Type.h"
#include "Ast_Expr.h"
#include "Ast_Stat.h"
#include "Parser.h"

#define CPPDOC_ACCEPT(NAME) void NAME::Accept(ITypeVisitor* visitor) { visitor->Visit(this); }
CPPDOC_TYPE_LIST(CPPDOC_ACCEPT)
#undef CPPDOC_ACCEPT

#define CPPDOC_ACCEPT(NAME) void NAME::Accept(IDeclarationVisitor* visitor) { visitor->Visit(this); }
CPPDOC_DECL_LIST(CPPDOC_ACCEPT)
#undef CPPDOC_ACCEPT

#define CPPDOC_ACCEPT(NAME) void NAME::Accept(IExprVisitor* visitor) { visitor->Visit(this); }
CPPDOC_EXPR_LIST(CPPDOC_ACCEPT)
#undef CPPDOC_ACCEPT

#define CPPDOC_ACCEPT(NAME) void NAME::Accept(IStatVisitor* visitor) { visitor->Visit(this); }
CPPDOC_STAT_LIST(CPPDOC_ACCEPT)
#undef CPPDOC_ACCEPT

/***********************************************************************
Resolving
***********************************************************************/

// Change all forward declaration symbols to their real definition
void Resolving::Calibrate()
{
	if (fullyCalibrated) return;
	vint forwards = 0;

	SortedList<Symbol*> used;
	for (vint i = 0; i < resolvedSymbols.Count(); i++)
	{
		auto& symbol = resolvedSymbols[i];
		if (symbol->isForwardDeclaration)
		{
			if (symbol->forwardDeclarationRoot)
			{
				if (used.Contains(symbol->forwardDeclarationRoot))
				{
					resolvedSymbols.RemoveAt(i);
					i--;
				}
				else
				{
					symbol = symbol->forwardDeclarationRoot;
				}
			}
			else
			{
				forwards++;
			}
		}
	}

	if (forwards == 0)
	{
		fullyCalibrated = true;
	}
}

/***********************************************************************
IsSameResolvedType
***********************************************************************/

class IsSameResolvedTypeVisitor : public Object, public virtual ITypeVisitor
{
public:
	bool					result = false;
	Ptr<Type>				peerType;

	void TestResolving(Ptr<Resolving> resolving)
	{
		if (auto type = peerType.Cast<ResolvableType>())
		{
			if (type->resolving)
			{
				resolving->Calibrate();
				type->resolving->Calibrate();
				result = CompareEnumerable(resolving->resolvedSymbols, type->resolving->resolvedSymbols) == 0;
			}
		}
	}

	void Visit(PrimitiveType* self)override
	{
		if (auto type = peerType.Cast<PrimitiveType>())
		{
			result = self->prefix == type->prefix && self->primitive == type->primitive;
		}
	}

	void Visit(ReferenceType* self)override
	{
		if (auto type = peerType.Cast<ReferenceType>())
		{
			result = self->reference == type->reference && IsSameResolvedType(self->type, type->type);
		}
	}

	void Visit(ArrayType* self)override
	{
		if (auto type = peerType.Cast<ArrayType>())
		{
			result = IsSameResolvedType(self->type, type->type);
		}
	}

	void Visit(CallingConventionType* self)override
	{
		if (auto type = peerType.Cast<CallingConventionType>())
		{
			result = self->callingConvention == type->callingConvention && IsSameResolvedType(self->type, type->type);
		}
	}

	void Visit(FunctionType* self)override
	{
		if (auto type = peerType.Cast<FunctionType>())
		{
			if (self->qualifierConstExpr != type->qualifierConstExpr) return;
			if (self->qualifierConst != type->qualifierConst) return;
			if (self->qualifierVolatile != type->qualifierVolatile) return;
			if (self->qualifierLRef != type->qualifierLRef) return;
			if (self->qualifierRRef != type->qualifierRRef) return;
			if (!IsSameResolvedType(self->returnType, type->returnType)) return;
			if (!IsSameResolvedType(self->decoratorReturnType, type->decoratorReturnType)) return;
			if (self->parameters.Count() != type->parameters.Count()) return;

			for (vint i = 0; i < self->parameters.Count(); i++)
			{
				if (!IsSameResolvedType(self->parameters[i]->type, type->parameters[i]->type)) return;
			}
			result = true;
		}
	}

	void Visit(MemberType* self)override
	{
		if (auto type = peerType.Cast<MemberType>())
		{
			result = IsSameResolvedType(self->classType, type->classType) && IsSameResolvedType(self->type, type->type);
		}
	}

	void Visit(DeclType* self)override
	{
		throw 0;
	}

	void Visit(DecorateType* self)override
	{
		if (auto type = peerType.Cast<DecorateType>())
		{
			result = self->isConstExpr == type->isConstExpr
				&& self->isConst == type->isConst
				&& self->isVolatile == type->isVolatile
				&& IsSameResolvedType(self->type, type->type);
		}
	}

	void Visit(RootType* self)override
	{
		result = peerType.Cast<RootType>();
	}

	void Visit(IdType* self)override
	{
		if (self->resolving)
		{
			TestResolving(self->resolving);
		}
	}

	void Visit(ChildType* self)override
	{
		if (self->resolving)
		{
			TestResolving(self->resolving);
		}
	}

	void Visit(GenericType* self)override
	{
		if (auto type = peerType.Cast<GenericType>())
		{
			if (!IsSameResolvedType(self->type, type->type)) return;
			if (self->arguments.Count() != type->arguments.Count()) return;

			for (vint i = 0; i < self->arguments.Count(); i++)
			{
				auto ga1 = self->arguments[i];
				auto ga2 = self->arguments[i];
				if ((ga1.type == nullptr) != (ga2.type == nullptr)) return;

				if (ga1.type && ga2.type)
				{
					if (!IsSameResolvedType(ga1.type, ga2.type)) return;
				}
				else
				{
					throw 0;
				}
			}
			result = true;
		}
	}

	void Visit(VariadicTemplateArgumentType* self)override
	{
		if (auto type = peerType.Cast<VariadicTemplateArgumentType>())
		{
			result = IsSameResolvedType(self->type, type->type);
		}
	}
};

// Test if two types are actually the same one
//   ArrayType length is ignored
//   FunctionType declarators are ignored
//   DeclType is not supported yet
//   Expression arguments in GenericType are not supported yet
bool IsSameResolvedType(Ptr<Type> t1, Ptr<Type> t2)
{
	if (t1 && t2)
	{
		IsSameResolvedTypeVisitor visitor;
		visitor.peerType = t2;
		t1->Accept(&visitor);
		return visitor.result;
	}
	else
	{
		return (t1 == nullptr) == (t2 == nullptr);
	}
}

/***********************************************************************
TypeToTsys
***********************************************************************/

class TypeToTsysVisitor : public Object, public virtual ITypeVisitor
{
public:
	List<ITsys*>&			result;
	ParsingArguments&		pa;

	TypeToTsysVisitor(ParsingArguments& _pa, List<ITsys*>& _result)
		:pa(_pa)
		, result(_result)
	{
	}

	void Visit(PrimitiveType* self)override
	{
		switch (self->prefix)
		{
		case CppPrimitivePrefix::_none:
			switch (self->primitive)
			{
			case CppPrimitiveType::_void:			result.Add(pa.tsys->PrimitiveOf({ TsysPrimitiveType::Void,		TsysBytes::_1 })); return;
			case CppPrimitiveType::_bool:			result.Add(pa.tsys->PrimitiveOf({ TsysPrimitiveType::Bool,		TsysBytes::_1 })); return;
			case CppPrimitiveType::_char:			result.Add(pa.tsys->PrimitiveOf({ TsysPrimitiveType::SChar,		TsysBytes::_1 })); return;
			case CppPrimitiveType::_wchar_t:		result.Add(pa.tsys->PrimitiveOf({ TsysPrimitiveType::UWChar,	TsysBytes::_2 })); return;
			case CppPrimitiveType::_char16_t:		result.Add(pa.tsys->PrimitiveOf({ TsysPrimitiveType::UChar,		TsysBytes::_2 })); return;
			case CppPrimitiveType::_char32_t:		result.Add(pa.tsys->PrimitiveOf({ TsysPrimitiveType::UChar,		TsysBytes::_4 })); return;
			case CppPrimitiveType::_short:			result.Add(pa.tsys->PrimitiveOf({ TsysPrimitiveType::SInt,		TsysBytes::_2 })); return;
			case CppPrimitiveType::_int:			result.Add(pa.tsys->PrimitiveOf({ TsysPrimitiveType::SInt,		TsysBytes::_4 })); return;
			case CppPrimitiveType::___int8:			result.Add(pa.tsys->PrimitiveOf({ TsysPrimitiveType::SInt,		TsysBytes::_1 })); return;
			case CppPrimitiveType::___int16:		result.Add(pa.tsys->PrimitiveOf({ TsysPrimitiveType::SInt,		TsysBytes::_2 })); return;
			case CppPrimitiveType::___int32:		result.Add(pa.tsys->PrimitiveOf({ TsysPrimitiveType::SInt,		TsysBytes::_4 })); return;
			case CppPrimitiveType::___int64:		result.Add(pa.tsys->PrimitiveOf({ TsysPrimitiveType::SInt,		TsysBytes::_8 })); return;
			case CppPrimitiveType::_long:			result.Add(pa.tsys->PrimitiveOf({ TsysPrimitiveType::SInt,		TsysBytes::_4 })); return;
			case CppPrimitiveType::_long_int:		result.Add(pa.tsys->PrimitiveOf({ TsysPrimitiveType::SInt,		TsysBytes::_4 })); return;
			case CppPrimitiveType::_long_long:		result.Add(pa.tsys->PrimitiveOf({ TsysPrimitiveType::SInt,		TsysBytes::_8 })); return;
			case CppPrimitiveType::_float:			result.Add(pa.tsys->PrimitiveOf({ TsysPrimitiveType::Float,		TsysBytes::_4 })); return;
			case CppPrimitiveType::_double:			result.Add(pa.tsys->PrimitiveOf({ TsysPrimitiveType::Float,		TsysBytes::_8 })); return;
			case CppPrimitiveType::_long_double:	result.Add(pa.tsys->PrimitiveOf({ TsysPrimitiveType::Float,		TsysBytes::_8 })); return;
			}
			break;
		case CppPrimitivePrefix::_signed:
			switch (self->primitive)
			{
			case CppPrimitiveType::_char:			result.Add(pa.tsys->PrimitiveOf({ TsysPrimitiveType::SChar,		TsysBytes::_1 })); return;
			case CppPrimitiveType::_wchar_t:		result.Add(pa.tsys->PrimitiveOf({ TsysPrimitiveType::SWChar,	TsysBytes::_2 })); return;
			case CppPrimitiveType::_char16_t:		result.Add(pa.tsys->PrimitiveOf({ TsysPrimitiveType::SChar,		TsysBytes::_2 })); return;
			case CppPrimitiveType::_char32_t:		result.Add(pa.tsys->PrimitiveOf({ TsysPrimitiveType::SChar,		TsysBytes::_4 })); return;
			case CppPrimitiveType::_short:			result.Add(pa.tsys->PrimitiveOf({ TsysPrimitiveType::SInt,		TsysBytes::_2 })); return;
			case CppPrimitiveType::_int:			result.Add(pa.tsys->PrimitiveOf({ TsysPrimitiveType::SInt,		TsysBytes::_4 })); return;
			case CppPrimitiveType::___int8:			result.Add(pa.tsys->PrimitiveOf({ TsysPrimitiveType::SInt,		TsysBytes::_1 })); return;
			case CppPrimitiveType::___int16:		result.Add(pa.tsys->PrimitiveOf({ TsysPrimitiveType::SInt,		TsysBytes::_2 })); return;
			case CppPrimitiveType::___int32:		result.Add(pa.tsys->PrimitiveOf({ TsysPrimitiveType::SInt,		TsysBytes::_4 })); return;
			case CppPrimitiveType::___int64:		result.Add(pa.tsys->PrimitiveOf({ TsysPrimitiveType::SInt,		TsysBytes::_8 })); return;
			case CppPrimitiveType::_long:			result.Add(pa.tsys->PrimitiveOf({ TsysPrimitiveType::SInt,		TsysBytes::_4 })); return;
			case CppPrimitiveType::_long_int:		result.Add(pa.tsys->PrimitiveOf({ TsysPrimitiveType::SInt,		TsysBytes::_4 })); return;
			case CppPrimitiveType::_long_long:		result.Add(pa.tsys->PrimitiveOf({ TsysPrimitiveType::SInt,		TsysBytes::_8 })); return;
			}
			break;
		case CppPrimitivePrefix::_unsigned:
			switch (self->primitive)
			{
			case CppPrimitiveType::_char:			result.Add(pa.tsys->PrimitiveOf({ TsysPrimitiveType::UChar,		TsysBytes::_1 })); return;
			case CppPrimitiveType::_wchar_t:		result.Add(pa.tsys->PrimitiveOf({ TsysPrimitiveType::UWChar,	TsysBytes::_2 })); return;
			case CppPrimitiveType::_char16_t:		result.Add(pa.tsys->PrimitiveOf({ TsysPrimitiveType::UChar,		TsysBytes::_2 })); return;
			case CppPrimitiveType::_char32_t:		result.Add(pa.tsys->PrimitiveOf({ TsysPrimitiveType::UChar,		TsysBytes::_4 })); return;
			case CppPrimitiveType::_short:			result.Add(pa.tsys->PrimitiveOf({ TsysPrimitiveType::UInt,		TsysBytes::_2 })); return;
			case CppPrimitiveType::_int:			result.Add(pa.tsys->PrimitiveOf({ TsysPrimitiveType::UInt,		TsysBytes::_4 })); return;
			case CppPrimitiveType::___int8:			result.Add(pa.tsys->PrimitiveOf({ TsysPrimitiveType::UInt,		TsysBytes::_1 })); return;
			case CppPrimitiveType::___int16:		result.Add(pa.tsys->PrimitiveOf({ TsysPrimitiveType::UInt,		TsysBytes::_2 })); return;
			case CppPrimitiveType::___int32:		result.Add(pa.tsys->PrimitiveOf({ TsysPrimitiveType::UInt,		TsysBytes::_4 })); return;
			case CppPrimitiveType::___int64:		result.Add(pa.tsys->PrimitiveOf({ TsysPrimitiveType::UInt,		TsysBytes::_8 })); return;
			case CppPrimitiveType::_long:			result.Add(pa.tsys->PrimitiveOf({ TsysPrimitiveType::UInt,		TsysBytes::_4 })); return;
			case CppPrimitiveType::_long_int:		result.Add(pa.tsys->PrimitiveOf({ TsysPrimitiveType::UInt,		TsysBytes::_4 })); return;
			case CppPrimitiveType::_long_long:		result.Add(pa.tsys->PrimitiveOf({ TsysPrimitiveType::UInt,		TsysBytes::_8 })); return;
			}
			break;
		}
		throw NotConvertableException();
	}

	void Visit(ReferenceType* self)override
	{
		throw NotConvertableException();
	}

	void Visit(ArrayType* self)override
	{
		throw NotConvertableException();
	}

	void Visit(CallingConventionType* self)override
	{
		throw NotConvertableException();
	}

	void Visit(FunctionType* self)override
	{
		throw NotConvertableException();
	}

	void Visit(MemberType* self)override
	{
		throw NotConvertableException();
	}

	void Visit(DeclType* self)override
	{
		throw NotConvertableException();
	}

	void Visit(DecorateType* self)override
	{
		throw NotConvertableException();
	}

	void Visit(RootType* self)override
	{
		throw NotConvertableException();
	}

	void Visit(IdType* self)override
	{
		throw NotConvertableException();
	}

	void Visit(ChildType* self)override
	{
		throw NotConvertableException();
	}

	void Visit(GenericType* self)override
	{
		throw NotConvertableException();
	}

	void Visit(VariadicTemplateArgumentType* self)override
	{
		throw NotConvertableException();
	}
};

// Convert type AST to type system object
void TypeToTsys(ParsingArguments& pa, Ptr<Type> t, List<ITsys*>& tsys)
{
	if (!t) throw NotConvertableException();
	TypeToTsysVisitor visitor(pa, tsys);
	t->Accept(&visitor);
}