#ifndef VCZH_DOCUMENT_CPPDOC_LEXERTOKENDEF
#define VCZH_DOCUMENT_CPPDOC_LEXERTOKENDEF

/***********************************************************************
Keywords
***********************************************************************/

#define CPP_KEYWORD_TOKENS(F)							\
	F(ALIGNAS,					alignas)				\
	F(__PTR32,					__ptr32)				\
	F(__PTR64,					__ptr64)				\
	F(OPERATOR,					operator)				\
	F(NEW,						new)					\
	F(DELETE,					delete)					\
	F(CONSTEXPR,				constexpr)				\
	F(CONST,					const)					\
	F(VOLATILE,					volatile)				\
	F(OVERRIDE,					override)				\
	F(NOEXCEPT,					noexcept)				\
	F(THROW,					throw)					\
	F(DECLTYPE,					decltype)				\
	F(__CDECL,					__cdecl)				\
	F(__CLRCALL,				__clrcall)				\
	F(__STDCALL,				__stdcall)				\
	F(__FASTCALL,				__fastcall)				\
	F(__THISCALL,				__thiscall)				\
	F(__VECTORCALL,				__vectorcall)			\
	F(TYPE_AUTO,				auto)					\
	F(TYPE_VOID,				void)					\
	F(TYPE_BOOL,				bool)					\
	F(TYPE_CHAR,				char)					\
	F(TYPE_WCHAR_T,				wchar_t)				\
	F(TYPE_CHAR16_T,			char16_t)				\
	F(TYPE_CHAR32_T,			char32_t)				\
	F(TYPE_SHORT,				short)					\
	F(TYPE_INT,					int)					\
	F(TYPE___INT8,				__int8)					\
	F(TYPE___INT16,				__int16)				\
	F(TYPE___INT32,				__int32)				\
	F(TYPE___INT64,				__int64)				\
	F(TYPE_LONG,				long)					\
	F(TYPE_FLOAT,				float)					\
	F(TYPE_DOUBLE,				double)					\
	F(SIGNED,					signed)					\
	F(UNSIGNED,					unsigned)				\
	F(STATIC,					static)					\
	F(VIRTUAL,					virtual)				\
	F(EXPLICIT,					explicit)				\
	F(INLINE,					inline)					\
	F(__INLINE,					__inline)				\
	F(__FORCEINLINE,			__forceinline)			\
	F(REGISTER,					register)				\
	F(MUTABLE,					mutable)				\
	F(THREAD_LOCAL,				thread_local)			\
	F(DECL_CLASS,				class)					\
	F(DECL_STRUCT,				struct)					\
	F(DECL_ENUM,				enum)					\
	F(DECL_UNION,				union)					\
	F(DECL_NAMESPACE,			namespace)				\
	F(DECL_TYPEDEF,				typedef)				\
	F(DECL_USING,				using)					\
	F(DECL_FRIEND,				friend)					\
	F(DECL_EXTERN,				extern)					\
	F(DECL_TEMPLATE,			template)				\
	F(STAT_RETURN,				return)					\
	F(STAT_BREAK,				break)					\
	F(STAT_CONTINUE,			continue)				\
	F(STAT_GOTO,				goto)					\
	F(STAT_IF,					if)						\
	F(STAT_ELSE,				else)					\
	F(STAT_WHILE,				while)					\
	F(STAT_DO,					do)						\
	F(STAT_FOR,					for)					\
	F(STAT_SWITCH,				switch)					\
	F(STAT_CASE,				case)					\
	F(STAT_DEFAULT,				default)				\
	F(STAT_TRY,					try)					\
	F(STAT_CATCH,				catch)					\
	F(STAT___TRY,				__try)					\
	F(STAT___EXCEPT,			__except)				\
	F(STAT___FINALLY,			__finally)				\
	F(STAT___LEAVE,				__leave)				\
	F(STAT___IF_EXISTS,			__if_exists)			\
	F(STAT___IF_NOT_EXISTS,		__if_not_exists)		\
	F(EXPR_TRUE,				true)					\
	F(EXPR_FALSE,				false)					\
	F(EXPR_THIS,				this)					\
	F(EXPR_NULLPTR,				nullptr)				\
	F(EXPR_TYPEID,				typeid)					\
	F(EXPR_SIZEOF,				sizeof)					\
	F(EXPR_DYNAMIC_CAST,		dynamic_cast)			\
	F(EXPR_STATIC_CAST,			static_cast)			\
	F(EXPR_CONST_CAST,			const_cast)				\
	F(EXPR_REINTERPRET_CAST,	reinterpret_cast)		\
	F(EXPR_SAFE_CAST,			safe_cast)				\
	F(TYPENAME,					typename)				\
	F(PUBLIC,					public)					\
	F(PROTECTED,				protected)				\
	F(PRIVATE,					private)				\
	F(__PRAGMA,					__pragma)				\
	F(__DECLSPEC,				__declspec)				\

/***********************************************************************
Others
***********************************************************************/

#define CPP_REGEX_TOKENS(F)																														\
	F(LBRACE,					L"/{")																											\
	F(RBRACE,					L"/}")																											\
	F(LBRACKET,					L"/[")																											\
	F(RBRACKET,					L"/]")																											\
	F(LPARENTHESIS,				L"/(")																											\
	F(RPARENTHESIS,				L"/)")																											\
	F(LT,						L"<")																											\
	F(GT,						L">")																											\
	F(EQ,						L"=")																											\
	F(NOT,						L"!")																											\
	F(PERCENT,					L"%")																											\
	F(COLON,					L":")																											\
	F(SEMICOLON,				L";")																											\
	F(DOT,						L".")																											\
	F(QUESTIONMARK,				L"/?")																											\
	F(COMMA,					L",")																											\
	F(MUL,						L"/*")																											\
	F(ADD,						L"/+")																											\
	F(SUB,						L"-")																											\
	F(DIV,						L"//")																											\
	F(XOR,						L"/^")																											\
	F(AND,						L"&")																											\
	F(OR,						L"/|")																											\
	F(REVERT,					L"~")																											\
	F(SHARP,					L"#")																											\
	F(INT,						L"(/d+('/d+)*)("		L"[uU]|[lL]|[uU][lL]|[lL][uU]|[lL][lL]|[uU][lL][lL]|[lL][uU][lL]|[lL][lL][uU]"	L")?")	\
	F(HEX,						L"0[xX][0-9a-fA-F]+("	L"[uU]|[lL]|[uU][lL]|[lL][uU]|[lL][lL]|[uU][lL][lL]|[lL][uU][lL]|[lL][lL][uU]"	L")?")	\
	F(BIN,						L"0[bB][01]+("			L"[uU]|[lL]|[uU][lL]|[lL][uU]|[lL][lL]|[uU][lL][lL]|[lL][uU][lL]|[lL][lL][uU]"	L")?")	\
	F(FLOAT,					L"(/d+.|./d+|/d+./d+)([eE][+/-]?/d+)?[fFlL]?")																	\
	F(STRING,					L"([uUL]|u8)?\"([^/\\\"]|/\\/.)*\"")																			\
	F(CHAR,						L"([uUL]|u8)?'([^/\\']|/\\/.)*'")																				\
	F(ID,						L"[a-zA-Z_][a-zA-Z0-9_]*")																						\
	F(SPACE,					L"[ \t\r\n\v\f]+")																								\
	F(DOCUMENT,					L"//////[^\r\n]*")																								\
	F(COMMENT1,					L"////[^\r\n]*")																								\
	F(COMMENT2,					L"///*([^*]|/*+[^*//])*/*+//")																					\

/***********************************************************************
Token List
***********************************************************************/

#define CPP_ALL_TOKENS(FKEYWORD, FREGEX)\
	CPP_KEYWORD_TOKENS(FKEYWORD)\
	CPP_REGEX_TOKENS(FREGEX)\

#endif