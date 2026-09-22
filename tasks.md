# tasks.md

## Task 1 — DXGI adapter enumeration + resolution rewrite — `completed`
- [x] `My_InitDxgi`: `CreateDXGIFactory` (local `IID_IDXGIFactory_` — MinGW import lib
      does not export it), `EnumAdapters(i)` until `E_INVALIDARG`
- [x] Log each adapter: `DXGI_ADAPTER_DESC.Description` + VEN/DEV/SUBSYS (timestamped)
- [x] `--gpu-filter` path: case-insensitive substring match on `Description` (`wcsistr`)
- [x] Win11 `HighPerfAdapter` registry path: match VEN/DEV/SUBSYS against adapter desc
- [x] Removed D3D9-only code: `Dywa_Direct3DCreate9`, `s_h_d3d9`, `s_pfn_Direct3DCreate9`,
      D3D9 resolution functions
- [x] `./build.sh` passes

## Task 2 — D3D11 device + keep-awake step — `completed`
- [x] `D3DWnd_CreateD3D11`: `D3D11CreateDevice(pAdapter, HARDWARE, D3D_FEATURE_LEVEL_11_0)`
- [x] 1x1 R8G8B8A8_UNORM texture (`D3D11_USAGE_DEFAULT` + `D3D11_BIND_UNORDERED_ACCESS`) + UAV
- [x] `D3DWnd_Step`: `ClearUnorderedAccessViewFloat` (trivial GPU write) +
      `GetDeviceRemovedReason` check; log tick with `PrnNowOut`
      (MinGW d3d11.h predates fences/deferred contexts → 11.0 immediate-context API)
- [x] Device-removed handling: release all, retry creation after 1 s
- [x] `./build.sh` passes

## Task 3 — Integration + cleanup — `completed`
- [x] `D3DWnd.c`: D3D9 create/step replaced with D3D11; window, tray icon, timer,
      exit menu unchanged
- [x] `build.sh`: `-ld3d11 -ldxgi` (was `-ld3d9`)
- [x] All D3D9 includes/vars removed (`base.h` now includes `<d3d11.h>`)
- [x] Renamed `src/d3d9` directory to `src/d3d11`
- [x] Updated `APP_ID` to reflect d3d11

## Manual verification (on Windows) — `pending`
- [ ] `--gpu-filter="AMD Radeon(TM) Graphics"` (iGPU, has display) — match + tick
- [ ] `--gpu-filter=7900` (headless dGPU) — must now appear in DXGI list, match + tick
- [ ] No filter on Win11 (HighPerfAdapter path)
- [ ] `-h` / `--help` displays usage and exits
