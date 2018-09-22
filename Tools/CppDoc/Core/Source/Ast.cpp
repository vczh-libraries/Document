#include "Ast.h"
#include "Ast_Decl.h"
#include "Ast_Type.h"
#include "Ast_Expr.h"
#include "Ast_Stat.h"

#define CPPDOC_ACCEPT(NAME) void NAME::Accept(ITypeVisitor* visitor) { visitor->Visit(this); }
CPPDOC_TYPE_LIST(CPPDOC_ACCEPT)
#undef CPPDOC_ACCEPT