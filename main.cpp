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


/*
* Macro to fail starting the service and actually report this back to the Service Manager
*  This is a messy macro, but I don't want to call another function in these failure cases
* and have to manage the scope of &serviceStatus differently.
*/
#define BailOnServiceStart ZeroMemory(&serviceStatus, sizeof(serviceStatus));\
serviceStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;\
serviceStatus.dwCurrentState = SERVICE_STOPPED;\
serviceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;\
serviceStatus.dwWin32ExitCode = GetLastError();\
serviceStatus.dwServiceSpecificExitCode = GetLastError();\
serviceStatus.dwCheckPoint = 0;\
serviceStatus.dwWaitHint = 0;\
if (!SetServiceStatus(statusHandle, &serviceStatus)) {\
	wprintf(L"SetServiceStatus returned FALSE -- error %d\n", GetLastError());\
	return;\
}\
if (INVALID_HANDLE_VALUE != logHandle) {\
	CloseHandle(logHandle);\
}\
return;

SERVICE_STATUS serviceStatus = { 0 };
SERVICE_STATUS_HANDLE statusHandle = nullptr;
HANDLE logHandle = INVALID_HANDLE_VALUE;
WCHAR logBuffer[MAX_PATH] = { 0 };
#define LOG_BUFFER_SIZE (MAX_PATH * sizeof(WCHAR))
#define SERVICE_NAME L"ShutdownBuddy"

#define WAIT_TIMER_INTERVAL_SECONDS 5
#define MS_IN_S 1000
#define HUNDREDNS_IN_MS 10000

/* Synchronisation and looping
* 
* We will have an Event which can be signalled to stop the worker thread
* and also a WaitableTimer for the worker thread so that we do not poll
* user session state too frequently.
* 
* We will WaitForMultipleObjects on either of these to be signalled in the
* worker thread. When signalled, we will check to see if the Event was the reason
* for the signal and, if so, stop the worker thread in an orderly fashion.
*/
HANDLE workerWaitableTimer = INVALID_HANDLE_VALUE;
HANDLE serviceToStopEvent = INVALID_HANDLE_VALUE;
HANDLE workerThread = INVALID_HANDLE_VALUE;

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
	LARGE_INTEGER timerDueTime;
	

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

	/*
	Service is starting
	*/
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


	// create synchronisation objects -- timer
	workerWaitableTimer = CreateWaitableTimer(NULL, TRUE, NULL);
	if (workerWaitableTimer == NULL) {
		wprintf(L"Unable to create waitable timer for the worker thread -- error %d\n", GetLastError());
		BailOnServiceStart;
	}
	timerDueTime.QuadPart = -(WAIT_TIMER_INTERVAL_SECONDS * MS_IN_S * HUNDREDNS_IN_MS);


	// prepare the stop event
	serviceToStopEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	if (serviceToStopEvent == NULL) {
		wprintf(L"Unable to create the event to notify worker thread on service stop -- error %d", GetLastError());
		BailOnServiceStart;
	}

	// start worker thread
	workerThread = CreateThread(NULL, 0, ServiceWorkerThread, NULL, 0, NULL);
	if (workerThread == NULL) {
		wprintf(L"Failed to create WorkerThread -- error %d", GetLastError());
		BailOnServiceStart;
	}

	// start timer
	SetWaitableTimer(workerWaitableTimer, &timerDueTime, WAIT_TIMER_INTERVAL_SECONDS * MS_IN_S, NULL, NULL, FALSE);

	WaitForSingleObject(workerThread, INFINITE);

exit:
	/*
	Service has stopped
	*/
	ZeroMemory(&serviceStatus, sizeof(serviceStatus));
	serviceStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
	serviceStatus.dwCurrentState = SERVICE_STOPPED;
	serviceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;
	serviceStatus.dwWin32ExitCode = 0;
	serviceStatus.dwServiceSpecificExitCode = 0;
	serviceStatus.dwCheckPoint = 0;
	serviceStatus.dwWaitHint = 0;
	if (!SetServiceStatus(statusHandle, &serviceStatus)) {
		wprintf(L"SetServiceStatus returned FALSE -- error %d\n", GetLastError());
		return;
	}

	StringCbPrintf(logBuffer, LOG_BUFFER_SIZE, L"Service has stopped.");
	WriteBufferToLog();

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
	if (!(WriteFile(logHandle, logBuffer, (wcslen(logBuffer) + 1) * sizeof(WCHAR), nullptr, nullptr))) {
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

	HANDLE waitableObjects[2] = { serviceToStopEvent, workerWaitableTimer };
	DWORD waitResult = MAXDWORD;
	assert(waitableObjects[0] != INVALID_HANDLE_VALUE);
	assert(waitableObjects[0] != NULL);
	assert(waitableObjects[1] != INVALID_HANDLE_VALUE);
	assert(waitableObjects[1] != NULL);
	
	StringCbPrintf(logBuffer, LOG_BUFFER_SIZE, L"Started ShutdownBuddy service worker thread\n");
	WriteBufferToLog();

	for (;;) {
		// check for the signalled state of the stop event
		if (waitResult == WAIT_OBJECT_0) {
			// being signalled to stop
			return ERROR_SUCCESS;
		}
		StringCbPrintf(logBuffer, LOG_BUFFER_SIZE, L"Worker thread wait result is %d", waitResult);
		WriteBufferToLog();
		
		
		result = LsaEnumerateLogonSessions(&logonSessionCount, &logonSessionListPtr);

		if (STATUS_SUCCESS != result) {
			wprintf(L"Failed to get logon session count: 0x%x", result);
			continue;
		}

		wprintf(L"logon session count: %d\n", logonSessionCount);

		do {
			result = LsaGetLogonSessionData(logonSessionListPtr, &logonSessionData);

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

		// wait for either the timer or the event
		waitResult = WaitForMultipleObjects(2, waitableObjects, FALSE, INFINITE);
	}

	return ERROR_SUCCESS;
}

/// <summary>
/// Responds to control events from the service manager.
/// </summary>
/// <param name="controlCode"></param>
/// <returns></returns>
VOID WINAPI ServiceCtrlHandler(DWORD controlCode) {
	switch (controlCode) {
	case SERVICE_CONTROL_STOP:
		// note that we are stopping
		ZeroMemory(&serviceStatus, sizeof(serviceStatus));
		serviceStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
		serviceStatus.dwCurrentState = SERVICE_STOP_PENDING;
		serviceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;
		serviceStatus.dwWin32ExitCode = 0;
		serviceStatus.dwServiceSpecificExitCode = 0;
		serviceStatus.dwCheckPoint = 0;
		serviceStatus.dwWaitHint = 0;
		if (!SetServiceStatus(statusHandle, &serviceStatus)) {
			wprintf(L"SetServiceStatus returned FALSE -- error %d\n", GetLastError());
			return;
		}

		StringCbPrintf(logBuffer, LOG_BUFFER_SIZE, L"Stopping service...\n");
		WriteBufferToLog();

		SetEvent(serviceToStopEvent);
		break;
	}
}