#include <Vlpp.h>

using namespace vl;
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
	return 0;
}