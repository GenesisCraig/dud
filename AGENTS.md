# AGENTS.md

This file provides guidance to Codex (claude.ai/code) when working with code in this repository.

## What this is

`dud` ("Disk Usage and Days Stale") is a Windows-only C++ console utility. It recursively walks each directory given on the command line and prints file count, dir count, size (MB), days since the newest modification anywhere beneath it, and an `ARCHIVE` flag. Purpose: find project directories to move to nearline/archival storage. See `README.md` for sample output and the feature to-do list (ERP project-status lookup, multilevel wildcards).

## Build

Visual Studio solution, toolset v141 (VS 2017), Debug/Release × Win32/x64. No linter. Tests are black-box: `test_dud.py` (stdlib `unittest`) runs a built `dud.exe` against temp dirs. Run all with `set DUD_EXE=<path to dud.exe>` then `python test_dud.py`; run one with `python test_dud.py DudTest.test_size_over_4gb_does_not_wrap`. It uses `fsutil` sparse files to fake >4 GB sizes without using disk.

```
msbuild dud.sln /p:Configuration=Release /p:Platform=x64
x64\Release\dud.exe f:\4*        # or: dud.exe --help
```

The v141 toolset isn't installed on this machine. To build without it, run `vcvars64.bat` from `C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools`, then:
`cl /EHsc /O2 /std:c++14 /FIexperimental/filesystem /D_SILENCE_EXPERIMENTAL_FILESYSTEM_DEPRECATION_WARNING dud\dud.cpp setargv.obj`. The `/FI` is needed because newer MSVC's `<filesystem>` no longer exposes `std::experimental`. The program ends with `PAUSE`, so pipe input in (`echo | dud.exe ...`).

`dud/sandbox/` holds a throwaway file for manual runs.

## Architecture

All live code is in `dud/dud.cpp` (constants in `dud/dud.h`). Only `dud.cpp` and `stdafx.cpp` are in `dud.vcxproj`.

- `main()` fills a global `vector<string> queue` from argv, captures `time_t now` once (read by all threads), then spawns `min(hardware_concurrency, 8) - 1` worker threads and joins them. The main thread does no work itself. Ends with `system("PAUSE")`.
- `worker()` pops paths off `queue` under `queueLock`, calls `getDirectoryInfo()`, and prints under `coutLock`. One thread per top-level argument; a single huge directory is not split across threads.
- `getDirectoryInfo()` uses `std::experimental::filesystem::recursive_directory_iterator`; each dir adds a nominal 512 bytes. `ARCHIVE` = stale > `kMinStalenessDays` and size > `kMinSizeMB` (despite the name, that constant is in bytes).
- Wildcards: `setargv.obj` is linked (Debug|Win32 and Release|x64 only) so the CRT expands single-level wildcards like `f:\5*`. Any argument still containing `*` is skipped with a warning.

## Gotchas

- `WorkQueue.h/.cpp` and `Worker.cpp` are abandoned (not in the project; `Worker.h` doesn't exist). The last commits removed classes as unnecessary — don't wire them back in without reason.
- `--csv` output is not clean CSV: the text header is still printed, and `cout` is locale-imbued, so sizes like `5,120` add extra commas. MB is also integer-divided there.
- The author is new to C++ (see README "Caveat Emptor"); explain non-obvious C++ when making changes.
