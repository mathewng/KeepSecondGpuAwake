// /home/mathew/Development/Github/KeepSecondGpuAwake/src/d3d9/base.h
#pragma once
#include <stdarg.h>
#include <stdalign.h>  // C23: alignment utilities
#include <stddef.h>    // C23: size_t, NULL
#include <windows.h>
#include <d3d11.h>
#include <shellapi.h>
#include "resource.h"

// C23: Compile-time assertions for critical sizes
_Static_assert(sizeof(HANDLE) == sizeof(void*), "HANDLE size mismatch");
_Static_assert(sizeof(HWND) == sizeof(void*), "HWND size mismatch");
_Static_assert(sizeof(HICON) == sizeof(void*), "HICON size mismatch");
_Static_assert(sizeof(HINSTANCE) == sizeof(void*), "HINSTANCE size mismatch");

#define APP_ID  "KeepSecondGpuAwake_d3d11"
#define APP_TITLE  "KeepSecondGpuAwake"
#define CAT__(a,b)  a ## b
#define CAT(a,b)  CAT__(a,b)
#define SAFEFREE(p,fn)  if (p) { (fn)(p); (p) = 0; }
#define SAFERELEASE(p)  if (p) { p->lpVtbl->Release(p); (p) = 0; }

// C23: Inline implementations for standard functions (per AGENTS.md - no VC++ redistributable)
static inline void *inline_memset(void *dest, int ch, size_t count) {
    unsigned char *d = (unsigned char *)dest;
    while (count--) *d++ = (unsigned char)ch;
    return dest;
}

static inline void *inline_memcpy(void *dest, const void *src, size_t count) {
    unsigned char *d = (unsigned char *)dest;
    const unsigned char *s = (const unsigned char *)src;
    while (count--) *d++ = *s++;
    return dest;
}

static inline int inline_memcmp(const void *s1, const void *s2, size_t count) {
    const unsigned char *p1 = (const unsigned char *)s1;
    const unsigned char *p2 = (const unsigned char *)s2;
    while (count--) {
        if (*p1 != *p2) return (*p1 > *p2) ? 1 : -1;
        p1++; p2++;
    }
    return 0;
}

#ifdef UNICODE
#define H__(a,b)  extern WCHAR a[sizeof(b)];
#else
#define H__(a,b)  static PCSTR const a = b;
#endif
#include "szlitera.h"

extern HINSTANCE const * const x_p_hInst;
#define APP_hInst  (*x_p_hInst)
extern HICON const * const x_p_hIcon;
#define APP_hIcon  (*x_p_hIcon)
extern UINT const * const x_p_wmTaskbarCreated;
#define WM_TaskbarCreated  (*x_p_wmTaskbarCreated)
extern HWND const * const x_p_hMainWnd;
#define APP_MainWnd  (*x_p_hMainWnd)

typedef struct AppCfg {
	UINT wake_interval; // in millisecond
#define APPCFGDEF_WAKE_INTERVAL  (10000)
	char gpu_filter[256];
} AppCfg;

extern AppCfg const * const x_p_appCfg;
#define APP_Cfg  (*x_p_appCfg)

typedef struct NotifyIconDataV1 {
	DWORD cbSize;
	HWND hWnd;
	UINT uID;
	UINT uFlags;
	UINT uCallbackMessage;
	HICON hIcon;
	TCHAR szTip[64];
} NotifyIconDataV1;

void *MemAllocZero(SIZE_T cb);
void MemFree(void *p);

void AppWinErrSetW32(PCSTR pszProc, DWORD w32err);
void AppWinErrSetHR(PCSTR pszProc, HRESULT hr);
void AppWinErrReset(void);

int App_fWrite(HANDLE hFile, void const *pBuf, int cbBuf);
int App_vfPrnf(HANDLE hFile, char const *pszFmt, va_list ap);
int App_fPrnf(HANDLE hFile, char const *pszFmt, ...);

// " 123" -> TRUE, 123
// "123" -> TRUE, 123
// "12,3" -> FALSE
// "-123" -> FALSE
// "123a" -> FALSE
BOOL App_tcs_to_UINT(LPCTSTR lpsz, UINT* pVal);

void PrnExoticErr_(HANDLE hFile, PCSTR pszCtx);
void PrnNow_(HANDLE hFile);
void PrnNowExoticErr(PCSTR pszCtx);
void PrnNowErr(PCSTR pszFmt, ...);
void PrnNowOut(PCSTR pszFmt, ...);
void PrnErr(char const *pszFmt, ...);
void PrnOut(char const *pszFmt, ...);

int IsWindows11OrGreater(void);
int ResolveAdapterIndex(void);