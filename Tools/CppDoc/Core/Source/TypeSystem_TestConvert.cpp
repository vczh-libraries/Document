#include "TypeSystem.h"
#include "Parser.h"
#include "Ast_Decl.h"
#include "Ast_Type.h"
#include "Ast_Resolving.h"

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
	return TsysConv::TrivalConversion;
}

using TCITestedSet = SortedList<Tuple<ITsys*, ITsys*>>;
TsysConv TestConvertInternal(const ParsingArguments& pa, ITsys* toType, ITsys* fromType, TCITestedSet& tested);

namespace TestConvert_Helpers
{
	bool IsCVSame(TsysCV toCV, TsysCV fromCV)
	{
		if (toCV.isGeneralConst != fromCV.isGeneralConst) return false;
		if (toCV.isVolatile != fromCV.isVolatile) return false;
		return true;
	}

	bool IsCVMatch(TsysCV toCV, TsysCV fromCV)
	{
		if (!toCV.isGeneralConst && fromCV.isGeneralConst) return false;
		if (!toCV.isVolatile && fromCV.isVolatile) return false;
		return true;
	}

	bool IsExactMatch(ITsys* toType, ITsys* fromType, bool& isAny)
	{
		if (toType == fromType)
		{
			return true;
		}
		else
		{
			return false;
		}
	}

	bool IsPointerOfTypeConvertable(ITsys* toType, ITsys* fromType, bool& isTrivial, bool& isAny)
	{
		if (toType == fromType)
		{
			return true;
		}

		TsysCV toCV, fromCV;
		TsysRefType toRef, fromRef;
		auto toEntity = toType->GetEntity(toCV, toRef);
		auto fromEntity = fromType->GetEntity(fromCV, fromRef);

		if (!IsCVSame(toCV, fromCV))
		{
			if (IsCVMatch(toCV, fromCV))
			{
				isTrivial = true;
			}
			else
			{
				return false;
			}
		}

		if (toEntity->IsUnknownType() || fromEntity->IsUnknownType())
		{
			if (toEntity->GetType() != TsysType::Member && fromEntity->GetType() != TsysType::Member)
			{
				isAny = true;
				return true;
			}
		}

		if (IsExactMatch(toEntity, fromEntity, isAny))
		{
			if (isAny)
			{
				isTrivial = false;
			}
			return true;
		}
		else
		{
			isAny = false;
			isTrivial = false;
			return false;
		}
	}

	template<typename T>
	Ptr<T> TryGetDeclFromType(ITsys* type)
	{
		if (type->GetType() != TsysType::Decl) return false;
		return type->GetDecl()->definition.Cast<T>();
	}

	template<typename T>
	Ptr<T> TryGetForwardDeclFromType(ITsys* type)
	{
		if (type->GetType() != TsysType::Decl) return false;
		return type->GetDecl()->GetAnyForwardDecl<T>();
	}

	bool IsExactOrTrivalConvert(ITsys* toType, ITsys* fromType, bool fromLRP, bool& isTrivial, bool& isAny)
	{
		TsysCV toCV, fromCV;
		TsysRefType toRef, fromRef;
		auto toEntity = toType->GetEntity(toCV, toRef);
		auto fromEntity = fromType->GetEntity(fromCV, fromRef);

		if (toEntity->IsUnknownType() || fromEntity->IsUnknownType())
		{
			isAny = true;
			return true;
		}

		if (toRef == TsysRefType::LRef && toCV.isGeneralConst)
		{
			fromLRP = true;
		}
		else switch (toRef)
		{
		case TsysRefType::LRef:
			switch (fromRef)
			{
			case TsysRefType::LRef:
				fromLRP = true;
				break;
			case TsysRefType::RRef:
			case TsysRefType::None:
				return false;
			}
			break;
		case TsysRefType::RRef:
			switch (fromRef)
			{
			case TsysRefType::LRef:
				return false;
			case TsysRefType::RRef:
			case TsysRefType::None:
				fromLRP = true;
				break;
			}
			break;
		}

		if (fromLRP)
		{
			if (!IsCVSame(toCV, fromCV))
			{
				if (IsCVMatch(toCV, fromCV))
				{
					isTrivial = true;
				}
				else
				{
					return false;
				}
			}
		}

		if (toEntity == fromEntity)
		{
			return true;
		}

		if (toEntity->GetType() == TsysType::Primitive && fromEntity->GetType() == TsysType::Decl)
		{
			if (auto decl = TryGetForwardDeclFromType<ForwardEnumDeclaration>(fromEntity))
			{
				if (decl->enumClass) return false;

				auto primitive = toEntity->GetPrimitive();
				if (primitive.type != TsysPrimitiveType::SInt) return false;
				if (primitive.bytes != TsysBytes::_4) return false;

				isTrivial = true;
				return true;
			}
		}

		if ((toEntity->GetType() == TsysType::Ptr && fromEntity->GetType() == TsysType::Ptr) ||
			(toEntity->GetType() == TsysType::Ptr && fromEntity->GetType() == TsysType::Array) ||
			(toEntity->GetType() == TsysType::Array && fromEntity->GetType() == TsysType::Array))
		{
			if (IsPointerOfTypeConvertable(toEntity->GetElement(), fromEntity->GetElement(), isTrivial, isAny))
			{
				return true;
			}
		}
		else
		{
			if (IsPointerOfTypeConvertable(toEntity, fromEntity, isTrivial, isAny))
			{
				return true;
			}
		}

		return false;
	}

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
				vint index = toSymbol->children.Keys().IndexOf(L"$__ctor");
				if (index == -1)
				{
					return TsysConv::Illegal;
				}

				const auto& ctors = toSymbol->children.GetByIndex(index);
				ExprTsysList funcTypes;
				for (vint i = 0; i < ctors.Count(); i++)
				{
					auto ctorSymbol = ctors[i];
					auto ctorDecl = ctorSymbol->GetAnyForwardDecl<ForwardFunctionDeclaration>();
					if (ctorDecl->decoratorDelete) continue;
					symbol_type_resolving::EvaluateSymbol(pa, ctorDecl.Obj());

					for (vint j = 0; j < ctorSymbol->evaluation.Get().Count(); j++)
					{
						auto tsys = ctorSymbol->evaluation.Get()[j];
						funcTypes.Add({ ctorSymbol.Obj(),ExprTsysType::PRValue,tsys });
					}
				}

				List<Ptr<ExprTsysList>> argTypesList;
				for (vint i = 0; i < fromEntity->GetParamCount(); i++)
				{
					argTypesList.Add(MakePtr<ExprTsysList>());
					argTypesList[i]->Add({ nullptr,init.types[i],fromEntity->GetParam(i) });
				}

				ExprTsysList result;
				symbol_type_resolving::VisitOverloadedFunction(pa, funcTypes, argTypesList, result, nullptr);
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
		TsysCV cv;
		TsysRefType ref;
		auto entity = type->GetEntity(cv, ref);
		if (entity->GetType() == TsysType::Init)
		{
			return TsysConv::Illegal;
		}

		switch (init.types[0])
		{
		case ExprTsysType::LValue:
			type = type->LRefOf();
			break;
		case ExprTsysType::XValue:
			type = type->RRefOf();
			break;
		}

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
				symbol_type_resolving::EvaluateSymbol(pa, currentClass.Obj());
				for (vint j = 0; j < currentClass->symbol->evaluation.Count(); j++)
				{
					auto& baseTypes = currentClass->symbol->evaluation.Get(j);
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
		vint index = fromSymbol->children.Keys().IndexOf(L"$__type");
		if (index == -1) return false;
		const auto& typeOps = fromSymbol->children.GetByIndex(index);

		auto newPa = pa.WithContext(fromSymbol);
		for (vint i = 0; i < typeOps.Count(); i++)
		{
			auto typeOpSymbol = typeOps[i];
			auto typeOpDecl = typeOpSymbol->GetAnyForwardDecl<ForwardFunctionDeclaration>();
			{
				if (typeOpDecl->decoratorExplicit) continue;
				if (typeOpDecl->decoratorDelete) continue;
				auto typeOpType = GetTypeWithoutMemberAndCC(typeOpDecl->type).Cast<FunctionType>();
				if (!typeOpType) continue;
				if (typeOpType->parameters.Count() != 0) continue;
				if (TestFunctionQualifier(fromCV, fromRef, typeOpType) == TsysConv::Illegal) continue;
			}
			symbol_type_resolving::EvaluateSymbol(pa, typeOpDecl.Obj());

			for (vint j = 0; j < typeOpSymbol->evaluation.Get().Count(); j++)
			{
				auto tsys = typeOpSymbol->evaluation.Get()[j];
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

		vint index = toSymbol->children.Keys().IndexOf(L"$__ctor");
		if (index == -1) return false;
		const auto& ctors = toSymbol->children.GetByIndex(index);

		auto newPa = pa.WithContext(toSymbol);
		for (vint i = 0; i < ctors.Count(); i++)
		{
			auto ctorSymbol = ctors[i];
			auto ctorDecl = ctorSymbol->GetAnyForwardDecl<ForwardFunctionDeclaration>();
			{
				if (ctorDecl->decoratorExplicit) continue;
				if (ctorDecl->decoratorDelete) continue;
				auto ctorType = GetTypeWithoutMemberAndCC(ctorDecl->type).Cast<FunctionType>();
				if (!ctorType) continue;
				if (ctorType->parameters.Count() != 1) continue;
			}
			symbol_type_resolving::EvaluateSymbol(pa, ctorDecl.Obj());

			for (vint j = 0; j < ctorSymbol->evaluation.Get().Count(); j++)
			{
				auto tsys = ctorSymbol->evaluation.Get()[j];
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
		if (toType->GetType() == TsysType::Ptr) return TsysConv::TrivalConversion;
		fromType = pa.tsys->Int();
	}

	if (fromType->GetType() == TsysType::Nullptr)
	{
		if (toType->GetType() == TsysType::Ptr) return TsysConv::Exact;
	}

	{
		bool isTrivial = false;
		bool isAny = false;
		if (IsExactOrTrivalConvert(toType, fromType, false, isTrivial, isAny))
		{
			return
				isAny ? TsysConv::Any :
				isTrivial ? TsysConv::TrivalConversion :
				TsysConv::Exact;
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