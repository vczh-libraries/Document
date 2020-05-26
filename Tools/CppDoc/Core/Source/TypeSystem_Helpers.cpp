#include "Ast_Resolving.h"

ITsys* ApplyExprTsysType(ITsys* tsys, ExprTsysType type)
{
	switch (type)
	{
	case ExprTsysType::LValue:
		return tsys->LRefOf();
	case ExprTsysType::XValue:
		return tsys->RRefOf();
	default:
		return tsys;
	}
}

ITsys* CvRefOf(ITsys* tsys, TsysCV cv, TsysRefType refType)
{
	switch (refType)
	{
	case TsysRefType::LRef:
		return tsys->CVOf(cv)->LRefOf();
	case TsysRefType::RRef:
		return tsys->CVOf(cv)->RRefOf();
	default:
		return tsys->CVOf(cv);
	}
}

ITsys* GetThisEntity(ITsys* thisType)
{
	TsysCV cv;
	TsysRefType ref;
	auto thisEntity = thisType->GetEntity(cv, ref);
	if (thisEntity->GetType() == TsysType::Ptr)
	{
		thisEntity = thisEntity->GetElement();
	}
	return thisEntity;
}

ITsys* ReplaceThisTypeInternal(ITsys* thisType, ITsys* entity)
{
	TsysCV cv;
	TsysRefType ref;
	auto thisEntity = thisType->GetEntity(cv, ref);
	if (thisEntity->GetType() == TsysType::Ptr)
	{
		return CvRefOf(ReplaceThisTypeInternal(thisType->GetElement(), entity), cv, ref)->PtrOf();
	}
	else
	{
		return CvRefOf(entity, cv, ref);
	}
}

ITsys* ReplaceThisType(ITsys* thisType, ITsys* entity)
{
	if (entity)
	{
		return ReplaceThisTypeInternal(thisType, GetThisEntity(entity));
	}
	else
	{
		return nullptr;
	}
}

TypeConv TestFunctionQualifier(TsysCV thisCV, TsysRefType thisRef, Ptr<FunctionType> funcType)
{
	bool tC = thisCV.isGeneralConst;
	bool dC = funcType->qualifierConstExpr || funcType->qualifierConst;
	bool tV = thisCV.isVolatile;
	bool dV = funcType->qualifierVolatile;
	bool tL = thisRef == TsysRefType::LRef;
	bool dL = funcType->qualifierLRef;
	bool tR = thisRef == TsysRefType::RRef;
	bool dR = funcType->qualifierRRef;

	if (tC && !dC || tV && !dV || tL && dR || tR && dL) return TypeConvCat::Illegal;
	if (tC == dC && tV == dV && ((tL == dL && tR == dR) || (!dL && !dR))) return TypeConvCat::Exact;
	return TypeConvCat::Trivial;
}