# xzx — ReShade Preset Control (Purple Edition)

A small native Windows app (Win32 + common controls) with a purple/black UI
that opens, edits, and saves ReShade `.ini` preset files. The built
executable is named **`xzx.exe`**.

## What it does
- Purple/black Windows UI.
- Opens and saves ReShade `.ini` preset files.
- Reads `[GLOBAL]` / `[GENERAL]` technique and configuration entries.
- Lets you toggle techniques on/off and edit numeric preset values.
- Writes the preset back while preserving unrelated lines.
- Creates a `.bak` backup before each save.
- Opens the preset's folder in Explorer.

ReShade presets are INI files, and the `Techniques=` entry controls which
effects are active. ReShade itself still compiles and renders the shaders;
this app only edits the preset that ReShade consumes.

---

## How to get xzx.exe

The source is pure Win32 C++, so it has to be compiled on (or for) Windows.
Pick whichever route is easiest for you.

### Option A - Double-click build (local, easiest)
On a Windows PC, just run **`build.bat`**. It auto-detects your compiler
(Visual Studio `cl`, or MinGW `g++`), compiles `src\main.cpp`, and drops
`xzx.exe` next to the script.

You need one of:
- **Visual Studio 2022** with the *Desktop development with C++* workload, or
- **MSYS2 / MinGW-w64** (provides `g++`).

### Option B - Build in the cloud (no compiler needed)
This repo includes a GitHub Actions workflow (`.github/workflows/build.yml`).
1. Push these files to a GitHub repo.
2. Open the **Actions** tab - the **Build xzx.exe** workflow runs automatically
   (or click *Run workflow*).
3. When it finishes, download the **`xzx`** artifact - it contains `xzx.exe`.

No local toolchain required; GitHub compiles it on a Windows runner.

### Option C - CMake by hand
From a Developer Command Prompt:
```powershell
cmake -S . -B build -A x64
cmake --build build --config Release
```
Output: `build\Release\xzx.exe`

No third-party libraries are required - the UI uses Win32 common controls.

---

## Using xzx.exe
1. Start `xzx.exe`.
2. Click **OPEN PRESET** and choose a ReShade `.ini` preset.
3. Use **TECHNIQUES** to enable/disable effects (select, then **TOGGLE SELECTED**).
4. Use **NUMERIC VALUES** - **EDIT SELECTED VALUE** to change numbers that exist in the preset.
5. Click **SAVE PRESET**. A `.bak` backup is created first.
6. In ReShade, reload the preset if your ReShade version needs it.

## Notes
- This project reconstructs the observable preset-management functionality
  and provides a purple UI. It does not claim to recover any original source
  and does not embed ReShade's runtime.
- Binary-analysis notes from the original upload are kept under `analysis/`.
  They are not needed to build and can be deleted.
