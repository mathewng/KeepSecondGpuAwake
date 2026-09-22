#include "D3DWnd.h"
#include <dxgi1_2.h>

#define MySelf  D3DWnd_Self

typedef struct MySelf {
	HWND hWnd;
	IDXGIAdapter *pAdapter;
	ID3D11Device *pDev;
	ID3D11DeviceContext *pCtx;
	ID3D11Texture2D *pTex;
	ID3D11UnorderedAccessView *pUav;
	D3D_FEATURE_LEVEL featLevel;
	NotifyIconDataV1 notifyIconData;
	IDXGISwapChain1 *pSwapChain;
	ID3D11RenderTargetView *pRtv;
} MySelf;

enum { CmdId_Exit = 111 };

enum { NotiIconMsg = 101 };

enum { TimerID_Step = 100 };
enum { StepDelay_CreateD3D = 1000 };
#define StepDelay_Tick  (APP_Cfg.wake_interval)

static void D3DWnd_DelSelf(MySelf *pSelf);
static BOOL D3DWnd_NewSelf(MySelf **ppSelf, HWND hWnd);
static MySelf *D3DWnd_GetSelf(HWND hWnd);

static BOOL D3DWnd_AddNotiIcon(MySelf *pSelf);
static void D3DWnd_DelNotiIcon(MySelf *pSelf);

static BOOL D3DWnd_CreateD3D(MySelf *pSelf);
static BOOL D3DWnd_Step(MySelf *pSelf);

static LRESULT CALLBACK D3DWnd_WndProc(HWND hWnd, UINT msg, WPARAM w, LPARAM l);
static void On_D3DWnd_NcDestroy(HWND hWnd);
static void On_D3DWnd_Command(HWND hWnd, UINT cmdId);
static void On_D3DWnd_Timer(HWND hWnd, UINT timerId);
static void On_D3DWnd_TaskbarCreated(HWND hWnd);
static void On_D3DWnd_NotiIcon_LButtonUp(HWND hWnd);
static void On_D3DWnd_NotiIcon_RButtonUp(HWND hWnd);
