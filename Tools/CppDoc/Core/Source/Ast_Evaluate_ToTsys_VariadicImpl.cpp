#include "Ast_Resolving_ExpandPotentialVta.h"

using namespace symbol_type_resolving;

namespace symbol_totsys_impl
{
	//////////////////////////////////////////////////////////////////////////////////////
	// ProcessFunctionType
	//////////////////////////////////////////////////////////////////////////////////////

	void ProcessFunctionType(const ParsingArguments& pa, ExprTsysList& result, FunctionType* self, TsysCallingConvention cc, bool memberOf, Array<ExprTsysItem>& args, SortedList<vint>& boundedAnys)
	{
		if (boundedAnys.Count() > 0)
		{
			AddExprTsysItemToResult(result, GetExprTsysItem(pa.tsys->Any()));
		}
		else
		{
			Array<ITsys*> params(args.Count() - 1);
			for (vint i = 1; i < args.Count(); i++)
			{
				params[i - 1] = args[i].tsys;
			}

			TsysFunc func(cc, self->ellipsis);
			if (func.callingConvention == TsysCallingConvention::None)
			{
				func.callingConvention =
					memberOf && !func.ellipsis
					? TsysCallingConvention::ThisCall
					: TsysCallingConvention::CDecl
					;
			}

			auto funcTsys = args[0].tsys->FunctionOf(params, func);
			AddExprTsysItemToResult(result, GetExprTsysItem(funcTsys));
		}
	}

	//////////////////////////////////////////////////////////////////////////////////////
	// ProcessCtorAccessExpr
	//////////////////////////////////////////////////////////////////////////////////////

	void ProcessCtorAccessExpr(const ParsingArguments& pa, ExprTsysList& result, CtorAccessExpr* self, Array<ExprTsysItem>& args, SortedList<vint>& boundedAnys)
	{
		AddInternal(result, args[0]);
	}

	//////////////////////////////////////////////////////////////////////////////////////
	// ProcessNewExpr
	//////////////////////////////////////////////////////////////////////////////////////

	void ProcessNewExpr(const ParsingArguments& pa, ExprTsysList& result, NewExpr* self, Array<ExprTsysItem>& args, SortedList<vint>& boundedAnys)
	{
		auto type = args[0].tsys;
		if (type->GetType() == TsysType::Array)
		{
			if (type->GetParamCount() == 1)
			{
				AddTemp(result, type->GetElement()->PtrOf());
			}
			else
			{
				AddTemp(result, type->GetElement()->ArrayOf(type->GetParamCount() - 1)->PtrOf());
			}
		}
		else
		{
			AddTemp(result, type->PtrOf());
		}
	}

	//////////////////////////////////////////////////////////////////////////////////////
	// ProcessUniversalInitializerExpr
	//////////////////////////////////////////////////////////////////////////////////////

	void ProcessUniversalInitializerExpr(const ParsingArguments& pa, ExprTsysList& result, UniversalInitializerExpr* self, Array<ExprTsysItem>& args, SortedList<vint>& boundedAnys)
	{
		if (boundedAnys.Count() > 0)
		{
			AddTemp(result, pa.tsys->Any());
		}
		else
		{
			AddTemp(result, pa.tsys->InitOf(args));
		}
	}
}