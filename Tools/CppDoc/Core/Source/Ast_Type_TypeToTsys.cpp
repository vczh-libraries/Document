#include "Ast.h"
#include "Ast_Type.h"
#include "Ast_Expr.h"
#include "Ast_Decl.h"
#include "Parser.h"

/***********************************************************************
TypeToTsys
***********************************************************************/

class TypeToTsysVisitor : public Object, public virtual ITypeVisitor
{
public:
	TypeTsysList&			result;
	ParsingArguments&		pa;

	TypeToTsysVisitor(ParsingArguments& _pa, TypeTsysList& _result)
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
			case CppPrimitiveType::_char:			result.Add(pa.tsys->PrimitiveOf({ TsysPrimitiveType::SInt,		TsysBytes::_1 })); return;
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
			case CppPrimitiveType::_char:			result.Add(pa.tsys->PrimitiveOf({ TsysPrimitiveType::UInt,		TsysBytes::_1 })); return;
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
		self->type->Accept(this);
		for (vint i = 0; i < result.Count(); i++)
		{
			auto tsys = result[i];
			switch (self->reference)
			{
			case CppReferenceType::LRef:
				tsys = tsys->LRefOf();
				break;
			case CppReferenceType::RRef:
				tsys = tsys->RRefOf();
				break;
			case CppReferenceType::Ptr:
				tsys = tsys->PtrOf();
				break;
			}
			result[i] = tsys;
		}
	}

	void Visit(ArrayType* self)override
	{
		self->type->Accept(this);
		for (vint i = 0; i < result.Count(); i++)
		{
			auto tsys = result[i];
			if (tsys->GetType() == TsysType::Array)
			{
				tsys = tsys->GetElement()->ArrayOf(tsys->GetParamCount() + 1);
			}
			else
			{
				tsys = tsys->ArrayOf(1);
			}
			result[i] = tsys;
		}
	}

	void Visit(CallingConventionType* self)override
	{
		self->type->Accept(this);
	}

	void CreateFunctionType(TypeTsysList* tsyses, vint* tsysIndex, vint level, vint count)
	{
		if (level == count)
		{
			Array<ITsys*> params(count - 1);
			for (vint i = 0; i < count - 1; i++)
			{
				params[i] = tsyses[i + 1][tsysIndex[i + 1]];
			}
			result.Add(tsyses[0][tsysIndex[0]]->FunctionOf(params));
		}
		else
		{
			vint levelCount = tsyses[level].Count();
			for (vint i = 0; i < levelCount; i++)
			{
				tsysIndex[level] = i;
				CreateFunctionType(tsyses, tsysIndex, level + 1, count);
			}
		}
	}

	void Visit(FunctionType* self)override
	{
		TypeTsysList* tsyses = nullptr;
		vint* tsysIndex = nullptr;
		try
		{
			vint count = self->parameters.Count() + 1;
			tsyses = new TypeTsysList[count];
			if (self->decoratorReturnType)
			{
				TypeToTsys(pa, self->decoratorReturnType, tsyses[0]);
			}
			else if (self->returnType)
			{
				TypeToTsys(pa, self->returnType, tsyses[0]);
			}
			else
			{
				tsyses[0].Add(pa.tsys->PrimitiveOf({ TsysPrimitiveType::Void,TsysBytes::_1 }));
			}

			for (vint i = 0; i < self->parameters.Count(); i++)
			{
				TypeToTsys(pa, self->parameters[i]->type, tsyses[i + 1]);
			}

			tsysIndex = new vint[count];
			memset(tsysIndex, 0, sizeof(vint) * count);
			CreateFunctionType(tsyses, tsysIndex, 0, count);

			delete[] tsyses;
			delete[] tsysIndex;
		}
		catch (...)
		{
			delete[] tsyses;
			delete[] tsysIndex;
			throw;
		}
	}

	void Visit(MemberType* self)override
	{
		TypeTsysList types, classTypes;
		TypeToTsys(pa, self->type, types);
		TypeToTsys(pa, self->classType, classTypes);

		for (vint i = 0; i < types.Count(); i++)
		{
			for (vint j = 0; j < classTypes.Count(); j++)
			{
				result.Add(types[i]->MemberOf(classTypes[j]));
			}
		}
	}

	void Visit(DeclType* self)override
	{
		if (self->expr)
		{
			ExprTsysList types;
			ExprToTsys(pa, self->expr, types);
			for (vint i = 0; i < types.Count(); i++)
			{
				auto exprTsys = types[i].tsys;
				if (exprTsys->GetType() == TsysType::Zero)
				{
					exprTsys = pa.tsys->PrimitiveOf({ TsysPrimitiveType::SInt,TsysBytes::_4 });
				}
				if (!result.Contains(exprTsys))
				{
					result.Add(exprTsys);
				}
			}
		}
	}

	void Visit(DecorateType* self)override
	{
		self->type->Accept(this);
		for (vint i = 0; i < result.Count(); i++)
		{
			result[i] = result[i]->CVOf({ (self->isConstExpr || self->isConst), self->isVolatile });
		}
	}

	void Visit(RootType* self)override
	{
		throw NotConvertableException();
	}

	void CreateDeclType(Ptr<Resolving> resolving)
	{
		if (!resolving || resolving->resolvedSymbols.Count() == 0)
		{
			throw NotConvertableException();
		}
		resolving->Calibrate();

		for (vint i = 0; i < resolving->resolvedSymbols.Count(); i++)
		{
			auto symbol = resolving->resolvedSymbols[i];
			result.Add(pa.tsys->DeclOf(symbol));
		}
	}

	void Visit(IdType* self)override
	{
		CreateDeclType(self->resolving);
	}

	void Visit(ChildType* self)override
	{
		CreateDeclType(self->resolving);
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
void TypeToTsys(ParsingArguments& pa, Ptr<Type> t, TypeTsysList& tsys)
{
	if (!t) throw NotConvertableException();
	TypeToTsysVisitor visitor(pa, tsys);
	t->Accept(&visitor);
}