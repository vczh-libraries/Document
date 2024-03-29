#ifndef VCZH_DOCUMENT_CPPDOC_AST_EVALUATESYMBOL_SHARED
#define VCZH_DOCUMENT_CPPDOC_AST_EVALUATESYMBOL_SHARED

#include "EvaluateSymbol.h"

namespace symbol_type_resolving
{
	struct Eval
	{
	public:
		bool								notEvaluated;
		Symbol*								symbol;
		ParsingArguments					declPa;
		symbol_component::Evaluation&		ev;
		TypeTsysList&						evaluatedTypes;

		vint								psVersion = 0;
		List<ResolvedItem>					psResolving;

		Eval(
			bool							_notEvaluated,
			Symbol*							_symbol,
			ParsingArguments				_declPa,
			symbol_component::Evaluation&	_ev
		)
			: notEvaluated(_notEvaluated)
			, symbol(_symbol)
			, declPa(_declPa)
			, ev(_ev)
			, evaluatedTypes(_ev.Get())
		{
		}

		operator bool()
		{
			return notEvaluated;
		}
	};

	extern symbol_component::Evaluation&		GetCorrectEvaluation(const ParsingArguments& pa, Declaration* decl, Ptr<TemplateSpec> spec, TemplateArgumentContext* argumentsToApply);
	extern ParsingArguments						GetPaFromInvokerPa(const ParsingArguments& pa, Symbol* declSymbol, TemplateArgumentContext* parentTaContext, TemplateArgumentContext* argumentsToApply);
	extern TypeTsysList&						FinishEvaluatingPotentialGenericSymbol(const ParsingArguments& declPa, Declaration* decl, Ptr<TemplateSpec> spec, TemplateArgumentContext* argumentsToApply);
	extern Eval									ProcessArguments(const ParsingArguments& invokerPa, Declaration* decl, Ptr<TemplateSpec> spec, ITsys*& parentDeclType, TemplateArgumentContext* argumentsToApply, bool allowEvaluating = false);
}

#endif