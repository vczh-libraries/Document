#ifndef VCZH_DOCUMENT_CPPDOC_SYMBOL_TEMPLATESPEC
#define VCZH_DOCUMENT_CPPDOC_SYMBOL_TEMPLATESPEC

#include "Ast_Decl.h"
#include "Parser.h"

namespace symbol_type_resolving
{
	extern Ptr<TemplateSpec>					GetTemplateSpecFromSymbol(Symbol* symbol);
	extern ITsys*								GetTemplateArgumentKey(const TemplateSpec::Argument& argument, ITsysAlloc* tsys);
	extern ITsys*								GetTemplateArgumentKey(Symbol* argumentSymbol, ITsysAlloc* tsys);
	extern void									CreateGenericFunctionHeader(const ParsingArguments& pa, Symbol* declSymbol, ITsys* parentDeclType, Ptr<TemplateSpec> spec, TypeTsysList& params, TsysGenericFunction& genericFunction);
	extern void									EnsureGenericTypeParameterAndArgumentMatched(ITsys* parameter, ITsys* argument);
}

#endif