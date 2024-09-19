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


DWORD WINAPI HandleHttpRequest(LPVOID lpParam)
{
    HANDLE httpRequestHandle = (HANDLE)lpParam; // LPVOID allows us to typecast to any data type

    // process the request and send a response
    // obtain the request information
    ULONG res;
    ULONG bytesRead;

    HTTP_REQUEST httpRequest;
    memset(&httpRequest, 0, sizeof(HTTP_REQUEST));

    res = HttpReceiveHttpRequest(
        httpRequestHandle,
        httpRequest.RequestId,
        0,
        &httpRequest,
        sizeof(HTTP_REQUEST),
        &bytesRead,
        NULL
    );

    if (res != NO_ERROR)
    {
        // too many response codes to fit within a single printf statement
        // call function to get the actual system message returned instead
        DWORD error = GetLastError();
        wprintf(L"ERROR: %s\n", returnMsgBuffer(error));

        handleHttpTermination();
    }
    else if (res == NO_ERROR)
    {
        if (httpRequest.Verb == HttpVerbGET)
        {
            // generate HTTP response
            HTTP_RESPONSE_V2 httpResponse;
            memset(&httpResponse, 0, sizeof(HTTP_RESPONSE_V2));

            httpResponse.StatusCode = 200; // HTTP-OK
            httpResponse.ReasonLength = 0;
            httpResponse.pReason = "OK";
            httpResponse.Headers.KnownHeaders[HttpHeaderContentType].pRawValue = "text/plain";
            httpResponse.Headers.KnownHeaders[HttpHeaderContentType].RawValueLength = sizeof("text/plain"); // 1 = NULL terminator
            httpResponse.EntityChunkCount = 1;

            /*
                The HTTP_DATA_CHUNK structure represents an individual block of data either in memory
                in a file, or in the HTTP Server API response-fragment cache.
            
                Sets the entity chunk data within the HTTP response body.
            */
            HTTP_DATA_CHUNK httpEntityChunk;
            memset(&httpEntityChunk, 0, sizeof(HTTP_DATA_CHUNK));
            
            // set HTTP response body content
            const char *httpResponseContent = "Hello, World!";

            // The data source is a memory data block. The union should be interpreted as a FromMemory structure.
            httpEntityChunk.DataChunkType = HttpDataChunkFromMemory;

            // Set HTTP response body content
            httpEntityChunk.FromMemory.pBuffer = (PVOID)httpResponseContent;
            httpEntityChunk.FromMemory.BufferLength = (ULONG) strlen(httpResponseContent) + 1;

            // Associate the entity chunk with the HTTP response
            httpResponse.pEntityChunks = &httpEntityChunk;

            ULONG bytesSent;
            ULONG flags = 0;

            res = HttpSendHttpResponse(
                httpRequestHandle,
                httpRequest.RequestId,
                flags,
                (PHTTP_RESPONSE)&httpResponse,
                NULL, // CachePolicy, must be NULL
                &bytesSent,
                NULL, // Reserved1, must be NULL
                0, // Reserved2, must be 0
                NULL, // synchronous calls
                NULL
            );

            if (res != NO_ERROR)
            {
                printf("FAILED: Could not send HTTP response to client.\n");

                DWORD error = GetLastError();
                LPWSTR errorMsg = returnMsgBuffer(error);

                wprintf(L"ERROR: %s\n", errorMsg);

                LocalFree(errorMsg);
                handleHttpTermination();
            }
            else
            {
                printf("SENT HTTP RESPONSE TO REMOTE CLIENT.\n");
            }
        }
    }

    return 0;
}


int main(void)
{
    ULONG res;
    ULONG httpTerminateRes;

    // HTTP_REQUEST httpRequestStruct;
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
        printf("Failed to initialize the HTTP server API driver. Could not allocate data nor start server.\n");

        // check flags param
        if (res == ERROR_INVALID_PARAMETER)
        {
            printf("ERROR: The flags parameter contains an unsupported value.\n");

            return -1;
        }

        DWORD error = GetLastError();
        LPWSTR errorMsg = returnMsgBuffer(error);

        wprintf(L"ERROR: %s\n", errorMsg);
        LocalFree(errorMsg);
        
        return -1;
    }

    printf("%s Initialized HTTP server API...\n", WEBD_SUCCESS);

    res = HttpCreateServerSession(httpApiVersion, &httpServerSessionId, 0);

    if (res != NO_ERROR)
    {
        HRESULT sysError = HRESULT_FROM_WIN32(res);
        LPWSTR error = returnMsgBuffer(sysError);

        wprintf(L"ERROR: %s\n", error);
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
                printf("ERROR: The version parameter contains an invalid version.\n");

                break;
            }
            case ERROR_INVALID_PARAMETER:
                break;

            case ERROR_ALREADY_EXISTS:
            {
                printf("ERROR: The pName parameter conflicts with an existing request queue that contains an identical name.\n");

                break;
            }
            case ERROR_ACCESS_DENIED:
            {
                printf("ERROR: The calling process does not have permission to open the request queue.\n");

                break;
            }
            case ERROR_DLL_INIT_FAILED:
            {
                printf("ERROR: The application has not called HttpInitialize prior to calling HttpCreateRequestQueue.\n");

                break;
            }

            default:
            {
                DWORD error = GetLastError();
                LPWSTR errorMsg = returnMsgBuffer(error);

                wprintf(L"ERROR: %s\n", errorMsg);
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


    // res = HttpAddUrl(httpRequestQueueHandle, L"http://127.0.0.1:8080/", NULL);

    // if (res != NO_ERROR)
    // {
    //     switch (res)
    //     {
    //         case ERROR_ACCESS_DENIED:
    //         {
    //             printf("ERROR: The calling application does not have permission to register the URL.\n");

    //             break;
    //         }
    //         case ERROR_DLL_INIT_FAILED:
    //         {
    //             printf("ERROR: The calling application did not call HttpInitialize before calling this function.\n");

    //             break;
    //         }
    //         case ERROR_INVALID_PARAMETER:
    //         {
    //             printf("ERROdsafsdaR: One of the parameters are invalid.\n");

    //             break;
    //         }
    //         case ERROR_ALREADY_EXISTS:
    //         {
    //             printf("ERROR: The specified UrlPrefix conflicts with an existing registration.\n");

    //             break;
    //         }
    //         case ERROR_NOT_ENOUGH_MEMORY:
    //         {
    //             printf("ERROR: Insufficient resources to complete the operation.\n");

    //             break;
    //         }
    //         default:
    //         {
    //             HRESULT sysError = HRESULT_FROM_WIN32(res);
    //             LPWSTR errorMsg = returnMsgBuffer(sysError);

    //             printf("%s ", WEBD_ERROR);
    //             wprintf(L"sfdsafa%s\n", errorMsg);
    //             LocalFree(errorMsg);

    //             break;
    //         }
    //     }

    //     handleHttpCloseRequestQueue(httpRequestQueueHandle);
    //     handleHttpTermination();
    // }

    // printf("++ Added URL=\"http://127.0.0.1:8080/\" to HTTP registration...\n");

    // /*
    //     Creates a URL Group under the specified server session.

    //     URL Groups are configuration containers for a set of URLs. They are created under 
    //     the server session and inherit the configuration settings of the server session.
    // */

    res = HttpCreateUrlGroup(httpServerSessionId, &httpUrlGroupId, 0);

    if (res != NO_ERROR)
    {
        printf("Failed to create URL group when calling HttpCreateUrlGroup.\n");

        if (res == ERROR_INVALID_PARAMETER)
        {
            printf("ERROR: The ServerSessionId parameter indicates a non-existing Server Session.\n");
        }
        else 
        {
            HRESULT sysError = HRESULT_FROM_WIN32(res);
            LPWSTR error = returnMsgBuffer(sysError);

            wprintf(L"ERROR: %s\n", error);
            LocalFree(error);
        }

        handleHttpCloseServerSession(httpServerSessionId);
        handleHttpTermination();
    }

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

    HTTP_BINDING_INFO httpBindingInfo;
    HTTP_PROPERTY_FLAGS httpPropertyFlags;

    memset(&httpBindingInfo, 0, sizeof(HTTP_BINDING_INFO));
    memset(&httpPropertyFlags, 0, sizeof(HTTP_PROPERTY_FLAGS));

    httpPropertyFlags.Present = 1;

    httpBindingInfo.RequestQueueHandle = httpRequestQueueHandle;
    httpBindingInfo.Flags = httpPropertyFlags;

    res = HttpSetUrlGroupProperty(
        httpUrlGroupId,
        HttpServerBindingProperty,
        &httpBindingInfo,
        (ULONG)sizeof(HTTP_BINDING_INFO)
    );

    if (res != NO_ERROR)
    {
        if (res == ERROR_INVALID_PARAMETER)
        {
            // too many response codes to fit within a single printf statement
            // call function to get the actual system message returned instead
            DWORD error = GetLastError();
            LPWSTR errorMsg = returnMsgBuffer(error);

            wprintf(L"ERROR: %s\n", errorMsg);
            LocalFree(errorMsg);
        }
        else 
        {
            HRESULT sysError = HRESULT_FROM_WIN32(res);
            LPWSTR error = returnMsgBuffer(sysError);

            wprintf(L"ERROR: %s\n", error);
            LocalFree(error);
        }

        handleHttpCloseUrlGroup(httpUrlGroupId);
        handleHttpCloseServerSession(httpServerSessionId);
        handleHttpTermination();
    }
    
    HTTP_REQUEST_ID httpRequestId = HTTP_NULL_ID;
    
    HTTP_REQUEST httpRequestBuffer;
    memset(&httpRequestBuffer, 0, sizeof(HTTP_REQUEST));

    ULONG requestBufferLen = sizeof(HTTP_REQUEST);
    ULONG bytesRet; // bytes returned
    ULONG flags = 0;

    // retrieves the next available HTTP request from the specified request queue either
    // synchronously or asynchronously
    while (1)
    {
        printf("++ Before calling HttpRecieveHttpRequest\n");

        res = HttpReceiveHttpRequest(
            httpRequestQueueHandle, 
            httpRequestId, // should only be set for the first call, set custom logic?
            flags,
            (PHTTP_REQUEST)&httpRequestBuffer,
            requestBufferLen,
            &bytesRet,
            NULL // NULL = synchronous calls
        );

        printf("++ After calling HttpReceiveHttpRequest\n\n");

        getchar();

        if (res == ERROR_MORE_DATA)
        {
            printf("++ Got more data from web client...\n");

            requestBufferLen += bytesRet;
            continue;
        }

        else if (res != NO_ERROR || res != ERROR_MORE_DATA || res != NO_ERROR && res != ERROR_MORE_DATA)
        {
            printf("Waiting...\n");

            switch (res)
            {
                case ERROR_INVALID_PARAMETER:
                {
                    printf("ERROR: One or more of the supplied parameters is in an unusable form.\n");

                    break;
                }
                
                default:
                {
                    HRESULT sysError = HRESULT_FROM_WIN32(res);
                    LPWSTR errorMsg = returnMsgBuffer(sysError);

                    printf("%s", WEBD_ERROR); // hacky but who cares
                    wprintf(L" (%0x): %s\n", sysError, errorMsg);
                    LocalFree(errorMsg);

                    break;
                }
            }

            handleHttpCloseRequestQueue(httpRequestQueueHandle);
            handleHttpCloseUrlGroup(httpUrlGroupId);
            handleHttpCloseServerSession(httpServerSessionId);
            handleHttpTermination();
         
            // DWORD httpThreadId;
            // HANDLE httpThreadHandle = CreateThread(NULL, 0, HandleHttpRequest, (LPVOID)httpRequestQueueHandle, 0, &httpThreadId);
        
            // if (httpThreadHandle == NULL)
            // {
            //     DWORD error = GetLastError();
            //     LPWSTR errorMsg = returnMsgBuffer(error);

            //     wprintf(L"ERROR: %s\n", errorMsg);
            //     LocalFree(errorMsg);

            //     CloseHandle(httpThreadHandle);
            //     break;
            // }
        }
    }

    handleHttpCloseRequestQueue(httpRequestQueueHandle);
    handleHttpCloseUrlGroup(httpUrlGroupId);
    handleHttpCloseServerSession(httpServerSessionId);
    handleHttpTermination();

    return 0;
}