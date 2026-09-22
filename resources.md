# Resources

## Windows 11 GPU Preference Mechanism

### Registry Location
```
HKCU\Software\Microsoft\DirectX\GraphicsSettings
  - DefaultHighPerfGPUApplicable  REG_DWORD  0x1  (feature enabled/disabled)

HKCU\Software\Microsoft\DirectX\UserGpuPreferences
  - DirectXUserGlobalSettings  REG_SZ  (global user-level preference)
```

### DirectXUserGlobalSettings Value Format
```
HighPerfAdapter=<VEN>&<DEV>&<SUBSYS>;SwapEffectUpgradeEnable=1;AutoHDREnable=0;VRROptimizeEnable=1;
```

- `HighPerfAdapter` — hex-encoded PCI IDs of the preferred GPU
  - VEN = vendor ID (e.g., `1002` = AMD, `10de` = NVIDIA)
  - DEV = device ID
  - SUBSYS = subsystem ID
- `SwapEffectUpgradeEnable` — typically `1`
- `AutoHDREnable` — HDR setting, typically `0`
- `VRROptimizeEnable` — typically `1`

### Sources
- [KapilArya.com](https://www.kapilarya.com/set-graphics-preference-for-apps-in-windows-11) — How to set graphics preference for apps in Windows 11 via registry
- [blog.usro.net](https://blog.usro.net/2025/04/how-to-set-gpu-priority-via-windows-registry) — Comprehensive guide on setting GPU priority via Windows Registry
- [NinjaOne](https://www.ninjaone.com/blog/restore-gpu-preferences-for-apps-in-windows) — Backup and restore GPU preferences for apps in Windows 10/11

## D3D9 Adapter Enumeration

### Key APIs
| API | Purpose |
|-----|---------|
| `IDirect3D9::GetAdapterCount()` | Returns number of adapters |
| `IDirect3D9::GetAdapterIdentifier(UINT Adapter, DWORD Flags, D3DADAPTER_IDENTIFIER9* pIdentifier)` | Gets identifier for adapter at index |
| `IDirect3D9::CreateDevice(UINT Adapter, ...)` | Creates device for specific adapter |

### D3DADAPTER_IDENTIFIER9 Fields
| Field | Type | Description |
|-------|------|-------------|
| `VendorId` | DWORD | Manufacturer (e.g., 0x1002 = AMD) |
| `DeviceId` | DWORD | Chipset model |
| `SubSysId` | DWORD | Specific board |
| `Description` | char[512] | Human-readable name |

### Adapter Selection
- `D3DADAPTER_DEFAULT` (0) = primary display adapter
- To select a specific adapter: pass its index (0, 1, 2, ...) to `CreateDevice()`

### Sources
- [Microsoft Learn — IDirect3D9::GetAdapterIdentifier](https://learn.microsoft.com/en-us/windows/win32/api/d3d9/nf-d3d9-idirect3d9-getadapteridentifier)
- [Microsoft Learn — D3DADAPTER_IDENTIFIER9](https://learn.microsoft.com/en-us/windows/win32/direct3d9/d3dadapter-identifier9)
- [Microsoft Learn — Selecting a Device](https://learn.microsoft.com/en-us/windows/win32/direct3d9/selecting-a-device)
- [Wikibooks — DirectX/9.0/Direct3D/Initialization](https://en.wikibooks.org/wiki/DirectX/9.0/Direct3D/Initialization)

## OS Version Detection

### RtlGetVersion vs VerifyVersionInfo

| Aspect | RtlGetVersion | VerifyVersionInfo |
|--------|--------------|-------------------|
| Purpose | Retrieves actual OS version | Compares requirements against OS |
| Deprecation | Not deprecated | Deprecated as of Windows 10 |
| Manifest affected | No — returns true version | Yes — lies if no compat manifest |
| Recommended | Yes | No (legacy only) |

### RtlGetVersion Signature
```c
typedef NTSTATUS (WINAPI *RtlGetVersion_t)(PRTL_OSVERSIONINFOW);
// PRTL_OSVERSIONINFOW -> OSVERSIONINFOW with dwMajorVersion, dwMinorVersion, etc.
```

Load from `ntdll.dll` via `GetProcAddress` to avoid linker dependency on undocumented export.

### Sources
- [StackOverflow — Programmatically set Graphics Performance for an app](https://stackoverflow.com/questions/59732181/programmatically-set-graphics-performance-for-an-app)
