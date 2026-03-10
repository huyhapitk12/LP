/*
 * LP Native HTTP Module — Zero-dependency HTTP requests
 * Uses WinHTTP on Windows.
 */

#ifndef LP_HTTP_H
#define LP_HTTP_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32)
  #include <windows.h>
  #include <winhttp.h>
  #pragma comment(lib, "winhttp.lib")
#else
  /* Basic POSIX HTTP using curl via popen for simplicity, or sockets later */
  #include <unistd.h>
#endif

/* lp_http_get(url) returns a dynamically allocated string containing the response body */
static inline char* lp_http_get(const char *url) {
#if defined(_WIN32)
    HINTERNET hSession = NULL, hConnect = NULL, hRequest = NULL;
    char *result = NULL;
    DWORD size = 0;
    DWORD downloaded = 0;
    
    /* We need to parse URL into host and path for WinHTTP */
    URL_COMPONENTS urlComp;
    memset(&urlComp, 0, sizeof(urlComp));
    urlComp.dwStructSize = sizeof(urlComp);
    
    /* Set lengths to non-zero to extract components */
    WCHAR hostName[256];
    WCHAR urlPath[1024];
    urlComp.lpszHostName = hostName;
    urlComp.dwHostNameLength = 256;
    urlComp.lpszUrlPath = urlPath;
    urlComp.dwUrlPathLength = 1024;
    
    /* Convert ASCII url to multi-byte for WinHTTP */
    int wlen = MultiByteToWideChar(CP_UTF8, 0, url, -1, NULL, 0);
    WCHAR *wurl = (WCHAR*)malloc(wlen * sizeof(WCHAR));
    MultiByteToWideChar(CP_UTF8, 0, url, -1, wurl, wlen);
    
    if (!WinHttpCrackUrl(wurl, 0, 0, &urlComp)) {
        free(wurl); return strdup("");
    }
    
    /* Initialize session */
    hSession = WinHttpOpen(L"LP HTTP/1.0",
                           WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                           WINHTTP_NO_PROXY_NAME,
                           WINHTTP_NO_PROXY_BYPASS, 0);
                           
    if (hSession) {
        hConnect = WinHttpConnect(hSession, hostName, urlComp.nPort, 0);
    }
    
    if (hConnect) {
        DWORD flags = (urlComp.nScheme == INTERNET_SCHEME_HTTPS) ? WINHTTP_FLAG_SECURE : 0;
        hRequest = WinHttpOpenRequest(hConnect, L"GET", urlPath, NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, flags);
    }
    
    int success = 0;
    if (hRequest) {
        success = WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0);
    }
    
    if (success) {
        success = WinHttpReceiveResponse(hRequest, NULL);
    }
    
    char *buffer = (char*)malloc(1);
    buffer[0] = '\0';
    int buffer_len = 0;
    
    if (success) {
        do {
            size = 0;
            if (!WinHttpQueryDataAvailable(hRequest, &size)) break;
            if (size == 0) break;
            
            char *chunk = (char*)malloc(size + 1);
            if (!WinHttpReadData(hRequest, (LPVOID)chunk, size, &downloaded)) {
                free(chunk);
                break;
            }
            
            buffer = (char*)realloc(buffer, buffer_len + downloaded + 1);
            memcpy(buffer + buffer_len, chunk, downloaded);
            buffer_len += downloaded;
            buffer[buffer_len] = '\0';
            
            free(chunk);
        } while (size > 0);
    }
    
    /* Cleanup */
    if (hRequest) WinHttpCloseHandle(hRequest);
    if (hConnect) WinHttpCloseHandle(hConnect);
    if (hSession) WinHttpCloseHandle(hSession);
    free(wurl);
    
    return buffer;
#else
    /* Fallback on Linux using curl */
    char cmd[1024];
    snprintf(cmd, sizeof(cmd), "curl -s \"%s\"", url);
    FILE *fp = popen(cmd, "r");
    if (!fp) return strdup("");
    
    char *buffer = (char*)malloc(1);
    buffer[0] = '\0';
    int buffer_len = 0;
    
    char chunk[4096];
    while (fgets(chunk, sizeof(chunk), fp) != NULL) {
        int len = strlen(chunk);
        buffer = (char*)realloc(buffer, buffer_len + len + 1);
        memcpy(buffer + buffer_len, chunk, len);
        buffer_len += len;
        buffer[buffer_len] = '\0';
    }
    pclose(fp);
    return buffer;
#endif
}

#endif /* LP_HTTP_H */
