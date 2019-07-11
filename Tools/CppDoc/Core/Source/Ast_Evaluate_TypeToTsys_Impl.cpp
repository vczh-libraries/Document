#include "Ast_Expr.h"
#include "Ast_Resolving.h"

namespace symbol_typetotsys_impl
{

	//////////////////////////////////////////////////////////////////////////////////////
	// ReferenceType
	//////////////////////////////////////////////////////////////////////////////////////

	ITsys* ProcessReferenceType(ReferenceType* self, ExprTsysItem arg)
	{
		switch (self->reference)
		{
		case CppReferenceType::LRef:
			return arg.tsys->LRefOf();
		case CppReferenceType::RRef:
			return arg.tsys->RRefOf();
		default:
			return arg.tsys->PtrOf();
		}
	}

	//////////////////////////////////////////////////////////////////////////////////////
	// ArrayType
	//////////////////////////////////////////////////////////////////////////////////////

	ITsys* ProcessArrayType(ArrayType* self, ExprTsysItem arg)
	{
		if (arg.tsys->GetType() == TsysType::Array)
		{
			return arg.tsys->GetElement()->ArrayOf(arg.tsys->GetParamCount() + 1);
		}
		else
		{
			return arg.tsys->ArrayOf(1);
		}
	}

	//////////////////////////////////////////////////////////////////////////////////////
	// DecorateType
	//////////////////////////////////////////////////////////////////////////////////////

	ITsys* ProcessDecorateType(DecorateType* self, ExprTsysItem arg)
	{
		return arg.tsys->CVOf({ (self->isConstExpr || self->isConst), self->isVolatile });
	}
}