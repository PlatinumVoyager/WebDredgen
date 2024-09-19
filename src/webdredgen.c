#include "../include/headermain.h"

#define WEBD_SUCCESS "\033[0;32mSUCCESS\033[0;m"
#define WEBD_ERROR "\033[0;31mERROR\033[0;m"

/*
    cl.exe (visual studio developer tools) build instructions:
        1. Gather * source files (src) into object files (obj)
            "cl /c .\src\*.c /Fo:.\obj\"

        2. Link object files (obj) into a final executable (bin)
            "link .\obj\*.obj /out:.\bin\webdredgen.exe"

    "cl /c .\src\*.c /Fo:.\obj\ ; link .\obj\*.obj /out:.\bin\webdredgen.exe ; .\bin\webdredgen.exe"
*/

int UNSUPPORTED_METHOD = 0;

DWORD customSendHttpResponse(HANDLE httpRequestQueue, PHTTP_REQUEST httpRequest,
    USHORT statusCode, PSTR httpReason, PSTR httpEntity);

// #define FREE_MEM(ptr) HeapFree(GetProcessHeap(), 0, (ptr)) 
// shows how we can define "define" directives for functions


DWORD WINAPI HandleHttpRequest(LPVOID lpParam)
{
    printf("** [PROC IN] HandleHttpRequest (FUNCTION) **\n");

    HANDLE httpRequestQueueHandle = (HANDLE)lpParam; // LPVOID allows us to typecast to any data type

    ULONG res;
    DWORD bytesRead;

    // process the request and send a response
    // obtain the request information
    HTTP_REQUEST_ID httpRequestId;
    PHTTP_REQUEST httpRequest;
    PCHAR httpRequestBuffer;
    ULONG httpRequestBufferLen;

    // allocate space for HTTP request
    httpRequestBufferLen = sizeof(HTTP_REQUEST) + 2048;
    httpRequestBuffer = (PCHAR) HeapAlloc(GetProcessHeap(), 0, httpRequestBufferLen);
    
    if (httpRequestBuffer == NULL)
    {
        printf("%s Failed to allocate buffer of 2KB for HTTP request buffer.\n", WEBD_ERROR);
        
        return -1;
    }

    httpRequest = (PHTTP_REQUEST)httpRequestBuffer;

    // wait for a new request
    HTTP_SET_NULL_ID(&httpRequestId);

    printf("HTTP_SET_NULL_ID\n");

    // generic flag construct for the server loop
    BOOL serverRunning = TRUE;

    while (serverRunning)
    {   
        printf("INFO: In while loop before RtlZeroMemory\n");

        RtlZeroMemory(httpRequest, httpRequestBufferLen);

        res = HttpReceiveHttpRequest(
            httpRequestQueueHandle,
            httpRequestId,
            0,
            httpRequest,
            httpRequestBufferLen,
            &bytesRead,
            NULL
        );

        printf("INFO: Setup res (HttpReceiveHttpRequest)\n");

        if (res == NO_ERROR)
        {
            switch (httpRequest->Verb)
            {
                case HttpVerbGET:
                {
                    wprintf(L"# HTTP GET request -> %hs\n", httpRequest->pRawUrl);

                    res = customSendHttpResponse(
                        httpRequestQueueHandle,
                        httpRequest,
                        200,
                        "OK",
                        "WEBDREDGEN SERVER v1.0\r\n"
                    );

                    break;
                }
                default:
                {
                    printf("GOT suspicious request, shutting down server...\n");
                    UNSUPPORTED_METHOD++;

                    return -1;
                }
            }
        }
        else if (res == ERROR_MORE_DATA)
        {
            // could not obtain data
            printf("Error: No more data!\n");

            /*
                input buffer is too small to hold the request headers
                increase the size and call again

                when calling the api again handle the request that failed by passing 
                a request id, the request id is from the old buffer
            */
            httpRequestId = httpRequest->RequestId;

            // free the old buffer and allocate enough memory
            httpRequestBufferLen = bytesRead;
            HeapFree(GetCurrentProcess(), 0, httpRequestBuffer);
            
            httpRequestBuffer = (PCHAR) HeapAlloc(GetCurrentProcess(), 0, httpRequestBufferLen);

            if (httpRequestBuffer == NULL)
            {
                printf("%s Failed to allocate heap memory for a new buffer (RESIZE HTTP BUFFER)\n", WEBD_ERROR);
            
                return -1;
            }
        
            httpRequest = (PHTTP_REQUEST)httpRequestBuffer;
        }
        else if (res == ERROR_CONNECTION_INVALID && !HTTP_IS_NULL_ID(&httpRequestId))
        {
            /*
                the TCP connection was corrupted by the peer when attempting to handle
                a request with more data to buffer

                handle the next request
            */

            printf("ERROR FATAL: TCP connection corrupted via remote peer!\n");

            HTTP_SET_NULL_ID(&httpRequestId);
        }
        else
        {   
            serverRunning = FALSE;
        }
    } // end

    if (httpRequestBuffer)
        HeapFree(GetCurrentProcess(), 0, httpRequestBuffer);

    // printf("RES => %lu\n", res);

    return res;
}


int main(void)
{
    ULONG res;

    HTTP_URL_GROUP_ID httpUrlGroupId;
    HTTP_SERVER_SESSION_ID httpServerSessionId;

    HTTPAPI_VERSION httpApiVersion;

    httpApiVersion.HttpApiMajorVersion = 2;
    httpApiVersion.HttpApiMinorVersion = 0;

    /*
        initializes the HTTP Server API driver, starts it, if it has not already been started
        and allocates data structures for the calling application to support response-queue
        creation and other operations
    */
    res = HttpInitialize(httpApiVersion, HTTP_INITIALIZE_SERVER, NULL);

    if (res != NO_ERROR)
    {
        printf("%s Failed to initialize the HTTP server API driver. Could not allocate data nor start server.\n", WEBD_ERROR);

        if (res == ERROR_INVALID_PARAMETER)
        {
            printf("%s The flags parameter contains an unsupported value.\n", WEBD_ERROR);

            return -1;
        }

        DWORD error = GetLastError();
        LPWSTR errorMsg = returnMsgBuffer(error);

        printf("%s", WEBD_ERROR);
        wprintf(L" %s\n", errorMsg);
        LocalFree(errorMsg);
        
        return -1;
    }

    printf("%s Initialized HTTP server API...\n", WEBD_SUCCESS);

    res = HttpCreateServerSession(httpApiVersion, &httpServerSessionId, 0);

    if (res != NO_ERROR)
    {
        HRESULT sysError = HRESULT_FROM_WIN32(res);
        LPWSTR error = returnMsgBuffer(sysError);

        printf("%s", WEBD_ERROR);
        wprintf(L" %s\n", error);
        LocalFree(error);

        handleHttpTermination();
    }

    printf("%s Created HTTP server session successfully...\n", WEBD_SUCCESS);

    HANDLE httpRequestQueueHandle;

    res = HttpCreateRequestQueue(
        httpApiVersion, 
        L"WEBDREDGEN SERVER QUEUE", 
        NULL,
        HTTP_CREATE_REQUEST_QUEUE_FLAG_CONTROLLER,
        &httpRequestQueueHandle
    );

    if (res != NO_ERROR)
    {
        switch (res)
        {
            case ERROR_REVISION_MISMATCH:
            {
                printf("%s The version parameter contains an invalid version.\n", WEBD_ERROR);

                break;
            }
            case ERROR_INVALID_PARAMETER:
                break;

            case ERROR_ALREADY_EXISTS:
            {
                printf("%s The pName parameter conflicts with an existing request queue that contains an identical name.\n", WEBD_ERROR);

                break;
            }
            case ERROR_ACCESS_DENIED:
            {
                printf("%s The calling process does not have permission to open the request queue.\n", WEBD_ERROR);

                break;
            }
            case ERROR_DLL_INIT_FAILED:
            {
                printf("%s The application has not called HttpInitialize prior to calling HttpCreateRequestQueue.\n", WEBD_ERROR);

                break;
            }

            default:
            {
                DWORD error = GetLastError();
                LPWSTR errorMsg = returnMsgBuffer(error);

                printf("%s", WEBD_ERROR);
                wprintf(L" %s\n", errorMsg);
                LocalFree(errorMsg);

                break;
            }
        }

        handleHttpTermination();
    }
    else if (res == NO_ERROR)
    {
        printf("%s Created HTTP request queue successfully...\n\n", WEBD_SUCCESS);    
    }

    /*
        Creates a URL Group under the specified server session.

        URL Groups are configuration containers for a set of URLs. They are created under 
        the server session and inherit the configuration settings of the server session.
    */
    res = HttpCreateUrlGroup(httpServerSessionId, &httpUrlGroupId, 0);

    if (res != NO_ERROR)
    {
        printf("%s Failed to create URL group when calling HttpCreateUrlGroup.\n", WEBD_ERROR);

        if (res == ERROR_INVALID_PARAMETER)
        {
            printf("%s The ServerSessionId parameter indicates a non-existing Server Session.\n", WEBD_ERROR);
        }
        else 
        {
            HRESULT sysError = HRESULT_FROM_WIN32(res);
            LPWSTR error = returnMsgBuffer(sysError);

            printf("%s", WEBD_ERROR);
            wprintf(L" %s\n", error);
            LocalFree(error);
        }

        handleHttpCloseServerSession(httpServerSessionId);
        handleHttpTermination();
    }

    // find grey to make ++ grey
    printf("++ Created URL group under the current server session...\n");

    // // Add a URL to the URL group
    res = HttpAddUrlToUrlGroup(httpUrlGroupId, L"http://127.0.0.1:8080/", 0, 0);

    if (res != NO_ERROR)
    {
        switch (res)
        {
            case ERROR_INVALID_PARAMETER:
            {
                printf("%s The URL group does not exist or the application does not have permission to add URLs to the group.\n", WEBD_ERROR);
            
                break;
            }
            case ERROR_ACCESS_DENIED:
            {
                printf("%s The calling process does not have permission to register the URL.\n", WEBD_ERROR);

                break;
            }
            case ERROR_ALREADY_EXISTS:
            {
                printf("%s The specified URL conflicts with an existing registration.\n", WEBD_ERROR);

                break;
            }
            default:
            {
                DWORD error = GetLastError();
                LPWSTR errorMsg = returnMsgBuffer(error);

                printf("%s", WEBD_ERROR);
                wprintf(L" %s\n", errorMsg);
                LocalFree(errorMsg);

                break;
            }
        }

        handleHttpCloseUrlGroup(httpUrlGroupId);
        handleHttpCloseServerSession(httpServerSessionId);
        handleHttpTermination();
    }

    printf("++ Set primary domain URL to HTTP URL group. DOMAIN=\"local\"\n\n");
    printf("++ Added URL=\"http://127.0.0.1:8080/\" to HTTP URL group registration...\n");

    HTTP_BINDING_INFO httpBindingInfo;
    HTTP_PROPERTY_FLAGS httpPropertyFlags;

    memset(&httpBindingInfo, 0, sizeof(HTTP_BINDING_INFO));
    memset(&httpPropertyFlags, 0, sizeof(HTTP_PROPERTY_FLAGS));

    httpPropertyFlags.Present = 1; // enable the request queue to be bound to the current 
                                   // url group

    httpBindingInfo.RequestQueueHandle = httpRequestQueueHandle;
    httpBindingInfo.Flags = httpPropertyFlags;

    res = HttpSetUrlGroupProperty(
        httpUrlGroupId,
        HttpServerBindingProperty,
        &httpBindingInfo,
        sizeof(HTTP_BINDING_INFO)
    );

    if (res != NO_ERROR)
    {
        if (res == ERROR_INVALID_PARAMETER)
        {
            // too many response codes to fit within a single printf statement
            // call function to get the actual system message returned instead
            DWORD error = GetLastError();
            LPWSTR errorMsg = returnMsgBuffer(error);

            printf("%s", WEBD_ERROR);
            wprintf(L" %s\n", errorMsg);
            LocalFree(errorMsg);
        }
        else 
        {
            HRESULT sysError = HRESULT_FROM_WIN32(res);
            LPWSTR error = returnMsgBuffer(sysError);

            printf("%s ", WEBD_ERROR);
            wprintf(L"%s\n", error);
            LocalFree(error);
        }

        handleHttpCloseUrlGroup(httpUrlGroupId);
        handleHttpCloseServerSession(httpServerSessionId);
        handleHttpTermination();
    }
    
    // retrieves the next available HTTP request from the specified request queue either
    // synchronously or asynchronously
    DWORD httpThreadId;
    HANDLE httpThreadHandle;

    HandleHttpRequest(httpRequestQueueHandle);

    httpThreadHandle = CreateThread(NULL, 0, HandleHttpRequest, (LPVOID)httpRequestQueueHandle, 0, &httpThreadId);

    if (httpThreadHandle == NULL)
    {
        DWORD error = GetLastError();
        LPWSTR errorMsg = returnMsgBuffer(error);

        printf("%s ", WEBD_ERROR);
        wprintf(L"%s\n", errorMsg);
        LocalFree(errorMsg);

        handleHttpCloseRequestQueue(httpRequestQueueHandle);
        handleHttpTermination();

        return -1;
    }

    WaitForSingleObject(httpThreadHandle, INFINITE);

    // need to create an event state

    handleHttpCloseRequestQueue(httpRequestQueueHandle);
    handleHttpCloseUrlGroup(httpUrlGroupId);
    handleHttpCloseServerSession(httpServerSessionId);
    handleHttpTermination();

    printf("STATUS: Exiting.\n");

    return 0;
}


DWORD customSendHttpResponse(HANDLE httpRequestQueue, PHTTP_REQUEST httpRequest, USHORT statusCode, PSTR httpReason, PSTR httpEntity)
{
    DWORD res, bytesSent;

    HTTP_RESPONSE httpResponse;
    HTTP_DATA_CHUNK httpDataChunk;

    // init the HTTP struct
    do 
    {
        memset(&httpResponse, 0, sizeof(httpResponse));

        httpResponse.StatusCode = statusCode;
        httpResponse.pReason = httpReason;
        httpResponse.ReasonLength = (USHORT) strlen(httpReason);

    } while ((VARIANT_BOOL)0);

    // add a known header
    do
    {
        httpResponse.Headers.KnownHeaders[HttpHeaderContentType].pRawValue = "text/html";
        httpResponse.Headers.KnownHeaders[HttpHeaderContentType].RawValueLength = (USHORT) strlen("text/html");
    
    } while ((VARIANT_BOOL)0);

    if (httpEntity)
    {
        // exists
        httpDataChunk.DataChunkType = HttpDataChunkFromMemory;
        httpDataChunk.FromMemory.pBuffer = httpEntity;
        httpDataChunk.FromMemory.BufferLength = (ULONG) strlen(httpEntity);

        httpResponse.EntityChunkCount = 1;
        httpResponse.pEntityChunks = &httpDataChunk;
    }

    // dont need to specify the content length since the http entity body is sent in one call
    res = HttpSendHttpResponse(
        httpRequestQueue,
        httpRequest->RequestId,
        0,
        &httpResponse,
        NULL,
        &bytesSent,
        NULL,
        0,
        NULL,
        NULL
    );

    if (res != NO_ERROR)
    {
        HRESULT sysError = HRESULT_FROM_WIN32(res);
        LPWSTR error = returnMsgBuffer(sysError);

        printf("%s ", WEBD_ERROR);
        wprintf(L"Failed to send the HTTP response back to the client!\n");

        handleHttpCloseRequestQueue(httpRequestQueue);
        handleHttpTermination();
    }

    return res;
}