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

#define assert(expression) if (!(expression)) { printf("assert on %d", __LINE__); /*TODO assert when in the service needs to be intelligent */ExitProcess(250); }


SERVICE_STATUS serviceStatus = { 0 };
SERVICE_STATUS_HANDLE statusHandle = nullptr;
HANDLE serviceStopEvent = INVALID_HANDLE_VALUE;
HANDLE logHandle = INVALID_HANDLE_VALUE;
WCHAR logBuffer[MAX_PATH] = { 0 };
#define LOG_BUFFER_SIZE (MAX_PATH * sizeof(WCHAR))
#define SERVICE_NAME L"ShutdownBuddy"

/// <summary>
/// Entry point
/// </summary>
/// <param name="argc"></param>
/// <param name="argv"></param>
/// <returns></returns>
int wmain(int argc, WCHAR* argv[]) {

	WCHAR serviceName[] = SERVICE_NAME;

	SERVICE_TABLE_ENTRY table[] = {
		{serviceName, (LPSERVICE_MAIN_FUNCTION)ServiceMain},
		{NULL, NULL }
	};

	if (!(StartServiceCtrlDispatcher(table))) {
		wprintf(L"Failed to start service dispatcher: %d", GetLastError());
		return GetLastError();
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
	// prepare temporary file for logging
	WCHAR tempPath[MAX_PATH + 1] = { 0 };
	WCHAR tempFileName[MAX_PATH + 1] = { 0 };
	

	if (GetTempPath(MAX_PATH, tempPath) == 0) {
		wprintf(L"Unable to get temp path name\n");
		goto exit;
	}

	if (GetTempFileName(tempPath, L"SdB", 0, tempFileName) == 0) {// capped to MAX_PATH
		wprintf(L"Unable to get temp file name\n");
		goto exit;
	}

	// start logging
	logHandle = CreateFile(tempFileName, GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (logHandle == INVALID_HANDLE_VALUE) {
		wprintf(L"Unable to open temp file for writing: %s (error 0x%x)", tempFileName, GetLastError());
		goto exit;
	}

	StringCbPrintf(logBuffer, LOG_BUFFER_SIZE, L"Started ShutdownBuddy service\n");
	WriteBufferToLog();

	statusHandle = RegisterServiceCtrlHandler(SERVICE_NAME, ServiceCtrlHandler);
	if (statusHandle == NULL) {
		wprintf(L"StatusHandle was NULL\n");
		return;
	}

	ZeroMemory(&serviceStatus, sizeof(serviceStatus));
	serviceStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
	serviceStatus.dwCurrentState = SERVICE_START_PENDING;
	serviceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;
	serviceStatus.dwWin32ExitCode = 0;
	serviceStatus.dwServiceSpecificExitCode = 0;
	serviceStatus.dwCheckPoint = 0;
	serviceStatus.dwWaitHint = 0;
	if (!SetServiceStatus(statusHandle, &serviceStatus)) {
		wprintf(L"SetServiceStatus returned FALSE -- error %d\n", GetLastError());
		return;
	}

exit:
	if (INVALID_HANDLE_VALUE != logHandle) {
		CloseHandle(logHandle);
	}

	
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

/// <summary>
/// Actually do the waiting and sleeping
/// </summary>
/// <param name="lpParam"></param>
/// <returns></returns>
DWORD WINAPI ServiceWorkerThread(LPVOID lpParam) {

	NTSTATUS result = -1;

	ULONG logonSessionCount = 0;
	ULONG originalSessionCount = 0;
	PLUID logonSessionListPtr = nullptr;
	PSECURITY_LOGON_SESSION_DATA logonSessionData = nullptr;

	PLUID nextLogonSessionID = nullptr;

	for (;;) {
		StringCbPrintf(logBuffer, LOG_BUFFER_SIZE, L"Started ShutdownBuddy service\n");
		WriteBufferToLog();
		result = LsaEnumerateLogonSessions(&logonSessionCount, &logonSessionListPtr);

		if (STATUS_SUCCESS != result) {
			wprintf(L"Failed to get logon session count: 0x%x", result);
			continue;
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

			logonSessionListPtr++; //pointer increment is a different operator! -- 8 bytes forward
			logonSessionCount--;

			LsaFreeReturnBuffer(logonSessionData);

		} while (logonSessionCount > 0);

		LsaFreeReturnBuffer(logonSessionListPtr);
	}

	return ERROR_SUCCESS;
}