#include <Windows.h>
#include <setupapi.h>
#include <cfgmgr32.h>
#include <tchar.h>
#include <initguid.h>
#include <Devpkey.h>
#include <stdio.h>
#include <stdlib.h>

#define ARRAY_SIZE(arr)     (sizeof(arr)/sizeof(arr[0]))

typedef unsigned int uint;

//Must be null terminated!
char* combineStrings(char* pFirstString, char* pSecondString) {
	uint firstLen = strlen(pFirstString);
	uint secondLen = strlen(pSecondString);
	char* pNewString = (char*)malloc(firstLen + secondLen + 1);
	memcpy(pNewString, pFirstString, firstLen);
	memcpy(pNewString + firstLen, pSecondString, secondLen);
	pNewString[firstLen + secondLen] = '\0';
	return pNewString;
}