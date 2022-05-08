#include <stdio.h>
#include <Windows.h>
#include <tchar.h>
#include <NTSecAPI.h>
#include <ntstatus.h>
#include <fileapi.h>
#include "main.h"
#pragma comment(lib, "secur32")

SERVICE_STATUS serviceStatus = { 0 };
SERVICE_STATUS_HANDLE statusHandle = nullptr;
HANDLE serviceStopEvent = INVALID_HANDLE_VALUE;
HANDLE logHandle = INVALID_HANDLE_VALUE;
WCHAR logBuffer[MAX_PATH] = { 0 };

int _tmain(int argc, TCHAR* argv[]) {

	ULONG logonSessionCount = 0;
	ULONG originalSessionCount = 0;
	PLUID logonSessionListPtr = nullptr;
	PSECURITY_LOGON_SESSION_DATA logonSessionData = nullptr;

	PLUID nextLogonSessionID = nullptr;

	// prepare temporary file for logging
	WCHAR tempPath[MAX_PATH] = { 0 };
	WCHAR tempFileName[MAX_PATH] = { 0 };

	if (GetTempPath(MAX_PATH, tempPath) == 0) {
		printf("Unable to get temp path name\n");
		return -1;
	}

	if (GetTempFileName(tempPath, L"ShutdownBuddy", 0, tempFileName) == 0) {// TODO how does this not overflow tempFileName??
		printf("Unable to get temp file name\n");
		return -2;
	}

	// start logging
	logHandle = CreateFile(tempFileName, GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (logHandle == INVALID_HANDLE_VALUE) {
		wprintf(L"Unable to open temp file for writing: %s (error 0x%x)", tempFileName, GetLastError());
		return GetLastError();
	}

	wsprintf(logBuffer, L"test"); //TODO not safe -- overflow
	WriteFile(logHandle, logBuffer, wcslen(logBuffer), nullptr, nullptr);
	FlushFileBuffers(logHandle);

	NTSTATUS result = LsaEnumerateLogonSessions(&logonSessionCount, &logonSessionListPtr);

	if (STATUS_SUCCESS != result) {
		printf("Failed to get logon session count: 0x%x", result);
		return result;
	}

	printf("logon session count: %d\n", logonSessionCount);

	do {
		NTSTATUS result = LsaGetLogonSessionData(logonSessionListPtr, &logonSessionData);

		if (STATUS_SUCCESS == result) {
			wprintf(L"username: %s\n", logonSessionData->UserName.Buffer);
		}
		else if (STATUS_ACCESS_DENIED == result) {
			printf("Access denied -- running as admin??\n");
		}
		else {
			printf("LsaGetLogonSession data fail: 0x%x\n", result);
		}

		//printf("ptr before 0x%x\n", logonSessionListPtr);
		//logonSessionListPtr += (sizeof(PLUID));
		logonSessionListPtr++; // why does this work?? -- increments pointer by 64 bits correctly.
		//printf("ptr after 0x%x\n\n", logonSessionListPtr);

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

void WINAPI ServiceMain(DWORD argc, LPTSTR* argv) {

}