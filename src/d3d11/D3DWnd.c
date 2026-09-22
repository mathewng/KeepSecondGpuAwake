#include "D3DWnd.hh"

BOOL Create_D3DWnd(HWND *phWnd)
{
	BOOL ok = FALSE;
	WNDCLASS wc;
	DWORD errRegisterClass = 0;
	HWND hWnd = NULL;
	MySelf *pSelf = NULL;
	ZeroMemory(&wc, sizeof(wc));
	wc.lpfnWndProc = D3DWnd_WndProc;
	wc.lpszClassName = D3DWnd_Class;
	wc.cbWndExtra = sizeof(void *);
	if (!RegisterClass(&wc)) {
		errRegisterClass = GetLastError();
	}
	hWnd = CreateWindowEx(0, D3DWnd_Class, D3DWnd_Class, WS_POPUP, 0, 0, 1, 1, NULL, NULL, APP_hInst, NULL);
	if (!hWnd) {
		if (errRegisterClass) {
			AppWinErrSetW32("RegisterClass", errRegisterClass);
		}
		else {
			AppWinErrSetW32("CreateWindowEx", GetLastError());
		}
		PrnNowExoticErr("D3DWnd");
		goto eof;
	}
	ok = D3DWnd_NewSelf(&pSelf, hWnd);
	if (!ok) goto eof;
	ok = TRUE; {
		SetWindowLongPtr(hWnd, 0, (LONG_PTR)pSelf);
		*phWnd = hWnd;
	}
eof:
	if (!ok) {
		SAFEFREE(pSelf, D3DWnd_DelSelf);
		SAFEFREE(hWnd, DestroyWindow);
	}
	return ok;
}

BOOL Start_D3DWnd(HWND hWnd, IDXGIAdapter *pAdapter)
{
	MySelf *pSelf = D3DWnd_GetSelf(hWnd);
	pSelf->pAdapter = pAdapter;
	D3DWnd_AddNotiIcon(pSelf);
	return D3DWnd_Step(pSelf);
}

static void D3DWnd_DelSelf(MySelf *pSelf)
{
	SAFERELEASE(pSelf->pUav);
	SAFERELEASE(pSelf->pTex);
	SAFERELEASE(pSelf->pCtx);
	SAFERELEASE(pSelf->pDev);
	pSelf->hWnd = NULL;
	MemFree(pSelf);
}

static BOOL D3DWnd_NewSelf(MySelf **ppSelf, HWND hWnd)
{
	BOOL ok = FALSE;
	MySelf *pSelf = NULL;
	pSelf = MemAllocZero(sizeof(*pSelf));
	if (!pSelf) {
		PrnNowExoticErr(NULL);
		goto eof;
	}
	pSelf->hWnd = hWnd;
	{
		NotifyIconDataV1 *p = &pSelf->notifyIconData;
		p->cbSize = sizeof(*p);
		p->hWnd = pSelf->hWnd;
		p->uFlags = NIF_ICON | NIF_TIP | NIF_MESSAGE;
		p->hIcon = APP_hIcon;
		lstrcpy(p->szTip, AppTitle);
		p->uCallbackMessage = NotiIconMsg;
	}
	ok = TRUE; {
		*ppSelf = pSelf;
	}
eof:
	if (!ok) {
		SAFEFREE(pSelf, D3DWnd_DelSelf);
	}
	return ok;
}

static MySelf *D3DWnd_GetSelf(HWND hWnd)
{
	return (MySelf *)GetWindowLongPtr(hWnd, 0);
}

static BOOL D3DWnd_AddNotiIcon(MySelf *pSelf)
{
	NotifyIconDataV1 *p = &pSelf->notifyIconData;
	return Shell_NotifyIcon(NIM_ADD, (NOTIFYICONDATA*)p);
}

static void D3DWnd_DelNotiIcon(MySelf *pSelf)
{
	NotifyIconDataV1 *p = &pSelf->notifyIconData; 
	Shell_NotifyIcon(NIM_DELETE, (NOTIFYICONDATA*)p);
}

#ifndef D3D11_ERROR_DEVICE_REMOVED
#define D3D11_ERROR_DEVICE_REMOVED 0x88760862
#endif

static BOOL D3DWnd_CreateD3D11(MySelf *pSelf)
{
	BOOL ok = FALSE;
	HRESULT hr = 0;
	PrnNowOut("D3D11CreateDevice...\n");
	{
		/* D3D11: non-NULL pAdapter requires D3D_DRIVER_TYPE_UNKNOWN;
		 * NULL pAdapter (default adapter) requires D3D_DRIVER_TYPE_HARDWARE */
		D3D_DRIVER_TYPE drvType = pSelf->pAdapter ? D3D_DRIVER_TYPE_UNKNOWN : D3D_DRIVER_TYPE_HARDWARE;
hr = D3D11CreateDevice(pSelf->pAdapter, drvType, NULL,
		0, NULL, 0,
		D3D11_SDK_VERSION, &pSelf->pDev, &pSelf->featLevel, &pSelf->pCtx);
	}
	if (FAILED(hr)) {
		AppWinErrSetHR("D3D11CreateDevice", hr);
		PrnNowExoticErr(NULL);
		goto eof;
	}
	PrnOut("|- Feature level 0x%04X\n", pSelf->featLevel);
	{
		D3D11_TEXTURE2D_DESC td;
		ZeroMemory(&td, sizeof(td));
		td.Width = 1;
		td.Height = 1;
		td.MipLevels = 1;
		td.ArraySize = 1;
		td.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		td.SampleDesc.Count = 1;
		td.Usage = D3D11_USAGE_DEFAULT;
		td.BindFlags = D3D11_BIND_UNORDERED_ACCESS;
		hr = pSelf->pDev->lpVtbl->CreateTexture2D(pSelf->pDev, &td, NULL, &pSelf->pTex);
		if (FAILED(hr)) {
			AppWinErrSetHR("ID3D11Device::CreateTexture2D", hr);
			PrnNowExoticErr(NULL);
			goto eof;
		}
		hr = pSelf->pDev->lpVtbl->CreateUnorderedAccessView(pSelf->pDev, (ID3D11Resource *)pSelf->pTex, NULL, &pSelf->pUav);
		if (FAILED(hr)) {
			AppWinErrSetHR("ID3D11Device::CreateUnorderedAccessView", hr);
			PrnNowExoticErr(NULL);
			goto eof;
		}
	}
	ok = TRUE;
eof:
	if (!ok) {
		SAFERELEASE(pSelf->pUav);
		SAFERELEASE(pSelf->pTex);
		SAFERELEASE(pSelf->pCtx);
		SAFERELEASE(pSelf->pDev);
	}
	return ok;
}

static BOOL D3DWnd_Step(MySelf *pSelf)
{
	BOOL ok = FALSE;
	HRESULT d3derr = 0;
	AppWinErrReset();
	if (!pSelf->pDev) {
		ok = D3DWnd_CreateD3D11(pSelf) && pSelf->pDev;
		if (!ok) {
			SetTimer(pSelf->hWnd, TimerID_Step, StepDelay_CreateD3D, NULL);
			goto eof;
		}
		SetTimer(pSelf->hWnd, TimerID_Step, StepDelay_Tick, NULL);
	}
	PrnNowOut("Tick... ");
	{
		FLOAT clr[4] = { 0.f, 1.f, 0.f, 1.f };
		pSelf->pCtx->lpVtbl->ClearUnorderedAccessViewFloat(pSelf->pCtx, pSelf->pUav, clr);
	}
	d3derr = pSelf->pDev->lpVtbl->GetDeviceRemovedReason(pSelf->pDev);
	if (FAILED(d3derr)) {
		PrnOut("\n");
		goto eof;
	}
	PrnOut("OK\n");
	ok = TRUE;
eof:
	if (d3derr == D3D11_ERROR_DEVICE_REMOVED) {
		PrnOut("|- D3D11_ERROR_DEVICE_REMOVED : Device removed\n");
		SAFERELEASE(pSelf->pUav);
		SAFERELEASE(pSelf->pTex);
		SAFERELEASE(pSelf->pCtx);
		SAFERELEASE(pSelf->pDev);
		SetTimer(pSelf->hWnd, TimerID_Step, StepDelay_CreateD3D, NULL);
	}
	else if (FAILED(d3derr)) {
		PrnOut("|- D3D11 HRESULT 0x%08lX\n", (unsigned long)d3derr);
	}
	return ok;
}

static LRESULT CALLBACK D3DWnd_WndProc(HWND hWnd, UINT msg, WPARAM w, LPARAM l)
{
	LRESULT lRes = 0;
	BOOL overrid = FALSE;
	switch (msg) {
	case WM_NCDESTROY:
		On_D3DWnd_NcDestroy(hWnd);
		break;
	case WM_COMMAND:
		On_D3DWnd_Command(hWnd, LOWORD(w));
		break;
	case WM_TIMER:
		On_D3DWnd_Timer(hWnd, (UINT)w);
		break;
	case NotiIconMsg:
		switch ((UINT)l) {
		case WM_LBUTTONUP:
			On_D3DWnd_NotiIcon_LButtonUp(hWnd);
			break;
		case WM_RBUTTONUP:
			On_D3DWnd_NotiIcon_RButtonUp(hWnd);
			break;
		}
		break;
	default:
		if (msg == WM_TaskbarCreated) {
			On_D3DWnd_TaskbarCreated(hWnd);
		}
		break;
	}
	if (!overrid) {
		lRes = DefWindowProc(hWnd, msg, w, l);
	}
	return lRes;
}

static void On_D3DWnd_NcDestroy(HWND hWnd)
{
	MySelf *pSelf = D3DWnd_GetSelf(hWnd);
	D3DWnd_DelNotiIcon(pSelf);
	D3DWnd_DelSelf(pSelf);
}

static void On_D3DWnd_Command(HWND hWnd, UINT cmdId)
{
	if (cmdId == CmdId_Exit) {
		SendMessage(hWnd, WM_CLOSE, 0, 0);
	}
}

static void On_D3DWnd_Timer(HWND hWnd, UINT timerId)
{
	if (timerId == TimerID_Step) {
		D3DWnd_Step(D3DWnd_GetSelf(hWnd));
	}
}

static void On_D3DWnd_TaskbarCreated(HWND hWnd)
{
	MySelf *pSelf = D3DWnd_GetSelf(hWnd);
	D3DWnd_AddNotiIcon(pSelf);
}

static void On_D3DWnd_NotiIcon_LButtonUp(HWND hWnd)
{
	hWnd = APP_MainWnd;
	if (!hWnd) {
		return;
	}
	if (IsWindowVisible(hWnd)) {
		ShowWindow(hWnd, HIDE_WINDOW);
	}
	else {
		ShowWindow(hWnd, SHOW_OPENWINDOW);
		SetForegroundWindow(hWnd);
	}
}

static void On_D3DWnd_NotiIcon_RButtonUp(HWND hWnd)
{
	POINT m;
	if (GetCursorPos(&m)) {
		HMENU hMenu = CreatePopupMenu();
		if (hMenu) {
			ChangeMenu(hMenu, 0, AppTitle, 0, MF_APPEND | MFS_DISABLED);
			ChangeMenu(hMenu, 0, NULL, 0, MF_APPEND | MF_SEPARATOR);
			ChangeMenu(hMenu, 0, CmdSz_Exit, CmdId_Exit, MF_APPEND | MF_BYCOMMAND);
			SetForegroundWindow(hWnd);
			TrackPopupMenu(hMenu, TPM_RIGHTBUTTON, m.x, m.y, 0, hWnd, NULL);
		}
	}
}
