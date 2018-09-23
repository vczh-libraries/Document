#include "Lexer.h"

/***********************************************************************
CreateCppLexer
***********************************************************************/

Ptr<RegexLexer> CreateCppLexer()
{
	List<WString> tokens;
#define DEFINE_TOKEN(NAME, REGEX) \
	do if ((vint)CppTokens::NAME != tokens.Add(REGEX)) { throw 0; } while(false)

	DEFINE_TOKEN(LBRACE, L"/{");
	DEFINE_TOKEN(RBRACE, L"/}");
	DEFINE_TOKEN(LBRACKET, L"/[");
	DEFINE_TOKEN(RBRACKET, L"/]");
	DEFINE_TOKEN(LPARENTHESIS, L"/(");
	DEFINE_TOKEN(RPARENTHESIS, L"/)");
	DEFINE_TOKEN(LT, L"<");
	DEFINE_TOKEN(GT, L">");
	DEFINE_TOKEN(EQ, L"=");
	DEFINE_TOKEN(NOT, L"!");
	DEFINE_TOKEN(PERCENT, L"%");
	DEFINE_TOKEN(COLON, L":");
	DEFINE_TOKEN(SEMICOLON, L";");
	DEFINE_TOKEN(DOT, L".");
	DEFINE_TOKEN(QUESTIONMARK, L"/?");
	DEFINE_TOKEN(COMMA, L",");
	DEFINE_TOKEN(MUL, L"/*");
	DEFINE_TOKEN(ADD, L"/+");
	DEFINE_TOKEN(SUB, L"-");
	DEFINE_TOKEN(DIV, L"//");
	DEFINE_TOKEN(XOR, L"/^");
	DEFINE_TOKEN(AND, L"&");
	DEFINE_TOKEN(OR, L"/|");
	DEFINE_TOKEN(REVERT, L"~");
	DEFINE_TOKEN(SHARP, L"#");

	DEFINE_TOKEN(INT, L"(/d+('/d+)*)[uU]?[lL]?");
	DEFINE_TOKEN(HEX, L"0[xX][0-9a-fA-F]+[uU]?[lL]?");
	DEFINE_TOKEN(BIN, L"0[bB][01]+[uU]?[lL]?");
	DEFINE_TOKEN(FLOAT, L"(/d+.|./d+|/d+./d+)([eE][+/-]?/d+)?[fFlL]?");
	DEFINE_TOKEN(STRING, L"([uUL]|u8)?\"([^/\\\"]|/\\/.)*\"");
	DEFINE_TOKEN(CHAR, L"([uUL]|u8)?'([^/\\']|/\\/.)*'");

	DEFINE_TOKEN(OPERATOR, L"operator");
	DEFINE_TOKEN(NEW, L"new");
	DEFINE_TOKEN(DELETE, L"delete");
	DEFINE_TOKEN(CONSTEXPR, L"constexpr");
	DEFINE_TOKEN(CONST, L"const");
	DEFINE_TOKEN(VOLATILE, L"volatile");
	DEFINE_TOKEN(OVERRIDE, L"override");
	DEFINE_TOKEN(NOEXCEPT, L"noexcept");
	DEFINE_TOKEN(THROW, L"throw");
	DEFINE_TOKEN(DECLTYPE, L"decltype");
	DEFINE_TOKEN(__CDECL, L"__cdecl");
	DEFINE_TOKEN(__CLRCALL, L"__clrcall");
	DEFINE_TOKEN(__STDCALL, L"__stdcall");
	DEFINE_TOKEN(__FASTCALL, L"__fastcall");
	DEFINE_TOKEN(__THISCALL, L"__thiscall");
	DEFINE_TOKEN(__VECTORCALL, L"__vectorcall");
	DEFINE_TOKEN(TYPE_AUTO, L"auto");
	DEFINE_TOKEN(TYPE_VOID, L"void");
	DEFINE_TOKEN(TYPE_BOOL, L"bool");
	DEFINE_TOKEN(TYPE_CHAR, L"char");
	DEFINE_TOKEN(TYPE_WCHAR_T, L"wchar_t");
	DEFINE_TOKEN(TYPE_CHAR16_T, L"char16_t");
	DEFINE_TOKEN(TYPE_CHAR32_T, L"char32_t");
	DEFINE_TOKEN(TYPE_SHORT, L"short");
	DEFINE_TOKEN(TYPE_INT, L"int");
	DEFINE_TOKEN(TYPE___INT8, L"__int8");
	DEFINE_TOKEN(TYPE___INT16, L"__int16");
	DEFINE_TOKEN(TYPE___INT32, L"__int32");
	DEFINE_TOKEN(TYPE___INT64, L"__int64");
	DEFINE_TOKEN(TYPE_LONG, L"long");
	DEFINE_TOKEN(TYPE_FLOAT, L"float");
	DEFINE_TOKEN(TYPE_DOUBLE, L"double");
	DEFINE_TOKEN(SIGNED, L"signed");
	DEFINE_TOKEN(UNSIGNED, L"unsigned");
	DEFINE_TOKEN(ID, L"[a-zA-Z_][a-zA-Z0-9_]*");

	DEFINE_TOKEN(SPACE, L"[ \t\r\n\v\f]+");
	DEFINE_TOKEN(DOCUMENT, L"//////[^\r\n]*");
	DEFINE_TOKEN(COMMENT1, L"////[^\r\n]*");
	DEFINE_TOKEN(COMMENT2, L"///*([^*]|/*+[^*//])*/*+//");

#undef DEFINE_TOKEN

	RegexProc proc;
	return new RegexLexer(tokens, proc);
}

/***********************************************************************
CppTokenCursor
***********************************************************************/

CppTokenCursor::CppTokenCursor(CppTokenReader* _reader, RegexToken _token)
	:reader(_reader)
	, token(_token)
{
}

Ptr<CppTokenCursor> CppTokenCursor::Next()
{
	if (!next)
	{
		next = reader->CreateNextToken();
	}
	return next;
}

/***********************************************************************
CppTokenReader
***********************************************************************/

Ptr<CppTokenCursor> CppTokenReader::CreateNextToken()
{
	while (tokenEnumerator->Next())
	{
		auto token = tokenEnumerator->Current();
		switch((CppTokens)token.token)
		{
		case CppTokens::SPACE:
		case CppTokens::COMMENT1:
		case CppTokens::COMMENT2:
			continue;
		}
		return new CppTokenCursor(this, token);
	}
	return nullptr;
}

CppTokenReader::CppTokenReader(Ptr<RegexLexer> _lexer, const WString& input)
	:lexer(_lexer)
{
	tokens = new RegexTokens(lexer->Parse(input));
	tokenEnumerator = tokens->CreateEnumerator();

	firstToken = CreateNextToken();
}

CppTokenReader::~CppTokenReader()
{
	delete tokenEnumerator;
}

Ptr<CppTokenCursor> CppTokenReader::GetFirstToken()
{
	return firstToken;
}