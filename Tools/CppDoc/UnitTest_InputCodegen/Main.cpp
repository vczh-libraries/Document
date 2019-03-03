#include <Vlpp.h>

using namespace vl;
using namespace vl::collections;
using namespace vl::filesystem;
using namespace vl::stream;

namespace operator_overloading
{
	struct Op
	{
		const wchar_t* name;
		bool parameter;

		Op() = default;
		Op(const wchar_t* _name, bool _parameter) :name(_name), parameter(_parameter) {}
	};

	Op ops1[] = {
		{L"++",		true},
		{L"--",		true},
		{L"++",		false},
		{L"--",		false},
		{L"~",		false},
		{L"!",		false},
		{L"-",		false},
		{L"+",		false},
		{L"&",		false},
		{L"*",		false},
	};

	Op ops2[] = {
		{L"*",		true},
		{L"/",		true},
		{L"%",		true},
		{L"+",		true},
		{L"-",		true},
		{L"<<",		true},
		{L">>",		true},
		{L"==",		true},
		{L"!=",		true},
		{L"<",		true},
		{L"<=",		true},
		{L">",		true},
		{L">=",		true},
		{L"&",		true},
		{L"|",		true},
		{L"^",		true},
		{L"&&",		true},
		{L"||",		true},
		{L"=",		true},
		{L"*=",		true},
		{L"/=",		true},
		{L"%=",		true},
		{L"+=",		true},
		{L"-=",		true},
		{L"<<=",	true},
		{L">>=",	true},
		{L"&=",		true},
		{L"|=",		true},
		{L"^=",		true},
		{L",",		true},
	};

	Op ops[(sizeof(ops1) + sizeof(ops2)) / sizeof(Op)];

	void GenerateInput1()
	{
		FilePath path = L"../UnitTest/TestOverloadingOperator_Input1.h";
		FileStream fileStream(path.GetFullPath(), FileStream::WriteOnly);
		Utf8Encoder encoder;
		EncoderStream encoderStream(fileStream, encoder);
		StreamWriter writer(encoderStream);

		writer.WriteLine(L"namespace test_overloading");
		writer.WriteLine(L"{");
		writer.WriteLine(L"\tstruct X");
		writer.WriteLine(L"\t{");
		for (auto op : ops)
		{
			writer.WriteString(L"\t\tvoid* operator");
			writer.WriteString(op.name);
			writer.WriteString(op.name[1] ? L"\t" : L"\t\t");
			writer.WriteLine(op.parameter ? L"(int);" : L"();");
		}
		writer.WriteLine(L"");
		for (auto op : ops)
		{
			writer.WriteString(L"\t\tbool* operator");
			writer.WriteString(op.name);
			writer.WriteString(op.name[1] ? L"\t" : L"\t\t");
			writer.WriteLine(op.parameter ? L"(int)const;" : L"()const;");
		}
		writer.WriteLine(L"\t};");
		writer.WriteLine(L"}");
	}

	void GenerateInput2()
	{
		FilePath path = L"../UnitTest/TestOverloadingOperator_Input2.h";
		FileStream fileStream(path.GetFullPath(), FileStream::WriteOnly);
		Utf8Encoder encoder;
		EncoderStream encoderStream(fileStream, encoder);
		StreamWriter writer(encoderStream);

		writer.WriteLine(L"namespace test_overloading");
		writer.WriteLine(L"{");
		writer.WriteLine(L"\tstruct Y");
		writer.WriteLine(L"\t{");
		writer.WriteLine(L"\t};");
		for (auto op : ops)
		{
			if (wcscmp(op.name, L"=") == 0) continue;
			writer.WriteString(L"\tvoid* operator");
			writer.WriteString(op.name);
			writer.WriteString(op.name[1] ? L"\t\t" : L"\t\t\t");
			writer.WriteLine(op.parameter ? L"(Y&, int);" : L"(Y&);");
		}
		for (auto op : ops2)
		{
			if (wcscmp(op.name, L"=") == 0) continue;
			writer.WriteString(L"\tvoid* operator");
			writer.WriteString(op.name);
			writer.WriteString(op.name[1] ? L"\t\t" : L"\t\t\t");
			writer.WriteLine(L"(int, Y&);");
		}
		writer.WriteLine(L"");
		for (auto op : ops)
		{
			if (wcscmp(op.name, L"=") == 0) continue;
			writer.WriteString(L"\tbool* operator");
			writer.WriteString(op.name);
			writer.WriteString(op.name[1] ? L"\t\t" : L"\t\t\t");
			writer.WriteLine(op.parameter ? L"(const Y&, int);" : L"(const Y&);");
		}
		for (auto op : ops2)
		{
			if (wcscmp(op.name, L"=") == 0) continue;
			writer.WriteString(L"\tbool* operator");
			writer.WriteString(op.name);
			writer.WriteString(op.name[1] ? L"\t\t" : L"\t\t\t");
			writer.WriteLine(L"(int, const Y&);");
		}
		writer.WriteLine(L"}");
	}

	void GenerateInput3()
	{
		FilePath path = L"../UnitTest/TestOverloadingOperator_Input3.h";
		FileStream fileStream(path.GetFullPath(), FileStream::WriteOnly);
		Utf8Encoder encoder;
		EncoderStream encoderStream(fileStream, encoder);
		StreamWriter writer(encoderStream);

		writer.WriteLine(L"namespace test_overloading");
		writer.WriteLine(L"{");
		writer.WriteLine(L"\tstruct Z");
		writer.WriteLine(L"\t{");
		writer.WriteLine(L"\t};");
		writer.WriteLine(L"}");
		for (auto op : ops)
		{
			if (wcscmp(op.name, L"=") == 0) continue;
			writer.WriteString(L"void* operator");
			writer.WriteString(op.name);
			writer.WriteString(op.name[1] ? L"\t\t\t" : L"\t\t\t\t");
			writer.WriteLine(op.parameter ? L"(test_overloading::Z&, int);" : L"(test_overloading::Z&);");
		}
		for (auto op : ops2)
		{
			if (wcscmp(op.name, L"=") == 0) continue;
			writer.WriteString(L"void* operator");
			writer.WriteString(op.name);
			writer.WriteString(op.name[1] ? L"\t\t\t" : L"\t\t\t\t");
			writer.WriteLine(L"(int, test_overloading::Z&);");
		}
		writer.WriteLine(L"");
		for (auto op : ops)
		{
			if (wcscmp(op.name, L"=") == 0) continue;
			writer.WriteString(L"bool* operator");
			writer.WriteString(op.name);
			writer.WriteString(op.name[1] ? L"\t\t\t" : L"\t\t\t\t");
			writer.WriteLine(op.parameter ? L"(const test_overloading::Z&, int);" : L"(const test_overloading::Z&);");
		}
		for (auto op : ops2)
		{
			if (wcscmp(op.name, L"=") == 0) continue;
			writer.WriteString(L"bool* operator");
			writer.WriteString(op.name);
			writer.WriteString(op.name[1] ? L"\t\t\t" : L"\t\t\t\t");
			writer.WriteLine(L"(int, const test_overloading::Z&);");
		}
	}
}

namespace generated_functions
{
	void GenerateInput()
	{
		List<WString> classNames;
		{
			FilePath path = L"../UnitTest/TestGeneratedFunctions_Input.h";
			FileStream fileStream(path.GetFullPath(), FileStream::WriteOnly);
			Utf8Encoder encoder;
			EncoderStream encoderStream(fileStream, encoder);
			StreamWriter writer(encoderStream);

			auto Write = [&](const wchar_t* input, const wchar_t* name, const wchar_t* keyword)
			{
				while (true)
				{
					auto pName = wcsstr(input, L"NAME");
					auto pKeyword = wcsstr(input, L"KEYWORD");
					vint skip = 0;
					const wchar_t* search = nullptr;
					const wchar_t* replace = nullptr;

					if (pName && pKeyword)
					{
						if (pName < pKeyword)
						{
							skip = 4;
							search = pName;
							replace = name;
						}
						else
						{
							skip = 7;
							search = pKeyword;
							replace = keyword;
						}
					}
					else if (pName)
					{
						skip = 4;
						search = pName;
						replace = name;
					}
					else if (pKeyword)
					{
						skip = 7;
						search = pKeyword;
						replace = keyword;
					}

					if (search)
					{
						writer.WriteString(WString(input, false).Left((vint)(search - input)));
						writer.WriteString(replace);
						input = search + skip;
					}
					else
					{
						writer.WriteString(input);
						return;
					}
				}
			};

			auto dDC = [&](const WString& name, const wchar_t* keyword)
			{
				writer.WriteString(L"\t\t");
				Write(L"NAME()=KEYWORD;", name.Buffer(), keyword);
				writer.WriteLine(L"");
			};

			auto dCC = [&](const WString& name, const wchar_t* keyword)
			{
				writer.WriteString(L"\t\t");
				Write(L"NAME(const NAME&)=KEYWORD;", name.Buffer(), keyword);
				writer.WriteLine(L"");
			};

			auto dMC = [&](const WString& name, const wchar_t* keyword)
			{
				writer.WriteString(L"\t\t");
				Write(L"NAME(NAME&&)=KEYWORD;", name.Buffer(), keyword);
				writer.WriteLine(L"");
			};

			auto dCA = [&](const WString& name, const wchar_t* keyword)
			{
				writer.WriteString(L"\t\t");
				Write(L"NAME&operator=(const NAME&)=KEYWORD;", name.Buffer(), keyword);
				writer.WriteLine(L"");
			};

			auto dMA = [&](const WString& name, const wchar_t* keyword)
			{
				writer.WriteString(L"\t\t");
				Write(L"NAME&operator=(NAME&&)=KEYWORD;", name.Buffer(), keyword);
				writer.WriteLine(L"");
			};

			auto dDD = [&](const WString& name, const wchar_t* keyword)
			{
				writer.WriteString(L"\t\t");
				Write(L"~NAME()=KEYWORD;", name.Buffer(), keyword);
				writer.WriteLine(L"");
			};

			writer.WriteLine(L"namespace test_generated_functions");
			writer.WriteLine(L"{");

			for (vint i = 0; i < 3 * 3 * 3 * 3 * 3 * 3; i++)
			{
				vint fDC = i % 3;
				vint fCC = (i / 3) % 3;
				vint fMC = (i / (3 * 3)) % 3;
				vint fCA = (i / (3 * 3 * 3)) % 3;
				vint fMA = (i / (3 * 3 * 3 * 3)) % 3;
				vint fDD = (i / (3 * 3 * 3 * 3 * 3)) % 3;

				WString className;
				className += (fDC == 0 ? L"oDC_" : fDC == 2 ? L"xDC_" : L"nDC_");
				className += (fCC == 0 ? L"oCC_" : fCC == 2 ? L"xCC_" : L"nCC_");
				className += (fMC == 0 ? L"oMC_" : fMC == 2 ? L"xMC_" : L"nMC_");
				className += (fCA == 0 ? L"oCA_" : fCA == 2 ? L"xCA_" : L"nCA_");
				className += (fMA == 0 ? L"oMA_" : fMA == 2 ? L"xMA_" : L"nMA_");
				className += (fDD == 0 ? L"oDD_" : fDD == 2 ? L"xDD_" : L"nDD_");
				classNames.Add(className);

				writer.WriteString(L"\tstruct ");
				writer.WriteLine(className);
				writer.WriteLine(L"\t{");
				if (fDC == 0) dDC(className, L"default"); else if (fDC == 2) dDC(className, L"delete");
				if (fCC == 0) dCC(className, L"default"); else if (fCC == 2) dCC(className, L"delete");
				if (fMC == 0) dMC(className, L"default"); else if (fMC == 2) dMC(className, L"delete");
				if (fCA == 0) dCA(className, L"default"); else if (fCA == 2) dCA(className, L"delete");
				if (fMA == 0) dMA(className, L"default"); else if (fMA == 2) dMA(className, L"delete");
				if (fDD == 0) dDD(className, L"default"); else if (fDD == 2) dDD(className, L"delete");
				writer.WriteLine(L"\t};");
			}

			writer.WriteLine(L"}");
			writer.WriteLine(L"using namespace test_generated_functions;");
		}
		{
			FilePath path = L"../UnitTest/TestGeneratedFunctions_Macro.h";
			FileStream fileStream(path.GetFullPath(), FileStream::WriteOnly);
			Utf8Encoder encoder;
			EncoderStream encoderStream(fileStream, encoder);
			StreamWriter writer(encoderStream);
			writer.WriteLine(L"#define GENERATED_FUNCTION_TYPES(F)\\");
			FOREACH(WString, className, classNames)
			{
				writer.WriteString(L"\tF(");
				writer.WriteString(className);
				writer.WriteLine(L")\\");
			}
			writer.WriteLine(L"");
		}
	}
}

int main()
{
	{
		using namespace operator_overloading;

		memcpy(&ops[0], &ops1[0], sizeof(ops1));
		memcpy(&ops[sizeof(ops1) / sizeof(Op)], &ops2[0], sizeof(ops2));
		GenerateInput1();
		GenerateInput2();
		GenerateInput3();
	}
	{
		using namespace generated_functions;
		GenerateInput();
	}
	return 0;
}