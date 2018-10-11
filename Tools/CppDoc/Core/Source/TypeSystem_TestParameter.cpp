#include "TypeSystem.h"

namespace TestConvert_Helpers
{
	bool IsExactSameType(ITsys* toType, ITsys* fromType)
	{
		return toType == fromType;
	}

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

	bool IsCVMatch(ITsys* toType, ITsys* fromType, bool(*matcher)(ITsys*, ITsys*) = &IsExactSameType)
	{
		if (toType->GetType() == TsysType::CV)
		{
			TsysCV toCV = toType->GetCV();
			toType = toType->GetElement();

			TsysCV fromCV;
			if (fromType->GetType() == TsysType::CV)
			{
				fromCV = fromType->GetCV();
				fromType = fromType->GetElement();
			}

			if (matcher(toType, fromType))
			{
				if (!(toCV.isConstExpr || toCV.isConst) && (fromCV.isConstExpr || fromCV.isConst)) return false;
				if (!toCV.isVolatile && fromCV.isVolatile) return false;
				return true;
			}
		}
		return false;
	}

	TsysConv TestExactOrTrival_RRefIllegal(ITsys* toType, ITsys* fromType)
	{
		if (toType == fromType)
		{
			return TsysConv::Exact;
		}

		if (toType->GetType() == TsysType::LRef && fromType->GetType() != TsysType::LRef)
		{
			if (toType->GetElement() == fromType)
			{
				return TsysConv::Exact;
			}
			if (IsCVMatch(toType->GetElement(), fromType))
			{
				return TsysConv::TrivalConversion;
			}
		}
		if (toType->GetType() != TsysType::LRef && fromType->GetType() == TsysType::LRef)
		{
			if (toType == fromType->GetElement())
			{
				return TsysConv::Exact;
			}
			if (IsCVMatch(toType, fromType->GetElement()))
			{
				return TsysConv::TrivalConversion;
			}
		}

		switch (toType->GetType())
		{
		case TsysType::LRef:
		case TsysType::RRef:
			if (toType->GetType() == fromType->GetType())
			{
				if (IsCVMatch(toType->GetElement(), fromType->GetElement()))
				{
					return TsysConv::TrivalConversion;
				}
			}
			break;
		}

		return TsysConv::Illegal;
	}

	TsysConv TestExactOrTrival_RRefLegal(ITsys* toType, ITsys* fromType)
	{
		if (fromType->GetType() == TsysType::RRef)
		{
			fromType = fromType->GetElement();
		}

		if (toType == fromType)
		{
			return TsysConv::Exact;
		}

		switch (toType->GetType())
		{
		case TsysType::Ptr:
			if (toType->GetType() == fromType->GetType())
			{
				if (IsCVMatch(toType->GetElement(), fromType->GetElement()))
				{
					return TsysConv::TrivalConversion;
				}
			}
			break;
		}

		if (toType->GetType() == TsysType::Ptr && fromType->GetType() == TsysType::Array)
		{
			if (toType->GetElement() == fromType->GetElement())
			{
				return TsysConv::Exact;
			}
			if (IsCVMatch(toType->GetElement(), fromType->GetElement()))
			{
				return TsysConv::TrivalConversion;
			}
		}

		if (IsCVMatch(toType, fromType))
		{
			return TsysConv::TrivalConversion;
		}
		return TsysConv::Illegal;
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
		auto conv = TestExactOrTrival_RRefIllegal(toType, fromType);
		if (conv != TsysConv::Illegal) return conv;
	}
	{
		auto conv = TestExactOrTrival_RRefLegal(toType, fromType);
		if (conv != TsysConv::Illegal) return conv;
	}

	if (fromType->GetType() == TsysType::RRef)
	{
		fromType = fromType->GetElement();
	}

	if (IsCVMatch(toType, fromType, &IsNumericPromotion)) return TsysConv::IntegralPromotion;
	if (IsCVMatch(toType, fromType, &IsNumericConversion)) return TsysConv::StandardConversion;

	if (toType->GetType() == TsysType::Ptr && fromType->GetType() == TsysType::Ptr)
	{
		if (IsCVMatch(toType->GetElement(), fromType->GetElement(), [](ITsys* toType, ITsys* fromType)
		{
			if (toType->GetType() == TsysType::Primitive && toType->GetPrimitive().type == TsysPrimitiveType::Void)
			{
				if (fromType->GetType() == TsysType::Member)
				{
					return false;
				}
				return true;
			}
			return false;
		}))
		{
			return TsysConv::StandardConversion;
		}
	}

	return TsysConv::Illegal;
}