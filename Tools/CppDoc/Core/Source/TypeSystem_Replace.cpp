#include "Ast_Resolving.h"
#include "EvaluateSymbol.h"
#include "Symbol_TemplateSpec.h"

/***********************************************************************
ReplaceGenericArgsInClass
***********************************************************************/

ITsys* ReplaceGenericArgsInClass(const ParsingArguments& pa, ITsys* decoratedClassType)
{
	switch (decoratedClassType->GetType())
	{
	case TsysType::LRef:
		return ReplaceGenericArgsInClass(pa, decoratedClassType->GetElement())->LRefOf();
	case TsysType::RRef:
		return ReplaceGenericArgsInClass(pa, decoratedClassType->GetElement())->RRefOf();
	case TsysType::Ptr:
		return ReplaceGenericArgsInClass(pa, decoratedClassType->GetElement())->PtrOf();
	case TsysType::CV:
		return ReplaceGenericArgsInClass(pa, decoratedClassType->GetElement())->CVOf(decoratedClassType->GetCV());
	case TsysType::Decl:
		return decoratedClassType;
	case TsysType::DeclInstant:
		{
			auto& di = decoratedClassType->GetDeclInstant();
			auto pdt = di.parentDeclType;
			auto ata = di.taContext;

			if (pdt)
			{
				pdt = ReplaceGenericArgsInClass(pa, pdt);
			}

			if (ata)
			{
				// enum could not be a template
				auto classDecl = di.declSymbol->GetAnyForwardDecl<ForwardClassDeclaration>();
				auto spec = classDecl->templateSpec;

				// check that all applied arguments are template arguments itself
				for (vint i = 0; i < spec->arguments.Count(); i++)
				{
					auto& argument = spec->arguments[i];
					if (argument.ellipsis)
					{
						if (ata->GetValue(i)->GetType() != TsysType::Any)
						{
							throw L"The applied template is not the template argument itself!";
						}
					}
					else
					{
						switch (argument.argumentType)
						{
						case CppTemplateArgumentType::HighLevelType:
							{
								auto tsys = ata->GetValue(i);
								if (tsys->GetType() != TsysType::GenericFunction)
								{
									throw L"The applied template is not the template argument itself!";
								}
								if (tsys->GetElement()->GetType() != TsysType::Any)
								{
									throw L"The applied template is not the template argument itself!";
								}
							}
							break;
						case CppTemplateArgumentType::Type:
							{
								auto tsys = ata->GetValue(i);
								if (tsys->GetType() != TsysType::GenericArg)
								{
									throw L"The applied template is not the template argument itself!";
								}
								if (tsys->GetGenericArg().argIndex != i)
								{
									throw L"The applied template is not the template argument itself!";
								}
								// skip checking declSymbolForGenericArg because
								//   during parsing a Declarator, it could be nullptr
								//   it could comes from a member defined outside of the class
								// so just skip it, assume that it is right
							}
							break;
						case CppTemplateArgumentType::Value:
							{
								if (ata->GetValue(i) != nullptr)
								{
									throw L"The applied template is not the template argument itself!";
								}
							}
							break;
						}
					}
				}

				// fill template arguments
				Array<ITsys*> params(spec->arguments.Count());
				for (vint i = 0; i < spec->arguments.Count(); i++)
				{
					auto argument = spec->arguments[i];
					auto key = symbol_type_resolving::GetTemplateArgumentKey(argument);
					ITsys* value = nullptr;
					
					if (pa.TryGetReplacedGenericArg(key, value))
					{
						params[i] = value;
					}
					else
					{
						params[i] = ata->GetValue(i);
					}
				}

				return pa.tsys->DeclInstantOf(di.declSymbol, &params, pdt);
			}
			else
			{
				return pa.tsys->DeclInstantOf(di.declSymbol, nullptr, pdt);
			}
		}
	default:
		throw L"Unexpected input type!";
	}
}

/***********************************************************************
ReplaceGenericArg
***********************************************************************/

ITsys* ReplaceGenericArg(const ParsingArguments& pa, ITsys* genericArgType)
{
	ITsys* result = nullptr;
	if (pa.TryGetReplacedGenericArg(genericArgType, result))
	{
		return result;
	}
	else
	{
		return genericArgType;
	}
}