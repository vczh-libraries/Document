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
GetSpecialMember
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
GenerateMembers
***********************************************************************/

void GenerateMembers(const ParsingArguments& pa, Symbol* classSymbol)
{
}