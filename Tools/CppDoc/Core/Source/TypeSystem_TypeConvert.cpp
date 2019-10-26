#include "Ast_Resolving.h"

using TCITestedSet = Dictionary<Tuple<ITsys*, ITsys*>, Nullable<TypeConv>>;
TypeConv TestTypeConversionInternal(const ParsingArguments& pa, ITsys* toType, ITsys* fromType, TCITestedSet& tested);

namespace TestTypeConversion_Impl
{
#define EXTRACT_ENTITY(ARG, VAR)														\
	TsysCV from##VAR##CV, to##VAR##CV;													\
	TsysRefType from##VAR##Ref, to##VAR##Ref;											\
	auto from##VAR##Entity = from##ARG->GetEntity(from##VAR##CV, from##VAR##Ref);		\
	auto to##VAR##Entity = to##ARG->GetEntity(to##VAR##CV, to##VAR##Ref)				\

#define ENTITY_VARS(VAR, DEC, DEL)			\
	ITsys*		DEC to##VAR##Entity		DEL	\
	ITsys*		DEC from##VAR##Entity	DEL	\
	TsysCV		DEC to##VAR##CV			DEL	\
	TsysCV		DEC from##VAR##CV		DEL	\
	TsysRefType	DEC to##VAR##Ref		DEL	\
	TsysRefType	DEC from##VAR##Ref			\

#define ENTITY_PASS(VAR)			\
	to##VAR##Entity		,			\
	from##VAR##Entity	,			\
	to##VAR##CV			,			\
	from##VAR##CV		,			\
	to##VAR##Ref		,			\
	from##VAR##Ref					\

#define _ ,

/***********************************************************************
Utilities
***********************************************************************/

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

	bool AllowEntityTypeChanging(TsysCV toCV, TsysCV fromCV, TsysRefType toRef, TsysRefType fromRef, bool& cvInvolved, bool& allowInheritanceOnly)
	{
		switch (toRef)
		{
		case TsysRefType::LRef:
			if (!toCV.isGeneralConst)
			{
				if (fromRef == TsysRefType::LRef)
				{
					allowInheritanceOnly = true;
				}
				else
				{
					return false;
				}
			}
			break;
		case TsysRefType::RRef:
			if (fromRef == TsysRefType::LRef)
			{
				return false;
			}
			break;
		}

		if (toRef != TsysRefType::None)
		{
			if (IsCVEqual(toCV, fromCV))
			{
				return true;
			}
			else if (IsCVCompatible(toCV, fromCV))
			{
				cvInvolved = true;
				return true;
			}
			else
			{
				return false;
			}
		}
		return true;
	}

	bool AllowConvertingToPointer(ITsys* toEntity, ITsys* fromEntity, ENTITY_VARS(Element, &, _), bool allowInheritanceOnly)
	{
		switch (toEntity->GetType())
		{
		case TsysType::Array:
			switch (fromEntity->GetType())
			{
			case TsysType::Array:
				break;
			default:
				return false;
			}
			break;
		case TsysType::Ptr:
			switch (fromEntity->GetType())
			{
			case TsysType::Array:
			case TsysType::Ptr:
				break;
			default:
				return false;
			}
			break;
		default:
			return false;
		}

		if (toEntity->GetType() == TsysType::Array)
		{
			toEntity = ArrayRemoveOneDim(toEntity);
		}
		else
		{
			if (allowInheritanceOnly) return false;
			toEntity = toEntity->GetElement();
		}

		if (fromEntity->GetType() == TsysType::Array)
		{
			fromEntity = ArrayRemoveOneDim(fromEntity);
		}
		else
		{
			if (allowInheritanceOnly) return false;
			fromEntity = fromEntity->GetElement();
		}

		toElementEntity = toEntity->GetEntity(toElementCV, toElementRef);
		fromElementEntity = fromEntity->GetEntity(fromElementCV, fromElementRef);

		if (fromElementRef != TsysRefType::None || toElementRef != TsysRefType::None)
		{
			throw L"Pointer to reference is illegal.";
		}

		return true;
	}

/***********************************************************************
Exact
***********************************************************************/

	bool IsExactEntityMatch(ITsys* toEntity, ITsys* fromEntity, bool& anyInvolved)
	{
		if (IsUnknownType(toEntity) || IsUnknownType(fromEntity))
		{
			anyInvolved = true;
			return true;
		}

		if (toEntity->GetType() != fromEntity->GetType())
		{
			if (toEntity->GetType() == TsysType::LRef && fromEntity->GetType() == TsysType::RRef)
			{
				if (IsUnknownType(fromEntity->GetElement()))
				{
					anyInvolved = true;
					return true;
				}
			}
			else if (toEntity->GetType() == TsysType::RRef && fromEntity->GetType() == TsysType::LRef)
			{
				if (IsUnknownType(toEntity->GetElement()))
				{
					anyInvolved = true;
					return true;
				}
			}
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
			return IsExactEntityMatch(toEntity->GetElement(), fromEntity->GetElement(), anyInvolved);
		case TsysType::Ptr:
			{
				auto toElement = toEntity->GetElement();
				auto fromElement = fromEntity->GetElement();
				bool _anyInvolved = false;
				if (!IsExactEntityMatch(toElement, fromElement, _anyInvolved)) return false;
				if (IsUnknownType(toElement) && fromElement->GetType() == TsysType::Member)
				{
					return false;
				}
				if (IsUnknownType(fromElement) && toElement->GetType() == TsysType::Member)
				{
					return false;
				}
				anyInvolved |= _anyInvolved;
				return true;
			}
		case TsysType::Array:
			return IsExactEntityMatch(
				ArrayRemoveOneDim(toEntity),
				ArrayRemoveOneDim(fromEntity),
				anyInvolved
			);
		case TsysType::Function:
			{
				if (!IsExactEntityMatch(toEntity->GetElement(), fromEntity->GetElement(), anyInvolved)) return false;
				if (toEntity->GetParamCount() != fromEntity->GetParamCount()) return false;
				vint count = toEntity->GetParamCount();
				for (vint i = 0; i < count; i++)
				{
					if (!IsExactEntityMatch(toEntity->GetParam(i), fromEntity->GetParam(i), anyInvolved)) return false;
				}
				return true;
			}
		case TsysType::Member:
			return
				IsExactEntityMatch(toEntity->GetElement(), fromEntity->GetElement(), anyInvolved) &&
				IsExactEntityMatch(toEntity->GetClass(), fromEntity->GetClass(), anyInvolved);
		case TsysType::CV:
			{
				auto toElement = toEntity->GetElement();
				auto fromElement = fromEntity->GetElement();
				bool _anyInvolved = false;
				if (!IsExactEntityMatch(toElement, fromElement, _anyInvolved)) return false;

				auto toCV = toEntity->GetCV();
				auto fromCV = fromEntity->GetCV();
				if (IsCVEqual(toCV, fromCV))
				{
					anyInvolved |= _anyInvolved;
					return true;
				}
				else if(IsCVCompatible(toCV, fromCV) && IsUnknownType(fromElement))
				{
					anyInvolved |= _anyInvolved;
					return true;
				}
				else if (IsUnknownType(toElement))
				{
					anyInvolved |= _anyInvolved;
					return true;
				}
				return false;
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
					if (!IsExactEntityMatch(toDI.parentDeclType, fromDI.parentDeclType, anyInvolved)) return false;
				}

				vint count = toEntity->GetParamCount();
				for (vint i = 0; i < count; i++)
				{
					if (!IsExactEntityMatch(toEntity->GetParam(i), fromEntity->GetParam(i), anyInvolved)) return false;
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

	TypeConv PerformExactEntityMatch(ENTITY_VARS(, , _))
	{
		bool anyInvolved = false;
		if (IsExactEntityMatch(toEntity, fromEntity, anyInvolved))
		{
			if (anyInvolved)
			{
				if (IsUnknownType(toEntity) || IsUnknownType(fromEntity))
				{
					return { TypeConvCat::Exact,false,true };
				}

				if (toEntity->GetType() != fromEntity->GetType())
				{
					if (toEntity->GetType() == TsysType::LRef && fromEntity->GetType() == TsysType::RRef)
					{
						if (IsUnknownType(fromEntity->GetElement()))
						{
							return { TypeConvCat::Exact,false,true };
						}
					}
					else if (toEntity->GetType() == TsysType::RRef && fromEntity->GetType() == TsysType::LRef)
					{
						if (IsUnknownType(toEntity->GetElement()))
						{
							return { TypeConvCat::Exact,false,true };
						}
					}
				}
			}

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

/***********************************************************************
Trivial
***********************************************************************/

	TypeConv PerformEnumToIntTrivialConversion(ENTITY_VARS(, , _))
	{
		if (toEntity->GetType() == TsysType::Primitive && fromEntity->GetType() == TsysType::Decl)
		{
			if (auto decl = TryGetForwardDeclFromType<ForwardEnumDeclaration>(fromEntity))
			{
				if (!decl->enumClass)
				{
					auto primitive = toEntity->GetPrimitive();
					if (primitive.type == TsysPrimitiveType::SInt && primitive.bytes == TsysBytes::_4)
					{
						return TypeConvCat::Trivial;
					}
				}
			}
		}
		return TypeConvCat::Illegal;
	}

	TypeConv PerformToPtrTrivialConversion(ENTITY_VARS(, , _))
	{
		bool cvInvolved = false;
		bool allowInheritanceOnly = false;
		if (!AllowEntityTypeChanging(toCV, fromCV, toRef, fromRef, cvInvolved, allowInheritanceOnly))
		{
			return TypeConvCat::Illegal;
		}

		ENTITY_VARS(Element, , ;);
		if (!AllowConvertingToPointer(toEntity, fromEntity, ENTITY_PASS(Element), allowInheritanceOnly))
		{
			return TypeConvCat::Illegal;
		}

		bool anyInvolved = false;
		if (IsExactEntityMatch(toElementEntity, fromElementEntity, anyInvolved))
		{
			if (anyInvolved)
			{
				bool toU = IsUnknownType(toElementEntity);
				bool fromU = IsUnknownType(fromElementEntity);

				if (toU && fromU)
				{
					return { TypeConvCat::Exact,false,true };
				}
				else if (toU)
				{
					if (fromElementEntity->GetType() == TsysType::Member)
					{
						return TypeConvCat::Illegal;
					}
				}
				else if (fromU)
				{
					if (toElementEntity->GetType() == TsysType::Member)
					{
						return TypeConvCat::Illegal;
					}
				}
			}

			if (IsCVEqual(toElementCV, fromElementCV))
			{
				return { TypeConvCat::Exact,false,anyInvolved };
			}
			else if (IsCVCompatible(toElementCV, fromElementCV))
			{
				return { TypeConvCat::Trivial,false,anyInvolved };
			}
		}
		return TypeConvCat::Illegal;
	}

/***********************************************************************
IntegralPromotion
***********************************************************************/

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

/***********************************************************************
Standard
***********************************************************************/

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

/***********************************************************************
UserDefined (Inheriting)
***********************************************************************/

	bool IsInheritingInternal(const ParsingArguments& pa, ITsys* toType, ITsys* fromType, SortedList<ITsys*>& visitedFroms, bool topLevel, bool& anyInvolved)
	{
		if (IsUnknownType(fromType))
		{
			if (topLevel) return false;
			anyInvolved = true;
			return true;
		}

		if (!TryGetDeclFromType<ClassDeclaration>(toType)) return false;
		if (!TryGetDeclFromType<ClassDeclaration>(fromType)) return false;
		if (visitedFroms.Contains(fromType)) return false;
		visitedFroms.Add(fromType);

		if (IsExactEntityMatch(toType, fromType, anyInvolved)) return true;
		auto& ev = symbol_type_resolving::EvaluateClassType(pa, fromType);
		for (vint i = 0; i < ev.ExtraCount(); i++)
		{
			auto& baseTypes = ev.GetExtra(i);
			for (vint j = 0; j < baseTypes.Count(); j++)
			{
				if (IsInheritingInternal(pa, toType, baseTypes[j], visitedFroms, false, anyInvolved))
				{
					return true;
				}
			}
		}
		return false;
	}

	bool IsInheriting(const ParsingArguments& pa, ITsys* toType, ITsys* fromType, bool& anyInvolved)
	{
		SortedList<ITsys*> visitedFroms;
		return IsInheritingInternal(pa, toType, fromType, visitedFroms, true, anyInvolved);
	}

	bool IsToPtrInheriting(const ParsingArguments& pa, ENTITY_VARS(, , _), bool& cvInvolved, bool& anyInvolved)
	{
		ENTITY_VARS(Element, , ;);
		if (!AllowConvertingToPointer(toEntity, fromEntity, ENTITY_PASS(Element), false))
		{
			return false;
		}

		bool _anyInvolved = false;
		if (IsInheriting(pa, toElementEntity, fromElementEntity, _anyInvolved))
		{
			if (IsCVEqual(toElementCV, fromElementCV))
			{
				anyInvolved |= _anyInvolved;
				return true;
			}
			else if (IsCVCompatible(toElementCV, fromElementCV))
			{
				cvInvolved = true;
				anyInvolved |= _anyInvolved;
				return true;
			}
		}
		return false;
	}

/***********************************************************************
UserDefined (fromType:Operator)
***********************************************************************/

	bool IsFromTypeOperator(const ParsingArguments& pa, ENTITY_VARS(, , _), bool& cvInvolved, bool& anyInvolved, TCITestedSet& tested)
	{
		auto fromClass = TryGetDeclFromType<ClassDeclaration>(fromEntity);
		if (!fromClass) return false;

		auto fromSymbol = fromClass->symbol;
		auto pTypeOps = fromSymbol->TryGetChildren_NFb(L"$__type");
		if (!pTypeOps) return false;

		auto toType = CvRefOf(toEntity, toCV, toRef);
		auto newPa = pa.WithScope(fromSymbol);
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
				if (TestFunctionQualifier(fromCV, fromRef, typeOpType).cat == TypeConvCat::Illegal) continue;
			}

			auto& evTypes = symbol_type_resolving::EvaluateFuncSymbol(pa, typeOpDecl.Obj(), (fromEntity->GetType() == TsysType::DeclInstant ? fromEntity : nullptr), nullptr);

			for (vint j = 0; j < evTypes.Count(); j++)
			{
				auto tsys = evTypes[j];
				{
					auto newFromType = tsys->GetElement()->RRefOf();
					if (!tested.Keys().Contains({ toType,newFromType }))
					{
						if (TestTypeConversionInternal(newPa, toType, newFromType, tested).cat != TypeConvCat::Illegal)
						{
							return true;
						}
					}
				}
			}
		}

		return false;
	}

/***********************************************************************
UserDefined (toType:Ctor)
***********************************************************************/

	bool IsToTypeCtor(const ParsingArguments& pa, ENTITY_VARS(, , _), bool& cvInvolved, bool& anyInvolved, TCITestedSet& tested)
	{
		if (toRef == TsysRefType::LRef && !toCV.isGeneralConst) return false;

		auto toClass = TryGetDeclFromType<ClassDeclaration>(toEntity);
		if (!toClass) return false;

		auto toType = CvRefOf(toEntity, toCV, toRef);
		auto fromType = CvRefOf(fromEntity, fromCV, fromRef);

		auto toSymbol = toClass->symbol;
		{
			auto newFromType = pa.tsys->DeclOf(toSymbol)->RRefOf();
			if (TestTypeConversionInternal(pa, toType, newFromType, tested).cat == TypeConvCat::Illegal)
			{
				return false;
			}
		}

		auto pCtors = toSymbol->TryGetChildren_NFb(L"$__ctor");
		if (!pCtors) return false;

		auto newPa = pa.WithScope(toSymbol);
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
			auto& evTypes = symbol_type_resolving::EvaluateFuncSymbol(pa, ctorDecl.Obj(), (toEntity->GetType() == TsysType::DeclInstant ? toEntity : nullptr), nullptr);

			for (vint j = 0; j < evTypes.Count(); j++)
			{
				auto tsys = evTypes[j];
				{
					auto newToType = tsys->GetParam(0);
					if (!tested.Keys().Contains({ newToType,fromType }))
					{
						if (TestTypeConversionInternal(newPa, newToType, fromType, tested).cat != TypeConvCat::Illegal)
						{
							return true;
						}
					}
				}
			}
		}

		return false;
	}

/***********************************************************************
ToVoidPtr
***********************************************************************/

	bool IsToVoidPtrConversion(ITsys* toType, ITsys* fromType, bool& cvInvolved)
	{
		ENTITY_VARS(Element, , ;);
		if (!AllowConvertingToPointer(toType, fromType, ENTITY_PASS(Element), false))
		{
			return false;
		}

		if (toElementEntity->GetType() == TsysType::Primitive && toElementEntity->GetPrimitive().type == TsysPrimitiveType::Void)
		{
			switch (fromElementEntity->GetType())
			{
			case TsysType::Function: return true;
			case TsysType::Member: return false;
			}

			if (IsCVEqual(toElementCV, fromElementCV))
			{
				return true;
			}
			else if (IsCVCompatible(toElementCV, fromElementCV))
			{
				cvInvolved = true;
				return true;
			}
		}
		return false;
	}

/***********************************************************************
Init
***********************************************************************/

	TypeConv IsUniversalInitialization(const ParsingArguments& pa, ENTITY_VARS(, , _), const TsysInit& init, TCITestedSet& tested)
	{
		if (auto toDecl = TryGetDeclFromType<ClassDeclaration>(toEntity))
		{
			auto toSymbol = toDecl->symbol;
			auto pCtors = toSymbol->TryGetChildren_NFb(L"$__ctor");
			if (!pCtors) return TypeConvCat::Illegal;

			ExprTsysList funcTypes;
			for (vint i = 0; i < pCtors->Count(); i++)
			{
				auto ctorSymbol = pCtors->Get(i);
				auto ctorDecl = ctorSymbol->GetAnyForwardDecl<ForwardFunctionDeclaration>();
				if (ctorDecl->decoratorDelete) continue;
				auto& evTypes = symbol_type_resolving::EvaluateFuncSymbol(pa, ctorDecl.Obj(), (toEntity->GetType() == TsysType::DeclInstant ? toEntity : nullptr), nullptr);

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
			bool anyInvolved = false;
			symbol_type_resolving::VisitOverloadedFunction(pa, funcTypes, argTypesList, boundedAnys, result, nullptr, &anyInvolved);
			if (result.Count() > 0)
			{
				return { TypeConvCat::UserDefined,false,anyInvolved };
			}
			else
			{
				return TypeConvCat::Illegal;
			}
		}

		if (fromEntity->GetParamCount() == 0)
		{
			return TypeConvCat::Exact;
		}

		if (fromEntity->GetParamCount() != 1)
		{
			return TypeConvCat::Illegal;
		}

		auto type = fromEntity->GetParam(0);
		{
			TsysCV cv;
			TsysRefType ref;
			auto entity = type->GetEntity(cv, ref);
			if (entity->GetType() == TsysType::Init)
			{
				return TypeConvCat::Illegal;
			}
		}

		return TestTypeConversionInternal(
			pa,
			CvRefOf(toEntity, toCV, toRef),
			ApplyExprTsysType(type, init.headers[0].type),
			tested
		);
	}

/***********************************************************************
TestTypeConversionImpl
***********************************************************************/

	TypeConv TestTypeConversionImpl(const ParsingArguments& pa, ITsys* toType, ITsys* fromType, TCITestedSet& tested)
	{
		EXTRACT_ENTITY(Type,);

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

#pragma warning (push)
#pragma warning (disable: 4003)

		{
			auto result = PerformExactEntityMatch(ENTITY_PASS());
			if (result.cat != TypeConvCat::Illegal) return result;
		}
		{
			auto result = PerformEnumToIntTrivialConversion(ENTITY_PASS());
			if (result.cat != TypeConvCat::Illegal) return result;
		}
		{
			auto result = PerformToPtrTrivialConversion(ENTITY_PASS());
			if (result.cat != TypeConvCat::Illegal) return result;
		}

		bool cvInvolved = false;
		bool anyInvolved = false;
		bool allowInheritanceOnly = false;
		if (!AllowEntityTypeChanging(toCV, fromCV, toRef, fromRef, cvInvolved, allowInheritanceOnly))
		{
			return TypeConvCat::Illegal;
		}

		if (!allowInheritanceOnly && fromEntity->GetType() == TsysType::Init)
		{
			return IsUniversalInitialization(pa, ENTITY_PASS(), fromEntity->GetInit(), tested);
		}

		if (!allowInheritanceOnly && IsNumericPromotion(toEntity, fromEntity))
		{
			return { TypeConvCat::IntegralPromotion,cvInvolved,false };
		}

		if (!allowInheritanceOnly && IsNumericConversion(toEntity, fromEntity))
		{
			return { TypeConvCat::Standard,cvInvolved,false };
		}

		if (IsInheriting(pa, toEntity, fromEntity, anyInvolved))
		{
			return { TypeConvCat::Standard,cvInvolved,anyInvolved };
		}

		if (!allowInheritanceOnly && IsToPtrInheriting(pa, ENTITY_PASS(), cvInvolved, anyInvolved))
		{
			return { TypeConvCat::Standard,cvInvolved,anyInvolved };
		}

		if (!allowInheritanceOnly && IsFromTypeOperator(pa, ENTITY_PASS(), cvInvolved, anyInvolved, tested))
		{
			return { TypeConvCat::UserDefined,cvInvolved,anyInvolved };
		}

		if (!allowInheritanceOnly && IsToTypeCtor(pa, ENTITY_PASS(), cvInvolved, anyInvolved, tested))
		{
			return { TypeConvCat::UserDefined,cvInvolved,anyInvolved };
		}

		if (!allowInheritanceOnly && IsToVoidPtrConversion(toEntity, fromEntity, cvInvolved))
		{
			return { TypeConvCat::ToVoidPtr,cvInvolved,false };
		}

#pragma warning (pop)
		return TypeConvCat::Illegal;
	}

#undef _
#undef ENTITY_PASS
#undef ENTITY_VARS
#undef EXTRACT_ENTITY
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