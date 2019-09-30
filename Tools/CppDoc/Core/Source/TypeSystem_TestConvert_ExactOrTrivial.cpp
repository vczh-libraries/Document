#include "TypeSystem.h"
#include "Parser.h"
#include "Ast_Resolving.h"

namespace TestConvert_Helpers
{
	bool IsExactMatch(ITsys* toType, ITsys* fromType, bool& isAny);

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

	ITsys* ArrayToPtr(ITsys* tsys)
	{
		if (tsys->GetType() == TsysType::Array)
		{
			if (tsys->GetParamCount() == 1)
			{
				tsys = tsys->GetElement()->PtrOf();
			}
			else
			{
				tsys = tsys->GetElement()->ArrayOf(tsys->GetParamCount() - 1)->PtrOf();
			}
		}
		return tsys;
	}

	ITsys* ArrayRemoveDims(ITsys* tsys, vint dimension)
	{
		if (tsys->GetType() == TsysType::Array)
		{
			vint d = tsys->GetParamCount();
			if (d == dimension)
			{
				tsys = tsys->GetElement();
			}
			else if (d > dimension)
			{
				tsys = tsys->GetElement()->ArrayOf(d - dimension);
			}
		}
		return tsys;
	}

	bool IsExactMatchInternal(ITsys* toType, ITsys* fromType, bool& isAny)
	{
		// this function is called to compare everything inside a pointer
		// so there is no way to have Zero or Nullptr

		if (toType == fromType)
		{
			if (toType->HasUnknownType())
			{
				isAny = true;
			}
			return true;
		}

		if (toType->IsUnknownType() || fromType->IsUnknownType())
		{
			isAny = true;
			return true;
		}

		if (toType->GetType() != TsysType::Init && fromType->GetType() == TsysType::Init && fromType->GetParamCount() == 1 && fromType->GetParam(0)->IsUnknownType())
		{
			isAny = true;
			return true;
		}

		switch (toType->GetType())
		{
		case TsysType::Primitive:
			return fromType->IsUnknownType();
		case TsysType::LRef:
			switch (fromType->GetType())
			{
			case TsysType::LRef:
				return IsExactMatch(toType->GetElement(), fromType->GetElement(), isAny);
			case TsysType::RRef:
				return IsExactMatch(toType, fromType->GetElement(), isAny);
			}
			break;
		case TsysType::RRef:
			switch (fromType->GetType())
			{
			case TsysType::LRef:
				return IsExactMatch(toType->GetElement(), fromType, isAny);
			case TsysType::RRef:
				return IsExactMatch(toType->GetElement(), fromType->GetElement(), isAny);
			}
			break;
		case TsysType::Ptr:
			switch (fromType->GetType())
			{
			case TsysType::Ptr:
				if ((toType->GetElement()->GetType() == TsysType::Member) == (fromType->GetElement()->GetType() == TsysType::Member))
				{
					return IsExactMatch(toType->GetElement(), fromType->GetElement(), isAny);
				}
			}
			break;
		case TsysType::Array:
			switch (fromType->GetType())
			{
			case TsysType::Array:
				if (toType->GetParamCount() == fromType->GetParamCount())
				{
					return IsExactMatch(toType->GetElement(), fromType->GetElement(), isAny);
				}
			}
			break;
		case TsysType::Function:
			switch (fromType->GetType())
			{
			case TsysType::Function:
				if (toType->GetParamCount() != fromType->GetParamCount())
				{
					return false;
				}
				if (toType->GetFunc().callingConvention != fromType->GetFunc().callingConvention)
				{
					return false;
				}
				if (toType->GetFunc().ellipsis != fromType->GetFunc().ellipsis)
				{
					return false;
				}
				if (!IsExactMatch(toType->GetElement(), fromType->GetElement(), isAny))
				{
					return false;
				}
				for (vint i = 0; i < toType->GetParamCount(); i++)
				{
					auto toParam = ArrayToPtr(toType->GetParam(i));
					auto fromParam = ArrayToPtr(fromType->GetParam(i));

					if (!IsExactMatch(toParam, fromParam, isAny))
					{
						return false;
					}
				}
				return true;
			}
			break;
		case TsysType::Member:
			switch (fromType->GetType())
			{
			case TsysType::Member:
				return IsExactMatch(toType->GetElement(), fromType->GetElement(), isAny) && IsExactMatch(toType->GetClass(), fromType->GetClass(), isAny);
			}
			break;
		case TsysType::CV:
			if (toType->GetElement()->IsUnknownType())
			{
				isAny = true;
				return true;
			}
			switch (fromType->GetType())
			{
			case TsysType::CV:
				{
					auto toCV = toType->GetCV();
					auto fromCV = fromType->GetCV();
					bool toU = toType->GetElement()->IsUnknownType();
					bool fromU = fromType->GetElement()->IsUnknownType();

					if (toU && fromU)
					{
						return true;
					}
					else if (toU)
					{
						if (!fromCV.isGeneralConst && toCV.isGeneralConst) return false;
						if (!fromCV.isVolatile && toCV.isVolatile) return false;
					}
					else if (fromU)
					{
						if (!toCV.isGeneralConst && fromCV.isGeneralConst) return false;
						if (!toCV.isVolatile && fromCV.isVolatile) return false;
					}
					return IsExactMatch(toType->GetElement(), fromType->GetElement(), isAny);
				}
			}
			break;
		case TsysType::Decl:
			return false;
		case TsysType::DeclInstant:
			// TODO: [Cpp.md] Deal with DeclInstant here
			throw 0;
		case TsysType::Init:
			switch (fromType->GetType())
			{
			case TsysType::Init:
				if (toType->GetParamCount() != fromType->GetParamCount())
				{
					return false;
				}
				for (vint i = 0; i < toType->GetParamCount(); i++)
				{
					auto toParam = ApplyExprTsysType(toType->GetParam(i), toType->GetInit().headers[i].type);
					auto fromParam = ApplyExprTsysType(fromType->GetParam(i), fromType->GetInit().headers[i].type);

					if (!IsExactMatch(toParam, fromParam, isAny))
					{
						return false;
					}
				}
				return true;
			}
			break;
		}
		return false;
	}

	bool IsExactMatch(ITsys* toType, ITsys* fromType, bool& isAny)
	{
		if (IsExactMatchInternal(toType, fromType, isAny))
		{
			return true;
		}
		
		if (fromType->GetType() == TsysType::Init && fromType->GetParamCount() == 1)
		{
			if (IsExactMatchInternal(toType, fromType->GetParam(0), isAny))
			{
				return true;
			}
		}

		return false;
	}

	bool IsPointerOfTypeConvertable(ITsys* toType, ITsys* fromType, bool& isTrivial, bool& isAny)
	{
		if (toType == fromType)
		{
			if (toType->HasUnknownType())
			{
				isAny = true;
			}
			return true;
		}

		TsysCV toCV, fromCV;
		TsysRefType toRef, fromRef;
		auto toEntity = toType->GetEntity(toCV, toRef);
		auto fromEntity = fromType->GetEntity(fromCV, fromRef);

		if (!toEntity->IsUnknownType() && !IsCVSame(toCV, fromCV))
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
			else
			{
				return false;
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

	bool IsExactOrTrivialConvert(ITsys* toType, ITsys* fromType, bool& isTrivial, bool& isAny)
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

		bool toConstLRef = toRef == TsysRefType::LRef && toCV.isGeneralConst;
		bool rRefToRRef = (toRef == TsysRefType::RRef && fromRef == TsysRefType::RRef);
		bool allowCVConversion = toConstLRef || rRefToRRef
			|| (toRef == TsysRefType::LRef && fromRef == TsysRefType::LRef)
			|| (toRef == TsysRefType::RRef && fromRef == TsysRefType::None)
			;
		bool allowEntityConversion = toConstLRef || rRefToRRef
			|| toRef == TsysRefType::None
			|| (toRef == TsysRefType::RRef && fromRef == TsysRefType::None)
			;

		if (allowCVConversion)
		{
			if (toEntity->GetType() == TsysType::Array && fromEntity->GetType() == TsysType::Array)
			{
				vint toD = toEntity->GetParamCount();
				vint fromD = fromEntity->GetParamCount();
				vint minD = toD < fromD ? toD : fromD;

				TsysCV newToCV, newFromCV;
				toEntity = ArrayRemoveDims(toEntity, minD)->GetEntity(newToCV, toRef);
				fromEntity = ArrayRemoveDims(fromEntity, minD)->GetEntity(newFromCV, fromRef);

				toCV.isGeneralConst |= newToCV.isGeneralConst;
				toCV.isVolatile |= newToCV.isVolatile;

				fromCV.isGeneralConst |= newFromCV.isGeneralConst;
				fromCV.isVolatile |= newFromCV.isVolatile;
			}

			if (!toEntity->IsUnknownType() && !IsCVSame(toCV, fromCV))
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

		if (allowEntityConversion)
		{
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
				toEntity = ArrayToPtr(toEntity);
				fromEntity = ArrayToPtr(fromEntity);
				return IsPointerOfTypeConvertable(toEntity->GetElement(), fromEntity->GetElement(), isTrivial, isAny);
			}
		}

		if (allowCVConversion || allowEntityConversion)
		{
			if (!IsExactMatch(toEntity, fromEntity, isAny))
			{
				return false;
			}

			if (rRefToRRef && !isAny && toEntity != fromEntity)
			{
				return false;
			}
			return true;
		}
		else
		{
			return IsExactMatch(toType, fromType, isAny);
		}
	}
}