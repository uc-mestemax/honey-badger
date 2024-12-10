#include <Windows.h>
#include <stdio.h>
#include "driver_install.h"
#include "harness_com.h"
#include "inject.h"

int main()
{
	printf("[*] Attempting to upgrade HarnessDrv\n");
	UpgradeHarnessDrv("[*] C:\\Users\\badger\\Desktop\\HarnessDrv");
	//UpgradeVMXCom("C:\\Users\\deffi\\source\\repos\\VMXCom\\x64\\Release");
	OpenHarnessDrv();
	if (ghHarnessDrv) {
		printf("[*] ghHarnessDrv: %p\n", ghHarnessDrv);
		InjectNTOSKRNL();
	}
	else {
		printf("[*] Failed to open handle to HarnessDrv\n");
	}
	printf("[*] All finished. Goodbye\n");
	return 0;
}