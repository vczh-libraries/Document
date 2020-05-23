#include <Ast_Decl.h>
#include <Ast_Expr.h>
#include "Util.h"

/***********************************************************************
LogDeclVisitor
***********************************************************************/

class LogDeclVisitor : public Object, public virtual IDeclarationVisitor, private LogIndentation
{
private:
	bool					semicolon;

	void WriteHeader(ForwardVariableDeclaration* self)
	{
		if (self->decoratorConstexpr) writer.WriteString(L"constexpr ");
		if (self->decoratorExtern) writer.WriteString(L"extern ");
		if (self->decoratorMutable) writer.WriteString(L"mutable ");
		if (self->decoratorRegister) writer.WriteString(L"register ");
		if (self->decoratorStatic) writer.WriteString(L"static ");
		if (self->decoratorThreadLocal) writer.WriteString(L"thread_local ");
		if (self->decoratorInline) writer.WriteString(L"inline ");
		if (self->decorator__Inline) writer.WriteString(L"__inline ");
		if (self->decorator__ForceInline) writer.WriteString(L"__forceinline ");
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
		if (self->decoratorConstexpr) writer.WriteString(L"constexpr ");
		if (self->decoratorExtern) writer.WriteString(L"extern ");
		if (self->decoratorFriend) writer.WriteString(L"friend ");
		if (self->decoratorInline) writer.WriteString(L"inline ");
		if (self->decorator__Inline) writer.WriteString(L"__inline ");
		if (self->decorator__ForceInline) writer.WriteString(L"__forceinline ");
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
		if (self->specializationSpec)
		{
			Log(self->specializationSpec->arguments, L"<", L">", writer);
		}
		writer.WriteString(L": ");
		Log(self->type, writer);
		if (self->decoratorAbstract)
		{
			writer.WriteString(L" = 0");
		}
		if (self->decoratorDefault)
		{
			writer.WriteString(L" = default");
		}
		if (self->decoratorDelete)
		{
			writer.WriteString(L" = delete");
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
		if (self->specializationSpec)
		{
			Log(self->specializationSpec->arguments, L"<", L">", writer);
		}
	}

	void WriteTemplateSpec(TemplateSpec* spec)
	{
		writer.WriteString(L"template<");
		for (vint i = 0; i < spec->arguments.Count(); i++)
		{
			if (i > 0) writer.WriteString(L", ");
			const auto& arg = spec->arguments[i];
			switch (arg.argumentType)
			{
			case CppTemplateArgumentType::HighLevelType:
				WriteTemplateSpec(arg.templateSpec.Obj());
				writer.WriteChar(L' ');
			case CppTemplateArgumentType::Type:
				writer.WriteString(L"typename");
				if (arg.name)
				{
					writer.WriteChar(L' ');
					if (arg.ellipsis)
					{
						writer.WriteString(L"...");
					}
					writer.WriteString(arg.name.name);
				}
				if (arg.type)
				{
					writer.WriteString(L" = ");
					Log(arg.type, writer);
				}
				break;
			case CppTemplateArgumentType::Value:
				Log(arg.type, writer);
				if (arg.name)
				{
					writer.WriteChar(L' ');
					if (arg.ellipsis)
					{
						writer.WriteString(L"...");
					}
					writer.WriteString(arg.name.name);
				}
				if (arg.expr)
				{
					writer.WriteString(L" = ");
					Log(arg.expr, writer);
				}
				break;
			}
		}
		writer.WriteString(L">");
	}

	void WriteTemplateSpecIfExists(Ptr<TemplateSpec> spec)
	{
		if (spec)
		{
			WriteTemplateSpec(spec.Obj());
			writer.WriteLine(L"");
			WriteIndentation();
		}
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
		WriteTemplateSpecIfExists(self->templateSpec);
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
		WriteTemplateSpecIfExists(self->templateSpec);
		writer.WriteString(L"__forward ");
		WriteHeader(self);
		if (semicolon) writer.WriteLine(L";");
	}

	void Visit(FriendClassDeclaration* self) override
	{
		WriteTemplateSpecIfExists(self->templateSpec);
		if (self->classType)
		{
			switch (self->classType.Value())
			{
			case CppClassType::Class:
				writer.WriteString(L"friend class ");
				break;
			case CppClassType::Struct:
				writer.WriteString(L"friend struct ");
				break;
			case CppClassType::Union:
				writer.WriteString(L"friend union ");
				break;
			}
		}
		else
		{
			writer.WriteString(L"friend ");
		}
		Log(self->usedClass, writer);
		if (semicolon) writer.WriteLine(L";");
	}

	void Visit(VariableDeclaration* self)override
	{
		for (vint i = 0; i < self->classSpecs.Count(); i++)
		{
			WriteTemplateSpecIfExists(self->classSpecs[i]);
		}
		WriteHeader(self);
		if (self->initializer)
		{
			Log(self->initializer, writer);
		}
		if (semicolon) writer.WriteLine(L";");
	}

	void Visit(FunctionDeclaration* self)override
	{
		for (vint i = 0; i < self->classSpecs.Count(); i++)
		{
			WriteTemplateSpecIfExists(self->classSpecs[i]);
		}
		WriteTemplateSpecIfExists(self->templateSpec);
		WriteHeader(self);
		writer.WriteLine(L"");
		for (vint i = 0; i < self->initList.Count(); i++)
		{
			auto& item = self->initList[i];
			WriteIndentation();
			writer.WriteString(i == 0 ? L"\t: " : L"\t, ");
			writer.WriteString(item.f0->name.name);
			writer.WriteString(L"(");
			if (item.f1)
			{
				Log(item.f1, writer);
			}
			writer.WriteLine(L")");
		}
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
		WriteTemplateSpecIfExists(self->templateSpec);
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
			auto pair = self->decls[i];
			if (pair.f1->implicitlyGeneratedMember) continue;
			WriteIndentation();
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

	void Visit(NestedAnonymousClassDeclaration* self)override
	{
		switch (self->classType)
		{
		case CppClassType::Class:
			writer.WriteLine(L"class");
			break;
		case CppClassType::Struct:
			writer.WriteLine(L"struct");
			break;
		case CppClassType::Union:
			writer.WriteLine(L"union");
			break;
		}

		WriteIndentation();
		writer.WriteLine(L"{");

		indentation++;
		for (vint i = 0; i < self->decls.Count(); i++)
		{
			WriteIndentation();
			writer.WriteString(L"public ");
			self->decls[i]->Accept(this);
		}
		indentation--;

		WriteIndentation();
		writer.WriteString(L"}");
		if (semicolon) writer.WriteLine(L";");
	}

	void Visit(UsingNamespaceDeclaration* self)override
	{
		writer.WriteString(L"using namespace ");
		Log(self->ns, writer);
		if (semicolon) writer.WriteLine(L";");
	}

	void Visit(UsingSymbolDeclaration* self)override
	{
		writer.WriteString(L"using ");
		if (self->type) Log(self->type, writer);
		if (self->expr) Log(self->expr, writer);
		if (semicolon) writer.WriteLine(L";");
	}

	void Visit(TypeAliasDeclaration* self)override
	{
		WriteTemplateSpecIfExists(self->templateSpec);
		writer.WriteString(L"using_type ");
		writer.WriteString(self->name.name);
		writer.WriteString(L": ");
		Log(self->type, writer);
		if (semicolon) writer.WriteLine(L";");
	}

	void Visit(ValueAliasDeclaration* self)override
	{
		WriteTemplateSpecIfExists(self->templateSpec);
		writer.WriteString(L"using_value ");
		if (self->decoratorConstexpr) writer.WriteString(L"constexpr ");
		writer.WriteString(self->name.name);
		if (self->specializationSpec)
		{
			Log(self->specializationSpec->arguments, L"<", L">", writer);
		}
		writer.WriteString(L": ");
		Log(self->type, writer);
		writer.WriteString(L" = ");
		Log(self->expr, writer);
		if (semicolon) writer.WriteLine(L";");
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

	void Visit(StaticAssertDeclaration* self) override
	{
		writer.WriteString(L"static_assert(");
		for (vint i = 0; i < self->exprs.Count(); i++)
		{
			if (i != 0)
			{
				writer.WriteString(L", ");
			}
			Log(self->exprs[i], writer);
		}
		writer.WriteString(L")");
		if (semicolon) writer.WriteLine(L";");
	}
};

/***********************************************************************
Log
***********************************************************************/

void Log(Ptr<Declaration> decl, StreamWriter& writer, vint indentation, bool semicolon)
{
	LogDeclVisitor visitor(writer, indentation, semicolon);
	decl->Accept(&visitor);
}