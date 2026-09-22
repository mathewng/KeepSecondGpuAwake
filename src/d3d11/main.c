#include "main.hh"
#include <string.h>

void *MemAllocZero(SIZE_T cb)
{
	void *p = HeapAlloc(s_hHeap, HEAP_ZERO_MEMORY, cb);
	if (!p) {
		AppWinErrSetW32("HeapAlloc", GetLastError());
	}
	return p;
}

void MemFree(void *p)
{
	HeapFree(s_hHeap, 0, p);
}

void AppWinErrSetW32(PCSTR pszProc, DWORD w32err)
{
	s_AppWinErrProc = pszProc;
	s_AppWinErrCode = w32err;
	s_AppWinErrCodeTy = AppWinErrCodeTy_W32;
}

void AppWinErrSetHR(PCSTR pszProc, HRESULT hr)
{
	s_AppWinErrProc = pszProc;
	s_AppWinErrCode = hr;
	s_AppWinErrCodeTy = AppWinErrCodeTy_HR;
}

void AppWinErrReset(void)
{
	s_AppWinErrProc = NULL;
	s_AppWinErrCode = 0;
	s_AppWinErrCodeTy = AppWinErrCodeTy_None;
}

int App_fWrite(HANDLE hFile, void const *pBuf, int cbBuf)
{
	DWORD cbWri;
	if (!WriteFile(hFile, pBuf, cbBuf, &cbWri, NULL)) {
		AppWinErrSetW32("WriteFile", GetLastError());
		return -1;
	}
	return (int)cbWri;
}

int App_vfPrnf(HANDLE hFile, char const *pszFmt, va_list ap)
{
	char szBuf[1024];
	int cbBuf = wvsprintfA(szBuf, pszFmt, ap);
	return App_fWrite(hFile, szBuf, cbBuf);
}

int App_fPrnf(HANDLE hFile, char const *pszFmt, ...)
{
	int ret = 0;
	va_list ap;
	va_start(ap, pszFmt);
	ret = App_vfPrnf(hFile, pszFmt, ap);
	va_end(ap);
	return ret;
}

BOOL App_tcs_to_UINT(LPCTSTR lpsz, UINT* pVal)
{
	BOOL ok = FALSE;
	HWND hDlg = 0;
	HWND hCtl = 0;
	hDlg = CreateWindow(TEXT("edit"), 0, WS_POPUP, 0, 0, 0, 0,
		0, 0, 0, 0);
	if (!hDlg) goto eof;
	hCtl = CreateWindow(TEXT("edit"), 0, WS_CHILD, 0, 0, 0, 0,
		hDlg, (HMENU)(WPARAM)(1234), 0, 0);
	if (!hCtl) goto eof;
	SetDlgItemText(hDlg, 1234, lpsz);
	*pVal = GetDlgItemInt(hDlg, 1234, &ok, FALSE);
eof:
	// hCtl will be destroyed when hDlg is destroyed
	if (hDlg) DestroyWindow(hDlg);
	return ok;
}

void PrnExoticErr_(HANDLE hFile, PCSTR pszCtx)
{
	if (s_AppWinErrProc) {
		if (s_AppWinErrCodeTy == AppWinErrCodeTy_W32) {
			App_fPrnf(hFile, "Win32 error %lu on %s\n", s_AppWinErrCode, s_AppWinErrProc);
		}
		if (s_AppWinErrCodeTy == AppWinErrCodeTy_HR) {
			App_fPrnf(hFile, "Error 0x%08lX on %s\n", s_AppWinErrCode, s_AppWinErrProc);
		}
		if (pszCtx) {
			App_fPrnf(hFile, "Context: %s\n", pszCtx);
		}
	}
	else if (pszCtx) {
		App_fPrnf(hFile, "%s\n", pszCtx);
	}
}

void PrnNow_(HANDLE hFile)
{
	SYSTEMTIME s;
	GetLocalTime(&s);
	App_fPrnf(hFile, "%u/%02u/%02u %02u:%02u:%02u.%03u",
		s.wYear, s.wMonth, s.wDay, s.wHour,
		s.wMinute, s.wSecond, s.wMilliseconds);
}

void PrnNowExoticErr(PCSTR pszCtx)
{
	PrnNow_(s_hStdErr);
	PrnErr(": ");
	PrnExoticErr_(s_hStdErr, pszCtx);
}

void PrnNowErr(PCSTR pszFmt, ...)
{
	va_list ap;
	va_start(ap, pszFmt);
	PrnNow_(s_hStdErr);
	PrnErr(": ");
	App_vfPrnf(s_hStdErr, pszFmt, ap);
	va_end(ap);
}

void PrnNowOut(PCSTR pszFmt, ...)
{
	va_list ap;
	va_start(ap, pszFmt);
	PrnNow_(s_hStdOut);
	PrnOut(": ");
	App_vfPrnf(s_hStdOut, pszFmt, ap);
	va_end(ap);
}

void PrnErr(char const *pszFmt, ...)
{
	va_list ap;
	va_start(ap, pszFmt);
	App_vfPrnf(s_hStdErr, pszFmt, ap);
	va_end(ap);
}

void PrnOut(char const *pszFmt, ...)
{
	va_list ap;
	va_start(ap, pszFmt);
	App_vfPrnf(s_hStdOut, pszFmt, ap);
	va_end(ap);
}

static BOOL My_StartWith(ChunkChunk *pCC, TCHAR const *pszKey, size_t *pcchMatch)
{
	BOOL ok = FALSE;
	size_t n = 0;
	for (;; ++n)
	{
		if (!pszKey[n]) { ok = TRUE; break; }
		if (pCC->szChunk[n] != pszKey[n]) break;
	}
	*pcchMatch = n;
	return ok;
}

static TCHAR *My_StripDQuotes(TCHAR const *pszVal, TCHAR *pszBuf, int cchBuf)
{
	size_t cch = lstrlen(pszVal);
	if (cch >= 2 && pszVal[0] == TEXT('"') && pszVal[cch - 1] == TEXT('"')) {
		pszVal++;
		cch -= 2;
	}
	if (cch >= (size_t)cchBuf) cch = (size_t)cchBuf - 1;
	lstrcpyn(pszBuf, pszVal, (int)cch + 1);
	return pszBuf;
}

static BOOL My_ParseCommandLine(void)
{
	ChunkChunk cc;
	int iChunk;
	size_t cchMatch = 0;
	ChunkChunkInit(&cc, GetCommandLine());
	for (iChunk = 0; ChunkChunkNext(&cc); ++iChunk)
	{
		TCHAR szVal[CHUNKCHUNK_SZCHUNK_SIZE];
		if (iChunk == 0) continue;
		if (My_StartWith(&cc, TEXT("--help"), &cchMatch) || My_StartWith(&cc, TEXT("-h"), &cchMatch))
		{
			PrnOut("Usage: %s [options]\n", APP_TITLE);
			PrnOut("Options:\n");
			PrnOut("  --wake-interval=<ms>  Wake interval in milliseconds (default: %u)\n", APPCFGDEF_WAKE_INTERVAL);
			PrnOut("  --gpu-filter=<str>    Filter GPU by substring match\n");
			PrnOut("  -h, --help            Show this help message\n");
			ExitProcess(0);
		}
		else if (My_StartWith(&cc, TEXT("--wake-interval="), &cchMatch))
		{
			if (!App_tcs_to_UINT(My_StripDQuotes(&cc.szChunk[cchMatch], szVal, (int)ARRAYSIZE(szVal)), &s_appCfg.wake_interval))
			{
				PrnNowExoticErr("Bad command line param for 'wake-interval'.");
			}
		}
		else if (My_StartWith(&cc, TEXT("--gpu-filter="), &cchMatch))
		{
			lstrcpynW(s_gpuFilterW, My_StripDQuotes(&cc.szChunk[cchMatch], szVal, (int)ARRAYSIZE(szVal)), (int)ARRAYSIZE(s_gpuFilterW));
		}
	}
}

/* RTL_OSVERSIONINFOEXW is already defined in winnt.h (MinGW-w64) */

static BOOL My_IsWindows11OrGreater(void)
{
	HMODULE hNtdll = GetModuleHandleW(L"ntdll.dll");
	if (!hNtdll) return FALSE;
	typedef NTSTATUS (WINAPI *RtlGetVersion_t)(PRTL_OSVERSIONINFOEXW);
	RtlGetVersion_t pfnRtlGetVersion = (RtlGetVersion_t)GetProcAddress(hNtdll, "RtlGetVersion");
	if (!pfnRtlGetVersion) return FALSE;
	RTL_OSVERSIONINFOEXW rovi;
	ZeroMemory(&rovi, sizeof(rovi));
	rovi.dwOSVersionInfoSize = sizeof(rovi);
	NTSTATUS status = pfnRtlGetVersion(&rovi);
	if (status != 0) return FALSE;
	if (rovi.dwMajorVersion > 10) return TRUE;
	if (rovi.dwMajorVersion == 10 && rovi.dwMinorVersion >= 0 && rovi.dwBuildNumber >= 22000) return TRUE;
	return FALSE;
}

static BOOL My_ParseHexVal(char const *pszHex, ULONG *pVal)
{
	ULONG val = 0;
	char const *p = pszHex;
	if (*p == '0' && (p[1] == 'x' || p[1] == 'X')) p += 2;
	for (; *p; ++p) {
		char c = *p;
		if (c >= '0' && c <= '9') val = val * 16 + (c - '0');
		else if (c >= 'a' && c <= 'f') val = val * 16 + (c - 'a' + 10);
		else if (c >= 'A' && c <= 'F') val = val * 16 + (c - 'A' + 10);
		else return FALSE;
	}
	*pVal = val;
	return TRUE;
}

static char const *My_StartWithStr(char const *str, char const *prefix)
{
	size_t n = 0;
	for (; prefix[n]; ++n) {
		if (str[n] != prefix[n]) return NULL;
	}
	return str + n;
}

/* MinGW's dxgi import lib does not export IID_IDXGIFactory */
static const GUID IID_IDXGIFactory_ = {0x7b7166ec, 0x21c7, 0x44ae, {0xb2, 0x1a, 0xc9, 0xae, 0x32, 0x1a, 0xe3, 0x69}};

static BOOL My_InitDxgi(void)
{
	BOOL ok = FALSE;
	HRESULT hr = 0;
	PrnNowOut("CreateDXGIFactory...\n");
	hr = CreateDXGIFactory(&IID_IDXGIFactory_, (void **)&s_pDxgiFactory);
	if (FAILED(hr)) {
		AppWinErrSetHR("CreateDXGIFactory", hr);
		PrnNowExoticErr(NULL);
		goto eof;
	}
	for (UINT i = 0; i < ARRAYSIZE(s_pAdapters); ++i) {
		IDXGIAdapter *pAd = NULL;
		hr = s_pDxgiFactory->lpVtbl->EnumAdapters(s_pDxgiFactory, i, &pAd);
		if (hr == E_INVALIDARG || hr == DXGI_ERROR_NOT_FOUND) break;
		if (FAILED(hr)) continue;
		s_pAdapters[s_cAdapters] = pAd;
		++s_cAdapters;
	}
	ok = TRUE;
eof:
	if (!ok) {
		SAFERELEASE(s_pDxgiFactory);
	}
	return ok;
}

static void My_LogDxgiAdapter(UINT i)
{
	DXGI_ADAPTER_DESC d;
	ZeroMemory(&d, sizeof(d));
	if (FAILED(s_pAdapters[i]->lpVtbl->GetDesc(s_pAdapters[i], &d))) {
		PrnNowOut("  adapter %u: GetDesc failed\n", i);
		return;
	}
	PrnNowOut("  adapter %u: %S (VEN=%04X DEV=%04X SUBSYS=%08X)\n",
		i, d.Description, d.VendorId, d.DeviceId, d.SubSysId);
}

static int My_ResolveAdapterIndexFromRegistry(void)
{
	HKEY hKey;
	LSTATUS status = RegOpenKeyExW(HKEY_CURRENT_USER,
		L"Software\\Microsoft\\DirectX\\UserGpuPreferences",
		0, KEY_READ, &hKey);
	if (status != ERROR_SUCCESS) {
		PrnNowOut("Registry key not found, falling back to default adapter\n");
		return -1;
	}
	const WCHAR *valName = L"DirectXUserGlobalSettings";
	DWORD type = 0, dataSize = 0;
	status = RegQueryValueExW(hKey, valName, NULL, &type, NULL, &dataSize);
	if (status != ERROR_SUCCESS || type != REG_SZ || dataSize == 0) {
		PrnNowOut("DirectXUserGlobalSettings not found, falling back to default adapter\n");
		RegCloseKey(hKey);
		return -1;
	}
	char *buf = (char *)MemAllocZero(dataSize + sizeof(char));
	if (!buf) {
		PrnNowExoticErr("MemAllocZero for registry data");
		RegCloseKey(hKey);
		return -1;
	}
	status = RegQueryValueExW(hKey, valName, NULL, NULL, (LPBYTE)buf, &dataSize);
	if (status != ERROR_SUCCESS) {
		PrnNowExoticErr("RegQueryValueExW DirectXUserGlobalSettings");
		MemFree(buf);
		RegCloseKey(hKey);
		return -1;
	}
	RegCloseKey(hKey);
	WCHAR *wbuf = (WCHAR *)buf;
	int cchBuf = (int)dataSize / sizeof(WCHAR);
	char *utf8Buf = (char *)MemAllocZero(cchBuf * 4 + 1);
	if (!utf8Buf) {
		PrnNowExoticErr("MemAllocZero for UTF-8 conversion");
		MemFree(buf);
		return -1;
	}
	int cbUtf8 = WideCharToMultiByte(CP_UTF8, 0, wbuf, cchBuf, utf8Buf, cchBuf * 4, NULL, NULL);
	if (cbUtf8 == 0) {
		PrnNowExoticErr("WideCharToMultiByte UTF-8 conversion");
		MemFree(buf);
		MemFree(utf8Buf);
		return -1;
	}
	utf8Buf[cbUtf8] = '\0';
	MemFree(buf);
	char const *p = utf8Buf;
	char const *highPerf = "HighPerfAdapter=";
	char const *match = NULL;
	for (; *p; ++p) {
		if (My_StartWithStr(p, highPerf)) {
			match = p + strlen(highPerf);
			break;
		}
	}
	MemFree(utf8Buf);
	if (!match) {
		PrnNowOut("HighPerfAdapter not found in DirectXUserGlobalSettings\n");
		return -1;
	}
	char *token = (char *)MemAllocZero(strlen(match) + 1);
	if (!token) {
		PrnNowExoticErr("MemAllocZero for HighPerfAdapter token");
		return -1;
	}
	size_t i = 0;
	for (; match[i] && match[i] != ';' && match[i] != ' '; ++i) {
		token[i] = match[i];
	}
	token[i] = '\0';
	char *venStr = token;
	char *devStr = strchr(token, '&');
	char *subsysStr = NULL;
	ULONG ven = 0, dev = 0, subsys = 0;
	if (devStr) {
		*devStr = '\0';
		devStr++;
		subsysStr = strchr(devStr, '&');
		if (subsysStr) {
			*subsysStr = '\0';
			subsysStr++;
		}
	}
	if (!My_ParseHexVal(venStr, &ven) || !My_ParseHexVal(devStr, &dev) || !My_ParseHexVal(subsysStr, &subsys)) {
		PrnNowOut("Failed to parse HighPerfAdapter hex values\n");
		MemFree(token);
		return -1;
	}
	MemFree(token);
	PrnNowOut("HighPerfAdapter: VEN=%04X DEV=%04X SUBSYS=%08X, scanning %u adapter(s)\n",
		ven, dev, subsys, s_cAdapters);
	for (UINT i = 0; i < s_cAdapters; ++i) {
		DXGI_ADAPTER_DESC d;
		ZeroMemory(&d, sizeof(d));
		if (FAILED(s_pAdapters[i]->lpVtbl->GetDesc(s_pAdapters[i], &d))) {
			PrnNowOut("  adapter %u: GetDesc failed\n", i);
			continue;
		}
		PrnNowOut("  adapter %u: %S (VEN=%04X DEV=%04X SUBSYS=%08X)\n",
			i, d.Description, d.VendorId, d.DeviceId, d.SubSysId);
		if (d.VendorId == ven && d.DeviceId == dev && d.SubSysId == subsys) {
			PrnNowOut("  adapter %u: MATCH\n", i);
			return (int)i;
		}
		PrnNowOut("  adapter %u: no match\n", i);
	}
	PrnNowOut("No adapter matched HighPerfAdapter\n");
	return -1;
}

static WCHAR *wcsistr(WCHAR const *str, WCHAR const *substr)
{
	WCHAR const *p1 = str;
	WCHAR const *p2 = substr;
	for (; *p1; ++p1) {
		WCHAR c1 = *p1;
		WCHAR c2 = *p2;
		if (c1 >= L'A' && c1 <= L'Z') c1 += 32;
		if (c2 >= L'A' && c2 <= L'Z') c2 += 32;
		if (c1 == c2) {
			WCHAR const *q1 = p1 + 1;
			WCHAR const *q2 = p2 + 1;
			while (*q1 && *q2) {
				WCHAR d1 = *q1;
				WCHAR d2 = *q2;
				if (d1 >= L'A' && d1 <= L'Z') d1 += 32;
				if (d2 >= L'A' && d2 <= L'Z') d2 += 32;
				if (d1 != d2) break;
				++q1;
				++q2;
			}
			if (!*q2) return (WCHAR *)p1;
		}
	}
	return NULL;
}

static int My_ResolveAdapterIndexFromGpuFilter(void)
{
	if (!s_gpuFilterW[0]) return -1;
	PrnNowOut("gpu-filter: '%S', scanning %u adapter(s)\n", s_gpuFilterW, s_cAdapters);
	for (UINT i = 0; i < s_cAdapters; ++i) {
		DXGI_ADAPTER_DESC d;
		ZeroMemory(&d, sizeof(d));
		if (FAILED(s_pAdapters[i]->lpVtbl->GetDesc(s_pAdapters[i], &d))) {
			PrnNowOut("  adapter %u: GetDesc failed\n", i);
			continue;
		}
		PrnNowOut("  adapter %u: %S (VEN=%04X DEV=%04X SUBSYS=%08X)\n",
			i, d.Description, d.VendorId, d.DeviceId, d.SubSysId);
		if (wcsistr(d.Description, s_gpuFilterW)) {
			PrnNowOut("  adapter %u: MATCH\n", i);
			return (int)i;
		}
		PrnNowOut("  adapter %u: no match\n", i);
	}
	PrnNowOut("No adapter matched filter '%S'\n", s_gpuFilterW);
	return -1;
}

int ResolveAdapterIndex(void)
{
	int idx = -1;
	if (s_gpuFilterW[0]) {
		idx = My_ResolveAdapterIndexFromGpuFilter();
	}
	else if (My_IsWindows11OrGreater()) {
		idx = My_ResolveAdapterIndexFromRegistry();
	}
	else {
		PrnNowOut("Not Windows 11+, using default adapter\n");
	}
	if (idx < 0) {
		PrnNowOut("Falling back to default adapter\n");
		idx = 0;
	}
	return idx;
}

int IsWindows11OrGreater(void)
{
	return My_IsWindows11OrGreater() ? 1 : 0;
}

int AppMain(void)
{
	HWND hD3DWnd = NULL;
#ifdef UNICODE
	{
		WCHAR *p; char const *q;
#define H__(a,b)  p = a; q = b; for (; *p = *q, *q; ++p, ++q);
#include "szlitera.h"
	}
#endif
	s_hStdErr = GetStdHandle(STD_ERROR_HANDLE);
	s_hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
	PrnOut("%s [build 2022.01.20] by raymai97\n\n", APP_TITLE);

	s_hHeap = GetProcessHeap();
	if (!s_hHeap) goto eof;
	s_hInst = GetModuleHandle(NULL);
	if (!s_hInst) goto eof;

	s_appCfg.wake_interval = APPCFGDEF_WAKE_INTERVAL;
	My_ParseCommandLine();
	s_hIcon = LoadIcon(APP_hInst, (void*)IDI_ICON1);
	if (!s_hIcon) {
		AppWinErrSetW32("LoadIcon", GetLastError());
		PrnNowExoticErr("ICON1");
		goto eof;
	}
	s_wmTaskbarCreated = RegisterWindowMessage(TaskbarCreated);
	if (!s_wmTaskbarCreated) {
		AppWinErrSetW32("RegisterWindowMessage", GetLastError());
		PrnNowExoticErr("TaskbarCreated");
		goto eof;
	}

	s_hMainWnd = GetConsoleWindow();
	if (s_hMainWnd) {
		SetWindowText(s_hMainWnd, L"KeepSecondGpuAwake");
		SendMessage(s_hMainWnd, WM_SETICON, 0, (LPARAM)s_hIcon);
		SendMessage(s_hMainWnd, WM_SETICON, 1, (LPARAM)s_hIcon);
	}

	if (!My_InitDxgi()) {
		goto eof;
	}
	{
		UINT i;
		PrnNowOut("DXGI adapters: %u\n", s_cAdapters);
		for (i = 0; i < s_cAdapters; ++i) {
			My_LogDxgiAdapter(i);
		}
	}
	if (!Create_D3DWnd(&hD3DWnd)) {
		goto eof;
	}
	{
		int adapterIndex = ResolveAdapterIndex();
		Start_D3DWnd(hD3DWnd, s_pAdapters[adapterIndex]);
	}
	while (IsWindow(hD3DWnd)) {
		MSG msg;
		if (GetMessage(&msg, NULL, 0, 0)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
eof:
	return 0;
}

void RawMain(void)
{
	ExitProcess(AppMain());
}

int main(void)
{
	return AppMain();
}
