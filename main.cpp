#include <stdio.h>
#include <Windows.h>
#include <tchar.h>
#include <NTSecAPI.h>
#include <ntstatus.h>
#include "main.h"
#pragma comment(lib, "secur32")

SERVICE_STATUS serviceStatus = { 0 };
SERVICE_STATUS_HANDLE statusHandle = nullptr;
HANDLE serviceStopEvent = INVALID_HANDLE_VALUE;

int _tmain(int argc, TCHAR* argv[]) {

	ULONG logonSessionCount = 0;
	LUID logonSessionList = { 0 };
	PLUID logonSessionListPtr = &logonSessionList;

	if (STATUS_SUCCESS == LsaEnumerateLogonSessions(&logonSessionCount, &logonSessionListPtr)) {
		printf("logon session count: %d", logonSessionCount);
	}
	else {
		printf("FAIL: %d", GetLastError());
	}

	return 0;
}

void WINAPI ServiceMain(DWORD argc, LPTSTR* argv) {

}