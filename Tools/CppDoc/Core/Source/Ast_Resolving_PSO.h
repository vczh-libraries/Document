#ifndef VCZH_DOCUMENT_CPPDOC_AST_RESOLVING_PSO
#define VCZH_DOCUMENT_CPPDOC_AST_RESOLVING_PSO

#include "Ast_Resolving_IFT.h"

namespace partial_specification_ordering
{
	extern void						AssignPSPrimary(
										const ParsingArguments& pa,
										Ptr<CppTokenCursor>& cursor,
										Symbol* symbol
									);

	extern bool						IsPSAncestor(
										const ParsingArguments& pa,
										Symbol* symbolA,
										Ptr<TemplateSpec> tA,
										Ptr<SpecializationSpec> psA,
										Symbol* symbolB,
										Ptr<TemplateSpec> tB,
										Ptr<SpecializationSpec> psB
									);
}

#endif