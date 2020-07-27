#include "ExportReflectableTypes.h"
#include <GacUICompiler.h>

using namespace vl::collections;
using namespace vl::stream;
using namespace vl::parsing::xml;
using namespace vl::reflection::description;

namespace vl
{
	namespace reflection
	{
		namespace description
		{
			extern bool LoadGuiBasicTypes();
			extern bool LoadGuiElementTypes();
			extern bool LoadGuiCompositionTypes();
			extern bool LoadGuiEventTypes();
			extern bool LoadGuiTemplateTypes();
			extern bool LoadGuiControlTypes();
		}
	}
}

void LogTypes(Ptr<XmlElement> xmlRoot, const WString& projectName, SortedList<WString>& loaded)
{
	auto xmlProject = MakePtr<XmlElement>();
	xmlProject->name.value = L"project";
	xmlRoot->subNodes.Add(xmlProject);
	{
		auto attr = MakePtr<XmlAttribute>();
		attr->name.value = L"name";
		attr->value.value = projectName;
		xmlProject->attributes.Add(attr);
	}

	vint count = GetGlobalTypeManager()->GetTypeDescriptorCount();
	for (vint i = 0; i < count; i++)
	{
		auto td = GetGlobalTypeManager()->GetTypeDescriptor(i);
		WString tdName = td->GetTypeName();
		WString cppName = CppGetFullName(td);

		if (loaded.Contains(tdName)) continue;
		loaded.Add(tdName);

		auto xmlType = MakePtr<XmlElement>();
		xmlProject->subNodes.Add(xmlType);

		switch (td->GetTypeDescriptorFlags())
		{
		case TypeDescriptorFlags::Object:
			xmlType->name.value = L"Object";
			break;
		case TypeDescriptorFlags::IDescriptable:
			xmlType->name.value = L"IDescriptable";
			break;
		case TypeDescriptorFlags::Class:
			xmlType->name.value = L"Class";
			break;
		case TypeDescriptorFlags::Interface:
			xmlType->name.value = L"Interface";
			break;
		case TypeDescriptorFlags::Primitive:
			xmlType->name.value = L"Primitive";
			break;
		case TypeDescriptorFlags::Struct:
			xmlType->name.value = L"Struct";
			break;
		case TypeDescriptorFlags::FlagEnum:
			xmlType->name.value = L"FlagEnum";
			break;
		case TypeDescriptorFlags::NormalEnum:
			xmlType->name.value = L"NormalEnum";
			break;
		default:
			xmlType->name.value = L"Undefined";
		}

		{
			auto attr = MakePtr<XmlAttribute>();
			attr->name.value = L"name";
			attr->value.value = tdName;
			xmlType->attributes.Add(attr);
		}
		{
			auto attr = MakePtr<XmlAttribute>();
			attr->name.value = L"cpp";
			attr->value.value = cppName;
			xmlType->attributes.Add(attr);
		}
	}
}

void ExportReflectableTypes(Folder outputPath)
{
	FilePath pathReflection = outputPath.GetFilePath() / L"Reflection.xml";

	auto xmlRoot = MakePtr<XmlElement>();
	xmlRoot->name.value = L"reflection";

	auto xmlDocument = MakePtr<XmlDocument>();
	xmlDocument->rootElement = xmlRoot;

	SortedList<WString> loaded;
	{
		LoadPredefinedTypes();
		GetGlobalTypeManager()->Load();
		LogTypes(xmlRoot, L"VlppReflection", loaded);
		ResetGlobalTypeManager();
	}
	{
		LoadPredefinedTypes();
		LoadParsingTypes();
		XmlLoadTypes();
		JsonLoadTypes();
		GetGlobalTypeManager()->Load();
		LogTypes(xmlRoot, L"VlppParser", loaded);
		ResetGlobalTypeManager();
	}
	{
		LoadPredefinedTypes();
		WfLoadLibraryTypes();
		GetGlobalTypeManager()->Load();
		LogTypes(xmlRoot, L"Workflow-Runtime", loaded);
		ResetGlobalTypeManager();
	}
	{
		LoadPredefinedTypes();
		LoadParsingTypes();
		WfLoadTypes();
		GetGlobalTypeManager()->Load();
		LogTypes(xmlRoot, L"Workflow-Compiler", loaded);
		ResetGlobalTypeManager();
	}
	{
		LoadPredefinedTypes();
		LoadParsingTypes();
		XmlLoadTypes();
		JsonLoadTypes();
		WfLoadLibraryTypes();
		LoadGuiBasicTypes();
		LoadGuiElementTypes();
		LoadGuiCompositionTypes();
		LoadGuiEventTypes();
		LoadGuiTemplateTypes();
		LoadGuiControlTypes();
		GetGlobalTypeManager()->Load();
		LogTypes(xmlRoot, L"GacUI", loaded);
		ResetGlobalTypeManager();
	}
	{
		LoadPredefinedTypes();
		LoadParsingTypes();
		GuiIqLoadTypes();
		GetGlobalTypeManager()->Load();
		LogTypes(xmlRoot, L"GacUI-Compiler", loaded);
		ResetGlobalTypeManager();
	}

	{
		FileStream fileStream(pathReflection.GetFullPath(), FileStream::WriteOnly);
		Utf8Encoder encoder;
		EncoderStream encoderStream(fileStream, encoder);
		StreamWriter writer(encoderStream);
		XmlPrint(xmlDocument, writer);
	}
}

void GuiMain()
{
}