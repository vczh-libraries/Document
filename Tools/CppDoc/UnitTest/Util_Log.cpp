#include <Parser.h>
#include <Ast_Type.h>
#include <Ast_Decl.h>
#include <Ast_Expr.h>
#include <Ast_Stat.h>
#include "Util.h"

void Log(Ptr<Initializer> initializer, StreamWriter& writer)
{
	switch (initializer->initializerType)
	{
	case InitializerType::Equal:
		writer.WriteString(L" = ");
		break;
	case InitializerType::Constructor:
		writer.WriteString(L" (");
		break;
	case InitializerType::Universal:
		writer.WriteString(L" {");
		break;
	}

	for (vint i = 0; i < initializer->arguments.Count(); i++)
	{
		if (i != 0)
		{
			writer.WriteString(L", ");
		}
		Log(initializer->arguments[i], writer);
	}

	switch (initializer->initializerType)
	{
	case InitializerType::Constructor:
		writer.WriteChar(L')');
		break;
	case InitializerType::Universal:
		writer.WriteChar(L'}');
		break;
	}
}

void Log(Ptr<Declarator> declarator, StreamWriter& writer)
{
	if (declarator->name)
	{
		writer.WriteString(declarator->name.name);
		writer.WriteString(L": ");
	}

	Log(declarator->type, writer);

	if (declarator->initializer)
	{
		Log(declarator->initializer, writer);
	}
}

/***********************************************************************
LogIndentation
***********************************************************************/

class LogIndentation
{
public:
	StreamWriter&			writer;
	vint					indentation;

	LogIndentation(StreamWriter& _writer, vint _indentation)
		:writer(_writer)
		, indentation(_indentation)
	{
	}

	void WriteIndentation()
	{
		for (vint i = 0; i < indentation; i++)
		{
			writer.WriteString(L"\t");
		}
	}
};

/***********************************************************************
LogExprVisitor
***********************************************************************/

class LogExprVisitor : public Object, public virtual IExprVisitor
{
private:
	StreamWriter&			writer;

public:
	LogExprVisitor(StreamWriter& _writer)
		:writer(_writer)
	{
	}

	void Visit(LiteralExpr* self)override
	{
		for (vint i = 0; i < self->tokens.Count(); i++)
		{
			if (i != 0)
			{
				writer.WriteChar(L' ');
			}
			writer.WriteString(self->tokens[i].reading, self->tokens[i].length);
		}
	}

	void Visit(ThisExpr* self)override
	{
		throw 0;
	}

	void Visit(NullptrExpr* self)override
	{
		writer.WriteString(L"nullptr");
	}

	void Visit(ParenthesisExpr* self)override
	{
		throw 0;
	}

	void Visit(CastExpr* self)override
	{
		throw 0;
	}

	void Visit(TypeidExpr* self)override
	{
		throw 0;
	}

	void Visit(IdExpr* self)override
	{
		writer.WriteString(self->name.name);
	}

	void Visit(ChildExpr* self)override
	{
		Log(self->classType, writer);
		writer.WriteString(L" :: ");
		writer.WriteString(self->name.name);
	}

	void Visit(FieldAccessExpr* self)override
	{
		Log(self->expr, writer);
		switch (self->type)
		{
		case CppFieldAccessType::Dot: writer.WriteString(L"."); break;
		case CppFieldAccessType::Arrow: writer.WriteString(L"->"); break;
		}
		writer.WriteString(self->name.name);
	}

	void Visit(ArrayAccessExpr* self)override
	{
		Log(self->expr, writer);
		writer.WriteChar(L'[');
		Log(self->index, writer);
		writer.WriteChar(L']');
	}

	void Visit(FuncAccessExpr* self)override
	{
		Log(self->expr, writer);
		writer.WriteChar(L'(');
		for (vint i = 0; i < self->arguments.Count(); i++)
		{
			if (i > 0) writer.WriteString(L", ");
			Log(self->arguments[i], writer);
		}
		writer.WriteChar(L')');
	}
};

/***********************************************************************
LogTypeVisitor
***********************************************************************/

class LogTypeVisitor : public Object, public virtual ITypeVisitor
{
private:
	StreamWriter&			writer;

public:
	LogTypeVisitor(StreamWriter& _writer)
		:writer(_writer)
	{
	}

	void Visit(PrimitiveType* self)override
	{
		switch (self->prefix)
		{
		case CppPrimitivePrefix::_signed:
			writer.WriteString(L"signed ");
			break;
		case CppPrimitivePrefix::_unsigned:
			writer.WriteString(L"unsigned ");
			break;
		}

		switch (self->primitive)
		{
		case CppPrimitiveType::_auto:			writer.WriteString(L"auto");		break;
		case CppPrimitiveType::_void:			writer.WriteString(L"void");		break;
		case CppPrimitiveType::_bool:			writer.WriteString(L"bool");		break;
		case CppPrimitiveType::_char:			writer.WriteString(L"char");		break;
		case CppPrimitiveType::_wchar_t:		writer.WriteString(L"wchar_t");		break;
		case CppPrimitiveType::_char16_t:		writer.WriteString(L"char16_t");	break;
		case CppPrimitiveType::_char32_t:		writer.WriteString(L"char32_t");	break;
		case CppPrimitiveType::_short:			writer.WriteString(L"short");		break;
		case CppPrimitiveType::_int:			writer.WriteString(L"int");			break;
		case CppPrimitiveType::___int8:			writer.WriteString(L"__int8");		break;
		case CppPrimitiveType::___int16:		writer.WriteString(L"__int16");		break;
		case CppPrimitiveType::___int32:		writer.WriteString(L"__int32");		break;
		case CppPrimitiveType::___int64:		writer.WriteString(L"__int64");		break;
		case CppPrimitiveType::_long:			writer.WriteString(L"long");		break;
		case CppPrimitiveType::_long_int:		writer.WriteString(L"long int");	break;
		case CppPrimitiveType::_long_long:		writer.WriteString(L"long long");	break;
		case CppPrimitiveType::_float:			writer.WriteString(L"float");		break;
		case CppPrimitiveType::_double:			writer.WriteString(L"double");		break;
		case CppPrimitiveType::_long_double:	writer.WriteString(L"long double");	break;
		default:
			throw 0;
		}
	}

	void Visit(ReferenceType* self)override
	{
		Log(self->type, writer);
		switch (self->reference)
		{
		case CppReferenceType::Ptr:				writer.WriteString(L" *");	break;
		case CppReferenceType::LRef:			writer.WriteString(L" &");	break;
		case CppReferenceType::RRef:			writer.WriteString(L" &&");	break;
		default:
			throw 0;
		}
	}

	void Visit(ArrayType* self)override
	{
		Log(self->type, writer);
		writer.WriteString(L" [");
		if (self->expr)
		{
			Log(self->expr, writer);
		}
		writer.WriteString(L"]");
	}

	void Visit(CallingConventionType* self)override
	{
		Log(self->type, writer);

		switch (self->callingConvention)
		{
		case CppCallingConvention::CDecl:
			writer.WriteString(L" __cdecl");
			break;
		case CppCallingConvention::ClrCall:
			writer.WriteString(L" __clrcall");
			break;
		case CppCallingConvention::StdCall:
			writer.WriteString(L" __stdcall");
			break;
		case CppCallingConvention::FastCall:
			writer.WriteString(L" __fastcall");
			break;
		case CppCallingConvention::ThisCall:
			writer.WriteString(L" __thiscall");
			break;
		case CppCallingConvention::VectorCall:
			writer.WriteString(L" __vectorcall");
			break;
		}
	}

	void Visit(FunctionType* self)override
	{
		if (self->decoratorReturnType)
		{
			writer.WriteChar(L'(');
			Log(self->returnType, writer);
			writer.WriteString(L"->");
			Log(self->decoratorReturnType, writer);
			writer.WriteChar(L')');
		}
		else
		{
			Log(self->returnType, writer);
		}

		writer.WriteString(L" (");
		for (vint i = 0; i < self->parameters.Count(); i++)
		{
			if (i != 0)
			{
				writer.WriteString(L", ");
			}
			Log(self->parameters[i], writer, 0, false);;
		}
		writer.WriteChar(L')');

		if (self->qualifierConstExpr) writer.WriteString(L" constexpr");
		if (self->qualifierConst) writer.WriteString(L" const");
		if (self->qualifierVolatile) writer.WriteString(L" volatile");
		if (self->qualifierLRef) writer.WriteString(L" &");
		if (self->qualifierRRef) writer.WriteString(L" &&");

		if (self->decoratorOverride) writer.WriteString(L" override");
		if (self->decoratorNoExcept) writer.WriteString(L" noexcept");
		if (self->decoratorThrow)
		{
			writer.WriteString(L" throw(");
			for (vint i = 0; i < self->exceptions.Count(); i++)
			{
				if (i != 0)
				{
					writer.WriteString(L", ");
				}
				Log(self->exceptions[i], writer);
			}
			writer.WriteChar(L')');
		}
	}

	void Visit(MemberType* self)override
	{
		Log(self->type, writer);
		writer.WriteString(L" (");
		Log(self->classType, writer);
		writer.WriteString(L" ::)");
	}

	void Visit(DeclType* self)override
	{
		writer.WriteString(L"decltype(");
		if (self->expr)
		{
			Log(self->expr, writer);
		}
		else
		{
			writer.WriteString(L"auto");
		}
		writer.WriteString(L")");
	}

	void Visit(DecorateType* self)override
	{
		Log(self->type, writer);
		if (self->isConstExpr)	writer.WriteString(L" constexpr");
		if (self->isConst)		writer.WriteString(L" const");
		if (self->isVolatile)	writer.WriteString(L" volatile");
	}

	void Visit(RootType* self)override
	{
		writer.WriteString(L"__root");
	}

	void Visit(IdType* self)override
	{
		writer.WriteString(self->name.name);
	}

	void Visit(ChildType* self)override
	{
		Log(self->classType, writer);
		writer.WriteString(L" :: ");
		if (self->typenameType) writer.WriteString(L"typename ");
		writer.WriteString(self->name.name);
	}

	void Visit(GenericType* self)override
	{
		Log(self->type, writer);
		writer.WriteString(L"<");
		for (vint i = 0; i < self->arguments.Count(); i++)
		{
			if (i != 0)
			{
				writer.WriteString(L", ");
			}

			auto arg = self->arguments[i];
			if (arg.expr) Log(arg.expr, writer);
			if (arg.type) Log(arg.type, writer);
		}
		writer.WriteString(L">");
	}

	void Visit(VariadicTemplateArgumentType* self)override
	{
		self->type->Accept(this);
		writer.WriteString(L"...");
	}
};

/***********************************************************************
LogStatVisitor
***********************************************************************/

class LogStatVisitor : public Object, public virtual IStatVisitor, private LogIndentation
{
public:
	LogStatVisitor(StreamWriter& _writer, vint _indentation)
		:LogIndentation(_writer, _indentation)
	{
	}

	void WriteSubStat(Ptr<Stat> stat)
	{
		if (stat.Cast<BlockStat>())
		{
			WriteIndentation();
			indentation++;
			stat->Accept(this);
			indentation--;
		}
		else
		{
			indentation++;
			WriteIndentation();
			stat->Accept(this);
			indentation--;
		}
	}

	void Visit(EmptyStat* self)override
	{
		writer.WriteLine(L";");
	}

	void Visit(BlockStat* self)override
	{
		bool hasIdentation = indentation > 0;
		if (hasIdentation) indentation--;
		writer.WriteLine(L"{");
		indentation++;
		for (vint i = 0; i < self->stats.Count(); i++)
		{
			WriteIndentation();
			self->stats[i]->Accept(this);
		}
		indentation--;
		WriteIndentation();
		writer.WriteLine(L"}");
		if (hasIdentation) indentation++;
	}

	void Visit(DeclStat* self)override
	{
		for (vint i = 0; i < self->decls.Count(); i++)
		{
			if (i != 0)
			{
				WriteIndentation();
			}
			Log(self->decls[i], writer, indentation, true);
		}
	}

	void Visit(ExprStat* self)override
	{
		Log(self->expr, writer);
		writer.WriteLine(L";");
	}

	void Visit(LabelStat* self)override
	{
		writer.WriteString(self->name.name);
		writer.WriteString(L": ");
		self->stat->Accept(this);
	}

	void Visit(DefaultStat* self)override
	{
		writer.WriteString(L"default: ");
		self->stat->Accept(this);
	}

	void Visit(CaseStat* self)override
	{
		writer.WriteString(L"case ");
		Log(self->expr, writer);
		writer.WriteString(L": ");
		self->stat->Accept(this);
	}

	void Visit(GotoStat* self)override
	{
		writer.WriteString(L"goto ");
		writer.WriteString(self->name.name);
		writer.WriteLine(L";");
	}

	void Visit(BreakStat* self)override
	{
		writer.WriteLine(L"break;");
	}

	void Visit(ContinueStat* self)override
	{
		writer.WriteLine(L"continue;");
	}

	void Visit(WhileStat* self)override
	{
		writer.WriteString(L"while (");
		if (self->varExpr)
		{
			Log(self->varExpr, writer, indentation, false);
		}
		else
		{
			Log(self->expr, writer);
		}
		writer.WriteLine(L")");
		WriteSubStat(self->stat);
	}

	void Visit(DoWhileStat* self)override
	{
		writer.WriteLine(L"do");
		WriteSubStat(self->stat);

		WriteIndentation();
		writer.WriteString(L"while (");
		Log(self->expr, writer);
		writer.WriteLine(L");");
	}

	void Visit(ForEachStat* self)override
	{
		writer.WriteString(L"foreach (");
		Log(self->varDecl, writer, 0, false);
		writer.WriteString(L" : ");
		Log(self->expr, writer);
		writer.WriteLine(L")");
		WriteSubStat(self->stat);
	}

	void Visit(ForStat* self)override
	{
		writer.WriteString(L"for (");
		if (self->init)
		{
			Log(self->init, writer);
		}
		else
		{
			for (vint i = 0; i < self->varDecls.Count(); i++)
			{
				if (i > 0)
				{
					writer.WriteString(L", ");
				}
				Log(self->varDecls[i], writer, 0, false);
			}
		}
		writer.WriteString(L"; ");

		if (self->expr) Log(self->expr, writer);
		writer.WriteString(L"; ");

		if (self->effect) Log(self->effect, writer);
		writer.WriteLine(L")");
		WriteSubStat(self->stat);
	}

	void Visit(IfElseStat* self)override
	{
		writer.WriteString(L"if (");
		for (vint i = 0; i < self->varDecls.Count(); i++)
		{
			Log(self->varDecls[i], writer, 0, false);
			if (i == self->varDecls.Count() - 1)
			{
				writer.WriteString(L"; ");
			}
			else
			{
				writer.WriteString(L", ");
			}
		}

		if (self->varExpr)
		{
			Log(self->varExpr, writer, indentation, false);
		}
		else
		{
			Log(self->expr, writer);
		}
		writer.WriteLine(L")");
		WriteSubStat(self->trueStat);

		if (self->falseStat)
		{
			WriteIndentation();
			if (self->falseStat.Cast<IfElseStat>())
			{
				writer.WriteString(L"else ");
				self->falseStat->Accept(this);
			}
			else
			{
				writer.WriteLine(L"else");
				WriteSubStat(self->falseStat);
			}
		}
	}

	void Visit(SwitchStat* self)override
	{
		writer.WriteString(L"switch (");
		if (self->varExpr)
		{
			Log(self->varExpr, writer, indentation, false);
		}
		else
		{
			Log(self->expr, writer);
		}
		writer.WriteLine(L")");
		WriteSubStat(self->stat);
	}

	void Visit(TryCatchStat* self)override
	{
		writer.WriteLine(L"try");
		WriteSubStat(self->tryStat);

		WriteIndentation();
		if (self->exception)
		{
			writer.WriteString(L"catch (");
			Log(self->exception, writer, 0, false);
			writer.WriteLine(L")");
		}
		else
		{
			writer.WriteLine(L"catch (...)");
		}
		WriteSubStat(self->catchStat);
	}

	void Visit(ReturnStat* self)override
	{
		if (self->expr)
		{
			writer.WriteString(L"return ");
			Log(self->expr, writer);
			writer.WriteLine(L";");
		}
		else
		{
			writer.WriteLine(L"return;");
		}
	}

	void Visit(__Try__ExceptStat* self)override
	{
		writer.WriteLine(L"__try");
		WriteSubStat(self->tryStat);

		WriteIndentation();
		writer.WriteString(L"__except (");
		Log(self->expr, writer);
		writer.WriteLine(L")");
		WriteSubStat(self->exceptStat);
	}

	void Visit(__Try__FinallyStat* self)override
	{
		writer.WriteLine(L"__try");
		WriteSubStat(self->tryStat);

		WriteIndentation();
		writer.WriteLine(L"__finally");
		WriteSubStat(self->finallyStat);
	}

	void Visit(__LeaveStat* self)override
	{
		writer.WriteLine(L"__leave;");
	}

	void Visit(__IfExistsStat* self)override
	{
		writer.WriteString(L"__if_exists (");
		Log(self->expr, writer);
		writer.WriteLine(L")");
		WriteSubStat(self->stat);
	}

	void Visit(__IfNotExistsStat* self)override
	{
		writer.WriteString(L"__if_not_exists (");
		Log(self->expr, writer);
		writer.WriteLine(L")");
		WriteSubStat(self->stat);
	}
};

/***********************************************************************
LogDeclVisitor
***********************************************************************/

class LogDeclVisitor : public Object, public virtual IDeclarationVisitor, private LogIndentation
{
private:
	bool					semicolon;


	void WriteHeader(ForwardVariableDeclaration* self)
	{
		if (self->decoratorExtern) writer.WriteString(L"extern ");
		if (self->decoratorMutable) writer.WriteString(L"mutable ");
		if (self->decoratorRegister) writer.WriteString(L"register ");
		if (self->decoratorStatic) writer.WriteString(L"static ");
		if (self->decoratorThreadLocal) writer.WriteString(L"thread_local ");
		if (self->name)
		{
			writer.WriteString(self->name.name);
			writer.WriteString(L": ");
		}
		Log(self->type, writer);
	}

	void WriteHeader(ForwardFunctionDeclaration* self)
	{
		if (self->decoratorExplicit) writer.WriteString(L"explicit ");
		if (self->decoratorExtern) writer.WriteString(L"extern ");
		if (self->decoratorFriend) writer.WriteString(L"friend ");
		if (self->decoratorInline) writer.WriteString(L"inline ");
		if (self->decoratorForceInline) writer.WriteString(L"__forceinline ");
		if (self->decoratorStatic) writer.WriteString(L"static ");
		if (self->decoratorVirtual) writer.WriteString(L"virtual ");
		switch (self->methodType)
		{
		case CppMethodType::Constructor:
			writer.WriteString(L"__ctor ");
			break;
		case CppMethodType::Destructor:
			writer.WriteString(L"__dtor ");
			break;
		case CppMethodType::TypeConversion:
			writer.WriteString(L"__type ");
			break;
		}
		writer.WriteString(self->name.name);
		writer.WriteString(L": ");
		Log(self->type, writer);
		if (self->decoratorAbstract)
		{
			writer.WriteString(L" = 0");
		}
	}

	void WriteHeader(ForwardEnumDeclaration* self)
	{
		writer.WriteString(self->enumClass ? L"enum class " : L"enum ");
		writer.WriteString(self->name.name);
		if (self->baseType)
		{
			writer.WriteString(L" : ");
			Log(self->baseType, writer);
		}
	}

	void WriteHeader(ForwardClassDeclaration* self)
	{
		switch (self->classType)
		{
		case CppClassType::Class:
			writer.WriteString(L"class ");
			break;
		case CppClassType::Struct:
			writer.WriteString(L"struct ");
			break;
		case CppClassType::Union:
			writer.WriteString(L"union ");
			break;
		}
		writer.WriteString(self->name.name);
	}

public:
	LogDeclVisitor(StreamWriter& _writer, vint _indentation, bool _semicolon)
		:LogIndentation(_writer, _indentation)
		, semicolon(_semicolon)
	{
	}

	void Visit(ForwardVariableDeclaration* self)override
	{
		writer.WriteString(L"__forward ");
		WriteHeader(self);
		if (semicolon) writer.WriteLine(L";");
	}

	void Visit(ForwardFunctionDeclaration* self)override
	{
		writer.WriteString(L"__forward ");
		WriteHeader(self);
		if (semicolon) writer.WriteLine(L";");
	}

	void Visit(ForwardEnumDeclaration* self)override
	{
		writer.WriteString(L"__forward ");
		WriteHeader(self);
		if (semicolon) writer.WriteLine(L";");
	}

	void Visit(ForwardClassDeclaration* self)override
	{
		writer.WriteString(L"__forward ");
		WriteHeader(self);
		if (semicolon) writer.WriteLine(L";");
	}

	void Visit(VariableDeclaration* self)override
	{
		WriteHeader(self);
		if (self->initializer)
		{
			Log(self->initializer, writer);
		}
		if (semicolon) writer.WriteLine(L";");
	}

	void Visit(FunctionDeclaration* self)override
	{
		WriteHeader(self);
		writer.WriteLine(L"");
		WriteIndentation();
		Log(self->statement, writer, indentation + 1);
	}

	void Visit(EnumItemDeclaration* self)override
	{
		writer.WriteString(self->name.name);
		if (self->value)
		{
			writer.WriteString(L" = ");
			Log(self->value, writer);
		}
		writer.WriteLine(L",");
	}

	void Visit(EnumDeclaration* self)override
	{
		WriteHeader(self);
		writer.WriteLine(L"");

		WriteIndentation();
		writer.WriteLine(L"{");

		indentation++;
		for (vint i = 0; i < self->items.Count(); i++)
		{
			WriteIndentation();
			self->items[i]->Accept(this);
		}
		indentation--;

		WriteIndentation();
		writer.WriteString(L"}");
		if (semicolon) writer.WriteLine(L";");
	}

	void Visit(ClassDeclaration* self)override
	{
		WriteHeader(self);
		for (vint i = 0; i < self->baseTypes.Count(); i++)
		{
			if (i == 0)
			{
				writer.WriteString(L" : ");
			}
			else
			{
				writer.WriteString(L", ");
			}

			auto pair = self->baseTypes[i];
			switch (pair.f0)
			{
			case CppClassAccessor::Public:
				writer.WriteString(L"public ");
				break;
			case CppClassAccessor::Protected:
				writer.WriteString(L"protected ");
				break;
			case CppClassAccessor::Private:
				writer.WriteString(L"private ");
				break;
			}
			Log(pair.f1, writer);
		}
		writer.WriteLine(L"");

		WriteIndentation();
		writer.WriteLine(L"{");

		indentation++;
		for (vint i = 0; i < self->decls.Count(); i++)
		{
			WriteIndentation();
			auto pair = self->decls[i];
			switch (pair.f0)
			{
			case CppClassAccessor::Public:
				writer.WriteString(L"public ");
				break;
			case CppClassAccessor::Protected:
				writer.WriteString(L"protected ");
				break;
			case CppClassAccessor::Private:
				writer.WriteString(L"private ");
				break;
			}
			pair.f1->Accept(this);
		}
		indentation--;

		WriteIndentation();
		writer.WriteString(L"}");
		if (semicolon) writer.WriteLine(L";");
	}

	void Visit(TypeAliasDeclaration* self)override
	{
		throw 0;
	}

	void Visit(UsingNamespaceDeclaration* self)override
	{
		writer.WriteString(L"using namespace ");
		Log(self->type, writer);
		if (semicolon) writer.WriteLine(L";");
	}

	void Visit(UsingDeclaration* self)override
	{
		throw 0;
	}

	void Visit(NamespaceDeclaration* self)override
	{
		writer.WriteString(L"namespace ");
		writer.WriteLine(self->name.name);

		WriteIndentation();
		writer.WriteLine(L"{");

		indentation++;
		for (vint i = 0; i < self->decls.Count(); i++)
		{
			WriteIndentation();
			self->decls[i]->Accept(this);
		}
		indentation--;

		WriteIndentation();
		writer.WriteLine(L"}");
	}
};

/***********************************************************************
Log
***********************************************************************/

void Log(Ptr<Expr> expr, StreamWriter& writer)
{
	LogExprVisitor visitor(writer);
	expr->Accept(&visitor);
}

void Log(Ptr<Type> type, StreamWriter& writer)
{
	if (type)
	{
		LogTypeVisitor visitor(writer);
		type->Accept(&visitor);
	}
	else
	{
		writer.WriteString(L"__null");
	}
}

void Log(Ptr<Stat> stat, StreamWriter& writer, vint indentation)
{
	LogStatVisitor visitor(writer, indentation);
	stat->Accept(&visitor);
}

void Log(Ptr<Declaration> decl, StreamWriter& writer, vint indentation, bool semicolon)
{
	LogDeclVisitor visitor(writer, indentation, semicolon);
	decl->Accept(&visitor);
}

void Log(Ptr<Program> program, StreamWriter& writer)
{
	for (vint i = 0; i < program->decls.Count(); i++)
	{
		Log(program->decls[i], writer, 0, true);
	}
}

void Log(ITsys* tsys, StreamWriter& writer)
{
	switch (tsys->GetType())
	{
	case TsysType::Nullptr:
		writer.WriteString(L"nullptr_t");
		return;
	case TsysType::Primitive:
		{
			auto primitive = tsys->GetPrimitive();
			switch (primitive.type)
			{
			case TsysPrimitiveType::SInt:
				switch (primitive.bytes)
				{
				case TsysBytes::_1:	writer.WriteString(L"__int8"); return;
				case TsysBytes::_2:	writer.WriteString(L"__int16"); return;
				case TsysBytes::_4:	writer.WriteString(L"__int32"); return;
				case TsysBytes::_8:	writer.WriteString(L"__int64"); return;
				}
				break;
			case TsysPrimitiveType::UInt:
				switch (primitive.bytes)
				{
				case TsysBytes::_1:	writer.WriteString(L"unsigned __int8"); return;
				case TsysBytes::_2:	writer.WriteString(L"unsigned __int16"); return;
				case TsysBytes::_4:	writer.WriteString(L"unsigned __int32"); return;
				case TsysBytes::_8:	writer.WriteString(L"unsigned __int64"); return;
				}
				break;
			case TsysPrimitiveType::Float:
				switch (primitive.bytes)
				{
				case TsysBytes::_4: writer.WriteString(L"float"); return;
				case TsysBytes::_8: writer.WriteString(L"double"); return;
				}
				break;
			case TsysPrimitiveType::SChar:
				switch (primitive.bytes)
				{
				case TsysBytes::_1: writer.WriteString(L"char"); return;
				case TsysBytes::_2: writer.WriteString(L"signed char16_t"); return;
				case TsysBytes::_4: writer.WriteString(L"signed char32_t"); return;
				}
				break;
			case TsysPrimitiveType::UChar:
				switch (primitive.bytes)
				{
				case TsysBytes::_1: writer.WriteString(L"unsigned char"); return;
				case TsysBytes::_2: writer.WriteString(L"char16_t"); return;
				case TsysBytes::_4: writer.WriteString(L"char32_t"); return;
				}
				break;
			case TsysPrimitiveType::SWChar:
				switch (primitive.bytes)
				{
				case TsysBytes::_2: writer.WriteString(L"signed wchar_t"); return;
				}
				break;
			case TsysPrimitiveType::UWChar:
				switch (primitive.bytes)
				{
				case TsysBytes::_2: writer.WriteString(L"wchar_t"); return;
				}
				break;
			case TsysPrimitiveType::Bool:
				switch (primitive.bytes)
				{
				case TsysBytes::_1: writer.WriteString(L"bool"); return;
				}
				break;
			case TsysPrimitiveType::Void:
				switch (primitive.bytes)
				{
				case TsysBytes::_1: writer.WriteString(L"void"); return;
				}
				break;
			}
		}
		break;
	case TsysType::LRef:
		Log(tsys->GetElement(), writer);
		writer.WriteString(L" &");
		return;
	case TsysType::RRef:
		Log(tsys->GetElement(), writer);
		writer.WriteString(L" &&");
		return;
	case TsysType::Ptr:
		Log(tsys->GetElement(), writer);
		writer.WriteString(L" *");
		return;
	case TsysType::Array:
		{
			Log(tsys->GetElement(), writer);
			writer.WriteString(L" [");
			vint dim = tsys->GetParamCount();
			for (vint i = 1; i < dim; i++)
			{
				writer.WriteChar(L',');
			}
			writer.WriteChar(L']');
		}
		return;
	case TsysType::Function:
		Log(tsys->GetElement(), writer);
		writer.WriteString(L" (");
		for (vint i = 0; i < tsys->GetParamCount(); i++)
		{
			if (i > 0) writer.WriteString(L", ");
			Log(tsys->GetParam(i), writer);
		}
		writer.WriteChar(L')');
		return;
	case TsysType::Member:
		Log(tsys->GetElement(), writer);
		writer.WriteString(L" (");
		Log(tsys->GetClass(), writer);
		writer.WriteString(L" ::)");
		return;
	case TsysType::CV:
		{
			auto cv = tsys->GetCV();
			Log(tsys->GetElement(), writer);
			if (cv.isConstExpr) writer.WriteString(L" constexpr");
			if (cv.isConst) writer.WriteString(L" const");
			if (cv.isVolatile) writer.WriteString(L" volatile");
		}
		return;
	case TsysType::Decl:
		{
			auto symbol = tsys->GetDecl();
			WString name;
			while (symbol && symbol->parent)
			{
				name = L"::" + symbol->name + name;
				symbol = symbol->parent;
			}
			writer.WriteString(name);
		}
		return;
	case TsysType::Generic:
		break;
	case TsysType::GenericArg:
		break;
	case TsysType::Expr:
		break;
	}
	throw L"Invalid!";
}