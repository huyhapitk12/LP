#ifndef LP_GUI_H
#define LP_GUI_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>

/* Window handle type */
typedef HWND LpGuiHandle;

static HINSTANCE _lp_gui_hInstance = NULL;
static int _lp_gui_class_registered = 0;

static LRESULT CALLBACK _lp_gui_wndproc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_DESTROY: PostQuitMessage(0); return 0;
        default: return DefWindowProcA(hwnd, msg, wParam, lParam);
    }
}

static inline int lp_gui_window(const char* title, int width, int height) {
    if (!_lp_gui_hInstance) _lp_gui_hInstance = GetModuleHandle(NULL);
    if (!_lp_gui_class_registered) {
        WNDCLASSA wc = {0};
        wc.lpfnWndProc = _lp_gui_wndproc;
        wc.hInstance = _lp_gui_hInstance;
        wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
        wc.lpszClassName = "LpGuiClass";
        wc.hCursor = LoadCursor(NULL, IDC_ARROW);
        RegisterClassA(&wc);
        _lp_gui_class_registered = 1;
    }
    HWND hwnd = CreateWindowExA(0, "LpGuiClass", title ? title : "LP GUI",
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT, width > 0 ? width : 640, height > 0 ? height : 480,
        NULL, NULL, _lp_gui_hInstance, NULL);
    return (int)(intptr_t)hwnd;
}

static inline int lp_gui_label(int parent, const char* text, int x, int y) {
    HWND hwnd = CreateWindowExA(0, "STATIC", text ? text : "",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        x, y, 300, 20, (HWND)(intptr_t)parent, NULL, _lp_gui_hInstance, NULL);
    return (int)(intptr_t)hwnd;
}

static inline int lp_gui_button(int parent, const char* text, int x, int y, int w, int h) {
    HWND hwnd = CreateWindowExA(0, "BUTTON", text ? text : "Button",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        x, y, w > 0 ? w : 100, h > 0 ? h : 30, (HWND)(intptr_t)parent, NULL, _lp_gui_hInstance, NULL);
    return (int)(intptr_t)hwnd;
}

static inline int lp_gui_input(int parent, int x, int y, int w, int h) {
    HWND hwnd = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "",
        WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
        x, y, w > 0 ? w : 200, h > 0 ? h : 25, (HWND)(intptr_t)parent, NULL, _lp_gui_hInstance, NULL);
    return (int)(intptr_t)hwnd;
}

static inline void lp_gui_set_text(int handle, const char* text) {
    SetWindowTextA((HWND)(intptr_t)handle, text ? text : "");
}

static inline const char* lp_gui_get_text(int handle) {
    int len = GetWindowTextLengthA((HWND)(intptr_t)handle);
    char* buf = (char*)malloc(len + 2);
    if (buf) { GetWindowTextA((HWND)(intptr_t)handle, buf, len + 1); buf[len] = '\0'; }
    return buf ? buf : strdup("");
}

static inline void lp_gui_run(int handle) {
    (void)handle;
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

static inline void lp_gui_message_box(const char* title, const char* message) {
    MessageBoxA(NULL, message ? message : "", title ? title : "LP", MB_OK | MB_ICONINFORMATION);
}

static inline int lp_gui_confirm(const char* title, const char* message) {
    return MessageBoxA(NULL, message ? message : "", title ? title : "LP", MB_YESNO | MB_ICONQUESTION) == IDYES;
}

#else
/* Linux/macOS fallback — console-based */
static inline int lp_gui_window(const char* title, int width, int height) {
    printf("[GUI] Window: %s (%dx%d)\n", title ? title : "LP GUI", width, height);
    return 1;
}
static inline int lp_gui_label(int parent, const char* text, int x, int y) {
    printf("[GUI] Label: %s at (%d,%d)\n", text ? text : "", x, y); return 1;
}
static inline int lp_gui_button(int parent, const char* text, int x, int y, int w, int h) {
    printf("[GUI] Button: %s at (%d,%d) %dx%d\n", text ? text : "Button", x, y, w, h); return 1;
}
static inline int lp_gui_input(int parent, int x, int y, int w, int h) {
    printf("[GUI] Input at (%d,%d) %dx%d\n", x, y, w, h); return 1;
}
static inline void lp_gui_set_text(int handle, const char* text) { (void)handle; (void)text; }
static inline const char* lp_gui_get_text(int handle) { (void)handle; return strdup(""); }
static inline void lp_gui_run(int handle) { (void)handle; printf("[GUI] Event loop (no-op on Linux)\n"); }
static inline void lp_gui_message_box(const char* title, const char* message) {
    printf("[%s] %s\n", title ? title : "LP", message ? message : "");
}
static inline int lp_gui_confirm(const char* title, const char* message) {
    printf("[%s] %s (y/n) ", title ? title : "LP", message ? message : "");
    return getchar() == 'y';
}
#endif

#endif
