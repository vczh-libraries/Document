#include "Parser.h"
#include "PSO.h"
#include "Ast_Decl.h"

extern bool						IsCStyleTypeReference(Ptr<CppTokenCursor>& cursor);
extern void						EnsureNoTemplateSpec(List<Ptr<TemplateSpec>>& specs, Ptr<CppTokenCursor>& cursor);
extern Ptr<TemplateSpec>		EnsureNoMultipleTemplateSpec(List<Ptr<TemplateSpec>>& specs, Ptr<CppTokenCursor>& cursor);

extern void						ParseDeclaration_Namespace(
									const ParsingArguments& pa,
									Ptr<CppTokenCursor>& cursor,
									List<Ptr<Declaration>>& output
								);

extern Ptr<EnumDeclaration>		ParseDeclaration_Enum_NotConsumeSemicolon(
									const ParsingArguments& pa,
									bool forTypeDef,
									Ptr<CppTokenCursor>& cursor,
									List<Ptr<Declaration>>& output
								);

extern Ptr<ClassDeclaration>	ParseDeclaration_Class_NotConsumeSemicolon(
									const ParsingArguments& pa,
									Ptr<Symbol> specSymbol,
									Ptr<TemplateSpec> spec,
									bool forTypeDef,
									Ptr<CppTokenCursor>& cursor,
									List<Ptr<Declaration>>& output
								);

extern void						ParseDeclaration_Using(
									const ParsingArguments& pa,
									Ptr<Symbol> specSymbol,
									Ptr<TemplateSpec> spec,
									Ptr<CppTokenCursor>& cursor,
									List<Ptr<Declaration>>& output
								);

extern void						ParseDeclaration_Typedef(
									const ParsingArguments& pa,
									Ptr<CppTokenCursor>& cursor,
									List<Ptr<Declaration>>& output
								);

#define FUNCVAR_DECORATORS(F)\
	F(CONSTEXPR, Constexpr)\
	F(DECL_EXTERN, Extern)\
	F(STATIC, Static)\
	F(MUTABLE, Mutable)\
	F(THREAD_LOCAL, ThreadLocal)\
	F(REGISTER, Register)\
	F(VIRTUAL, Virtual)\
	F(EXPLICIT, Explicit)\
	F(INLINE, Inline)\
	F(__INLINE, __Inline)\
	F(__FORCEINLINE, __ForceInline)\

#define FUNCVAR_DECORATORS_FOR_FUNCTION(F)\
	F(Constexpr)\
	F(Extern)\
	F(Friend)\
	F(Static)\
	F(Virtual)\
	F(Explicit)\
	F(Inline)\
	F(__Inline)\
	F(__ForceInline)\
	F(Abstract)\
	F(Default)\
	F(Delete)\

#define FUNCVAR_DECORATORS_FOR_VARIABLE(F)\
	F(Constexpr)\
	F(Extern)\
	F(Static)\
	F(Mutable)\
	F(ThreadLocal)\
	F(Register)\
	F(Inline)\
	F(__Inline)\
	F(__ForceInline)\

#define FUNCVAR_PARAMETER(NAME) bool decorator##NAME,
#define FUNCVAR_ARGUMENT(NAME) decorator##NAME,
#define FUNCVAR_FILL_DECLARATOR(NAME) decl->decorator##NAME = decorator##NAME;

extern Ptr<TemplateSpec>		AssignContainerClassDeclsToSpecs(
									List<Ptr<TemplateSpec>>& specs,
									Ptr<Declarator> declarator,
									List<Ptr<TemplateSpec>>& containerClassSpecs,
									List<ClassDeclaration*>& containerClassDecls,
									Ptr<CppTokenCursor>& cursor
								);

extern void						ParseDeclaration_Function(
									const ParsingArguments& pa,
									Ptr<Symbol> specSymbol,
									List<Ptr<TemplateSpec>>& specs,
									Ptr<Declarator> declarator,
									Ptr<FunctionType> funcType,
									FUNCVAR_DECORATORS_FOR_FUNCTION(FUNCVAR_PARAMETER)
									CppMethodType methodType,
									Ptr<CppTokenCursor>& cursor,
									List<Ptr<Declaration>>& output
								);

extern void						ParseDeclaration_Variable(
									const ParsingArguments& pa,
									Ptr<Symbol> specSymbol,
									List<Ptr<TemplateSpec>>& specs,
									Ptr<Declarator> declarator,
									FUNCVAR_DECORATORS_FOR_VARIABLE(FUNCVAR_PARAMETER)
									Ptr<CppTokenCursor>& cursor,
									List<Ptr<Declaration>>& output
								);

extern void						ParseDeclaration_FuncVar(
									const ParsingArguments& pa,
									Ptr<Symbol> specSymbol,
									List<Ptr<TemplateSpec>>& specs,
									bool decoratorFriend,
									Ptr<CppTokenCursor>& cursor,
									List<Ptr<Declaration>>& output
								);