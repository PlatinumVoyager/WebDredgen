#include "../include/headermain.h"


/*
    Cleans up resources used by the HTTP Server API to process calls by an application.
    source: https://learn.microsoft.com/en-us/windows/win32/api/http/nf-http-httpterminate
*/
int handleHttpTermination(void)
{
    ULONG ret;

    // HTTP_INITIALIZE_CONFIG = Release all resources used by applications that modify the HTTP configuration.
    // HTTP_INITIALIZE_SERVER = Release all resources used by server applications.
    ret = HttpTerminate(HTTP_INITIALIZE_SERVER | HTTP_INITIALIZE_CONFIG, NULL);

    if (ret != NO_ERROR)
    {
        if (ret == ERROR_INVALID_PARAMETER)
        {
            printf("ERROR: One or more of the supplied parameters is in an unusable form.\n");
            
            return 0;
        }
        else
        {
            DWORD error = GetLastError();
            wprintf(L"ERROR: %s\n", returnMsgBuffer(error));

            exit(1);
        }
    }

    printf("SUCCESS: Cleaned up resources used by the HTTP server API.\n");

    exit(0);
}

/*
    Deletes the server session identified by the server session ID. 
    All remaining URL Groups associated with the server session will also be closed.

    source: https://learn.microsoft.com/en-us/windows/win32/api/http/nf-http-httpcloseserversession
*/
int handleHttpCloseServerSession(HTTP_SERVER_SESSION_ID httpSessionId)
{
    ULONG res = HttpCloseServerSession(httpSessionId);

    if (res != NO_ERROR)
    {
        if (res == ERROR_INVALID_PARAMETER)
        {
            printf("ERROR: The server session does not exist or you do not have permission to close the server session.\n");
        
            return 0;
        }
        else
        {
            DWORD error = GetLastError();
            wprintf(L"ERROR: %s\n", returnMsgBuffer(error));

            exit(1);
        }
    }

    printf("SUCCESS: Closed the server session gracefully.\n");

    return 0;
} 

/*
    Closes the URL Group identified by the URL Group ID. 
    This call also removes all of the URLs that are associated with the URL Group.

    source: https://learn.microsoft.com/en-us/windows/win32/api/http/nf-http-httpcloseurlgroup
*/
int handleHttpCloseUrlGroup(HTTP_URL_GROUP_ID httpUrlGroupId)
{
    ULONG res = HttpCloseUrlGroup(httpUrlGroupId);

    if (res != NO_ERROR)
    {
        if (res == ERROR_INVALID_PARAMETER)
        {
            printf("ERROR: The ID of the URL group does not exist or the application does not have permission to close the URL group.\n");

            return 0;
        }
        else
        {
            DWORD error = GetLastError();
            wprintf(L"ERROR: %s (FILE=%hs,LINE=%d)\n", returnMsgBuffer(error), __FILE__, __LINE__);

            exit(1);
        }
    }

    printf("SUCCESS: Closed the HTTP server URL group gracefully.\n");

    return 0;
}

/*
    Closes the handle to the specified request queue created by HttpCreateRequestQueue.
    source: https://learn.microsoft.com/en-us/windows/win32/api/http/nf-http-httpcloserequestqueue
*/
int handleHttpCloseRequestQueue(HANDLE requestQueueHandle)
{
    ULONG res = HttpCloseRequestQueue(requestQueueHandle);

    if (res != NO_ERROR)
    {
        if (res == ERROR_INVALID_PARAMETER)
        {
            printf("ERROR: The application does not have permission to close the request queue.\n");
        
            return 0;
        }
        else
        {
            DWORD error = GetLastError();
            wprintf(L"ERRsOR: %s\n", returnMsgBuffer(error));

            exit(1);
        }
    }

    printf("SUCCESS: Closed the HTTP request queue gracefully.\n");

    return 0;
}


LPWSTR returnMsgBuffer(DWORD errorCode)
{
    LPWSTR msgBuffer = NULL;

    DWORD result = FormatMessageW
    (
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        errorCode,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPWSTR)&msgBuffer,
        0,
        NULL
    );

    return msgBuffer;
}