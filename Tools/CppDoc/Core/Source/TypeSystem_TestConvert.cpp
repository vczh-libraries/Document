#include "TypeSystem.h"
#include "Parser.h"
#include "Ast_Decl.h"
#include "Ast_Type.h"

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

	Ptr<ClassDeclaration> TryGetClassFromType(ITsys* type)
	{
		if (type->GetType() != TsysType::Decl) return false;
		auto symbol = type->GetDecl();
		if (symbol->decls.Count() != 1) return false;
		return symbol->decls[0].Cast<ClassDeclaration>();
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
				goto ALLOW;
			}
		}

		if (toRef == TsysRefType::LRef || fromRef == TsysRefType::LRef)
		{
			if (!TryGetClassFromType(toEntity) || !TryGetClassFromType(fromEntity))
			{
				return false;
			}
		}

	ALLOW:
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

	bool IsToBaseClassConversion(ParsingArguments& pa, ITsys* toType, ITsys* fromType)
	{
		{
			TsysCV toCV, fromCV;
			TsysRefType toRef, fromRef;
			auto toEntity = toType->GetEntity(toCV, toRef);
			auto fromEntity = fromType->GetEntity(fromCV, fromRef);

			if (toRef != fromRef) return false;
			if (toRef != TsysRefType::None)
			{
				if (!IsCVMatch(toCV, fromCV)) return false;

				toType = toEntity;
				fromType = fromEntity;
				goto BEGIN_SEARCHING_FOR_BASE_CLASSES;
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
		}
		else
		{
			return false;
		}

	BEGIN_SEARCHING_FOR_BASE_CLASSES:
		if (!TryGetClassFromType(toType)) return false;

		List<ITsys*> searched;
		searched.Add(fromType);
		for (vint i = 0; i < searched.Count(); i++)
		{
			auto currentType = searched[i];
			if (currentType == toType) return true;
			if (auto currentClass = TryGetClassFromType(currentType))
			{
				ParsingArguments newPa(pa, currentClass->symbol);
				for (vint j = 0; j < currentClass->baseTypes.Count(); j++)
				{
					TypeTsysList baseTypes;
					TypeToTsys(newPa, currentClass->baseTypes[j].f1, baseTypes);
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

	bool IsCustomOperatorConversion(ParsingArguments& pa, ITsys* toType, ITsys* fromType)
	{
		return false;
	}

	bool IsCustomContructorConversion(ParsingArguments& pa, ITsys* toType, ITsys* fromType)
	{
		return false;
	}
}
using namespace TestConvert_Helpers;

TsysConv TestConvert(ParsingArguments& pa, ITsys* toType, ITsys* fromType)
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
	if (IsToBaseClassConversion(pa, toType, fromType)) return TsysConv::StandardConversion;
	if (IsCustomOperatorConversion(pa, toType, fromType)) return TsysConv::StandardConversion;
	if (IsCustomContructorConversion(pa, toType, fromType)) return TsysConv::StandardConversion;

	return TsysConv::Illegal;
}