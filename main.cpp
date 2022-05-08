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
	ULONG originalSessionCount = 0;
	PLUID logonSessionListPtr = nullptr;
	PSECURITY_LOGON_SESSION_DATA logonSessionData = nullptr;

	PLUID nextLogonSessionID = nullptr;

	NTSTATUS result = LsaEnumerateLogonSessions(&logonSessionCount, &logonSessionListPtr);
	originalSessionCount = logonSessionCount;

	if (STATUS_SUCCESS != result) {
		printf("Faield to get logon session count: 0x%x", result);
		return result;
	}

	printf("logon session count: %d\n", logonSessionCount);

	do {
		NTSTATUS result = LsaGetLogonSessionData(logonSessionListPtr, &logonSessionData);

		if (STATUS_SUCCESS == result) {
			wprintf(L"username: %s\n", logonSessionData->UserName.Buffer);
		}
		else {
			printf("LsaGetLogonSession data fail: 0x%x\n", result);
		}

		logonSessionListPtr += sizeof(PLUID);
		originalSessionCount--;
		printf("logonSessionCount now: %lld\n", originalSessionCount);
	}
	while (originalSessionCount > 0);

	LsaFreeReturnBuffer(logonSessionListPtr);

	return 0;
}

void WINAPI ServiceMain(DWORD argc, LPTSTR* argv) {

}