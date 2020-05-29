#include "Util.h"

void Log(Ptr<Initializer> initializer, StreamWriter& writer, vint indentation)
{
	switch (initializer->initializerType)
	{
	case CppInitializerType::Equal:
		writer.WriteString(L" = ");
		break;
	case CppInitializerType::Constructor:
		writer.WriteString(L" (");
		break;
	case CppInitializerType::Universal:
		writer.WriteString(L" {");
		break;
	}

	for (vint i = 0; i < initializer->arguments.Count(); i++)
	{
		if (i != 0)
		{
			writer.WriteString(L", ");
		}
		Log(initializer->arguments[i].item, writer, indentation);
		if (initializer->arguments[i].isVariadic)
		{
			writer.WriteString(L"...");
		}
	}

	switch (initializer->initializerType)
	{
	case CppInitializerType::Constructor:
		writer.WriteChar(L')');
		break;
	case CppInitializerType::Universal:
		writer.WriteChar(L'}');
		break;
	}
}

void Log(VariadicList<GenericArgument>& arguments, const wchar_t* open, const wchar_t* close, StreamWriter& writer, vint indentation)
{
	writer.WriteString(open);
	for (vint i = 0; i < arguments.Count(); i++)
	{
		if (i != 0)
		{
			writer.WriteString(L", ");
		}

		auto arg = arguments[i].item;
		if (arg.expr) Log(arg.expr, writer, indentation);
		if (arg.type) Log(arg.type, writer, indentation);
		if (arguments[i].isVariadic)
		{
			writer.WriteString(L"...");
		}
	}
	writer.WriteString(close);
}

/***********************************************************************
Log
***********************************************************************/

void Log(Ptr<Program> program, StreamWriter& writer)
{
	for (vint i = program->createdForwardDeclByCStyleTypeReference; i < program->decls.Count(); i++)
	{
		Log(program->decls[i], writer, 0, true);
	}
}

WString GetSymbolName(Symbol* symbol, Symbol* stopAt)
{
	WString name;
	while ((symbol != stopAt) && symbol->GetParentScope())
	{
		if (symbol->kind == symbol_component::SymbolKind::GenericTypeArgument)
		{
			name = (symbol->ellipsis ? L"::[..." : L"::[") + symbol->name + L"]" + name;
		}
		else
		{
			name = L"::" + symbol->name + name;
		}
		symbol = symbol->GetParentScope();
	}
	return name;
}

void Log(ITsys* tsys, StreamWriter& writer)
{
	if (!tsys)
	{
		writer.WriteString(L"*");
		return;
	}
	switch (tsys->GetType())
	{
	case TsysType::Any:
		writer.WriteString(L"any_t");
		return;
	case TsysType::Zero:
		writer.WriteString(L"0");
		return;
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
		{
			auto func = tsys->GetFunc();
			Log(tsys->GetElement(), writer);

			switch (func.callingConvention)
			{
			case TsysCallingConvention::CDecl:
				writer.WriteString(L" __cdecl(");
				break;
			case TsysCallingConvention::ClrCall:
				writer.WriteString(L" __clrcall(");
				break;
			case TsysCallingConvention::StdCall:
				writer.WriteString(L" __stdcall(");
				break;
			case TsysCallingConvention::FastCall:
				writer.WriteString(L" __fastcall(");
				break;
			case TsysCallingConvention::ThisCall:
				writer.WriteString(L" __thiscall(");
				break;
			case TsysCallingConvention::VectorCall:
				writer.WriteString(L" __vectorcall(");
				break;
			default:
				writer.WriteString(L" (");
			}
			for (vint i = 0; i < tsys->GetParamCount(); i++)
			{
				if (i > 0) writer.WriteString(L", ");
				Log(tsys->GetParam(i), writer);
			}

			if (func.ellipsis)
			{
				if (tsys->GetParamCount() > 0) writer.WriteString(L", ");
				writer.WriteString(L"...");
			}
			writer.WriteChar(L')');
		}
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
			if (cv.isGeneralConst) writer.WriteString(L" const");
			if (cv.isVolatile) writer.WriteString(L" volatile");
		}
		return;
	case TsysType::Decl:
		{
			writer.WriteString(GetSymbolName(tsys->GetDecl(), nullptr));
		}
		return;
	case TsysType::DeclInstant:
		{
			const auto& di = tsys->GetDeclInstant();
			if (auto parent = di.parentDeclType)
			{
				Log(parent, writer);
			}
			
			writer.WriteString(GetSymbolName(di.declSymbol, (di.parentDeclType ? di.parentDeclType->GetDecl() : nullptr)));
			if (tsys->GetParamCount())
			{
				writer.WriteString(L"<");
				for (vint i = 0; i < tsys->GetParamCount(); i++)
				{
					if (i > 0) writer.WriteString(L", ");
					Log(tsys->GetParam(i), writer);
				}
				writer.WriteString(L">");
			}
		}
		return;
	case TsysType::Init:
		{
			writer.WriteChar(L'{');
			for (vint i = 0; i < tsys->GetParamCount(); i++)
			{
				if (i > 0) writer.WriteString(L", ");
				Log(tsys->GetParam(i), writer);
				switch (tsys->GetInit().headers[i].type)
				{
				case ExprTsysType::LValue:
					writer.WriteString(L" $L");
					break;
				case ExprTsysType::PRValue:
					writer.WriteString(L" $PR");
					break;
				case ExprTsysType::XValue:
					writer.WriteString(L" $X");
					break;
				}
			}
			writer.WriteChar(L'}');
		}
		return;
	case TsysType::GenericFunction:
		{
			writer.WriteChar(L'<');
			auto genericFunction = tsys->GetGenericFunction();
			for (vint i = 0; i < tsys->GetParamCount(); i++)
			{
				if (i > 0) writer.WriteString(L", ");
				if (genericFunction.spec->arguments[i].ellipsis)
				{
					writer.WriteString(L"...");
				}
				if (i < genericFunction.filledArguments)
				{
					writer.WriteChar(L'=');
				}
				if (genericFunction.spec->arguments[i].argumentType != CppTemplateArgumentType::Value)
				{
					Log(tsys->GetParam(i), writer);
				}
				else
				{
					writer.WriteChar(L'*');
				}
			}
			writer.WriteString(L"> ");
			Log(tsys->GetElement(), writer);
		}
		return;
	case TsysType::GenericArg:
		{
			auto genericArg = tsys->GetGenericArg();
			writer.WriteString(GetSymbolName(genericArg.argSymbol->GetParentScope(), nullptr));
			writer.WriteString(L"::[");
			writer.WriteString(genericArg.argSymbol->name);
			writer.WriteChar(L']');
		}
		return;
	}
	throw L"Invalid!";
}