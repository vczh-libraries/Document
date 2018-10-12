#include "TypeSystem.h"

namespace TestConvert_Helpers
{
	bool IsNumericPromotion(ITsys* toType, ITsys* fromType)
	{
		if (toType->GetType() != TsysType::Primitive) return false;
		if (fromType->GetType() != TsysType::Primitive) return false;

		auto toP = toType->GetPrimitive();
		auto fromP = fromType->GetPrimitive();
		if (toP.type == fromP.type ||
			(toP.type == TsysPrimitiveType::SInt && fromP.type == TsysPrimitiveType::UInt) ||
			(toP.type == TsysPrimitiveType::UInt && fromP.type == TsysPrimitiveType::SInt))
		{
			return toP.bytes > fromP.bytes;
		}
		return false;
	}

	bool IsNumericConversion(ITsys* toType, ITsys* fromType)
	{
		if (toType->GetType() != TsysType::Primitive) return false;
		if (fromType->GetType() != TsysType::Primitive) return false;
		return false;
	}

	bool IsCVSame(TsysCV toCV, TsysCV fromCV)
	{
		if ((toCV.isConstExpr || toCV.isConst) != (fromCV.isConstExpr || fromCV.isConst)) return false;
		if (toCV.isVolatile != fromCV.isVolatile) return false;
		return true;
	}

	bool IsCVMatch(TsysCV toCV, TsysCV fromCV)
	{
		if ((toCV.isConstExpr || toCV.isConst) && !(fromCV.isConstExpr || fromCV.isConst)) return false;
		if (toCV.isVolatile && !fromCV.isVolatile) return false;
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
			case TsysRefType::None: break;
			}
			break;
		case TsysRefType::RRef:
			switch (fromRef)
			{
			case TsysRefType::LRef: return false;
			case TsysRefType::RRef: fromLRP = true; break;
			case TsysRefType::None: break;
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
			case TsysPrimitiveType::Void:
				return TsysConv::StandardConversion;
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

	if (fromType->GetType() == TsysType::RRef)
	{
		fromType = fromType->GetElement();
	}

	//if (IsCVMatch(toType, fromType, &IsNumericPromotion)) return TsysConv::IntegralPromotion;
	//if (IsCVMatch(toType, fromType, &IsNumericConversion)) return TsysConv::StandardConversion;

	//if (toType->GetType() == TsysType::Ptr && fromType->GetType() == TsysType::Ptr)
	//{
	//	if (IsCVMatch(toType->GetElement(), fromType->GetElement(), [](ITsys* toType, ITsys* fromType)
	//	{
	//		if (toType->GetType() == TsysType::Primitive && toType->GetPrimitive().type == TsysPrimitiveType::Void)
	//		{
	//			if (fromType->GetType() == TsysType::Member)
	//			{
	//				return false;
	//			}
	//			return true;
	//		}
	//		return false;
	//	}))
	//	{
	//		return TsysConv::StandardConversion;
	//	}
	//}

	return TsysConv::Illegal;
}