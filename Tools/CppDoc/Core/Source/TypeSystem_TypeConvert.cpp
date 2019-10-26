#include "Ast_Resolving.h"

using TCITestedSet = Dictionary<Tuple<ITsys*, ITsys*>, Nullable<TypeConv>>;
TypeConv TestTypeConversionInternal(const ParsingArguments& pa, ITsys* toType, ITsys* fromType, TCITestedSet& tested);

namespace TestTypeConversion_Impl
{
	bool IsUnknownType(ITsys* type)
	{
		switch (type->GetType())
		{
		case TsysType::Any:
			return true;
		case TsysType::GenericArg:
			return true;
		case TsysType::GenericFunction:
			return type->GetGenericFunction().declSymbol->kind == symbol_component::SymbolKind::GenericTypeArgument;
		default:
			return false;
		}
	}

	bool IsCVEqual(TsysCV toCV, TsysCV fromCV)
	{
		if (toCV.isGeneralConst != fromCV.isGeneralConst) return false;
		if (toCV.isVolatile != fromCV.isVolatile) return false;
		return true;
	}

	bool IsCVCompatible(TsysCV toCV, TsysCV fromCV)
	{
		if (!toCV.isGeneralConst && fromCV.isGeneralConst) return false;
		if (!toCV.isVolatile && fromCV.isVolatile) return false;
		return true;
	}

	ITsys* ArrayRemoveOneDim(ITsys* tsys)
	{
		if (tsys->GetType() == TsysType::Array)
		{
			vint d = tsys->GetParamCount();
			if (d <= 1)
			{
				tsys = tsys->GetElement();
			}
			else
			{
				tsys = tsys->GetElement()->ArrayOf(d - 1);
			}
		}
		return tsys;
	}

	bool IsExactEntityMatch(const ParsingArguments& pa, ITsys* toEntity, ITsys* fromEntity, bool& anyInvolved)
	{
		if (IsUnknownType(toEntity) || IsUnknownType(fromEntity))
		{
			anyInvolved = true;
			return true;
		}
		if (toEntity->GetType() != fromEntity->GetType())
		{
			return false;
		}

		switch (toEntity->GetType())
		{
		case TsysType::Nullptr:
			return fromEntity->GetType() == TsysType::Nullptr;
		case TsysType::Primitive:
			{
				auto toP = toEntity->GetPrimitive();
				auto fromP = fromEntity->GetPrimitive();
				return toP.type == fromP.type && toP.bytes == fromP.bytes;
			}
		case TsysType::LRef:
		case TsysType::RRef:
		case TsysType::Ptr:
			return IsExactEntityMatch(pa, toEntity->GetElement(), fromEntity->GetElement(), anyInvolved);
		case TsysType::Array:
			return IsExactEntityMatch(
				pa,
				ArrayRemoveOneDim(toEntity),
				ArrayRemoveOneDim(fromEntity),
				anyInvolved
			);
		case TsysType::Function:
			{
				if (!IsExactEntityMatch(pa, toEntity->GetElement(), fromEntity->GetElement(), anyInvolved)) return false;
				if (toEntity->GetParamCount() != fromEntity->GetParamCount()) return false;
				vint count = toEntity->GetParamCount();
				for (vint i = 0; i < count; i++)
				{
					if (!IsExactEntityMatch(pa, toEntity->GetParam(i), fromEntity->GetParam(i), anyInvolved)) return false;
				}
				return true;
			}
		case TsysType::Member:
			return
				IsExactEntityMatch(pa, toEntity->GetElement(), fromEntity->GetElement(), anyInvolved) &&
				IsExactEntityMatch(pa, toEntity->GetClass(), fromEntity->GetClass(), anyInvolved);
		case TsysType::CV:
			{
				auto toCV = toEntity->GetCV();
				auto fromCV = fromEntity->GetCV();
				return IsCVEqual(toCV, fromCV);
			}
		case TsysType::Decl:
			return toEntity == fromEntity;
		case TsysType::DeclInstant:
			{
				const auto& toDI = toEntity->GetDeclInstant();
				const auto& fromDI = fromEntity->GetDeclInstant();
				if (toDI.declSymbol != fromDI.declSymbol) return false;

				if (toDI.parentDeclType && fromDI.parentDeclType)
				{
					if (!IsExactEntityMatch(pa, toDI.parentDeclType, fromDI.parentDeclType, anyInvolved)) return false;
				}

				vint count = toEntity->GetParamCount();
				for (vint i = 0; i < count; i++)
				{
					if (!IsExactEntityMatch(pa, toEntity->GetParam(i), fromEntity->GetParam(i), anyInvolved)) return false;
				}
			}
			break;
		case TsysType::Init:
			{
				if (toEntity->GetParamCount() != fromEntity->GetParamCount()) return false;
				vint count = toEntity->GetParamCount();
				const auto& toI = toEntity->GetInit();
				const auto& fromI = fromEntity->GetInit();
				for (vint i = 0; i < count; i++)
				{
					if (!IsExactEntityMatch(
						pa,
						ApplyExprTsysType(toEntity->GetParam(i), toI.headers[i].type),
						ApplyExprTsysType(fromEntity->GetParam(i), fromI.headers[i].type),
						anyInvolved
					))
					{
						return false;
					}
				}
				return true;
			}
		}
		return false;
	}

	TypeConv PerformExactEntityMatch(const ParsingArguments& pa, ITsys* toEntity, ITsys* fromEntity, TsysCV toCV, TsysCV fromCV, TsysRefType toRef, TsysRefType fromRef)
	{
		bool anyInvolved = false;
		if (IsExactEntityMatch(pa, toEntity, fromEntity, anyInvolved))
		{
			switch (toRef)
			{
			case TsysRefType::None:
				return { TypeConvCat::Exact,false,anyInvolved };
			case TsysRefType::LRef:
				if (toCV.isGeneralConst || fromRef == TsysRefType::LRef)
				{
					if (IsCVEqual(toCV, fromCV))
					{
						return { TypeConvCat::Exact,false,anyInvolved };
					}
					else if (IsCVCompatible(toCV, fromCV))
					{
						return { TypeConvCat::Trivial,false,anyInvolved };
					}
				}
				break;
			case TsysRefType::RRef:
				if (fromRef != TsysRefType::LRef)
				{
					if (IsCVEqual(toCV, fromCV))
					{
						return { TypeConvCat::Exact,false,anyInvolved };
					}
					else if (IsCVCompatible(toCV, fromCV))
					{
						return { TypeConvCat::Trivial,false,anyInvolved };
					}
				}
			}
		}
		return TypeConvCat::Illegal;
	}

	bool AllowEntityTypeChanging(TsysCV toCV, TsysCV fromCV, TsysRefType toRef, TsysRefType fromRef)
	{
		switch (toRef)
		{
		case TsysRefType::LRef:
			if (!toCV.isGeneralConst)
			{
				return false;
			}
			break;
		case TsysRefType::RRef:
			if (fromRef == TsysRefType::LRef)
			{
				return false;
			}
		}

		if (toRef != TsysRefType::None)
		{
			if (!IsCVCompatible(toCV, fromCV))
			{
				return false;
			}
		}
		return true;
	}

	TypeConv PetformToPtrTrivialConversion(const ParsingArguments& pa, ITsys* toEntity, ITsys* fromEntity, TsysCV toCV, TsysCV fromCV, TsysRefType toRef, TsysRefType fromRef)
	{
		if (!AllowEntityTypeChanging(toCV, fromCV, toRef, fromRef))
		{
			return TypeConvCat::Illegal;
		}

		if (toEntity->GetType() == TsysType::Ptr)
		{
			auto toElement = toEntity->GetElement();
			ITsys* fromElement = nullptr;
			if (fromEntity->GetType() == TsysType::Ptr)
			{
				fromElement = fromEntity->GetElement();
			}
			else if (fromEntity->GetType() == TsysType::Array)
			{
				fromElement = ArrayRemoveOneDim(fromEntity);
			}
			else
			{
				return TypeConvCat::Illegal;
			}

			TsysCV fromElementCV, toElementCV;
			TsysRefType fromElementRef, toElementRef;
			auto fromElementEntity = fromElement->GetEntity(fromElementCV, fromElementRef);
			auto toElementEntity = toElement->GetEntity(toElementCV, toElementRef);

			if (fromElementRef != TsysRefType::None || toElementRef != TsysRefType::None)
			{
				throw L"Pointer to reference is illegal.";
			}

			bool anyInvolved = false;
			if (IsExactEntityMatch(pa, toElementEntity, fromElementEntity, anyInvolved))
			{
				if (IsCVEqual(toElementCV, fromElementCV))
				{
					return { TypeConvCat::Exact,false,anyInvolved };
				}
				else
				{
					return { TypeConvCat::Trivial,false,anyInvolved };
				}
			}
		}
		return TypeConvCat::Illegal;
	}

	bool IsNumericPromotion(ITsys* toType, ITsys* fromType)
	{
		if (toType->GetType() != TsysType::Primitive) return false;
		auto toP = toType->GetPrimitive();
		if (toP.type == TsysPrimitiveType::Void) return false;

		if (auto decl = TryGetForwardDeclFromType<ForwardEnumDeclaration>(fromType))
		{
			return !decl->enumClass;
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

	TypeConv TestTypeConversionImpl(const ParsingArguments& pa, ITsys* toType, ITsys* fromType, TCITestedSet& tested)
	{
		TsysCV fromCV, toCV;
		TsysRefType fromRef, toRef;
		auto fromEntity = fromType->GetEntity(fromCV, fromRef);
		auto toEntity = toType->GetEntity(toCV, toRef);

		switch (fromType->GetType())
		{
		case TsysType::Zero:
			if (toEntity->GetType() == TsysType::Ptr)
			{
				return { TypeConvCat::Trivial,(toCV.isGeneralConst || toCV.isVolatile) };
			}
			else
			{
				fromEntity = fromType = pa.tsys->Int();
				fromCV = { false,false };
				fromRef = TsysRefType::None;
			}
			break;
		case TsysType::Nullptr:
			if (toEntity->GetType() == TsysType::Ptr)
			{
				return { TypeConvCat::Exact,(toCV.isGeneralConst || toCV.isVolatile) };
			}
			break;
		}

		{
			auto result = PerformExactEntityMatch(pa, toEntity, fromEntity, toCV, fromCV, toRef, fromRef);
			if (result.cat != TypeConvCat::Illegal) return result;
		}
		{
			auto result = PetformToPtrTrivialConversion(pa, toEntity, fromEntity, toCV, fromCV, toRef, fromRef);
			if (result.cat != TypeConvCat::Illegal) return result;
		}
		if (!AllowEntityTypeChanging(toCV, fromCV, toRef, fromRef))
		{
			return TypeConvCat::Illegal;
		}
		if (IsNumericPromotion(toEntity, fromEntity))
		{
			return { TypeConvCat::IntegralPromotion,!IsCVEqual(toCV,fromCV),false };
		}
		if (IsNumericConversion(toEntity, fromEntity))
		{
			return { TypeConvCat::Standard,!IsCVEqual(toCV,fromCV),false };
		}

		return TypeConvCat::Illegal;
	}
}
using namespace TestTypeConversion_Impl;

TypeConv TestTypeConversionInternal(const ParsingArguments& pa, ITsys* toType, ITsys* fromType, TCITestedSet& tested)
{
	Tuple<ITsys*, ITsys*> pair(toType, fromType);
	vint index = tested.Keys().IndexOf(pair);
	if (index == -1)
	{
		tested.Add(pair, {});
		auto result = TestTypeConversionImpl(pa, toType, fromType, tested);
		tested.Set(pair, result);
		return result;
	}
	else
	{
		auto result = tested.Values()[index];
		if (result)
		{
			return result.Value();
		}
		else
		{
			// make sure caller doesn't call this function recursively for the same pair of type.
			struct TestConvertInternalStackOverflowException {};
			throw TestConvertInternalStackOverflowException();
		}
	}
}

TCITestedSet* globalCache = nullptr;

TypeConv TestTypeConversion(const ParsingArguments& pa, ITsys* toType, ExprTsysItem fromItem)
{
	TCITestedSet localCache;
	return TestTypeConversionInternal(pa, toType, ApplyExprTsysType(fromItem.tsys, fromItem.type), (globalCache ? *globalCache : localCache));
}