@echo off
chcp 65001 >nul
setlocal EnableDelayedExpansion

:: --- Language Detection ---
set "UI_LANG=EN"
for /f "tokens=3" %%a in ('reg query "HKEY_CURRENT_USER\Control Panel\International" /v "LocaleName" 2^>nul') do set "LOCALE_NAME=%%a"
if /i "!LOCALE_NAME:~0,5!"=="zh-TW" set "UI_LANG=TW"
if /i "!LOCALE_NAME:~0,5!"=="zh-HK" set "UI_LANG=TW"
if /i "!LOCALE_NAME:~0,5!"=="zh-MO" set "UI_LANG=TW"
if /i "!LOCALE_NAME:~0,5!"=="zh-CN" set "UI_LANG=CN"
if /i "!LOCALE_NAME:~0,5!"=="zh-SG" set "UI_LANG=CN"

:: Set UI strings and CodePage
if "!UI_LANG!"=="TW" (
    set "STR_TITLE=這個程式將協助您設定 MameCloudRom (MCR) 環境。"
    set "STR_CONFIRM=是否繼續 (y/n)? "
    set "STR_CHECK_WINFSP=[1/4] 正在檢查 WinFsp..."
    set "STR_WINFSP_NOT_FOUND=- 找不到 WinFsp。正在透過 Winget 安裝..."
    set "STR_WINFSP_ADMIN=- 可能需要管理員權限..."
    set "STR_WINFSP_WAIT=- 當您看到 WinFsp 安裝完成後，請按任意鍵繼續..."
    set "STR_WINFSP_ERR=[錯誤] 安裝後仍找不到 WinFsp 服務。"
    set "STR_WINFSP_MANUAL=請從此處下載並手動安裝: https://winfsp.dev/"
    set "STR_WINFSP_CORE=(請記得勾選 "Core" 組件)"
    set "STR_WINFSP_ALREADY=- WinFsp 已安裝。"
    set "STR_WINFSP_SUCCESS=- WinFsp 安裝成功。"
    set "STR_SYNC_DLL=[2/4] 正在同步 WinFsp DLL..."
    set "STR_DLL_FOUND=- 找到系統 WinFsp DLL 放於: "
    set "STR_DLL_ERR=[警告] 複製 DLL 失敗。檔案可能正在被使用或需要管理員權限。"
    set "STR_DLL_SUCCESS=- DLL 更新成功。"
    set "STR_DLL_MISSING=[警告] 在預設位置找不到 WinFsp DLL。跳過同步 (如果缺少 DLL，MCR 可能無法執行)。"
    set "STR_CONFIG=[3/4] 正在配制 MCR..."
    set "STR_CONFIG_ERR=[錯誤] 找不到 config.bat！"
    set "STR_FINISH=[4/4] 設定完成！"
    set "STR_RUN_NOW=您現在可以執行 "mcr-run.bat" 開始遊戲！"
    set "STR_WINGET_MISSING=[提示] 找不到 winget 指令。"
) else if "!UI_LANG!"=="CN" (
    set "STR_TITLE=这个程序将协助您设置 MameCloudRom (MCR) 环境。"
    set "STR_CONFIRM=是否继续 (y/n)? "
    set "STR_CHECK_WINFSP=[1/4] 正在检查 WinFsp..."
    set "STR_WINFSP_NOT_FOUND=- 找不到 WinFsp。正在通过 Winget 安装..."
    set "STR_WINFSP_ADMIN=- 可能需要管理员权限..."
    set "STR_WINFSP_WAIT=- 当您看到 WinFsp 安装完成后，请按任意键继续..."
    set "STR_WINFSP_ERR=[错误] 安装后仍找不到 WinFsp 服务。"
    set "STR_WINFSP_MANUAL=请从此处下载并手动安装: https://winfsp.dev/"
    set "STR_WINFSP_CORE=(请记得勾选 "Core" 组件)"
    set "STR_WINFSP_ALREADY=- WinFsp 已安装。"
    set "STR_WINFSP_SUCCESS=- WinFsp 安装成功。"
    set "STR_SYNC_DLL=[2/4] 正在同步 WinFsp DLL..."
    set "STR_DLL_FOUND=- 找到系统 WinFsp DLL 位于: "
    set "STR_DLL_ERR=[警告] 复制 DLL 失败。文件可能正在被使用或需要管理员权限。"
    set "STR_DLL_SUCCESS=- DLL 更新成功。"
    set "STR_DLL_MISSING=[警告] 在默认位置找不到 WinFsp DLL。跳过同步 (如果缺少 DLL，MCR 可能无法运行)。"
    set "STR_CONFIG=[3/4] 正在配置 MCR..."
    set "STR_CONFIG_ERR=[错误] 找不到 config.bat！"
    set "STR_FINISH=[4/4] 设置完成！"
    set "STR_RUN_NOW=您现在可以运行 "mcr-run.bat" 开始游戏！"
    set "STR_WINGET_MISSING=[提示] 找不到 winget 命令。"
) else (
    :: Default English
    set "STR_TITLE=This program will setup MameCloudRom (MCR) environment."
    set "STR_CONFIRM=Continue (y/n)? "
    set "STR_CHECK_WINFSP=[1/4] Checking WinFsp..."
    set "STR_WINFSP_NOT_FOUND=- WinFsp not found. Installing via Winget..."
    set "STR_WINFSP_ADMIN=- Requesting administrative privileges may be required..."
    set "STR_WINFSP_WAIT=- When you see the WinFsp installation is finished, press any key to continue..."
    set "STR_WINFSP_ERR=[ERROR] WinFsp service not found after installation attempt."
    set "STR_WINFSP_MANUAL=Please manually install it from: https://winfsp.dev/"
    set "STR_WINFSP_CORE=(Remember to select "Core" components)"
    set "STR_WINFSP_ALREADY=- WinFsp is already installed."
    set "STR_WINFSP_SUCCESS=- WinFsp installed successfully."
    set "STR_SYNC_DLL=[2/4] Syncing WinFsp DLL..."
    set "STR_DLL_FOUND=- Found system WinFsp DLL at: "
    set "STR_DLL_ERR=[WARNING] Failed to copy DLL. It might be in use or require Admin rights."
    set "STR_DLL_SUCCESS=- DLL updated successfully."
    set "STR_DLL_MISSING=[WARNING] Could not find WinFsp DLL at default location. Skipping DLL sync."
    set "STR_CONFIG=[3/4] Configuring MCR..."
    set "STR_CONFIG_ERR=[ERROR] config.bat not found!"
    set "STR_FINISH=[4/4] Setup Completed!"
    set "STR_RUN_NOW=You can now run "mcr-run.bat" to play!"
    set "STR_WINGET_MISSING=[INFO] winget command not found."
)

:banner
cls
echo    __  ___               _______             _____           
echo   /  ^|/  /__ ___ _  ___ / ___/ /__  __ _____/ / _ \___  __ _ 
echo  / /^|^_/ / _ `/  ' \/ -_) /__/ / _ \/ // / _  / , _/ _ \/  ' \
echo /_/  /_/\_,_/_/_/_/\__/\___/_/\___/\_,_/\_,_/_/^|^_^|\___/_/_/_/
echo.                                                             
echo                                                  by anomixer
echo.
echo ================================================================
echo !STR_TITLE!
echo ================================================================
echo.

:confirm
set /p "ans=!STR_CONFIRM!"
if /i "!ans!"=="y" goto check_winfsp_start
if /i "!ans!"=="n" exit /b
goto confirm

:check_winfsp_start
echo.
echo !STR_CHECK_WINFSP!
sc query WinFsp.Launcher >nul 2>&1
if %errorlevel% equ 0 goto check_winfsp_found

REM WinFsp not found, try install
where winget >nul 2>&1
if %errorlevel% neq 0 (
    echo !STR_WINGET_MISSING!
    echo !STR_WINFSP_MANUAL!
    echo !STR_WINFSP_CORE!
    pause
    exit /b
)

echo !STR_WINFSP_NOT_FOUND!
echo !STR_WINFSP_ADMIN!
winget install winfsp.winfsp --accept-source-agreements --accept-package-agreements

echo.
echo !STR_WINFSP_WAIT!
pause >nul

REM Verify installation again
sc query WinFsp.Launcher >nul 2>&1
if %errorlevel% equ 0 goto check_winfsp_installed

REM Install failed
echo.
echo !STR_WINFSP_ERR!
echo !STR_WINFSP_MANUAL!
echo !STR_WINFSP_CORE!
pause
exit /b

:check_winfsp_found
echo !STR_WINFSP_ALREADY!
goto copy_dll

:check_winfsp_installed
echo !STR_WINFSP_SUCCESS!
goto copy_dll

:copy_dll
echo.
echo !STR_SYNC_DLL!
set "WINFSP_DLL_SRC=C:\Program Files (x86)\WinFsp\bin\winfsp-x64.dll"

if not exist "%WINFSP_DLL_SRC%" goto dll_not_found

echo !STR_DLL_FOUND! "%WINFSP_DLL_SRC%"
copy /y "%WINFSP_DLL_SRC%" ".\winfsp-x64.dll" >nul
if %errorlevel% neq 0 (
    echo !STR_DLL_ERR!
) else (
    echo !STR_DLL_SUCCESS!
)
goto run_config

:dll_not_found
echo !STR_DLL_MISSING!
goto run_config

:run_config
echo.
echo !STR_CONFIG!
echo.
if exist config.bat (
    call config.bat
) else (
    echo !STR_CONFIG_ERR!
    pause
    exit /b
)

:finish
echo.
echo ================================================================
echo !STR_FINISH!
echo ================================================================
echo.
echo !STR_RUN_NOW!
echo.
pause
