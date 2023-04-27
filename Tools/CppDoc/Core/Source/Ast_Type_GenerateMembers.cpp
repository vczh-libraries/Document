#include "Ast.h"
#include "Ast_Type.h"
#include "EvaluateSymbol.h"

/***********************************************************************
GetSpecialMember
***********************************************************************/

Symbol* GetSpecialMember(const ParsingArguments& pa, Symbol* classSymbol, ITsys* classType, SpecialMemberKind kind)
{
	{
		auto classDecl = classSymbol->GetImplDecl_NFb<ClassDeclaration>();
		if (!classDecl) return nullptr;
		if (classDecl->classType == CppClassType::Union) return nullptr;
	}

	WString memberName;
	switch (kind)
	{
	case SpecialMemberKind::DefaultCtor:
	case SpecialMemberKind::CopyCtor:
	case SpecialMemberKind::MoveCtor:
		memberName = WString::Unmanaged(L"$__ctor");
		break;
	case SpecialMemberKind::CopyAssignOp:
	case SpecialMemberKind::MoveAssignOp:
		memberName = WString::Unmanaged(L"operator =");
		break;
	case SpecialMemberKind::Dtor:
		memberName = L"~" + classSymbol->name;
		break;
	}

	if (auto pMembers = classSymbol->TryGetChildren_NFb(memberName))
	{
		for (vint i = 0; i < pMembers->Count(); i++)
		{
			auto& childSymbol = pMembers->Get(i);
			if (!childSymbol.childExpr && !childSymbol.childType)
			{
				auto member = childSymbol.childSymbol.Obj();
				auto forwardFunc = member->GetAnyForwardDecl<ForwardFunctionDeclaration>();
				if (!forwardFunc) continue;
				if (symbol_type_resolving::IsStaticSymbol<ForwardFunctionDeclaration>(member)) continue;

				auto& types = symbol_type_resolving::EvaluateFuncSymbol(pa, forwardFunc.Obj(), nullptr, nullptr);
				for (vint j = 0; j < types.Count(); j++)
				{
					auto funcType = types[j];
					if (funcType->GetType() != TsysType::Function)
					{
						continue;
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
							if (refType == TsysRefType::LRef)
							{
								if (entity == classType)
								{
									return member;
								}
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
							if (refType == TsysRefType::RRef)
							{
								if (entity == classType)
								{
									return member;
								}
							}
						}
						break;
					}
				}
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
	auto forwardFunc = member->GetAnyForwardDecl<ForwardFunctionDeclaration>();
	if (!forwardFunc) return false;
	if (forwardFunc->decoratorDefault) return true;
	if (forwardFunc->decoratorDelete) return false;
	return true;
}

bool IsSpecialMemberFeatureEnabled(const ParsingArguments& pa, ITsys* classType, SpecialMemberKind kind)
{
	switch (classType->GetType())
	{
	case TsysType::Decl:
	case TsysType::DeclInstant:
		break;
	default:
		throw TypeCheckerException();
	}

	auto classSymbol = classType->GetDecl();

	Symbol* symbolDefaultCtor = nullptr;
	Symbol* symbolCopyCtor = nullptr;
	Symbol* symbolMoveCtor = nullptr;
	Symbol* symbolCopyAssignOp = nullptr;
	Symbol* symbolMoveAssignOp = nullptr;
	Symbol* symbolDtor = nullptr;

#define SYMBOL(KIND) (symbol##KIND ? symbol##KIND : (symbol##KIND = GetSpecialMember(pa, classSymbol, classType, SpecialMemberKind::KIND)))
#define DEFINED(KIND) (SYMBOL(KIND) != nullptr)
#define DELETED(KIND) (SYMBOL(KIND) && symbol##KIND->GetAnyForwardDecl<ForwardFunctionDeclaration>()->decoratorDelete)
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
	case TsysType::DeclInstant:
		{
			auto classDecl = type->GetDecl()->GetImplDecl_NFb<ClassDeclaration>();
			if (!classDecl) return true;
			if (classDecl->classType == CppClassType::Union) return true;
			return IsSpecialMemberFeatureEnabled(pa, type, kind);
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
		throw TypeCheckerException();
	}
	throw TypeCheckerException();
}

/***********************************************************************
GenerateMembers
***********************************************************************/

bool IsSpecialMemberBlockedByDefinition(const ParsingArguments& pa, ClassDeclaration* classDecl, SpecialMemberKind kind, bool passIfFieldHasInitializer)
{
	{
		bool exit = false;
		symbol_type_resolving::EnumerateClassSymbolBaseTypes(pa, classDecl, nullptr, nullptr, [&](ITsys* classType, ITsys* baseType)
		{
			if (!IsSpecialMemberEnabledForType(pa, baseType, kind))
			{
				exit = true;
			}
			return exit;
		});

		if (exit)
		{
			return true;
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

			bool isVariadic = false;
			auto& types = symbol_type_resolving::EvaluateVarSymbol(pa, varDecl.Obj(), nullptr, isVariadic);
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

Ptr<VariableDeclaration> GenerateCopyParameter(const ParsingArguments& pa, Symbol* classSymbol)
{
	auto idType = Ptr(new IdType);
	idType->name.name = classSymbol->name;
	idType->name.type = CppNameType::Normal;
	Resolving::AddSymbol(pa, idType->resolving, classSymbol);

	auto decoratedType = Ptr(new DecorateType);
	decoratedType->type = idType;
	decoratedType->isConst = true;

	auto refType = Ptr(new ReferenceType);
	refType->type = decoratedType;
	refType->reference = CppReferenceType::LRef;

	auto parameter = Ptr(new VariableDeclaration);
	parameter->type = refType;
	return parameter;
}

Ptr<Type> GenerateRefType(const ParsingArguments& pa, Symbol* classSymbol, CppReferenceType ref)
{
	auto idType = Ptr(new IdType);
	idType->name.name = classSymbol->name;
	idType->name.type = CppNameType::Normal;
	Resolving::AddSymbol(pa, idType->resolving, classSymbol);

	auto refType = Ptr(new ReferenceType);
	refType->type = idType;
	refType->reference = ref;
	return refType;
}

Ptr<VariableDeclaration> GenerateMoveParameter(const ParsingArguments& pa, Symbol* classSymbol)
{
	auto parameter = Ptr(new VariableDeclaration);
	parameter->type = GenerateRefType(pa, classSymbol, CppReferenceType::RRef);
	return parameter;
}

Ptr<ForwardFunctionDeclaration> GenerateCtor(Symbol* classSymbol, bool deleted, Ptr<VariableDeclaration> parameter)
{
	auto decl = Ptr(new ForwardFunctionDeclaration);
	decl->name.name = L"$__ctor";
	decl->name.type = CppNameType::Constructor;
	decl->methodType = CppMethodType::Constructor;
	{
		auto funcType = Ptr(new FunctionType);
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

Ptr<ForwardFunctionDeclaration> GenerateAssignOp(const ParsingArguments& pa, Symbol* classSymbol, bool deleted, Ptr<VariableDeclaration> parameter)
{
	auto decl = Ptr(new ForwardFunctionDeclaration);
	decl->name.name = L"operator =";
	decl->name.type = CppNameType::Operator;
	decl->methodType = CppMethodType::Function;
	{
		auto funcType = Ptr(new FunctionType);
		decl->type = funcType;
		
		funcType->returnType = GenerateRefType(pa, classSymbol, CppReferenceType::LRef);
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
			// TODO: [Cpp.md] Deal with DeclInstant here
			// Find a way to store different results for different type argument combinations
			auto& ev = symbol_type_resolving::EvaluateClassSymbol(pa, classDecl.Obj(), nullptr, nullptr);
			auto classType = ev.Get()[0];
			if (classType->GetType() == TsysType::GenericFunction)
			{
				classType = classType->GetElement();
			}

			auto symbolDefaultCtor = GetSpecialMember(pa, classSymbol, classType, SpecialMemberKind::DefaultCtor);
			auto symbolCopyCtor = GetSpecialMember(pa, classSymbol, classType, SpecialMemberKind::CopyCtor);
			auto symbolMoveCtor = GetSpecialMember(pa, classSymbol, classType, SpecialMemberKind::MoveCtor);
			auto symbolCopyAssignOp = GetSpecialMember(pa, classSymbol, classType, SpecialMemberKind::CopyAssignOp);
			auto symbolMoveAssignOp = GetSpecialMember(pa, classSymbol, classType, SpecialMemberKind::MoveAssignOp);
			auto symbolDtor = GetSpecialMember(pa, classSymbol, classType, SpecialMemberKind::Dtor);

			auto enabledCopyCtor = IsSpecialMemberEnabled(symbolCopyCtor);
			auto enabledCopyAssignOp = IsSpecialMemberEnabled(symbolCopyAssignOp);

			bool ctorDefined = classSymbol->TryGetChildren_NFb(L"$__ctor") != nullptr;
			bool assignDefined = classSymbol->TryGetChildren_NFb(L"operator =") != nullptr;

			List<Ptr<ForwardFunctionDeclaration>> generatedMembers;
			bool generatedEnabledCopyCtor = false;
			bool generatedEnabledCopyAssignOp = false;

			if (!ctorDefined)
			{
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

				generatedMembers.Add(GenerateCtor(classSymbol, deleted, GenerateCopyParameter(pa, classSymbol)));
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
					generatedMembers.Add(GenerateCtor(classSymbol, deleted, GenerateMoveParameter(pa, classSymbol)));
				}
			}

			if (!assignDefined)
			{
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

					generatedMembers.Add(GenerateAssignOp(pa, classSymbol, deleted, GenerateCopyParameter(pa, classSymbol)));
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
						generatedMembers.Add(GenerateAssignOp(pa, classSymbol, deleted, GenerateMoveParameter(pa, classSymbol)));
					}
				}
			}

			if (!symbolDtor)
			{
				bool deleted = true;
				if (!IsSpecialMemberBlockedByDefinition(pa, classDecl.Obj(), SpecialMemberKind::Dtor, false))
				{
					deleted = false;
				}

				auto decl = Ptr(new ForwardFunctionDeclaration);
				decl->name.name = L"~" + classSymbol->name;
				decl->name.type = CppNameType::Destructor;
				decl->methodType = CppMethodType::Destructor;
				decl->type = Ptr(new FunctionType);
				if (!deleted) decl->decoratorDefault = true;
				if (deleted) decl->decoratorDelete = true;

				generatedMembers.Add(decl);
			}

			for (vint i = 0; i < generatedMembers.Count(); i++)
			{
				auto decl = generatedMembers[i];
				decl->implicitlyGeneratedMember = true;
				classDecl->decls.Add({ CppClassAccessor::Public,decl });
				classSymbol->CreateFunctionSymbol_NFb(decl)->CreateFunctionForwardSymbol_F(decl, nullptr);
			}
		}
	}
}