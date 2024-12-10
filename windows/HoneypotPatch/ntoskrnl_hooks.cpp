#include "ntoskrnl_hooks.h"

//Remember not to reference functions that reside in usermode. Many of them won't work correctly and will cause a system panic

uint tokenObjSize = 0x0;

//Monitored heap range
void* gpHeapRangeStart;
void* gpHeapRangeEnd;
void(*pfn)();

bool InitializeIndirects() {

	return true;
}

void TokenCreationHandler() {

}

void ExceptionHandlerHook(void *pTrapFrame) {
	
	//if () {

	//}
}