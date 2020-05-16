#ifndef VCZH_DOCUMENT_CPPDOC_AST_EVALUATESYMBOL
#define VCZH_DOCUMENT_CPPDOC_AST_EVALUATESYMBOL

#include "Symbol.h"
#include "Ast_Decl.h"

namespace symbol_type_resolving
{
	template<typename TForward>
	bool IsStaticSymbol(Symbol* symbol)
	{
		bool isStatic = false;
		switch (symbol->GetCategory())
		{
		case symbol_component::SymbolCategory::Normal:
			if (auto decl = symbol->GetImplDecl_NFb<TForward>())
			{
				isStatic |= decl->decoratorStatic;
			}
			{
				const auto& decls = symbol->GetForwardDecls_N();
				for (vint i = 0; i < decls.Count(); i++)
				{
					if (auto decl = decls[i].Cast<TForward>())
					{
						isStatic |= decl->decoratorStatic;
					}
				}
			}
			break;
		case symbol_component::SymbolCategory::FunctionBody:
			if (auto decl = symbol->GetAnyForwardDecl<TForward>())
			{
				isStatic |= decl->decoratorStatic;
			}
			break;
		case symbol_component::SymbolCategory::Function:
			{
				const auto& symbols = symbol->GetForwardSymbols_F();
				for (vint i = 0; i < symbols.Count(); i++)
				{
					isStatic |= IsStaticSymbol<TForward>(symbols[i].Obj());
				}
			}
			{
				const auto& symbols = symbol->GetImplSymbols_F();
				for (vint i = 0; i < symbols.Count(); i++)
				{
					isStatic |= IsStaticSymbol<TForward>(symbols[i].Obj());
				}
			}
			break;
		default:
			throw UnexpectedSymbolCategoryException();
		}
		return isStatic;
	}

	extern TypeTsysList&						EvaluateVarSymbol(const ParsingArguments& invokerPa, ForwardVariableDeclaration* varDecl, ITsys* parentDeclType, bool& isVariadic);
	extern void									SetFuncTypeByReturnStat(const ParsingArguments& pa, FunctionDeclaration* funcDecl, TypeTsysList& returnTypes, TemplateArgumentContext* argumentsToApply);
	extern TypeTsysList&						EvaluateFuncSymbol(const ParsingArguments& invokerPa, ForwardFunctionDeclaration* funcDecl, ITsys* parentDeclType, TemplateArgumentContext* argumentsToApply);
	extern TypeTsysList&						EvaluateTypeAliasSymbol(const ParsingArguments& invokerPa, TypeAliasDeclaration* usingDecl, ITsys* parentDeclType, TemplateArgumentContext* argumentsToApply);
	extern TypeTsysList&						EvaluateValueAliasSymbol(const ParsingArguments& invokerPa, ValueAliasDeclaration* usingDecl, ITsys* parentDeclType, TemplateArgumentContext* argumentsToApply);
	extern ITsys*								EvaluateGenericArgumentSymbol(Symbol* symbol);

	extern TypeTsysList&						EvaluateForwardClassSymbol(const ParsingArguments& invokerPa, ForwardClassDeclaration* classDecl, ITsys* parentDeclType, TemplateArgumentContext* argumentsToApply);
	extern symbol_component::Evaluation&		EvaluateClassSymbol(const ParsingArguments& invokerPa, ClassDeclaration* classDecl, ITsys* parentDeclType, TemplateArgumentContext* argumentsToApply);
	extern void									ExtractClassType(ITsys* classType, ClassDeclaration*& classDecl, ITsys*& parentDeclType, TemplateArgumentContext*& argumentsToApply);
	extern TsysPSRecord*						EvaluateClassPSRecord(const ParsingArguments& invokerPa, ITsys* classType);
	extern void									EnsurePSRecordPrimaryEvaluated(const ParsingArguments& invokerPa, ITsys* classType, TsysPSRecord* psr);

	template<typename TCallback>
	void EnumerateClassPSInstances(const ParsingArguments& invokerPa, ITsys* classType, TCallback&& callback)
	{
		if (auto psr = EvaluateClassPSRecord(invokerPa, classType))
		{
			switch (psr->version)
			{
			case TsysPSRecord::PSInstanceVersionUnevaluated:
			case TsysPSRecord::PSInstanceVersionEvaluated:
			case TsysPSRecord::PSPrimaryThisVersion:
				callback(classType);
				break;
			default:
				for (vint i = 0; i < psr->evaluatedTypes.Count(); i++)
				{
					if (callback(psr->evaluatedTypes[i]))
					{
						return;
					}
				}
			}
		}
		else
		{
			callback(classType);
		}
	}

	template<typename TCallback>
	void EnumerateClassSymbolBaseTypes(const ParsingArguments& invokerPa, ClassDeclaration* classDecl, ITsys* parentDeclType, TemplateArgumentContext* argumentsToApply, TCallback&& callback)
	{
		auto& ev = EvaluateClassSymbol(invokerPa, classDecl, parentDeclType, argumentsToApply);
		auto classType = ev.Get()[0];
		if (classType->GetType() == TsysType::GenericFunction)
		{
			classType = classType->GetElement();
		}

		EnumerateClassPSInstances(invokerPa, classType, [&](ITsys* psEntity)
		{
			ClassDeclaration* cd = nullptr;
			ITsys* pdt = nullptr;
			TemplateArgumentContext* ata = nullptr;
			ExtractClassType(psEntity, cd, pdt, ata);

			auto& evPs = EvaluateClassSymbol(invokerPa, cd, pdt, ata);
			auto ctPs = evPs.Get()[0];
			if (ctPs->GetType() == TsysType::GenericFunction)
			{
				ctPs = ctPs->GetElement();
			}

			for (vint i = 0; i < evPs.ExtraCount(); i++)
			{
				auto& tsys = evPs.GetExtra(i);
				for (vint j = 0; j < tsys.Count(); j++)
				{
					if (callback(ctPs, tsys[j]))
					{
						return true;
					}
				}
			}
			return false;
		});
	}

	template<typename TCallback>
	void EnumerateClassPrimaryInstances(const ParsingArguments& invokerPa, ITsys* classType, bool allowNonDecl, TCallback&& callback)
	{
		switch (classType->GetType())
		{
		case TsysType::Decl:
		case TsysType::DeclInstant:
			if (auto psr = classType->GetPSRecord())
			{
				EnsurePSRecordPrimaryEvaluated(invokerPa, classType, psr);
				if (psr->version == TsysPSRecord::PSInstanceVersionEvaluated)
				{
					for (vint i = 0; i < psr->evaluatedTypes.Count(); i++)
					{
						if (callback(psr->evaluatedTypes[i]))
						{
							return;
						}
					}
					return;
				}
			}
			callback(classType);
			break;
		default:
			if (allowNonDecl)
			{
				callback(classType);
			}
			else
			{
				throw TypeCheckerException();
			}
		}
	}
}

#endif