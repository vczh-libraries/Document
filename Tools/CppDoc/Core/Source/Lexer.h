#ifndef VCZH_DOCUMENT_CPPDOC_LEXER
#define VCZH_DOCUMENT_CPPDOC_LEXER

#include "Vlpp.h"

using namespace vl;
using namespace vl::collections;
using namespace vl::regex;
using namespace vl::stream;
using namespace vl::filesystem;

/***********************************************************************
Definition
***********************************************************************/

#define CPP_KEYWORD_TOKENS(F)\
	F(ALIGNAS,					alignas)\
	F(__PTR32,					__ptr32)\
	F(__PTR64,					__ptr64)\
	F(OPERATOR,					operator)\
	F(NEW,						new)\
	F(DELETE,					delete)\
	F(CONSTEXPR,				constexpr)\
	F(CONST,					const)\
	F(VOLATILE,					volatile)\
	F(OVERRIDE,					override)\
	F(NOEXCEPT,					noexcept)\
	F(THROW,					throw)\
	F(DECLTYPE,					decltype)\
	F(__CDECL,					__cdecl)\
	F(__CLRCALL,				__clrcall)\
	F(__STDCALL,				__stdcall)\
	F(__FASTCALL,				__fastcall)\
	F(__THISCALL,				__thiscall)\
	F(__VECTORCALL,				__vectorcall)\
	F(TYPE_AUTO,				auto)\
	F(TYPE_VOID,				void)\
	F(TYPE_BOOL,				bool)\
	F(TYPE_CHAR,				char)\
	F(TYPE_WCHAR_T,				wchar_t)\
	F(TYPE_CHAR16_T,			char16_t)\
	F(TYPE_CHAR32_T,			char32_t)\
	F(TYPE_SHORT,				short)\
	F(TYPE_INT,					int)\
	F(TYPE___INT8,				__int8)\
	F(TYPE___INT16,				__int16)\
	F(TYPE___INT32,				__int32)\
	F(TYPE___INT64,				__int64)\
	F(TYPE_LONG,				long)\
	F(TYPE_FLOAT,				float)\
	F(TYPE_DOUBLE,				double)\
	F(SIGNED,					signed)\
	F(UNSIGNED,					unsigned)\
	F(STATIC,					static)\
	F(VIRTUAL,					virtual)\
	F(EXPLICIT,					explicit)\
	F(INLINE,					inline)\
	F(__FORCEINLINE,			__forceinline)\
	F(REGISTER,					register)\
	F(MUTABLE,					mutable)\
	F(THREAD_LOCAL,				thread_local)\
	F(DECL_CLASS,				class)\
	F(DECL_STRUCT,				struct)\
	F(DECL_ENUM,				enum)\
	F(DECL_UNION,				union)\
	F(DECL_NAMESPACE,			namespace)\
	F(DECL_TYPEDEF,				typedef)\
	F(DECL_USING,				using)\
	F(DECL_FRIEND,				friend)\
	F(DECL_EXTERN,				extern)\
	F(DECL_TEMPLATE,			template)\
	F(STAT_RETURN,				return)\
	F(STAT_BREAK,				break)\
	F(STAT_CONTINUE,			continue)\
	F(STAT_GOTO,				goto)\
	F(STAT_IF,					if)\
	F(STAT_ELSE,				else)\
	F(STAT_WHILE,				while)\
	F(STAT_DO,					do)\
	F(STAT_FOR,					for)\
	F(STAT_SWITCH,				switch)\
	F(STAT_CASE,				case)\
	F(STAT_DEFAULT,				default)\
	F(STAT_TRY,					try)\
	F(STAT_CATCH,				catch)\
	F(STAT___TRY,				__try)\
	F(STAT___EXCEPT,			__except)\
	F(STAT___FINALLY,			__finally)\
	F(STAT___LEAVE,				__leave)\
	F(STAT___IF_EXISTS,			__if_exists)\
	F(STAT___IF_NOT_EXISTS,		__if_not_exists)\
	F(THIS,						this)\
	F(NULLPTR,					nullptr)\
	F(TYPEID,					typeid)\
	F(SIZEOF,					sizeof)\
	F(DYNAMIC_CAST,				dynamic_cast)\
	F(STATIC_CAST,				static_cast)\
	F(CONST_CAST,				const_cast)\
	F(REINTERPRET_CAST,			reinterpret_cast)\
	F(SAFE_CAST,				safe_cast)\
	F(TYPENAME,					typename)\
	F(PUBLIC,					public)\
	F(PROTECTED,				protected)\
	F(PRIVATE,					private)\

enum class CppTokens
{
	LBRACE, RBRACE,
	LBRACKET, RBRACKET,
	LPARENTHESIS, RPARENTHESIS,
	LT, GT, EQ, NOT,
	PERCENT, COLON, SEMICOLON, DOT, QUESTIONMARK, COMMA,
	MUL, ADD, SUB, DIV, XOR, AND, OR, REVERT, SHARP,
	INT, HEX, BIN, FLOAT,
	STRING, CHAR,
#define DEFINE_KEYWORD_TOKEN(NAME, KEYWORD) NAME,
	CPP_KEYWORD_TOKENS(DEFINE_KEYWORD_TOKEN)
#undef DEFINE_KEYWORD_TOKEN
	ID,
	SPACE, DOCUMENT, COMMENT1, COMMENT2,
};

extern Ptr<RegexLexer> CreateCppLexer();

/***********************************************************************
Reader
***********************************************************************/

class CppTokenCursor;
class CppTokenReader;

class CppTokenCursor
{
	friend class CppTokenReader;
private:
	CppTokenReader*				reader;
	Ptr<CppTokenCursor>			next;

	CppTokenCursor(CppTokenReader* _reader, RegexToken _token);
public:
	RegexToken					token;

	Ptr<CppTokenCursor>			Next();
};

class CppTokenReader : public Object
{
	friend class CppTokenCursor;
protected:
	Ptr<RegexLexer>				lexer;
	Ptr<RegexTokens>			tokens;
	IEnumerator<RegexToken>*	tokenEnumerator;

	Ptr<CppTokenCursor>			firstToken;

	Ptr<CppTokenCursor>			CreateNextToken();
public:
	CppTokenReader(Ptr<RegexLexer> _lexer, const WString& input);
	~CppTokenReader();

	Ptr<CppTokenCursor>			GetFirstToken();
};

#endif