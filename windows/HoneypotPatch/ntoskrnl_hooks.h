#pragma once
#include "common_types.h"

extern "C" {
	void SepCreateTokenExHook();
	void ObfDereferenceObjectWithTagHook();
	void KiPageFaultHook();
}

bool InitializeIndirects();
void TokenCreationHandler();
void ExceptionHandler(void* pTrapFrame);