#include "Parser.h"
#include "Parser_Declarator.h"
#include "Ast_Decl.h"
#include "Ast_Expr.h"

/***********************************************************************
EnsureFunctionBodyParsed
***********************************************************************/

class ProcessDelayParseDeclarationVisitor : public Object, public IDeclarationVisitor
{
public:
	void Visit(ForwardVariableDeclaration* self) override
	{
	}

	void Visit(ForwardFunctionDeclaration* self) override
	{
	}

	void Visit(ForwardEnumDeclaration* self) override
	{
	}

	void Visit(ForwardClassDeclaration* self) override
	{
	}

	void Visit(FriendClassDeclaration* self) override
	{
	}

	void Visit(VariableDeclaration* self) override
	{
	}

	void Visit(FunctionDeclaration* self) override
	{
		EnsureFunctionBodyParsed(self);
	}

	void Visit(EnumItemDeclaration* self) override
	{
	}

	void Visit(EnumDeclaration* self) override
	{
	}

	void Visit(ClassDeclaration* self) override
	{
		for (vint i = 0; i < self->decls.Count(); i++)
		{
			self->decls[i].get<1>()->Accept(this);
		}
	}

	void Visit(NestedAnonymousClassDeclaration* self) override
	{
		for (vint i = 0; i < self->decls.Count(); i++)
		{
			self->decls[i]->Accept(this);
		}
	}

	void Visit(UsingNamespaceDeclaration* self) override
	{
	}

	void Visit(UsingSymbolDeclaration* self) override
	{
	}

	void Visit(TypeAliasDeclaration* self) override
	{
	}

	void Visit(ValueAliasDeclaration* self) override
	{
	}

	void Visit(NamespaceDeclaration* self) override
	{
		for (vint i = 0; i < self->decls.Count(); i++)
		{
			self->decls[i]->Accept(this);
		}
	}

	void Visit(StaticAssertDeclaration* self) override
	{
	}
};

void EnsureFunctionBodyParsed(FunctionDeclaration* funcDecl)
{
	if (funcDecl->delayParse)
	{
		auto delayParse = funcDecl->delayParse;
		funcDecl->delayParse = nullptr;

		if (delayParse->pa.recorder)
		{
			delayParse->pa.recorder->BeginDelayParse(funcDecl);
		}

		if (TestToken(delayParse->begin, CppTokens::COLON))
		{
			while (true)
			{
				auto item = Ptr(new FunctionDeclaration::InitItem);
				{
					auto oldCursor = delayParse->begin;
					try
					{
						item->type = ParseLongType(delayParse->pa, delayParse->begin);
					}
					catch (const StopParsingException&)
					{
						delayParse->begin = oldCursor;
					}
				}
				if (!item->type)
				{
					item->field = Ptr(new IdExpr);
					if (!ParseCppName(item->field->name, delayParse->begin))
					{
						throw StopParsingException(delayParse->begin);
					}
					if (item->field->name.type != CppNameType::Normal)
					{
						throw StopParsingException(delayParse->begin);
					}
				}

				if (TestToken(delayParse->begin, CppTokens::LBRACE))
				{
					item->universalInitialization = true;
				}
				else
				{
					RequireToken(delayParse->begin, CppTokens::LPARENTHESIS);
					item->universalInitialization = false;
				}

				if (item->universalInitialization)
				{
					if (TestToken(delayParse->begin, CppTokens::RBRACE))
					{
						goto SKIP_ARGUMENTS;
					}
				}
				else
				{
					if (TestToken(delayParse->begin, CppTokens::RPARENTHESIS))
					{
						goto SKIP_ARGUMENTS;
					}
				}

				while (true)
				{
					auto argument = ParseExpr(delayParse->pa, pea_Argument(), delayParse->begin);
					bool isVariadic = TestToken(delayParse->begin, CppTokens::DOT, CppTokens::DOT, CppTokens::DOT);
					item->arguments.Add({ argument,isVariadic });
					if (!TestToken(delayParse->begin, CppTokens::COMMA))
					{
						if (item->universalInitialization)
						{
							RequireToken(delayParse->begin, CppTokens::RBRACE);
						}
						else
						{
							RequireToken(delayParse->begin, CppTokens::RPARENTHESIS);
						}
						goto SKIP_ARGUMENTS;
					}
				}
			SKIP_ARGUMENTS:
				funcDecl->initList.Add(item);

				if (!TestToken(delayParse->begin, CppTokens::COMMA))
				{
					if (TestToken(delayParse->begin, CppTokens::LBRACE, false))
					{
						break;
					}
					else
					{
						throw StopParsingException(delayParse->begin);
					}
				}
			}
		}

		funcDecl->statement = ParseStat(delayParse->pa, delayParse->begin);
		if (delayParse->begin)
		{
			if (delayParse->end.reading != delayParse->begin->token.reading)
			{
				throw StopParsingException(delayParse->begin);
			}
		}
		else
		{
			if (delayParse->end.reading != nullptr)
			{
				throw StopParsingException(delayParse->begin);
			}
		}

		if (delayParse->pa.recorder)
		{
			delayParse->pa.recorder->EndDelayParse(funcDecl);
		}
	}
}

void PredefineType(Ptr<Program> program, const ParsingArguments& pa, const wchar_t* name, bool isForwardDeclaration, CppClassType classType, symbol_component::SymbolKind symbolKind)
{
	auto decl = isForwardDeclaration ? Ptr(new ForwardClassDeclaration) : (Ptr<ForwardClassDeclaration>)Ptr(new ClassDeclaration);
	decl->classType = classType;
	decl->name.name = name;
	program->decls.Insert(program->createdForwardDeclByCStyleTypeReference++, decl);
	if (isForwardDeclaration)
	{
		pa.root->AddForwardDeclToSymbol_NFb(decl, symbolKind);
	}
	else
	{
		pa.root->AddImplDeclToSymbol_NFb(decl, symbolKind);
	}
}

bool ParseTypeOrExpr(const ParsingArguments& pa, const ParsingExprArguments& pea, Ptr<CppTokenCursor>& cursor, Ptr<Type>& type, Ptr<Expr>& expr)
{
	auto oldCursor = cursor;
	try
	{
		expr = ParseExpr(pa, pea, cursor);

		// the expression could be too long and make the destructor stackoverflow
		while (oldCursor != cursor)
		{
			oldCursor = oldCursor->Next();
		}
		return false;
	}
	catch (const StopParsingException&)
	{
	}
	cursor = oldCursor;
	type = ParseType(pa, cursor);
	return true;
}

Ptr<Program> ParseProgram(ParsingArguments& pa, Ptr<CppTokenCursor>& cursor)
{
	auto program = Ptr(new Program);
	pa.program = program;

	// these types will be used before it is defined
	/*
	PredefineType(program, pa, L"__m64", true, CppClassType::Union, symbol_component::SymbolKind::Struct);
	PredefineType(program, pa, L"__m128", true, CppClassType::Union, symbol_component::SymbolKind::Struct);
	PredefineType(program, pa, L"__m128d", true, CppClassType::Struct, symbol_component::SymbolKind::Struct);
	PredefineType(program, pa, L"__m128i", true, CppClassType::Union, symbol_component::SymbolKind::Union);
	PredefineType(program, pa, L"__m256", true, CppClassType::Union, symbol_component::SymbolKind::Union);
	PredefineType(program, pa, L"__m256d", true, CppClassType::Struct, symbol_component::SymbolKind::Struct);
	PredefineType(program, pa, L"__m256i", true, CppClassType::Union, symbol_component::SymbolKind::Union);
	PredefineType(program, pa, L"__m512", true, CppClassType::Union, symbol_component::SymbolKind::Union);
	PredefineType(program, pa, L"__m512d", true, CppClassType::Struct, symbol_component::SymbolKind::Struct);
	PredefineType(program, pa, L"__m512i", true, CppClassType::Union, symbol_component::SymbolKind::Union);
	*/

	if (pa.recorder)
	{
		pa.recorder->BeginPhase(IIndexRecorder::Phase::ParseProgram);
	}
	while (cursor)
	{
		ParseDeclaration(pa, cursor, program->decls);
	}

	if (pa.recorder)
	{
		pa.recorder->BeginPhase(IIndexRecorder::Phase::ParseFunctionBodies);
	}
	ProcessDelayParseDeclarationVisitor visitor;
	for (vint i = 0; i < program->decls.Count(); i++)
	{
		program->decls[i]->Accept(&visitor);
	}
	return program;
}