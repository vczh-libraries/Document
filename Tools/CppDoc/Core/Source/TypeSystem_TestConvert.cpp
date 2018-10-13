#include "TypeSystem.h"

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

	bool IsCVMatch(ITsys* toType, ITsys* fromType, bool& performedLRPTrivalConversion)
	{
		TsysCV toCV, fromCV;
		if (toType->GetType() == TsysType::CV)
		{
			toCV = toType->GetCV();
			toType = toType->GetElement();
		}
		if (fromType->GetType() == TsysType::CV)
		{
			fromCV = fromType->GetCV();
			fromType = fromType->GetElement();
		}

		if (toType == fromType)
		{
			if (IsCVSame(toCV, fromCV)) return true;
			if (IsCVMatch(toCV, fromCV))
			{
				performedLRPTrivalConversion = true;
				return true;
			}
		}
		return false;
	}

	bool IsExactOrTrivalConvert(ITsys* toType, ITsys* fromType, bool fromLRP, bool& performedLRPTrivalConversion)
	{
		TsysCV toCV, fromCV;
		TsysRefType toRef, fromRef;
		auto toEntity = toType->GetEntity(toCV, toRef);
		auto fromEntity = fromType->GetEntity(fromCV, fromRef);

		if (toRef == TsysRefType::LRef && toCV.isGeneralConst)
		{
			fromLRP = true;
		}
		else switch (toRef)
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

		if ((toEntity->GetType() == TsysType::Ptr && fromEntity->GetType() == TsysType::Ptr) ||
			(toEntity->GetType() == TsysType::Ptr && fromEntity->GetType() == TsysType::Array) ||
			(toEntity->GetType() == TsysType::Array && fromEntity->GetType() == TsysType::Array))
		{
			if (IsCVMatch(toEntity->GetElement(), fromEntity->GetElement(), performedLRPTrivalConversion))
			{
				return true;
			}
		}

		return false;
	}

	bool IsEntityConversionAllowed(ITsys*& toType, ITsys*& fromType)
	{
		TsysCV toCV, fromCV;
		TsysRefType toRef, fromRef;
		auto toEntity = toType->GetEntity(toCV, toRef);
		auto fromEntity = fromType->GetEntity(fromCV, fromRef);

		if (toRef == TsysRefType::LRef)
		{
			if (toCV.isGeneralConst)
			{
				return true;
			}
		}

		if (toRef == TsysRefType::LRef || fromRef == TsysRefType::LRef)
		{
			return false;
		}

		toType = toEntity;
		fromType = fromEntity;
		return true;
	}

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

	bool IsToBaseClassConversion(ITsys* toType, ITsys* fromType)
	{
		return false;
	}

	bool IsCustomOperatorConversion(ITsys* toType, ITsys* fromType)
	{
		return false;
	}

	bool IsCustomContructorConversion(ITsys* toType, ITsys* fromType)
	{
		return false;
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

	auto toEntity = toType;
	auto fromEntity = fromType;
	if (!IsEntityConversionAllowed(toEntity, fromEntity))
	{
		return TsysConv::Illegal;
	}

	if (IsNumericPromotion(toEntity, fromEntity)) return TsysConv::IntegralPromotion;
	if (IsNumericConversion(toEntity, fromEntity)) return TsysConv::StandardConversion;
	if (IsPointerConversion(toEntity, fromEntity)) return TsysConv::StandardConversion;
	if (IsToBaseClassConversion(toEntity, fromEntity)) return TsysConv::StandardConversion;
	if (IsCustomOperatorConversion(toType, fromType)) return TsysConv::StandardConversion;
	if (IsCustomContructorConversion(toType, fromType)) return TsysConv::StandardConversion;

	return TsysConv::Illegal;
}