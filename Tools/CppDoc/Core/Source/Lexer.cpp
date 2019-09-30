#include "Lexer.h"

/***********************************************************************
CreateCppLexer
***********************************************************************/

Ptr<RegexLexer> CreateCppLexer()
{
	struct ThisShouldNotHappen {};

	List<WString> tokens;
#define DEFINE_REGEX_TOKEN(NAME, REGEX) if ((vint)CppTokens::NAME != tokens.Add(REGEX)) { throw ThisShouldNotHappen(); }
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

void CppTokenCursor::Clone(Ptr<CppTokenReader>& _reader, Ptr<CppTokenCursor>& _cursor)
{
	_reader = new CppTokenReader(*reader, reader->skipSpaceAndComment);
	_cursor = new CppTokenCursor(_reader.Obj(), token);

	auto reading = this;
	auto writing = _cursor.Obj();
	while (reading->next)
	{
		reading = reading->next.Obj();
		auto next = new CppTokenCursor(writing->reader, reading->token);
		writing->next = next;
		writing = next;
	}
}

/***********************************************************************
CppTokenReader
***********************************************************************/

Ptr<CppTokenCursor> CppTokenReader::CreateNextToken()
{
	while (tokenEnumerator->Next())
	{
		auto token = tokenEnumerator->Current();
		if (skipSpaceAndComment)
		{
			switch ((CppTokens)token.token)
			{
			case CppTokens::SPACE:
			case CppTokens::COMMENT1:
			case CppTokens::COMMENT2:
				continue;
			}
		}
		return new CppTokenCursor(this, token);
	}
	return nullptr;
}

CppTokenReader::CppTokenReader(const CppTokenReader& _reader, bool _skipSpaceAndComment)
	:gotFirstToken(_reader.gotFirstToken)
	, skipSpaceAndComment(_skipSpaceAndComment)
	, lexer(_reader.lexer)
	, tokens(_reader.tokens)
	, tokenEnumerator(_reader.tokenEnumerator ? _reader.tokenEnumerator->Clone() : nullptr)
{
}

CppTokenReader::CppTokenReader(Ptr<RegexLexer> _lexer, const WString& input, bool _skipSpaceAndComment)
	:skipSpaceAndComment(_skipSpaceAndComment)
	, lexer(_lexer)
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
	gotFirstToken = true;
	return CreateNextToken();
}