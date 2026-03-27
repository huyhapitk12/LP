#ifndef LP_SOCKET_H
#define LP_SOCKET_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
  #ifndef _WINSOCKAPI_
    #include <winsock2.h>
    #include <ws2tcpip.h>
  #endif
  #pragma comment(lib, "ws2_32.lib")
#else
  #include <sys/socket.h>
  #include <netinet/in.h>
  #include <arpa/inet.h>
  #include <netdb.h>
  #include <unistd.h>
  #include <fcntl.h>
  #define SOCKET int
  #define INVALID_SOCKET (-1)
  #define closesocket close
#endif

/* Initialize Winsock on Windows */
static inline void lp_socket_init_env() {
#ifdef _WIN32
    static int initialized = 0;
    if (!initialized) {
        WSADATA wsaData;
        WSAStartup(MAKEWORD(2, 2), &wsaData);
        initialized = 1;
    }
#endif
}

/* Pre-allocated recv buffer pool — avoids malloc/free per recv() call */
#define LP_SOCKET_RECV_POOL_SIZE 8192
static char lp_socket_recv_pool[LP_SOCKET_RECV_POOL_SIZE];

static inline int lp_socket_create(const char* type_str) {
    lp_socket_init_env();
    int type = SOCK_STREAM;
    if (type_str && (strcmp(type_str, "udp") == 0 || strcmp(type_str, "UDP") == 0)) {
        type = SOCK_DGRAM;
    }
    SOCKET sock = socket(AF_INET, type, 0);
    return (int)sock;
}

static inline int lp_socket_connect(int sock, const char* host, int port) {
    if (sock == (int)INVALID_SOCKET || !host) return -1;
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons((unsigned short)port);
    struct hostent* he = gethostbyname(host);
    if (!he) return -1;
    memcpy(&addr.sin_addr, he->h_addr_list[0], he->h_length);
    return connect((SOCKET)sock, (struct sockaddr*)&addr, sizeof(addr));
}

static inline int lp_socket_bind(int sock, const char* host, int port) {
    if (sock == (int)INVALID_SOCKET) return -1;
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons((unsigned short)port);
    if (host && strlen(host) > 0) addr.sin_addr.s_addr = inet_addr(host);
    else addr.sin_addr.s_addr = INADDR_ANY;
    return bind((SOCKET)sock, (struct sockaddr*)&addr, sizeof(addr));
}

static inline int lp_socket_listen(int sock, int backlog) {
    if (sock == (int)INVALID_SOCKET) return -1;
    return listen((SOCKET)sock, backlog > 0 ? backlog : 5);
}

static inline int lp_socket_accept(int sock) {
    if (sock == (int)INVALID_SOCKET) return (int)INVALID_SOCKET;
    struct sockaddr_in client_addr;
    int addrlen = sizeof(client_addr);
#ifdef _WIN32
    return (int)accept((SOCKET)sock, (struct sockaddr*)&client_addr, &addrlen);
#else
    return (int)accept((SOCKET)sock, (struct sockaddr*)&client_addr, (socklen_t*)&addrlen);
#endif
}

static inline int lp_socket_send(int sock, const char* data) {
    if (sock == (int)INVALID_SOCKET || !data) return 0;
    return (int)send((SOCKET)sock, data, (int)strlen(data), 0);
}

static inline const char* lp_socket_recv(int sock, int bufsize) {
    if (sock == (int)INVALID_SOCKET || bufsize <= 0) return strdup("");
    
    /* Fast path: use pre-allocated pool for small recv */
    if (bufsize <= LP_SOCKET_RECV_POOL_SIZE) {
        int bytes_received = recv((SOCKET)sock, lp_socket_recv_pool, bufsize, 0);
        if (bytes_received > 0) {
            lp_socket_recv_pool[bytes_received] = '\0';
            return strdup(lp_socket_recv_pool);
        }
        return strdup("");
    }
    
    /* Large recv: allocate dynamically */
    char* buffer = (char*)malloc(bufsize + 1);
    if (!buffer) return strdup("");
    int bytes_received = recv((SOCKET)sock, buffer, bufsize, 0);
    if (bytes_received > 0) {
        buffer[bytes_received] = '\0';
        return buffer;
    }
    free(buffer);
    return strdup("");
}

static inline void lp_socket_close(int sock) {
    if (sock != (int)INVALID_SOCKET) closesocket((SOCKET)sock);
}

static inline void lp_socket_set_nonblocking(int sock, int enabled) {
    if (sock == (int)INVALID_SOCKET) return;
#ifdef _WIN32
    u_long mode = enabled ? 1 : 0;
    ioctlsocket((SOCKET)sock, FIONBIO, &mode);
#else
    int flags = fcntl((SOCKET)sock, F_GETFL, 0);
    if (enabled) flags |= O_NONBLOCK;
    else flags &= ~O_NONBLOCK;
    fcntl((SOCKET)sock, F_SETFL, flags);
#endif
}

static inline int lp_socket_sendto(int sock, const char* data, const char* host, int port) {
    if (sock == (int)INVALID_SOCKET || !data || !host) return 0;
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons((unsigned short)port);
    addr.sin_addr.s_addr = inet_addr(host);
    return (int)sendto((SOCKET)sock, data, (int)strlen(data), 0, (struct sockaddr*)&addr, sizeof(addr));
}

static inline const char* lp_socket_recvfrom(int sock, int bufsize) {
    if (sock == (int)INVALID_SOCKET || bufsize <= 0) return strdup("");
    char* buffer = (char*)malloc(bufsize + 1);
    if (!buffer) return strdup("");
    struct sockaddr_in from_addr;
    int from_len = sizeof(from_addr);
#ifdef _WIN32
    int bytes = recvfrom((SOCKET)sock, buffer, bufsize, 0, (struct sockaddr*)&from_addr, &from_len);
#else
    int bytes = recvfrom((SOCKET)sock, buffer, bufsize, 0, (struct sockaddr*)&from_addr, (socklen_t*)&from_len);
#endif
    if (bytes > 0) { buffer[bytes] = '\0'; return buffer; }
    free(buffer);
    return strdup("");
}

static inline const char* lp_socket_get_host_by_name(const char* hostname) {
    lp_socket_init_env();
    if (!hostname) return strdup("0.0.0.0");
    struct hostent* he = gethostbyname(hostname);
    if (he && he->h_addr_list[0]) {
        struct in_addr addr;
        memcpy(&addr, he->h_addr_list[0], sizeof(struct in_addr));
        return strdup(inet_ntoa(addr));
    }
    return strdup("0.0.0.0");
}

static inline const char* lp_socket_get_local_ip() {
    lp_socket_init_env();
    char hostname[256];
    if (gethostname(hostname, sizeof(hostname)) == 0) {
        return lp_socket_get_host_by_name(hostname);
    }
    return strdup("127.0.0.1");
}

/* select() wrapper — check if socket is ready for read/write with timeout_ms */
static inline int lp_socket_select_read(int sock, int timeout_ms) {
    if (sock == (int)INVALID_SOCKET) return 0;
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET((SOCKET)sock, &readfds);
    struct timeval tv;
    tv.tv_sec = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;
    return select((int)sock + 1, &readfds, NULL, NULL, &tv) > 0;
}

static inline int lp_socket_select_write(int sock, int timeout_ms) {
    if (sock == (int)INVALID_SOCKET) return 0;
    fd_set writefds;
    FD_ZERO(&writefds);
    FD_SET((SOCKET)sock, &writefds);
    struct timeval tv;
    tv.tv_sec = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;
    return select((int)sock + 1, NULL, &writefds, NULL, &tv) > 0;
}

#endif
