#include "Utility.h"
#include <Windows.h>

wchar_t* ReadBigFile(const FilePath& filePath)
{
	HANDLE handle = CreateFile(filePath.GetFullPath().Buffer(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, NULL, NULL);
	TEST_ASSERT(handle != INVALID_HANDLE_VALUE);

	DWORD fileSize = GetFileSize(handle, NULL);
	TEST_ASSERT(fileSize != INVALID_FILE_SIZE);

	char* utf8 = new char[fileSize + 1];
	DWORD read = 0;
	TEST_ASSERT(ReadFile(handle, utf8, fileSize, &read, NULL) == TRUE);
	TEST_ASSERT(read == fileSize);
	CloseHandle(handle);
	utf8[fileSize] = 0;

	int bufferSize = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, utf8, -1, NULL, 0);
	TEST_ASSERT(bufferSize > 0);
	auto buffer = new wchar_t[bufferSize + 1];
	MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, utf8, -1, buffer, bufferSize + 1);

	delete[] utf8;
	return buffer;
}