#include <windows.h>
#include <netsh.h>
#include <stdio.h>
#include <shlwapi.h>
#include <string.h>

int main() {
	STARTUPINFOA            startupInfo = {  };
	PROCESS_INFORMATION     processInformation = { };

	auto res = CreateProcessA(
		NULL,
		"\"C:\\Program Files (x86)\\byond\\bin\\dreamseeker.exe\"",
		NULL,
		NULL,
		TRUE,
		CREATE_SUSPENDED,
		NULL,
		NULL,
		&startupInfo, 
		&processInformation);

	if (!res)
	{
		fprintf(stderr, "CreateProcess failed\n");
		return FALSE;
	}

	wchar_t executablePath[1024];
	GetModuleFileNameW(NULL, executablePath, 1024);
	PathRemoveFileSpecW(executablePath);
	wcscat(executablePath, L"\\hook.dll");

	wprintf(L"hook path: %ls\n", executablePath);

	// lpcwszDll is a string that contains path to a DLL hook
	auto nLength = wcslen(executablePath) * sizeof(WCHAR);

	// allocate mem for dll name
	auto lpRemoteString = VirtualAllocEx(processInformation.hProcess, NULL, nLength + 1, MEM_COMMIT, PAGE_READWRITE);

	if (!lpRemoteString)
	{
		fprintf(stderr, "Failed to allocate memory in the target process\n");
		
		CloseHandle(processInformation.hProcess);
		return FALSE;
	}

	// write DLL hook name
	if (!WriteProcessMemory(processInformation.hProcess, lpRemoteString, executablePath, nLength, NULL)) 
	{
		fprintf(stderr, "Failed to write memory to the target process\n");

		VirtualFreeEx(processInformation.hProcess, lpRemoteString, 0, MEM_RELEASE);
		CloseHandle(processInformation.hProcess);
		return FALSE;
	}

	LPVOID lpLoadLibraryW = NULL;
	lpLoadLibraryW = GetProcAddress(GetModuleHandleA("KERNEL32.DLL"), "LoadLibraryW");

	if (!lpLoadLibraryW)
	{
		fprintf(stderr, "GetProcAddress failed\n");
		CloseHandle(processInformation.hProcess);
		return FALSE;
	}

	// call LoadLibraryW
	HANDLE hThread = CreateRemoteThread(
		processInformation.hProcess,
		NULL,
		NULL,
		(LPTHREAD_START_ROUTINE)lpLoadLibraryW,
		lpRemoteString,
		NULL,
		NULL);

	if (!hThread) 
	{
		fprintf(stderr, "CreateRemoteThread failed\n");
		CloseHandle(processInformation.hProcess);
		return FALSE; 
	}
	else
	{
		WaitForSingleObject(hThread, 4000);
		ResumeThread(processInformation.hThread);
	}
}