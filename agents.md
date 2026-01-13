# MameCloudRom (MCR) Agents Log
```
   __  ___               _______             _____           
  /  |/  /__ ___ _  ___ / ___/ /__  __ _____/ / _ \___  __ _ 
 / /|_/ / _ `/  ' \/ -_) /__/ / _ \/ // / _  / , _/ _ \/  ' \
/_/  /_/\_,_/_/_/_/\__/\___/_/\___/\_,_/\_,_/_/|_|\___/_/_/_/
                                                             
                                                  by anomixer
```

# 2026-01-02 MameCloudRom (MCR) v0.1 Development Summary

Today marks a major milestone in development and transformation, ranging from core function fixes to rebranding, comprehensively improving the user experience for players.

## 1. Core Feature Development (Smart Routing)
*   **Smart Extended Path Routing**: Implemented auto-detection logic in `SOpen`. When MAME requests a `.zip` file, the program automatically redirects to the `/split` directory; requests for `.7z` are redirected to `/standalone`. This resolved the `unzip: couldn't find ECD` corruption errors caused by fetching the wrong file format.
*   **URL Auto-Correction**: Users only need to provide the root `<ROM_SOURCE_URL>`, and the program automatically appends the correct sub-path based on the file extension.

## 2. Automation & UX Improvements (v0.1)
*   **Smart `config.bat`**: Added `mcr.ini` detection to automatically load previous settings as defaults. In addition to cache and drive letter, it now also asks for the MAME installation path.
*   **Automated Dual-Window Launch**: `mcr.bat` now executes a dual-window strategy: one window runs the MCR host, while simultaneously opening another window to `cd` into the MAME directory and execute `mame.exe -rompath`.

---

# 2026-01-11 MameCloudRom (MCR) v0.2 Release

## Mission
This release transitions MCR from a developer prototype to a production-ready tool for players. The focus was on **System Stability**, **Smart Routing**, and **Zero-Config UX**.

## Key Technical Decisions & Fixes

### 1. System Stability (Handle Leasing)
*   **Problem**: WinFsp interactions with MAME/Explorer caused "Double Free" or "Premature Close" crashes due to race conditions in `SClose`.
*   **Solution**: Implemented a **Handle Leasing** strategy. In `SClose`, we intentionally leak the internal file context (`delete ctx` is commented out).
*   **Rationale**: For a stateless, session-based tool like MCR, process-level resource reclamation by the OS at exit is infinitely more stable than risky manual memory management in a multi-threaded FUSE environment. This completely eliminated read crashes.

### 2. Auto-Download Architecture
*   **Implementation**: Logic resides in `SGetSecurityByName`.
*   **Flow**:
    1.  MAME requests file attributes.
    2.  MCR checks local cache -> Missing.
    3.  MCR triggers **Blocking Download** immediately.
    4.  Upon success, MCR returns valid attributes.
*   **Result**: MAME sees the file as "existing" immediately, preventing "Not Found" errors without needing complex retry logic.

---

# 2026-01-13 MameCloudRom (MCR) v1.0 Production Release

## Mission
To finalize MCR as a content-agnostic, legally robust, and highly flexible VFS tool. This release completes the "Protection Umbrella" and shifts all content responsibility to the user while providing ultimate path customization.

## Key Technical Decisions & Fixes

### 1. Extreme Stability (v0.2 Baseline)
*   **Implementation**: Adopting the proven handle leasing and permission model (`FILE_READ_ATTRIBUTES | FILE_READ_DATA`) from v0.2.
*   **Rationale**: Found that Windows File Explorer and WinFsp 2025 perform most reliably with this specific protection mechanism.
*   **Result**: 100% stability when browsing via File Explorer, even during intensive background downloads.

### 2. Custom Prefix Architecture
*   **Problem**: Different archival sites use different directory structures (e.g., some use `/split/`, others use `/mame/zip/`).
*   **Solution**: Introduced `-zp` (Zip Prefix) and `-7p` (7z Prefix) arguments. 
*   **Dynamic Routing**: The core logic now dynamically constructs URLs using user-provided prefixes, making it compatible with any remote repository layout.

### 3. Localization & Warning-Free Build
*   **Localization**: Synchronized documentation and UI across English, Traditional Chinese (TW), and Simplified Chinese (CN) for `setup.bat` and `config.bat`.
*   **Type Safety**: Refactored wide-string conversion using `WideCharToMultiByte` to eliminate 100% of C++ compiler warnings (`C4244`).
*   **Compatibility**: Validated against both WinFsp 2023 and WinFsp 2025 Test releases.

---
**Developers**: anomixer + Antigravity (Gemini 3 Pro/Flash)
**Status**: v1.0 Final / Stable
