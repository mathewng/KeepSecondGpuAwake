# KeepSecondGpuAwake

This project keeps the second GPU awake, especially when no monitor is connected to it.

Works across vendors (AMD, Intel, NVIDIA) to prevent GPU sleep when disconnected from a display.

## Usage

```
KeepSecondGpuAwake.exe [options]
```

Options:
- `--wake-interval=<ms>`  Wake interval in milliseconds (default: 500)
- `--gpu-filter=<str>`    Filter GPU by substring match (e.g., "7900", "Radeon")
- `-h`, `--help`          Show this help message

## Building

Requires `x86_64-w64-mingw32-gcc` (mingw-w64 toolchain).

```bash
./build.sh
```