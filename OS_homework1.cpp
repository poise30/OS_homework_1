#include <windows.h>
#include <Strsafe.h>
#include <stdlib.h>
#include <memory>
#include <stdio.h>
#include <malloc.h>
#include <crtdbg.h>

bool is_file_existsW(_In_ const wchar_t* file_path)
{
	_ASSERTE(NULL != file_path);
	_ASSERTE(TRUE != IsBadStringPtrW(file_path, MAX_PATH));
	if ((NULL == file_path) || (TRUE == IsBadStringPtrW(file_path, MAX_PATH))) return false;

	WIN32_FILE_ATTRIBUTE_DATA info = { 0 };

	if (GetFileAttributesExW(file_path, GetFileExInfoStandard, &info) == 0)
		return false;
	else
		return true;
}

//err, ���ڿ�, �����μ�
void print(_In_ const char* fmt, _In_ ...)
{	//_ln_ �б�����?


	char log_buffer[2048];
	va_list args; //�����μ�

	va_start(args, fmt); //args �� fmt ���·� �о����
	HRESULT hRes = StringCbVPrintfA(log_buffer, sizeof(log_buffer), fmt, args);
	if (S_OK != hRes) //S_OK �Լ��� �����Ͽ����� �ǹ�
	{
		//���� ���
		fprintf(
			stderr,
			"%s, StringCbVPrintfA() failed. res = 0x%08x",
			__FUNCTION__,
			hRes
			);
		return;
	}

	OutputDebugStringA(log_buffer); //������Ҷ� ���
	fprintf(stdout, "%s \n", log_buffer);
}


/**----------------------------------------------------------------------------
* StopWatch.h
*-----------------------------------------------------------------------------
*
*-----------------------------------------------------------------------------
* All rights reserved by somma (fixbrain@gmail.com, unsorted@msn.com)
*-----------------------------------------------------------------------------
* 26:12:2011   17:53 created
* - OpenMP ���� ���α׷��� by ������ (CStopWatch class)
**---------------------------------------------------------------------------*/
#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

class StopWatch
{
private:
	LARGE_INTEGER   mFreq, mStart, mEnd;
	float                   mTimeforDuration;
public:
	StopWatch() : mTimeforDuration(0)
	{
		mFreq.LowPart = mFreq.HighPart = 0;
		mStart = mFreq;
		mEnd = mFreq;
		QueryPerformanceFrequency(&mFreq);
	}
	~StopWatch()
	{
	}

public:
	void Start(){ QueryPerformanceCounter(&mStart); }
	void Stop()
	{
		QueryPerformanceCounter(&mEnd);
		mTimeforDuration = (mEnd.QuadPart - mStart.QuadPart) / (float)mFreq.QuadPart;
	}
	float GetDurationSecond() { return mTimeforDuration; }
	float GetDurationMilliSecond() { return mTimeforDuration * 1000.f; }

};

/**
* @brief
* @param
* @see
* @remarks
* @code
* @endcode
* @return
**/
bool read_file_using_memory_map()
{
	// current directory �� ���Ѵ�.
	wchar_t *buf = NULL;
	uint32_t buflen = 0;
	buflen = GetCurrentDirectoryW(buflen, buf);
	if (0 == buflen)
	{
		print("err ] GetCurrentDirectoryW() failed. gle = 0x%08x", GetLastError());
		return false;
	}

	buf = (PWSTR)malloc(sizeof(WCHAR) * buflen);
	if (0 == GetCurrentDirectoryW(buflen, buf))
	{
		print("err ] GetCurrentDirectoryW() failed. gle = 0x%08x", GetLastError());
		free(buf);
		return false;
	}

	// current dir \\ test.txt ���ϸ� ����
	wchar_t file_name[260];
	if (!SUCCEEDED(StringCbPrintfW(
		file_name,
		sizeof(file_name),
		L"%ws\\test.txt",
		buf)))
	{
		print("err ] can not create file name");
		free(buf);
		return false;
	}
	free(buf); buf = NULL;

	if (true != is_file_existsW(file_name))
	{
		print("err ] no file exists. file = %ws", file_name);
		return false;
	}

	//���Ͽ���
	HANDLE file_handle = CreateFileW(
		(LPCWSTR)file_name,
		GENERIC_READ,
		NULL,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL
		);
	if (INVALID_HANDLE_VALUE == file_handle)
	{
		print("err ] CreateFile(%ws) failed, gle = %u", file_name, GetLastError());
		return false;
	}

	// check file size
	//
	LARGE_INTEGER fileSize;
	if (TRUE != GetFileSizeEx(file_handle, &fileSize))
	{
		print("err ] GetFileSizeEx(%ws) failed, gle = %u", file_name, GetLastError());
		CloseHandle(file_handle);
		return false;
	}

	// [ WARN ]
	//
	// 4Gb �̻��� ������ ��� MapViewOfFile()���� ������ ���ų�
	// ���� ������ �̵��� ������ ��
	// FilIoHelperClass ����� �̿��ؾ� ��
	//
	_ASSERTE(fileSize.HighPart == 0);
	if (fileSize.HighPart > 0)
	{
		print("file size = %I64d (over 4GB) can not handle. use FileIoHelperClass",
			fileSize.QuadPart);
		CloseHandle(file_handle);
		return false;
	}

	DWORD file_size = (DWORD)fileSize.QuadPart;
	HANDLE file_map = CreateFileMapping(
		file_handle,
		NULL,
		PAGE_READONLY,
		0,
		0,
		NULL
		);
	if (NULL == file_map)
	{
		print("err ] CreateFileMapping(%ws) failed, gle = %u", file_name, GetLastError());
		CloseHandle(file_handle);
		return false;
	}

	PCHAR file_view = (PCHAR)MapViewOfFile(
		file_map,
		FILE_MAP_READ,
		0,
		0,
		0
		);
	if (file_view == NULL)
	{
		print("err ] MapViewOfFile(%ws) failed, gle = %u", file_name, GetLastError());

		CloseHandle(file_map);
		CloseHandle(file_handle);
		return false;
	}

	// do some io
	char a = file_view[0];  // 0x d9
	char b = file_view[1];  // 0xb3



	// close all
	UnmapViewOfFile(file_view);
	CloseHandle(file_map);
	CloseHandle(file_handle);
	return true;
}


/**
* @brief
* @param
* @see
* @remarks
* @code
* @endcode
* @return
**/
//���ϰ��, ����ũ��
bool create_very_big_file(_In_ const wchar_t* file_path, _In_ uint32_t size_in_mb)
{
	_ASSERTE(NULL != file_path);
	if (NULL == file_path) return false;

	if (is_file_existsW(file_path))
	{
		::DeleteFileW(file_path);
	}

	// create very big file
	HANDLE file_handle = CreateFile(
		file_path,
		GENERIC_WRITE,
		FILE_SHARE_READ, //�ٸ� �����忡�� READ ���� �����ϴ°� ���� , NULL�� ���� ������ �ٸ� ������ ������
		NULL,
		CREATE_NEW,
		FILE_ATTRIBUTE_NORMAL,
		NULL
		);
	if (INVALID_HANDLE_VALUE == file_handle)
	{
		print("err ] CreateFile( %ws ) failed. gle = %u", file_path, GetLastError());
		return false;
	}
	//������ 4G�� �Ѿ�� ���� ��, 4����Ʈ����? 32bit���� 64bit ����� ����(4G) �Ѿ��..��Ʈ�� c++ stl? item�� ���������� ���̸� ���Ҷ�
	//4byte integer ���, 32bit�ӽſ��� 64bit�����͸� �ٷ궩 �� ��ٷο�.
		LARGE_INTEGER file_size = { 0 };
	//file_size.LowPart = 0;
	//file_size.HighPart = 1;
	file_size.QuadPart = (1024 * 1024) * size_in_mb;

	//1GB ��ŭ ������ pointer�̵�	
	if (!SetFilePointerEx(file_handle, file_size, NULL, FILE_BEGIN))
	{
		print("err ] SetFilePointerEx() failed. gle = %u", GetLastError());

		CloseHandle(file_handle);
		return false;
	}
	//ȣ�� �ϸ� ���ϻ����Ϸ�
	SetEndOfFile(file_handle);
	CloseHandle(file_handle);
	return true;
}


/**
* @brief
* @param
* @see
* @remarks
* @code
* @endcode
* @return
**/
typedef struct map_context
{
	HANDLE  handle;
	DWORD   size;
	HANDLE  map;
	PCHAR   view;
}*pmap_context;

pmap_context open_map_context(_In_ const wchar_t* file_path)
{
	_ASSERTE(NULL != file_path);
	if (NULL == file_path) return false;
	if (!is_file_existsW(file_path)) return false;;

	pmap_context ctx = (pmap_context)malloc(sizeof(map_context));
	RtlZeroMemory(ctx, sizeof(map_context));

	bool ret = false;

#pragma warning(disable: 4127)
	do
	{
		ctx->handle = CreateFileW(
			(LPCWSTR)file_path,
			GENERIC_READ,
			NULL,
			NULL,
			OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL,
			NULL
			);
		if (INVALID_HANDLE_VALUE == ctx->handle)
		{
			print("err ] CreateFile( %ws ) failed. gle = %u", file_path, GetLastError());
			break;
		}

		// check file size
		//
		LARGE_INTEGER fileSize;
		if (TRUE != GetFileSizeEx(ctx->handle, &fileSize))
		{
			print("err ] GetFileSizeEx( %ws ) failed. gle = %u", file_path, GetLastError());
			break;
		}

		// [ WARN ]
		//
		// 4Gb �̻��� ������ ��� MapViewOfFile()���� ������ ���ų�
		// ���� ������ �̵��� ������ ��
		// FilIoHelperClass ����� �̿��ؾ� ��
		//
		_ASSERTE(fileSize.HighPart == 0);
		if (fileSize.HighPart > 0)
		{
			print("err ] file is too large to map. file = %ws, size = %llu", file_path, fileSize.QuadPart);
			break;
		}

		ctx->size = (DWORD)fileSize.QuadPart;
		ctx->map = CreateFileMapping(
			ctx->handle,
			NULL,
			PAGE_READONLY,
			0,
			0,
			NULL
			);
		if (NULL == ctx->map)
		{
			print("err ] CreateFileMapping( %ws ) failed. gle = %u", file_path, GetLastError());
			break;
		}

		ctx->view = (PCHAR)MapViewOfFile(
			ctx->map,
			FILE_MAP_READ,
			0,
			0,
			0
			);
		if (ctx->view == NULL)
		{
			print("err ] MapViewOfFile( %ws ) failed. gle = %u", file_path, GetLastError());
			break;
		}

		ret = true;
	} while (FALSE);
#pragma warning(default: 4127)

	if (!ret)
	{
		if (NULL != ctx->view) UnmapViewOfFile(ctx->view);
		if (NULL != ctx->map) CloseHandle(ctx->map);
		if (INVALID_HANDLE_VALUE != ctx->handle) CloseHandle(ctx->handle);

		free(ctx); ctx = NULL;
	}

	return ctx;
}

/**
* @brief
* @param
* @see
* @remarks
* @code
* @endcode
* @return
**/
pmap_context create_map_context(_In_ const wchar_t* file_path, _In_ uint32_t file_size)
{
	_ASSERTE(NULL != file_path);
	if (NULL == file_path) return false;
	if (is_file_existsW(file_path))
	{
		DeleteFileW(file_path);
	}

	pmap_context ctx = (pmap_context)malloc(sizeof(map_context));
	RtlZeroMemory(ctx, sizeof(map_context));

	bool ret = false;

#pragma warning(disable: 4127)
	do
	{
		ctx->handle = CreateFileW(
			(LPCWSTR)file_path,
			GENERIC_READ | GENERIC_WRITE,
			NULL,
			NULL,
			CREATE_NEW,
			FILE_ATTRIBUTE_NORMAL,
			NULL
			);
		if (INVALID_HANDLE_VALUE == ctx->handle)
		{
			print("err ] CreateFile( %ws ) failed. gle = %u", file_path, GetLastError());
			break;
		}

		ctx->size = file_size;
		ctx->map = CreateFileMapping(
			ctx->handle,
			NULL,
			PAGE_READWRITE,
			0,
			ctx->size,
			NULL
			);
		if (NULL == ctx->map)
		{
			print("err ] CreateFileMapping( %ws ) failed. gle = %u", file_path, GetLastError());
			break;
		}

		ctx->view = (PCHAR)MapViewOfFile(
			ctx->map,
			FILE_MAP_WRITE,
			0,
			0,
			ctx->size
			);
		if (ctx->view == NULL)
		{
			print("err ] MapViewOfFile( %ws ) failed. gle = %u", file_path, GetLastError());
			break;
		}

		ret = true;
	} while (FALSE);
#pragma warning(default: 4127)

	if (!ret)
	{
		if (NULL != ctx->view) UnmapViewOfFile(ctx->view);
		if (NULL != ctx->map) CloseHandle(ctx->map);
		if (INVALID_HANDLE_VALUE != ctx->handle) CloseHandle(ctx->handle);

		free(ctx); ctx = NULL;
	}

	return ctx;
}

/**
* @brief
* @param
* @see
* @remarks
* @code
* @endcode
* @return
**/
void close_map_context(_In_ pmap_context ctx)
{
	if (NULL != ctx)
	{
		if (NULL != ctx->view) UnmapViewOfFile(ctx->view);
		if (NULL != ctx->map) CloseHandle(ctx->map);
		if (INVALID_HANDLE_VALUE != ctx->handle) CloseHandle(ctx->handle);
		free(ctx);
	}
}


/**
* @brief
* @param
* @see
* @remarks
* @code
* @endcode
* @return
**/
bool
file_copy_using_memory_map(
_In_ const wchar_t* src_file,
_In_ const wchar_t* dst_file
)
{
	_ASSERTE(NULL != src_file);
	_ASSERTE(NULL != dst_file);
	if (NULL == src_file || NULL == dst_file) return false;

	if (!is_file_existsW(src_file))
	{
		print("err ] no src file = %ws", src_file);
		return false;
	}

	if (is_file_existsW(dst_file))
	{
		DeleteFileW(dst_file);
	}

	// map src, dst file
	pmap_context src_ctx = open_map_context(src_file);
	pmap_context dst_ctx = create_map_context(dst_file, src_ctx->size);
	if (NULL == src_ctx || NULL == dst_ctx)
	{
		print("err ] open_map_context() failed.");
		close_map_context(src_ctx);
		close_map_context(dst_ctx);
		return false;
	}

	// copy src to dst by mmio
	for (uint32_t i = 0; i < src_ctx->size; ++i)
	{
		dst_ctx->view[i] = src_ctx->view[i];
	}

	return true;
}

/**
* @brief
* @param
* @see
* @remarks
* @code
* @endcode
* @return
**/
bool
file_copy_using_read_write(
_In_ const wchar_t* src_file,
_In_ const wchar_t* dst_file
)
{
	_ASSERTE(NULL != src_file);
	_ASSERTE(NULL != dst_file);
	if (NULL == src_file || NULL == dst_file) return false;

	if (!is_file_existsW(src_file))
	{
		print("err ] no src file = %ws", src_file);
		return false;
	}

	if (is_file_existsW(dst_file))
	{
		DeleteFileW(dst_file);
	}

	// open src file with READ mode
	HANDLE src_handle = CreateFileW(
		src_file,
		GENERIC_READ,
		FILE_SHARE_READ,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL
		);
	if (INVALID_HANDLE_VALUE == src_handle)
	{
		print("err ] CreateFile( %ws ) failed. gle = %u", src_file, GetLastError());
		return false;
	}

	// open dst file with WRITE mode
	HANDLE dst_handle = CreateFileW(
		dst_file,
		GENERIC_WRITE,
		FILE_SHARE_READ,
		NULL,
		CREATE_NEW,
		FILE_ATTRIBUTE_NORMAL,
		NULL
		);
	if (INVALID_HANDLE_VALUE == dst_handle)
	{
		print("err ] CreateFile( %ws ) failed. gle = %u", dst_file, GetLastError());

		CloseHandle(src_handle);
		return false;
	}

	// file copy
	bool ret = false;
	char buf[4096] = { 0 };
	DWORD bytes_written = 0;
	DWORD bytes_read = 0;

	do
	{
		// read from src ���������� ������ ���� ������ READ FILE ������ ������
		if (!ReadFile(src_handle, buf, sizeof(buf), &bytes_read, NULL))
		{
			print("err ] ReadFile( src_handle ) failed. gle = %u", GetLastError());
			break;
		}
		else
		{
			// please read
			// .https://msdn.microsoft.com/en-us/library/windows/desktop/aa365690(v=vs.85).aspx
			if (0 == bytes_read) //������ ���� ����Ʈ�� ���� 0�̸� ���̴�.
			{
				ret = true;
				break;
			}
		}

		// write to dst
		if (!WriteFile(dst_handle, buf, sizeof(buf), &bytes_written, NULL))
		{
			print("err ] WriteFile( dst_handle ) failed. gle = %u", GetLastError());
			break;
		}
	} while (true);


	CloseHandle(src_handle);
	CloseHandle(dst_handle);
	return ret;
}



/**
* @brief
* @param
* @see
* @remarks
* @code
* @endcode
* @return
**/


char* WcsToMbsUTF8(_In_ const wchar_t* wcs)
{
	_ASSERTE(NULL != wcs);
	if (NULL == wcs) return NULL;

	int outLen = WideCharToMultiByte(CP_UTF8, 0, wcs, -1, NULL, 0, NULL, NULL);
	if (0 == outLen) return NULL;

	char* outChar = (char*)malloc(outLen * sizeof(char));
	if (NULL == outChar) return NULL;
	RtlZeroMemory(outChar, outLen);

	if (0 == WideCharToMultiByte(CP_UTF8, 0, wcs, -1, outChar, outLen, NULL, NULL))
	{
		print("err, WideCharToMultiByte() failed, errcode=0x%08x", GetLastError());

		free(outChar);
		return NULL;
	}

	return outChar;
}

wchar_t* Utf8MbsToWcs(_In_ const char* utf8)
{
	_ASSERTE(NULL != utf8);
	if (NULL == utf8) return NULL;

	int outLen = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, utf8, -1, NULL, 0);
	if (0 == outLen) return NULL;

	wchar_t* outWchar = (wchar_t*)malloc(outLen * (sizeof(wchar_t)));  // outLen contains NULL char 
	if (NULL == outWchar) return NULL;
	RtlZeroMemory(outWchar, outLen);

	if (0 == MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, utf8, -1, outWchar, outLen))
	{
		print("err , MultiByteToWideChar() failed, errcode=0x%08x", GetLastError());

		free(outWchar);
		return NULL;
	}

	return outWchar;
}

bool create_bob_txt()
{
	// current directory �� ���Ѵ�.
	wchar_t *buf = NULL;
	uint32_t buflen = 0;
	//buf �� ���丮 ��θ� ������
	//���ۿ� ������ ���̸� ����
	buflen = GetCurrentDirectoryW(buflen, buf);
	if (0 == buflen)
	{
		print("err, GetCurrentDirectoryW() failed. gle = 0x%08x", GetLastError());
		return false;
	}
	//PWSTR -> Pointer to wide string
	buf = (PWSTR)malloc(sizeof(WCHAR) * buflen);
	if (0 == GetCurrentDirectoryW(buflen, buf))
	{
		print("err, GetCurrentDirectoryW() failed. gle = 0x%08x", GetLastError());
		free(buf);
		return false;
	}

	// current dir \\ bob.txt ���ϸ� ����
	/*
	HRESULT StringCbVPrintf(
	_Out_ LPTSTR  pszDest, //destination buffer
	_In_  size_t  cbDest, // destination buffer size
	_In_  LPCTSTR pszFormat, //format string
	_In_  va_list argList // be inserted into the pszFomat string
	);
	*/
	wchar_t *file_path = buf;
	wchar_t file_name[260];
	wchar_t file_name2[260];
	//��θ� ����
	if (!SUCCEEDED(StringCbPrintfW(
		file_name,
		sizeof(file_name),
		L"%ws\\bob.txt",
		buf)))
	{
		print("err, can not create file name");
		free(buf);
		return false;
	}
	//��θ� ����
	if (!SUCCEEDED(StringCbPrintfW(
		file_name2,
		sizeof(file_name2),
		L"%ws\\bob2.txt",
		buf)))
	{
		print("err, can not create file name");
		free(buf);
		return false;
	}
	free(buf); buf = NULL;

	if (true == is_file_existsW(file_name))
	{
		::DeleteFileW(file_name);
	}

	// ���� ����
	HANDLE file_handle = CreateFileW(
		file_name,
		GENERIC_WRITE,
		FILE_SHARE_READ,
		NULL,
		CREATE_NEW,
		FILE_ATTRIBUTE_NORMAL,
		NULL);
	if (file_handle == INVALID_HANDLE_VALUE)
	{
		print("err, CreateFile(path=%ws), gle=0x%08x", file_name, GetLastError());
		return false;
	}

	// ���Ͽ� ������ ����
	// �ѱ۷� ����1
	DWORD bytes_written = 0; //DWORD = word * 2
	wchar_t string_buf[1024];
	char *multi_buf = 0;
	if (!SUCCEEDED(StringCbPrintfW(
		string_buf,
		sizeof(string_buf),
		L"���ع��� ��λ��� ������ �⵵�� �ϴ����� �����ϻ� �츮���󸸼�")))
	{
		print("err, can not create data to write.");
		CloseHandle(file_handle);
		return false;
	}

	multi_buf = WcsToMbsUTF8(string_buf);

	if (!WriteFile(file_handle, multi_buf, strlen(multi_buf), &bytes_written, NULL))
	{
		print("err, WriteFile() failed. gle = 0x%08x", GetLastError());
		CloseHandle(file_handle);
		return false;
	}

	// ����� ���� 1
	if (!SUCCEEDED(StringCbPrintfW(
		string_buf,
		sizeof(string_buf),
		L"All work and no play makes jack a dull boy.")))
	{
		print("err, can not create data to write.");
		CloseHandle(file_handle);
		return false;
	}

	multi_buf = WcsToMbsUTF8(string_buf);

	if (!WriteFile(file_handle, multi_buf, strlen(multi_buf), &bytes_written, NULL))
	{
		print("err, WriteFile() failed. gle = 0x%08x", GetLastError());
		CloseHandle(file_handle);
		return false;
	}

	// �ѱ۷� ���� 2
	wchar_t string_uni[1024] = { 0, };
	char string_bufa[1024] = { 0, };
	char *utf8_buf = 0;
	if (!SUCCEEDED(StringCbPrintfA(
		string_bufa,
		sizeof(string_bufa),
		"���ع��� ��λ��� ������ �⵵�� �ϴ����� �����ϻ� �츮���󸸼�")))
	{
		print("err, can not create data to write.");
		CloseHandle(file_handle);
		return false;
	}

	// ��Ƽ����Ʈ -> �����ڵ� ��ȯ
	int nLen = MultiByteToWideChar(CP_ACP, 0, string_bufa, strlen(string_bufa), NULL, NULL);
	MultiByteToWideChar(CP_ACP, 0, string_bufa, strlen(string_bufa), string_uni, nLen);

	// �����ڵ� -> UTF-8 ��ȯ
	utf8_buf = WcsToMbsUTF8(string_uni);

	if (!WriteFile(file_handle, utf8_buf, strlen(utf8_buf), &bytes_written, NULL)){
		print("err, WriteFile() failed. gle = 0x%08x", GetLastError());
		CloseHandle(file_handle);
		return false;
	}

	// ����� ����2
	if (!SUCCEEDED(StringCbPrintfA(
		string_bufa,
		sizeof(string_bufa),
		"All work and no play makes jack a dull boy.")))
	{
		print("err, can not create data to write.");
		CloseHandle(file_handle);
		return false;
	}


	// ��Ƽ����Ʈ -> �����ڵ� ��ȯ
	nLen = MultiByteToWideChar(CP_ACP, 0, string_bufa, strlen(string_bufa), NULL, NULL);
	MultiByteToWideChar(CP_ACP, 0, string_bufa, strlen(string_bufa), string_uni, nLen);

	// �����ڵ� -> UTF-8 ��ȯ
	utf8_buf = WcsToMbsUTF8(string_uni);

	if (!WriteFile(file_handle, utf8_buf, strlen(utf8_buf), &bytes_written, NULL)){
		print("err, WriteFile() failed. gle = 0x%08x", GetLastError());
		CloseHandle(file_handle);
		return false;
	}


	//bob.txt -> bob2.txt �� ����
	CopyFile(file_name, file_name2, false);

	/** ---- ReadFile() api �� ���ؼ� ���� �б�  **/
	HANDLE file_handle2 = CreateFile(
		file_name2,
		GENERIC_READ,
		FILE_SHARE_READ,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL);
	if (file_handle2 == INVALID_HANDLE_VALUE)
	{
		print("err, CreateFile(path=%ws), gle=0x%08x", file_name, GetLastError());
		return false;
	}
	char strutf8[1024] = { 0, };
	char strmulti[1024] = { 0, };
	wchar_t strUni[256] = { 0, };
	DWORD size;
	ReadFile(file_handle2, strutf8, 1024, &size, NULL);

	// UTF-8 -> �����ڵ� ��ȯ
	int utfLen = MultiByteToWideChar(CP_UTF8, 0, strutf8, strlen(strutf8), NULL, NULL);
	MultiByteToWideChar(CP_UTF8, 0, strutf8, strlen(strutf8), strUni, utfLen);

	// �����ڵ� ->  ��Ƽ����Ʈ��ȯ
	int len = WideCharToMultiByte(CP_ACP, 0, strUni, -1, NULL, 0, NULL, NULL);
	WideCharToMultiByte(CP_ACP, 0, strUni, -1, strmulti, len, NULL, NULL);
	printf("ReadFile() api ���\n");
	printf("%s\n\n", strmulti);

	CloseHandle(file_handle2);

	/** ---- Memory Mapped I/O �� �̿��ؼ� ���� �б� **/
	//������ ����
	HANDLE file_handle3 = CreateFileW(
		(LPCWSTR)file_name2, //���� �̸�
		GENERIC_READ, //�б�����
		NULL, //�ٸ� ���μ������� �д°� ����
		NULL, //
		OPEN_EXISTING, //������ �����Ҷ� ����
		FILE_ATTRIBUTE_NORMAL,
		NULL
		);
	if (INVALID_HANDLE_VALUE == file_handle3)
	{
		print("err, CreateFile(%ws) failed, gle = %u", file_name2, GetLastError());
		return false;
	}

	// check file size
	//
	LARGE_INTEGER fileSize;
	if (TRUE != GetFileSizeEx(file_handle3, &fileSize))
	{
		print("err, GetFileSizeEx(%ws) failed, gle = %u", file_name2, GetLastError());
		CloseHandle(file_handle3);
		return false;
	}

	// [ WARN ]
	//
	// 4Gb �̻��� ������ ��� MapViewOfFile()���� ������ ���ų�
	// ���� ������ �̵��� ������ ��
	// FilIoHelperClass ����� �̿��ؾ� ��
	//
	_ASSERTE(fileSize.HighPart == 0);
	if (fileSize.HighPart > 0)
	{
		print("file size = %I64d (over 4GB) can not handle. use FileIoHelperClass",
			fileSize.QuadPart);
		CloseHandle(file_handle3);
		return false;
	}

	DWORD file_size = (DWORD)fileSize.QuadPart;
	//���ϸ���
	HANDLE file_map = CreateFileMapping(
		file_handle3,
		NULL,
		PAGE_READONLY,
		0,
		0,
		NULL
		);
	if (NULL == file_map)
	{
		print("err, CreateFileMapping(%ws) failed, gle = %u", file_name2, GetLastError());
		CloseHandle(file_handle);
		return false;
	}

	PCHAR file_view = (PCHAR)MapViewOfFile(
		file_map,
		FILE_MAP_READ,
		0,
		0,
		0
		);
	if (file_view == NULL)
	{
		print("err, MapViewOfFile(%ws) failed, gle = %u", file_name2, GetLastError());

		CloseHandle(file_map);
		CloseHandle(file_handle);
		return false;
	}

	char memmulti[1024] = { 0, };
	wchar_t memUni[256] = { 0, };

	// UTF-8 -> �����ڵ� ��ȯ
	utfLen = MultiByteToWideChar(CP_UTF8, 0, file_view, strlen(file_view), NULL, NULL);
	MultiByteToWideChar(CP_UTF8, 0, file_view, strlen(file_view), memUni, utfLen);

	// �����ڵ� ->  ��Ƽ����Ʈ ��ȯ
	len = WideCharToMultiByte(CP_ACP, 0, memUni, -1, NULL, 0, NULL, NULL);
	WideCharToMultiByte(CP_ACP, 0, memUni, -1, memmulti, len, NULL, NULL);

	printf("Memory-mapped I/O ���\n");
	printf("%s\n", memmulti);

	// close all
	UnmapViewOfFile(file_view);
	CloseHandle(file_map);
	CloseHandle(file_handle3);
	CloseHandle(file_handle);

	//bob.txt, bob2.txt ����
	DeleteFileW(file_name);
	DeleteFileW(file_name2);

	// ���� �ݱ�

	return true;

}

int main(){
	create_bob_txt();
	return 0;

}
