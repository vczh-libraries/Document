#include "TypeSystem.h"
#include "Parser.h"
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

TsysConv TestFunctionQualifier(TsysCV thisCV, TsysRefType thisRef, Ptr<FunctionType> funcType)
{
	bool tC = thisCV.isGeneralConst;
	bool dC = funcType->qualifierConstExpr || funcType->qualifierConst;
	bool tV = thisCV.isVolatile;
	bool dV = funcType->qualifierVolatile;
	bool tL = thisRef == TsysRefType::LRef;
	bool dL = funcType->qualifierLRef;
	bool tR = thisRef == TsysRefType::RRef;
	bool dR = funcType->qualifierRRef;

	if (tC && !dC || tV && !dV || tL && dR || tR && dL) return TsysConv::Illegal;
	if (tC == dC && tV == dV && ((tL == dL && tR == dR) || (!dL && !dR))) return TsysConv::Exact;
	return TsysConv::TrivialConversion;
}

using TCITestedSet = SortedList<Tuple<ITsys*, ITsys*>>;
TsysConv TestConvertInternal(const ParsingArguments& pa, ITsys* toType, ITsys* fromType, TCITestedSet& tested);

namespace TestConvert_Helpers
{
	extern bool IsCVMatch(TsysCV toCV, TsysCV fromCV);
	extern bool IsExactOrTrivialConvert(ITsys* toType, ITsys* fromType, bool& isTrivial, bool& isAny);

	bool IsEntityConversionAllowed(ITsys* toEntity, TsysCV toCV, TsysRefType toRef, ITsys* fromEntity, TsysCV fromCV, TsysRefType fromRef)
	{
		if (toRef == TsysRefType::LRef && toCV.isGeneralConst)
		{
			return true;
		}

		if (toRef == TsysRefType::None)
		{
			return true;
		}

		if (toRef == TsysRefType::LRef || fromRef == TsysRefType::LRef)
		{
			if (!TryGetDeclFromType<ClassDeclaration>(toEntity) || !TryGetDeclFromType<ClassDeclaration>(fromEntity))
			{
				return false;
			}
		}

		return true;
	}

	TsysConv IsUniversalInitialization(const ParsingArguments& pa, ITsys* toType, ITsys* fromEntity, const TsysInit& init, TCITestedSet& tested)
	{
		TsysCV toCV;
		TsysRefType toRef;
		auto toEntity = toType->GetEntity(toCV, toRef);

		if (toEntity->GetType() == TsysType::Decl)
		{
			if (auto toDecl = TryGetDeclFromType<ClassDeclaration>(toEntity))
			{
				auto toSymbol = toDecl->symbol;
				auto pCtors = toSymbol->TryGetChildren_NFb(L"$__ctor");
				if (!pCtors) return TsysConv::Illegal;

				ExprTsysList funcTypes;
				for (vint i = 0; i < pCtors->Count(); i++)
				{
					auto ctorSymbol = pCtors->Get(i);
					auto ctorDecl = ctorSymbol->GetAnyForwardDecl<ForwardFunctionDeclaration>();
					if (ctorDecl->decoratorDelete) continue;
					auto& evTypes = symbol_type_resolving::EvaluateFuncSymbol(pa, ctorDecl.Obj());

					for (vint j = 0; j < evTypes.Count(); j++)
					{
						auto tsys = evTypes[j];
						funcTypes.Add({ ctorSymbol.Obj(),ExprTsysType::PRValue,tsys });
					}
				}

				Array<ExprTsysItem> argTypesList(fromEntity->GetParamCount());
				for (vint i = 0; i < fromEntity->GetParamCount(); i++)
				{
					argTypesList[i] = { init.headers[i],fromEntity->GetParam(i) };
				}

				SortedList<vint> boundedAnys;
				ExprTsysList result;
				symbol_type_resolving::VisitOverloadedFunction(pa, funcTypes, argTypesList, boundedAnys, result, nullptr);
				if (result.Count() > 0)
				{
					return TsysConv::UserDefinedConversion;
				}
				else
				{
					return TsysConv::Illegal;
				}
			}
		}

		if (fromEntity->GetParamCount() == 0)
		{
			return TsysConv::Exact;
		}

		if (fromEntity->GetParamCount() != 1)
		{
			return TsysConv::Illegal;
		}

		auto type = fromEntity->GetParam(0);
		{
			TsysCV cv;
			TsysRefType ref;
			auto entity = type->GetEntity(cv, ref);
			if (entity->GetType() == TsysType::Init)
			{
				return TsysConv::Illegal;
			}
		}
		type = ApplyExprTsysType(type, init.headers[0].type);

		if (tested.Contains({ toType,type }))
		{
			return TsysConv::Illegal;
		}
		else
		{
			return TestConvertInternal(pa, toType, type, tested);
		}
	}

	bool IsNumericPromotion(ITsys* toType, ITsys* fromType)
	{
		if (toType->GetType() != TsysType::Primitive) return false;
		auto toP = toType->GetPrimitive();
		if (toP.type == TsysPrimitiveType::Void) return false;

		if (auto decl = TryGetForwardDeclFromType<ForwardEnumDeclaration>(fromType))
		{
			if (decl->enumClass) return false;
			return true;
		}

		if (fromType->GetType() != TsysType::Primitive) return false;
		auto fromP = fromType->GetPrimitive();
		if (fromP.type == TsysPrimitiveType::Void) return false;

		bool toF = toP.type == TsysPrimitiveType::Float;
		bool fromF = fromP.type == TsysPrimitiveType::Float;
		if (toF != fromF) return false;
		return toP.bytes > fromP.bytes;
	}

	bool IsNumericConversion(ITsys* toType, ITsys* fromType)
	{
		if (toType->GetType() != TsysType::Primitive) return false;
		if (fromType->GetType() != TsysType::Primitive) return false;

		auto toP = toType->GetPrimitive();
		auto fromP = fromType->GetPrimitive();

		if (toP.type == TsysPrimitiveType::Void) return false;
		if (fromP.type == TsysPrimitiveType::Void) return false;

		return true;
	}

	bool IsPointerConversion(ITsys* toType, ITsys* fromType)
	{
		if (toType->GetType() == TsysType::Ptr && fromType->GetType() == TsysType::Ptr)
		{
			TsysCV toCV, fromCV;
			TsysRefType toRef, fromRef;
			auto toEntity = toType->GetElement()->GetEntity(toCV, toRef);
			auto fromEntity = fromType->GetElement()->GetEntity(fromCV, fromRef);

			if (toEntity->GetType() == TsysType::Primitive && toEntity->GetPrimitive().type == TsysPrimitiveType::Void)
			{
				switch (fromEntity->GetType())
				{
				case TsysType::Function: return true;
				case TsysType::Member: return false;
				default: return IsCVMatch(toCV, fromCV);
				}
			}
		}
		return false;
	}

	bool IsEntityTypeInheriting(const ParsingArguments& pa, ITsys* toType, ITsys* fromType)
	{
		TsysCV toCV, fromCV;
		TsysRefType toRef, fromRef;
		toType = toType->GetEntity(toCV, toRef);
		fromType = fromType->GetEntity(fromCV, fromRef);

		if (toType->GetType() == TsysType::Ptr && fromType->GetType() == TsysType::Ptr)
		{
			toType = toType->GetElement()->GetEntity(toCV, toRef);
			fromType = fromType->GetElement()->GetEntity(fromCV, fromRef);
		}

		if (!TryGetDeclFromType<ClassDeclaration>(toType)) return false;

		List<ITsys*> searched;
		searched.Add(fromType);
		for (vint i = 0; i < searched.Count(); i++)
		{
			auto currentType = searched[i];
			if (currentType == toType) return true;
			if (auto currentClass = TryGetDeclFromType<ClassDeclaration>(currentType))
			{
				auto& ev = symbol_type_resolving::EvaluateClassSymbol(pa, currentClass.Obj());
				for (vint j = 0; j < ev.Count(); j++)
				{
					auto& baseTypes = ev.Get(j);
					for (vint k = 0; k < baseTypes.Count(); k++)
					{
						if (!searched.Contains(baseTypes[k]))
						{
							searched.Add(baseTypes[k]);
						}
					}
				}
			}
		}
		return false;
	}

	bool IsToBaseClassConversion_AssumingInheriting(const ParsingArguments& pa, ITsys* toType, ITsys* fromType)
	{
		{
			TsysCV toCV, fromCV;
			TsysRefType toRef, fromRef;
			auto toEntity = toType->GetEntity(toCV, toRef);
			auto fromEntity = fromType->GetEntity(fromCV, fromRef);

			if (toRef != TsysRefType::None && fromRef != TsysRefType::None)
			{
				if ((toRef != TsysRefType::LRef) != (fromRef != TsysRefType::LRef))
				{
					return false;
				}

				if (!IsCVMatch(toCV, fromCV))
				{
					return false;
				}
			}

			toType = toEntity;
			fromType = fromEntity;
			if (toRef != TsysRefType::None && fromRef != TsysRefType::None)
			{
				return true;
			}
		}

		if (toType->GetType() == TsysType::Ptr && fromType->GetType() == TsysType::Ptr)
		{
			TsysCV toCV, fromCV;
			TsysRefType toRef, fromRef;
			auto toEntity = toType->GetElement()->GetEntity(toCV, toRef);
			auto fromEntity = fromType->GetElement()->GetEntity(fromCV, fromRef);

			if (toRef != TsysRefType::None) return false;
			if (fromRef != TsysRefType::None) return false;
			if (!IsCVMatch(toCV, fromCV)) return false;

			toType = toEntity;
			fromType = fromEntity;
			return true;
		}
		else
		{
			return false;
		}
	}

	bool IsCustomOperatorConversion(const ParsingArguments& pa, ITsys* toType, ITsys* fromType, TCITestedSet& tested)
	{
		TsysCV fromCV;
		TsysRefType fromRef;
		auto fromEntity = fromType->GetEntity(fromCV, fromRef);

		auto fromClass = TryGetDeclFromType<ClassDeclaration>(fromEntity);
		if (!fromClass) return false;

		auto fromSymbol = fromClass->symbol;
		auto pTypeOps = fromSymbol->TryGetChildren_NFb(L"$__type");
		if (!pTypeOps) return false;

		auto newPa = pa.WithContext(fromSymbol);
		for (vint i = 0; i < pTypeOps->Count(); i++)
		{
			auto typeOpSymbol = pTypeOps->Get(i);
			auto typeOpDecl = typeOpSymbol->GetAnyForwardDecl<ForwardFunctionDeclaration>();
			{
				if (typeOpDecl->decoratorExplicit) continue;
				if (typeOpDecl->decoratorDelete) continue;
				auto typeOpType = GetTypeWithoutMemberAndCC(typeOpDecl->type).Cast<FunctionType>();
				if (!typeOpType) continue;
				if (typeOpType->parameters.Count() != 0) continue;
				if (TestFunctionQualifier(fromCV, fromRef, typeOpType) == TsysConv::Illegal) continue;
			}
			auto& evTypes = symbol_type_resolving::EvaluateFuncSymbol(pa, typeOpDecl.Obj());

			for (vint j = 0; j < evTypes.Count(); j++)
			{
				auto tsys = evTypes[j];
				{
					auto newFromType = tsys->GetElement()->RRefOf();
					if (!tested.Contains({ toType,newFromType }))
					{
						if (TestConvertInternal(newPa, toType, newFromType, tested) != TsysConv::Illegal)
						{
							return true;
						}
					}
				}
			}
		}

		return false;
	}

	bool IsCustomContructorConversion(const ParsingArguments& pa, ITsys* toType, ITsys* fromType, TCITestedSet& tested)
	{
		TsysCV toCV;
		TsysRefType toRef;
		auto toEntity = toType->GetEntity(toCV, toRef);
		if (toRef == TsysRefType::LRef && !toCV.isGeneralConst) return false;

		auto toClass = TryGetDeclFromType<ClassDeclaration>(toEntity);
		if (!toClass) return false;

		auto toSymbol = toClass->symbol;
		{
			auto newFromType = pa.tsys->DeclOf(toSymbol)->RRefOf();
			if (!tested.Contains({ toType,newFromType }))
			{
				if (TestConvertInternal(pa, toType, newFromType, tested) == TsysConv::Illegal)
				{
					return false;
				}
			}
		}

		auto pCtors = toSymbol->TryGetChildren_NFb(L"$__ctor");
		if (!pCtors) return false;

		auto newPa = pa.WithContext(toSymbol);
		for (vint i = 0; i < pCtors->Count(); i++)
		{
			auto ctorSymbol = pCtors->Get(i);
			auto ctorDecl = ctorSymbol->GetAnyForwardDecl<ForwardFunctionDeclaration>();
			{
				if (ctorDecl->decoratorExplicit) continue;
				if (ctorDecl->decoratorDelete) continue;
				auto ctorType = GetTypeWithoutMemberAndCC(ctorDecl->type).Cast<FunctionType>();
				if (!ctorType) continue;
				if (ctorType->parameters.Count() != 1) continue;
			}
			auto& evTypes = symbol_type_resolving::EvaluateFuncSymbol(pa, ctorDecl.Obj());

			for (vint j = 0; j < evTypes.Count(); j++)
			{
				auto tsys = evTypes[j];
				{
					auto newToType = tsys->GetParam(0);
					if (!tested.Contains({ newToType,fromType }))
					{
						if (TestConvertInternal(newPa, newToType, fromType, tested) != TsysConv::Illegal)
						{
							return true;
						}
					}
				}
			}
		}

		return false;
	}
}
using namespace TestConvert_Helpers;

TsysConv TestConvertInternalUnsafe(const ParsingArguments& pa, ITsys* toType, ITsys* fromType, TCITestedSet& tested)
{
	if (fromType->GetType() == TsysType::Zero)
	{
		if (toType->GetType() == TsysType::Ptr) return TsysConv::TrivialConversion;
		fromType = pa.tsys->Int();
	}

	if (fromType->GetType() == TsysType::Nullptr)
	{
		if (toType->GetType() == TsysType::Ptr) return TsysConv::Exact;
	}

	{
		bool isTrivial = false;
		bool isAny = false;
		if (IsExactOrTrivialConvert(toType, fromType, isTrivial, isAny))
		{
			return
				isAny ? TsysConv::Any :
				isTrivial ? TsysConv::TrivialConversion :
				TsysConv::Exact;
		}
		else if (toType->HasUnknownType() || fromType->HasUnknownType())
		{
			return TsysConv::Illegal;
		}
	}

	TsysCV fromCV, toCV;
	TsysRefType fromRef, toRef;
	auto fromEntity = fromType->GetEntity(fromCV, fromRef);
	auto toEntity = toType->GetEntity(toCV, toRef);

	if (!IsEntityConversionAllowed(toEntity, toCV, toRef, fromEntity, fromCV, fromRef))
	{
		return TsysConv::Illegal;
	}

	if (fromEntity->GetType() == TsysType::Init)
	{
		auto& init = fromEntity->GetInit();
		return IsUniversalInitialization(pa, toType, fromEntity, init, tested);
	}

	if (IsNumericPromotion(toEntity, fromEntity))
	{
		return TsysConv::IntegralPromotion;
	}

	if (IsNumericConversion(toEntity, fromEntity))
	{
		return TsysConv::StandardConversion;
	}

	if (IsPointerConversion(toEntity, fromEntity))
	{
		return TsysConv::StandardConversion;
	}

	if (IsEntityTypeInheriting(pa, toType, fromType))
	{
		if (IsToBaseClassConversion_AssumingInheriting(pa, toType, fromType))
		{
			return TsysConv::StandardConversion;
		}
	}
	else
	{
		if (IsCustomOperatorConversion(pa, toType, fromType, tested))
		{
			return TsysConv::UserDefinedConversion;
		}
		if (IsCustomContructorConversion(pa, toType, fromType, tested))
		{
			return TsysConv::UserDefinedConversion;
		}
	}

	return TsysConv::Illegal;
}

TsysConv TestConvertInternal(const ParsingArguments& pa, ITsys* toType, ITsys* fromType, TCITestedSet& tested)
{
	Tuple<ITsys*, ITsys*> pair(toType, fromType);
	if (tested.Contains(pair))
	{
		// make sure caller doesn't call this function recursively for the same pair of type.
		struct TestConvertInternalStackOverflowException {};
		throw TestConvertInternalStackOverflowException();
	}

	vint index = tested.Add(pair);
	auto result = TestConvertInternalUnsafe(pa, toType, fromType, tested);
	tested.RemoveAt(index);
	return result;
}

TsysConv TestConvert(const ParsingArguments& pa, ITsys* toType, ExprTsysItem fromItem)
{
	TCITestedSet tested;
	return TestConvertInternal(pa, toType, (fromItem.type == ExprTsysType::LValue ? fromItem.tsys->LRefOf() : fromItem.tsys), tested);
}