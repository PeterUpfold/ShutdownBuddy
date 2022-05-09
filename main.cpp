#include <stdio.h>
#include <Windows.h>
#include <tchar.h>
#include <NTSecAPI.h>
#include <ntstatus.h>
#include <fileapi.h>
#include <strsafe.h>
#include <processthreadsapi.h>
#include "main.h"
#pragma comment(lib, "secur32")

#define assert(expression) if (!(expression)) { printf("assert on %d", __LINE__); ExitProcess(250); }


SERVICE_STATUS serviceStatus = { 0 };
SERVICE_STATUS_HANDLE statusHandle = nullptr;
HANDLE serviceStopEvent = INVALID_HANDLE_VALUE;
HANDLE logHandle = INVALID_HANDLE_VALUE;
WCHAR logBuffer[MAX_PATH] = { 0 };
#define LOG_BUFFER_SIZE (MAX_PATH * sizeof(WCHAR))

/// <summary>
/// Entry point
/// </summary>
/// <param name="argc"></param>
/// <param name="argv"></param>
/// <returns></returns>
int _tmain(int argc, TCHAR* argv[]) {

	ULONG logonSessionCount = 0;
	ULONG originalSessionCount = 0;
	PLUID logonSessionListPtr = nullptr;
	PSECURITY_LOGON_SESSION_DATA logonSessionData = nullptr;

	PLUID nextLogonSessionID = nullptr;

	// prepare temporary file for logging
	WCHAR tempPath[MAX_PATH + 1] = { 0 };
	WCHAR tempFileName[MAX_PATH + 1] = { 0 };

	if (GetTempPath(MAX_PATH, tempPath) == 0) {
		wprintf(L"Unable to get temp path name\n");
		return -1;
	}

	if (GetTempFileName(tempPath, L"SdB", 0, tempFileName) == 0) {// capped to MAX_PATH
		wprintf(L"Unable to get temp file name\n");
		return -2;
	}

	// start logging
	logHandle = CreateFile(tempFileName, GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (logHandle == INVALID_HANDLE_VALUE) {
		wprintf(L"Unable to open temp file for writing: %s (error 0x%x)", tempFileName, GetLastError());
		return GetLastError();
	}

	StringCbPrintf(logBuffer, LOG_BUFFER_SIZE, L"Test test test\n");
	WriteBufferToLog();	

	NTSTATUS result = LsaEnumerateLogonSessions(&logonSessionCount, &logonSessionListPtr);

	if (STATUS_SUCCESS != result) {
		wprintf(L"Failed to get logon session count: 0x%x", result);
		return result;
	}

	wprintf(L"logon session count: %d\n", logonSessionCount);

	do {
		NTSTATUS result = LsaGetLogonSessionData(logonSessionListPtr, &logonSessionData);

		if (STATUS_SUCCESS == result) {
			StringCbPrintf(logBuffer, LOG_BUFFER_SIZE, L"username: %s\n", logonSessionData->UserName.Buffer);
			WriteBufferToLog();
			StringCbPrintf(logBuffer, LOG_BUFFER_SIZE, L"logonType: %d\n", logonSessionData->LogonType);
			WriteBufferToLog();
		}
		else if (STATUS_ACCESS_DENIED == result) {
			StringCbPrintf(logBuffer, LOG_BUFFER_SIZE, L"Access denied on session reverse numbered %d -- running as admin??\n", logonSessionCount);
			WriteBufferToLog();
		}
		else {
			StringCbPrintf(logBuffer, LOG_BUFFER_SIZE, L"LsaGetLogonSession data fail: 0x%x\n", result);
			WriteBufferToLog();
		}

		logonSessionListPtr++; // why does this work?? -- increments pointer by 64 bits correctly.
		logonSessionCount--;

		LsaFreeReturnBuffer(logonSessionData);

	}
	while (logonSessionCount > 0);

	LsaFreeReturnBuffer(logonSessionListPtr);

	if (INVALID_HANDLE_VALUE != logHandle) {
		CloseHandle(logHandle);
	}

	return 0;
}

/// <summary>
/// Service entry point
/// </summary>
/// <param name="argc"></param>
/// <param name="argv"></param>
/// <returns></returns>
void WINAPI ServiceMain(DWORD argc, LPTSTR* argv) {

}

/// <summary>
/// Write the log buffer to the log file.
/// </summary>
/// <param name=""></param>
void WriteBufferToLog(void) {
	assert(logHandle != INVALID_HANDLE_VALUE);
	if (!(WriteFile(logHandle, logBuffer, (wcslen(logBuffer) + 1 * sizeof(WCHAR)), nullptr, nullptr))) {
		wprintf(L"Failed to write to logfile Error: %d.\n", GetLastError());
	}
	StringCbPrintf(logBuffer, (MAX_PATH * sizeof(WCHAR)), L"");
}