# AGENTS.md — Development Protocol

## Workflow
1. Read `plan.md` for the full architecture and task list
2. Read `tasks.md` for current task state — only work on `pending` tasks
3. Mark tasks as `in_progress` / `completed` in `tasks.md` as you work
4. After completing a task, update `plan.md` if architecture changed
5. Run `./build.sh` to verify builds before committing

## Task Tracking
Tasks are tracked in `tasks.md` with status: `pending` | `in_progress` | `completed`
Implementation order: Task 1 → Task 2 → Task 3 (as defined in `plan.md`)

## Code Style
- C99, no C++
- No VC++ redistributable dependency — use inline implementations for `memset`/`memcpy`/`memcmp`
- Error handling: `goto eof` pattern with `SAFEFREE`/`SAFERELEASE` macros
- Use `AppWinErrSetW32()` / `AppWinErrSetHR()` for error tracking
- Timestamped logging: `PrnNowOut()` / `PrnNowErr()`

## Build
```bash
./build.sh
```

Requires `x86_64-w64-mingw32-gcc` (mingw-w64 toolchain).
