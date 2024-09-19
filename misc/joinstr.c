#include <Windows.h>
#include <wchar.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


int main(void)
{
    printf("START\n");

    LPWSTR scheme = L"http://";
    LPWSTR host = L"192.168.0.1";
    LPWSTR port = L"8080";

    // calculate the length of the string
    size_t joinLen = wcslen(scheme) + wcslen(host) + wcslen(port) + 2; // : and \0

    LPWSTR url = (LPWSTR) malloc(joinLen + sizeof(wchar_t)); // LPWSTR = wchar_t

    if (url == NULL)
    {
        printf("FAILED TO ALLOCATE MEMORY.\n"); 
        exit(1);
    }

    printf("HERE\n");

    // initializes the url buffer with an empty string, allowing the functions to work
    wcscpy_s(url, joinLen, L"");

    wcscat_s(url, joinLen * sizeof(wchar_t), scheme);
    wcscat_s(url, joinLen * sizeof(wchar_t), host);
    wcscat_s(url, joinLen * sizeof(wchar_t), L":");
    wcscat_s(url, joinLen * sizeof(wchar_t), port);

    wprintf(L"URL: %ls\n", url);
    free(url);

    return 0;
}