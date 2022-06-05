#pragma once
#include <Windows.h>
#include <tchar.h>

VOID WINAPI ServiceMain(DWORD argc, LPTSTR* argv);
VOID WINAPI ServiceCtrlHandler(DWORD);
DWORD WINAPI ServiceWorkerThread(LPVOID lpParam);
void WriteBufferToLog(void);

BOOL CALLBACK EnumWindowStationProc(
	_In_ LPTSTR windowStation,
	_In_ LPARAM param
);

BOOL ExplorerIsRunningAsSID(PSID sid);