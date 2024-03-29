#ifndef VCZH_DOCUMENT_CPPDOC_SYMBOL_TEMPLATESPEC
#define VCZH_DOCUMENT_CPPDOC_SYMBOL_TEMPLATESPEC

#include "Symbol.h"
#include "Ast_Decl.h"

namespace symbol_type_resolving
{
	extern Ptr<TemplateSpec>					GetTemplateSpecFromDecl(Ptr<Declaration> decl);
	extern Ptr<TemplateSpec>					GetTemplateSpecFromSymbol(Symbol* symbol);
	extern ITsys*								GetTemplateArgumentKey(const TemplateSpec::Argument& argument);
	extern ITsys*								GetTemplateArgumentKey(Symbol* argumentSymbol);
	extern Symbol*								TemplateArgumentPatternToSymbol(ITsys* tsys);
	extern void									CreateGenericFunctionHeader(const ParsingArguments& pa, Symbol* declSymbol, ITsys* parentDeclType, Ptr<TemplateSpec> spec, TypeTsysList& params, TsysGenericFunction& genericFunction);
	extern void									EnsureGenericTypeParameterAndArgumentMatched(ITsys* parameter, ITsys* argument);
}

#endif