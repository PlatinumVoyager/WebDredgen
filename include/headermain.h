#ifndef HEADERMAIN_H
#define HEADERMAIN_H

#include <WinSock2.h>
#include <Windows.h>
#include <winhttp.h>
#include <wininet.h>
#include <http.h>
#include <processthreadsapi.h>

#include <winerror.h>
#include <wchar.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "winhttp.lib")
#pragma comment(lib, "wininet.lib")
#pragma comment(lib, "httpapi.lib")
#pragma comment(lib, "kernel32.lib")

// --------- define directives ---------
#define WEBDREDGEN_SERVER_PORT 7743

// --------- function prototypes ---------

// Custom helper functions to clean up allocated memory, data, etc.
int handleHttpTermination(void);
int handleHttpCloseServerSession(HTTP_SERVER_SESSION_ID httpSessionId);
int handleHttpCloseUrlGroup(HTTP_URL_GROUP_ID httpUrlGroupId);
int handleHttpCloseRequestQueue(HANDLE requestQueueHandle);

// generic system wide error handler when defined errors are not applicable
LPWSTR returnMsgBuffer(DWORD errorCode);

// create function prototypes for obtaining IPv4 addresses to local wireless LAN interfaces

// ---------------------------------------

#endif