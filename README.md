<p align="center">
  <img src="https://raw.githubusercontent.com/anomixer/MameCloudRom/website/mamecloudrom_nobg-t.png" width="400" alt="MameCloudRom Logo">
</p>

# MameCloudRom (MCR)

[繁體中文版](./README-TW.md) | [简体中文版](./README-CN.md)

[Official Website](https://anomixer.github.io/MameCloudRom/index.html)

> ⚠ **This project does NOT include or distribute ROMs. It is a client-side file system tool. All content sources, downloading activities, and their legality are determined solely by the user, who assumes full responsibility.**


> [!IMPORTANT]
> **LEGAL & DMCA POLICY**
> This repository is a technical research project for virtual file system (VFS) implementation.
> 1. **No Copyrighted Content**: This repo does **NOT** host or distribute any game ROMs, BIOS files, or proprietary binaries.
> 2. **Client-Side Tool**: MCR is a "utility pipe" that fetches data from user-provided endpoints. The developer does not provide or maintain any ROM libraries.
> 3. **Open Source**: This code is provided under the MIT License as open-source software for technical research and legal use purposes.


MameCloudRom (MCR) is a Windows virtual file system (using WinFsp) designed for MAME ROM management. It implements a **Lazy Download** mechanism: when MAME requests a ROM that doesn't exist locally, MCR automatically downloads it from a configured remote server and serves it seamlessly.

## Motivation

MAME updates almost monthly, and with each update, ROM filenames and contents can change. This creates a vicious cycle where players are forced to download tens of gigabytes of update packages just to keep their sets compatible with the latest version. There is nothing more frustrating than wanting to play a quick game like `pacman`, only to find it won't start due to a version mismatch in your ROM set.

Most players only frequently play a small handful of games. Some third-party archival sites maintain up-to-date MAME-to-ROM mappings. MCR was born from this frustration: **"The proxy only fetches the correct ROM when you actually want to play the game."**

It's designed to solve the "ROM Version Hell" faced by 95% of players. Stop managing files and start playing. The primary objective of this project is to research and validate file system proxying and lazy-fetching mechanisms, rather than to provide or promote access to any specific content.

## Key Features (v1.0)

*   **On-Demand Download**: Files are only downloaded when accessed, saving local disk space.
*   **WinFsp Integration**: High-performance virtual disk driver with optimized **Stateless** mode.
*   **Stateless Stability**: Solved common errors like "Too many links" by enforcing stable metadata (IndexNumber=0).
*   **Explorer Friendly**: Fully compatible with Windows File Explorer (8-byte alignment fix), allowing you to browse/manage ROMs as if they were local without crashes.
*   **Scan Optimization**: Distinguishes between MAME auditing and game launching. Auditing (Metadata check) doesn't trigger downloads, making MAME launch near-instant.
*   **Extreme Stability**: Employs the optimized handle protection logic from v0.2, ensuring 100% stability even with WinFsp 2025 and complex File Explorer operations.
*   **Custom Path Prefixes**: Supports `-zp` (Zip) and `-7p` (7z) prefixes to adapt to any archival server structure.
*   **Detailed Downloader**: Visual real-time progress bars via `curl` and file size (MB/KB) display.
*   **Auto-Config**: Easy setup via `setup.bat` and configuration via `config.bat`.

## Prerequisites

1.  **Windows 10/11**
2.  **WinFsp** (Windows File System Proxy) - **Core Component**
    *   **Automatic Install**: Run `setup.bat` to install automatically.
    *   **Manual Install**: [WinFsp Version 2023/2025](https://winfsp.dev/) (Ensure "Core" is selected; developers should also select "Developer" components).
3.  **curl**: Built-in to Windows 10/11 (used for download progress display).
4.  **Visual Studio 2022** (Optional): Only required for developers building from source.
5.  **MAME**: The emulator itself. [Download Official Release](https://www.mamedev.org/release.php).
6.  **Third-party ROM URL**: A valid web source providing ROM files (e.g., `https://YourRomSite.com/download/`).

## Quick Setup (Recommended)

1.  Run `setup.bat` for the first-time installation. (**To change settings later, simply run `config.bat` directly**).
2.  Follow the prompts to set your cache directory, drive letter, MAME executable, and URL prefixes.
3.  The script will generate `mcr-run.bat`.
4.  Run `mcr-run.bat`. It will start the virtual drive AND automatically launch MAME for you in a new window.

## Build Instructions (Developers)

If you wish to compile the project manually:

1.  Install WinFsp with **Developer** components.
2.  Open the project folder in Visual Studio 2022.
3.  VS will automatically configure CMake.
4.  Select the `Release` configuration and "Build All".
5.  Alternatively, run `build.bat` from the root directory.

> [!TIP]
> **Pre-built binaries**: For users without Visual Studio, the project folder already includes a pre-built `mcr.exe`. You can skip the build step and go straight to **Quick Setup**.

## Usage

Start the program from the command line:

```cmd
mcr.exe [-m <MountPoint>] [-c <CacheDir>] [-u <BaseUrl>] [-zp <ZipPrefix>] [-7p <7zPrefix>] [-7z] [-d]
```

*   **No Args**: If you run `mcr.exe` without arguments, it will automatically load settings from `mcr.ini` (produced by `config.bat`).
*   `-zp`: Zip folder path prefix (default: `split/`).
*   `-7p`: 7z folder path prefix (default: `standalone/`).
*   `-7z`: Enable .7z file support.
*   `-d`: Enable debug mode (detailed console logs).

## How it Works

MameCloudRom acts as an intelligent intermediary. Here is the sequence of events during a game startup:

1.  **Interception**: When MAME searches for a ROM in its `-rompath Z:\`, it sends standard file requests to Windows.
2.  **WinFsp Hand-off**: WinFsp intercepts these requests and passes them to the user-mode MCR application.
3.  **Stateless Cache Check**: MCR checks your local cache directory.
    *   **Cache Hit**: Served immediately from disk.
    *   **Cache Miss**: MCR proceeds to download.
4.  **On-the-Fly Download**: MCR constructs the correct URL and triggers `curl` to fetch the file.
5.  **Seamless Delivery**: Once completed, MAME continues to load the game as if the file had always been there.

## File Structure

- **mcr.exe**: The main application and virtual drive service.
- **setup.bat**: The main installer. Checks requirements (WinFsp), fetches DLLs, and multi-language support.
- **config.bat**: Interactive configuration utility (called by setup.bat).
- **winfsp-x64.dll**: Essential library (automatically fetched from your system by `setup.bat`).
- **mcr-run.bat**: (Generated) The main launcher that starts the virtual drive and MAME simultaneously.
- **mcr.ini**: (Generated) Stores your cache path, drive letter, and prefixes.
- **src/**: Source code directory (C++).
- **build.bat**: Build script for developers (compiles and copies binaries).

## Demo Video

https://github.com/anomixer/MameCloudRom/raw/main/mcr-demo_h264NV.mp4

## Supported Scope & Limitations

*   **Supported**: Game ROMs (`.zip`, `.7z`) and various device/BIOS files requested via `rompath`.
*   **Unsupported**:
    *   **Large CHD Files**: Recommended to manage CHD files manually due to size.
    *   **Auxiliary Assets**: Snaps, icons, samples are best managed manually.
*   **File Deletion**: Deleting files on the virtual drive (`Z:\`) is **ineffective**. To delete a downloaded ROM, you must manually delete it from your local Cache directory.

## Version History

### v1.0 (2026-01-13)
**Focus: Production Maturity & User Flexibility**

*   **Extreme Stability**: Re-implemented v0.2 handle leasing for 100% stable File Explorer browsing.
*   **Custom Prefix Logic**: Added `-zp` and `-7p` to support any remote directory structure.
*   **Multi-Language UI**: Fully localized `setup.bat` and `config.bat` (EN/TW/CN).
*   **Warning-Free Build**: Clean C++ compilation with strict type safety.
*   **Legacy Support**: Compatible with WinFsp 2023 and the new 2025 test version.

### v0.2 (2026-01-11)
**Focus: Stability Baseline**

*   **Handle Leasing**: Prevented crashes during complex WinFsp/MAME interactions.
*   **Stateless Focus**: Fixed "Too many links" errors via metadata stabilization.

---

## Disclaimer & Legal

MameCloudRom (MCR) is a technical proof-of-concept for the **WinFsp file system proxy architecture**. MCR is content-agnostic and operates purely as a file system proxy; it does not distinguish between copyrighted or non-copyrighted content.

*   **No ROMs Included**: This repository contains **source code only**. No copyrighted ROMs or BIOS files are distributed with this software.
*   **External Content**: Any example URL referenced in configuration or documentation is provided strictly for technical demonstration purposes only. MCR does not require, recommend, or endorse any specific third-party content source.
*   **Copyright Compliance**: Users are responsible for ensuring they have the legal right to access any files they download.
*   **Use at Your Own Risk**: The authors accept no liability for any legal issues or data loss. Distributed under the [MIT License](./LICENSE).


Repository: [GitHub - anomixer/MameCloudRom](https://github.com/anomixer/MameCloudRom)
License: [MIT](./LICENSE)
