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
	PLUID logonSessionListPtr = nullptr;
	PSECURITY_LOGON_SESSION_DATA logonSessionData = nullptr;

	if (STATUS_SUCCESS == LsaEnumerateLogonSessions(&logonSessionCount, &logonSessionListPtr)) {
		printf("logon session count: %d\n", logonSessionCount);

		if (STATUS_SUCCESS == LsaGetLogonSessionData(logonSessionListPtr, &logonSessionData)) {
			wprintf(L"username: %s\n", logonSessionData->UserName.Buffer);
		}
		else {
			printf("LsaGetLogonSession data fail\n");
		}

		LsaFreeReturnBuffer(logonSessionListPtr);
	}
	else {
		printf("FAIL: %d", GetLastError());
	}

	return 0;
}

void WINAPI ServiceMain(DWORD argc, LPTSTR* argv) {

}