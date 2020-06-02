#include "EvaluateSymbol_Shared.h"
#include "Symbol_TemplateSpec.h"
#include "Symbol_Resolve.h"
#include "IFT.h"

namespace symbol_type_resolving
{
	/***********************************************************************
	EvaluateForwardClassSymbol: Evaluate the declared type for a class
	***********************************************************************/

	TypeTsysList& EvaluateForwardClassSymbol(
		const ParsingArguments& invokerPa,
		ForwardClassDeclaration* classDecl,
		ITsys* parentDeclType,
		TemplateArgumentContext* argumentsToApply
	)
	{
		auto eval = ProcessArguments(invokerPa, classDecl, classDecl->templateSpec, parentDeclType, argumentsToApply);
		if (eval)
		{
			for (vint i = 0; i < eval.evaluatedTypes.Count(); i++)
			{
				auto tsys = eval.evaluatedTypes[i];
				if (tsys->GetType() == TsysType::GenericFunction)
				{
					auto expect = GetTemplateArgumentKey(classDecl->templateSpec->arguments[0]);
					auto actual = tsys->GetParam(0);
					if (expect != actual)
					{
						eval.notEvaluated = true;
						eval.evaluatedTypes.Clear();
						eval.ev.progress = symbol_component::EvaluationProgress::Evaluating;
					}
				}
			}
		}

		if (eval)
		{
			if (classDecl->templateSpec && classDecl->templateSpec->arguments.Count() > 0)
			{
				Array<ITsys*> params(classDecl->templateSpec->arguments.Count());
				for (vint i = 0; i < classDecl->templateSpec->arguments.Count(); i++)
				{
					auto argumentSymbol = classDecl->templateSpec->arguments[i].argumentSymbol;
					if (argumentsToApply)
					{
						auto pattern = GetTemplateArgumentKey(argumentSymbol);
						params[i] = ReplaceGenericArg(eval.declPa, pattern);
						if (params[i] != pattern)
						{
							continue;
						}
					}

					if(argumentSymbol->ellipsis)
					{
						params[i] = eval.declPa.tsys->Any();
					}
					else if(argumentSymbol->kind==symbol_component::SymbolKind::GenericTypeArgument)
					{
						params[i] = EvaluateGenericArgumentType(argumentSymbol);
					}
					else
					{
						params[i] = nullptr;
					}
				}
				auto diTsys = eval.declPa.tsys->DeclInstantOf(eval.symbol, &params, eval.declPa.parentDeclType);
				eval.evaluatedTypes.Add(diTsys);
			}
			else
			{
				if (eval.declPa.parentDeclType)
				{
					eval.evaluatedTypes.Add(eval.declPa.tsys->DeclInstantOf(eval.symbol, nullptr, eval.declPa.parentDeclType));
				}
				else
				{
					eval.evaluatedTypes.Add(eval.declPa.tsys->DeclOf(eval.symbol));
				}
			}

			return FinishEvaluatingPotentialGenericSymbol(eval.declPa, classDecl, classDecl->templateSpec, argumentsToApply);
		}
		else
		{
			return eval.evaluatedTypes;
		}
	}

	/***********************************************************************
	EvaluateClassSymbol: Evaluate the declared type and base types for a class
	***********************************************************************/

	symbol_component::Evaluation& EvaluateClassSymbol(
		const ParsingArguments& invokerPa,
		ClassDeclaration* classDecl,
		ITsys* parentDeclType,
		TemplateArgumentContext* argumentsToApply
	)
	{
		EvaluateForwardClassSymbol(invokerPa, classDecl, parentDeclType, argumentsToApply);
		auto eval = ProcessArguments(invokerPa, classDecl, classDecl->templateSpec, parentDeclType, argumentsToApply);

		if (!eval.ev.skipEvaluatingBaseTypes)
		{
			if (eval.ev.progress == symbol_component::EvaluationProgress::Evaluated)
			{
				if (eval.ev.ExtraCount() == classDecl->baseTypes.Count())
				{
					return eval.ev;
				}
			}

			eval.ev.skipEvaluatingBaseTypes = true;

			Array<Ptr<TypeTsysList>> baseTsys(classDecl->baseTypes.Count());
			for (vint i = 0; i < classDecl->baseTypes.Count(); i++)
			{
				auto tsysList = MakePtr<TypeTsysList>();
				baseTsys[i] = tsysList;
				bool isVta = false;
				TypeToTsysInternal(eval.declPa, classDecl->baseTypes[i].item.f1, *tsysList.Obj(), isVta);
				if (isVta != classDecl->baseTypes[i].isVariadic)
				{
					throw TypeCheckerException();
				}
			}

			eval.ev.progress = symbol_component::EvaluationProgress::Evaluating;
			eval.ev.AllocateExtra(classDecl->baseTypes.Count());
			for (vint i = 0; i < classDecl->baseTypes.Count(); i++)
			{
				eval.ev.ReplaceExtra(i, baseTsys[i]);
			}
			eval.ev.progress = symbol_component::EvaluationProgress::Evaluated;

			eval.ev.skipEvaluatingBaseTypes = false;
		}
		return eval.ev;
	}

	/***********************************************************************
	ExtractClassType: Convert ITsys* to arguments for EvaluateClassSymbol
	***********************************************************************/

	template<typename TClassDecl>
	void ExtractClassTypeInternal(
		ITsys* classType,
		TClassDecl*& classDecl,
		ITsys*& parentDeclType,
		TemplateArgumentContext*& argumentsToApply
	)
	{
		switch (classType->GetType())
		{
		case TsysType::Decl:
		case TsysType::DeclInstant:
			{
				auto symbol = classType->GetDecl();
				classDecl = symbol->GetAnyForwardDecl<TClassDecl>().Obj();
				if (!classDecl) throw TypeCheckerException();

				if (classType->GetType() == TsysType::Decl)
				{
					parentDeclType = nullptr;
					argumentsToApply = nullptr;
				}
				else
				{
					const auto& di = classType->GetDeclInstant();
					parentDeclType = di.parentDeclType;
					argumentsToApply = di.taContext.Obj();
				}
				return;
			}
		default:
			throw TypeCheckerException();
		}
	}

	void ExtractClassType(
		ITsys* classType,
		ClassDeclaration*& classDecl,
		ITsys*& parentDeclType,
		TemplateArgumentContext*& argumentsToApply
	)
	{
		ExtractClassTypeInternal(classType, classDecl, parentDeclType, argumentsToApply);
	}

	/***********************************************************************
	GetPaInsideClass: Get a ParsingArguments for evaluating class members
	***********************************************************************/

	ITsys* AdjustParentDeclType(ITsys* classType)
	{
		switch (classType->GetType())
		{
		case TsysType::DeclInstant:
			{
				auto& di = classType->GetDeclInstant();
				if (di.taContext)
				{
					return classType;
				}
				else
				{
					return di.parentDeclType;
				}
			}
		default:
			return nullptr;
		}
	}

	ParsingArguments GetPaInsideClass(const ParsingArguments& invokerPa, ITsys* classType)
	{
		switch (classType->GetType())
		{
		case TsysType::Decl:
			return invokerPa.AdjustForDecl(classType->GetDecl());
		case TsysType::DeclInstant:
			{
				auto& di = classType->GetDeclInstant();
				if (di.taContext)
				{
					auto pa = invokerPa.AdjustForDecl(classType->GetDecl(), classType);
					pa.taContext = di.taContext.Obj();
					return pa;
				}
				else
				{
					auto pa = invokerPa.AdjustForDecl(classType->GetDecl(), di.parentDeclType);
					pa.taContext = di.parentDeclType->GetDeclInstant().taContext.Obj();
					return pa;
				}
			}
		default:
			throw TypeCheckerException();
		}
	}

	/***********************************************************************
	EvaluateClassPSRecord: Get an up-to-date TsysPSRecord from an ITsys*
	***********************************************************************/

	TsysPSRecord* EvaluateClassPSRecord(const ParsingArguments& invokerPa, ITsys* classType)
	{
		auto psr = classType->GetPSRecord();
		if (!psr) return nullptr;

		switch (psr->version)
		{
		case TsysPSRecord::PSInstanceVersionUnevaluated:
		case TsysPSRecord::PSInstanceVersionEvaluated:
		case TsysPSRecord::PSPrimaryThisVersion:
			break;
		default:
			{
				ForwardClassDeclaration* cd = nullptr;
				ITsys* pdt = nullptr;
				TemplateArgumentContext* ata = nullptr;
				ExtractClassTypeInternal(classType, cd, pdt, ata);

				vint version = cd->symbol->GetPSPrimaryVersion_NF();
				if (psr->version == version) return psr;
				psr->version = version;
				psr->evaluatedTypes.Clear();

				Dictionary<Symbol*, Ptr<TemplateArgumentContext>> psResult;
				infer_function_type::InferPartialSpecializationPrimary<ForwardClassDeclaration>(invokerPa, psResult, cd->symbol, pdt, ata);

				if (psResult.Count() == 0 || infer_function_type::IsValuableTaContextWithMatchedPSChildren(ata))
				{
					psr->evaluatedTypes.Add(classType);
				}

				for (vint i = 0; i < psResult.Count(); i++)
				{
					auto psSymbol = psResult.Keys()[i];
					auto psCd = psSymbol->GetAnyForwardDecl<ForwardClassDeclaration>().Obj();
					auto psAta = psResult.Values()[i].Obj();
					auto& ev = EvaluateForwardClassSymbol(invokerPa, psCd, pdt, psAta);
					if (!psr->evaluatedTypes.Contains(ev.Get(0)))
					{
						psr->evaluatedTypes.Add(ev.Get(0));
					}
				}
			}
		}

		return psr;
	}

	/***********************************************************************
	EnsurePSRecordPrimaryEvaluated: Ensure PSInstanceVersionEvaluated if it is PSInstanceVersionUnevaluated
	***********************************************************************/

	void EnsurePSRecordPrimaryEvaluated(const ParsingArguments& invokerPa, ITsys* classType, TsysPSRecord* psr)
	{
		if (psr->version == TsysPSRecord::PSInstanceVersionUnevaluated)
		{
			psr->version = TsysPSRecord::PSInstanceVersionEvaluated;

			auto cd = classType->GetDecl()->GetAnyForwardDecl<ForwardClassDeclaration>().Obj();
			auto psPa = GetPaInsideClass(invokerPa, classType);

			auto genericType = MakePtr<GenericType>();
			{
				CopyFrom(genericType->arguments, cd->specializationSpec->arguments);

				auto idType = MakePtr<IdType>();
				idType->cStyleTypeReference = true;
				idType->name = cd->name;
				idType->resolving = ResolveSymbolInContext(psPa, idType->name, idType->cStyleTypeReference).types;
				genericType->type = idType;
			}

			TypeToTsysNoVta(psPa, genericType, psr->evaluatedTypes);
		}
	}
}