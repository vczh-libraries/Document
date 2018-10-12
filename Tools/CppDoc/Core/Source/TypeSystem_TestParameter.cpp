#include "TypeSystem.h"

namespace TestConvert_Helpers
{
	bool IsNumericPromotion(ITsys* toType, ITsys* fromType)
	{
		if (toType->GetType() != TsysType::Primitive) return false;
		if (fromType->GetType() != TsysType::Primitive) return false;

		auto toP = toType->GetPrimitive();
		auto fromP = fromType->GetPrimitive();

		if (toP.type == TsysPrimitiveType::Void) return false;
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

	bool IsCVSame(TsysCV toCV, TsysCV fromCV)
	{
		if ((toCV.isConstExpr || toCV.isConst) != (fromCV.isConstExpr || fromCV.isConst)) return false;
		if (toCV.isVolatile != fromCV.isVolatile) return false;
		return true;
	}

	bool IsCVMatch(TsysCV toCV, TsysCV fromCV)
	{
		if (!(toCV.isConstExpr || toCV.isConst) && (fromCV.isConstExpr || fromCV.isConst)) return false;
		if (!toCV.isVolatile && fromCV.isVolatile) return false;
		return true;
	}

	bool IsExactOrTrivalConvert(ITsys* toType, ITsys* fromType, bool fromLRP, bool& performedLRPTrivalConversion)
	{
		TsysCV toCV, fromCV;
		TsysRefType toRef, fromRef;
		auto toEntity = toType->GetEntity(toCV, toRef);
		auto fromEntity = fromType->GetEntity(fromCV, fromRef);

		switch (toRef)
		{
		case TsysRefType::LRef:
			switch (fromRef)
			{
			case TsysRefType::LRef: fromLRP = true; break;
			case TsysRefType::RRef: return false;
			case TsysRefType::None: fromLRP = true; break;
			}
			break;
		case TsysRefType::RRef:
			switch (fromRef)
			{
			case TsysRefType::LRef: return false;
			case TsysRefType::RRef: fromLRP = true; break;
			case TsysRefType::None: return false;
			}
			break;
		case TsysRefType::None:
			switch (fromRef)
			{
			case TsysRefType::LRef: break;
			case TsysRefType::RRef: break;
			case TsysRefType::None: break;
			}
			break;
		}

		if (fromLRP)
		{
			if (!IsCVSame(toCV, fromCV))
			{
				if (IsCVMatch(toCV, fromCV))
				{
					performedLRPTrivalConversion = true;
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

		if (toEntity->GetType() == TsysType::Ptr && fromEntity->GetType() == TsysType::Ptr)
		{
			return IsExactOrTrivalConvert(toEntity->GetElement(), fromEntity->GetElement(), true, performedLRPTrivalConversion);
		}

		if (toEntity->GetType() == TsysType::Ptr && fromEntity->GetType() == TsysType::Array)
		{
			return IsExactOrTrivalConvert(toEntity->GetElement(), fromEntity->GetElement(), true, performedLRPTrivalConversion);
		}

		if (toEntity->GetType() == TsysType::Array && fromEntity->GetType() == TsysType::Array)
		{
			return IsExactOrTrivalConvert(toEntity->GetElement(), fromEntity->GetElement(), true, performedLRPTrivalConversion);
		}

		return false;
	}

	bool IsEntityConversionAllowed(ITsys*& toType, ITsys*& fromType)
	{
		TsysCV toCV, fromCV;
		TsysRefType toRef, fromRef;
		auto toEntity = toType->GetEntity(toCV, toRef);
		auto fromEntity = fromType->GetEntity(fromCV, fromRef);

		if (toRef == TsysRefType::LRef || fromRef == TsysRefType::LRef)
		{
			return false;
		}

		toType = toEntity;
		fromType = fromEntity;
		return true;
	}
}
using namespace TestConvert_Helpers;

TsysConv TestConvert(ITsys* toType, ITsys* fromType)
{
	if (fromType->GetType() == TsysType::Zero)
	{
		if (toType->GetType() == TsysType::Ptr) return TsysConv::TrivalConversion;
		if (toType->GetType() == TsysType::CV)
		{
			toType = toType->GetElement();
		}

		if (toType->GetType() == TsysType::Primitive)
		{
			switch (toType->GetPrimitive().type)
			{
			case TsysPrimitiveType::Float:
				return TsysConv::StandardConversion;
			case TsysPrimitiveType::Void:
				return TsysConv::Illegal;
			default:
				return TsysConv::Exact;
			}
		}
	}

	if (fromType->GetType() == TsysType::Nullptr)
	{
		if (toType->GetType() == TsysType::Ptr) return TsysConv::Exact;
	}

	{
		bool performedLRPTrivalConversion = false;
		if (IsExactOrTrivalConvert(toType, fromType, false, performedLRPTrivalConversion))
		{
			return performedLRPTrivalConversion ? TsysConv::TrivalConversion : TsysConv::Exact;
		}
	}

	if (!IsEntityConversionAllowed(toType, fromType))
	{
		return TsysConv::Illegal;
	}

	if (IsNumericPromotion(toType, fromType)) return TsysConv::IntegralPromotion;
	if (IsNumericConversion(toType, fromType)) return TsysConv::StandardConversion;

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
			case TsysType::Function: return TsysConv::StandardConversion;
			case TsysType::Member: return TsysConv::Illegal;
			}
			if (IsCVMatch(toCV, fromCV)) return TsysConv::StandardConversion;
		}
	}
	return TsysConv::Illegal;
}