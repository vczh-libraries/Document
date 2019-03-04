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
	auto classDecl = classSymbol->declaration.Cast<ClassDeclaration>();
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

	vint index = classSymbol->children.Keys().IndexOf(memberName);
	if (index == -1) return nullptr;

	auto& members = classSymbol->children.GetByIndex(index);
	for (vint i = 0; i < members.Count(); i++)
	{
		auto member = members[i].Obj();
		auto forwardFunc = member->GetAnyForwardDecl<ForwardFunctionDeclaration>();
		if (!forwardFunc) continue;
		if (symbol_type_resolving::IsStaticSymbol<ForwardFunctionDeclaration>(member)) continue;

		symbol_type_resolving::EvaluateSymbol(pa, forwardFunc.Obj());
		if (member->evaluation.Count() == 1)
		{
			auto& types = member->evaluation.Get();
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
	}
	return nullptr;
}

/***********************************************************************
IsSpecialMemberEnabled
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

/***********************************************************************
IsSpecialMemberEnabledForType
***********************************************************************/

bool IsSpecialMemberEnabledForType(const ParsingArguments& pa, ITsys* type, SpecialMemberKind kind)
{
	switch (type->GetType())
	{
	case TsysType::Primitive:
	case TsysType::Ptr:
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
			if (!type->GetDecl()->declaration) return false;
			auto classDecl = type->GetDecl()->declaration.Cast<ClassDeclaration>();
			if (!classDecl) return true;
			if (classDecl->classType == CppClassType::Union) return true;
			auto specialMember = GetSpecialMember(pa, classDecl->symbol, kind);
			return IsSpecialMemberEnabled(specialMember);
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
	symbol_type_resolving::EvaluateSymbol(pa, classDecl);
	auto classSymbol = classDecl->symbol;
	for (vint i = 0; i < classSymbol->evaluation.Count(); i++)
	{
		auto& types = classSymbol->evaluation.Get(i);
		for (vint j = 0; j < types.Count(); j++)
		{
			if (!IsSpecialMemberEnabledForType(pa, types[j], SpecialMemberKind::DefaultCtor))
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
			if (varDecl->symbol->evaluation.Count() == 1)
			{
				auto& types = varDecl->symbol->evaluation.Get();
				for (vint j = 0; j < types.Count(); j++)
				{
					if (!IsSpecialMemberEnabledForType(pa, types[j], SpecialMemberKind::DefaultCtor))
					{
						return true;
					}
				}
			}
		}
	}
	return false;
}

void GenerateMembers(const ParsingArguments& pa, Symbol* classSymbol)
{
	if (auto classDecl = classSymbol->declaration.Cast<ClassDeclaration>())
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

			if (!symbolDefaultCtor)
			{
				bool deleted = true;
				if (!IsSpecialMemberBlockedByDefinition(pa, classDecl.Obj(), SpecialMemberKind::DefaultCtor, true))
				{
					if (!classSymbol->children.Keys().Contains(L"$__ctor"))
					{
						deleted = false;
					}
				}

				auto decl = MakePtr<ForwardFunctionDeclaration>();
				decl->name.name = L"$__ctor";
				decl->name.type = CppNameType::Constructor;
				decl->methodType = CppMethodType::Constructor;
				decl->type = MakePtr<FunctionType>();
				if (!deleted) decl->decoratorDefault = true;
				if (deleted) decl->decoratorDelete = true;

				generatedMembers.Add(decl);
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

				auto decl = MakePtr<ForwardFunctionDeclaration>();
				decl->name.name = L"$__ctor";
				decl->name.type = CppNameType::Constructor;
				decl->methodType = CppMethodType::Constructor;
				{
					auto funcType = MakePtr<FunctionType>();
					decl->type = funcType;

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
					funcType->parameters.Add(parameter);
				}
				if (!deleted) decl->decoratorDefault = true;
				if (deleted) decl->decoratorDelete = true;

				generatedMembers.Add(decl);
			}
			if (!symbolMoveCtor)
			{
				if (!IsSpecialMemberBlockedByDefinition(pa, classDecl.Obj(), SpecialMemberKind::MoveCtor, false))
				{
				}
			}
			if (!symbolCopyAssignOp)
			{
				if (!IsSpecialMemberBlockedByDefinition(pa, classDecl.Obj(), SpecialMemberKind::CopyAssignOp, false))
				{
				}
			}
			if (!symbolMoveAssignOp)
			{
				if (!IsSpecialMemberBlockedByDefinition(pa, classDecl.Obj(), SpecialMemberKind::MoveAssignOp, false))
				{
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
				classDecl->decls.Add({ CppClassAccessor::Public,decl });
				classSymbol->CreateForwardDeclSymbol(decl, nullptr, symbol_component::SymbolKind::Function);
			}
		}
	}
}