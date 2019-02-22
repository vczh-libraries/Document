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

TsysConv TestConvertInternal(ParsingArguments& pa, ITsys* toType, ITsys* fromType);

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

	template<typename T>
	Ptr<T> TryGetDeclFromType(ITsys* type)
	{
		if (type->GetType() != TsysType::Decl) return false;
		auto symbol = type->GetDecl();
		if (symbol->decls.Count() != 1) return false;
		return symbol->decls[0].Cast<T>();
	}

	bool IsExactOrTrivalConvert(ITsys* toType, ITsys* fromType, bool fromLRP, bool& performedTrivalConversion)
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
					performedTrivalConversion = true;
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
			if (auto decl = TryGetDeclFromType<ForwardEnumDeclaration>(fromEntity))
			{
				if (decl->enumClass) return false;

				auto primitive = toEntity->GetPrimitive();
				if (primitive.type != TsysPrimitiveType::SInt) return false;
				if (primitive.bytes != TsysBytes::_4) return false;

				performedTrivalConversion = true;
				return true;
			}
		}

		if ((toEntity->GetType() == TsysType::Ptr && fromEntity->GetType() == TsysType::Ptr) ||
			(toEntity->GetType() == TsysType::Ptr && fromEntity->GetType() == TsysType::Array) ||
			(toEntity->GetType() == TsysType::Array && fromEntity->GetType() == TsysType::Array))
		{
			if (IsCVMatch(toEntity->GetElement(), fromEntity->GetElement(), performedTrivalConversion))
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

		if (toRef == TsysRefType::LRef && toCV.isGeneralConst)
		{
			goto ALLOW;
		}

		if (toRef == TsysRefType::None)
		{
			goto ALLOW;
		}

		if (toRef == TsysRefType::LRef || fromRef == TsysRefType::LRef)
		{
			if (!TryGetDeclFromType<ClassDeclaration>(toEntity) || !TryGetDeclFromType<ClassDeclaration>(fromEntity))
			{
				return false;
			}
		}

	ALLOW:
		toType = toEntity;
		fromType = fromEntity;
		return true;
	}

	TsysConv IsUniversalInitialization(ParsingArguments& pa, ITsys* toType, ITsys* fromEntity, const TsysInit& init)
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
					if (ctorSymbol->decls.Count() != 1) continue;
					auto ctorDecl = ctorSymbol->decls[0].Cast<ForwardFunctionDeclaration>();
					symbol_type_resolving::EvaluateSymbol(pa, ctorSymbol.Obj(), ctorDecl.Obj());

					for (vint j = 0; j < ctorSymbol->evaluatedTypes->Count(); j++)
					{
						auto tsys = ctorSymbol->evaluatedTypes->Get(j);
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
				symbol_type_resolving::VisitOverloadedFunction(pa, funcTypes, argTypesList, result);
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
		return TestConvertInternal(pa, toType, type);
	}

	bool IsNumericPromotion(ITsys* toType, ITsys* fromType)
	{
		if (toType->GetType() != TsysType::Primitive) return false;
		auto toP = toType->GetPrimitive();
		if (toP.type == TsysPrimitiveType::Void) return false;

		if (auto decl = TryGetDeclFromType<ForwardEnumDeclaration>(fromType))
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

	bool IsToBaseClassConversion(ParsingArguments& pa, ITsys* toType, ITsys* fromType)
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
		if (!TryGetDeclFromType<ClassDeclaration>(toType)) return false;

		List<ITsys*> searched;
		searched.Add(fromType);
		for (vint i = 0; i < searched.Count(); i++)
		{
			auto currentType = searched[i];
			if (currentType == toType) return true;
			if (auto currentClass = TryGetDeclFromType<ClassDeclaration>(currentType))
			{
				symbol_type_resolving::EvaluateSymbol(pa, currentClass->symbol, currentClass.Obj());
				for (vint j = 0; j < currentClass->symbol->evaluatedBaseTypes->Count(); j++)
				{
					auto& baseTypes = *currentClass->symbol->evaluatedBaseTypes->Get(j).Obj();
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
		TsysCV fromCV;
		TsysRefType fromRef;
		auto fromEntity = fromType->GetEntity(fromCV, fromRef);

		auto fromClass = TryGetDeclFromType<ClassDeclaration>(fromEntity);
		if (!fromClass) return false;

		auto fromSymbol = fromClass->symbol;
		vint index = fromSymbol->children.Keys().IndexOf(L"$__type");
		if (index == -1) return false;
		const auto& typeOps = fromSymbol->children.GetByIndex(index);

		ParsingArguments newPa(pa, fromSymbol);
		for (vint i = 0; i < typeOps.Count(); i++)
		{
			auto typeOpSymbol = typeOps[i];
			if (typeOpSymbol->decls.Count() != 1) continue;
			auto typeOpDecl = typeOpSymbol->decls[0].Cast<ForwardFunctionDeclaration>();
			{
				if (typeOpDecl->decoratorExplicit) return false;
				auto typeOpType = GetTypeWithoutMemberAndCC(typeOpDecl->type).Cast<FunctionType>();
				if (!typeOpType) continue;
				if (typeOpType->parameters.Count() != 0) continue;
				if (TestFunctionQualifier(fromCV, fromRef, typeOpType) == TsysConv::Illegal) continue;
			}
			symbol_type_resolving::EvaluateSymbol(pa, typeOpSymbol.Obj(), typeOpDecl.Obj());

			for (vint j = 0; j < typeOpSymbol->evaluatedTypes->Count(); j++)
			{
				auto tsys = typeOpSymbol->evaluatedTypes->Get(j);
				if (TestConvertInternal(newPa, toType, tsys->GetElement()->RRefOf()) != TsysConv::Illegal)
				{
					return true;
				}
			}
		}

		return false;
	}

	bool IsCustomContructorConversion(ParsingArguments& pa, ITsys* toType, ITsys* fromType)
	{
		TsysCV toCV;
		TsysRefType toRef;
		auto toEntity = toType->GetEntity(toCV, toRef);
		if (toRef == TsysRefType::LRef && !toCV.isGeneralConst) return false;

		auto toClass = TryGetDeclFromType<ClassDeclaration>(toEntity);
		if (!toClass) return false;

		auto toSymbol = toClass->symbol;
		if (TestConvertInternal(pa, toType, pa.tsys->DeclOf(toSymbol)->RRefOf()) == TsysConv::Illegal) return false;

		vint index = toSymbol->children.Keys().IndexOf(L"$__ctor");
		if (index == -1) return false;
		const auto& ctors = toSymbol->children.GetByIndex(index);

		ParsingArguments newPa(pa, toSymbol);
		for (vint i = 0; i < ctors.Count(); i++)
		{
			auto ctorSymbol = ctors[i];
			if (ctorSymbol->decls.Count() != 1) continue;
			auto ctorDecl = ctorSymbol->decls[0].Cast<ForwardFunctionDeclaration>();
			{
				if (ctorDecl->decoratorExplicit) return false;
				auto ctorType = GetTypeWithoutMemberAndCC(ctorDecl->type).Cast<FunctionType>();
				if (!ctorType) continue;
				if (ctorType->parameters.Count() != 1) continue;
			}
			symbol_type_resolving::EvaluateSymbol(pa, ctorSymbol.Obj(), ctorDecl.Obj());

			for (vint j = 0; j < ctorSymbol->evaluatedTypes->Count(); j++)
			{
				auto tsys = ctorSymbol->evaluatedTypes->Get(j);
				if (TestConvertInternal(newPa, tsys->GetParam(0), fromType) != TsysConv::Illegal)
				{
					return true;
				}
			}
		}

		return false;
	}
}
using namespace TestConvert_Helpers;

TsysConv TestConvertInternal(ParsingArguments& pa, ITsys* toType, ITsys* fromType)
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
		bool performedTrivalConversion = false;
		if (IsExactOrTrivalConvert(toType, fromType, false, performedTrivalConversion))
		{
			return performedTrivalConversion ? TsysConv::TrivalConversion : TsysConv::Exact;
		}
	}

	auto toEntity = toType;
	auto fromEntity = fromType;
	if (!IsEntityConversionAllowed(toEntity, fromEntity))
	{
		return TsysConv::Illegal;
	}

	{
		TsysCV fromCV;
		TsysRefType fromRef;
		auto fromEntity = fromType->GetEntity(fromCV, fromRef);
		if (fromEntity->GetType() == TsysType::Init)
		{
			auto& init = fromEntity->GetInit();
			return IsUniversalInitialization(pa, toType, fromEntity, init);
		}
	}

	if (IsNumericPromotion(toEntity, fromEntity)) return TsysConv::IntegralPromotion;
	if (IsNumericConversion(toEntity, fromEntity)) return TsysConv::StandardConversion;
	if (IsPointerConversion(toEntity, fromEntity)) return TsysConv::StandardConversion;
	if (IsToBaseClassConversion(pa, toType, fromType)) return TsysConv::StandardConversion;
	if (IsCustomOperatorConversion(pa, toType, fromType)) return TsysConv::UserDefinedConversion;
	if (IsCustomContructorConversion(pa, toType, fromType)) return TsysConv::UserDefinedConversion;

	return TsysConv::Illegal;
}

TsysConv TestConvert(ParsingArguments& pa, ITsys* toType, ExprTsysItem fromItem)
{
	return TestConvertInternal(pa, toType, (fromItem.type == ExprTsysType::LValue ? fromItem.tsys->LRefOf() : fromItem.tsys));
}