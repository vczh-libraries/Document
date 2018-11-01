#include "Lexer.h"

/***********************************************************************
CreateCppLexer
***********************************************************************/

Ptr<RegexLexer> CreateCppLexer()
{
	List<WString> tokens;
#define DEFINE_REGEX_TOKEN(NAME, REGEX) if ((vint)CppTokens::NAME != tokens.Add(REGEX)) { throw 0; }
#define DEFINE_KEYWORD_TOKEN(NAME, KEYWORD) DEFINE_REGEX_TOKEN(NAME, L#KEYWORD)
	CPP_ALL_TOKENS(DEFINE_KEYWORD_TOKEN, DEFINE_REGEX_TOKEN)
#undef DEFINE_KEYWORD_TOKEN
#undef DEFINE_REGEX_TOKEN

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
}

CppTokenReader::~CppTokenReader()
{
	delete tokenEnumerator;
}

Ptr<CppTokenCursor> CppTokenReader::GetFirstToken()
{
	if (gotFirstToken)
	{
		throw L"You cannot call GetFirstToken() twice.";
	}
	return CreateNextToken();
}