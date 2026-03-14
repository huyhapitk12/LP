/*
 * LP Native HTTP Module — Zero-dependency HTTP requests
 * Uses WinHTTP on Windows.
 * 
 * SECURITY NOTE: On Linux, this module uses fork/exec with curl instead of popen()
 * to prevent command injection vulnerabilities. URLs are validated before use.
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
  /* Basic POSIX HTTP using curl via fork/exec for security */
  #include <unistd.h>
  #include <sys/wait.h>
  #include <fcntl.h>
  #include <errno.h>

  /* 
   * Security: Validate URL to prevent command injection
   * Returns 1 if URL appears safe, 0 otherwise
   */
  static inline int lp_url_is_safe(const char *url) {
      if (!url || !*url) return 0;
      
      /* URL must start with http:// or https:// */
      if (strncmp(url, "http://", 7) != 0 && strncmp(url, "https://", 8) != 0) {
          return 0;
      }
      
      /* Check for shell metacharacters that could enable command injection */
      for (const char *p = url; *p; p++) {
          switch (*p) {
              case ';': case '|': case '&': case '`': case '$':
              case '(': case ')': case '{': case '}': case '[': case ']':
              case '<': case '>': case '\'': case '"': case '\\':
              case '\n': case '\r': case '\0':
                  return 0;
              default:
                  break;
          }
      }
      return 1;
  }

  /* 
   * Security: Run curl with fork/exec and capture output via pipe
   * This avoids shell interpretation completely
   */
  static inline char* lp_curl_get(const char *url) {
      int pipefd[2];
      pid_t pid;
      
      if (!lp_url_is_safe(url)) {
          return strdup("");
      }
      
      if (pipe(pipefd) < 0) {
          return strdup("");
      }
      
      pid = fork();
      if (pid < 0) {
          close(pipefd[0]);
          close(pipefd[1]);
          return strdup("");
      }
      
      if (pid == 0) {
          /* Child process */
          close(pipefd[0]);
          dup2(pipefd[1], STDOUT_FILENO);
          close(pipefd[1]);
          
          /* Use execvp to avoid shell - curl receives URL as separate arg */
          const char *curl_args[] = {"curl", "-s", "-L", url, NULL};
          execvp("curl", (char *const *)curl_args);
          
          /* If exec fails */
          _exit(127);
      }
      
      /* Parent process */
      close(pipefd[1]);
      
      char *buffer = (char*)malloc(1);
      buffer[0] = '\0';
      size_t buffer_len = 0;
      size_t buffer_cap = 1;
      
      char chunk[4096];
      ssize_t n;
      while ((n = read(pipefd[0], chunk, sizeof(chunk))) > 0) {
          if (buffer_len + n >= buffer_cap) {
              buffer_cap = buffer_len + n + 1;
              char *new_buf = (char*)realloc(buffer, buffer_cap);
              if (!new_buf) {
                  free(buffer);
                  close(pipefd[0]);
                  waitpid(pid, NULL, 0);
                  return strdup("");
              }
              buffer = new_buf;
          }
          memcpy(buffer + buffer_len, chunk, n);
          buffer_len += n;
          buffer[buffer_len] = '\0';
      }
      
      close(pipefd[0]);
      waitpid(pid, NULL, 0);
      
      return buffer;
  }

  /* 
   * Security: Run curl POST with fork/exec
   */
  static inline char* lp_curl_post(const char *url, const char *data) {
      int pipefd[2];
      int data_pipe[2];
      pid_t pid;
      
      if (!lp_url_is_safe(url)) {
          return strdup("");
      }
      
      if (pipe(pipefd) < 0) {
          return strdup("");
      }
      
      /* Create pipe for POST data if needed */
      if (data && pipe(data_pipe) < 0) {
          close(pipefd[0]);
          close(pipefd[1]);
          return strdup("");
      }
      
      pid = fork();
      if (pid < 0) {
          close(pipefd[0]);
          close(pipefd[1]);
          if (data) {
              close(data_pipe[0]);
              close(data_pipe[1]);
          }
          return strdup("");
      }
      
      if (pid == 0) {
          /* Child process */
          close(pipefd[0]);
          dup2(pipefd[1], STDOUT_FILENO);
          close(pipefd[1]);
          
          if (data) {
              close(data_pipe[1]);
              dup2(data_pipe[0], STDIN_FILENO);
              close(data_pipe[0]);
              
              const char *curl_args[] = {"curl", "-s", "-L", "-X", "POST", 
                                         "-d", "@-", url, NULL};
              execvp("curl", (char *const *)curl_args);
          } else {
              const char *curl_args[] = {"curl", "-s", "-L", "-X", "POST", url, NULL};
              execvp("curl", (char *const *)curl_args);
          }
          
          _exit(127);
      }
      
      /* Parent process */
      close(pipefd[1]);
      
      if (data) {
          close(data_pipe[0]);
          /* Write POST data to child's stdin */
          size_t data_len = strlen(data);
          size_t written = 0;
          while (written < data_len) {
              ssize_t n = write(data_pipe[1], data + written, data_len - written);
              if (n < 0) {
                  if (errno == EINTR) continue;
                  break;
              }
              written += n;
          }
          close(data_pipe[1]);
      }
      
      char *buffer = (char*)malloc(1);
      buffer[0] = '\0';
      size_t buffer_len = 0;
      size_t buffer_cap = 1;
      
      char chunk[4096];
      ssize_t n;
      while ((n = read(pipefd[0], chunk, sizeof(chunk))) > 0) {
          if (buffer_len + n >= buffer_cap) {
              buffer_cap = buffer_len + n + 1;
              char *new_buf = (char*)realloc(buffer, buffer_cap);
              if (!new_buf) {
                  free(buffer);
                  close(pipefd[0]);
                  waitpid(pid, NULL, 0);
                  return strdup("");
              }
              buffer = new_buf;
          }
          memcpy(buffer + buffer_len, chunk, n);
          buffer_len += n;
          buffer[buffer_len] = '\0';
      }
      
      close(pipefd[0]);
      waitpid(pid, NULL, 0);
      
      return buffer;
  }
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
    /* SECURITY: Use fork/exec instead of popen() */
    return lp_curl_get(url);
#endif
}

/* lp_http_post(url, data) returns a dynamically allocated string containing the response body */
static inline char* lp_http_post(const char *url, const char *data) {
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
        hRequest = WinHttpOpenRequest(hConnect, L"POST", urlPath, NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, flags);
    }
    
    int success = 0;
    if (hRequest) {
        /* Set standard Content-Type for POST, although the user can define JSON/form later */
        LPCWSTR header = L"Content-Type: application/x-www-form-urlencoded\r\n";
        WinHttpAddRequestHeaders(hRequest, header, (ULONG)-1L, WINHTTP_ADDREQ_FLAG_ADD);
        
        DWORD data_len = data ? (DWORD)strlen(data) : 0;
        success = WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, (LPVOID)data, data_len, data_len, 0);
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
    /* SECURITY: Use fork/exec instead of popen() */
    return lp_curl_post(url, data);
#endif
}

#endif /* LP_HTTP_H */
