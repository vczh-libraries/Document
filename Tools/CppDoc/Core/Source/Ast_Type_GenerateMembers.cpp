#include "Ast.h"
#include "Ast_Type.h"
#include "Ast_Decl.h"
#include "Ast_Resolving.h"
#include "Parser.h"

/***********************************************************************
GetSpecialMember
***********************************************************************/

Symbol* GetSpecialMember(const ParsingArguments& pa, Symbol* classSymbol, SpecialMemberKind kind)
{
	auto classDecl = classSymbol->GetImplDecl_NFb<ClassDeclaration>();
	if (!classDecl) return nullptr;
	if (classDecl->classType == CppClassType::Union) return nullptr;

	WString memberName;
	switch (kind)
	{
	case SpecialMemberKind::DefaultCtor:
	case SpecialMemberKind::CopyCtor:
	case SpecialMemberKind::MoveCtor:
		memberName = WString(L"$__ctor", false);
		break;
	case SpecialMemberKind::CopyAssignOp:
	case SpecialMemberKind::MoveAssignOp:
		memberName = WString(L"operator =", false);
		break;
	case SpecialMemberKind::Dtor:
		memberName = L"~" + classSymbol->name;
		break;
	}

	auto pMembers = classSymbol->TryGetChildren_NFb(memberName);
	if (!pMembers) return nullptr;
	for (vint i = 0; i < pMembers->Count(); i++)
	{
		auto member = pMembers->Get(i).Obj();
		auto forwardFunc = member->GetAnyForwardDecl_NFFb<ForwardFunctionDeclaration>();
		if (!forwardFunc) continue;
		if (symbol_type_resolving::IsStaticSymbol<ForwardFunctionDeclaration>(member)) continue;

		auto& types = symbol_type_resolving::EvaluateFuncSymbol(pa, forwardFunc.Obj());
		for (vint j = 0; j < types.Count(); j++)
		{
			auto funcType = types[j];
			if (funcType->GetType() != TsysType::Function)
			{
				throw NotResolvableException();
			}

			switch (kind)
			{
			case SpecialMemberKind::DefaultCtor:
			case SpecialMemberKind::Dtor:
				if (funcType->GetParamCount() == 0)
				{
					return member;
				}
				break;
			case SpecialMemberKind::CopyCtor:
			case SpecialMemberKind::CopyAssignOp:
				if (funcType->GetParamCount() == 1)
				{
					auto paramType = funcType->GetParam(0);
					TsysCV cv;
					TsysRefType refType;
					auto entity = paramType->GetEntity(cv, refType);
					if (refType == TsysRefType::LRef && entity->GetType() == TsysType::Decl && entity->GetDecl() == classSymbol)
					{
						return member;
					}
				}
				break;
			case SpecialMemberKind::MoveCtor:
			case SpecialMemberKind::MoveAssignOp:
				if (funcType->GetParamCount() == 1)
				{
					auto paramType = funcType->GetParam(0);
					TsysCV cv;
					TsysRefType refType;
					auto entity = paramType->GetEntity(cv, refType);
					if (refType == TsysRefType::RRef && entity->GetType() == TsysType::Decl && entity->GetDecl() == classSymbol)
					{
						return member;
					}
				}
				break;
			}
		}
	}
	return nullptr;
}

/***********************************************************************
IsSpecialMemberFeatureEnabled
***********************************************************************/

bool IsSpecialMemberEnabled(Symbol* member)
{
	if (!member) return false;
	auto forwardFunc = member->GetAnyForwardDecl_NFFb<ForwardFunctionDeclaration>();
	if (!forwardFunc) return false;
	if (forwardFunc->decoratorDefault) return true;
	if (forwardFunc->decoratorDelete) return false;
	return true;
}

bool IsSpecialMemberFeatureEnabled(const ParsingArguments& pa, Symbol* classSymbol, SpecialMemberKind kind)
{
	Symbol* symbolDefaultCtor = nullptr;
	Symbol* symbolCopyCtor = nullptr;
	Symbol* symbolMoveCtor = nullptr;
	Symbol* symbolCopyAssignOp = nullptr;
	Symbol* symbolMoveAssignOp = nullptr;
	Symbol* symbolDtor = nullptr;

#define SYMBOL(KIND) (symbol##KIND ? symbol##KIND : (symbol##KIND = GetSpecialMember(pa, classSymbol, SpecialMemberKind::KIND)))
#define DEFINED(KIND) (SYMBOL(KIND) != nullptr)
#define DELETED(KIND) (SYMBOL(KIND) && symbol##KIND->GetAnyForwardDecl_NFFb<ForwardFunctionDeclaration>()->decoratorDelete)
#define ENABLED(KIND) IsSpecialMemberEnabled(SYMBOL(KIND))

	switch (kind)
	{
	case SpecialMemberKind::DefaultCtor:	return ENABLED(DefaultCtor) && ENABLED(Dtor);
	case SpecialMemberKind::CopyCtor:		return ENABLED(CopyCtor) && ENABLED(Dtor);
	case SpecialMemberKind::MoveCtor:		return (ENABLED(CopyCtor) || ENABLED(MoveCtor)) && !DELETED(MoveCtor) && ENABLED(Dtor);
	case SpecialMemberKind::CopyAssignOp:	return ENABLED(CopyAssignOp);
	case SpecialMemberKind::MoveAssignOp:	return (ENABLED(CopyAssignOp) || ENABLED(MoveAssignOp)) && !DELETED(MoveAssignOp);
	case SpecialMemberKind::Dtor:			return ENABLED(Dtor);
	}
	return false;

#undef SYMBOL
#undef DEFINED
#undef DELETED
#undef ENABLED
}

/***********************************************************************
IsSpecialMemberEnabledForType
***********************************************************************/

bool IsSpecialMemberEnabledForType(const ParsingArguments& pa, ITsys* type, SpecialMemberKind kind)
{
	switch (type->GetType())
	{
	case TsysType::Any:
	case TsysType::Primitive:
	case TsysType::Ptr:
	case TsysType::GenericArg:
		return true;
	case TsysType::LRef:
	case TsysType::RRef:
		switch (kind)
		{
		case SpecialMemberKind::DefaultCtor: return false;
		case SpecialMemberKind::CopyCtor: return true;
		case SpecialMemberKind::MoveCtor: return true;
		case SpecialMemberKind::CopyAssignOp: return false;
		case SpecialMemberKind::MoveAssignOp: return false;
		case SpecialMemberKind::Dtor: return true;
		}
		break;
	case TsysType::Array:
		return IsSpecialMemberEnabledForType(pa, type->GetElement(), kind);
	case TsysType::CV:
		switch (kind)
		{
		case SpecialMemberKind::DefaultCtor: return false;
		case SpecialMemberKind::CopyCtor: return IsSpecialMemberEnabledForType(pa, type->GetElement(), kind);
		case SpecialMemberKind::MoveCtor: return IsSpecialMemberEnabledForType(pa, type->GetElement(), kind);
		case SpecialMemberKind::CopyAssignOp: return false;
		case SpecialMemberKind::MoveAssignOp: return false;
		case SpecialMemberKind::Dtor: return true;
		}
		break;
	case TsysType::Decl:
		{
			auto classDecl = type->GetDecl()->GetImplDecl_NFb<ClassDeclaration>();
			if (!classDecl) return true;
			if (classDecl->classType == CppClassType::Union) return true;
			return IsSpecialMemberFeatureEnabled(pa, classDecl->symbol, kind);
		}
	case TsysType::Init:
		for (vint i = 0; i < type->GetParamCount(); i++)
		{
			if (!IsSpecialMemberEnabledForType(pa, type->GetParam(i), kind))
			{
				return false;
			}
		}
		return true;
	case TsysType::Function:
	case TsysType::Member:
	case TsysType::GenericFunction:
		// these type cannot be used as a value
		throw NotResolvableException();
	}
	throw NotResolvableException();
}

/***********************************************************************
GenerateMembers
***********************************************************************/

bool IsSpecialMemberBlockedByDefinition(const ParsingArguments& pa, ClassDeclaration* classDecl, SpecialMemberKind kind, bool passIfFieldHasInitializer)
{
	auto& ev = symbol_type_resolving::EvaluateClassSymbol(pa, classDecl);
	auto classSymbol = classDecl->symbol;
	for (vint i = 0; i < ev.Count(); i++)
	{
		auto& types = ev.Get(i);
		for (vint j = 0; j < types.Count(); j++)
		{
			if (!IsSpecialMemberEnabledForType(pa, types[j], kind))
			{
				return true;
			}
		}
	}

	for (vint i = 0; i < classDecl->decls.Count(); i++)
	{
		if (auto varDecl = classDecl->decls[i].f1.Cast<VariableDeclaration>())
		{
			if (passIfFieldHasInitializer && varDecl->initializer)
			{
				continue;
			}

			auto& types = symbol_type_resolving::EvaluateVarSymbol(pa, varDecl.Obj());
			for (vint j = 0; j < types.Count(); j++)
			{
				if (!IsSpecialMemberEnabledForType(pa, types[j], kind))
				{
					return true;
				}
			}
		}
	}
	return false;
}

Ptr<VariableDeclaration> GenerateCopyParameter(Symbol* classSymbol)
{
	auto idType = MakePtr<IdType>();
	idType->name.name = classSymbol->name;
	idType->name.type = CppNameType::Normal;
	idType->resolving = MakePtr<Resolving>();
	idType->resolving->resolvedSymbols.Add(classSymbol);

	auto decoratedType = MakePtr<DecorateType>();
	decoratedType->type = idType;
	decoratedType->isConst = true;

	auto refType = MakePtr<ReferenceType>();
	refType->type = decoratedType;
	refType->reference = CppReferenceType::LRef;

	auto parameter = MakePtr<VariableDeclaration>();
	parameter->type = refType;
	return parameter;
}

Ptr<Type> GenerateRefType(Symbol* classSymbol, CppReferenceType ref)
{
	auto idType = MakePtr<IdType>();
	idType->name.name = classSymbol->name;
	idType->name.type = CppNameType::Normal;
	idType->resolving = MakePtr<Resolving>();
	idType->resolving->resolvedSymbols.Add(classSymbol);

	auto refType = MakePtr<ReferenceType>();
	refType->type = idType;
	refType->reference = ref;
	return refType;
}

Ptr<VariableDeclaration> GenerateMoveParameter(Symbol* classSymbol)
{
	auto parameter = MakePtr<VariableDeclaration>();
	parameter->type = GenerateRefType(classSymbol, CppReferenceType::RRef);
	return parameter;
}

Ptr<ForwardFunctionDeclaration> GenerateCtor(Symbol* classSymbol, bool deleted, Ptr<VariableDeclaration> parameter)
{
	auto decl = MakePtr<ForwardFunctionDeclaration>();
	decl->name.name = L"$__ctor";
	decl->name.type = CppNameType::Constructor;
	decl->methodType = CppMethodType::Constructor;
	{
		auto funcType = MakePtr<FunctionType>();
		decl->type = funcType;
		if (parameter)
		{
			funcType->parameters.Add({ parameter,false });
		}
	}
	if (!deleted) decl->decoratorDefault = true;
	if (deleted) decl->decoratorDelete = true;
	return decl;
}

Ptr<ForwardFunctionDeclaration> GenerateAssignOp(Symbol* classSymbol, bool deleted, Ptr<VariableDeclaration> parameter)
{
	auto decl = MakePtr<ForwardFunctionDeclaration>();
	decl->name.name = L"operator =";
	decl->name.type = CppNameType::Operator;
	decl->methodType = CppMethodType::Function;
	{
		auto funcType = MakePtr<FunctionType>();
		decl->type = funcType;
		
		funcType->returnType = GenerateRefType(classSymbol, CppReferenceType::LRef);
		funcType->parameters.Add({ parameter,false });
	}
	if (!deleted) decl->decoratorDefault = true;
	if (deleted) decl->decoratorDelete = true;
	return decl;
}

void GenerateMembers(const ParsingArguments& pa, Symbol* classSymbol)
{
	if (auto classDecl = classSymbol->GetImplDecl_NFb<ClassDeclaration>())
	{
		if (classDecl->classType != CppClassType::Union)
		{
			auto symbolDefaultCtor = GetSpecialMember(pa, classSymbol, SpecialMemberKind::DefaultCtor);
			auto symbolCopyCtor = GetSpecialMember(pa, classSymbol, SpecialMemberKind::CopyCtor);
			auto symbolMoveCtor = GetSpecialMember(pa, classSymbol, SpecialMemberKind::MoveCtor);
			auto symbolCopyAssignOp = GetSpecialMember(pa, classSymbol, SpecialMemberKind::CopyAssignOp);
			auto symbolMoveAssignOp = GetSpecialMember(pa, classSymbol, SpecialMemberKind::MoveAssignOp);
			auto symbolDtor = GetSpecialMember(pa, classSymbol, SpecialMemberKind::Dtor);

			auto enabledDefaultCtor = IsSpecialMemberEnabled(symbolDefaultCtor);
			auto enabledCopyCtor = IsSpecialMemberEnabled(symbolCopyCtor);
			auto enabledMoveCtor = IsSpecialMemberEnabled(symbolMoveCtor);
			auto enabledCopyAssignOp = IsSpecialMemberEnabled(symbolCopyAssignOp);
			auto enabledMoveAssignOp = IsSpecialMemberEnabled(symbolMoveAssignOp);
			auto enabledDtor = IsSpecialMemberEnabled(symbolDtor);

			List<Ptr<ForwardFunctionDeclaration>> generatedMembers;
			bool generatedEnabledCopyCtor = false;
			bool generatedEnabledCopyAssignOp = false;

			if (!symbolDefaultCtor)
			{
				bool deleted = true;
				if (!IsSpecialMemberBlockedByDefinition(pa, classDecl.Obj(), SpecialMemberKind::DefaultCtor, true))
				{
					if (!classSymbol->TryGetChildren_NFb(L"$__ctor"))
					{
						deleted = false;
					}
				}

				generatedMembers.Add(GenerateCtor(classSymbol, deleted, nullptr));
			}
			if (!symbolCopyCtor)
			{
				bool deleted = true;
				if (!IsSpecialMemberBlockedByDefinition(pa, classDecl.Obj(), SpecialMemberKind::CopyCtor, false))
				{
					if (!symbolMoveCtor && !symbolMoveAssignOp)
					{
						deleted = false;
					}
				}

				generatedMembers.Add(GenerateCtor(classSymbol, deleted, GenerateCopyParameter(classSymbol)));
				generatedEnabledCopyCtor = !deleted;
			}
			if (!symbolMoveCtor)
			{
				bool deleted = true;
				if (!IsSpecialMemberBlockedByDefinition(pa, classDecl.Obj(), SpecialMemberKind::MoveCtor, false))
				{
					if (!symbolCopyCtor && !symbolCopyAssignOp && !symbolMoveAssignOp && !symbolDtor)
					{
						deleted = false;
					}
				}

				if (!deleted || !(enabledCopyCtor || generatedEnabledCopyCtor))
				{
					generatedMembers.Add(GenerateCtor(classSymbol, deleted, GenerateMoveParameter(classSymbol)));
				}
			}
			if (!symbolCopyAssignOp)
			{
				bool deleted = true;
				if (!IsSpecialMemberBlockedByDefinition(pa, classDecl.Obj(), SpecialMemberKind::CopyAssignOp, false))
				{
					if (!symbolMoveCtor && !symbolMoveAssignOp)
					{
						deleted = false;
					}
				}

				generatedMembers.Add(GenerateAssignOp(classSymbol, deleted, GenerateCopyParameter(classSymbol)));
				generatedEnabledCopyAssignOp = !deleted;
			}
			if (!symbolMoveAssignOp)
			{
				bool deleted = true;
				if (!IsSpecialMemberBlockedByDefinition(pa, classDecl.Obj(), SpecialMemberKind::MoveAssignOp, false))
				{
					if (!symbolCopyCtor && !symbolCopyAssignOp && !symbolMoveCtor && !symbolDtor)
					{
						deleted = false;
					}
				}

				if (!deleted || !(enabledCopyAssignOp || generatedEnabledCopyAssignOp))
				{
					generatedMembers.Add(GenerateAssignOp(classSymbol, deleted, GenerateMoveParameter(classSymbol)));
				}
			}
			if (!symbolDtor)
			{
				bool deleted = true;
				if (!IsSpecialMemberBlockedByDefinition(pa, classDecl.Obj(), SpecialMemberKind::Dtor, false))
				{
					deleted = false;
				}

				auto decl = MakePtr<ForwardFunctionDeclaration>();
				decl->name.name = L"~" + classSymbol->name;
				decl->name.type = CppNameType::Destructor;
				decl->methodType = CppMethodType::Destructor;
				decl->type = MakePtr<FunctionType>();
				if (!deleted) decl->decoratorDefault = true;
				if (deleted) decl->decoratorDelete = true;

				generatedMembers.Add(decl);
			}

			for (vint i = 0; i < generatedMembers.Count(); i++)
			{
				auto decl = generatedMembers[i];
				decl->implicitlyGeneratedMember = true;
				classDecl->decls.Add({ CppClassAccessor::Public,decl });
				classSymbol->CreateFunctionSymbol_NFb(decl)->CreateFunctionForwardSymbol_F(decl);
			}
		}
	}
}