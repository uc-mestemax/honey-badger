#pragma once
#pragma once
#include <Windows.h>
#include "common_types.h"
#include "time.h"
#include <string>

#define __FILENAME__ (strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : __FILE__)
#define ERROR_PRINT(msg)(printf("ERROR %s-->%s:%d %s\n", __FILENAME__, __func__, __LINE__, msg))
#define CRITICAL_ERROR(msg)(printf("CRITICAL ERROR %s-->%s:%d %s\n", __FILENAME__, __func__, __LINE__, msg), exit(-1))

#define CPPHTTPLIB_OPENSSL_SUPPORT

#ifdef _DEBUG
#define DEBUG_PRINT(msg)(printf("%s --> %s:%d   %s\n", __FILENAME__, __func__, __LINE__, msg))
#else
#define DEBUG_PRINT(msg)(printf("%s-->%s:%d %s\n", __FILENAME__, __func__, __LINE__, msg))
#endif

void* zalloc(size_t size);
void PrintIndents(uint numIndents);
double RoundTo(double value, double precision = 1.0);
bool RoundStr(char* pStr, uint numDecimals);