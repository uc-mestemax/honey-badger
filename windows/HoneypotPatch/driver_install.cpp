#include <Windows.h>
#include <setupapi.h>
#include <iostream>
#include "misc.h"

bool install_HarnessDrv(const char* driverDirectory) {
	SC_HANDLE manager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if (manager)
	{
		wprintf(L"Opened SC Manager\n");
	}
	else
	{
		wprintf(L"Open SC Manager failed\n");
		return 0;
	}

	UINT ErrorLine;
	BOOL bRes = FALSE;
	BOOL FileWasInUse = FALSE;
	LPCSTR  SourceFile = ("HarnessDrv.sys");
	LPCSTR SourcePathRoot = driverDirectory;
	LPCSTR DestinationName = "C:\\WINDOWS\\system32\\drivers\\HarnessDrv.sys";
	HINF HInf = SetupOpenInfFileA(combineStrings((char*)driverDirectory, (char*)"\\HarnessDrv.inf"), NULL, INF_STYLE_OLDNT | INF_STYLE_WIN4, &ErrorLine);

	void* Context = NULL;
	if (SetupInstallFileExA(HInf, NULL, SourceFile, SourcePathRoot, DestinationName, SP_COPY_NEWER_OR_SAME, NULL, Context, &FileWasInUse)) {
		printf("Sucessfully Installed\n");
		SC_HANDLE service = CreateService(manager, L"HarnessDrv", L"HarnessDrv", SERVICE_ALL_ACCESS, SERVICE_KERNEL_DRIVER, SERVICE_AUTO_START, SERVICE_ERROR_NORMAL, L"\\SystemRoot\\system32\\drivers\\HarnessDrv.sys", NULL, NULL, NULL, NULL, NULL);
		if (service) {
			BOOL result = StartService(service, 0, NULL);
			if (result) {
				printf("Service has been started\n");
				CloseServiceHandle(service);
				CloseServiceHandle(manager);
				return true;
			}
			else {
				printf("Failed to start service %d\n", GetLastError());
			}
			CloseServiceHandle(service);
		}
		else {
			printf("Failed to create service %d\n", GetLastError());
		}
	}
	else {
		printf("Failed to install file %d %d\n", ErrorLine, GetLastError());
	}

	CloseServiceHandle(manager);

	return false;
}

bool delete_HarnessDrv() {
	SC_HANDLE manager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if (manager)
	{
		wprintf(L"[*] Opened SC Manager\n");
	}
	else
	{
		wprintf(L"[*] Open SC Manager failed\n");
		return 0;
	}

	SC_HANDLE service = OpenService(manager, L"HarnessDrv", SERVICE_ALL_ACCESS);
	if (service) {
		SERVICE_STATUS status = { 0 };
		BOOL result = ControlService(service, SERVICE_CONTROL_STOP, &status);
		if (result || GetLastError() == ERROR_SERVICE_NOT_ACTIVE) {
			if (DeleteService(service)) {
				printf("[*] Successfully removed HarnessDrv\n");
				CloseServiceHandle(service);
				CloseServiceHandle(manager);
				return true;
			}
			else {
				printf("[*] Failed to remove HarnessDrv %d\n", GetLastError());
			}
		}
		else {
			printf("[*] Failed to stop service %d\n", GetLastError());
		}
		CloseServiceHandle(service);
	}
	else if (GetLastError() == ERROR_SERVICE_DOES_NOT_EXIST) {
		CloseServiceHandle(manager);
		return true;
	}
	else {
		printf("[*] Failed to open service %d\n", GetLastError());
	}

	CloseServiceHandle(manager);

	return false;
}

bool UpgradeHarnessDrv(const char* driverDirectory) {
	if (delete_HarnessDrv()) {
		return install_HarnessDrv(driverDirectory);
	}
	return false;
}